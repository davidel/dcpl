#include "dcpl/format.h"

#include <cinttypes>
#include <cstdlib>
#include <sstream>

#include "dcpl/core_utils.h"

namespace dcpl {

std::string format_duration(double elapsed) {
  DCPL_CHECK_GE(elapsed, 0.0) << "Duration must be non negative";

  std::intmax_t usec_time = static_cast<std::intmax_t>(elapsed * 1e6);
  std::imaxdiv_t dr;

  dr = std::imaxdiv(usec_time, 1000000);
  unsigned int usecs = static_cast<unsigned int>(dr.rem);

  dr = std::imaxdiv(dr.quot, 60);
  unsigned int secs = static_cast<unsigned int>(dr.rem);

  dr = std::imaxdiv(dr.quot, 60);
  unsigned int mins = static_cast<unsigned int>(dr.rem);

  dr = std::imaxdiv(dr.quot, 24);
  unsigned int hours = static_cast<unsigned int>(dr.rem);
  unsigned int days = static_cast<unsigned int>(dr.quot);

  std::stringstream ss;

  if (days > 0) {
    ss << days << "d:";
  }
  if (hours > 0) {
    ss << format("%02uh:", hours);
  }
  if (mins > 0) {
    ss << format("%02um:", mins);
  }
  ss << format("%02us:", secs) << format("%06ju", usecs);

  return ss.str();
}

std::string format_bytes(umaxint_t size) {
  static constexpr double one_tb = qpow<double>(1024.0, 4);
  static constexpr double one_gb = qpow<double>(1024.0, 3);
  static constexpr double one_mb = qpow<double>(1024.0, 2);
  static constexpr double one_kb = qpow<double>(1024.0, 1);

  double dbl_size = static_cast<double>(size);
  std::stringstream ss;

  if (dbl_size >= one_tb) {
    ss << format("%.2f TB", dbl_size / one_tb);
  } else if (dbl_size >= one_gb) {
    ss << format("%.2f GB", dbl_size / one_gb);
  } else if (dbl_size >= one_mb) {
    ss << format("%.2f MB", dbl_size / one_mb);
  } else if (dbl_size >= one_kb) {
    ss << format("%.2f KB", dbl_size / one_kb);
  } else {
    ss << format("%.2f B", dbl_size);
  }

  return ss.str();
}

}

