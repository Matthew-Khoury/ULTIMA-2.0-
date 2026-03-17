cmake_minimum_required(VERSION 4.1)
project(ULTIMA_2_0_)

set(CMAKE_CXX_STANDARD 14)

# 1. Find the ncurses package
find_package(Curses REQUIRED)

# 2. Include the directories found
include_directories(${CURSES_INCLUDE_DIR})

# 3. Add your executables
add_executable(ULTIMA_2_0_
        ULTIMA.cpp
        ULTIMA.h
        Semaphore.h
        Semaphore.cpp
        Scheduler.cpp
        Scheduler.h
        Queue.h)

# 4. Link the ncurses library
target_link_libraries(ULTIMA_2_0_ ${CURSES_LIBRARIES})
