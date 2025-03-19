#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "dcpl/constants.h"
#include "dcpl/core_utils.h"
#include "dcpl/types.h"

namespace dcpl {

template <typename T>
std::size_t hash_combine(std::size_t seed, const T& v) {
  std::hash<T> hasher;

  return seed ^ (hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

template <typename T = std::size_t>
T hash_bytes(const void* ptr, std::size_t len, T seed) {
  constexpr std::size_t shift_bits = (3 * bit_sizeof<T>()) / 4 - 1;
  constexpr T mul = consts::load<T, std::uint32_t>({
      0xc6a4a793UL,
      0x5bd1e995UL,
      0xab0e9789UL,
      0x38b34ae5UL
    });

  const char* buf = static_cast<const char*>(ptr);
  const char* end = buf + len / sizeof(T);
  T hash = seed ^ (len * mul);

  for (const char* p = buf; p != end; p += sizeof(T)) {
    T data = vload<T>(p);

    data *= mul;
    data ^= data >> shift_bits;
    data *= mul;
    hash ^= data;
    hash *= mul;
  }

  dcpl::ssize_t left = len % sizeof(T);

  if (left != 0) {
    T data = 0;

    for (--left; left >= 0; --left) {
      data = (data << 8) | end[left];
    }

    hash ^= data;
    hash *= mul;
  }

  hash ^= hash >> shift_bits;
  hash *= mul;
  hash ^= hash >> shift_bits;

  return hash;
}

}

