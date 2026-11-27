#include "frame.h"

#include <string.h>

size_t frame_encode(uint8_t *output, size_t capacity,
                    const uint8_t *payload, size_t length)
{
    if (length == 0 || length > FRAME_MAX_PAYLOAD ||
        capacity < FRAME_HEADER_SIZE + length) {
        return 0;
    }
    uint32_t size = (uint32_t)length;
    output[0] = (uint8_t)(size >> 24);
    output[1] = (uint8_t)(size >> 16);
    output[2] = (uint8_t)(size >> 8);
    output[3] = (uint8_t)size;
    memcpy(output + FRAME_HEADER_SIZE, payload, length);
    return FRAME_HEADER_SIZE + length;
}

int frame_feed(struct frame_reader *reader, const uint8_t *data, size_t count,
               frame_handler handle, void *context)
{
    while (count > 0) {
        if (reader->used < FRAME_HEADER_SIZE) {
            size_t take = FRAME_HEADER_SIZE - reader->used;
            if (take > count) take = count;
            memcpy(reader->bytes + reader->used, data, take);
            reader->used += take;
            data += take;
            count -= take;
            if (reader->used < FRAME_HEADER_SIZE) return 0;
        }

        const uint8_t *header = reader->bytes;
        uint32_t length = ((uint32_t)header[0] << 24) |
                          ((uint32_t)header[1] << 16) |
                          ((uint32_t)header[2] << 8) | header[3];
        if (length == 0 || length > FRAME_MAX_PAYLOAD) return -1;

        size_t remaining = FRAME_HEADER_SIZE + length - reader->used;
        size_t take = remaining < count ? remaining : count;
        memcpy(reader->bytes + reader->used, data, take);
        reader->used += take;
        data += take;
        count -= take;
        if (take == remaining) {
            handle(reader->bytes + FRAME_HEADER_SIZE, length, context);
            reader->used = 0;
        }
    }
    return 0;
}
