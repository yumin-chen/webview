#ifndef ALLOY_FS_HPP
#define ALLOY_FS_HPP

#include <string>
#include <vector>
#include <fstream>
#include "../../core/mquickjs/mquickjs.h"

namespace alloy {

class FS {
public:
    static std::vector<uint8_t> read_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (file.read((char*)buffer.data(), size)) {
            return buffer;
        }
        return {};
    }

    static JSValue js_read_file(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
        // Implementation that calls read_file and returns an ArrayBuffer
        return JS_UNDEFINED;
    }
};

} // namespace alloy

#endif
