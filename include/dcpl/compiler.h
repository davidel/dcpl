#pragma once

#if defined(__GNUC__) || defined(__clang__)

#include <cxxabi.h>

namespace dcpl {

template<typename T>
std::string type_name() {
  std::string tname = typeid(T).name();
  int status = -1;
  char *demangled_name = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);

  if (status == 0) {
    tname = demangled_name;
    std::free(demangled_name);
  }

  return tname;
}

}

#define DCPL_LIKELY(cond) __builtin_expect(!!(cond), true)
#define DCPL_UNLIKELY(cond) __builtin_expect(!!(cond), false)

#else

namespace dcpl {

template<typename T>
std::string type_name() {
  return typeid(T).name();
}

}

#define DCPL_LIKELY(cond) (cond)
#define DCPL_UNLIKELY(cond) (cond)

#endif

