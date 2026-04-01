#ifndef ALLOY_BINDINGS_HPP
#define ALLOY_BINDINGS_HPP

#include "webview/webview.h"
#include "../../core/mquickjs/mquickjs.h"
#include <functional>
#include <string>
#include <vector>

namespace alloy {

typedef std::function<std::string(const std::string&)> BindingFn;

void bind_global(webview_t w, JSContext *ctx, const char *name, BindingFn fn);

} // namespace alloy

#endif
