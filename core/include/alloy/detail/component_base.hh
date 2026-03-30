#ifndef ALLOY_DETAIL_COMPONENT_BASE_HH
#define ALLOY_DETAIL_COMPONENT_BASE_HH

#include "../api.h"
#include "signal.hh"
#include <yoga/Yoga.h>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace alloy::detail {

struct event_slot {
  alloy_event_cb_t fn{};
  void            *userdata{};
};

class component_base {
public:
  virtual ~component_base() {
      if (m_yoga_node) YGNodeFree(m_yoga_node);
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

  void bind_property(alloy_prop_id_t prop, signal_base *sig) {
      if (!sig) return;
      m_bindings[prop] = sig;
      sig->subscribe(this, prop, [this, prop](const signal_value& val) {
          on_signal_changed(prop, val);
      });
      // Apply initial value
      on_signal_changed(prop, sig->get_value());
  }

  void unbind_property(alloy_prop_id_t prop) {
      auto it = m_bindings.find(prop);
      if (it != m_bindings.end()) {
          it->second->unsubscribe(this, prop);
          m_bindings.erase(it);
      }
  }

  void on_signal_changed(alloy_prop_id_t prop, const signal_value& val) {
      switch (prop) {
          case ALLOY_PROP_TEXT:
          case ALLOY_PROP_LABEL:
              if (auto* s = std::get_if<std::string>(&val.data)) set_text(*s);
              break;
          case ALLOY_PROP_CHECKED:
              if (auto* b = std::get_if<bool>(&val.data)) set_checked(*b);
              else if (auto* i = std::get_if<int>(&val.data)) set_checked(*i != 0);
              break;
          case ALLOY_PROP_VALUE:
              if (auto* d = std::get_if<double>(&val.data)) set_value(*d);
              else if (auto* i = std::get_if<int>(&val.data)) set_value((double)*i);
              break;
          case ALLOY_PROP_ENABLED:
              if (auto* b = std::get_if<bool>(&val.data)) set_enabled(*b);
              break;
          case ALLOY_PROP_VISIBLE:
              if (auto* b = std::get_if<bool>(&val.data)) set_visible(*b);
              break;
      }
  }

  bool is_container() const { return m_is_container; }

  void add_child(component_base* child) {
      m_children.push_back(child);
      YGNodeInsertChild(m_yoga_node, child->yoga_node(), YGNodeGetChildCount(m_yoga_node));
  }

  void apply_layout() {
      float left = YGNodeLayoutGetLeft(m_yoga_node);
      float top = YGNodeLayoutGetTop(m_yoga_node);
      float width = YGNodeLayoutGetWidth(m_yoga_node);
      float height = YGNodeLayoutGetHeight(m_yoga_node);

      apply_native_layout(left, top, width, height);

      for (auto* child : m_children) {
          child->apply_layout();
      }
  }

  virtual void apply_native_layout(float x, float y, float w, float h) = 0;

protected:
  explicit component_base(bool is_container = false)
      : m_is_container{is_container},
        m_yoga_node{YGNodeNew()} {}

  YGNodeRef m_yoga_node{};
  bool      m_is_container{};
  std::vector<component_base*> m_children;

private:
  std::unordered_map<alloy_event_type_t, event_slot> m_events;
  std::unordered_map<alloy_prop_id_t, signal_base *> m_bindings;
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_COMPONENT_BASE_HH
