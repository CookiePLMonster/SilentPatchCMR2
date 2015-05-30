#ifndef __MEMORYMGR
#define __MEMORYMGR

#define WRAPPER __declspec(naked)
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }
#define VARJMP(a) { _asm jmp a }
#define WRAPARG(a) UNREFERENCED_PARAMETER(a)

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((DWORD_PTR*)this) = (DWORD_PTR)a

enum
{
	PATCH_CALL,
	PATCH_JUMP,
	PATCH_NOTHING,
};

inline signed char* GetVer()
{
	static signed char	bVer = -1;
	return &bVer;
}

inline bool* GetEuropean()
{
	static bool			bEuropean;
	return &bEuropean;
}

inline void* GetDummy()
{
	static DWORD		dwDummy;
	return &dwDummy;
}

inline ptrdiff_t GetModule()
{
	static HMODULE		hModule = GetModuleHandle(nullptr);
	return (ptrdiff_t)hModule;
}

template<typename AT>
inline AT DynBaseAddress(AT address)
{
	return GetModule() - 0x400000 + address;
}

#if defined GINPUT_COMPILE_III_VERSION

// This function initially detects III version then chooses the address basing on game version
template<typename T>
inline T AddressByVersion(DWORD address10, DWORD address11, DWORD addressSteam)
{
	signed char*	bVer = GetVer();

	if ( *bVer == -1 )
	{
		if (*(DWORD*)0x5C1E70 == 0x53E58955) *bVer = 0;
		else if (*(DWORD*)0x5C2130 == 0x53E58955) *bVer = 1;
		else if (*(DWORD*)0x5C6FD0 == 0x53E58955) *bVer = 2;
	}

	switch ( *bVer )
	{
	case 1:
		assert(address11);
		return (T)address11;
	case 2:
		assert(addressSteam);
		return (T)addressSteam;
	default:
		assert(address10);
		return (T)address10;
	}
}

#elif defined GINPUT_COMPILE_VC_VERSION

// This function initially detects VC version then chooses the address basing on game version
template<typename T>
inline T AddressByVersion(DWORD address10, DWORD address11, DWORD addressSteam)
{
	signed char*	bVer = GetVer();

	if ( *bVer == -1 )
	{
		if (*(DWORD*)0x667BF0 == 0x53E58955) *bVer = 0;
		else if (*(DWORD*)0x667C40 == 0x53E58955) *bVer = 1;
		else if (*(DWORD*)0x666BA0 == 0x53E58955) *bVer = 2;
	}

	switch ( *bVer )
	{
	case 1:
		assert(address11);
		return (T)address11;
	case 2:
		assert(addressSteam);
		return (T)addressSteam;
	default:
		assert(address10);
		return (T)address10;
	}
}

#elif defined GINPUT_COMPILE_SA_VERSION

// This function initially detects SA version then chooses the address basing on game version
template<typename T>
inline T AddressByVersion(DWORD address10, DWORD address11, DWORD addressSteam)
{
	signed char*	bVer = GetVer();
	bool*			bEuropean = GetEuropean();

	if ( *bVer == -1 )
	{
		if ( *(DWORD*)DynBaseAddress(0x858D21) == 0x3539F633 )
		{
			*bVer = 3;
			*bEuropean = false;
		}

		else if ( *(DWORD*)DynBaseAddress(0x82457C) == 0x94BF )
		{
			*bVer = 0;
			*bEuropean = false;
		}
		else if ( *(DWORD*)DynBaseAddress(0x8245BC) == 0x94BF )
		{
			*bVer = 0;
			*bEuropean = true;
		}
		else if ( *(DWORD*)DynBaseAddress(0x8252FC) == 0x94BF )
		{
			*bVer = 1;
			*bEuropean = false;
		}
		else if ( *(DWORD*)DynBaseAddress(0x82533C) == 0x94BF )
		{
			*bVer = 1;
			*bEuropean = true;
		}
		else if (*(DWORD*)DynBaseAddress(0x85EC4A) == 0x94BF )
		{
			*bVer = 2;
			*bEuropean = false;
		}
	}

	switch ( *bVer )
	{
	case 1:
		assert(address11);

		// Safety measures - if null, return dummy var pointer to prevent a crash
		if ( !address11 )
			return (T)GetDummy();

		// Adjust to US if needed
		if ( !(*bEuropean) && address11 > 0x746FA0 )
		{
			if ( address11 < 0x7BB240 )
				address11 -= 0x50;
			else
				address11 -= 0x40;
		}
		return (T)address11;
	case 2:
		assert(addressSteam);
		// Safety measures - if null, return dummy var pointer to prevent a crash
		if ( !addressSteam )
			return (T)GetDummy();

		return (T)addressSteam;
	case 3:
		// TODO: DO
		return (T)GetDummy();
	default:
		assert(address10);

		// Safety measures - if null, return dummy var pointer to prevent a crash
		if ( !address10 )
			return (T)GetDummy();

		// Adjust to EU if needed
		if ( *bEuropean && address10 > 0x7466D0 )
		{
			if ( address10 < 0x7BA940 )
				address10 += 0x50;
			else
				address10 += 0x40;
		}
		return (T)address10;
	}
}

template<typename T>
inline T AddressByRegion_10(DWORD address10)
{
	bool*			bEuropean = GetEuropean();
	signed char*	bVer = GetVer();

	if ( *bVer == -1 )
	{
		if ( *(DWORD*)0x82457C == 0x94BF )
		{
			*bVer = 0;
			*bEuropean = false;
		}
		else if ( *(DWORD*)0x8245BC == 0x94BF )
		{
			*bVer = 0;
			*bEuropean = true;
		}
		else
		{
			assert(!"AddressByRegion_10 on non-1.0 EXE!");
		}
	}

	// Adjust to EU if needed
	if ( *bEuropean && address10 > 0x7466D0 )
	{
		if ( address10 < 0x7BA940 )
			address10 += 0x50;
		else
			address10 += 0x40;
	}
	return (T)address10;
}

template<typename T>
inline T AddressByRegion_11(DWORD address11)
{
	bool*			bEuropean = GetEuropean();
	signed char*	bVer = GetVer();

	if ( *bVer == -1 )
	{
		if ( *(DWORD*)0x8252FC == 0x94BF )
		{
			*bVer = 1;
			*bEuropean = false;
		}
		else if ( *(DWORD*)0x82533C == 0x94BF )
		{
			*bVer = 1;
			*bEuropean = true;
		}
		else
		{
			assert(!"AddressByRegion_11 on non-1.01 EXE!");
		}
	}

	// Adjust to US if needed
	if ( !(*bEuropean) && address11 > 0x746FA0 )
	{
		if ( address11 < 0x7BB240 )
			address11 -= 0x50;
		else
			address11 -= 0x40;
	}
	return (T)address11;
}

#endif

namespace Memory
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{*(T*)address = value; }

	template<typename AT>
	inline void		Nop(AT address, unsigned int nCount)
	// TODO: Finish multibyte nops
	{ memset((void*)address, 0x90, nCount); }

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
	{
		DWORD		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		switch ( nType )
		{
		case PATCH_JUMP:
			*(BYTE*)address = 0xE9;
			break;
		case PATCH_CALL:
			*(BYTE*)address = 0xE8;
			break;
		}

		*(ptrdiff_t*)((DWORD)address + 1) = dwHook - (DWORD)address - 5;
	}
};

namespace MemoryVP
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{
		DWORD		dwProtect[2];
		VirtualProtect((void*)address, sizeof(T), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
		*(T*)address = value;
		VirtualProtect((void*)address, sizeof(T), dwProtect[0], &dwProtect[1]);
	}

	template<typename AT>
	inline void		Nop(AT address, unsigned int nCount)
	{
		DWORD		dwProtect[2];
		VirtualProtect((void*)address, nCount, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
		memset((void*)address, 0x90, nCount);
		VirtualProtect((void*)address, nCount, dwProtect[0], &dwProtect[1]);
	}

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
	{
		DWORD		dwProtect[2];
		switch ( nType )
		{
		case PATCH_JUMP:
			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			*(BYTE*)address = 0xE9;
			break;
		case PATCH_CALL:
			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			*(BYTE*)address = 0xE8;
			break;
		default:
			VirtualProtect((void*)((DWORD)address + 1), 4, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			break;
		}
		DWORD		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		*(ptrdiff_t*)((DWORD)address + 1) = (DWORD)dwHook - (DWORD)address - 5;
		if ( nType == PATCH_NOTHING )
			VirtualProtect((void*)((DWORD)address + 1), 4, dwProtect[0], &dwProtect[1]);
		else
			VirtualProtect((void*)address, 5, dwProtect[0], &dwProtect[1]);
	}

	namespace DynBase
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			MemoryVP::Patch(DynBaseAddress(address), value);
		}

		template<typename AT>
		inline void		Nop(AT address, unsigned int nCount)
		{
			MemoryVP::Nop(DynBaseAddress(address), nCount);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType=PATCH_NOTHING)
		{
			MemoryVP::InjectHook(DynBaseAddress(address), hook, nType);
		}
	};
};

#endif