//****************************************************************************
// File:	Model.cpp
// Class:	CTemporalRef, CModel
//****************************************************************************
// 08/12/2014  Rémi Saint-Amant	Replace CWCategory by CWVariables
// 04/07/2013  Rémi Saint-Amant	Add SetStaticData
// 14/11/2011  Rémi Saint-Amant Add THREAD_SAFE code
// 03/08/2011  Rémi Saint-Amant Add behavior info
// 01/06/2010  Rémi Saint-Amant	Add m_bOutputJulianDay to manage old model compatibility
// 10/05/2010  Rémi Saint-Amant	Replace outputType by Type/Mode
// 11/12/2008  Rémi Saint-Amant	push_back simulated Annealing functionality
// 15/09/2008  Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"
#include <float.h>

#include "ModelBase/Model.h"
#include "ModelBase/ModelInput.h"
#include "ModelBase/WGInput.h"
#include "ModelBase/CommunicationStream.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;



namespace WBSF
{


#define SUCCESS						0x00000000
	// general error
#define UNKNOW_ERROR				0xffffffff


#define FIRST_MODEL_ERROR			0x00000021
#define MODEL_ERROR1				0x00000021 //bad input file
#define MODEL_ERROR2				0x00000022 //unable read ids file
#define MODEL_ERROR3				0x00000023 //unable read tgo file
#define MODEL_ERROR4				0x00000024 //unable write ido file
#define MODEL_ERROR5				0x00000025 //not enought memory
#define MODEL_ERROR7				0x00000027 //disk space
#define LAST_MODEL_ERROR			0x00000027


	static const char* TNAME(size_t i){ return GetTemporalName(i); }



	const char* CModel::FILE_EXT = ".mdl";
	const char* CModel::XML_FLAG = "BioSIMModel";
	const char* CModel::MEMBER_NAME[NB_MEMBER] = { "Title", "Version", "ID", "EXE", "Behaviour", "Description", "WindowRect", "SimulatedCategory", "TransferFileVersion", "IOFileVersion", "NbYearMin", "NbYearMax", "ThreadSafe", "InputVariableArray", "SSI", "OutputTypeMode", "JulianDay", "MissingValue", "OutputVariableArray", "Credit", "Copyright", "HelpType", "HelpFileName", "HelpText" };
	const short CModel::TRANSLATED_MEMBER[NB_TRANSLATED_MEMBER] = { TITLE, DESCRIPTION, CREDIT, COPYRIGHT, HELP_FILENAME, HELP_TEXT };

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////


	CModel::CModel()
	{
		m_hDll = NULL;
		m_RunModelFile = NULL;
		m_RunModelStream = NULL;
		Reset();
		ASSERT(!GetString(IDS_BSC_ERRORINMODEL).empty());
	}

	CModel::CModel(const CModel& in)
	{
		m_hDll = NULL;
		operator = (in);
	}

	CModel::~CModel()
	{
		UnloadDLL();
	}

	

	void CModel::Reset()
	{
		UnloadDLL();
		assert(m_hDll == NULL);
		assert(m_RunModelFile == NULL);
		assert(m_RunModelStream == NULL);

		m_windowRect = { 0, 0, 250, 100 };
		m_SetStaticData = NULL;
		m_GetSimulatedAnnealingSize = NULL;
		m_SetSimulatedAnnealingSize = NULL;
		m_InitSimulatedAnnealing = NULL;
		m_GetFValue = NULL;

		m_filePath.clear();
		m_title.clear();
		m_version = "1.0";
		m_behaviour = DETERMINISTIC;
		m_variables = "T";
		m_outputTM = CTM(CTM::DAILY);
		m_missValue = MISSING;

		m_transferFileVersion = VERSION_XML;
		//m_IOFileVersion=VERSION_ASCII_WITH_HEADER;
		m_nbYearMin = 1;
		m_nbYearMax = 999;
		m_bThreadSafe = true;
		m_SSI.clear();

		m_inputList.clear();
		m_outputList.clear();
	}

	ERMsg CModel::Load(const string& filePath, short language)
	{
		ASSERT(!filePath.empty());

		ERMsg msg;

		m_filePath = filePath;
		int version = GetFileVersion(filePath);

		if (version < 3)
		{
			msg.ajoute("Unsupported old model version. Ask conversion to developer");
		}
		else
		{
			ERMsg msg;

			//zen::XmlDoc doc;
			//msg = load(filePath, doc);

			//if (msg)
			//{
			msg = zen::LoadXML(filePath, "Model", "3", *this);
			//}
		}

		return msg;
	}

	ERMsg CModel::Save(const string& filePath)
	{
		return zen::SaveXML(filePath, "Model", "3", *this);
	}

	//********************************************************

	bool CModel::operator == (const CModel& in)const
	{
		bool bEqual = true;

		if (m_title != in.m_title) bEqual = false;
		if (m_version != in.m_version) bEqual = false;
		if (m_extension != in.m_extension) bEqual = false;
		if (m_DLLName != in.m_DLLName) bEqual = false;
		if (m_behaviour != in.m_behaviour) bEqual = false;
		if (m_description != in.m_description) bEqual = false;
		if (m_windowRect.left != in.m_windowRect.left) bEqual = false;
		if (m_windowRect.right != in.m_windowRect.right) bEqual = false;
		if (m_windowRect.top != in.m_windowRect.top) bEqual = false;
		if (m_windowRect.bottom != in.m_windowRect.bottom) bEqual = false;
		if (m_credit != in.m_credit) bEqual = false;
		if (m_transferFileVersion != in.m_transferFileVersion)bEqual = false;
		if (m_SSI != in.m_SSI)bEqual = false;
		if (m_outputTM != in.m_outputTM)bEqual = false;
		if (m_missValue != in.m_missValue)bEqual = false;
		if (m_nbYearMin != in.m_nbYearMin)bEqual = false;
		if (m_nbYearMax != in.m_nbYearMax)bEqual = false;
		if (m_bThreadSafe != in.m_bThreadSafe)bEqual = false;
		if (m_variables != in.m_variables)bEqual = false;
		if (m_copyright != in.m_copyright)bEqual = false;
		if (m_helpFileName != in.m_helpFileName)bEqual = false;
		if (m_inputList != in.m_inputList)bEqual = false;;
		if (m_outputList != in.m_outputList)bEqual = false;;

		return bEqual;
	}

	bool CModel::operator != (const CModel& model)const
	{
		return !(*this == model);
	}

	CModel& CModel::operator = (const CModel& in)
	{
		if (&in != this)
		{
			//m_name = in.m_name;
			m_filePath = in.m_filePath;
			m_title = in.m_title;
			m_version = in.m_version;
			m_extension = in.m_extension;
			m_DLLName = in.m_DLLName;
			m_behaviour = in.m_behaviour;
			m_description = in.m_description;


			m_windowRect = in.m_windowRect;
			m_credit = in.m_credit;

			m_inputList = in.m_inputList;
			m_outputList = in.m_outputList;

			m_variables = in.m_variables;
			m_transferFileVersion = in.m_transferFileVersion;
			m_outputTM = in.m_outputTM;
			m_SSI = in.m_SSI;
			//m_bOutputJulianDay = in.m_bOutputJulianDay;

			m_missValue = in.m_missValue;
			//m_IOFileVersion = in.m_IOFileVersion;
			m_nbYearMin = in.m_nbYearMin;
			m_nbYearMax = in.m_nbYearMax;
			m_bThreadSafe = in.m_bThreadSafe;

			m_copyright = in.m_copyright;
			// m_helpType = in.m_helpType;
			m_helpFileName = in.m_helpFileName;
			//m_helpText = in.m_helpText;

			UnloadDLL();
		}

		ASSERT(in == *this);

		return *this;
	}

	void CModel::GetDefaultParameter(CModelInput& modelInput)const
	{
		modelInput.clear();
		modelInput.SetExtension(m_extension);
		modelInput.SetName(STRDEFAULT);

		for (size_t i = 0; i < m_inputList.size(); i++)
		{
			int type = m_inputList[i].GetType();
			if (type != CModelInputParameterDef::kMVTitle &&
				type != CModelInputParameterDef::kMVLine &&
				type != CModelInputParameterDef::kMVStaticText)
			{
				CModelInputParam p;
				p.m_name = m_inputList[i].m_name;
				//			p.m_type = m_inputList[i].m_type;
				p.m_value = m_inputList[i].m_default;
				modelInput.push_back(p);
			}
		}
	}

	void CModel::SetDefaultParameter(const CModelInput& modelInput)
	{
		for (size_t i = 0, p = 0; i < m_inputList.size() && p < modelInput.size(); i++)
		{
			int type = m_inputList[i].GetType();
			if (type != CModelInputParameterDef::kMVTitle &&
				type != CModelInputParameterDef::kMVLine &&
				type != CModelInputParameterDef::kMVStaticText)
			{
				m_inputList[i].m_name = modelInput[p].m_name;
				m_inputList[i].m_default = modelInput[p].m_value;
				p++;
			}
		}
	}

	ERMsg CModel::LoadDLL()
	{

		ERMsg msg;

		//m_CS.Enter();
		if (m_hDll == NULL)
		{
			string filePath = GetDLLFilePath();

			if (Find(filePath, ".dll"))
			{
				//it's not a exe but a dll.
				m_hDll = LoadLibrary(convert(filePath).c_str());
				if (m_hDll == NULL)
				{
					msg.ajoute(FormatMsg(IDS_BSC_UNABLE_LOADDLL, filePath));
				}
			}
		}
		//m_CS.Leave();

		return msg;
	}

	void CModel::UnloadDLL()
	{
		if (m_hDll)
		{
			//ASSERT(GetModuleHandleW(convert(GetDLLFilePath()).c_str()) != NULL);

			//to prevent a bug in the VCOMP100.dll we must wait 1 sec before closing dll
			//see http://support.microsoft.com/kb/94248
			Sleep(1000);
			bool bFree = FreeLibrary(m_hDll) != 0;
			if (bFree)
			{
				m_hDll = NULL;
				m_RunModelFile = NULL;
				m_RunModelStream = NULL;
				m_SetStaticData = NULL;
				//m_simulFonc2=NULL;
				m_GetSimulatedAnnealingSize = NULL;
				m_SetSimulatedAnnealingSize = NULL;
				m_InitSimulatedAnnealing = NULL;
				m_GetFValue = NULL;
			}



			//ASSERT(GetModuleHandleW(convert(GetDLLFilePath()).c_str()) == NULL);
		}
	}

	ERMsg  CModel::RunModel(const string & nameInputFile)
	{
		ERMsg msg;

		string filePath = GetDLLFilePath();

		if (Find(filePath, ".dll"))
		{
			msg = LoadDLL();

			if (msg && m_RunModelFile == NULL)
			{
				ASSERT(m_hDll != NULL);
				m_RunModelFile = (RunModelFileF)GetProcAddress(m_hDll, "RunModelFile");
			}

			if (msg)
			{
				if (m_RunModelFile != NULL)
					msg = RunModelDLL(nameInputFile);//dll
			}
		}
		else
		{
			msg = RunModelEXE(nameInputFile);
		}


		return msg;
	}

	ERMsg CModel::RunModel(std::istream& inStream, std::iostream& outStream)
	{
		ERMsg msg;

		if (m_RunModelStream == NULL)
		{
			msg = LoadDLL();
			if (msg)
			{
				ASSERT(m_hDll != NULL);
				m_RunModelStream = (RunModelStreamF)GetProcAddress(m_hDll, "RunModelStream");

				if (m_RunModelStream == NULL)
				{
					msg.ajoute(FormatMsg(IDS_BSC_UNABLE_GETFUNC, "RunModelStream", GetDLLFilePath()));
				}
			}
		}

		if (msg)
		{
			if (!m_RunModelStream(inStream, outStream))
				msg = CCommunicationStream::GetErrorMessage(outStream);
		}

		return msg;
	}

	ERMsg CModel::SetStaticData(std::istream& inStream)
	{
		ERMsg msg;

		//set static data on if they have data
		inStream.seekg(0, std::istream::end);
		if (inStream.tellg() > 0)
		{
			if (m_SetStaticData == NULL)
			{
				msg = LoadDLL();
				if (msg)
				{
					ASSERT(m_hDll != NULL);
					m_SetStaticData = (SetStaticDataF)GetProcAddress(m_hDll, "SetStaticData");

					if (m_SetStaticData == NULL)
					{
						msg.ajoute(FormatMsg(IDS_BSC_UNABLE_GETFUNC, "SetStaticData", GetDLLFilePath()));
					}
				}
			}

			if (msg)
			{
				inStream.seekg(0, std::istream::beg);
				m_SetStaticData(inStream);
			}
		}

		return msg;
	}

	ERMsg CModel::RunModelEXE(const string & nameInputFile)
	{
		ERMsg msg;

		string IDsFileName(nameInputFile);

		string exeName = GetDLLFilePath();


		IDsFileName = '"' + IDsFileName + '"';
		string command = exeName + " " + IDsFileName;

		STARTUPINFO si;
		::ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;


		PROCESS_INFORMATION pi;

		//char* pCommand = (char*) command.c_str();//.GetBuffer(1);
		wstring commandW = convert(command);
		if (::CreateProcessW(NULL, &(commandW[0]), NULL, NULL,
			FALSE, NULL, NULL, NULL, &si, &pi))//NORMAL_PRIORITY_CLASS
		{
			::CloseHandle(pi.hThread);
			::WaitForSingleObject(pi.hProcess, INFINITE);


			DWORD code;
			GetExitCodeProcess(pi.hProcess, &code);

			if (code != SUCCESS)
			{
				msg = FormatModelError(code);

				if (!GetDiskSpaceOk(nameInputFile))
				{
					msg.ajoute(GetString(IDS_BSC_DISK_FULL));
				}
			}

			::CloseHandle(pi.hProcess);
		}
		else
		{
			DWORD err = GetLastError();
			switch (err)
			{
			case ERROR_FILE_NOT_FOUND:
			{
				msg.ajoute(FormatMsg(IDS_BSC_EXE_NOTFOUND, exeName));
				break;
			}
			case ERROR_BAD_EXE_FORMAT:
			{
				msg.ajoute(FormatMsg(IDS_BSC_ERROR_BAD_EXE_FORMAT, exeName));
				break;
			}
			case ERROR_ACCESS_DENIED:
			default:
			{
				PSTR lpMsgBuf = NULL;
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
					err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPSTR)&lpMsgBuf, 0, NULL);

				msg.ajoute(FormatMsg(IDS_BSC_UNKNOW_ERROR, lpMsgBuf, exeName));

				break;
			}
			}
		}

		return msg;
	}

	ERMsg CModel::RunModelDLL(const string & nameInputFile)
	{

		//string IDsFileName(nameInputFile);
		ERMsg msg;

		ASSERT(m_RunModelFile != NULL);

		char errorMessage[1024] = { 0 };
		if (!m_RunModelFile(nameInputFile.c_str(), errorMessage))
		{
			msg.ajoute(errorMessage);

			if (!GetDiskSpaceOk(nameInputFile))
			{
				msg.ajoute(GetString(IDS_BSC_DISK_FULL));
			}
		}

		return msg;
	}



	bool CModel::GetDiskSpaceOk(const string & nameInputFile)
	{
		// receives the number of bytes on disk available to the caller
		ULARGE_INTEGER FreeBytesAvailableToCaller;
		// receives the number of bytes on disk
		ULARGE_INTEGER TotalNumberOfBytes;
		// receives the free bytes on disk
		ULARGE_INTEGER TotalNumberOfFreeBytes;
		string path = WBSF::GetPath(nameInputFile);

		//verification supplémentaire pour vérifier qu'il y a assez d'espace sur le disque
		GetDiskFreeSpaceExW(convert(path).c_str(), &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes);

		return FreeBytesAvailableToCaller.QuadPart > 500000;
	}

	ERMsg CModel::VerifyModelInput(const CModelInput& modelInput)const
	{
		return modelInput.IsValid(m_inputList);
	}


	ERMsg CModel::FormatModelError(int code)
	{
		string filePath = GetDLLFilePath(filePath);

		ERMsg msg;

		string error;

		//if (m_RunModelFile == NULL)
		//error = FormatMsg(IDS_SIM_ERRORINMODEL, filePath, ToString(code) );
		//else 
		error = FormatMsg(IDS_BSC_ERRORINDLL, m_DLLName, ToString(code));

		msg.ajoute(error);

		switch (code)
		{
			//no input file
		case MODEL_ERROR1:	error = GetString(IDS_BSC_NO_INPUT_FILE); break;
			//unable read ids file
		case MODEL_ERROR2:	error = GetString(IDS_BSC_UNABLE_READ_INPUT_FILE); break;
			//unable read tgo file
		case MODEL_ERROR3:	error = GetString(IDS_BSC_UNABLE_READ_TGO_FILE); break;
			//unable write ido file
		case MODEL_ERROR4:	error = GetString(IDS_BSC_UNABLE_WRITE_IDO_FILE); break;
			//not enought memory
		case MODEL_ERROR5:	error = GetString(IDS_BSC_NOT_ENOUGH_MEMORY); break;
		case MODEL_ERROR7:	error = GetString(IDS_BSC_DISK_FULL); break;
		default: break;
		}

		msg.ajoute(error);

		return msg;
	}


	string CModel::GetDLLVersion(const string& filePath)
	{
		string version;

		//regarder de près DLLGetVersion dans la doc pour voir si on peux utuliser ça
		HINSTANCE hDLL = LoadLibraryW(convert(filePath).c_str());
		GetModelVersionF GetModelVersion = NULL;
		if (hDLL)
			GetModelVersion = (GetModelVersionF)GetProcAddress(hDLL, "GetModelVersion");

		if (hDLL && GetModelVersion)
		{
			version = GetModelVersion();
		}
		else
		{
			//try with the version of the file info
			version = GetVersionString(filePath);
		}

		if (hDLL)
			FreeLibrary(hDLL);

		return version;
	}


	CTRef CModel::GetTRef(CTM TM, const std::vector<double>& in)
	{
		//ASSERT( in.size() == GetNbOutputReference(TM) );

		short p[4] = { 0 };
		p[0] = in.size() > 0 ? short(in[0]) : 0;
		p[1] = in.size() > 1 ? short(in[1]) : 0;
		p[2] = in.size() > 2 ? short(in[2]) : 0;
		p[3] = in.size() > 3 ? short(in[3]) : 0;

		CTRef ref;
		ref.Set(p[0], p[1] - 1, p[2] - 1, p[3] % 24, TM);

		return ref;
	}


	void CModel::SetExtension(const string& ext)
	{
		ASSERT(!ext.empty());
		ASSERT(ext[0] == '.');

		m_extension = ext;
	}

	CModelInputParameterDefVector CModel::GetInputDefinition(bool RemoveNonVariable)const
	{
		CModelInputParameterDefVector varList;
		if (RemoveNonVariable)
		{
			varList.clear();
			for (int i = 0; i < m_inputList.size(); i++)
			{
				if (m_inputList[i].IsAVariable())
					varList.push_back(m_inputList[i]);
			}
		}
		else
		{
			varList = m_inputList;
		}

		return varList;
	}


	short CModel::GetFileVersion(const string& filePath)
	{
		short version = -1;

		ifStream file;
		if (file.open(filePath))
		{
			string buffer;
			std::getline(file, buffer);
			MakeLower(buffer);

			if (buffer.find("<?xml") != string::npos)
				version = 3;
			else version = 2;

			file.close();
		}

		return version;
	}

	string CModel::GetDocumentation()const
	{
		string doc;

		StringVector title = Tokenize(GetString(IDS_STR_MODEL_DOC_TITLE), "|");

		int c = 0;
		doc += title[c++] + GetTitle() + "\n";
		doc += title[c++] + m_copyright + "\n";
		doc += title[c++] + m_extension + "\n";
		doc += title[c++] + m_description + "\n\n";
		doc += title[c++] + m_variables.to_string() + "\n\n";

		doc += title[c++];
		for (size_t i = 0; i < m_inputList.size(); i++)
		{
			if (m_inputList[i].IsAVariable())
				doc += string("\t") + m_inputList[i].GetName() + ":\t" + m_inputList[i].GetDescription() + "\n";
		}

		doc += "\n";
		doc += title[c++] + string(m_outputTM.GetTypeName()) + "\n\n";

		doc += title[c++];
		for (size_t i = 0; i < m_outputList.size(); i++)
			doc += string("\t") + m_outputList[i].m_title + ":\t" + m_outputList[i].m_description + " [" + m_outputList[i].m_units + "]\n";

		doc += "\n";
		doc += title[c++] + m_credit + "\n\n";

		return doc;
	}

	ERMsg CModel::VerifyWGInput(const CWGInput& WGInput)const
	{
		ERMsg msg;

		if (WGInput.GetNbYears() < GetNbYearMin() ||
			WGInput.GetNbYears() > GetNbYearMax())
		{
			msg.ajoute(FormatMsg(IDS_BSC_NB_YEAR_INVALID, ToString(WGInput.GetNbYears()), GetName(), ToString(GetNbYearMin()), ToString(GetNbYearMax())));
		}


		return msg;
	}

	ERMsg CModel::VerifyInputs(const StringVector& SSIHeader, CWVariables variables)const
	{
		ERMsg msg;

		StringVector SSI(m_SSI, "|");
		for (size_t i = 0; i < m_SSI.size(); i++)
		{
			if (!SSIHeader.Find(SSI[i], false))
			{
				msg.ajoute("Missing SSI: " + SSI[i]);
				//msg.ajoute(FormatMsg(IDS_BSC_MISSING_WEATHER, HOURLY_DATA::GetVariableName(v)));
			}
		}


		for (size_t v = 0; v < m_variables.size(); v++)
		{
			if (m_variables[v] && !variables[v])
			{
				msg.ajoute(FormatMsg(IDS_BSC_MISSING_WEATHER, HOURLY_DATA::GetVariableName(v)));
			}
		}

		return msg;
	}



	//*******************************************************************
	//Simulated Annealing part
	ERMsg CModel::SetSimulatedAnnealingSize(long size, string resultFilePath)
	{
		ERMsg msg;
		if (m_hDll == NULL)
		{
			msg = LoadDLL();
		}

		if (!msg)
			return msg;

		ASSERT(m_hDll);
		m_GetSimulatedAnnealingSize = (GetSimulatedAnnealingSizeF)GetProcAddress(m_hDll, "GetSimulatedAnnealingSize");
		m_SetSimulatedAnnealingSize = (SetSimulatedAnnealingSizeF)GetProcAddress(m_hDll, "SetSimulatedAnnealingSize");
		m_InitSimulatedAnnealing = (InitSimulatedAnnealingF)GetProcAddress(m_hDll, "InitSimulatedAnnealing");
		m_GetFValue = (GetFValueF)GetProcAddress(m_hDll, "GetFValue");



		if (m_GetSimulatedAnnealingSize &&
			m_SetSimulatedAnnealingSize &&
			m_InitSimulatedAnnealing &&
			m_GetFValue)
		{
			m_SetSimulatedAnnealingSize(size, resultFilePath.c_str());
			ASSERT(m_GetSimulatedAnnealingSize() == size);
		}
		else
		{
			msg.ajoute("This dll is not created for SimulatedAnnealing");
		}

		return msg;
	}

	ERMsg CModel::InitSimulatedAnnealing(long ID, const string& IDsFilePath)
	{
		ASSERT(m_SetSimulatedAnnealingSize && m_InitSimulatedAnnealing && m_GetFValue);

		ERMsg msg;

		char error[1024] = { 0 };
		if (!m_InitSimulatedAnnealing(ID, IDsFilePath.c_str(), error))
		{
			msg.ajoute(error);
		}
		//	}
		//	else
		//	{
		//		msg.ajoute("Error loading dll for simulated annealing");
		//	}

		return msg;
	}


	double CModel::GetFValue(const vector<double>& X, CStatisticXYVector& stat)
	{
		ASSERT(m_GetFValue);
		return m_GetFValue((int)X.size(), X.data(), stat);
	}




}