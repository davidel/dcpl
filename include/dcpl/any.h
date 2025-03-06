#pragma once

#include <any>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace dcpl {

class any {
 public:
  any() = default;

  template <typename T>
  any(T cv) : value_(std::move(cv)) { }

  template <typename T>
  any& operator=(T cv) {
    value_ = std::move(cv);

    return *this;
  }

  template <typename U>
  operator U() const {
    return std::any_cast<U>(value_);
  }

  // Spaceships do not seem to be working here ...
  template <typename U>
  auto operator==(const U& rhs) const {
    return cast<U>() == rhs;
  }

  template <typename U>
  bool operator!=(const U& rhs) const {
    return cast<U>() != rhs;
  }

  template <typename U>
  bool operator<=(const U& rhs) const {
    return cast<U>() <= rhs;
  }

  template <typename U>
  bool operator>=(const U& rhs) const {
    return cast<U>() >= rhs;
  }

  template <typename U>
  bool operator<(const U& rhs) const {
    return cast<U>() < rhs;
  }

  template <typename U>
  bool operator>(const U& rhs) const {
    return cast<U>() > rhs;
  }

  template <typename U>
  U cast() const {
    return std::any_cast<U>(value_);
  }

  template <typename U>
  U* ptr_cast() const {
    return std::any_cast<std::remove_pointer_t<U>>(&value_);
  }

  bool has_value() const {
    return value_.has_value();
  }

 private:
  std::any value_;
};

template <typename K, typename... ARGS>
using any_map = std::map<K, any, ARGS...>;

template <typename K, typename... ARGS>
using any_umap = std::unordered_map<K, any, ARGS...>;

}

