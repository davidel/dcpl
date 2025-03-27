#include "dcpl/file.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>

#include "dcpl/assert.h"
#include "dcpl/cleanup.h"
#include "dcpl/constants.h"
#include "dcpl/core_utils.h"
#include "dcpl/system.h"

namespace dcpl {
namespace {

// Linux still has the limit of 2GB for read/write operations, even on 64bit builds.
constexpr std::size_t max_rw_chunk = 1ULL << 30;

std::size_t read_file(int fd, void* data, std::size_t size) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::read(fd, ptr, csize);

    if (count < 0) {
      return consts::invalid_size;
    }

    tx_size -= count;
    ptr += count;
    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

std::size_t write_file(int fd, const void* data, std::size_t size) {
  std::size_t tx_size = size;
  const char* ptr = reinterpret_cast<const char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::write(fd, ptr, csize);

    if (count < 0) {
      return consts::invalid_size;
    }

    tx_size -= count;
    ptr += count;
    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

std::size_t pread_file(int fd, void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = size;
  char* ptr = reinterpret_cast<char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::pread(fd, ptr, csize, off + size - tx_size);

    if (count < 0) {
      return consts::invalid_size;
    }

    tx_size -= count;
    ptr += count;
    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

std::size_t pwrite_file(int fd, const void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = size;
  const char* ptr = reinterpret_cast<const char*>(data);

  while (tx_size > 0) {
    std::size_t csize = std::min<std::size_t>(tx_size, max_rw_chunk);
    dcpl::ssize_t count = ::pwrite(fd, ptr, csize, off + size - tx_size);

    if (count < 0) {
      return consts::invalid_size;
    }

    tx_size -= count;
    ptr += count;
    if (count != csize) {
      break;
    }
  }

  return size - tx_size;
}

}

file::mmap::mmap(int fd, mmap_mode mode, fileoff_t offset, std::size_t size,
                 std::size_t align) :
    mode_(mode),
    offset_(offset),
    size_(size),
    align_(align) {
  int flags = 0;
  cleanup cleanups;

  if (fd != -1) {
    fd_ = ::dup(fd);
    DCPL_ASSERT(fd_ != -1) << "Unable to duplicate file descriptor: "
                           << std::strerror(errno);
    cleanups.push([fd = fd_]() { ::close(fd); });
    flags = (mode_ & mmap_priv) != 0 ? MAP_PRIVATE : MAP_SHARED;
  } else {
    flags = MAP_PRIVATE | MAP_ANON;
  }

  // The ::mmap() API is supposed to fail with zero bytes mappings, and we want to
  // be able to create such mappings. We simply let `base_` remain nullptr and we
  // handle such case in other APIs.
  if (size_ > 0) {
    int prot = (mode_ & mmap_write) != 0 ? PROT_READ | PROT_WRITE : PROT_READ;

    base_ = ::mmap(nullptr, size_, prot, flags, fd_, offset_);
    DCPL_ASSERT(base_ != MAP_FAILED) << "Failed to mmap file: " << std::strerror(errno);
  }

  cleanups.reset();
}

file::mmap::mmap(mmap&& ref) :
    fd_(ref.fd_),
    mode_(ref.mode_),
    offset_(ref.offset_),
    size_(ref.size_),
    align_(ref.align_),
    base_(ref.base_) {
  ref.fd_ = -1;
  ref.base_ = nullptr;
}

file::mmap::~mmap() {
  if (base_ != nullptr) {
    DCPL_ASSERT(::munmap(base_, size_) != -1)
        << "Failed to unmap file section: " << std::strerror(errno);
  }
  if (fd_ != -1) {
    ::close(fd_);
  }
}

void file::mmap::sync() {
  DCPL_ASSERT(fd_ != -1) << "Cannot sync anonymous mmap";

  if ((mode_ & mmap_priv) != 0) {
    std::size_t tx_size = pwrite_file(fd_, reinterpret_cast<char*>(base_) + align_,
                                      size_ - align_, offset_ + align_);

    DCPL_CHECK_EQ(tx_size, size_ - align_)
        << "Failed to write file: " << std::strerror(errno);
  } else if (base_ != nullptr) {
    DCPL_ASSERT(::msync(base_, size_, MS_SYNC) == 0)
        << "Failed to sync mmap: " << std::strerror(errno);
  }

  DCPL_ASSERT(::fdatasync(fd_) == 0)
      << "Failed to sync mmap file section: " << std::strerror(errno);
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

file::file(int fd, std::string path, open_mode mode) :
    path_(std::move(path)),
    mode_(mode) {
  fd_ = ::dup(fd);
  DCPL_ASSERT(fd_ != -1) << "Unable to duplicate file descriptor ("
                         << std::strerror(errno) << "): " << path_;
}

file::~file() {
  close();
}

void file::close() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

fileoff_t file::size() {
  fileoff_t offset = tell();
  fileoff_t file_size = seek(seek_end, 0);

  seek(seek_set, offset);

  return file_size;
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
  std::size_t tx_size = write_file(fd_, data, size);

  DCPL_CHECK_EQ(tx_size, size)
      << "Failed to write file (" << std::strerror(errno) << "): " << path_;
}

void file::read(void* data, std::size_t size) {
  std::size_t tx_size = read_file(fd_, data, size);

  DCPL_CHECK_EQ(tx_size, size)
      << "Failed to read file (" << std::strerror(errno) << "): " << path_;
}

std::size_t file::read_some(void* data, std::size_t size) {
  std::size_t tx_size = read_file(fd_, data, size);

  DCPL_ASSERT(tx_size != consts::invalid_size)
      << "Failed to read file (" << std::strerror(errno) << "): " << path_;

  return tx_size;
}

void file::pwrite(const void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = pwrite_file(fd_, data, size, off);

  DCPL_CHECK_EQ(tx_size, size)
      << "Failed to write file (" << std::strerror(errno) << "): " << path_;
}

void file::pread(void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = pread_file(fd_, data, size, off);

  DCPL_CHECK_EQ(tx_size, size)
      << "Failed to read file (" << std::strerror(errno) << "): " << path_;
}

std::size_t file::pread_some(void* data, std::size_t size, fileoff_t off) {
  std::size_t tx_size = pread_file(fd_, data, size, off);

  DCPL_ASSERT(tx_size != consts::invalid_size)
      << "Failed to read file (" << std::strerror(errno) << "): " << path_;

  return tx_size;
}

void file::truncate(fileoff_t size) {
  DCPL_ASSERT(::ftruncate(fd_, size) == 0)
      << "Failed to resize file (" << std::strerror(errno)
      << ") : " << path_;
}

void file::sync() {
  DCPL_ASSERT(::fdatasync(fd_) == 0)
      << "Failed to sync file (" << std::strerror(errno) << "): " << path_;
}

file::mmap file::view(mmap_mode mode, fileoff_t offset, std::size_t size) {
  DCPL_ASSERT((mode & mmap_write) == 0 || (mode_ & open_write) != 0)
      << "Cannot create a writable view of a file opened in read mode: " << path_;

  if (size == 0) {
    fileoff_t file_size = this->size();

    size = int_cast<std::size_t>(file_size - offset);
  }

  std::size_t pgsize = page_size();
  std::size_t align = static_cast<std::size_t>(offset % pgsize);

  return mmap(fd_, mode, offset - align, size + align, align);
}

file::mmap file::view(std::string path, mmap_mode mode, fileoff_t offset,
                      std::size_t size) {
  open_mode file_mode = (mode & mmap_write) != 0 ? open_read | open_write : open_read;
  file vfile(std::move(path), file_mode);

  return vfile.view(mode, offset, size);
}

file::mmap file::view(mmap_mode mode, std::size_t size) {
  return mmap(-1, mode, 0, size, 0);
}

}

