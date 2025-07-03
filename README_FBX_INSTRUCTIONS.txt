This file dictates how you should setup the relevant FBX SDK binaries needed to use the export functionality (and build the project :D)

1- Make a new directory and call it something like (FBX SDK 2020.3.7)

2- Download the two folders from the MEGA drive (release and debug):
Release: https://mega.nz/folder/8S1zRQSY#3dJUXJSsOBzxIAJQGqqS7A
Debug: https://mega.nz/folder/ZGcWiDbL#_d6Kb2ftTeWJ9ZmGBAuiQg

3- Change the snippet in the CMakeLists.txt

FROM:

# Path to FBX SDK (update this to your install path)
set(FBX_SDK_ROOT "D:/2020.3.7/temp/fbx202037_fbxsdk_vs2022_win")
# Choose lib path based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(FBX_SDK_LIB_DIR "${FBX_SDK_ROOT}/lib/x64/release")
else()
    set(FBX_SDK_LIB_DIR "${FBX_SDK_ROOT}/lib/x64/debug")
endif()

TO:

# Path to FBX SDK (update this to your install path)
set(FBX_SDK_ROOT "C:/FBX SDK 2020.3.7")
# Choose lib path based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(FBX_SDK_LIB_DIR "${FBX_SDK_ROOT}/release")
else()
    set(FBX_SDK_LIB_DIR "${FBX_SDK_ROOT}/debug")
endif()

4- NOTE: You'll notice a difference between the actual CMakeLists.txt in the export_branch and here, basically, I'm defaulting that the user will have a x64 machine only not a armx64 machine

5- The include directory will be uploaded directly to the GitHub repo in the export_branch, as its size is only ~5MBs
