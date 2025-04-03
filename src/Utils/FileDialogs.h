#pragma once
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
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