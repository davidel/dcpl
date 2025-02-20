#pragma once

#include <cstdint>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "dcpl/assert.h"
#include "dcpl/types.h"

namespace dcpl {

template <typename T>
std::span<std::uint8_t> varint_encode(T value, std::span<std::uint8_t> data) {
  static_assert(std::is_unsigned<T>::value, "Varint encoding requires unsigned type");

  std::uint8_t* ptr = data.data();
  std::uint8_t* top = ptr + data.size();
  T cvalue = value;
  constexpr T mask = static_cast<T>(0x7f);

  while (cvalue & ~mask) {
    DCPL_ASSERT(top > ptr + 1) << "Buffer overflow while encoding varint: value="
                               << value << " size=" << data.size();

    *ptr++ = 0x80 | static_cast<std::uint8_t>(cvalue & mask);
    cvalue >>= 7;
  }
  *ptr++ = static_cast<std::uint8_t>(cvalue);

  return { ptr, static_cast<std::size_t>(top - ptr) };
}

template <typename T>
std::tuple<T, std::span<const std::uint8_t>> varint_decode(std::span<const std::uint8_t> data) {
  static_assert(std::is_unsigned<T>::value, "Varint decoding requires unsigned type");

  const std::uint8_t* ptr = data.data();
  const std::uint8_t* top = ptr + data.size();
  unsigned int nbits = 0;
  T value = 0;

  while (ptr < top && *ptr & 0x80) {
    DCPL_CHECK_LT(nbits, bit_sizeof<T>()) << "Too many bits while decoding varint";

    value |= static_cast<T>(0x7f & *ptr++) << nbits;
    nbits += 7;
  }
  DCPL_ASSERT(ptr < top) << "Buffer overflow while decoding varint: value=" << value
                         << " size=" << data.size();
  DCPL_CHECK_LT(nbits, bit_sizeof<T>()) << "Too many bits while decoding varint";

  value |= static_cast<T>(*ptr++) << nbits;

  return { value, std::span<const std::uint8_t>(ptr, static_cast<std::size_t>(top - ptr)) };
}

template <typename C>
std::vector<std::uint8_t> varint_vec_encode(const C& idata) {
  std::size_t max_size = idata.size() * sizeof(typename C::value_type) * 2;
  std::vector<std::uint8_t> odata(max_size, 0);
  std::span<std::uint8_t> enc_buf(odata);

  for (const auto& n : idata) {
    enc_buf = varint_encode(n, enc_buf);
  }
  odata.resize(odata.size() - enc_buf.size());

  return odata;
}

template <typename T>
std::vector<T> varint_vec_decode(std::span<const std::uint8_t> idata) {
  std::vector<T> odata;

  odata.reserve(idata.size());

  std::span<const std::uint8_t> rem = idata;

  while (!rem.empty()) {
    auto [value, drem] = varint_decode<T>(rem);

    odata.push_back(value);
    rem = drem;
  }

  return odata;
}

}

