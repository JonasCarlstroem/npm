cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(npm_registry INTERFACE)

set(LIB_TARGETS npm_registry PARENT_SCOPE)