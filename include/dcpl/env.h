#pragma once

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "dcpl/utils.h"

namespace dcpl {

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
T genenv(const std::string& name, const T& defval) {
  char* env = std::getenv(name.c_str());

  return (env != nullptr) ? to_number<T>(std::string_view(env)) : defval;
}

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
std::optional<T> genenv(const std::string& name) {
  char* env = std::getenv(name.c_str());

  return (env != nullptr) ? to_number<T>(std::string_view(env)) : std::nullopt;
}

template <>
std::string genenv(const std::string& name, std::string defval) {
  char* env = std::getenv(name.c_str());

  return (env != nullptr) ? std::string(env) : std::move(defval);
}

template <>
std::optional<std::string> genenv(const std::string& name) {
  char* env = std::getenv(name.c_str());

  return (env != nullptr) ? std::string(env) : std::nullopt;
}

}

