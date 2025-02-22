// dllmain.cpp : Defines the entry point for the DLL application.
#include "Basic/UtilStd.h"
#include <windows.h>

#include "Geomatic/UtilGDAL.h"
#include "Basic/DynamicRessource.h"
#include "Basic/WeatherDatabase.h"

#pragma warning(disable: 4275 4251 4005)
#include "gdal_priv.h"


HMODULE g_hDLL = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH: 
    {
        g_hDLL = hModule;
        CDynamicResources::set(g_hDLL);
        char path[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(g_hDLL, path, sizeof(path)) != 0)
        {
            WBSF::CWeatherDatabase::set_azure_dll_filepath(WBSF::GetPath(path) + "azure_weather.dll");
        }

        GDALSetCacheMax64(128 * 1024 * 1024);
        WBSF::RegisterGDAL();


        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

