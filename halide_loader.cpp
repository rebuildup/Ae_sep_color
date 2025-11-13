#include "halide_loader.h"

#ifdef AE_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <delayimp.h>
#include <string>
#include <cstring>

namespace
{
HMODULE LoadHalideFromModuleDirectory()
{
	HMODULE module_handle = nullptr;
	if (!GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&LoadHalideFromModuleDirectory),
			&module_handle))
	{
		return nullptr;
	}

	wchar_t module_path[MAX_PATH] = {};
	DWORD length = GetModuleFileNameW(module_handle, module_path, static_cast<DWORD>(MAX_PATH));
	if (length == 0 || length == MAX_PATH)
	{
		return nullptr;
	}

	for (DWORD i = length; i > 0; --i)
	{
		if (module_path[i - 1] == L'\\' || module_path[i - 1] == L'/')
		{
			module_path[i - 1] = L'\0';
			break;
		}
	}

	std::wstring halide_path(module_path);
	halide_path.append(L"\\Halide.dll");

	return LoadLibraryExW(halide_path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

FARPROC WINAPI DelayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	if (dliNotify == dliNotePreLoadLibrary && pdli && pdli->szDll && _stricmp(pdli->szDll, "Halide.dll") == 0)
	{
		HMODULE loaded = LoadHalideFromModuleDirectory();
		if (loaded != nullptr)
		{
			return reinterpret_cast<FARPROC>(loaded);
		}
	}
	return nullptr;
}
} // namespace

extern "C"
{
PfnDliHook __pfnDliNotifyHook2 = DelayLoadHook;
}

bool EnsureHalideRuntimeLoaded()
{
	if (GetModuleHandleW(L"Halide.dll") != nullptr)
	{
		return true;
	}

	HMODULE loaded = LoadHalideFromModuleDirectory();
	return loaded != nullptr;
}

#else

bool EnsureHalideRuntimeLoaded()
{
	return true;
}

#endif


