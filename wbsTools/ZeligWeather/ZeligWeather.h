// ZeligWeather.cpp : Defines the exported functions for the DLL application.
//
#pragma once


#ifdef  ZELIGWEATHER_EXPORTS 
	/*Enabled as "export" while compiling the dll project*/
#define DLL_API __declspec(dllexport)  
#else
   /*Enabled as "import" in the Client side for using already created dll file*/
#define DLL_API __declspec(dllimport)  
#endif

extern "C"
{

	enum TZeligWeatherOutput { OUT_T_MEAN, OUT_T_SD, OUT_P_TOT, OUT_P_SD, NB_OUTPUS };
	typedef float ZeligWeatherOutput[12][NB_OUTPUS];



	DLL_API bool LoadZeligWeather(const char* path);
	DLL_API bool GetZeligWeather(double latitude, double longitude, double elevation, ZeligWeatherOutput output);
	DLL_API bool GetZeligNormals(double latitude, double longitude, double elevation, float out_normals[12][11]);
	DLL_API bool GetZeligWG(const char* NFilePath, double latitude, double longitude, double elevation, float out_normals[12][11]);
}