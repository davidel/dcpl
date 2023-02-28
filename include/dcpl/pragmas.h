#pragma once

#if defined(_MSC_VER)

#define DCPL_WARNPUSH __pragma(warning(push))
#define DCPL_WARNPOP __pragma(warning(pop))
#define DCPL_WARNOFF(...) __pragma(warning(disable : __VA_ARGS__))

#define DCPL_WARNOFF_UNUSED_PARAM DCPL_WARNOFF(4100)
#define DCPL_WARNOFF_UNUSED_FUNCTION DCPL_WARNOFF(4505)
#define DCPL_WARNOFF_SIGN_COMPARE DCPL_WARNOFF(4388, 4389)

#elif defined(__GNUC__) || defined(__clang__)

#define DCPL_PRAGMA(X) _Pragma(#X)
#define DCPL_WARNPUSH DCPL_PRAGMA(GCC diagnostic push)
#define DCPL_WARNPOP DCPL_PRAGMA(GCC diagnostic pop)
#define DCPL_WARNOFF(wname) DCPL_PRAGMA(GCC diagnostic ignored #wname)

#define DCPL_WARNOFF_UNUSED_PARAM DCPL_WARNOFF(-Wunused-parameter)
#define DCPL_WARNOFF_UNUSED_FUNCTION DCPL_WARNOFF(-Wunused-function)
#define DCPL_WARNOFF_SIGN_COMPARE DCPL_WARNOFF(-Wsign-compare)

#else

#define DCPL_WARNPUSH
#define DCPL_WARNPOP
#define DCPL_WARNOFF_UNUSED_PARAM
#define DCPL_WARNOFF_UNUSED_FUNCTION
#define DCPL_WARNOFF_SIGN_COMPARE

#endif

