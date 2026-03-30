#ifndef ALLOY_SIGNAL_HH
#define ALLOY_SIGNAL_HH

#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <algorithm>

namespace alloy::detail {

using signal_value = std::variant<int, double, std::string, bool>;

class component_base;

class signal_base {
public:
    virtual ~signal_base() = default;
    virtual signal_value get_value() const = 0;

    void subscribe(component_base* component, alloy_prop_id_t prop) {
        m_subscribers.push_back({component, prop});
    }

    void unsubscribe(component_base* component, alloy_prop_id_t prop) {
        m_subscribers.erase(
            std::remove_if(m_subscribers.begin(), m_subscribers.end(),
                [=](const auto& s) { return s.component == component && s.prop == prop; }),
            m_subscribers.end());
    }

protected:
    struct subscriber {
        component_base* component;
        alloy_prop_id_t prop;
    };
    std::vector<subscriber> m_subscribers;

    void notify();
};

} // namespace alloy::detail

#endif
