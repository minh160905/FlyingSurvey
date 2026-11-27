#include "../src/common/frame.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct expected_frame {
    const uint8_t *payload;
    size_t length;
};

struct expected_stream {
    const struct expected_frame *frames;
    size_t count;
    size_t delivered;
};

static void check_frame(const uint8_t *payload, size_t length, void *context)
{
    struct expected_stream *stream = context;
    assert(stream->delivered < stream->count);
    const struct expected_frame *expected = &stream->frames[stream->delivered++];
    assert(length == expected->length);
    assert(memcmp(payload, expected->payload, length) == 0);
}

static void test_stream(void)
{
    const uint8_t wire[] = {0, 0, 0, 3, 'a', 0, 'b', 0, 0, 0, 1, 'c'};
    const struct expected_frame frames[] = {{wire + 4, 3}, {wire + 11, 1}};
    /* Includes every split inside a header/body, and both frames in one read. */
    for (size_t split = 0; split <= sizeof(wire); ++split) {
        struct frame_reader reader = {0};
        struct expected_stream stream = {frames, 2, 0};
        assert(frame_feed(&reader, wire, split, check_frame, &stream) == 0);
        size_t complete = (size_t)(split >= 7) + (size_t)(split == sizeof(wire));
        assert(stream.delivered == complete);
        assert(frame_feed(&reader, wire + split, sizeof(wire) - split,
                          check_frame, &stream) == 0);
        assert(stream.delivered == 2 && reader.used == 0);
    }
}

static void test_invalid_lengths(void)
{
    const uint8_t headers[][4] = {{0, 0, 0, 0}, {0, 0, 0x40, 1},
                                  {0xff, 0xff, 0xff, 0xff}};
    for (size_t i = 0; i < sizeof(headers) / sizeof(headers[0]); ++i) {
        struct frame_reader reader = {0};
        struct expected_stream stream = {0};
        assert(frame_feed(&reader, headers[i], 3, check_frame, &stream) == 0);
        assert(frame_feed(&reader, headers[i] + 3, 1, check_frame, &stream) == -1);
        assert(stream.delivered == 0);
    }
}

static void test_encoder_and_maximum(void)
{
    uint8_t payload[FRAME_MAX_PAYLOAD] = {1, 2, 3};
    payload[sizeof(payload) - 1] = 0xff;
    uint8_t wire[FRAME_HEADER_SIZE + FRAME_MAX_PAYLOAD];
    assert(frame_encode(wire, sizeof(wire), payload, 0) == 0);
    assert(frame_encode(wire, sizeof(wire), payload, sizeof(payload) + 1) == 0);
    assert(frame_encode(wire, sizeof(wire) - 1, payload, sizeof(payload)) == 0);
    assert(frame_encode(wire, sizeof(wire), payload, sizeof(payload)) == sizeof(wire));
    const uint8_t header[] = {0, 0, 0x40, 0};
    assert(memcmp(wire, header, sizeof(header)) == 0);
    struct frame_reader reader = {0};
    const struct expected_frame frame = {payload, sizeof(payload)};
    struct expected_stream stream = {&frame, 1, 0};
    for (size_t i = 0; i < sizeof(wire); ++i) {
        assert(frame_feed(&reader, wire + i, 1, check_frame, &stream) == 0);
    }
    assert(stream.delivered == 1 && reader.used == 0);
}

int main(void)
{
    test_stream();
    test_invalid_lengths();
    test_encoder_and_maximum();
    puts("PASS: split headers, split payloads, concatenated frames, length bounds");
    return 0;
}
