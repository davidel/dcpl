#include "dcpl/core_utils.h"

#include <cerrno>
#include <exception>

namespace dcpl {

std::string to_upper(const char* data) {
  return to_upper<std::string_view>(data);
}

std::string to_lower(const char* data) {
  return to_lower<std::string_view>(data);
}

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

std::fstream open(const std::string& path, std::ios::openmode mode) {
  std::fstream file(path, mode);

  try {
    file.exceptions(std::ios::failbit | std::ios::badbit);
  } catch (std::ios::failure& exception) {
    if (errno == 0) {
      throw;
    }
    throw std::system_error{ errno, std::generic_category(),
      "Opening file \"" + path + "\"" };
  }

  return file;
}

}

