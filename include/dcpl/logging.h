#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace dcpl {
namespace logging {

constexpr int LEVEL_SPACE = 10;

constexpr int SPAM = 0;
constexpr int VERBOSE = SPAM + LEVEL_SPACE;
constexpr int DEBUG = VERBOSE + LEVEL_SPACE;
constexpr int INFO = DEBUG + LEVEL_SPACE;
constexpr int WARNING = INFO + LEVEL_SPACE;
constexpr int ERROR = WARNING + LEVEL_SPACE;
constexpr int CRITICAL = ERROR + LEVEL_SPACE;

struct noop { };

class logger : public noop {
 public:
  using sink_fn = std::function<void(std::string_view, std::string_view)>;

  inline static int current_level = INFO;

  logger(const char* path, int lineno, int level) :
      path_(path),
      lineno_(lineno),
      level_(level) {
  }

  ~logger();

  template <typename T>
  logger& operator<<(const T& value) {
    ss_ << value;

    return *this;
  }

  static int register_sink(sink_fn sinkfn);

  static void unregister_sink(int sid);

 private:
  std::string create_header() const;

  static void init();

  const char* path_ = nullptr;
  int lineno_ = -1;
  int level_ = SPAM;
  std::stringstream ss_;
};

}

#if defined(NDEBUG)
#define DCPL_LOGDEBUG false
#else // #if defined(NDEBUG)
#define DCPL_LOGDEBUG true
#endif // #if defined(NDEBUG)

#define LOG(level) ((level) < dcpl::logging::logger::current_level) ?   \
  dcpl::logging::noop() : dcpl::logging::logger(__FILE__, __LINE__, level)

#define DLOG(level) (!DCPL_LOGDEBUG || (level) < dcpl::logging::logger::current_level) ? \
  dcpl::logging::noop() : dcpl::logging::logger(__FILE__, __LINE__, level)

}

