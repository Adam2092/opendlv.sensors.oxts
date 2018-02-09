# Copyright (C) 2018  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.2)

project(oxts)

# This project requires C++14 or newer.
set(CMAKE_CXX_STANDARD 14)
# Strip unneeded symbols from binaries.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
# Build a static binary.
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
# Add further warning levels.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Threads are necessary for linking.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Generate cluon-msc from cluon-complete.hpp.
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/cluon-msc
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/cluon-complete.hpp ${CMAKE_BINARY_DIR}/cluon-complete.cpp
                   COMMAND ${CMAKE_CXX_COMPILER} -o ${CMAKE_BINARY_DIR}/cluon-msc ${CMAKE_BINARY_DIR}/cluon-complete.cpp -std=c++14 -pthread -D HAVE_CLUON_MSC
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cluon-complete.hpp)


# Generate messages from messages.odvd file.
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/messages.cpp
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMAND ${CMAKE_BINARY_DIR}/cluon-msc --cpp-sources --cpp-add-include-file=messages.hpp --out=${CMAKE_BINARY_DIR}/messages.cpp ${CMAKE_CURRENT_SOURCE_DIR}/messages.odvd
                   COMMAND ${CMAKE_BINARY_DIR}/cluon-msc --cpp-headers --out=${CMAKE_BINARY_DIR}/messages.hpp ${CMAKE_CURRENT_SOURCE_DIR}/messages.odvd
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/messages.odvd ${CMAKE_BINARY_DIR}/cluon-msc)
# Add current build directory as include directory as it contains generated files.
include_directories(SYSTEM ${CMAKE_BINARY_DIR})


# Create executable.
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/oxts.cpp ${CMAKE_CURRENT_SOURCE_DIR}/oxts_decoder.cpp messages.cpp)
target_link_libraries(${PROJECT_NAME} Threads::Threads)


# Add unit testing.
enable_testing()
add_executable(${PROJECT_NAME}-Runner Test_oxts.cpp ${CMAKE_CURRENT_SOURCE_DIR}/oxts_decoder.cpp messages.cpp)
target_link_libraries(${PROJECT_NAME}-Runner Threads::Threads)
add_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)

