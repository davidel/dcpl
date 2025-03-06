#pragma once

#include <fstream>
#include <string>

namespace dcpl {

class atomic_write {
 public:
  atomic_write(std::string path, std::ios::openmode mode);

  ~atomic_write();

  const std::string& path() const {
    return path_;
  }

  std::fstream& file() {
    return file_;
  }

  void commit();

 private:
  std::string path_;
  std::string temp_path_;
  std::fstream file_;
  bool committed_ = false;
};

}

