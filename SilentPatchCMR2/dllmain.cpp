#include <windows.h>
#include <cstdio>
#include <Shlwapi.h>

#include "MemoryMgr.h"

#define CMRCALL __stdcall

void LogToFile(const char* str, ...)
{
	if ( FILE* LogFile = fopen("VCSPC.log", "a") )
	{
		SYSTEMTIME	systemTime;
		va_list		arguments;
		char		TempString[MAX_PATH];

		va_start(arguments, str);
		vsnprintf(TempString, sizeof(TempString), str, arguments);
		GetLocalTime(&systemTime);
		fprintf(LogFile, "[%02d/%02d/%d %02d:%02d:%02d] ", systemTime.wDay, systemTime.wMonth, systemTime.wYear, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		fputs(TempString, LogFile);
		fputc('\n', LogFile);
		fclose(LogFile);
		va_end(arguments);
	}
}

void LogThis(float fX, float fY, void* ptr)
{
	LogToFile("Call: %u %g %g", ptr, fX, fY);
}

void __declspec(naked) ResHack()
{
	_asm
	{
		push	[esp]
		push	[esp+4+0Ch]
		push	[esp+8+10h]
		call	LogThis
		add		esp, 0Ch
		push	4C7D50h
		retn
	}
}

int GetResWidth()
{
	return 1280;
}

int GetResHeight()
{
	return 720;
}

struct CMR2Instance
{
  int m_dwWidth;
  int m_dwHeight;
  int m_dwDepth;
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  int m_bFullscreen;
  int field_20;
  char gap_24[912];
 // IDirectDraw7 *m_pDirectDrawCreator;
 // IDirectDraw7 *m_pDirectDrawDevice;
  char gap_3BC[24];
  int field_24;
};

CMR2Instance* const pTheGame = (CMR2Instance*)0x660830;

void WINAPI FullSizeWindow(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	X += 320;
	Y += 240;
	cx -= 640;
	cy -= 480;

	//X = GetSystemMetrics(SM_CXSCREEN);
	//Y = GetSystemMetrics(SM_CYSCREEN);

	//int		temp = pTheGame->m_dwHeight;

	SetWindowPos(hWnd, hWndInsertAfter, X - (pTheGame->m_dwWidth/2), Y - (pTheGame->m_dwHeight/2), pTheGame->m_dwWidth, pTheGame->m_dwHeight, uFlags);
}

WRAPPER void CMRCALL RenderText(int nIndex, const char* pText, int nX, int nY, unsigned char* pColour, int flags)
{ EAXJMP(0x40B880); }

void TestDraw()
{
	unsigned char	colour[] = { 255, 255, 255, 255 };

	RenderText(1, "Custom draw! This game looks fun to mess with!", pTheGame->m_dwWidth / 2, pTheGame->m_dwHeight / 2, colour, 4+2);

}

HANDLE& hLogFile = *(HANDLE*)0x66734C;
BOOL& bLogInitialized = *(BOOL*)0x667200;
void OpenLogFile(const wchar_t* pFileName)
{
	// TEMP: Opens a log file in the game directory
	wchar_t		wcLogPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, wcLogPath);
	PathAppend(wcLogPath, pFileName);

	hLogFile = CreateFile(wcLogPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if ( hLogFile != INVALID_HANDLE_VALUE )
		bLogInitialized = TRUE;
}

char* ReadRegistryString(const char* pKey)
{
	// TODO: We should remove all depends on registry keys whatsoever

	char*	pGameBuf = (char*)0x663EE4;
	HKEY	hKey;
	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Codemasters\\Colin McRae Rally 2", 0, KEY_READ, &hKey) == ERROR_SUCCESS )
	{
		DWORD	cbSize = MAX_PATH;
		RegQueryValueExA(hKey, pKey, nullptr, nullptr, reinterpret_cast<LPBYTE>(pGameBuf), &cbSize);
		RegCloseKey(hKey);
	}
	else
		pGameBuf[0] = '\0';

	// TODO: Make use of inbuilt logger to make it report errors here

	return pGameBuf;
}

void ApplyHooks()
{
	using namespace Memory;

	// Here all patching takes place
	// Simplified and fixed regkey reading
	// VirtualStore isn't pwning the game anymore - it apparently used to in some cases
	InjectHook(0x4AA720, ReadRegistryString, PATCH_JUMP);

	// TEMP: Logging to game dir
	Patch<const wchar_t*>(0x4A973B, L"output.log");
	InjectHook(0x4A973F, OpenLogFile);

	InjectHook(0x4D0B38, TestDraw, PATCH_CALL);
	Patch<WORD>(0x4D0B3D, 0x2CEB);

	// Windowed mode
	Patch<BYTE>(0x4A7A98, 0x79);
	//Patch<DWORD>(0x4A7A8D, 0);
	Patch<BYTE>(0x4A7A6F, 0xEB);

	Nop(0x4A7B7F, 1);
	InjectHook(0x4A7B80, FullSizeWindow, PATCH_CALL);

	//Patch<DWORD>(0x4A8200, 0);
	Patch<DWORD>(0x4A81B2, WS_POPUP);
	
	//MemoryVP::Patch<DWORD>(0x4A7BB7, 0x840);

	//MemoryVP::InjectHook(0x405C10, GetResWidth, PATCH_JUMP);
	//MemoryVP::InjectHook(0x405C30, GetResHeight, PATCH_JUMP);
	//MemoryVP::Patch<BYTE>(0x4C85E1, 0xC3);
	//MemoryVP::Patch<DWORD>(0x513200, 0);
	//MemoryVP::Patch(0x513200, &ResHack);
	//Patch<BYTE>(0x4A7AB9, 0x11);
	//Patch<BYTE>(0x4A7AAC, 0xE9);
	//Patch<DWORD>(0x4A7BB7, 0x00000001l + 0x00000010l);

	// 16:9 aspect ratio
	static const double		dAspectRatio = (65536.0 /** (4.0/3.0)*/ / (16.0/9.0)) /** (3.0/4.0)*/;
	Patch<const void*>(0x422D7B, &dAspectRatio);

	
	// Re-apply VirtualProtect on .text section
	DWORD		dwProtect;
	VirtualProtect((LPVOID)0x401000, 0x10F6F6, PAGE_EXECUTE_READ, &dwProtect);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	//UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpReserved);

	switch ( reason )
	{
	case DLL_PROCESS_ATTACH:
		Memory::InjectHook(0x405690, ApplyHooks);
		break;
	}

	return TRUE;
}