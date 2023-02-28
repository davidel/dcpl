#include <sys/mman.h>
#include <sys/types.h>

#include <span>
#include <string_view>

#pragma once

namespace dcpl {

class mapfile {
 public:
  explicit mapfile(const char* path);

  virtual ~mapfile();

  template <typename T>
  std::span<const T> data() const {
    return std::span<const T>(reinterpret_cast<const T*>(base_), size_);
  }

  operator std::string_view() const {
    auto cont = data<char>();

    return { cont.data(), cont.size() };
  }

 private:
  int fd_ = -1;
  void* base_ = MAP_FAILED;
  off_t size_ = 0;
};

}

