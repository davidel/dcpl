#pragma once

#include "dcpl/coro/coro.h"
#include "dcpl/coro/mutex.h"

namespace dcpl::coro {

class event : public wait_base {
 public:
  explicit event(std::size_t trigger = 1) : trigger_(trigger) { }

  [[nodiscard]] auto wait() {
    struct awaiter {
      event* ev;

      constexpr bool await_ready() const noexcept { return false; }
      constexpr void await_resume() const noexcept { }
      bool await_suspend(std::coroutine_handle<> coro) const noexcept {
        return ev->do_wait(coro);
      }
    };

    return awaiter{this};
  }

  void set(std::size_t count = 1);

  void clear();

 private:
  bool do_wait(std::coroutine_handle<> coro);

  std::size_t trigger_;
  std::size_t count_ = 0;
};

}
