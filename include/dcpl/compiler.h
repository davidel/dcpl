#pragma once

#if defined(__GNUC__) || defined(__clang__)

#define DCPL_LIKELY(cond) __builtin_expect(!!(cond), true)
#define DCPL_UNLIKELY(cond) __builtin_expect(!!(cond), false)

#else

#define DCPL_LIKELY(cond) (cond)
#define DCPL_UNLIKELY(cond) (cond)

#endif

