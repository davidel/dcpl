#include "dcpl/temp_file.h"

#include "dcpl/core_utils.h"
#include "dcpl/fs.h"

namespace dcpl {

temp_file::temp_file(std::ios::openmode mode, std::optional<std::string> path) :
    path_(path.has_value() ? temp_path(*path) : temp_path()),
    file_(open(path_, mode | std::ios::in | std::ios::out | std::ios::trunc)) {
}

temp_file::~temp_file() {
  if (!released_) {
    close();
    fs::remove(path_);
  }
}

}

