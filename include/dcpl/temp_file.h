#pragma once

#include <fstream>
#include <optional>
#include <string>

namespace dcpl {

class temp_file {
 public:
  explicit temp_file(std::ios::openmode mode,
                     std::optional<std::string> path = std::nullopt);

  ~temp_file();

  const std::string& path() const {
    return path_;
  }

  std::fstream& file() {
    return file_;
  }

  void release() {
    released_ = true;
  }

 private:
  std::string path_;
  std::fstream file_;
  bool released_ = false;
};

}

