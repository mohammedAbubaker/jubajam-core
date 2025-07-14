cmake_minimum_required(VERSION 3.30)
include(FetchContent)
project(jubajam-core LANGUAGES CXX C)
add_executable(main src/main.cpp)

# ------------------------------------------------------------------ 
add_library                 (l_glad STATIC vendor/glad/src/glad.c)
set_target_properties       (l_glad PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories  (l_glad PUBLIC vendor/glad/include)
# ------------------------------------------------------------------ 
FetchContent_Declare        (h_assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v6.0.2)
FetchContent_MakeAvailable  (h_assimp)
add_library                 (l_assimp ALIAS assimp)
# ------------------------------------------------------------------ 
FetchContent_Declare        (h_glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable  (h_glfw)
add_library                 (l_glfw ALIAS glfw)
# ------------------------------------------------------------------ 
FetchContent_Declare        (l_stb GIT_REPOSITORY https://github.com/nothings/stb.git)
FetchContent_MakeAvailable  (l_stb)
add_library                 (l_stb INTERFACE)
target_include_directories  (l_stb INTERFACE ${l_stb_SOURCE_DIR})
# ------------------------------------------------------------------ 

target_link_libraries(main PRIVATE l_glad l_assimp l_glfw l_stb)
