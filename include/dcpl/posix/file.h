#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "dcpl/assert.h"
#include "dcpl/types.h"

namespace dcpl {

class file {
 public:
  using open_mode = std::size_t;
  using seek_mode = std::size_t;
  using mmap_mode = std::size_t;

  class mmap {
    friend class file;

   public:
    mmap(mmap&& ref);

    mmap(const mmap&) = delete;

    ~mmap();

    template <typename T = char>
    std::span<T> data() const {
      char* base = reinterpret_cast<char*>(base_) + align_;
      std::size_t size = size_ - align_;

      DCPL_CHECK_EQ(size % sizeof(T), 0);

      return { reinterpret_cast<T*>(base), size / sizeof(T) };
    }

    operator std::string_view() const {
      auto mdata = data<std::string_view::value_type>();

      return { mdata.data(), mdata.size() };
    }

    void sync();

   private:
    mmap(std::string path, int fd, mmap_mode mode, fileoff_t offset,
         std::size_t size, std::size_t align);

    std::string path_;
    int fd_ = -1;
    mmap_mode mode_ = 0;
    fileoff_t offset_ = 0;
    std::size_t size_ = 0;
    std::size_t align_ = 0;
    void* base_ = nullptr;
  };

  static constexpr open_mode open_read = 1;
  static constexpr open_mode open_write = 1 << 1;
  static constexpr open_mode open_create = 1 << 2;
  static constexpr open_mode open_trunc = 1 << 3;
  static constexpr open_mode open_append = 1 << 4;

  static constexpr seek_mode seek_set = 0;
  static constexpr seek_mode seek_cur = 1;
  static constexpr seek_mode seek_end = 2;

  static constexpr mmap_mode mmap_read = 1;
  static constexpr mmap_mode mmap_write = 1 << 1;
  static constexpr mmap_mode mmap_priv = 1 << 2;

  file(std::string path, open_mode mode, int perms = 0600);

  file(int fd, std::string path, open_mode mode);

  file(const file& other) = delete;

  ~file();

  void close();

  const std::string& path() const {
    return path_;
  }

  int fileno() const {
    return fd_;
  }

  fileoff_t size();

  fileoff_t tell();

  fileoff_t seek(seek_mode pos, fileoff_t off);

  void write(const void* data, std::size_t size);

  void read(void* data, std::size_t size);

  std::size_t read_some(void* data, std::size_t size);

  void pwrite(const void* data, std::size_t size, fileoff_t off);

  void pread(void* data, std::size_t size, fileoff_t off);

  std::size_t pread_some(void* data, std::size_t size, fileoff_t off);

  void truncate(fileoff_t size);

  void sync();

  mmap view(mmap_mode mode, fileoff_t offset, std::size_t size);

  static mmap view(std::string path, mmap_mode mode, fileoff_t offset,
                   std::size_t size);

  static mmap view(mmap_mode mode, std::size_t size);

 private:
  std::string path_;
  open_mode mode_ = 0;
  int fd_ = -1;
};

}

