#include "dcpl/temp_file.h"

#include "dcpl/core_utils.h"
#include "dcpl/fs.h"

namespace dcpl {

temp_file::temp_file(std::ios::openmode mode, std::optional<std::string> path) :
    path_(path),
    file_(open(path_, mode | std::ios::in | std::ios::out | std::ios::trunc)) {
}

temp_file::~temp_file() {
  close();
}

}

