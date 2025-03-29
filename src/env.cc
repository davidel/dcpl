#include "dcpl/env.h"

#include <cstring>

#include "dcpl/core_utils.h"

namespace dcpl {

std::string getenv(const char* name, std::string defval) {
  char* env = std::getenv(name);

  return (env != nullptr) ? std::string(env) : std::move(defval);
}

std::optional<std::string> getenv(const char* name) {
  char* env = std::getenv(name);

  if (env == nullptr) {
    return std::nullopt;
  }

  return std::string(env);
}

std::optional<std::string> getenv_arg(int* argc, char** argv, const char* name) {
  for (int i = 1; i < *argc; ++i) {
    char* arg = argv[i];

    if (*arg == '-') {
      arg = (arg[1] == '-') ? arg + 2 : arg + 1;

      if (std::strcmp(arg, name) == 0) {
        int num_args = 1;
        std::string param_str;

        if (i + 1 >= *argc) {
          param_str = "1";
        } else {
          char* param = argv[i + 1];

          if (*param != '-') {
            param_str = param;
            ++num_args;
          } else {
            param_str = "1";
          }
        }

        for (int j = i; j + num_args < *argc; ++j) {
          argv[j] = argv[j + num_args];
        }

        *argc -= num_args;

        return std::move(param_str);
      }
    }
  }

  std::string env_name = to_upper(name);

  return getenv(env_name.c_str());
}

}

