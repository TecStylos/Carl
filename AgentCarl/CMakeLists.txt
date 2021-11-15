cmake_minimum_required(VERSION 3.8)

project("AgentCarl")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(
	AGENT_CARL_SOURCES
	"src/PayloadHandle.cpp"
)

if (WIN32)
	set(
		AGENT_CARL_SOURCES
		${AGENT_CARL_SOURCES}
		"src/Platform/Win32ErrorMessage.cpp"
		"src/Platform/Win32PayloadHandle.cpp"
	)
elseif (UNIX)
	set(
		AGENT_CARL_SOURCES
		${AGENT_CARL_SOURCES}
	)
endif()

add_executable(
	AgentCarl
	${AGENT_CARL_SOURCES}
	"src/AgentCarl.cpp"
)

SET_TARGET_PROPERTIES(
	AgentCarl PROPERTIES PREFIX ""
)

string(
	TOLOWER
	${MSVC_C_ARCHITECTURE_ID}
	AGENT_CARL_ARCHITECTURE
)

set_target_properties(
	AgentCarl PROPERTIES OUTPUT_NAME "AgentCarl_${AGENT_CARL_ARCHITECTURE}"
)

target_include_directories(
	AgentCarl PUBLIC
	"include"
)

string(
	TOUPPER
	${CMAKE_BUILD_TYPE}
	UPPER_BUILD_TYPE
)

set(
	AGENT_CARL_COMPILE_DEFINITIONS
	"AGENT_CARL_BUILD_TYPE_${UPPER_BUILD_TYPE}"
)

if (WIN32)
	set(
		AGENT_CARL_COMPILE_DEFINITIONS
		${AGENT_CARL_COMPILE_DEFINITIONS}
		"AGENT_CARL_PLATFORM_WINDOWS"
	)
endif()

if (UNIX)
	set(
		AGENT_CARL_COMPILE_DEFINITIONS
		${AGENT_CARL_COMPILE_DEFINITIONS}
		"AGENT_CARL_PLATFORM_UNIX"
	)
	target_link_libraries(
		AgentCarl PUBLIC
		dl
	)
endif()

target_compile_definitions(
	AgentCarl PUBLIC
	${AGENT_CARL_COMPILE_DEFINITIONS}
)
