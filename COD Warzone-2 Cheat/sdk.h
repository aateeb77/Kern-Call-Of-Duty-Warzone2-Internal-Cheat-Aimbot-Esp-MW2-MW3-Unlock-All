#pragma once
#include "stdafx.h"

namespace direcX 
{
	constexpr uint32_t command_queue = 0x2053D8D8; // 48 8B 0D ? ? ? ? 4C 8B C3 48 8B 15 ? ? ? ? 48 8B 01 FF 50 78 4C 8B 4F 50
}

namespace g_data
{
	extern HWND hWind;
	extern uintptr_t base;
	extern uintptr_t peb;
	extern uintptr_t visible_base;
	void init();
}

namespace sdk
{
	
}
