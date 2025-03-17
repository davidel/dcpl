#pragma once

#include <optional>
#include <string>

#include "dcpl/fs.h"

namespace dcpl {

class temp_path {
 public:
  explicit temp_path(std::optional<std::string> path = std::nullopt);

  ~temp_path();

  const std::string& path() const {
    return path_;
  }

  operator std::string() const {
    return path_;
  }

  operator const char*() const {
    return path_.c_str();
  }

  operator stdfs::path() const {
    return path_;
  }

  void release() {
    released_ = true;
  }

 private:
  std::string path_;
  bool released_ = false;
};

}

