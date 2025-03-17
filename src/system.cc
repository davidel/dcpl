#include "dcpl/system.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>

#include "dcpl/assert.h"

namespace dcpl {

std::size_t page_size() {
  return static_cast<std::size_t>(::getpagesize());
}

}

