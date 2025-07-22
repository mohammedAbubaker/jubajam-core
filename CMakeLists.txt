cmake_minimum_required(VERSION 3.30)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
project(jubaengine LANGUAGES CXX C)
add_executable(main src/scratch.cpp)

find_package(Vulkan REQUIRED)
# ------------------------------------------------------------------ 
FetchContent_Declare        (h_glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable  (h_glfw)
add_library                 (l_glfw ALIAS glfw)
# ------------------------------------------------------------------ 
FetchContent_Declare        (l_stb GIT_REPOSITORY https://github.com/nothings/stb.git)
FetchContent_MakeAvailable  (l_stb)
add_library                 (l_stb INTERFACE)
target_include_directories  (l_stb INTERFACE ${l_stb_SOURCE_DIR})

target_link_libraries(main PRIVATE Vulkan::Vulkan l_glfw l_stb)
