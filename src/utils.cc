#include "dcpl/utils.h"

#include <cinttypes>
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

ns_time nstime() {
  std::chrono::time_point<std::chrono::high_resolution_clock>
      now = std::chrono::high_resolution_clock::now();

  return { now.time_since_epoch() };
}

double time() {
  return from_nsecs(nstime());
}

ns_time to_nsecs(double secs) {
  return ns_time{ static_cast<ns_time::rep>(secs * 1e9) };
}

double from_nsecs(ns_time nsecs) {
  std::imaxdiv_t dres = std::imaxdiv(static_cast<std::intmax_t>(nsecs.count()), 10000);

  return static_cast<double>(dres.quot) * 1e-5 + static_cast<double>(dres.rem) * 1e-9;
}

void sleep_for(ns_time nsecs) {
  std::this_thread::sleep_for(nsecs);
}

void sleep_until(ns_time epoch_time) {
  std::this_thread::sleep_until(time_point(epoch_time));
}

}

