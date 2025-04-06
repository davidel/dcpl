#pragma once

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

// Limited local dependecies! Both compiler.h and pragmas.h have no dependecies at all.
#include "dcpl/compiler.h"
#include "dcpl/pragmas.h"

namespace dcpl {
namespace detail {

struct base_asserter { };

template <typename X = std::runtime_error>
struct asserter : public base_asserter {
  explicit asserter(const char* msg, bool needs_separator = true) :
      needs_separator(needs_separator) {
    error_stream << msg;
  }

  asserter(asserter&& ref) = default;

  [[noreturn]] ~asserter() noexcept(false) {
    throw X(error_stream.str());
  }

  template <typename T>
  asserter& operator<<(const T& value) {
    if (needs_separator) {
      error_stream << "; ";
      needs_separator = false;
    }
    error_stream << value;
    return *this;
  }

  std::stringstream error_stream;
  bool needs_separator = true;
};

#define DCPL_DEFCHECK(name, op)                                         \
  template <typename T1, typename T2>                                   \
  std::unique_ptr<asserter<>> name##_check(const T1& v1, const T2& v2,  \
                                           const char* s1, const char* s2) { \
    DCPL_WARNPUSH;                                                      \
    DCPL_WARNOFF_SIGN_COMPARE;                                          \
    if (v1 op v2) [[likely]] {                                          \
      return nullptr;                                                   \
    }                                                                   \
    DCPL_WARNPOP;                                                       \
    auto astr = std::make_unique<asserter<>>("Check failed: ",          \
                                             /*needs_separator=*/ false); \
    *astr << s1 << " " #op " " << s2 << " (" << v1 << " " #op " " << v2 << ")"; \
    astr->needs_separator = true;                                       \
    return astr;                                                        \
  }

DCPL_DEFCHECK(EQ, ==);
DCPL_DEFCHECK(NE, !=);
DCPL_DEFCHECK(LT, <);
DCPL_DEFCHECK(LE, <=);
DCPL_DEFCHECK(GT, >);
DCPL_DEFCHECK(GE, >=);

#define DCPL_CHECK_OP(name, v1, v2)                                     \
  while (auto astr = dcpl::detail::name##_check(v1, v2, #v1, #v2))      \
    [[unlikely]] dcpl::detail::asserter<>(std::move(*astr.release()))

}

// There is no way in c++20 to use the [[likely]]/[[unlikely]] attributes
// for ternary expressions. So use DCPL_LIKELY() (which maps to GCC/CLANG
// __builtin_expect() and noop on MSVC) to hint the likely branch.
#define DCPL_ASSERT(cond) DCPL_LIKELY(cond) ? dcpl::detail::base_asserter() : \
  dcpl::detail::asserter<>("Assert failed: " #cond)

#define DCPL_XTHROW(ex) dcpl::detail::asserter<ex>("Exception: ")
#define DCPL_THROW() DCPL_XTHROW(std::runtime_error)

#define DCPL_CHECK_EQ(v1, v2) DCPL_CHECK_OP(EQ, v1, v2)
#define DCPL_CHECK_NE(v1, v2) DCPL_CHECK_OP(NE, v1, v2)
#define DCPL_CHECK_LT(v1, v2) DCPL_CHECK_OP(LT, v1, v2)
#define DCPL_CHECK_LE(v1, v2) DCPL_CHECK_OP(LE, v1, v2)
#define DCPL_CHECK_GT(v1, v2) DCPL_CHECK_OP(GT, v1, v2)
#define DCPL_CHECK_GE(v1, v2) DCPL_CHECK_OP(GE, v1, v2)

}

