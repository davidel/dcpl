#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

#include "dcpl/assert.h"

namespace dcpl {

template <typename B = const char>
class memory {
 public:
  memory(B* data, std::size_t size) :
      data_(data, size) {
    static_assert(sizeof(B) == 1);
  }

  explicit memory(std::span<B> data) :
      data_(data) {
    static_assert(sizeof(B) == 1);
  }

  explicit memory(std::string_view data) :
      data_(data.data(), data.size()) {
  }

  void seek(std::size_t offset) {
    DCPL_CHECK_LE(offset, data_.size()) << "Seek out of bounds";

    offset_ = offset;
  }

  std::span<B> skip(std::size_t size) {
    DCPL_CHECK_LE(offset_ + size, data_.size()) << "Skip out of bounds";

    offset_ += size;

    return data_.subspan(offset_ - size, size);
  }

  std::size_t tell() const {
    return offset_;
  }

  std::size_t size() const {
    return data_.size();
  }

  template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
  T read() {
    B* ptr = data_.data() + offset_;
    B* top = ptr + data_.size();

    DCPL_CHECK_GE(top - ptr, sizeof(T)) << "Read out of bounds";

    T value{};

    std::memcpy(&value, ptr, sizeof(value));

    offset_ += sizeof(value);

    return value;
  }

  template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
  void write(const T& value) {
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

