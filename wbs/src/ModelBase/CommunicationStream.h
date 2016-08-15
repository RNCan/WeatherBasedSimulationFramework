//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/Location.h"
#include "ModelBase/InputParam.h"
#include "ModelBase/WGInput.h"

namespace WBSF
{
	class CModelStatVector;
	class CWeatherStation;



	class CFileStaticData
	{
	public:

		CFileStaticData(const std::string& filePath = "", const std::string& data = "")
		{
			m_filePath = filePath;
			m_data = data;
		}

		bool operator == (const std::string& in)const { return m_filePath == in; }
		bool operator != (const std::string&  in)const { return !operator==(in); }

		std::string m_filePath;
		std::string m_data;
	};

	class CFileStaticDataVector : public std::vector < CFileStaticData >
	{
	public:

		ERMsg Load(const std::vector<std::string>& filePath);
		int Find(const std::string& filePath)const;
	};

	class CStaticDataStream
	{
	public:

		enum TSection { FILES, NB_SECTIONS };



		ERMsg WriteStream(std::ostream& stream);
		ERMsg ReadStream(std::istream& stream);
		const std::string& GetFileData(const std::string& filePath)const;
		const std::string& GetFileData(int index)const;

		CFileStaticDataVector m_files;
	};

	//****************************************************************
	// CTransferInfoIn 

	class CTransferInfoIn
	{
	public:

		enum TMember{ TRANSFER_TYPE, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NORMAL_FILE_PATH, DAILY_FILE_PATH, HOURLY_FILE_PATH, LOC_FILE_PATH, MODEL_NAME, TG_INPUT_NAME, MODEL_INPUT_NAME, PARAMETERS_VARIATIONS_NAME, LOC_COUNTER, PARAM_COUNTER, REP_COUNTER, LOCATION, WG_INPUT, INPUT_PARAM, OUTPUT_VARIABLES, SEED, OUTPUT_TM, LANGUAGE, NB_MEMBERS };
		//OUTPUT_PARAM, 
		enum TTransferVersion{ VERSION_TEXT, VERSION_XML, VERSION_STREAM, NB_TRANSFER_VERSION };

		static const char* GetMemberName(int i){ _ASSERTE(i >= 0 && i < NB_MEMBERS); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CTransferInfoIn();
		void Reset();

		bool operator ==(const CTransferInfoIn& in)const;
		bool operator !=(const CTransferInfoIn& in)const{ return !operator==(in); }

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath)const;
		void WriteStream(std::ostream& stream)const;//Create XML stream 
		ERMsg ReadStream(std::istream& stream);		//XML pass in stream


		short	m_transferTypeVersion;			//transfer type 0=VERSION_TEXT, 1=VERSION_XML, 2=VERSION_STREAM
		std::string m_inputInfoPath;		//transfile file path (for text and XML transfer type)
		std::string m_inputWeatherFilePath;	//weather input file path in csv format with header
		std::string m_outputFilePath;		//output data file path in csv format and header

		std::string m_modelName;
		std::string m_normalDBFilePath;		//file path of normals database
		std::string m_dailyDBFilePath;		//file path of daily database
		std::string m_hourlyDBFilePath;		//file path of hourly database
		std::string m_locationsFilePath;	//file path of location list
		std::string m_WGInputName;			//WG input name
		std::string m_modelInputName;		//Model input name
		std::string m_parametersVariationsName;//model input parameters variations file path
		CCounter m_locCounter;					//location count
		CCounter m_paramCounter;				//parameter count
		CCounter m_repCounter;					//iteration count

		CLocation m_loc;						//info about simulation point
		CWGInput m_WG;							//info about the weather generation parameters used to generate input file
		CParameterVector m_inputParameters;		//input parameters names and values
		CParameterVector m_outputVariables;		//output variables types and names
		size_t m_seed;							//0=RANDOM, 1=FIXE    

		CTM		m_TM;							//output file temporal type
		short	m_language;						//language: 0=FRENCH, 1 = ENGLISH





	protected:

		ERMsg SaveV1(const std::string& filePath)const;

		std::string m_filePath;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBERS];
	};



	class CTransferInfoOut
	{
	public:

		enum { LOC_COUNTER, PARAM_COUNTER, REP_COUNTER, OUTPUT_TM, ERROR_MSG, NB_MEMBER };
		//enum TTransferVersion{ VERSION_TEXT_OLD, VERSION_TEXT, VERSION_XML, VERSION_STREAM};
		static const char* GetMemberName(int i){ _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CTransferInfoOut();
		void Reset();

		bool operator ==(const CTransferInfoOut& in)const;
		bool operator !=(const CTransferInfoOut& in)const{ return !operator==(in); }

		size_t GetSectionNo()const;
		CCounter m_locCounter;					//location count
		CCounter m_paramCounter;				//parameter count
		CCounter m_repCounter;					//iteration count	

		CTM m_TM;								//
		ERMsg m_msg;							//error message;


		void WriteStream(std::ostream& outStream)const;//Create XML stream 
		ERMsg ReadStream(std::istream& inStream);//XML pass in stream

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);
		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		//void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }

	protected:

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};


	class CCommunicationStream
	{
	public:

		static void WriteInputStream(const CTransferInfoIn& info, const CWeatherStation&  weather, std::ostream& stream);
		static ERMsg ReadInputStream(std::istream& stream, CTransferInfoIn& info, CWeatherStation& weather);

		static void WriteOutputStream(const CTransferInfoOut& info, const CModelStatVector& data, std::ostream& stream);
		static ERMsg ReadOutputStream(std::istream& stream, CTransferInfoOut& info, CModelStatVector& data);
		static ERMsg ReadOutputStream(std::istream& stream, CTransferInfoOut& info);

		static ERMsg GetErrorMessage(std::istream& stream);
	};


}


namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CTransferInfoIn& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::TRANSFER_TYPE)](in.m_transferTypeVersion);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::INPUT_FILE_PATH)](in.m_inputWeatherFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_FILE_PATH)](in.m_outputFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::NORMAL_FILE_PATH)](in.m_normalDBFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::DAILY_FILE_PATH)](in.m_dailyDBFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::HOURLY_FILE_PATH)](in.m_hourlyDBFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOC_FILE_PATH)](in.m_locationsFilePath);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::MODEL_NAME)](in.m_modelName);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::TG_INPUT_NAME)](in.m_WGInputName);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::MODEL_INPUT_NAME)](in.m_modelInputName);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::PARAMETERS_VARIATIONS_NAME)](in.m_parametersVariationsName);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOC_COUNTER)](in.m_locCounter);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::PARAM_COUNTER)](in.m_paramCounter);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::REP_COUNTER)](in.m_repCounter);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOCATION)](in.m_loc);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::WG_INPUT)](in.m_WG);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::INPUT_PARAM)](in.m_inputParameters);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_VARIABLES)](in.m_outputVariables);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::SEED)](in.m_seed);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_TM)](in.m_TM);
		out[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LANGUAGE)](in.m_language);

	}




	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CTransferInfoIn& out)
	{
		XmlIn in(input);

		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::TRANSFER_TYPE)](out.m_transferTypeVersion);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::INPUT_FILE_PATH)](out.m_inputWeatherFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_FILE_PATH)](out.m_outputFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::NORMAL_FILE_PATH)](out.m_normalDBFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::DAILY_FILE_PATH)](out.m_dailyDBFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::HOURLY_FILE_PATH)](out.m_hourlyDBFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOC_FILE_PATH)](out.m_locationsFilePath);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::MODEL_NAME)](out.m_modelName);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::TG_INPUT_NAME)](out.m_WGInputName);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::MODEL_INPUT_NAME)](out.m_modelInputName);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::PARAMETERS_VARIATIONS_NAME)](out.m_parametersVariationsName);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOC_COUNTER)](out.m_locCounter);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::PARAM_COUNTER)](out.m_paramCounter);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::REP_COUNTER)](out.m_repCounter);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LOCATION)](out.m_loc);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::WG_INPUT)](out.m_WG);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::INPUT_PARAM)](out.m_inputParameters);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_VARIABLES)](out.m_outputVariables);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::SEED)](out.m_seed);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::OUTPUT_TM)](out.m_TM);
		in[WBSF::CTransferInfoIn::GetMemberName(WBSF::CTransferInfoIn::LANGUAGE)](out.m_language);

		return true;
	}


		template <> inline
			void writeStruc(const WBSF::CTransferInfoOut& in, XmlElement& output)
		{
			XmlOut out(output);

			std::string msg = WBSF::GetErrorString(in.m_msg, "¦");
			out[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::LOC_COUNTER)](in.m_locCounter);
			out[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::PARAM_COUNTER)](in.m_paramCounter);
			out[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::REP_COUNTER)](in.m_repCounter);
			out[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::OUTPUT_TM)](in.m_TM);
			out[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::ERROR_MSG)](msg);


		}

		template <> inline
			bool readStruc(const XmlElement& input, WBSF::CTransferInfoOut& out)
		{
			XmlIn in(input);

			std::string msg;
			in[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::LOC_COUNTER)](out.m_locCounter);
			in[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::PARAM_COUNTER)](out.m_paramCounter);
			in[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::REP_COUNTER)](out.m_repCounter);
			in[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::OUTPUT_TM)](out.m_TM);
			in[WBSF::CTransferInfoOut::GetMemberName(WBSF::CTransferInfoOut::ERROR_MSG)](msg);
			out.m_msg = WBSF::GetErrorMsgFromString(msg, "¦");

			return true;
		}


}
