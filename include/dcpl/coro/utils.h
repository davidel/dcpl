#pragma once

#include <type_traits>

#include "dcpl/coro/coro.h"
#include "dcpl/threadpool.h"

namespace dcpl::coro {

template <typename T = no_value>
value_base<T>* value_base_ptr(std::coroutine_handle<> coro) {
  auto handle = std::coroutine_handle<coro_promise<T>>::from_address(coro.address());

  return &handle.promise();
}

template <typename T>
const T& get_value(std::coroutine_handle<> coro) {
  return value_base_ptr<T>(coro)->value();
}

template <typename T>
T extract_value(std::coroutine_handle<> coro) {
  return value_base_ptr<T>(coro)->extract_value();
}

inline void spawn(std::coroutine_handle<> coro) {
  dcpl::threadpool::get()->push_work([coro]() { coro.resume(); });
}

inline auto schedule() {
  struct awaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept { }
    void await_suspend(std::coroutine_handle<> coro) const noexcept {
      spawn(coro);
    }
  };

  return awaiter{};
}

// Hack to get current coroutine handle. Use as:
//
//   auto handle = co_await get_coro_handle();
//
inline auto get_coro_handle() {
  struct awaitable {
    std::coroutine_handle<> handle;

    constexpr bool await_ready() { return false; }

    bool await_suspend(std::coroutine_handle<> hcoro) {
      handle = hcoro;
      return false;
    }

    constexpr std::coroutine_handle<> await_resume() { return handle; }
  };

  return awaitable{};
}

}
