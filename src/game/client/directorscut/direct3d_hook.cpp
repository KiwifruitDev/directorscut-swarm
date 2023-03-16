//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Direct3D hook
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#undef RegSetValueEx
#undef RegSetValue
#undef RegQueryValueEx
#undef RegQueryValue
#undef RegOpenKeyEx
#undef RegOpenKey
#undef RegCreateKeyEx
#undef RegCreateKey
#undef ReadConsoleInput
#undef INVALID_HANDLE_VALUE
#undef GetCommandLine

#include "direct3d_hook.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

endSceneFunc trampEndScene = nullptr;

CDirect3DHook g_Direct3DHook;

CDirect3DHook& GetDirect3DHook()
{
	return g_Direct3DHook;
}

// adapted from https://guidedhacking.com/threads/simple-x86-c-trampoline-hook.14188/
// hookedFn: The function that's about to the hooked
// hookFn: The function that will be executed before `hookedFn` by causing `hookFn` to take a detour
void* WINAPI CDirect3DHook::HookFn(char* hookedFn, char* hookFn, int copyBytesSize, unsigned char* backupBytes, std::string descr)
{

	if (copyBytesSize < 5)
	{
		// the function prologue of the hooked function
		// should be of size 5 (or larger)
		return nullptr;
	}

	//
	// 1. Backup the original function prologue
	//
	if (!ReadProcessMemory(GetCurrentProcess(), hookedFn, backupBytes, copyBytesSize, 0))
	{
		return nullptr;
	}

	//
	// 2. Setup the trampoline
	// --> Cause `hookedFn` to return to `hookFn` without causing an infinite loop
	// Otherwise calling `hookedFn` directly again would then call `hookFn` again, and so on :)
	//
	// allocate executable memory for the trampoline
	// the size is (amount of bytes copied from the original function) + (size of a relative jump + address)

	char* trampoline = (char*)VirtualAlloc(0, copyBytesSize + SIZE_OF_REL_JMP, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// steal the first `copyBytesSize` bytes from the original function
	// these will be used to make the trampoline work
	// --> jump back to `hookedFn` without executing `hookFn` again
	memcpy(trampoline, hookedFn, copyBytesSize);
	// append the relative JMP instruction after the stolen instructions
	memcpy(trampoline + copyBytesSize, REL_JMP, sizeof(REL_JMP));

	// calculate the offset between the hooked function and the trampoline
	// --> distance between the trampoline and the original function `hookedFn`
	// this will land directly *after* the inserted JMP instruction, hence subtracting 5
	int hookedFnTrampolineOffset = hookedFn - trampoline - SIZE_OF_REL_JMP;
	memcpy(trampoline + copyBytesSize + 1, &hookedFnTrampolineOffset, sizeof(hookedFnTrampolineOffset));

	//
	// 3. Detour the original function `hookedFn`
	// --> cause `hookedFn` to execute `hookFn` first
	// remap the first few bytes of the original function as RXW
	DWORD oldProtect;
	if (!VirtualProtect(hookedFn, copyBytesSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		return nullptr;
	}

	// best variable name ever
	// calculate the size of the relative jump between the start of `hookedFn` and the start of `hookFn`.
	int hookedFnHookFnOffset = hookFn - hookedFn - SIZE_OF_REL_JMP;

	// Take a relative jump to `hookFn` at the beginning
	// of course, `hookFn` has to expect the same parameter types and values
	memcpy(hookedFn, REL_JMP, sizeof(REL_JMP));
	memcpy(hookedFn + 1, &hookedFnHookFnOffset, sizeof(hookedFnHookFnOffset));

	// restore the previous protection values
	VirtualProtect(hookedFn, copyBytesSize, oldProtect, &oldProtect);

	return trampoline;
}

int CDirect3DHook::GetD3D9Device()
{
	if (!hwnd) {
		return 1;
	}

	if (!d3d9DeviceTable) {
		return 2;
	}

	IDirect3D9* d3dSys = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3dSys) {
		return 3;
	}

	IDirect3DDevice9* dummyDev = NULL;

	// options to create dummy device
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = false;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;

	HRESULT dummyDeviceCreated = d3dSys->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &dummyDev);

	if (dummyDeviceCreated != S_OK)
	{
		// may fail in windowed fullscreen mode, trying again with windowed mode
		d3dpp.Windowed = ~d3dpp.Windowed;

		dummyDeviceCreated = d3dSys->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &dummyDev);

		if (dummyDeviceCreated != S_OK)
		{
			d3dSys->Release();
			return 4;
		}
	}

	memcpy(d3d9DeviceTable, *reinterpret_cast<void***>(dummyDev), sizeof(d3d9DeviceTable));

	dummyDev->Release();
	d3dSys->Release();
	return 0;
}

int CDirect3DHook::Hook(HWND hwnd, char* endScenePointer)
{
	if (hooked)
		return 1;
	this->hwnd = hwnd;
	int deviceGot = GetD3D9Device();
	if (deviceGot == 0)
	{
		ogEndSceneAddress = d3d9DeviceTable[42];
		trampEndScene = (endSceneFunc)HookFn(ogEndSceneAddress, endScenePointer, 7, endSceneBytes, "EndScene");
		if (!trampEndScene)
		{
			return 2;
		}
		hooked = true;
		return 0;
	}
	else
	{
		return deviceGot+3;
	}
}

void CDirect3DHook::Unhook()
{
	Restore(ogEndSceneAddress, endSceneBytes, 7, "EndScene");
	hooked = false;
}

bool WINAPI CDirect3DHook::Restore(char* fn, unsigned char* writeBytes, int writeSize, std::string descr)
{
	DWORD oldProtect;
	if (!VirtualProtect(fn, writeSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		return false;
	}

	if (!WriteProcessMemory(GetCurrentProcess(), fn, writeBytes, writeSize, 0))
	{
		return false;
	}

	if (!VirtualProtect(fn, writeSize, oldProtect, &oldProtect))
	{
		return false;
	}

	return false;
}
