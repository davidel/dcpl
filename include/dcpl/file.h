#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "dcpl/types.h"

namespace dcpl {

class file {
 public:
  using open_mode = std::uint32_t;
  using seek_mode = std::uint32_t;

  static constexpr open_mode read = 0;
  static constexpr open_mode write = 1;
  static constexpr open_mode create = 1 << 1;
  static constexpr open_mode trunc = 1 << 2;

  static constexpr seek_mode seek_set = 0;
  static constexpr seek_mode seek_cur = 1;
  static constexpr seek_mode seek_end = 2;

  file(std::string path, open_mode mode, int perms = 0600);

  file(const file& other) = delete;

  ~file();

  fileoff_t tell() const;

  fileoff_t seek(seek_mode pos, fileoff_t off);

  void store(const void* data, std::size_t size);

  void load(void* data, std::size_t size);

  void sync();

 private:
  std::string path_;
  open_mode mode_ = read;
  int fd_ = -1;
};

}

