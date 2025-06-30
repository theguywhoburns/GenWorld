#pragma once

#include <string>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif

#if !defined(_glfw3_native_h_)
#include <GLFW/glfw3native.h>
#endif

namespace Utils {
    class FileDialogs {
    public:
        // These return empty strings if cancelled
        static std::string OpenFile(const char* title, const char* filter, GLFWwindow* window = nullptr);
        static std::string SaveFile(const char* title, const char* filter, GLFWwindow* window = nullptr);
        // Utility function to normalize file paths
        static std::string NormalizePath(const std::string& path) {
            std::string normalized = path;
            // Replace Windows-style backslashes with forward slashes
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            return normalized;
        }
    };
}