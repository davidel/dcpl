#include "dcpl/atomic_write.h"

#include "dcpl/core_utils.h"
#include "dcpl/fs.h"

namespace dcpl {

atomic_write::atomic_write(std::string path, std::ios::openmode mode) :
    path_(std::move(path)),
    temp_path_(get_temp_path(path_)),
    file_(open(temp_path_, mode | std::ios::out | std::ios::trunc)) {
}

atomic_write::~atomic_write() {
  if (!committed_) {
    file_.close();
    fs::remove(temp_path_);
  }
}

void atomic_write::commit() {
  file_.close();
  stdfs::rename(temp_path_, path_);
  committed_ = true;
}

}

