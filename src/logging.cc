#include "dcpl/logging.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/env.h"
#include "dcpl/file.h"
#include "dcpl/os.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {
namespace logging {
namespace {

static constexpr int lid_size = 2;

std::map<int, const char*> lev_to_name{
  { SPAM, "SPAM" },
  { VERBOSE, "VERBOSE" },
  { DEBUG, "DEBUG" },
  { INFO, "INFO" },
  { WARNING, "WARNING" },
  { ERROR, "ERROR" },
  { CRITICAL, "CRITICAL" }
};

std::once_flag init_flag;
int next_sinkfn_id = 1;
std::map<int, logger::sink_fn> sinks;
std::mutex sinks_lock;

std::string_view get_level_id(int level, char lid[lid_size]) {
  static_assert(LEVEL_SPACE <= 10);
  static_assert(lid_size == 2);

  int base = (level / LEVEL_SPACE) * LEVEL_SPACE;
  int offset = level % LEVEL_SPACE;
  const char* name = lev_to_name.at(base);

  lid[0] = name[0];
  if (offset == 0) {
    lid[1] = name[1];
  } else {
    lid[1] = "0123456789"[offset];
  }

  return { lid, lid_size };
}

void stderr_logger(std::string_view hdr, std::string_view msg) {
  std::cerr << hdr << msg << "\n";
}

}

logger::~logger() {
  std::call_once(init_flag, []() { logger::init(); });

  std::string hdr = create_header();
  std::string msg = ss_.str();
  auto log_fn = [&](std::string_view msg) {
    std::lock_guard guard(sinks_lock);

    for (const auto& [sid, sinkfn] : sinks) {
      sinkfn(hdr, msg);
    }
  };

  enum_lines(msg, log_fn);
}

std::string logger::create_header() const {
  constexpr long s2nano = 1000000000;
  std::stringstream ss;
  char lid[lid_size] = {};

  ss << get_level_id(level_, lid);

  auto now = nstime();
  std::tm time_data = os::localtime(static_cast<std::time_t>(now / s2nano));
  char time_buffer[32];

  std::strftime(time_buffer, sizeof(time_buffer), "%Y%m%d %H:%M:%S", &time_data);

  char us_buffer[16];

  std::snprintf(us_buffer, sizeof(us_buffer), "%06ld",
                static_cast<long>((now % s2nano) / 1000));

  const char* fname = std::strrchr(path_, '/');

  ss << time_buffer << "." << us_buffer  << "\t" << os::getpid() << " "
     << (fname != nullptr ? fname + 1 : path_) << ":" << lineno_ << "] ";

  return ss.str();
}

void logger::init() {
  int level = getenv<int>("DCPL_LOG_LEVEL", INFO);

  DCPL_CHECK_GE(level, LEVEL_MIN);
  DCPL_CHECK_LT(level, LEVEL_MAX);

  logger::current_level = level;

  if (getenv<int>("DCPL_STDERR_LOG", 1) != 0) {
    register_sink(stderr_logger);
  }

  std::optional<std::string> log_paths = getenv("DCPL_LOG_PATHS");

  if (log_paths) {
    std::vector<std::string> paths = split_line<std::string>(*log_paths, ';');

    for (const auto& path : paths) {
      std::shared_ptr<file> logfile =
          std::make_shared<file>(path, file::open_write | file::open_create |
                                 file::open_append);
      auto logfn = [logfile](std::string_view hdr, std::string_view msg) {
        logfile->write(hdr.data(), hdr.size());
        logfile->write(msg.data(), msg.size());
        logfile->write("\n", 1);
      };

      register_sink(std::move(logfn));
    }
  }
}

int logger::register_sink(sink_fn sinkfn) {
  std::lock_guard guard(sinks_lock);
  int sid = next_sinkfn_id;

  sinks.emplace(sid, std::move(sinkfn));
  ++next_sinkfn_id;

  return sid;
}

void logger::unregister_sink(int sid) {
  std::lock_guard guard(sinks_lock);

  sinks.erase(sid);
}

}

}

