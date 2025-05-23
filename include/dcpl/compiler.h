#pragma once

#include <string>

#if defined(__GNUC__) || defined(__clang__)

#include <cxxabi.h>

#include <cstdlib>

namespace dcpl {

static inline std::string demangle(const char* name) {
  std::string tname;
  int status = -1;
  char *demangled_name = abi::__cxa_demangle(name, NULL, NULL, &status);

  if (status == 0) {
    tname = demangled_name;
    std::free(demangled_name);
  } else {
    tname = name;
  }

  return tname;
}

template<typename T>
std::string type_name() {
  return demangle(typeid(T).name());
}

}

#define DCPL_LIKELY(cond) __builtin_expect(!!(cond), true)
#define DCPL_UNLIKELY(cond) __builtin_expect(!!(cond), false)

#define DCPL_INLINE inline __attribute__((always_inline))

#else // defined(__GNUC__) || defined(__clang__)

namespace dcpl {

static inline std::string demangle(const char* name) {
  return name;
}

template<typename T>
std::string type_name() {
  return typeid(T).name();
}

}

#define DCPL_LIKELY(cond) (cond)
#define DCPL_UNLIKELY(cond) (cond)

// This could be __forceinline if MSVC.
#define DCPL_INLINE inline

#endif // defined(__GNUC__) || defined(__clang__)

#define DCPL_BEGIN_PACKED _Pragma("pack(push, 1)")
#define DCPL_END_PACKED _Pragma("pack(pop)")

