#pragma once

#include <iostream>
#include <sstream>

namespace dcpl {

struct block_stream {
  explicit block_stream(std::ostream& stream) : os(stream) { }

  ~block_stream() {
    os << ss.str();
  }

  template <typename T>
  block_stream& operator<<(const T& v) {
    ss << v;
    return *this;
  }

  std::ostream& os;
  std::stringstream ss;
};

block_stream ostream() {
  return block_stream(std::cout);
}

block_stream estream() {
  return block_stream(std::cerr);
}

}
