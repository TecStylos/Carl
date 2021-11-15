#include <iostream>
#include <thread>
#include <filesystem>

#include "PayloadHandle.h"
#include "PayloadError.h"

int main(int argc, const char** argv, const char** env)
{
	std::cout << "WDIR -> " << std::filesystem::current_path().string() << std::endl;
	for (int i = 0; i < argc; ++i)
	{
		std::cout << "ARG -> " << argv[i] << std::endl;
	}
	if (argc != 5)
	{
		std::cout << "Usage: AgentCarl [targetPID] [dllPath] [initFuncName] [message]" << std::endl;
		return -1;
	}

	int targetPID = 0;
	try
	{
		targetPID = std::stoul(argv[1]);
	}
	catch (std::runtime_error& err)
	{
		std::cout << "Invalid PID! -> " << err.what() << std::endl;
		return -2;
	}

	std::string dllPath = argv[2];
	std::string initFuncName = argv[3];
	std::string message = argv[4];

	auto handle = AgentCarl::PayloadHandle::create(dllPath, targetPID);

	try
	{
		handle->inject();
	}
	catch (const AgentCarl::PayloadError& err)
	{
		std::cout << "Unable to inject payload!: " << err.what() << std::endl;
		return -3;
	}

	try
	{
		handle->call(initFuncName, message.c_str(), message.size() + 1);
	}
	catch (const AgentCarl::PayloadError& err)
	{
		std::cout << "Unable to call init function!" << std::endl;
		return -4;
	}

	return 0;
}