#pragma once

#include <mutex>
#include <optional>
#include <queue>

#include "dcpl/coro/coro.h"

namespace dcpl::coro {

class wait_base {
 protected:
  std::mutex lock_;
  std::queue<std::coroutine_handle<>> coros_;
};

class mutex : public wait_base {
 public:
  [[nodiscard]] auto lock() {
    struct awaiter {
      mutex* mtx;

      constexpr bool await_ready() const noexcept { return false; }
      constexpr void await_resume() const noexcept { }
      bool await_suspend(std::coroutine_handle<> coro) const noexcept {
        return mtx->do_lock(coro);
      }
    };

    return awaiter{this};
  }

  [[nodiscard]] auto lock_guard() {
    struct guard {
      mutex* mtx;

      ~guard() {
        unlock();
      }

      void unlock() {
        if (mtx != nullptr) {
          mtx->unlock();
        }
        mtx = nullptr;
      }
    };
    struct awaiter {
      mutex* mtx;

      constexpr bool await_ready() const noexcept { return false; }

      [[nodiscard]] guard await_resume() const noexcept {
        return guard{mtx};
      }

      bool await_suspend(std::coroutine_handle<> coro) const noexcept {
        return mtx->do_lock(coro);
      }
    };

    return awaiter{this};
  }

  void unlock();

 private:
  bool do_lock(std::coroutine_handle<> coro);

  bool locked_ = false;
};

}
