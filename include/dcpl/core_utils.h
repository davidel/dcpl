#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

// Dependencies must be limited!
#include "dcpl/assert.h"

namespace dcpl {

namespace {

template <typename T>
using ref_pair = std::pair<std::reference_wrapper<const typename T::key_type>,
                           std::reference_wrapper<const typename T::mapped_type>>;

}

template <typename T, typename F>
std::vector<ref_pair<T>>
map_sort(const T& data, const F& cmp) {
  using mpair = ref_pair<T>;
  std::vector<mpair> sorted;

  sorted.reserve(data.size());
  for (const auto& it : data) {
    sorted.push_back(mpair{it.first, it.second});
  }

  std::sort(sorted.begin(), sorted.end(), cmp);

  return std::move(sorted);
}

template <typename T, typename C>
T map_add_values(const C& data) {
  return std::accumulate(data.begin(), data.end(), static_cast<double>(0),
                         [](T value, const auto& it) {
                           return value + it.second;
                         });
}

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

