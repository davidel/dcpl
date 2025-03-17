#pragma once

#include <filesystem>
#include <string>

namespace stdfs = std::filesystem;

namespace dcpl {
namespace fs {

void remove(const std::string& path);

void create_directory(const std::string& path);

void create_directories(const std::string& path);

void remove_all(const std::string& path);

}

std::string get_temp_path(const std::string& path);

std::string get_temp_path();

}

