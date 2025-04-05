#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

#include "dcpl/rcu/rcu.h"

namespace dcpl::rcu {

template <typename T>
struct deleter {
  void operator()(T* ptr) const {
    free_object<T>(ptr);
  }
};

template <typename T>
struct deleter<T[]> {
  void operator()(T* ptr) const {
    free_array<T>(ptr);
  }
};

template <typename T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;

template<typename T, typename... ARGS>
unique_ptr<T> make_unique(ARGS&&... args) {
  return unique_ptr<T>(new T(std::forward<ARGS>(args)...));
}

template <typename T>
std::enable_if_t<std::is_unbounded_array_v<T>, unique_ptr<T>>
make_unique(std::size_t size) {
  return unique_ptr<T>(new std::remove_extent_t<T>[size]());
}

template<typename T, typename... ARGS>
std::shared_ptr<T> make_shared(ARGS&&... args) {
  return std::shared_ptr<T>(new T(std::forward<ARGS>(args)...), deleter<T>());
}

template<typename T>
std::shared_ptr<T> create_shared(std::remove_extent_t<T>* ptr) {
  return std::shared_ptr<T>(ptr, deleter<T>());
}

}

