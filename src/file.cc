#include "dcpl/file.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

#include "dcpl/assert.h"

namespace dcpl {
namespace {

constexpr std::size_t max_rw_chunk = 1ULL << 30;

}

file::file(std::string path, open_mode mode, int perms) :
    path_(std::move(path)),
    mode_(mode) {
  int file_mode = (mode_ & open_write) != 0 ? O_RDWR : O_RDONLY;

  if ((mode_ & open_create) != 0) {
    file_mode |= O_CREAT;
  }
  if ((mode_ & open_trunc) != 0) {
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

fileoff_t file::size() {
  fileoff_t offset = tell();
  fileoff_t size = seek(seek_end, 0);

  seek(seek_set, offset);

  return size;
}

fileoff_t file::tell() {
  return seek(seek_cur, 0);
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
      << ") (" << std::strerror(errno) << "): " << path_;

  return offset;
}

void file::write(const void* data, std::size_t size) {
  std::size_t tx_size = size;
  const char* ptr = reinterpret_cast<const char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::write(fd_, ptr, csize);

    DCPL_CHECK_EQ(csize, count)
        << "Failed to write file (" << std::strerror(errno) << "): " << path_;

    tx_size -= csize;
    ptr += csize;
  }
}

void file::read(void* data, std::size_t size) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::read(fd_, ptr, csize);

    DCPL_CHECK_EQ(csize, count)
        << "Failed to read file (" << std::strerror(errno) << "): " << path_;

    tx_size -= csize;
    ptr += csize;
  }
}

std::size_t file::read_some(void* data, std::size_t size) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::read(fd_, ptr, csize);

    DCPL_CHECK_GE(count, 0)
        << "Failed to read file (" << std::strerror(errno) << "): " << path_;

    tx_size -= count;
    ptr += count;

    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

void file::pwrite(const void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = size;
  const char* ptr = reinterpret_cast<const char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::pwrite(fd_, ptr, csize, off + size - tx_size);

    DCPL_CHECK_EQ(csize, count)
        << "Failed to write file (" << std::strerror(errno) << "): " << path_;

    tx_size -= csize;
    ptr += csize;
  }
}

void file::pread(void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::pread(fd_, ptr, csize, off + size - tx_size);

    DCPL_CHECK_EQ(csize, count)
        << "Failed to read file (" << std::strerror(errno) << "): " << path_;

    tx_size -= csize;
    ptr += csize;
  }
}

ssize_t file::pread_some(void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::pread(fd_, ptr, csize, off + size - tx_size);

    DCPL_CHECK_GE(count, 0)
        << "Failed to read file (" << std::strerror(errno) << "): " << path_;

    tx_size -= count;
    ptr += count;

    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

void file::sync() {
  DCPL_ASSERT((mode_ & open_write) != 0)
      << "Cannot sync an mmap opened in read mode: " << path_;

  DCPL_ASSERT(::fdatasync(fd_) == 0)
      << "Failed to sync file (" << std::strerror(errno) << "): " << path_;
}

}

