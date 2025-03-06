#pragma once

#include <any>
#include <map>
#include <unordered_map>
#include <utility>

namespace dcpl {

struct any {
  any() = default;

  template <typename T>
  any(T cv) : v(std::move(cv)) { }

  template <typename U>
  operator U() const {
    return std::any_cast<U>(v);
  }

  // Spaceships do not seem to be working here ...
  template <typename U>
  bool operator==(const U& rhs) const {
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
    return std::any_cast<U>(v);
  }

  std::any v;
};

template <typename K, typename... ARGS>
using any_map = std::map<K, any, ARGS...>;

template <typename K, typename... ARGS>
using any_umap = std::unordered_map<K, any, ARGS...>;

}

