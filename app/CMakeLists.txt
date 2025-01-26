cmake_minimum_required(VERSION 3.12)
project(app
        VERSION 0.0.0
        DESCRIPTION "Cache Test Application"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW) # avoid warning of relative paths translation


add_library(cache_cpp SHARED
    cache.cpp
    cache.hpp
)
target_include_directories(cache_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(cache_test
    cache_test.cpp
)

target_link_libraries(cache_test PRIVATE cache_cpp)


