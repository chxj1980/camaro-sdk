// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "UVCPlugin_i.h"
#include "dllmain.h"
#include "Xuproxy.h"

CUVCPluginModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return _AtlModule.DllMain(dwReason, lpReserved); 
}
