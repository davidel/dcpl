#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

#include "dcpl/assert.h"
#include "dcpl/constants.h"
#include "dcpl/core_utils.h"

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

  void align(std::size_t size) {
    seek(round_up(offset_, size));
  }

  std::span<B> skip(std::size_t size) {
    DCPL_CHECK_LE(offset_ + size, data_.size()) << "Skip out of bounds";

    offset_ += size;

    return data_.subspan(offset_ - size, size);
  }

  std::span<B> span(std::size_t size = consts::all) {
    std::size_t ssize = (size == consts::all) ? data_.size() - offset_ : size;

    DCPL_CHECK_LE(offset_ + ssize, data_.size()) << "Span out of bounds";

    return data_.subspan(offset_, ssize);
  }

  std::size_t tell() const {
    return offset_;
  }

  std::size_t size() const {
    return data_.size();
  }

  void read(void* data, std::size_t size) {
    DCPL_CHECK_GE(data_.size() - offset_, size) << "Read out of bounds";

    std::memcpy(data, data_.data() + offset_, size);
    offset_ += size;
  }

  template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
  T read() {
    T value = *reinterpret_cast<const T*>(data_.data() + offset_);

    offset_ += sizeof(T);

    return value;
  }

  void write(const void* data, std::size_t size) {
    DCPL_CHECK_GE(data_.size() - offset_, size) << "Write out of bounds";

    std::memcpy(data_.data() + offset_, data, size);
    offset_ += size;
  }

  template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
  void write(const T& value) {
    *reinterpret_cast<T*>(data_.data() + offset_) = value;
    offset_ += sizeof(T);
  }

  template <typename T,
    typename std::enable_if_t<std::is_arithmetic_v<typename T::key_type>>* = nullptr,
            typename std::enable_if_t<std::is_arithmetic_v<typename T::mapped_type>>* = nullptr>
  void write(const T& value) {
    size_type size = value.size();

    write(size);
    for (const auto& it : value) {
      write(it.first);
      write(it.second);
    }
  }

  template <typename T,
    typename std::enable_if_t<std::is_arithmetic_v<typename T::value_type>>* = nullptr>
  void write(const T& value) {
    size_type size = value.size();

    write(size);
    for (const auto& item : value) {
      write(item);
    }
  }

  template <typename T>
  std::span<T> read(std::size_t count) {
    B* ptr = data_.data() + offset_;
    B* top = ptr + data_.size();

    DCPL_CHECK_GE(top - ptr, count * sizeof(T)) << "Read out of bounds";

    offset_ += count * sizeof(T);

    return { reinterpret_cast<T*>(ptr), count };
  }

  template <typename T>
  std::span<T> read_sequence() {
    size_type count = read<size_type>();

    return read<T>(count);
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

