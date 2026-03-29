#ifndef ALLOY_RUNTIME_HPP
#define ALLOY_RUNTIME_HPP

#include "webview/webview.h"
#include <string>

namespace alloy {

class runtime {
public:
    static void init(webview::webview& w);
    static void stop();
};

} // namespace alloy

#endif // ALLOY_RUNTIME_HPP
