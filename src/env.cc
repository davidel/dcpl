#include "dcpl/env.h"

namespace dcpl {

std::string getenv(const std::string& name, std::string defval) {
  char* env = std::getenv(name.c_str());

  return (env != nullptr) ? std::string(env) : std::move(defval);
}

std::optional<std::string> getenv(const std::string& name) {
  char* env = std::getenv(name.c_str());

  if (env == nullptr) {
    return std::nullopt;
  }

  return std::string(env);
}

}

