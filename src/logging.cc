#include "dcpl/logging.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <mutex>

#include "dcpl/core_utils.h"
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

}

logger::~logger() {
  std::call_once(init_flag, []() { logger::init(); });

  std::string hdr = create_header();
  std::string msg = ss_.str();
  auto log_fn = [&](std::string_view line) {
    std::cerr << hdr << line << "\n";

    std::lock_guard guard(sinks_lock);

    for (const auto& [sid, sinkfn] : sinks) {
      sinkfn(hdr, line);
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

  ss << time_buffer << "." << us_buffer  << "\t" << os::getpid() << " "
     << path_ << ":" << lineno_ << "] ";

  return ss.str();
}

void logger::init() {

}

int logger::register_sink(sink_fn sinkfn) {
  int sid;
  {
    std::lock_guard guard(sinks_lock);

    sinks.emplace(next_sinkfn_id, std::move(sinkfn));

    sid = next_sinkfn_id;
    ++next_sinkfn_id;
  }

  return sid;
}

void logger::unregister_sink(int sid) {
  std::lock_guard guard(sinks_lock);

  sinks.erase(sid);
}

}

}

