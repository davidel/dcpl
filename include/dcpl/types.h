#pragma once

#include <climits>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <ios>
#include <type_traits>
#include <random>
#include <vector>

// NOTE: Should not have any local dependency!

namespace dcpl {

template <typename T>
constexpr std::size_t bit_sizeof() {
  return sizeof(T) * CHAR_BIT;
}

using bitmap = std::vector<bool>;

using rnd_generator = std::mt19937_64;

using int_t = std::ptrdiff_t;
using uint_t = typename std::make_unsigned<int_t>::type;

#if defined(INT128_MAX)

using maxint_t = std::int128_t;
using umaxint_t = std::uint128_t;

#elif defined(__GLIBCXX_TYPE_INT_N_0) // ifdef INT128_MAX

using maxint_t = __GLIBCXX_TYPE_INT_N_0;
using umaxint_t = unsigned __GLIBCXX_TYPE_INT_N_0;

#else // ifdef INT128_MAX

using maxint_t = std::intmax_t;
using umaxint_t = std::uintmax_t;

#endif // ifdef INT128_MAX

using ssize_t = std::ptrdiff_t;

using fileoff_t = std::streamoff;

static constexpr std::size_t MAXINT_NBITS = bit_sizeof<maxint_t>();

template <typename T>
using ilist = std::initializer_list<T>;

struct cstr_equal {
  bool operator()(const char* a, const char* b) const {
    return std::strcmp(a, b) == 0;
  }
};

struct cstr_less {
  bool operator()(const char* a, const char* b) const {
    return std::strcmp(a, b) < 0;
  }
};

}

