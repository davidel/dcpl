#pragma once

#include <unistd.h>

#include <cstdint>
#include <cstddef>
#include <ctime>

namespace dcpl {
namespace os {

using pid_t = ::pid_t;

pid_t getpid();

std::tm localtime(std::time_t epoc_time);

std::size_t page_size();

}

}

