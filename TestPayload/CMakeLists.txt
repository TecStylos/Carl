cmake_minimum_required(VERSION 3.8)

project("TestPayload")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	TestPayload SHARED 
	"TestPayload.cpp"
)

target_link_libraries(
	TestPayload PUBLIC 
	EHSN
)

target_include_directories(
	TestPayload PUBLIC 
	"../vendor/asio/asio/include"
	"../Carl/include"
)

SET_TARGET_PROPERTIES(
	TestPayload PROPERTIES PREFIX ""
)

set_target_properties(
	TestPayload PROPERTIES OUTPUT_NAME "TestPayload_${CARL_ARCHITECTURE}"
)

if (WIN32)
	add_custom_command(
		TARGET TestPayload POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	"$<TARGET_FILE_DIR:TestPayload>/TestPayload_${CARL_ARCHITECTURE}.dll"
    	"${PROJECT_SOURCE_DIR}/../bintools/$<CONFIG>/"
	)
endif()