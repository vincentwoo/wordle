# CMakeList.txt : CMake project for wordle, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("wordle")

# Add source to this project's executable.
add_executable (wordle "wordle.cpp"  "util.h")

INCLUDE_DIRECTORIES("C:\\boost_1_78_0")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ranked_wordlist.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# TODO: Add tests and install targets if needed.
