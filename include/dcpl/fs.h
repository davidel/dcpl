#pragma once

#include <filesystem>
#include <string>

namespace dcpl {

namespace fs = std::filesystem;

std::string temp_path(const fs::path& path);

std::string temp_path();

}

