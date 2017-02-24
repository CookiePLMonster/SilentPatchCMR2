#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <Shlwapi.h>

#include "MemoryMgr.h"

enum
{
	Region_Europe = 0,
	Region_America,
	Region_Japan,
	Region_Poland
};

char* const HDPath = (char*)0x6640F0;
char* const CDPath = (char*)0x664400;

BOOL& bHDPathInit = *(BOOL*)0x664604;
BOOL& bCDPathInit = *(BOOL*)0x664608;

HWND* const gameWindows = (HWND*)0x663C84;
DWORD& dwCurrentWindow = *(DWORD*)0x663DAC;

MSG& Msg = *(MSG*)0x663C68;

void SetHDPath(const char* pPath)
{
	strcpy(HDPath, pPath);
	bHDPathInit = TRUE;
}

DWORD& Region = *(DWORD*)0x52EA54;

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

class MMIOFile
{
private:
	// Size: 0x34 bytes
	void*	m_pMem;
	HMMIO	m_hMmio;

public:
	BOOL	Close()
	{
		if ( m_hMmio != nullptr )
		{
			mmioClose( m_hMmio, 0 );
			m_hMmio = nullptr;
		}
		return FALSE;
	}
};

CMR2Instance* const pTheGame = (CMR2Instance*)0x660830;

void FullSizeWindow(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	X += 320;
	Y += 240;
	cx -= 640;
	cy -= 480;

	SetWindowPos(hWnd, hWndInsertAfter, X - (pTheGame->m_dwWidth/2), Y - (pTheGame->m_dwHeight/2), pTheGame->m_dwWidth, pTheGame->m_dwHeight, uFlags);
}

double CalcRatio(CMR2Instance* pGame)
{
	return (65536.0 * (16.0/9.0)) * ( static_cast<double>(pGame->m_dwHeight) / pGame->m_dwWidth );
}

WRAPPER void RenderText(int nIndex, const char* pText, int nX, int nY, unsigned char* pColour, int flags)
{ EAXJMP(0x40B880); }

WRAPPER void RunAtExitCallbacks(int nID) { EAXJMP(0x49C0F0); }

void DrawSPText()
{
	unsigned char	colour[] = { 255, 255, 255, 200 };

	RenderText(0, "SPCMR2 Build 2 ("__DATE__")", 10, pTheGame->m_dwHeight - 25, colour, 0);
}

/*HANDLE& hLogFile = *(HANDLE*)0x66734C;
BOOL& bLogInitialized = *(BOOL*)0x667200;*/
void OpenLogFile(const wchar_t* pFileName)
{
}

void LogToFile(const char* pLine)
{
}

void CloseLog()
{
}

void ShowNoCDNotification()
{
	MessageBox( gameWindows[dwCurrentWindow], L"Loading failed! Make sure you have specified a correct region in SPCMR2.ini file.\n\n"
											  L"Installed regions can be found in CountrySpecific directory.\n\n"
											  L"If the error persists despite selecting an installed region, your game installation might be corrupted or it's not Full.\n\n"
											  L"NOTE: Currently, SilentPatchCMR2 only supports Full game installations.",
											  L"CMR2 File Load Error",
											  MB_OK|MB_ICONWARNING );

	RunAtExitCallbacks(0);
	CloseLog();
	ExitProcess(Msg.wParam);
}

char* ReadRegistryString(const char* pKey)
{
	char*	pGameBuf = (char*)0x663EE4;
	HKEY	hKey;
	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Codemasters\\Colin McRae Rally 2", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS )
	{
		DWORD	cbSize = MAX_PATH;
		RegQueryValueExA(hKey, pKey, nullptr, nullptr, reinterpret_cast<LPBYTE>(pGameBuf), &cbSize);
		RegCloseKey(hKey);
	}
	else
		pGameBuf[0] = '\0';

	return pGameBuf;
}

const char* ReadRegistryString_Stub(const char* pKey)
{
	return pKey;
}

bool InitialisePaths()
{
	// TODO: Support installations other than Full
	SetHDPath(".");

	return true;
}

DWORD rawFOV; // default 161218
void ReadINI( bool& bDebugOverlay, bool& bWindow, bool& bBorderless )
{
	wchar_t buffer[32];

	static const wchar_t* const Regions[] = { L"EUROPE", L"AMERICA", L"JAPAN", L"POLAND" };
	GetPrivateProfileString( L"SilentPatch", L"Region", L"EUROPE", buffer, _countof(buffer), L".\\SPCMR2.ini" );

	Region = Region_Europe;
	for ( int i = 0; i < _countof(Regions); i++ )
	{
		if ( !_wcsicmp( buffer, Regions[i] ) )
		{
			Region = i;
			break;
		}
	}

	GetPrivateProfileString( L"SilentPatch", L"FOV", L"70.0", buffer, _countof(buffer), L".\\SPCMR2.ini" );
	float fFOV = static_cast<float>(_wtof(buffer));

	if ( fFOV < 30.0f )
		fFOV = 30.0f;
	else if ( fFOV > 150.0f )
		fFOV = 150.0f;

	rawFOV = (161218.0f*70.0f) / fFOV;

	bWindow = GetPrivateProfileInt( L"SilentPatch", L"Window", TRUE, L".\\SPCMR2.ini" ) != FALSE;
	bBorderless = GetPrivateProfileInt( L"SilentPatch", L"Borderless", TRUE, L".\\SPCMR2.ini" ) != FALSE;
}

void __declspec(naked) GetFOV()
{
	_asm
	{
		mov		edx, [rawFOV]
		retn
	}
}

void __declspec(naked) FrontendDrawHook()
{
	_asm
	{
		call	DrawSPText
		mov		eax, dword ptr ds:[817FC4h]
		
		mov		ecx, 4D0B70h
		jmp		ecx
	}
}

void ApplyHooks()
{
	using namespace Memory;

	bool	bDebugOverlay = false, bWindow = false, bBorderless = false;

	ReadINI( bDebugOverlay, bWindow, bBorderless );

	// Here all patching takes place

	// Simplified and fixed regkey reading
	// VirtualStore isn't pwning the game anymore - it apparently used to in some cases
	InjectHook(0x4AA720, ReadRegistryString, PATCH_JUMP);

	// Nulled logging (for now?)
	InjectHook(0x4A973F, OpenLogFile);
	InjectHook(0x4AB670, LogToFile, PATCH_JUMP);
	InjectHook(0x4AB6F0, CloseLog, PATCH_JUMP);

	InjectHook(0x4D0B6B, FrontendDrawHook, PATCH_JUMP);

	// Fix MMIO Close crash
	InjectHook(0x4BD960, &MMIOFile::Close, PATCH_JUMP);

	if ( bWindow )
	{
		// Windowed mode
		Patch<BYTE>(0x4A7A98, 0x79);
		Patch<BYTE>(0x4A7A6F, 0xEB);

		Nop(0x4A7B7F, 1);
		InjectHook(0x4A7B80, FullSizeWindow, PATCH_CALL);

		if ( bBorderless )
		{
			// Borderless
			Patch<DWORD>(0x4A81B2, WS_POPUP);
		}
	}

	// Changes to paths handling
	InjectHook(0x4D176C, InitialisePaths);
	Patch<const char*>(0x40EDA1, "CountrySpecific");
	Patch<const char*>(0x40ED61, "Fonts");
	Patch<const char*>(0x40EDD1, "CountrySpecific");
	Patch<const char*>(0x40EDB1, "SetupRep");
	Patch<const char*>(0x40ED31, "Sounds");
	Patch<const char*>(0x40ED81, "FrontEnd\\Videos");
	Patch<const char*>(0x40ED21, "Game\\Tracks");
	Patch<const char*>(0x40EDC1, "Game\\BigFiles");
	Patch<const char*>(0x40ED51, "Textures");
	Patch<const char*>(0x40ED41, "Game\\Cars");
	Patch<const char*>(0x40ED91, "FrontEnd");
	Patch<const char*>(0x40ED71, "Sounds\\Music");

	InjectHook(0x40E3FF, ReadRegistryString_Stub);
	InjectHook(0x40E4AD, ReadRegistryString_Stub);

	// New no CD notification
	InjectHook(0x4AA480, ShowNoCDNotification, PATCH_JUMP);

	// CDPath -> HDPath
	Patch<WORD>(0x4AA710, 0xEEEB);

	// Skip region reading (already read from INI)
	InjectHook(0x4D15F2, 0x4D1728, PATCH_JUMP);

	// Proper aspect ratio handling
	Patch<WORD>(0x422D74, 0x9050);
	Patch<DWORD>(0x422D76, 0x90909090);
	InjectHook(0x422D7A, CalcRatio, PATCH_CALL);

	// Adjustable FOV
	InjectHook(0x422D61, GetFOV, PATCH_CALL);
	InjectHook(0x422DAA, GetFOV, PATCH_CALL);

	
	// Re-apply VirtualProtect on .text section
	DWORD		dwProtect;
	VirtualProtect((LPVOID)0x401000, 0x110000, PAGE_EXECUTE_READ, &dwProtect);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpReserved);

	switch ( reason )
	{
	case DLL_PROCESS_ATTACH:
		Memory::InjectHook(0x405690, ApplyHooks);
		break;
	}

	return TRUE;
}