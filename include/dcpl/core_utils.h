#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <istream>
#include <ios>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// Dependencies must be limited!
// The "dcpl/assert.h" include only depends on "dcpl/compiler.h" and "dcpl/pragmas.h"
// which in turn have no dependencies.
// The "dcpl/constants.h" in turn, has not local dependencies.
#include "dcpl/assert.h"
#include "dcpl/constants.h"

namespace dcpl {

namespace {

template <typename T>
struct ref_pair {
  std::reference_wrapper<const typename T::key_type> key;
  std::reference_wrapper<const typename T::mapped_type> value;
};

}

template <typename T>
class scoped_change {
 public:
  scoped_change(T* value, T scope_value) :
      value_(value),
      saved_(std::move(*value_)) {
    *value_ = std::move(scope_value);
  }

  ~scoped_change() {
    *value_ = std::move(saved_);
  }

 private:
  T* value_ = nullptr;
  T saved_;
};

template <typename T, typename F>
std::vector<ref_pair<T>>
map_sort(const T& data, const F& cmp) {
  using mpair = ref_pair<T>;
  std::vector<mpair> sorted;

  sorted.reserve(data.size());
  for (const auto& [key, value] : data) {
    sorted.push_back(mpair{ key, value });
  }

  std::sort(sorted.begin(), sorted.end(), cmp);

  return sorted;
}

template <typename T, typename C>
T map_add_values(const C& data) {
  return std::accumulate(data.begin(), data.end(), static_cast<T>(0),
                         [](const T& value, const auto& it) {
                           return value + it.second;
                         });
}

template <typename T, typename C>
T sum(const C& data) {
  return std::accumulate(data.begin(), data.end(), static_cast<T>(0), std::plus<T>());
}

template<typename T, typename C>
std::unordered_map<typename C::value_type, T> unique_count(const C& data) {
  std::unordered_map<typename C::value_type, T> uniq;

  uniq.reserve(data.size());
  for (const auto& v : data) {
    uniq[v] += static_cast<T>(1);
  }

  return uniq;
}

template<typename T>
const typename T::mapped_type* get(const T& data, const typename T::key_type& key) {
  auto it = data.find(key);

  return (it != data.end()) ? &it->second : nullptr;
}

template<typename T>
typename T::mapped_type* get(T& data, const typename T::key_type& key) {
  auto it = data.find(key);

  return (it != data.end()) ? &it->second : nullptr;
}

template<typename T>
typename T::mapped_type get_or(const T& data, const typename T::key_type& key,
                               typename T::mapped_type defval) {
  auto* value = get(data, key);

  return (value != nullptr) ? *value : std::move(defval);
}

template<typename T, typename C>
std::vector<T> to_vector(const C& data) {
  return std::vector<T>(data.begin(), data.end());
}

template <typename T, typename C>
std::vector<T> to_vector_cast(const C& data) {
  std::vector<T> dest;

  dest.reserve(data.size());
  for (const auto& value : data) {
    dest.push_back(static_cast<T>(value));
  }

  return dest;
}

template <typename T, typename F>
std::string to_helper(const T& data, const F& tfn) {
  std::string result(data.size(), 0);

  for (std::size_t i = 0; i < data.size(); ++i) {
    result[i] = static_cast<char>(tfn(static_cast<unsigned char>(data[i])));
  }

  return result;
}

template <typename T>
std::string to_upper(const T& data) {
  return to_helper(data, [](auto c) { return std::toupper(c); });
}

template <typename T>
std::string to_lower(const T& data) {
  return to_helper(data, [](auto c) { return std::tolower(c); });
}

std::string to_upper(const char* data);

std::string to_lower(const char* data);

template <typename T, typename S>
std::span<T> reinterpret_span(std::span<S> source) {
  static_assert((sizeof(T) >= sizeof(S) && sizeof(T) % sizeof(S) == 0) ||
                sizeof(S) % sizeof(T) == 0, "Mismatching size");

  return std::span<T>{ reinterpret_cast<T*>(source.data()), source.size() };
}

template <typename T, typename S,
          typename std::enable_if_t<std::is_pointer_v<S>>* = nullptr>
T c_cast(S value) {
  using NP = std::remove_pointer_t<S>;

  return reinterpret_cast<T>(const_cast<std::remove_cv_t<NP>*>(value));
}

template <typename T, typename S,
          typename std::enable_if_t<!std::is_pointer_v<S>>* = nullptr>
T c_cast(S value) {
  return reinterpret_cast<T>(value);
}

template <typename T, typename S,
          std::enable_if_t<std::is_integral_v<T>, T>* = nullptr,
          std::enable_if_t<std::is_integral_v<S>, S>* = nullptr>
T int_cast(const S& value) {
  T t_cast = static_cast<T>(value);

  DCPL_CHECK_EQ(value, static_cast<S>(t_cast));

  return t_cast;
}

template <typename T>
std::span<char> to_string(const T& value, std::span<char> buf,
                          std::enable_if_t<std::is_integral_v<T>, T>* = nullptr) {
  typename std::make_unsigned<T>::type uvalue;

  if constexpr (std::is_signed_v<T>) {
    uvalue = value >= 0 ? value : -value;
  } else {
    uvalue = value;
  }

  DCPL_ASSERT(!buf.empty());

  char* base = buf.data();
  char* ptr = base + buf.size();

  do {
    --ptr;
    *ptr = "0123456789"[uvalue % 10];
    uvalue /= 10;
  } while (uvalue != 0 && ptr > base);
  DCPL_ASSERT(uvalue == 0) << "Exceeded buffer size";

  if constexpr (std::is_signed_v<T>) {
    if (value < 0) {
      DCPL_CHECK_GT(ptr, base) << "Exceeded buffer size";
      --ptr;
      *ptr = '-';
    }
  }

  return { ptr, static_cast<std::size_t>(buf.size() - (ptr - base)) };
}

bool is_prime(std::size_t n);

std::size_t next_prime(std::size_t n);

template <typename T, typename S>
T round_up(T value, S step) {
  T tstep = static_cast<T>(step);

  return ((value + tstep - 1) / tstep) * tstep;
}

template <typename T>
constexpr T qpow(T base, int n) {
  if (n == 0) {
    return 1;
  }

  T result = base;

  for (--n; n > 0; --n) {
    result *= base;
  }

  return result;
}

std::fstream open(const std::string& path, std::ios::openmode mode);

template <typename T>
std::streampos stream_size(T* stream) {
  std::streampos pos = stream->tellg();

  stream->seekg(0, std::ios::end);

  std::streampos size = stream->tellg();

  stream->seekg(pos, std::ios::beg);

  return size;
}

template <typename T>
bool eof(T* stream) {
  std::streampos pos = stream->tellg();

  stream->seekg(0, std::ios::end);

  std::streampos size = stream->tellg();

  stream->seekg(pos, std::ios::beg);

  return size == pos;
}

template<typename T>
void sink(const T& value) {
  volatile T no_discard = value;
}

template <typename T>
T vload(const void* ptr) {
  return *reinterpret_cast<const T*>(ptr);
}

template <typename T>
void vstore(void* ptr, const T& value) {
  *reinterpret_cast<T*>(ptr) = value;
}

template <typename D, typename C, typename F>
void enum_splits(D data, C sep, const F& enum_fn) {
  D enum_data(data);

  while (!enum_data.empty()) {
    typename D::size_type nlpos = enum_data.find(sep);

    if (nlpos == D::npos) {
      nlpos = enum_data.size();
    }

    D line = enum_data.substr(0, nlpos);

    enum_fn(line);

    enum_data.remove_prefix(std::min(nlpos + 1, enum_data.size()));
  }
}

template <typename F>
void enum_lines(std::string_view data, const F& enum_fn) {
  enum_splits(data, '\n', enum_fn);
}

template <typename T = std::string_view>
std::vector<T> split_line(std::string_view data, char sep) {
  std::vector<T> splits;

  enum_splits(data, sep, [&](std::string_view part) {
    splits.emplace_back(part);
  });

  return splits;
}

template <typename T>
auto pop_back(T* vec) {
  auto value(std::move(vec->back()));

  vec->pop_back();
  return value;
}

template <typename T>
auto to_span(const T& data, std::size_t offset = 0,
             std::size_t size = dcpl::consts::all) {
  DCPL_CHECK_LE(offset, data.size());

  std::size_t ssize = std::min<std::size_t>(data.size() - offset, size);

  return std::span{ data.data() + offset, ssize };
}

}

