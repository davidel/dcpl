#include "dcpl/file.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "dcpl/assert.h"

namespace dcpl {

file::file(std::string path, open_mode mode, int perms) :
    path_(std::move(path)),
    mode_(mode) {
  int file_mode = (mode_ & write) != 0 ? O_RDWR : O_RDONLY;

  if ((mode_ & create) != 0) {
    file_mode |= O_CREAT;
  }
  if ((mode_ & trunc) != 0) {
    file_mode |= O_TRUNC;
  }

  fd_ = ::open(path_.c_str(), file_mode, perms);
  DCPL_ASSERT(fd_ != -1) << "Unable to open file (" << std::strerror(errno)
                         << "): " << path_;
}

file::~file() {
  if (fd_ != -1) {
    ::close(fd_);
  }
}

fileoff_t file::tell() const {
  return static_cast<fileoff_t>(::lseek(fd_, 0, SEEK_CUR));
}

fileoff_t file::seek(seek_mode pos, fileoff_t off) {
  fileoff_t offset = -1;

  switch (pos) {
    case seek_set:
      offset = static_cast<fileoff_t>(::lseek(fd_, off, SEEK_SET));
      break;

    case seek_cur:
      offset = static_cast<fileoff_t>(::lseek(fd_, off, SEEK_CUR));
      break;

    case seek_end:
      offset = static_cast<fileoff_t>(::lseek(fd_, off, SEEK_END));
      break;

    default:
      DCPL_THROW() << "Invalid seek mode: " << pos;
  }

  DCPL_CHECK_GE(offset, 0)
      << "Failed to seek(" << pos << ", " << off
      << ") (" << std::strerror(errno) << ") : " << path_;

  return offset;
}

void file::store(const void* data, std::size_t size) {
  dcpl::ssize_t count = ::write(fd_, data, size);

  DCPL_CHECK_EQ(size, count)
      << "Failed to write file (" << std::strerror(errno) << ") : " << path_;
}

void file::load(void* data, std::size_t size) {
  dcpl::ssize_t count = ::read(fd_, data, size);

  DCPL_CHECK_EQ(size, count)
      << "Failed to read file (" << std::strerror(errno) << ") : " << path_;
}

std::size_t file::load_some(void* data, std::size_t size) {
  dcpl::ssize_t count = ::read(fd_, data, size);

  DCPL_CHECK_GE(count, 0)
      << "Failed to read file (" << std::strerror(errno) << ") : " << path_;

  return count;
}

void file::sync() {
  DCPL_ASSERT((mode_ & write) != 0)
      << "Cannot sync an mmap opened in read mode: " << path_;

  DCPL_ASSERT(::fdatasync(fd_) == 0)
      << "Failed to sync file (" << std::strerror(errno) << ") : " << path_;
}

}

