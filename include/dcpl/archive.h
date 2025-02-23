#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"

namespace dcpl {

class Archive {
 public:
  using allocator = std::pmr::unsynchronized_pool_resource;

  explicit Archive(std::fstream* file, allocator* ator = nullptr) :
      file_(file),
      ator_(ator) {
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const T& data) {
    file_->write(reinterpret_cast<const char*>(&data), sizeof(data));
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void read(T& data) {
    file_->read(reinterpret_cast<char*>(&data), sizeof(data));
  }

  template <typename T>
  void write_ptr(const T& data) {
    if (data) {
      write(static_cast<char>(1));
      write(*data);
    } else {
      write(static_cast<char>(0));
    }
  }

  template <typename T>
  void write(const std::unique_ptr<T>& data) {
    write_ptr(data);
  }

  template <typename T>
  void write(const std::shared_ptr<T>& data) {
    write_ptr(data);
  }

  template <typename T, typename C>
  void read_ptr(T& data, const C& ctor) {
    char marker;

    read(marker);
    if (marker) {
      data = ctor();
      read(*data);
    } else {
      data = nullptr;
    }
  }

  template <typename T>
  void read(std::unique_ptr<T>& data) {
    read_ptr(data, std::make_unique<T>);
  }

  template <typename T>
  void read(std::shared_ptr<T>& data) {
    read_ptr(data, std::make_shared<T>);
  }

  template <typename T>
  void write_array(const T& data) {
    write(data.size());
    for (const auto& value : data) {
      write(value);
    }
  }

  template <typename T>
  void write(const std::vector<T>& data) {
    write_array(data);
  }

  void write(const std::string& data) {
    write_array(data);
  }

  void write(std::string_view data) {
    write_array(data);
  }

  template <typename T>
  void write(std::span<T> data) {
    write_array(data);
  }

  template <typename T>
  void read_array(T& data) {
    decltype(data.size()) size;

    read(size);
    data.resize(size);
    for (decltype(size) i = 0; i < size; ++i) {
      read(data[i]);
    }
  }

  template <typename T>
  void read_span(T& data) {
    DCPL_CHECK_NE(ator_, nullptr) << "unable to deserialize spans without an allocator";

    decltype(data.size()) size;

    read(size);

    typename T::value_type* ptr = reinterpret_cast<typename T::value_type*>(
        ator_->allocate(size * sizeof(typename T::value_type)));

    for (decltype(size) i = 0; i < size; ++i) {
      read(ptr[i]);
    }

    data = T{ ptr, size };
  }

  template <typename T>
  void read(std::vector<T>& data) {
    read_array(data);
  }

  void read(std::string& data) {
    read_array(data);
  }

  void read(std::string_view& data) {
    read_span(data);
  }

  template <typename T>
  void read(std::span<T>& data) {
    read_span(data);
  }

  template <typename T>
  void write_map(const T& data) {
    write(data.size());
    for (const auto& it : data) {
      write(it.first);
      write(it.second);
    }
  }

  template <typename... T>
  void write(const std::map<T...>& data) {
    write_map(data);
  }

  template <typename... T>
  void write(const std::unordered_map<T...>& data) {
    write_map(data);
  }

  template <typename T>
  void read_map(T& data) {
    decltype(data.size()) size;

    read(size);
    for (decltype(size) i = 0; i < size; ++i) {
      typename T::key_type key;
      typename T::mapped_type value;

      read(key);
      read(value);
      data.emplace(std::move(key), std::move(value));
    }
  }

  template <typename... T>
  void read(std::map<T...>& data) {
    read_map(data);
  }

  template <typename... T>
  void read(std::unordered_map<T...>& data) {
    read_map(data);
  }

  template <typename T>
  void write_set(const T& data) {
    write(data.size());
    for (const auto& value : data) {
      write(value);
    }
  }

  template <typename... T>
  void write(const std::set<T...>& data) {
    write_set(data);
  }

  template <typename... T>
  void write(const std::unordered_set<T...>& data) {
    write_set(data);
  }

  template <typename T>
  void read_set(T& data) {
    decltype(data.size()) size;

    read(size);
    for (decltype(size) i = 0; i < size; ++i) {
      typename T::value_type value;

      read(value);
      data.emplace(std::move(value));
    }
  }

  template <typename... T>
  void read(std::set<T...>& data) {
    read_set(data);
  }

  template <typename... T>
  void read(std::unordered_set<T...>& data) {
    read_set(data);
  }

 private:
  std::fstream* file_;
  allocator* ator_ = nullptr;
};

}

