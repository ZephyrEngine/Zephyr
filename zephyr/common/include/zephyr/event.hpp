
#pragma once

#include <zephyr/integer.hpp>
#include <zephyr/panic.hpp>
#include <algorithm>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace zephyr {

  template<typename... Args>
  class Event {
    public:
      using SubID = u64;
      using Handler = std::function<void(Args...)>;

      struct Subscription {
        Subscription(SubID id, Handler handler) : id{id}, handler{handler} {}
        SubID id;
        Handler handler;
      };

      Event() = default;
      Event(const Event& other) = delete;

      Event& operator=(Event const& other) = delete;

      void Emit(Args&&... args) const {
        for(const Subscription& subscription : m_subscription_list) {
          subscription.handler(std::forward<Args>(args)...);
        }
      }

      SubID Subscribe(Handler handler) {
        return m_subscription_list.emplace_back(NextID(), std::move(handler)).id;
      }

      void Unsubscribe(SubID id) {
        const auto match = std::ranges::find_if(m_subscription_list, [id](const Subscription& subscription) { return subscription.id == id; });
        if(match != m_subscription_list.end()) {
          m_subscription_list.erase(match);
        }
      }

    private:
      SubID NextID() {
        if(m_next_id == std::numeric_limits<decltype(m_next_id)>::max()) [[unlikely]] {
          ZEPHYR_PANIC("Reached the maximum number of event subscriptions. What?");
        }
        return m_next_id++;
      }

      SubID m_next_id{0u};
      std::vector<Subscription> m_subscription_list{};
  };

  typedef Event<> VoidEvent;

} // namespace zephyr