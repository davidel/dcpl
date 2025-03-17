#pragma once

#include <fstream>
#include <optional>
#include <string>

#include "dcpl/temp_path.h"

namespace dcpl {

class temp_file {
 public:
  explicit temp_file(std::ios::openmode mode,
                     std::optional<std::string> path = std::nullopt);

  ~temp_file();

  const std::string& path() const {
    return path_.path();
  }

  std::fstream& file() {
    return file_;
  }

  void close() {
    if (!closed_) {
      file_.close();
      closed_ = true;
    }
  }

  void release() {
    path_.release();
  }

 private:
  temp_path path_;
  std::fstream file_;
  bool closed_ = false;
};

}

