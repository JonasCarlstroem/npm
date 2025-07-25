cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_library(npm_registry INTERFACE)
CREATE_LIB(npm_registry INTERFACE)

target_include_directories(npm_registry INTERFACE 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/npm>
)

INSTALL_LIB(npm_registry True npm)

#set(LIB_TARGETS npm_registry PARENT_SCOPE)