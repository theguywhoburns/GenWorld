cmake_minimum_required(VERSION 3.12)
project(GenWorld VERSION 0.1.0 LANGUAGES C CXX)
include(cmake/CPM.cmake)

# For GCC/Clang filesystem linking
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

find_package(OpenGL REQUIRED)

# GLFW
CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
    OPTIONS
    "GLFW_BUILD_DOCS OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_EXAMPLES OFF"
)
CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG v2.0.8
)
add_subdirectory(${glad_SOURCE_DIR}/cmake)
glad_add_library(glad LANGUAGE C API gl:core=3.3)
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
)

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG docking
)
file(
    GLOB imgui_SOURCES
    CONFIGURE_DEPENDS
    ${imgui_SOURCE_DIR}/*.c
    ${imgui_SOURCE_DIR}/*.cpp
    ${imgui_SOURCE_DIR}/backends/*glfw*.c
    ${imgui_SOURCE_DIR}/backends/*glfw*.cpp
    ${imgui_SOURCE_DIR}/backends/*opengl3*.c
    ${imgui_SOURCE_DIR}/backends/*opengl3*.cpp
)
add_library(imgui STATIC
    ${imgui_SOURCES}
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)
CPMAddPackage(
    NAME assimp
    GITHUB_REPOSITORY assimp/assimp
    GIT_TAG v6.0.2
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
    "ASSIMP_BUILD_TESTS OFF"
    "ASSIMP_NO_EXPORT OFF"
    "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
)

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    # Linux-specific
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
endif()

file(GLOB_RECURSE MY_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${MY_SOURCES} ${PLATFORM_SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17 c_std_99)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    glad
    glm
    assimp
    imgui
    ${OPENGL_LIBRARIES}
    $<$<PLATFORM_ID:Linux>:${GTK3_LIBRARIES}>
)
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    # These include directories are automatically added by the targets in the target_link_libraries
    # ${imgui_INCLUDE_DIRS}
    # ${glad_INCLUDE_DIRS}
    # ${OPENGL_INCLUDE_DIRS}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
    $<$<PLATFORM_ID:Linux>:${GTK3_INCLUDE_DIRS}>
)

if(UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_NAME} PUBLIC ${GTK3_CFLAGS_OTHER})
endif()


# Fonts Configuration
# Not used anywhere but decided to keep em
# set(FONT_DIR "${CMAKE_SOURCE_DIR}/Resources/assets/fonts/Source_Sans_3/static")
# set(REGULAR_FONT "${FONT_DIR}/SourceSans3-Regular.ttf")
# set(BOLD_FONT "${FONT_DIR}/SourceSans3-Bold.ttf")

# TODO: Proper function for file copy
# Copy assets to build directory
file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*")
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*")
file(GLOB_RECURSE MODEL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Models/*")
file(GLOB_RECURSE TEXTURE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Textures/*")

foreach(FILE ${SHADER_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Shaders" "${FILE}")
    configure_file(${FILE} "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${REL_PATH}" COPYONLY)
endforeach()

foreach(FILE ${RESOURCE_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources" "${FILE}")
    configure_file(${FILE} "${CMAKE_CURRENT_BINARY_DIR}/Resources/${REL_PATH}" COPYONLY)
endforeach()

foreach(FILE ${MODEL_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Models" "${FILE}")
    configure_file(${FILE} "${CMAKE_CURRENT_BINARY_DIR}/Models/${REL_PATH}" COPYONLY)
endforeach()

foreach(FILE ${TEXTURE_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Textures" "${FILE}")
    configure_file(${FILE} "${CMAKE_CURRENT_BINARY_DIR}/Textures/${REL_PATH}" COPYONLY)
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
# No tests so not needed
# include(CTest)
# enable_testing()
