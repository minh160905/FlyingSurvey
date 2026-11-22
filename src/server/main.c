#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

    int listener = socket(AF_INET, SOCK_STREAM, 0);
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

    for (;;) {
        pause();
    }
}
