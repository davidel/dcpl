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
  using size_type = std::uint64_t;

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

  template <typename K, typename V, typename F,
            typename std::enable_if_t<std::is_arithmetic_v<K>>* = nullptr,
            typename std::enable_if_t<std::is_arithmetic_v<V>>* = nullptr>
  void enumerate_kvpairs(const F& enum_fn) {
    size_type count = read<size_type>();

    for (size_type i = 0; i < count; ++i) {
      K key = read<K>();
      V value = read<V>();

      enum_fn(key, value);
    }
  }

 private:
  std::span<B> data_;
  std::size_t offset_ = 0;
};

}

