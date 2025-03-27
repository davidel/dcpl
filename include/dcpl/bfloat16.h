#pragma once

#include <cstdint>
#include <iostream>

namespace dcpl {

class bfloat16 {
 public:
  bfloat16() = default;

  bfloat16(float value) :
      value_(to_bfloat16(value)) {
  }

  bfloat16(double value) :
      value_(to_bfloat16(static_cast<float>(value))) {
  }

  bfloat16& operator=(float value) {
    value_ = to_bfloat16(value);

    return *this;
  }

  bfloat16& operator=(double value) {
    value_ = to_bfloat16(static_cast<float>(value));

    return *this;
  }

  operator float() const {
    return to_float();
  }

  operator double() const {
    return static_cast<double>(to_float());
  }

  friend std::ostream& operator<<(std::ostream& os, const bfloat16& value) {
    os << value.to_float();

    return os;
  }

 private:
  float to_float() const {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    std::uint32_t ivalue = static_cast<std::uint32_t>(value_);
#else
    std::uint32_t ivalue = static_cast<std::uint32_t>(value_) << 16;
#endif

    return *reinterpret_cast<float*>(&ivalue);
  }

  static std::uint16_t to_bfloat16(float value) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return *reinterpret_cast<std::uint16_t*>(&value);
#else
    return *(reinterpret_cast<std::uint16_t*>(&value) + 1);
#endif
  }

  std::uint16_t value_ = 0;
};

}

