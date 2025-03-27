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

  bfloat16& operator=(float value) {
    value_ = to_bfloat16(value);

    return *this;
  }

  float operator+(float value) const {
    return to_float() + value;
  }

  float operator-(float value) const {
    return to_float() - value;
  }

  float operator*(float value) const {
    return to_float() * value;
  }

  float operator/(float value) const {
    return to_float() / value;
  }

  auto operator<=>(float value) const {
    return to_float() <=> value;
  }

  operator float() const {
    return to_float();
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

