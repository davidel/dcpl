#include "dcpl/fs.h"

#include <system_error>

#include "dcpl/core_utils.h"
#include "dcpl/string_formatter.h"
#include "dcpl/utils.h"

namespace dcpl {
namespace fs {

void remove(const std::string& path) {
  std::error_code error;

  if (!stdfs::remove(path, error)) {
    throw stdfs::filesystem_error(_S() << "Unable to remove file: " << path,
                                  error);
  }
}

}

std::string temp_path(const std::string& path) {
  return _S() << path << "." << rand_string(12);
}

std::string temp_path() {
  return stdfs::temp_directory_path() / rand_string(12);
}

atomic_write::atomic_write(std::string path, std::ios::openmode mode) :
    path_(std::move(path)),
    temp_path_(temp_path(path_)),
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

