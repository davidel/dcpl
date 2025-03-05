#pragma once

// Use the system installed one if available, otherwise the bundled one.
#if __has_include(<nlohmann/json.hpp>)

// sudo apt install nlohmann-json3-dev
#include <nlohmann/json.hpp>

#else // __has_include(<nlohmann/json.hpp>)

// To update:
//
//  wget -O json_lib.h https://raw.githubusercontent.com/nlohmann/json/refs/heads/develop/single_include/nlohmann/json.hpp
#include "json_lib.h"

#endif // __has_include(<nlohmann/json.hpp>)

namespace dcpl {

namespace json = nlohmann;

}

