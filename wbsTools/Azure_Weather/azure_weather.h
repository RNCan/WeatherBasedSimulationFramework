#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the AZURE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// AZURE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef AZURE_EXPORTS
#define AZURE_API __declspec(dllexport)
#else
#define AZURE_API __declspec(dllimport)
#endif


//#include <string>

//AZURE_API bool download_azure_blob_to_stream(const std::string& account_name, const std::string& account_key, const std::string& container_name, const std::string &blob, unsigned long long offset, unsigned long long size, std::ostream &os);
//AZURE_API bool init_connection(const std::string& connextion_name, const std::string& account_name, const std::string& account_key, const std::string& container_name)

//extern "C" AZURE_API bool load_azure_weather_years(const std::string& account_name, const std::string& account_key, const std::string& container_name, const std::string &blob_name, void* pData);
//extern "C"
//{
//	AZURE_API bool load_azure_weather_years(const char* account_name, const char* account_key, const char*  container_name, const char* blob_name, void* pData);
//}


