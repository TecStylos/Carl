cmake_minimum_required(VERSION 3.8)

project("Carl")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(
	CARL_SOURCES
	"src/Carl.cpp"
)

add_library(
	Carl SHARED
	${CARL_SOURCES}
)

SET_TARGET_PROPERTIES(
	Carl PROPERTIES PREFIX ""
)

target_include_directories(
	Carl PUBLIC
	"include"
)