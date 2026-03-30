#ifndef ALLOY_DETAIL_SIGNAL_HH
#define ALLOY_DETAIL_SIGNAL_HH

#include "../api.h"
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <algorithm>
#include <memory>

namespace alloy::detail {

struct signal_value {
    std::variant<std::string, double, int, bool> data;

    signal_value() : data(false) {}
    explicit signal_value(std::string v) : data(std::move(v)) {}
    explicit signal_value(double v) : data(v) {}
    explicit signal_value(int v) : data(v) {}
    explicit signal_value(bool v) : data(v) {}
};

class signal_base {
public:
    virtual ~signal_base() = default;
    virtual const signal_value& get_value() = 0;

    void subscribe(void* subscriber, alloy_prop_id_t prop, std::function<void(const signal_value&)> cb) {
        m_subscribers.push_back({subscriber, prop, cb});
    }

    void unsubscribe(void* subscriber, alloy_prop_id_t prop) {
        m_subscribers.erase(std::remove_if(m_subscribers.begin(), m_subscribers.end(),
            [=](const auto& s) { return s.subscriber == subscriber && s.prop == prop; }),
            m_subscribers.end());
    }

protected:
    void notify() {
        const auto& val = get_value();
        for (const auto& s : m_subscribers) {
            s.callback(val);
        }
    }

    struct subscriber_info {
        void* subscriber;
        alloy_prop_id_t prop;
        std::function<void(const signal_value&)> callback;
    };
    std::vector<subscriber_info> m_subscribers;
};

class signal : public signal_base {
public:
    explicit signal(signal_value initial) : m_value(std::move(initial)) {}

    const signal_value& get_value() override { return m_value; }

    void set_value(signal_value v) {
        m_value = std::move(v);
        notify();
    }

private:
    signal_value m_value;
};

class computed : public signal_base {
public:
    using compute_fn = std::function<void(const std::vector<signal_base*>&, signal_value&, void*)>;

    computed(std::vector<signal_base*> deps, compute_fn fn, void* userdata)
        : m_deps(std::move(deps)), m_fn(fn), m_userdata(userdata) {
        for (auto* dep : m_deps) {
            dep->subscribe(this, (alloy_prop_id_t)-1, [this](const signal_value&) {
                recompute();
            });
        }
        recompute();
    }

    const signal_value& get_value() override { return m_value; }

private:
    void recompute() {
        m_fn(m_deps, m_value, m_userdata);
        notify();
    }

    std::vector<signal_base*> m_deps;
    compute_fn m_fn;
    void* m_userdata;
    signal_value m_value;
};

class effect {
public:
    using run_fn = std::function<void(void*)>;

    effect(std::vector<signal_base*> deps, run_fn fn, void* userdata)
        : m_deps(std::move(deps)), m_fn(fn), m_userdata(userdata) {
        for (auto* dep : m_deps) {
            dep->subscribe(this, (alloy_prop_id_t)-1, [this](const signal_value&) {
                m_fn(m_userdata);
            });
        }
        m_fn(m_userdata);
    }

private:
    std::vector<signal_base*> m_deps;
    run_fn m_fn;
    void* m_userdata;
};

} // namespace alloy::detail

#endif // ALLOY_DETAIL_SIGNAL_HH
