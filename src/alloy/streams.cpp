#include "streams.hpp"
#include <algorithm>
#include <cstring>

namespace alloy {

void ArrayBufferSink::start(bool asUint8Array, size_t highWaterMark, bool stream) {
    m_asUint8Array = asUint8Array;
    m_highWaterMark = highWaterMark;
    m_stream = stream;
    m_ended = false;
    m_buffer.clear();
    if (m_highWaterMark > 0) {
        m_buffer.reserve(m_highWaterMark);
    }
}

size_t ArrayBufferSink::write(const uint8_t* data, size_t len) {
    if (m_ended) return 0;
    m_buffer.insert(m_buffer.end(), data, data + len);
    return len;
}

std::vector<uint8_t> ArrayBufferSink::flush() {
    if (!m_stream) {
        // If not in stream mode, flush might just return current size or similar
        // depending on JS implementation. Based on requirements:
        // "returns the number of bytes written since the last flush"
        return {};
    }
    std::vector<uint8_t> result = std::move(m_buffer);
    m_buffer.clear();
    if (m_highWaterMark > 0) {
        m_buffer.reserve(m_highWaterMark);
    }
    return result;
}

std::vector<uint8_t> ArrayBufferSink::end() {
    m_ended = true;
    std::vector<uint8_t> result = std::move(m_buffer);
    m_buffer.clear();
    return result;
}

// Internal mapping of C++ instance to JS object would go here in a real implementation.
// For now, these remain as placeholders for the binding logic.

JSValue ArrayBufferSink::js_constructor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Allocation of native ArrayBufferSink and associating with JS object
    return JS_UNDEFINED;
}

JSValue ArrayBufferSink::js_start(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Parse options { asUint8Array, highWaterMark, stream }
    return JS_UNDEFINED;
}

JSValue ArrayBufferSink::js_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Convert JS chunk to uint8_t* and call write()
    return JS_NewInt32(ctx, 0);
}

JSValue ArrayBufferSink::js_flush(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Call flush() and return ArrayBuffer or Uint8Array
    return JS_UNDEFINED;
}

JSValue ArrayBufferSink::js_end(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Call end() and return ArrayBuffer or Uint8Array
    return JS_UNDEFINED;
}

} // namespace alloy
