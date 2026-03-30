#ifndef ALLOY_COMPONENT_BASE_HH
#define ALLOY_COMPONENT_BASE_HH

#include "../api.h"
#include <string_view>
#include <unordered_map>
#include <string>

namespace alloy::detail {

struct event_slot {
  alloy_event_cb_t fn{};
  void            *userdata{};
};

class component_base {
public:
  virtual ~component_base() = default;

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

  virtual void *native_handle() = 0;

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

  struct layout_props {
      float flex = 0.0f;
      float width = -1.0f; // -1 for auto
      float height = -1.0f;
      float padding[4] = {0,0,0,0}; // t, r, b, l
      float margin[4] = {0,0,0,0};
  };

  layout_props& layout() { return m_layout; }

protected:
  explicit component_base(bool is_container = false)
      : m_is_container{is_container} {}

  bool m_is_container{};
  layout_props m_layout;

private:
  std::unordered_map<alloy_event_type_t, event_slot> m_events;
};

} // namespace alloy::detail

#endif
