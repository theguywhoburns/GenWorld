# GenWorld

# Project Setup

## Dependencies
This project requires the following dependencies to be cloned inside the `vendor` folder:

- [GLM](https://github.com/g-truc/glm)
- [GLFW](https://github.com/glfw/glfw)
- [Assimp](https://github.com/assimp/assimp)

## Cloning Dependencies
Navigate to the project's root directory and run the following commands:

```sh
cd vendor
git clone https://github.com/g-truc/glm.git
git clone https://github.com/glfw/glfw.git
git clone https://github.com/assimp/assimp.git
```

## Compiling Assimp

Assimp must be compiled as a **static library**. Follow the appropriate instructions based on your platform.

### 🪟 For Windows (MinGW)

1. Navigate to the Assimp directory:

   ```sh
   cd vendor/assimp
   ```

2. Run CMake with the following configuration:

   ```sh
   cmake CMakeLists.txt -D BUILD_SHARED_LIBS=OFF \
                        -D CMAKE_C_COMPILER="C:/path/to/your/gcc/bin/gcc.exe" \
                        -D CMAKE_CXX_COMPILER="C:/path/to/your/gcc/bin/g++.exe" \
                        -G "MinGW Makefiles"
   ```

### 🐧 For Unix/Linux

1. Navigate to the Assimp directory:

   ```sh
   cd vendor/assimp
   ```

2. Run CMake using the following configuration:

   ```sh
   cmake CMakeLists.txt -D BUILD_SHARED_LIBS=OFF \
                        -D CMAKE_C_COMPILER=gcc \
                        -D CMAKE_CXX_COMPILER=g++ \
                        -G "Unix Makefiles"
   ```

---

This setup ensures Assimp is compiled as a static library with MinGW. For more details, refer to the related discussion: [Assimp Discussion #5633](https://github.com/assimp/assimp/discussions/5633).

