cmake_minimum_required(VERSION 3.21)
project(SearchSocial)

set(CMAKE_CXX_STANDARD 17)

add_executable(SearchSocial main.cpp iterator_range.h parse.cpp parse.h search_server.cpp search_server.h test_runner.h duration.h profile.h Synchronized.h)