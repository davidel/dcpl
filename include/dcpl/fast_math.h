#pragma once

#include <cmath>
#include <cstdint>

namespace dcpl::fmath {
namespace {

union dpack {
  double d;
  std::int64_t i;
};

}

static inline double log(double value) {
  dpack ux = { value };

  return (ux.i - 4606921278410026770LL) * 1.539095918623324e-16;
}

static inline double exp(double value) {
  dpack ux, vx;

  ux.i = static_cast<std::int64_t>(3248660424278399LL * value + 0x3fdf127e83d16f12LL);
  vx.i = static_cast<std::int64_t>(0x3fdf127e83d16f12LL - 3248660424278399LL * value);

  return ux.d / vx.d;
}

}

