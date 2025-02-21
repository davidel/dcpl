#pragma once

#include <cstdio>
#include <iostream>
#include <memory>

#include "dcpl/assert.h"

namespace dcpl {

template <typename T>
class format {
 public:
  format(const char* fmt, T value) :
      fmt_(fmt),
      value_(std::move(value)) {
  }

  friend std::ostream& operator<<(std::ostream& stream, const format& fmt) {
    char sbuf[128];
    int size = std::snprintf(sbuf, sizeof(sbuf), fmt.fmt_, fmt.value_);

    DCPL_CHECK_GE(size, 0) << "Invalid format: " << fmt.fmt_;
    if (size < sizeof(sbuf)) {
      stream << sbuf;
    } else {
      std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size + 1);

      std::snprintf(buffer.get(), size + 1, fmt.fmt_, fmt.value_);
      stream << buffer.get();
    }

    return stream;
  }

 private:
  const char* fmt_ = nullptr;
  T value_;
};

}

