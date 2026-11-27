#ifndef FLYING_SURVEY_FRAME_H
#define FLYING_SURVEY_FRAME_H

#include <stddef.h>
#include <stdint.h>

enum { FRAME_HEADER_SIZE = 4, FRAME_MAX_PAYLOAD = 16 * 1024 };

/* Zero-initialize one reader per connection. */
struct frame_reader {
    uint8_t bytes[FRAME_HEADER_SIZE + FRAME_MAX_PAYLOAD];
    size_t used;
};

/* Payload is valid only during the callback. */
typedef void (*frame_handler)(const uint8_t *payload, size_t length, void *context);

/* Returns the number of bytes written, or zero for invalid length/capacity. */
size_t frame_encode(uint8_t *output, size_t capacity,
                    const uint8_t *payload, size_t length);

/* Returns -1 on invalid wire length; discard this connection on error. */
int frame_feed(struct frame_reader *reader, const uint8_t *data, size_t count,
               frame_handler handle, void *context);

#endif
