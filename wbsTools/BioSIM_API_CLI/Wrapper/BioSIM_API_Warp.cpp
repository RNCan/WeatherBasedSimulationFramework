//Entity.cpp
#include "BioSIM_API_Warp.h"

namespace BioSIM_Wrapper
{
	
	/*static void int_array_conversion(array<int>^ data)
	{
		pin_ptr<unsigned int> arrayPin = &data[0];
		unsigned int size = data->Length;
	}*/

	TeleIO::TeleIO()
	{}


	//TeleIO::TeleIO(bool compress, String^ msg, String^ comment, String^ metadata, String^ data)
	//	//:	ManagedObject(new WBSF::CTeleIO(compress, string_to_stdstring(msg), string_to_stdstring(comment), string_to_stdstring(metadata), string_to_stdstring(data)))
	//{
	//	compress= compress;	//if output is compress or not
	//	msg= msg;		//error message
	//	comment= comment;	//comments
	//	metadata= metadata;	//output metadatan in XML
	//	data= data;		//output data 
	//}


	//TeleIO::TeleIO(const TeleIO^ IO)
	//{
	//	compress = IO->compress;	//if output is compress or not
	//	msg = IO->msg;		//error message
	//	comment = IO->comment;	//comments
	//	metadata = IO->metadata;	//output metadatan in XML
	//	data = IO->data;		//output data 
	//}

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
		
		//pin_ptr<unsigned char> contentPtr = &(in.data[0]);
		//data = std::string(contentPtr, in.data.Length());
		
		data = gcnew cli::array<unsigned char>(int(in.m_data.length()));
		for (size_t i = 0; i < in.m_data.size(); ++i)
			data[int(i)] = in.m_data[i];

		//System::Runtime::InteropServices::Marshal::Copy(IntPtr(&(const_cast<std::string&>(in.m_data)[0])), data, 0, int(in.m_data.length()));



		/*if(compress)
			data = gcnew String(in.m_data.c_str(), in.m_data.length());
		else 
			data = gcnew String(in.m_data.c_str(), in.m_data.length());*/
	}

	/*void TeleIO::GetTeleIO(WBSF::CTeleIO& out)const
	{
		out.m_compress = compress;
		out.m_msg = string_to_stdstring(msg);
		out.m_comment = string_to_stdstring(comment);
		out.m_metadata = string_to_stdstring(metadata);
		out.m_data = string_to_stdstring(data);
	}
*/
	//*********************************


	//WeatherGeneratorOptions::WeatherGeneratorOptions() :
		//ManagedObject(new WBSF::WeatherGeneratorOptions())
	//{
	//}

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
		//IOout = gcnew TeleIO;
/*

		IOout.compress = IO.m_compress;
		IOout.msg = gcnew String(IO.m_msg.c_str());
		IOout.comment = gcnew String(IO.m_comment.c_str());
		IOout.metadata = gcnew String(IO.m_metadata.c_str());
		IOout.data = gcnew String(IO.m_data.c_str());
*/
//return IOout;
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
		//IO_in.m_data. = gcnew String( input->data..ToByte(), input->data->Length);
		
		//array<unsigned char>^ const content = GetArray();
		//pin_ptr<unsigned char> contentPtr = &input->data[0];
		//IO_in.m_data = std::string(contentPtr, input->data.Length());

		//IO_in.m_data.assign((&input->data[0]), (&input->data[0]) + input->data->Length + 1);

		//PVOID data;
		//...some code in another DLL for writing to "data" using &data
		//BYTE* dataBYTE = (BYTE*)IO_in.m_data;
		////array<unsigned char>^ data2 = gcnew array<unsigned char>(strlen((char*)dataBYTE));
		//IO_in.m_data.resize(input->data->Length);
		//CopyBYTEToManagedByte(dataBYTE, input->data); // My function for doing the above problem Marshal::Copy.

		//array<unsigned char>^ dataRet = gcnew array<unsigned char>(ms->Length); //ms = MemoryStream from earlier code
		//BYTE* dataR = new BYTE[dataRet->Length];
		//dataR[dataRet->Length] = '\0';

		IO_in.m_data.resize(input->data->Length);
		for (size_t i = 0; i < input->data->Length; ++i)
			IO_in.m_data[i] = input->data[int(i)];


		//CopyManagedByteToBYTE(input->data, IO_in.m_data); // Earlier method Marshal::Copy.
		//*pvBuffer = (PVOID)dataR; // pvBuffer = PVOID* parameter for buffer.

		
		//std::memcpy(&IO_in.m_data[0], IntPtr((void *)input->data), input->data->Length);
		//System::Runtime::InteropServices::Marshal::Copy(IntPtr(&IO_in.m_data[0]), input->data, 0, input->data->Length);

			
		//Text::Encoding^ enc = Text::Encoding::ASCII;
		//s = enc->GetString(input->data);



		//IO_in.m_data.resize(input->data->Length);
		

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