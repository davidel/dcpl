#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include "dcpl/assert.h"
#include "dcpl/types.h"

namespace dcpl {

class mapfile {
 public:
  using open_mode = std::size_t;

  static constexpr open_mode open_read = 1;
  static constexpr open_mode open_write = 1 << 1;
  static constexpr open_mode open_create = 1 << 2;
  static constexpr open_mode open_trunc = 1 << 3;
  static constexpr open_mode open_priv = 1 << 4;

  mapfile(std::string path, open_mode mode);

  mapfile(const mapfile& other) = delete;

  ~mapfile();

  const std::string& path() const {
    return path_;
  }

  template <typename T>
  std::span<T> data() const {
    DCPL_CHECK_EQ(size_ % sizeof(T), 0)
        << "Mapped memory size not a multiple of " << sizeof(T);
    DCPL_ASSERT(std::is_const_v<T> || (mode_ & open_write) != 0)
        << "Memory map openend in read only mode: " << path_;

    return std::span<T>(reinterpret_cast<T*>(base_), size_ / sizeof(T));
  }

  operator std::string_view() const {
    auto view = data<const char>();

    return { view.data(), view.size() };
  }

  void resize(fileoff_t size);

  void sync();

  static std::size_t page_size();

 private:
  std::string path_;
  open_mode mode_ = open_read;
  std::size_t page_size_ = 0;
  std::size_t mapped_size_ = 0;
  int fd_ = -1;
  void* base_ = nullptr;
  fileoff_t size_ = 0;
};

}

