cmake_minimum_required(VERSION 3.10)

project(home-monitor)

# prepare use of pkgconfig
find_package(PkgConfig REQUIRED)

# predefine compile flags
set(cxxflags -Werror -Wno-error=deprecated-declarations -Wall -Wextra -Wconversion -Wno-psabi)

# enable C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)