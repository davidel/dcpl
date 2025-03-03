#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <ostream>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "dcpl/core_utils.h"
#include "dcpl/hash.h"

namespace dcpl {

template <typename T>
inline void vstream(std::ostream& os, const T& v) {
  os << v;
}

template <>
inline void vstream<std::string>(std::ostream& os, const std::string& v) {
  os << "\"" << v << "\"";
}

template <>
inline void vstream<const char*>(std::ostream& os, const char* const& v) {
  os << "\"" << v << "\"";
}

template <typename S>
std::ostream& stream_container(std::ostream& os, const S& cont) {
  os << "(";

  std::size_t count = 0;

  for (const auto& sv : cont) {
    if (count++ > 0) {
      os << ", ";
    }
    vstream(os, sv);
  }
  os << ")";

  return os;
}

template <typename T, std::size_t... Is>
void stream_tuple(std::ostream& os, const T& tp, std::index_sequence<Is...>) {
  ((os << (Is == 0 ? "" : ", "), vstream(os, std::get<Is>(tp))), ...);
}

}

namespace std {

namespace fs = std::filesystem;

template <typename T>
ostream& operator<<(ostream& os, const span<T>& spn) {
  return dcpl::stream_container(os, spn);
}

template <typename T>
ostream& operator<<(ostream& os, const vector<T>& vec) {
  return dcpl::stream_container(os, vec);
}

template<typename... Args>
ostream& operator<<(ostream& os, const tuple<Args...>& tp) {
  os << "(";

  dcpl::stream_tuple(os, tp, index_sequence_for<Args...>{});

  return os << ")";
}

#if defined(__GLIBCXX_TYPE_INT_N_0)

inline ostream& operator<<(ostream& os, __int128 v) {
  char buf[80];
  std::span<char> sbuf = dcpl::to_string(v, buf);

  os.write(sbuf.data(), sbuf.size());

  return os;
}

inline ostream& operator<<(ostream& os, unsigned __int128 v) {
  char buf[80];
  std::span<char> sbuf = dcpl::to_string(v, buf);

  os.write(sbuf.data(), sbuf.size());

  return os;
}

#endif // defined(__GLIBCXX_TYPE_INT_N_0)

template <typename T>
struct hash<span<T>> {
  size_t operator()(span<T> values) const {
    size_t seed = dcpl::hash_combine(0xcc9e2d51, values.size() * 5711);

    for (const auto& v : values) {
      seed = dcpl::hash_combine(seed, v);
    }

    return seed;
  }
};

template <typename T>
bool operator==(span<T> lhs, span<T> rhs) {
  return std::equal(begin(lhs), end(lhs), begin(rhs), end(rhs));
}

}

