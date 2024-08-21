#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <type_traits>

namespace dcpl::coro {

class exception_base {
 public:
  void unhandled_exception() {
    ex_ptr_ = std::current_exception();
  }

  void check_exception() const {
    if (ex_ptr_ != nullptr) {
      std::rethrow_exception(ex_ptr_);
    }
  }

 private:
  std::exception_ptr ex_ptr_;
};

template <typename T>
class value_base : public exception_base {
 public:
  template <std::convertible_to<T> S>
  void return_value(S&& x) {
    value_ = std::move(x);
    has_value_ = true;
  }

  template <std::convertible_to<T> S>
  std::suspend_always yield_value(S&& x) {
    value_ = std::move(x);
    has_value_ = true;

    return {};
  }

  const T& value() const {
    check_exception();

    return value_;
  }

  T extract_value() {
    check_exception();
    has_value_ = false;

    return std::move(value_);
  }

  bool has_value() const {
    return has_value_;
  }

  void clear_value() {
    has_value_ = false;
    value_ = T{};
  }

 private:
  T value_;
  bool has_value_ = false;
};

struct no_value {};

template <>
class value_base<no_value> : public exception_base {
 public:
  void return_void() {}
};

template <typename T, typename IS, typename FS>
class coro;

template <typename T = no_value,
          typename IS = std::suspend_always,
          typename FS = std::suspend_always>
class coro_promise : public value_base<T> {
 public:
  coro<T, IS, FS> get_return_object() {
    return {std::coroutine_handle<coro_promise>::from_promise(*this)};
  }

  IS initial_suspend() noexcept {
    return {};
  }

  FS final_suspend() noexcept {
    return {};
  }
};

template <typename T = no_value,
          typename IS = std::suspend_always,
          typename FS = std::suspend_always>
class [[nodiscard]] coro {
 public:
  using promise_type = coro_promise<T, IS, FS>;

  coro(std::coroutine_handle<promise_type> coro) noexcept :
      coro_(coro) {
  }

  coro(coro&& other) noexcept :
      coro_(other.coro_) {
    other.coro_ = nullptr;
  }

  coro(const coro& other) = delete;

  ~coro() {
    destroy();
  }

  void destroy(bool force = false) {
    if (coro_ != nullptr) {
      // If the coroutine is not suspended when it returns, its state is automatically
      // destroyed so we must not repeat that here.
      // Coroutines without a final suspension should be used with care since the coro
      // object might end up referring to an invalid handle when the coroutine quits.
      if (force || std::is_same_v<FS, std::suspend_always>) {
        coro_.destroy();
      }
      coro_ = nullptr;
    }
  }

  coro& operator=(const coro& other) = delete;

  coro& operator=(coro&& other) {
    if (this != &other) {
      destroy();
      coro_ = other.coro_;
      other.coro_ = nullptr;
    }

    return *this;
  }

  promise_type& promise() {
    return coro_.promise();
  }

  const promise_type& promise() const {
    return coro_.promise();
  }

  const T& value() const {
    return promise().value();
  }

  T extract_value() {
    return promise().extract_value();
  }

  std::optional<T> next_value() {
    if (done()) {
      return std::nullopt;
    }

    promise_type& prom = promise();

    prom.clear_value();
    resume();
    if (!prom.has_value()) {
      return std::nullopt;
    }

    return prom.extract_value();
  }

  std::coroutine_handle<promise_type> release() {
    std::coroutine_handle<promise_type> coro = coro_;

    coro_ = nullptr;

    return coro;
  }

  bool done() const {
    return coro_ == nullptr || coro_.done();
  }

  operator bool() const {
    return !done();
  }

  void resume() const {
    coro_.resume();
  }

  void operator()() const {
    resume();
  }

  std::coroutine_handle<promise_type> handle() const {
    return coro_;
  }

  operator std::coroutine_handle<>() const {
    return handle();
  }

 private:
  std::coroutine_handle<promise_type> coro_;
};

template <typename T = no_value,
          typename FS = std::suspend_always>
using ns_coro = coro<T, std::suspend_never, FS>;

}
