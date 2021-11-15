cmake_minimum_required(VERSION 3.8)

project("Carl")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(
	CARL_SOURCES
	"src/Carl.cpp"
	"src/AgentRunner.cpp"
	"src/ProcessArchitecture.cpp"
	"src/PayloadConnector.cpp"
)

if (WIN32)
	set(
		CARL_SOURCES
		${CARL_SOURCES}
		"src/Platform/Win32AgentRunner.cpp"
		"src/Platform/Win32ProcessArchitecture.cpp"
	 "src/Platform/Win32ErrorMessage.cpp")
endif()

if (UNIX)
	set(
		CARL_SOURCES
		${CARL_SOURCES}
		"src/Platform/UnixAgentRunner.cpp"
		"src/Platform/UnixProcessArchitecture.cpp"
	)
endif()

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
	"../vendor/asio/asio/include"
)

string(
	TOUPPER
	${CMAKE_BUILD_TYPE}
	UPPER_BUILD_TYPE
)

set(
	CARL_COMPILE_DEFINITIONS
	"CARL_BUILD_TYPE_${UPPER_BUILD_TYPE}"
)

if (WIN32)
	set(
		CARL_COMPILE_DEFINITIONS
		${CARL_COMPILE_DEFINITIONS}
		"CARL_PLATFORM_WINDOWS"
	)
endif()

if (UNIX)
	set(
		CARL_COMPILE_DEFINITIONS
		${CARL_COMPILE_DEFINITIONS}
		"CARL_PLATFORM_UNIX"
	)
endif()

target_compile_definitions(
	Carl PUBLIC
	${CARL_COMPILE_DEFINITIONS}
	"CARL_BUILD_LIBRARY"
)
