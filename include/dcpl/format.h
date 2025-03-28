#pragma once

#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "dcpl/assert.h"
#include "dcpl/types.h"

namespace dcpl {

template <typename T>
class format {
 public:
  format(const char* fmt, T value) :
      fmt_(fmt),
      value_(std::move(value)) {
  }

  friend std::ostream& operator<<(std::ostream& stream, const format& fmt) {
    char sbuf[64];
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

class format_time {
 public:
  format_time(const char* fmt, std::time_t epoch_time) :
      fmt_(fmt),
      epoch_time_(epoch_time) {
  }

  friend std::ostream& operator<<(std::ostream& stream, const format_time& fmt);

 private:
  const char* fmt_ = nullptr;
  std::time_t epoch_time_ = 0;
};

std::string format_duration(double elapsed);

std::string format_bytes(umaxint_t size);

}

