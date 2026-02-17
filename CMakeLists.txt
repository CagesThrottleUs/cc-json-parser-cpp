cmake_minimum_required(VERSION 3.10.0)
project(cc-json-parser-cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES "src/*.cpp")
add_executable(cc-json-parser-cpp ${SRC_FILES})
target_include_directories(cc-json-parser-cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
