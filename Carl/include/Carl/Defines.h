#pragma once

#if defined CARL_PLATFORM_WINDOWS
#if defined CARL_BUILD_LIBRARY
#define CARL_API __declspec(dllexport)
#else
#define CARL_API __declspec(dllimport)
#endif
#elif defined CARL_PLATFORM_UNIX
#if defined CARL_BUILD_LIBRARY
#define CARL_API __attribute__((visibility("default")))
#else
#define CARL_API
#endif
#else
#error CARL_PLATFORM_<PLATFORM> NOT DEFINED!
#endif