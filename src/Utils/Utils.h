#pragma once

#include <string>

namespace Utils
{
    // Normalize file paths by replacing Windows-style backslashes with forward slashes
    std::string NormalizePath(const std::string &path);
}
