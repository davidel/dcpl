#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <fstream>
#include <map>
#include <memory>
#include <memory_resource>
#include <queue>
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

class archive {
 public:
  using allocator = std::pmr::unsynchronized_pool_resource;

  explicit archive(std::fstream* file, allocator* ator = nullptr) :
      file_(file),
      ator_(ator) {
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void store(const T& data) {
    file_->write(reinterpret_cast<const char*>(&data), sizeof(data));
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void load(T& data) {
    file_->read(reinterpret_cast<char*>(&data), sizeof(data));
  }

  template <typename T>
  void store_ptr(const T& data) {
    if (data) {
      store(static_cast<char>(1));
      store(*data);
    } else {
      store(static_cast<char>(0));
    }
  }

  template <typename T>
  void store(const std::unique_ptr<T>& data) {
    store_ptr(data);
  }

  template <typename T>
  void store(const std::shared_ptr<T>& data) {
    store_ptr(data);
  }

  template <typename T, typename C>
  void load_ptr(T& data, const C& ctor) {
    char marker;

    load(marker);
    if (marker) {
      data = ctor();
      load(*data);
    } else {
      data = nullptr;
    }
  }

  template <typename T>
  void load(std::unique_ptr<T>& data) {
    load_ptr(data, std::make_unique<T>);
  }

  template <typename T>
  void load(std::shared_ptr<T>& data) {
    load_ptr(data, std::make_shared<T>);
  }

  template <typename T>
  void store_vector(const T& data) {
    store(data.size());
    for (const auto& value : data) {
      store(value);
    }
  }

  template <typename T>
  void store(const std::vector<T>& data) {
    store_vector(data);
  }

  template <typename T>
  void store(const std::deque<T>& data) {
    store_vector(data);
  }

  template <typename T>
  void store(const std::queue<T>& data) {
    store_vector(data);
  }

  void store(const std::string& data) {
    store_vector(data);
  }

  void store(std::string_view data) {
    store_vector(data);
  }

  template <typename T>
  void store(std::span<T> data) {
    store_vector(data);
  }

  template <typename T, std::size_t N>
  void store(const std::array<T, N>& data) {
    for (const auto& value : data) {
      store(value);
    }
  }

  template <typename T>
  void load_vector(T& data) {
    decltype(data.size()) size;

    load(size);
    data.resize(size);
    for (decltype(size) i = 0; i < size; ++i) {
      load(data[i]);
    }
  }

  template <typename T>
  void load_span(T& data) {
    DCPL_CHECK_NE(ator_, nullptr) << "unable to deserialize spans without an allocator";

    decltype(data.size()) size;

    load(size);

    typename T::value_type* ptr = reinterpret_cast<typename T::value_type*>(
        ator_->allocate(size * sizeof(typename T::value_type)));

    for (decltype(size) i = 0; i < size; ++i) {
      load(ptr[i]);
    }

    data = T{ ptr, size };
  }

  template <typename T>
  void load(std::vector<T>& data) {
    load_vector(data);
  }

  template <typename T>
  void load(std::deque<T>& data) {
    load_vector(data);
  }

  template <typename T>
  void load(std::queue<T>& data) {
    load_vector(data);
  }

  void load(std::string& data) {
    load_vector(data);
  }

  void load(std::string_view& data) {
    load_span(data);
  }

  template <typename T>
  void load(std::span<T>& data) {
    load_span(data);
  }

  template <typename T, std::size_t N>
  void load(std::array<T, N>& data) {
    for (auto& value : data) {
      load(value);
    }
  }

  template <typename T>
  void store_map(const T& data) {
    store(data.size());
    for (const auto& it : data) {
      store(it.first);
      store(it.second);
    }
  }

  template <typename... T>
  void store(const std::map<T...>& data) {
    store_map(data);
  }

  template <typename... T>
  void store(const std::unordered_map<T...>& data) {
    store_map(data);
  }

  template <typename T>
  void load_map(T& data) {
    decltype(data.size()) size;

    load(size);
    for (decltype(size) i = 0; i < size; ++i) {
      typename T::key_type key;
      typename T::mapped_type value;

      load(key);
      load(value);
      data.emplace(std::move(key), std::move(value));
    }
  }

  template <typename... T>
  void load(std::map<T...>& data) {
    load_map(data);
  }

  template <typename... T>
  void load(std::unordered_map<T...>& data) {
    load_map(data);
  }

  template <typename T>
  void store_set(const T& data) {
    store(data.size());
    for (const auto& value : data) {
      store(value);
    }
  }

  template <typename... T>
  void store(const std::set<T...>& data) {
    store_set(data);
  }

  template <typename... T>
  void store(const std::unordered_set<T...>& data) {
    store_set(data);
  }

  template <typename T>
  void load_set(T& data) {
    decltype(data.size()) size;

    load(size);
    for (decltype(size) i = 0; i < size; ++i) {
      typename T::value_type value;

      load(value);
      data.emplace(std::move(value));
    }
  }

  template <typename... T>
  void load(std::set<T...>& data) {
    load_set(data);
  }

  template <typename... T>
  void load(std::unordered_set<T...>& data) {
    load_set(data);
  }

  template <typename T, typename S = decltype(&T::store)>
  void store(const T& data) {
    data.store(this);
  }

  template <typename T, typename S = decltype(&T::load)>
  void load(T& data) {
    data.load(this);
  }

 private:
  std::fstream* file_;
  allocator* ator_ = nullptr;
};

}

