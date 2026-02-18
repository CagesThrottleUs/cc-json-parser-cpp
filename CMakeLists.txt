cmake_minimum_required(VERSION 3.10.0)
project(cc-json-parser-cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bad to recurse because CMake does not know when to regenerate
# file(GLOB_RECURSE SRC_FILES "src/*.cpp")
set(SRC_FILES
  src/main.cpp
  src/parser/parser.cpp
  src/tokenizer/tokenizer.cpp
  src/file_handler/utf8_file.cpp
  src/exceptions/exceptions.cpp
)
add_executable(cc-json-parser-cpp ${SRC_FILES})
target_include_directories(cc-json-parser-cpp PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Boost REQUIRED COMPONENTS iostreams CONFIG)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::iostreams)
