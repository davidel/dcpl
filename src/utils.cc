#include "dcpl/utils.h"

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

std::span<const char> to_span(const std::string& str) {
  return { str.c_str(), str.size() };
}

std::span<const char> to_span(const char* str) {
  return { str, std::strlen(str) };
}

}

