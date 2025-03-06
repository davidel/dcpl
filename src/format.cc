#include "dcpl/format.h"

#include <cinttypes>
#include <cstdlib>
#include <sstream>

#include "dcpl/assert.h"

namespace dcpl {

std::string format_duration(double elapsed) {
  DCPL_CHECK_GE(elapsed, 0.0) << "Duration must be non negative";

  std::intmax_t msec_time = static_cast<std::intmax_t>(elapsed * 1000.0);
  std::imaxdiv_t dr;

  dr = std::imaxdiv(msec_time, 1000);
  unsigned int msecs = static_cast<unsigned int>(dr.rem);

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
  ss << format("%02us:", secs) << msecs;

  return ss.str();
}

}

