#pragma once

#include <filesystem>
#include <string>
#include <string_view>

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

// No-alloc versions of the stdfs::path ones ...
std::string_view basename(std::string_view path);
std::string_view dirname(std::string_view path);

}

