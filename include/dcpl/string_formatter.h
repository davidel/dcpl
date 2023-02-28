#pragma once

#include <sstream>
#include <string>

#include "dcpl/stdns_override.h"

namespace dcpl {

class string_formatter {
public:
  template <typename T>
  string_formatter& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

  std::string str() const {
    return stream_.str();
  }

  operator std::string() const {
    return str();
  }

private:
  std::stringstream stream_;
};

using _S = string_formatter;

}

