#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace dcpl::logging {

constexpr int LEVEL_SPACE = 10;

constexpr int SPAM = 0;
constexpr int VERBOSE = SPAM + LEVEL_SPACE;
constexpr int DEBUG = VERBOSE + LEVEL_SPACE;
constexpr int INFO = DEBUG + LEVEL_SPACE;
constexpr int WARNING = INFO + LEVEL_SPACE;
constexpr int ERROR = WARNING + LEVEL_SPACE;
constexpr int CRITICAL = ERROR + LEVEL_SPACE;

constexpr int LEVEL_MIN = SPAM;
constexpr int LEVEL_MAX = CRITICAL + LEVEL_SPACE;

struct noop { };

class logger : public noop {
 public:
  using sink_fn = std::function<void(std::string_view, std::string_view)>;

  inline static int current_level = INFO;
  inline static bool stderr_log = true;

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

  static void setup(int* argc, char** argv);

  static int register_sink(sink_fn sinkfn);

  static void unregister_sink(int sid);

 private:
  std::string create_header() const;

  const char* path_ = nullptr;
  int lineno_ = -1;
  int level_ = SPAM;
  std::stringstream ss_;
};

}

namespace dcplog = dcpl::logging;

#if defined(NDEBUG)
#define DCPL_LOGDEBUG false
#else // #if defined(NDEBUG)
#define DCPL_LOGDEBUG true
#endif // #if defined(NDEBUG)

#define DCPL_LOG(level) ((level) < dcplog::logger::current_level) ?     \
  dcplog::noop() : dcplog::logger(__FILE__, __LINE__, level)

#define DCPL_LOGD(level) (!DCPL_LOGDEBUG || (level) < dcplog::logger::current_level) ? \
  dcplog::noop() : dcplog::logger(__FILE__, __LINE__, level)

#define DCPL_SLOG() DCPL_LOG(dcplog::SPAM)
#define DCPL_VLOG() DCPL_LOG(dcplog::VERBOSE)
#define DCPL_DLOG() DCPL_LOG(dcplog::DEBUG)
#define DCPL_ILOG() DCPL_LOG(dcplog::INFO)
#define DCPL_WLOG() DCPL_LOG(dcplog::WARNING)
#define DCPL_ELOG() DCPL_LOG(dcplog::ERROR)
#define DCPL_CLOG() DCPL_LOG(dcplog::CRITICAL)

#define DCPL_SLOGD() DCPL_LOGD(dcplog::SPAM)
#define DCPL_VLOGD() DCPL_LOGD(dcplog::VERBOSE)
#define DCPL_DLOGD() DCPL_LOGD(dcplog::DEBUG)
#define DCPL_ILOGD() DCPL_LOGD(dcplog::INFO)
#define DCPL_WLOGD() DCPL_LOGD(dcplog::WARNING)
#define DCPL_ELOGD() DCPL_LOGD(dcplog::ERROR)
#define DCPL_CLOGD() DCPL_LOGD(dcplog::CRITICAL)

