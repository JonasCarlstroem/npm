cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(npm_registry INTERFACE)

install(TARGETS npm_registry
		EXPORT libTargets
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		RUNTIME DESTINATION bin
		INCLUDES DESTINATION include)

install(DIRECTORY include/ DESTINATION include)

set(LIB_TARGETS npm_registry PARENT_SCOPE)