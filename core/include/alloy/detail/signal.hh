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
