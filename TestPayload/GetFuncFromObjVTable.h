#pragma once

#include <stdint.h>

template <typename FuncType>
FuncType GetFuncFromObjVTable(void* pObj, uint64_t vtableIndex)
{
	uintptr_t* vtable = *(uintptr_t**)pObj;
	void* funcAddr = (void*)vtable[vtableIndex];

	return (FuncType&)funcAddr;
}