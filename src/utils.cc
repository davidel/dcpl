#include "dcpl/utils.h"

#include <thread>

namespace dcpl {

std::string_view read_line(std::string_view* data) {
  std::string_view::size_type pos = data->find_first_of('\n');
  std::string_view ln;

  if (pos == std::string_view::npos) {
    ln = *data;
    *data = std::string_view();
  } else {
    ln = std::string_view(data->data(), pos);
    data->remove_prefix(pos + 1);
  }

  return ln;
}

rnd_generator create_rnd_generator() {
  std::random_device rd;

  return rnd_generator(rd());
}

std::string rand_string(std::size_t size, rnd_generator* rgen) {
  static const std::string_view chars{
    "0123456789abcdefghijklmnopqrstuvwxyz"
  };

  rnd_generator_ensure ergen(rgen);
  std::uniform_int_distribution<int> distrib(0, chars.size() - 1);
  std::string result(size, 0);

  for (auto& cref : result) {
    cref = chars[distrib(ergen.get())];
  }

  return result;
}

std::span<const char> to_span(const std::string& str) {
  return { str.c_str(), str.size() };
}

std::span<const char> to_span(const char* str) {
  return { str, std::strlen(str) };
}

nstime_t nstime() {
  std::chrono::time_point<std::chrono::high_resolution_clock>
      now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<nstime_t, std::nano> epoch = now.time_since_epoch();

  return epoch.count();
}

double time() {
  return from_nsecs(nstime());
}

void sleep_for(nstime_t nsecs) {
  std::this_thread::sleep_for(duration(nsecs));
}

void sleep_until(nstime_t epoch_time) {
  std::this_thread::sleep_until(time_point(epoch_time));
}

}

