#pragma once
#include <string>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

namespace Utils {
    class FileDialogs {
    public:
        // These return empty strings if cancelled
        static std::string OpenFile(const char* title, const char* filter, GLFWwindow* window = nullptr);
        static std::string SaveFile(const char* title, const char* filter, GLFWwindow* window = nullptr);
    };
}