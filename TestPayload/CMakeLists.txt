cmake_minimum_required(VERSION 3.8)

project("TestPlugin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(
	TestPayload SHARED 
	"TestPayload.cpp"
)