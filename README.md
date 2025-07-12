# GenWorld

<img width="800" height="600" alt="Screenshot 2025-07-06 170824" src="https://github.com/user-attachments/assets/e57a6809-8ad7-45a6-bb7b-eb4024fc250d" />
<img width="800" height="600" alt="Screenshot 2025-07-07 225446" src="https://github.com/user-attachments/assets/85efebba-27ad-475e-b896-56469db3571c" />
<img width="800" height="600" alt="Screenshot 2025-07-05 225308" src="https://github.com/user-attachments/assets/224db705-b016-4076-8148-eae57d2d4bcc" />

# Project Setup

## Dependencies
This project uses the following dependencies as Git submodules inside the `vendor` folder:

- [GLM](https://github.com/g-truc/glm)
- [GLFW](https://github.com/glfw/glfw)
- [Assimp](https://github.com/assimp/assimp)

## Cloning the Repository with Submodules
To clone this repository along with all dependencies, use:

```sh
git clone --recurse-submodules <repo-url>
```

If you have already cloned the repository without submodules, initialize and update them with:

```sh
git submodule update --init --recursive
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

