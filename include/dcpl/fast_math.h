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

}

