#include "Utils.h"
#include <string>
#include <algorithm>

// Utility function to normalize file paths
namespace Utils
{
  std::string NormalizePath(const std::string &path)
  {
    std::string normalized = path;
    // Replace Windows-style backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
  }
}
