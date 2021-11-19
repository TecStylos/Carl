cmake_minimum_required(VERSION 3.8)

project("TestInjector")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	TestInjector
	"TestInjector.cpp"
)

target_include_directories(
	TestInjector PUBLIC
	"../Carl/include"
	"../vendor/Detours/src"
)

target_link_libraries(
	TestInjector PUBLIC
	Carl
	Detours
)

set_target_properties(
	TestInjector PROPERTIES OUTPUT_NAME "TestInjector_${CARL_ARCHITECTURE}"
)

if (WIN32)
	target_compile_definitions(
		TestInjector PUBLIC
		"CARL_PLATFORM_WINDOWS"
	)
	add_custom_command(
		TARGET TestInjector POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	"${PROJECT_SOURCE_DIR}/../bin/${CARL_ARCHITECTURE}-$<CONFIG>/Carl/Carl_${CARL_ARCHITECTURE}.dll"
    	$<TARGET_FILE_DIR:TestInjector>
	)
endif()