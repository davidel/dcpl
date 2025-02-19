#pragma once

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <type_traits>
#include <random>
#include <vector>

// NOTE: Should not have any local dependency!

namespace dcpl {

using bitmap = std::vector<bool>;

using rnd_generator = std::mt19937_64;

using int_t = std::ptrdiff_t;
using uint_t = typename std::make_unsigned<int_t>::type;

using maxint_t = __int128;
using umaxint_t = unsigned __int128;

using ssize_t = std::ptrdiff_t;

static constexpr std::size_t MAXINT_NBITS = CHAR_BIT * sizeof(maxint_t);

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

