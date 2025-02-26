#pragma once

#include <cmath>
#include <cstdint>

namespace dcpl::fmath {

static inline double log(double value) {
  union {
    double d;
    std::int64_t i;
  } ux = { value };

  return (ux.i - 4606921278410026770LL) * 1.539095918623324e-16;
}

static inline double exp(double value) {
  union {
    double d;
    std::int64_t i;
  } ux, vx;

  ux.i = static_cast<std::int64_t>(3248660424278399LL * value + 0x3fdf127e83d16f12LL);
  vx.i = static_cast<std::int64_t>(0x3fdf127e83d16f12LL - 3248660424278399LL * value);

  return ux.d / vx.d;
}

}

