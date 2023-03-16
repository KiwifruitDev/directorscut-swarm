//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Direct3D hook
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECT3D_HOOK_H_
#define _DIRECT3D_HOOK_H_

#include "d3d9.h"
#include <string>

typedef HRESULT(APIENTRY* endSceneFunc)(LPDIRECT3DDEVICE9 pDevice);

class CDirect3DHook
{
public:
	int GetD3D9Device();
	int Hook(HWND hwnd, char* endScenePointer);
	void Unhook();
protected:
	// TODO: Restore
	bool WINAPI Restore(char* fn, unsigned char* writeBytes, int writeSize, std::string descr);
	void* WINAPI HookFn(char* hookedFn, char* hookFn, int copyBytesSize, unsigned char* backupBytes, std::string descr);
	HWND hwnd = nullptr;
	char* ogEndSceneAddress;
	unsigned char endSceneBytes[7];
	char* d3d9DeviceTable[119];
	bool hooked = false;
	bool first = true;
	const char* REL_JMP = "\xE9";
	const char* NOP = "\x90";
	// 1 byte instruction +  4 bytes address
	const unsigned int SIZE_OF_REL_JMP = 5;
};

extern endSceneFunc trampEndScene;

// singleton
CDirect3DHook& GetDirect3DHook();

#endif
