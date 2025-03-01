#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "dcpl/assert.h"

namespace dcpl {

template <typename B>
class memory {
 public:
  explicit memory(std::span<B> data) :
      data_(data) {
    static_assert(sizeof(B) == 1);
  }

  void seek(std::size_t offset) {
    DCPL_CHECK_LE(offset, data_.size()) << "Seek out of bounds";

    offset_ = offset;
  }

  void skip(std::size_t size) {
    DCPL_CHECK_LE(offset_ + size, data_.size()) << "Skip out of bounds";

    offset_ += size;
  }

  std::size_t tell() const {
    return offset_;
  }

  std::size_t size() const {
    return data_.size();
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  T read() {
    B* ptr = data_.data() + offset_;
    B* top = ptr + data_.size();

    DCPL_CHECK_GE(top - ptr, sizeof(T)) << "Read out of bounds";

    T value{};

    std::memcpy(&value, ptr, sizeof(value));

    offset_ += sizeof(value);

    return value;
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const T& value) {
    static_assert(!std::is_const_v<B>, "Write not allowed on const memory");

    B* ptr = data_.data() + offset_;
    B* top = ptr + data_.size();

    DCPL_CHECK_GE(top - ptr, sizeof(T)) << "Write out of bounds";

    std::memcpy(ptr, &value, sizeof(value));

    offset_ += sizeof(value);
  }

  template <typename T>
  std::span<T> read(std::size_t count) {
    B* ptr = data_.data() + offset_;
    B* top = ptr + data_.size();

    DCPL_CHECK_GE(top - ptr, count * sizeof(T)) << "Read out of bounds";

    offset_ += count * sizeof(T);

    return { reinterpret_cast<T*>(ptr), count };
  }

 private:
  std::span<B> data_;
  std::size_t offset_ = 0;
};

}

