#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "dcpl/types.h"

namespace dcpl {

class file {
 public:
  using open_mode = std::size_t;
  using seek_mode = std::size_t;

  static constexpr open_mode open_read = 1;
  static constexpr open_mode open_write = 1 << 1;
  static constexpr open_mode open_create = 1 << 2;
  static constexpr open_mode open_trunc = 1 << 3;

  static constexpr seek_mode seek_set = 0;
  static constexpr seek_mode seek_cur = 1;
  static constexpr seek_mode seek_end = 2;

  file(std::string path, open_mode mode, int perms = 0600);

  file(const file& other) = delete;

  ~file();

  const std::string& path() const {
    return path_;
  }

  fileoff_t size();

  fileoff_t tell();

  fileoff_t seek(seek_mode pos, fileoff_t off);

  void write(const void* data, std::size_t size);

  void read(void* data, std::size_t size);

  std::size_t read_some(void* data, std::size_t size);

  void pwrite(const void* data, std::size_t size, fileoff_t off);

  void pread(void* data, std::size_t size, fileoff_t off);

  ssize_t pread_some(void* data, std::size_t size, fileoff_t off);

  void sync();

 private:
  std::string path_;
  open_mode mode_ = open_read;
  int fd_ = -1;
};

}

