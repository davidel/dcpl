#include "dcpl/mapfile.h"

#include <fcntl.h>
#include <unistd.h>

#include "dcpl/assert.h"
#include "dcpl/cleanup.h"

namespace dcpl {

mapfile::mapfile(const char* path) {
  fd_ = ::open(path, O_RDONLY);
  DCPL_ASSERT(fd_ != -1) << "Unable to open file: " << path;

  cleanup clean_file([this]() { ::close(fd_); });

  size_ = ::lseek(fd_, 0, SEEK_END);
  ::lseek(fd_, 0, SEEK_SET);

  base_ = ::mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
  DCPL_ASSERT(base_ != MAP_FAILED) << "Failed to mmap file: " << path;

  clean_file.reset();
}

mapfile::~mapfile() {
  if (base_ != MAP_FAILED) {
    ::munmap(base_, size_);
  }
  if (fd_ != -1) {
    ::close(fd_);
  }
}

}
