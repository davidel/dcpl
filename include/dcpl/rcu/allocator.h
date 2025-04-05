#pragma once

#include <limits>
#include <new>
#include <type_traits>

#include "dcpl/rcu/rcu.h"

namespace dcpl::rcu {

// NOTE: Using this allocator fed into stdc++ classes does not make them RCU safe!
template<typename T = char>
struct allocator {
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;

  allocator() = default;

  template<typename U>
  constexpr allocator(const allocator<U>&) noexcept {}

  [[nodiscard]] T* allocate(size_type n) {
    if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
      throw std::bad_array_new_length();
    }

    T* ptr = static_cast<T*>(operator new(n * sizeof(T)));

    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return ptr;
  }

  void deallocate(T* p, size_type n) noexcept {
    mem_delete(p);
  }
};

template <typename T, typename U>
bool operator==(const allocator<T>&, const allocator<U>&) {
  return true;
}

template <typename T, typename U>
bool operator!=(const allocator<T>&, const allocator<U>&) {
  return false;
}

}

