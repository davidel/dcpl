#include "dcpl/fs.h"

#include "dcpl/utils.h"

namespace dcpl {

std::string temp_path(const fs::path& path) {
  return path / rand_string(12);
}

std::string temp_path() {
  return fs::temp_directory_path() / rand_string(12);
}

}

