#pragma once

#include <climits>
#include <cstddef>
#include <initializer_list>

namespace dcpl::consts {

static constexpr std::size_t all = static_cast<std::size_t>(-1);
static constexpr std::size_t invalid_index = static_cast<std::size_t>(-1);
static constexpr std::size_t invalid_size = static_cast<std::size_t>(-1);

template <typename T, typename S>
constexpr T load(const std::initializer_list<S> values) {
  constexpr std::size_t tbits = sizeof(T) * CHAR_BIT;
  constexpr std::size_t sbits = sizeof(S) * CHAR_BIT;
  T value = 0;
  std::size_t nbits = 0;

  for (const auto& svalue : values) {
    if (nbits >= tbits) {
      break;
    }
    value = (value << sbits) | svalue;
    nbits += sbits;
  }

  return value;
}

}

