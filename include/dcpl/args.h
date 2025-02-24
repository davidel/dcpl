#pragma once

#include <any>
#include <cstdint>
#include <exception>
#include <map>
#include <string>
#include <vector>

#include "dcpl/compiler.h"
#include "dcpl/core_utils.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {

class args {
 public:
  using bool_t = bool;
  using boolv_t = std::vector<bool>;
  using int_t = dcpl::maxint_t;
  using intv_t = std::vector<dcpl::maxint_t>;
  using uint_t = dcpl::umaxint_t;
  using uintv_t = std::vector<dcpl::umaxint_t>;
  using float_t = float;
  using floatv_t = std::vector<float>;
  using double_t = double;
  using doublev_t = std::vector<double>;
  using string_t = std::string;
  using stringv_t = std::vector<std::string>;

  class parsed {
   public:
    parsed(std::map<std::string, std::any> args,
           std::vector<std::string> unargs) :
        args_(std::move(args)),
        unargs_(std::move(unargs)) {
    }

    template <typename T>
    const T* find(const std::string& name) const {
      const T* value = nullptr;
      auto ait = args_.find(name);

      if (ait != args_.end() && ait->second.has_value()) {
        value = std::any_cast<const T>(&ait->second);

        DCPL_CHECK_NE(value, nullptr) << "Requested wrong type ("
                                      << dcpl::type_name<T>() << ") for flag: " << name;
      }

      return value;
    }

    template <typename T>
    const T* get(const std::string& name) const {
      const T* value = find<T>(name);

      DCPL_CHECK_NE(value, nullptr) << "Unable to find flag: " << name;

      return value;
    }

    const std::vector<std::string>& unargs() const {
      return unargs_;
    }

   private:
    std::map<std::string, std::any> args_;
    std::vector<std::string> unargs_;
  };

  struct global_base { };

  template <typename T>
  struct global : public global_base {
    global(std::string name, std::string help) {
      g_global_flags.emplace_back(std::move(name), dcpl::type_name<T>(),
                                  std::any(), std::move(help));
    }

    global(std::string name, T defval, std::string help) {
      g_global_flags.emplace_back(std::move(name), dcpl::type_name<T>(),
                                  std::move(defval), std::move(help));
    }
  };

  template <typename T>
  void add(std::string name, std::string help) {
    flags_.emplace_back(std::move(name), dcpl::type_name<T>(), std::any(),
                        std::move(help));
  }

  template <typename T>
  void add(std::string name, T defval, std::string help) {
    flags_.emplace_back(std::move(name), dcpl::type_name<T>(), std::move(defval),
                        std::move(help));
  }

  parsed parse(int argc, char** argv) const;

 private:
  struct flag {
    std::string name;
    std::string type;
    std::any defval;
    std::string help;
  };

  std::map<std::string_view, const flag*> collect_args() const;

  static std::any parse_range(char** argv, int pos, int count, const flag& fvalue);

  std::vector<flag> flags_;
  static std::vector<flag> g_global_flags;
};

}

