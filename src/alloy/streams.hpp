#ifndef ALLOY_STREAMS_HPP
#define ALLOY_STREAMS_HPP

#include <vector>
#include <string>
#include <cstdint>
#include "../../core/mquickjs/mquickjs.h"

namespace alloy {

class ArrayBufferSink {
    std::vector<uint8_t> m_buffer;
    bool m_asUint8Array = false;
    bool m_stream = false;
    size_t m_highWaterMark = 0;
    bool m_ended = false;

public:
    void start(bool asUint8Array, size_t highWaterMark, bool stream);
    size_t write(const uint8_t* data, size_t len);
    std::vector<uint8_t> flush();
    std::vector<uint8_t> end();

    // JS wrappers
    static JSValue js_constructor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
    static JSValue js_start(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
    static JSValue js_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
    static JSValue js_flush(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
    static JSValue js_end(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
};

} // namespace alloy

#endif
