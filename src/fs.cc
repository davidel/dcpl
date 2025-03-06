#include "dcpl/fs.h"

#include <system_error>

#include "dcpl/core_utils.h"
#include "dcpl/env.h"
#include "dcpl/string_formatter.h"
#include "dcpl/utils.h"

namespace dcpl {
namespace {

static const std::size_t temp_size = getenv<std::size_t>("DCPL_TEMP_SIZE", 12);

}

namespace fs {

void remove(const std::string& path) {
  std::error_code error;

  if (!stdfs::remove(path, error)) {
    throw stdfs::filesystem_error(_S() << "Unable to remove file: " << path,
                                  error);
  }
}

void create_directory(const std::string& path) {
  std::error_code error;

  if (!stdfs::create_directory(path, error)) {
    throw stdfs::filesystem_error(_S() << "Unable to create folder: " << path,
                                  error);
  }
}

void create_directories(const std::string& path) {
  std::error_code error;

  if (!stdfs::create_directories(path, error)) {
    throw stdfs::filesystem_error(_S() << "Unable to create folders: " << path,
                                  error);
  }
}

void remove_all(const std::string& path) {
  std::error_code error;

  if (stdfs::remove_all(path, error) == static_cast<std::uintmax_t>(-1)) {
    throw stdfs::filesystem_error(_S() << "Unable to recursively remove files: "
                                  << path, error);
  }
}

}

std::string temp_path(const std::string& path) {
  return _S() << path << "." << rand_string(temp_size);
}

std::string temp_path() {
  return stdfs::temp_directory_path() / rand_string(temp_size);
}

}

