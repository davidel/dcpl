#pragma once

#include <cstdint>
#include <functional>

namespace dcpl {

template <typename T>
std::size_t hash_combine(std::size_t seed, const T& v) {
  std::hash<T> hasher;

  return seed ^ (hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}

