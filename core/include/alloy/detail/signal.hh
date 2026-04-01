/*
 * AlloyScript Runtime - CC0 Unlicense Public Domain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ALLOY_DETAIL_SIGNAL_HH
#define ALLOY_DETAIL_SIGNAL_HH

#include "../api.h"
#include <string>
#include <vector>
#include <variant>
#include <functional>

namespace alloy::detail {

struct signal_value {
  std::variant<std::string, double, int, bool> data;
};

class component_base;

class signal_base {
public:
  virtual ~signal_base() = default;

  struct subscription {
    component_base* component;
    alloy_prop_id_t prop;
  };

  void subscribe(component_base* c, alloy_prop_id_t p) {
    m_subscribers.push_back({c, p});
  }

  void unsubscribe(component_base* c, alloy_prop_id_t p) {
    // Basic implementation
  }

protected:
  void notify_subscribers(const signal_value& val);

private:
  std::vector<subscription> m_subscribers;
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_SIGNAL_HH
