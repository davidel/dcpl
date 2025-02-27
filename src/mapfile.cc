#include "dcpl/mapfile.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "dcpl/assert.h"
#include "dcpl/cleanup.h"

namespace dcpl {
namespace {

struct mm_pages_conf {
  int flags = 0;
  std::size_t page_size = 0;
  std::size_t max_mmap_size = 0;
};

mm_pages_conf get_mm_config() {
  // Do more things with hugetlb pages here ...
  return { 0, static_cast<std::size_t>(::getpagesize()), 1ULL << 42 };
}

}

mapfile::mapfile(std::string path, open_mode mode) :
    path_(std::move(path)),
    mode_(mode) {
  int file_mode = (mode_ & write) != 0 ? O_RDWR : O_RDONLY;

  if ((mode_ & create) != 0) {
    file_mode |= O_CREAT;
  }
  if ((mode_ & trunc) != 0) {
    file_mode |= O_TRUNC;
  }

  mm_pages_conf mmconf = get_mm_config();

  page_size_ = mmconf.page_size;

  fd_ = ::open(path_.c_str(), file_mode, 0600);
  DCPL_ASSERT(fd_ != -1) << "Unable to open file (" << std::strerror(errno)
                         << "): " << path_;

  cleanup cleanups([this]() { ::close(fd_); });

  size_ = ::lseek(fd_, 0, SEEK_END);
  ::lseek(fd_, 0, SEEK_SET);

  base_ = ::mmap(nullptr, mmconf.max_mmap_size, PROT_NONE,
                 MAP_SHARED | mmconf.flags, fd_, 0);
  DCPL_ASSERT(base_ != MAP_FAILED) << "Failed to mmap file (" << std::strerror(errno)
                                   << "): " << path_;

  mapped_size_ = mmconf.max_mmap_size;

  cleanups.push([this]() { ::munmap(base_, mapped_size_); });

  int prot = (mode_ & write) != 0 ? PROT_READ | PROT_WRITE : PROT_READ;

  DCPL_ASSERT(::mprotect(base_, size_, prot) == 0)
      << "Failed to set mmap memory protection (" << std::strerror(errno)
      << ") : " << path_;

  cleanups.reset();
}

mapfile::~mapfile() {
  if (base_ != MAP_FAILED) {
    ::munmap(base_, mapped_size_);
  }
  if (fd_ != -1) {
    ::close(fd_);
  }
}

void mapfile::resize(fileoff_t size) {
  DCPL_ASSERT((mode_ & write) != 0)
      << "Cannot resize an mmap opened in read mode: " << path_;

  DCPL_ASSERT(::msync(base_, size_, MS_SYNC) == 0)
      << "Failed to sync mmap memory (" << std::strerror(errno)
      << ") : " << path_;

  DCPL_ASSERT(::mprotect(base_, size_, PROT_NONE) == 0)
      << "Failed to set mmap memory protection (" << std::strerror(errno)
      << ") : " << path_;

  DCPL_ASSERT(::ftruncate(fd_, size) == 0)
      << "Failed to resize mmap file (" << std::strerror(errno)
      << ") : " << path_;

  size_ = size;

  DCPL_ASSERT(::mprotect(base_, size_, PROT_READ | PROT_WRITE) == 0)
      << "Failed to set mmap memory protection (" << std::strerror(errno)
      << ") : " << path_;
}

}

