// ZeligWeatherTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <crtdbg.h>
#include <windows.h>

#if ((ULONG_MAX) == (UINT_MAX))
# define IS32BIT
#else
# define IS64BIT
#endif

enum TZeligWeatherOutput { OUT_T_MEAN, OUT_T_SD, OUT_P_TOT, OUT_P_SD, NB_OUTPUS };
typedef float ZeligWeatherOutput[12][NB_OUTPUS];
typedef float ZeligWeatherNormals[12][11];

typedef bool(*LoadZeligWeatherF)(const char* path);
typedef bool(*GetZeligWeatherF)(double latitude, double longitude, double elevation, ZeligWeatherOutput output);
typedef bool(*GetZeligNormalsF)(double latitude, double longitude, double elevation, ZeligWeatherNormals output);
typedef bool(*GetZeligWGF)(const char* path, double latitude, double longitude, double elevation, ZeligWeatherNormals output);

int main()
{

	//CDynamicResources::set(GetModuleHandle(NULL));

	//HMODULE hDll = LoadLibrary(L"D:\\Project\\wbsTools\\ZeligWeather\\Debug\\ZeligWeather_d.dll");
#ifdef _DEBUG
#ifdef WIN64
	HMODULE hDll = LoadLibrary(L"D:\\Project\\wbsTools\\ZeligWeather\\x64\\Debug\\ZeligWeather_d.dll");
#else
	HMODULE hDll = LoadLibrary(L"D:\\Project\\wbsTools\\ZeligWeather\\Debug\\ZeligWeather_d.dll");
#endif
#else
#ifdef WIN64
	HMODULE hDll = LoadLibrary(L"D:\\Project\\wbsTools\\ZeligWeather\\x64\\Release\\ZeligWeather.dll");
#else
	HMODULE hDll = LoadLibrary(L"D:\\Project\\wbsTools\\ZeligWeather\\Release\\ZeligWeather.dll");
#endif
#endif

	_ASSERTE(hDll != NULL);

	LoadZeligWeatherF LoadZeligWeather = (LoadZeligWeatherF)GetProcAddress(hDll, "LoadZeligWeather");
	GetZeligWeatherF GetZeligWeather = (GetZeligWeatherF)GetProcAddress(hDll, "GetZeligWeather");
	GetZeligNormalsF GetZeligNormals = (GetZeligNormalsF)GetProcAddress(hDll, "GetZeligNormals");
	GetZeligWGF GetZeligWG = (GetZeligWGF)GetProcAddress(hDll, "GetZeligWG");
	

	bool bLoad = LoadZeligWeather("D:\\Project\\wbsTools\\ZeligWeather\\ZeligWeatherTest\\Canada 1981-2010.NormalsDB");
	//bool bLoad = LoadZeligWeather("H:\\BioSIM_Database\\Normals\\Canada 1981-2010.NormalsDB");
	//
	_ASSERTE(bLoad);

	ZeligWeatherOutput output = { 0 };
	bool bGetWeather;
	//
	//std::cout << "Meteo pour 45.1333 -74.35	53.3" << std::endl;
	//bool bGetWeather = GetZeligWeather(45.1333, -74.35, 53.3, output);
	//_ASSERTE(bLoad);
	//
	//for (size_t m = 0; m < 12; m++)
	//{
	//	for (size_t i = 0; i < 4; i++)
	//		std::cout << output[m][i] << "\t";

	//	std::cout << std::endl;
	//}

	
	std::cout << "Meteo pour AC109 (49.5197 -85.4365	324)" << std::endl;
	bGetWeather = GetZeligWeather(49.5197,-85.4365,324, output);

	_ASSERTE(bLoad);

	for (size_t m = 0; m < 12; m++)
	{
		for (size_t i = 0; i < 4; i++)
			std::cout << output[m][i] << "\t";

		std::cout << std::endl;
	}

	std::cout << "Normals pour AC109" << std::endl;
	ZeligWeatherNormals normals = { 0 };
	bGetWeather = GetZeligNormals(49.5197, -85.4365, 324, normals);
	for (size_t m = 0; m < 12; m++)
	{
		for (size_t f = 0; f < 11; f++)
			std::cout << normals[m][f] << "\t";

		std::cout << std::endl;
	}

	std::cout << "WG pour AC109" << std::endl;
	ZeligWeatherNormals WG = { 0 };
	bGetWeather = GetZeligWG("D:\\Project\\wbsTools\\ZeligWeather\\ZeligWeatherTest\\Canada 1981-2010.NormalsDB", 49.5197, -85.4365, 324, WG);
	for (size_t m = 0; m < 12; m++)
	{
		for (size_t f = 0; f < 11; f++)
			std::cout << WG[m][f] << "\t";

		std::cout << std::endl;
	}


	/*std::cout << "Meteo pour Ontario" << std::endl;


	double OntarioLoc[5][3] =
	{
		{49.535,-85.7865,333},
		{49.5197,-85.4365,324},
		{49.4583,-85.2972,331},
		{49.3767,-86.014,284},
		{49.3482,-85.8573,341}
	};

	

	
	for (size_t l = 0; l < 5; l++)
	{
		ZeligWeatherOutput output = { 0 };
		bool bGetWeather = GetZeligWeather(OntarioLoc[l][0], OntarioLoc[l][1], OntarioLoc[l][2], output);
		_ASSERTE(bLoad);

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t v = 0; v < 4; v++)
				std::cout << output[m][v] << "\t";

			std::cout << std::endl;
		}
	}
*/

	FreeLibrary(hDll);
	hDll = NULL;
}
