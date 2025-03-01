#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <istream>
#include <list>
#include <map>
#include <memory>
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
  struct deletable {
    virtual ~deletable() { }
  };

  template <typename T>
  struct span_buffer : public deletable {
    explicit span_buffer(std::size_t size) :
        buffer(std::make_unique<T[]>(size)) {
    }

    std::unique_ptr<T[]> buffer;
  };

 public:
  using span_storage = std::vector<std::unique_ptr<deletable>>;

  explicit archive(std::iostream* stream, span_storage* span_stg = nullptr) :
      stream_(stream),
      span_stg_(span_stg) {
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void store(const T& data) {
    stream_->write(reinterpret_cast<const char*>(&data), sizeof(data));
  }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void load(T& data) {
    stream_->read(reinterpret_cast<char*>(&data), sizeof(data));
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
  void store(const std::list<T>& data) {
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
    for (auto& value : data) {
      load(value);
    }
  }

  template <typename T>
  void load_span(T& data) {
    DCPL_CHECK_NE(span_stg_, nullptr)
        << "Unable to deserialize spans without a span storage";

    decltype(data.size()) size;

    load(size);

    using span_type = span_buffer<typename T::value_type>;

    std::unique_ptr<span_type> span = std::make_unique<span_type>(size);

    for (decltype(size) i = 0; i < size; ++i) {
      load(span->buffer[i]);
    }
    data = T{ span->buffer.get(), size };

    span_stg_->push_back(std::move(span));
  }

  template <typename T>
  void load(std::vector<T>& data) {
    load_vector(data);
  }

  template <typename T>
  void load(std::list<T>& data) {
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

  template <bool RESERVE, typename T>
  auto load_size(T& data) {
    decltype(data.size()) size;

    load(size);
    if constexpr (RESERVE) {
      data.reserve(size);
    }

    return size;
  }

  template <bool RESERVE, typename T>
  void load_map(T& data) {
    auto size = load_size<RESERVE>(data);

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
    load_map<false>(data);
  }

  template <typename... T>
  void load(std::unordered_map<T...>& data) {
    load_map<true>(data);
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

  template <bool RESERVE, typename T>
  void load_set(T& data) {
    auto size = load_size<RESERVE>(data);

    for (decltype(size) i = 0; i < size; ++i) {
      typename T::value_type value;

      load(value);
      data.emplace(std::move(value));
    }
  }

  template <typename... T>
  void load(std::set<T...>& data) {
    load_set<false>(data);
  }

  template <typename... T>
  void load(std::unordered_set<T...>& data) {
    load_set<true>(data);
  }

  template <typename T, void (T::*S)(archive*) const = &T::store>
  void store(const T& data) {
    data.store(this);
  }

  template <typename T, void (T::*S)(archive*) = &T::load>
  void load(T& data) {
    data.load(this);
  }

 private:
  std::iostream* stream_ = nullptr;
  span_storage* span_stg_ = nullptr;
};

}

