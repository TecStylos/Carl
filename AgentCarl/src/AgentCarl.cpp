#include <iostream>
#include <thread>

#include "PayloadHandle.h"

int main(int argc, const char** argv, const char** env)
{
	if (argc != 5)
	{
		while (true)
			std::this_thread::sleep_for(std::chrono::seconds(10));
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
		return -1;
	}

	std::string dllPath = argv[2];
	std::string initFuncName = argv[3];
	std::string message = argv[4];

	auto handle = AgentCarl::PayloadHandle::create(dllPath, targetPID, 0);

	if (!handle)
	{
		std::cout << "Unable to create payload handle!" << std::endl;
		return -1;
	}

	if (!handle->inject())
	{
		std::cout << "Unable to inject payload!" << std::endl;
		return -1;
	}

	if (!handle->call(initFuncName, message.c_str(), message.size() + 1))
	{
		std::cout << "Unable to call init function!" << std::endl;
		return -1;
	}

	if (!handle->detach())
	{
		std::cout << "Unable to detach payload!" << std::endl;
		return -1;
	}
	return 0;
}