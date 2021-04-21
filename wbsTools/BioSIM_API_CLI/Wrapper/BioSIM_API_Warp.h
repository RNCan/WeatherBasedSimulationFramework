//Entity.h
#pragma once
#include "ManagedObject.h"
#include "../Core/BioSIM_API.h"

using namespace System;
namespace BioSIM_Wrapper
{
	using namespace System::Runtime::InteropServices;
	static std::string string_to_stdstring(const String^ string)
	{
		const char* chars= (const char*)(Marshal::StringToHGlobalAnsi(const_cast<String^>(string))).ToPointer();
		std::string str(chars);
		Marshal::FreeHGlobal(IntPtr((void*)chars));

		return str;
	}

	
	static String^ stdstring_to_string(const std::string& string)
	{
		System::String^ str = gcnew System::String(string.c_str(), 0, int(string.length()), System::Text::Encoding::UTF8);
		return str;
	}

	


	public ref class TeleIO
	{
	public:

		TeleIO();
		TeleIO(const WBSF::CTeleIO& in);
		TeleIO(const TeleIO% IO);


		bool compress;	//if output is compress or not
		String^ msg;		//error message
		String^ comment;	//comments
		String^ metadata;	//output metadatan in XML
		cli::array<unsigned char>^ data;
	};


	public ref class WeatherGenerator : public ManagedObject<WBSF::WeatherGenerator>
	{

	public:

		//enum TParam { SHORE, NORMALS, DAILY, HOURLY, GRIBS, DEM, NB_PAPAMS };

		WeatherGenerator(const String^ name);
		String^ Initialize(const String^ str_options);
		TeleIO^ Generate(const String^ str_options);
		TeleIO^ GetNormals(const String^ str_options);

	};


	public ref class ModelExecution : public ManagedObject<WBSF::ModelExecution>
	{

	public:

		//enum TParam { MODEL, NB_PAPAMS };

		ModelExecution(const String^ name);
		String^ Initialize(const String^ str_options);
		TeleIO^ Execute(const String^ str_options, const TeleIO^ input);
		String^ GetWeatherVariablesNeeded();
		String^ GetDefaultParameters();
		String^ Help();


	};



}

