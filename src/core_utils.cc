#include "dcpl/core_utils.h"

namespace dcpl {

bool is_prime(std::size_t n) {
  if (n <= 3) {
    return true;
  }
  if (n % 2 == 0) {
    return false;
  }

  std::size_t top = n / 2;

  for (std::size_t i = 3; i < top; i += 2) {
    if (n % i == 0) {
      return false;
    }
  }

  return true;
}

std::size_t next_prime(std::size_t n) {
  for (++n; !is_prime(n); ++n) {}

  return n;
}

}

