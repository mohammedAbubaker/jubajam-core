
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(jubajam-core)
add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glad)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw-3.4)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glm)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/json)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json)
