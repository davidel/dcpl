#include "dcpl/os.h"

#include <time.h>
#include <unistd.h>

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

std::size_t page_size() {
  return static_cast<std::size_t>(::getpagesize());
}

}

}

