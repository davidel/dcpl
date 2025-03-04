#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace stdfs = std::filesystem;

namespace dcpl {
namespace fs {

void remove(const std::string& path);

void create_directory(const std::string& path);

void create_directories(const std::string& path);

void remove_all(const std::string& path);

}

std::string temp_path(const std::string& path);

std::string temp_path();

class atomic_write {
 public:
  atomic_write(std::string path, std::ios::openmode mode);

  ~atomic_write();

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

