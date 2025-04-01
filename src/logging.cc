#include "dcpl/logging.h"

#include <array>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/env.h"
#include "dcpl/format.h"
#include "dcpl/fs.h"
#include "dcpl/os.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {
namespace logging {
namespace {

static constexpr int lid_size = 1;

const std::array<const char*, 7> lev_to_name{
  "SPAM",
  "VERBOSE",
  "DEBUG",
  "INFO",
  "WARNING",
  "ERROR",
  "CRITICAL"
};

int next_sinkfn_id = 1;
std::map<int, logger::sink_fn> sinks;
std::shared_mutex sinks_lock;

std::string_view get_level_id(int level, char lid[lid_size]) {
  static_assert(LEVEL_SPACE <= 10);
  static_assert(lid_size == 1);

  DCPL_CHECK_GE(level, LEVEL_MIN);
  DCPL_CHECK_LT(level, LEVEL_MAX);

  int base = level / LEVEL_SPACE;
  const char* name = lev_to_name.at(base);

  lid[0] = name[0];

  return { lid, lid_size };
}

int parse_level(std::string_view level_str) {
  std::string_view::size_type dpos = level_str.find(':');
  int offset = 0;

  if (dpos != std::string_view::npos) {
    offset = to_number<int>(level_str.substr(dpos + 1));
    level_str = level_str.substr(0, dpos);
  }

  int base = 0;

  for (auto name : lev_to_name) {
    if (level_str == name) {
      break;
    }
    base += LEVEL_SPACE;
  }

  int level = base + offset;

  DCPL_CHECK_GE(level, LEVEL_MIN);
  DCPL_CHECK_LT(level, LEVEL_MAX);

  return level;
}

}

logger::~logger() {
  std::string hdr = create_header();
  std::string msg = ss_.str();
  auto log_fn = [&](std::string_view msg) {
    if (stderr_log) {
      std::cerr << hdr << msg << "\n";
    }

    for (const auto& [sid, sinkfn] : sinks) {
      sinkfn(hdr, msg);
    }
  };

  std::shared_lock guard(sinks_lock);

  enum_lines(msg, log_fn);
}

std::string logger::create_header() const {
  constexpr long s2nano = 1000000000;
  std::stringstream ss;
  char lid[lid_size] = {};

  ss << get_level_id(level_, lid);

  ns_time now = nstime();

  ss << format_time("%Y%m%d %H:%M:%S", static_cast<std::time_t>(now.count() / s2nano))
     << format(".%06ld", static_cast<long>((now.count() % s2nano) / 1000))
     << " " << os::getpid() << " " << basename(path_) << ":" << lineno_ << "] ";

  return ss.str();
}

void logger::setup(int* argc, char** argv) {
  std::optional<std::string> log_level = getenv_arg(argc, argv, "dcpl_log_level");

  if (log_level) {
    current_level = parse_level(*log_level);
  }

  stderr_log = getenv_arg<bool>(argc, argv, "dcpl_stderr_log", true);

  std::optional<std::string> log_paths = getenv_arg(argc, argv, "dcpl_log_paths");

  if (log_paths) {
    std::vector<std::string> paths = split_line<std::string>(*log_paths, ';');

    for (const auto& path : paths) {
      std::shared_ptr<std::fstream> logfile =
          std::make_shared<std::fstream>(open(path, std::ios::out | std::ios::ate));

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
  std::unique_lock guard(sinks_lock);
  int sid = next_sinkfn_id;

  sinks.emplace(sid, std::move(sinkfn));
  ++next_sinkfn_id;

  return sid;
}

void logger::unregister_sink(int sid) {
  std::unique_lock guard(sinks_lock);

  sinks.erase(sid);
}

}

}

