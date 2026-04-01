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

#ifndef ALLOY_DETAIL_COMPONENT_BASE_HH
#define ALLOY_DETAIL_COMPONENT_BASE_HH

#include "../api.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Forward decl
namespace alloy::detail {
class signal_base;
struct signal_value;
}

#include <yoga/Yoga.h>

namespace alloy::detail {

struct event_slot {
  alloy_event_cb_t fn{};
  void            *userdata{};
};

class component_base {
public:
  virtual ~component_base() {
    if (m_yoga_node) {
      YGNodeFree(m_yoga_node);
    }
  }

  // Property setters — implemented by each backend subclass
  virtual alloy_error_t set_text(std::string_view text)    = 0;
  virtual alloy_error_t get_text(char *buf, size_t len)    = 0;
  virtual alloy_error_t set_checked(bool v)                = 0;
  virtual bool          get_checked()                      = 0;
  virtual alloy_error_t set_value(double v)                = 0;
  virtual double        get_value()                        = 0;
  virtual alloy_error_t set_enabled(bool v)                = 0;
  virtual bool          get_enabled()                      = 0;
  virtual alloy_error_t set_visible(bool v)                = 0;
  virtual bool          get_visible()                      = 0;
  virtual alloy_error_t set_style(const alloy_style_t &s)  = 0;

  // Native handle access (for layout, embedding)
  virtual void *native_handle() = 0;

  // Yoga layout node
  YGNodeRef yoga_node() const { return m_yoga_node; }

  // Event dispatch — called by backend on native event
  void fire_event(alloy_event_type_t ev) {
    auto it = m_events.find(ev);
    if (it != m_events.end() && it->second.fn) {
      it->second.fn(static_cast<alloy_component_t>(this), ev,
                    it->second.userdata);
    }
  }

  void set_event_callback(alloy_event_type_t ev,
                          alloy_event_cb_t fn, void *ud) {
    m_events[ev] = {fn, ud};
  }

  bool is_container() const { return m_is_container; }

  void add_child(component_base *child) {
    m_children.push_back(child);
    YGNodeInsertChild(m_yoga_node, child->yoga_node(),
                      YGNodeGetChildCount(m_yoga_node));
  }

  const std::vector<component_base *> &children() const { return m_children; }

protected:
  explicit component_base(bool is_container = false)
      : m_is_container{is_container},
        m_yoga_node{YGNodeNew()} {}

  YGNodeRef m_yoga_node{};
  bool      m_is_container{};
  std::vector<component_base *> m_children;

private:
  std::unordered_map<alloy_event_type_t, event_slot> m_events;
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_COMPONENT_BASE_HH
