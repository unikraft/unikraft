// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>

bool g_isWin81OrLater = true;

bool isWin81OrLater();

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
    (void)hModule;
    (void)lpReserved;
    switch (ul_reason_for_call) {
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        return TRUE;
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        break;
    }

    // Get Windows Version
    g_isWin81OrLater = isWin81OrLater();

    return TRUE;
}
