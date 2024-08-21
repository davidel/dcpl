#pragma once

#include <functional>

#include "dcpl/coro/coro.h"
#include "dcpl/coro/mutex.h"

namespace dcpl::coro {

class condition_variable : public wait_base {
 public:
  [[nodiscard]] auto wait(mutex* mtx, std::function<bool()> wait_fn) {
    struct awaiter {
      condition_variable* cv;
      mutex* mtx;
      std::function<bool()> wait_fn;

      bool await_ready() const noexcept {
        return wait_fn();
      }

      constexpr void await_resume() const noexcept { }

      bool await_suspend(std::coroutine_handle<> coro) const noexcept {
        cv->do_wait(coro, mtx, wait_fn);
        return true;
      }
    };

    return awaiter{this, mtx, wait_fn};
  }

  void notify_one();

  void notify_all();

 private:
  [[nodiscard]] auto suspend() {
    struct awaiter {
      condition_variable* cv;

      constexpr bool await_ready() const noexcept { return false; }
      constexpr void await_resume() const noexcept { }
      bool await_suspend(std::coroutine_handle<> coro) const noexcept {
        cv->do_suspend(coro);
        return true;
      }
    };

    return awaiter{this};
  }

  coro<> waiter(std::coroutine_handle<> coro, mutex* mtx,
                std::function<bool()> wait_fn);

  void do_wait(std::coroutine_handle<> coro, mutex* mtx,
               std::function<bool()> wait_fn);

  void do_suspend(std::coroutine_handle<> coro);
};

}
