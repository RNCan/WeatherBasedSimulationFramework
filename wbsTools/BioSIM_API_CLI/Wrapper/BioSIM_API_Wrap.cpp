//Entity.cpp
#include "BioSIM_API_Wrap.h"

namespace BioSIM_Wrapper
{
	

	TeleIO::TeleIO()
	{}

	TeleIO::TeleIO(const TeleIO% IO)
	{
		compress = IO.compress;	//if output is compress or not
		msg = IO.msg;		//error message
		comment = IO.comment;	//comments
		metadata = IO.metadata;	//output metadatan in XML
		data = IO.data;		//output data 
	}


	TeleIO::TeleIO(const WBSF::CTeleIO& in)
	{
		compress = in.m_compress;
		msg = gcnew String(in.m_msg.c_str());
		comment = gcnew String(in.m_comment.c_str());
		metadata = gcnew String(in.m_metadata.c_str());
		
		data = gcnew cli::array<unsigned char>(int(in.m_data.length()));
		for (size_t i = 0; i < in.m_data.size(); ++i)
			data[int(i)] = in.m_data[i];
	}

	//*********************************

	WeatherGenerator::WeatherGenerator(const String^ name) :
		ManagedObject(new WBSF::WeatherGenerator(string_to_stdstring(name)))
	{
	}

	String^ WeatherGenerator::Initialize(const String^ str_options)
	{
		return stdstring_to_string(m_Instance->Initialize(string_to_stdstring(str_options)));
	}



	TeleIO^ WeatherGenerator::Generate(const String^ str_options)
	{
		WBSF::CTeleIO IO = m_Instance->Generate(string_to_stdstring(str_options));
		return gcnew TeleIO(IO);
	}


	TeleIO^ WeatherGenerator::GetNormals(const String^ str_options)
	{
		WBSF::CTeleIO IO = m_Instance->Generate(string_to_stdstring(str_options));
		return gcnew TeleIO(IO);
	}

	//************************************************************************
	ModelExecution::ModelExecution(const String^ name) :
		ManagedObject(new WBSF::ModelExecution(string_to_stdstring(name)))
	{
	}


	String^ ModelExecution::Initialize(const String^ str_options)
	{
		std::string msg = m_Instance->Initialize(string_to_stdstring(str_options));
		String^ msg_out = stdstring_to_string(msg);
	
	
		return msg_out;
	}



	TeleIO^ ModelExecution::Execute(const String^ str_options, const TeleIO^ input)
	{
		WBSF::CTeleIO IO_in;
		
		IO_in.m_compress = input->compress;
		IO_in.m_msg = string_to_stdstring(input->msg);
		IO_in.m_comment = string_to_stdstring(input->comment);
		IO_in.m_metadata = string_to_stdstring(input->metadata);

		IO_in.m_data.resize(input->data->Length);
		for (size_t i = 0; i < input->data->Length; ++i)
			IO_in.m_data[i] = input->data[int(i)];


		WBSF::CTeleIO IO_out = m_Instance->Execute(string_to_stdstring(str_options), IO_in);

		return gcnew TeleIO(IO_out);
	}

	String^ ModelExecution::GetWeatherVariablesNeeded()
	{
		std::string str = m_Instance->GetWeatherVariablesNeeded();
		return gcnew String(str.c_str());
	}

	String^ ModelExecution::GetDefaultParameters()
	{
		std::string str = m_Instance->GetDefaultParameters();
		return gcnew String(str.c_str());
	}

	String^ ModelExecution::Help()
	{
		std::string str = m_Instance->Help();
		return gcnew String(str.c_str()); 
	}




}