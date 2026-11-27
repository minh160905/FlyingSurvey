#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../common/frame.h"

static void received_frame(const uint8_t *payload, size_t length, void *context)
{
    (void)payload;
    const int *fd = context;
    printf("Received frame on fd %d (%zu bytes)\n", *fd, length);
    fflush(stdout);
}

static int serve(int listener)
{
    enum { MAX_CLIENTS = 64 };
    struct pollfd sockets[MAX_CLIENTS + 1];
    struct frame_reader *readers = calloc(MAX_CLIENTS, sizeof(*readers));
    if (!readers) {
        perror("server: frame buffers");
        return EXIT_FAILURE;
    }
    sockets[0] = (struct pollfd){.fd = listener, .events = POLLIN};
    for (int i = 1; i <= MAX_CLIENTS; ++i) {
        sockets[i] = (struct pollfd){.fd = -1, .events = POLLIN};
    }

    for (;;) {
        if (poll(sockets, MAX_CLIENTS + 1, -1) < 0) {
            if (errno == EINTR) continue;
            perror("server: poll");
            break;
        }
        if (sockets[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "server: listener unavailable\n");
            break;
        }
        if (sockets[0].revents & POLLIN) {
            int fd = accept(listener, NULL, NULL);
            if (fd >= 0) {
                int slot = 1;
                while (slot <= MAX_CLIENTS && sockets[slot].fd >= 0) ++slot;
                if (slot <= MAX_CLIENTS) {
                    sockets[slot].fd = fd;
                    readers[slot - 1].used = 0;
                } else {
                    close(fd);
                }
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR &&
                       errno != ECONNABORTED) {
                perror("server: accept");
                break;
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; ++i) {
            if (!sockets[i].revents) continue;
            struct frame_reader *reader = &readers[i - 1];
            uint8_t chunk[4096];
            ssize_t n = recv(sockets[i].fd, chunk, sizeof(chunk), MSG_DONTWAIT);
            if (n > 0) {
                if (frame_feed(reader, chunk, (size_t)n, received_frame,
                               &sockets[i].fd) == 0) continue;
                fprintf(stderr, "server: invalid frame length on fd %d\n", sockets[i].fd);
            } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                continue;
            } else if (n == 0 && reader->used) {
                fprintf(stderr, "server: incomplete frame on fd %d\n", sockets[i].fd);
            }
            close(sockets[i].fd);
            sockets[i].fd = -1;
        }
    }
    for (int i = 1; i <= MAX_CLIENTS; ++i) {
        if (sockets[i].fd >= 0) close(sockets[i].fd);
    }
    free(readers);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    const struct option options[] = {
        {"host", required_argument, NULL, 'H'},
        {"port", required_argument, NULL, 'p'},
        {0, 0, NULL, 0},
    };
    const char *host = "127.0.0.1";
    const char *port_text = NULL;
    int option;

    while ((option = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (option) {
        case 'H':
            host = optarg;
            break;
        case 'p':
            port_text = optarg;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    if (port_text == NULL || optind != argc) {
        fprintf(stderr, "Usage: %s [--host IPv4] --port PORT\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *port_end;
    long port = strtol(port_text, &port_end, 10);
    if (port_text[0] < '0' || port_text[0] > '9' || *port_end != '\0' ||
        port < 1 || port > UINT16_MAX) {
        fprintf(stderr, "server: invalid port '%s' (expected 1-65535)\n", port_text);
        return EXIT_FAILURE;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons((uint16_t)port),
    };
    if (inet_pton(AF_INET, host, &address.sin_addr) != 1) {
        fprintf(stderr, "server: invalid IPv4 address '%s'\n", host);
        return EXIT_FAILURE;
    }

    int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener == -1) {
        perror("server: socket");
        return EXIT_FAILURE;
    }

    if (bind(listener, (const struct sockaddr *)&address, sizeof(address)) == -1) {
        fprintf(stderr, "server: bind %s:%ld: %s\n", host, port, strerror(errno));
        close(listener);
        return EXIT_FAILURE;
    }

    if (listen(listener, SOMAXCONN) == -1) {
        fprintf(stderr, "server: listen %s:%ld: %s\n", host, port, strerror(errno));
        close(listener);
        return EXIT_FAILURE;
    }

    printf("Listening on %s:%ld\n", host, port);
    fflush(stdout);

    int status = serve(listener);
    close(listener);
    return status;
}
