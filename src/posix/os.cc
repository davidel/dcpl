#include "dcpl/os.h"

#include <math.h>

namespace dcpl {
namespace os {

pid_t getpid() {
  return ::getpid();
}

std::tm localtime(std::time_t epoc_time) {
  std::tm time_data;

  ::localtime_r(&epoc_time, &time_data);

  return time_data;
}

}

}

