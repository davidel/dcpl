#include "dcpl/temp_path.h"

#include "dcpl/fs.h"

namespace dcpl {

temp_path::temp_path(std::optional<std::string> path) :
    path_(path.has_value() ? get_temp_path(*path) : get_temp_path()) {
}

temp_path::~temp_path() {
  if (!released_ && std::filesystem::exists(path_)) {
    fs::remove(path_);
  }
}

}

