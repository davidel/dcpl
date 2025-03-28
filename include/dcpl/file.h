#pragma once

#if defined(__unix__)

#include "dcpl/posix/file.h"

#elif defined(_WIN32)

#include "dcpl/windows/file.h"

#endif

