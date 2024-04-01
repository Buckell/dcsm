cmake_minimum_required(VERSION 3.28)
project(dcsm_protocol)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)

# CLion analysis breaks if dcsm.hpp is not explicitly included. It is normally fine, but not with this project.
add_executable(dcsm_protocol main.cpp include/dcsm.hpp)


################ TESTING ########################

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TESTING_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/testing/*.cpp
)

enable_testing()

add_executable(dcsm_test ${TESTING_SOURCE_FILES})

target_link_libraries(
    dcsm_test
    GTest::gtest_main
)
