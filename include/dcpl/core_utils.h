#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

// Dependencies must be limited!
#include "dcpl/assert.h"

namespace dcpl {

template<typename T, typename C>
std::vector<T> to_vector(const C& data) {
  return std::vector<T>(data.begin(), data.end());
}

template <typename T, typename C>
std::vector<T> to_vector_cast(const C& data) {
  std::vector<T> dest;

  dest.reserve(data.size());
  for (const auto& value : data) {
    dest.push_back(static_cast<T>(value));
  }

  return std::move(dest);
}

template <typename T, typename S>
std::span<T> reinterpret_span(std::span<S> source) {
  static_assert(sizeof(T) == sizeof(S), "Mismatching size");

  return std::span<T>{ reinterpret_cast<T*>(source.data()), source.size() };
}

template <typename T>
std::span<char> to_string(const T& value, std::span<char> buf,
                          std::enable_if_t<std::is_integral_v<T>, T>* = nullptr) {
  typename std::make_unsigned<T>::type uvalue;

  if constexpr (std::is_signed_v<T>) {
    uvalue = value >= 0 ? value : -value;
  } else {
    uvalue = value;
  }

  DCPL_ASSERT(!buf.empty());

  char* base = buf.data();
  char* ptr = base + buf.size();

  do {
    --ptr;
    *ptr = "0123456789"[uvalue % 10];
    uvalue /= 10;
  } while (uvalue != 0 && ptr > base);
  DCPL_ASSERT(uvalue == 0) << "Exceeded buffer size";

  if constexpr (std::is_signed_v<T>) {
    if (value < 0) {
      DCPL_CHECK_GT(ptr, base) << "Exceeded buffer size";
      --ptr;
      *ptr = '-';
    }
  }

  return { ptr, static_cast<std::size_t>(buf.size() - (ptr - base)) };
}

bool is_prime(std::size_t n);

std::size_t next_prime(std::size_t n);

}

