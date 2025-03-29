#pragma once

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "dcpl/utils.h"

namespace dcpl {

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
T getenv(const char* name, const T& defval) {
  char* env = std::getenv(name);

  return (env != nullptr) ? to_number<T>(std::string_view(env)) : defval;
}

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
std::optional<T> getenv(const char* name) {
  char* env = std::getenv(name);

  if (env == nullptr) {
    return std::nullopt;
  }

  return to_number<T>(std::string_view(env));
}

std::string getenv(const char* name, std::string defval);

std::optional<std::string> getenv(const char* name);

std::optional<std::string> getenv_arg(int* argc, char** argv, const char* name);

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
T getenv_arg(int* argc, char** argv, const char* name, const T& defval) {
  std::optional<std::string> arg = getenv_arg(argc, argv, name);

  return arg ? to_number<T>(*arg) : defval;
}

template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
std::optional<T> getenv_arg(int* argc, char** argv, const char* name) {
  std::optional<std::string> arg = getenv_arg(argc, argv, name);

  if (!arg) {
    return std::nullopt;
  }

  return to_number<T>(*arg);
}

}

