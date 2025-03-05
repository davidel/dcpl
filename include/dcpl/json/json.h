#pragma once

// Use the system installed one if available, otherwise the bundled one.
#if __has_include(<nlohmann/json.hpp>)

// sudo apt install nlohmann-json3-dev
#include <nlohmann/json.hpp>

#else // __has_include(<nlohmann/json.hpp>)

#include "json_lib.h"

#endif // __has_include(<nlohmann/json.hpp>)

namespace dcpl {

namespace json = nlohmann;

}

