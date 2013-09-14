// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "mhook/mhook-lib/mhook.h"
#include <winscard.h>
#include <stdio.h>
#include <malloc.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef LONG (WINAPI *PSCARDTRANSMIT)(
  _In_         SCARDHANDLE hCard,
  _In_         LPCSCARD_IO_REQUEST pioSendPci,
  _In_         LPCBYTE pbSendBuffer,
  _In_         DWORD cbSendLength,
  _Inout_opt_  LPSCARD_IO_REQUEST pioRecvPci,
  _Out_        LPBYTE pbRecvBuffer,
  _Inout_      LPDWORD pcbRecvLength	
	);

PSCARDTRANSMIT WINAPI OrigSCardTransmit = (PSCARDTRANSMIT)::GetProcAddress(::GetModuleHandle(L"winscard"), "SCardTransmit");

typedef HMODULE (WINAPI *PLOADLIBRARY_A)(
	_In_	LPCSTR lpFileName
	);
// we need to hook all LoadLibrary* functions to catch when winscard.dll is loaded
PLOADLIBRARY_A WINAPI OrigLoadLibraryA = (PLOADLIBRARY_A)::GetProcAddress(::GetModuleHandle(L"kernel32"), "LoadLibraryA");

typedef HMODULE (WINAPI *PLOADLIBRARY_W)(
	_In_	LPCWSTR lpFileName
	);

PLOADLIBRARY_W WINAPI OrigLoadLibraryW = (PLOADLIBRARY_W)::GetProcAddress(::GetModuleHandle(L"kernel32"), "LoadLibraryW");

typedef HMODULE (WINAPI *PLOADLIBRARYEX_W)(
	_In_	LPCWSTR lpFileName,
    _In_	HANDLE hFile,
	_In_    DWORD dwFlags
	);

PLOADLIBRARYEX_W WINAPI OrigLoadLibraryExW = (PLOADLIBRARYEX_W)::GetProcAddress(::GetModuleHandle(L"kernel32"), "LoadLibraryExW");

typedef HMODULE (WINAPI *PLOADLIBRARYEX_A)(
	_In_	LPCSTR lpFileName,
    _In_	HANDLE hFile,
	_In_    DWORD dwFlags
	);

PLOADLIBRARYEX_A WINAPI OrigLoadLibraryExA = (PLOADLIBRARYEX_A)::GetProcAddress(::GetModuleHandle(L"kernel32"), "LoadLibraryExA");

bool ALREADY_HOOKED = false;
wchar_t pathToLog[MAX_PATH+10]; 

void byte2hex(LPCBYTE byteArray,DWORD cbLength, char *str){
    LPCBYTE pin = byteArray;
    const char * hex = "0123456789ABCDEF";
    char * pout = str;
    unsigned int i = 0;
    for(; i < cbLength-1; ++i){
        *pout++ = hex[(*pin>>4)&0xF];
        *pout++ = hex[(*pin++)&0xF];
        *pout++ = ':';
    }
    *pout++ = hex[(*pin>>4)&0xF];
    *pout++ = hex[(*pin)&0xF];
    *pout = 0;
}

// Wraper around original SCardTransmit that does the actuall logging
LONG WINAPI HookedSCardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer,LPDWORD pcbRecvLength){
	LONG result = NULL;
	char *pcSendBufferHex = (char *)malloc(cbSendLength*3+1);
	char *pcRecvBufferHex;
	DWORD dwCount;
	HANDLE hLogFile;
	//log send buffer
	byte2hex(pbSendBuffer,cbSendLength,pcSendBufferHex);
	hLogFile = CreateFile(pathToLog, GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hLogFile != INVALID_HANDLE_VALUE){
		SetFilePointer(hLogFile, 0, 0, FILE_END);
		WriteFile(hLogFile, "Winscard!SCardTransmit:\n", 24, &dwCount,NULL);
		WriteFile(hLogFile, ">>> ", 4, &dwCount,NULL);
		WriteFile(hLogFile, pcSendBufferHex,strlen(pcSendBufferHex) , &dwCount,NULL);
		WriteFile(hLogFile, "\r\n",2 , &dwCount,NULL);
	}
	free(pcSendBufferHex);
	//invoke original
	if(OrigSCardTransmit != NULL){
		result = (*OrigSCardTransmit)(hCard,pioSendPci,pbSendBuffer,cbSendLength,pioRecvPci,pbRecvBuffer,pcbRecvLength);
	}
	//log receive buffer
	pcRecvBufferHex = (char *)malloc(*pcbRecvLength*3+1);
	byte2hex(pbRecvBuffer,*pcbRecvLength,pcRecvBufferHex);
	if(hLogFile != INVALID_HANDLE_VALUE){
		WriteFile(hLogFile, "<<< ", 4, &dwCount,NULL);
		WriteFile(hLogFile, pcRecvBufferHex,strlen(pcRecvBufferHex) , &dwCount,NULL);
		WriteFile(hLogFile, "\r\n",2 , &dwCount,NULL);
		CloseHandle(hLogFile);
	}
	free(pcRecvBufferHex);
	return result;
}

// called in every hooked LoadLibrary* function, if winscard.dll is loaded, hook SCardTransmit
void checkAndHook(){
	if((GetModuleHandle(L"winscard.dll") != NULL)  && !ALREADY_HOOKED){
		// hook it 
		OrigSCardTransmit = (PSCARDTRANSMIT)::GetProcAddress(::GetModuleHandle(L"winscard"), "SCardTransmit");
		Mhook_SetHook((PVOID*)&OrigSCardTransmit, HookedSCardTransmit);
		ALREADY_HOOKED = true;
	}
}

// Simple hooks for LoadLibrary* functions
HMODULE WINAPI HookedLoadLibraryA(LPCSTR lpFileName){
	HMODULE result = NULL;

	if(OrigLoadLibraryA != NULL){
		result = (*OrigLoadLibraryA)(lpFileName);
	}
	// do check and hook if winscard.dll
	checkAndHook();
	return result;
}

HMODULE WINAPI HookedLoadLibraryW(LPCWSTR lpFileName){
	HMODULE result = NULL;

	if(OrigLoadLibraryW != NULL){
		result = (*OrigLoadLibraryW)(lpFileName);
	}
	checkAndHook();
	return result;
}

HMODULE WINAPI HookedLoadLibraryExA(LPCSTR lpFileName,HANDLE hFile, DWORD dwFlags){
	HMODULE result = NULL;

	if(OrigLoadLibraryA != NULL){
		result = (*OrigLoadLibraryExA)(lpFileName,hFile,dwFlags);
	}
	checkAndHook();
	return result;
}

HMODULE WINAPI HookedLoadLibraryExW(LPCWSTR lpFileName,HANDLE hFile, DWORD dwFlags){
	HMODULE result = NULL;

	if(OrigLoadLibraryW != NULL){
		result = (*OrigLoadLibraryExW)(lpFileName,hFile,dwFlags);
	}
	checkAndHook();
	return result;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
	wchar_t *pos = NULL;
	wchar_t pathToExe[MAX_PATH];
	switch (ul_reason_for_call){
	case DLL_PROCESS_ATTACH:
		// get the path to this dll, log file will be saved as <PATH>\Application.exe.txt
		// so each application will have it's own log file
		GetModuleFileNameW((HINSTANCE)&__ImageBase, pathToLog, MAX_PATH); 
		// currently i don't really care about actuall path being longer than MAX_PATH
		pos = wcsrchr(pathToLog,'\\');
		*++pos = '\0';
		pos = NULL;
		if(GetModuleFileName(NULL,pathToExe,MAX_PATH)){
			pos = wcsrchr(pathToExe,'\\');
			*++pos;
		}
		if(pos != NULL){
			wcscat_s(pathToLog,MAX_PATH,pos);
			wcscat_s(pathToLog,MAX_PATH,L".txt");
		}else{ // clumsy , i know 
			wcscat_s(pathToLog,MAX_PATH,L"winscard.txt");
		}
		// place all needed hooks
		Mhook_SetHook((PVOID*)&OrigLoadLibraryA, HookedLoadLibraryA);
		Mhook_SetHook((PVOID*)&OrigLoadLibraryW, HookedLoadLibraryW);
		Mhook_SetHook((PVOID*)&OrigLoadLibraryExW, HookedLoadLibraryExW);
		Mhook_SetHook((PVOID*)&OrigLoadLibraryExA, HookedLoadLibraryExA);
		break;
	case DLL_PROCESS_DETACH:
		Mhook_Unhook((PVOID*)&OrigLoadLibraryA);
		Mhook_Unhook((PVOID*)&OrigLoadLibraryW);
		Mhook_Unhook((PVOID*)&OrigLoadLibraryExA);
		Mhook_Unhook((PVOID*)&OrigLoadLibraryExW);
		if(ALREADY_HOOKED){
			//unhook winscard too
			Mhook_Unhook((PVOID*)&OrigSCardTransmit);
			ALREADY_HOOKED = false;
		}
		break;
	}
	return TRUE;
}

