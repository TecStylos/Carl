cmake_minimum_required(VERSION 3.8)

project("BUILD ALL")

string(
	TOLOWER
	${MSVC_C_ARCHITECTURE_ID}
	CARL_ARCHITECTURE
)

add_subdirectory("Carl")
add_subdirectory("AgentCarl")
add_subdirectory("TestPayload")
add_subdirectory("TestInjector")
add_subdirectory("vendor/EHSN")