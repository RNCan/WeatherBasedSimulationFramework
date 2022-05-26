//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <Shlwapi.h>

#include "Basic/Rijndael.h"
#include "FileManager/FileManager.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/CopyExport.h"

#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{
	static const int NB_DAY_MIN_MONTHLY = 25;
	static const int NB_DAY_MIN_ANNUAL = 340;
	//**********************************************************************
	//CCopyExport


	const char* CCopyExport::XML_FLAG = "CopyExport";
	const char* CCopyExport::MEMBER_NAME[NB_MEMBER_EX] = { "FilePathOut", "UserName", "Password" };
	const int CCopyExport::CLASS_NUMBER = CExecutableFactory::RegisterClass(CCopyExport::GetXMLFlag(), &CCopyExport::CreateObject);

	CCopyExport::CCopyExport()
	{
		Reset();
	}

	CCopyExport::~CCopyExport()
	{}

	void CCopyExport::Reset()
	{
		CExecutable::Reset();

		m_outputFilePath.clear();
		m_userName.clear();
		m_password.clear();
		m_bShowFTPTransfer = true;
		m_name = "CopyExport";
	}

	CCopyExport::CCopyExport(const CCopyExport& in)
	{
		operator=(in);
	}


	CCopyExport& CCopyExport::operator =(const CCopyExport& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_outputFilePath = in.m_outputFilePath;
			m_userName = in.m_userName;
			m_password = in.m_password;
			m_bShowFTPTransfer = in.m_bShowFTPTransfer;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CCopyExport::operator == (const CCopyExport& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_outputFilePath != in.m_outputFilePath)bEqual = false;
		if (m_userName != in.m_userName)bEqual = false;
		if (m_password != in.m_password)bEqual = false;
		if (m_bShowFTPTransfer != in.m_bShowFTPTransfer)bEqual = false;

		return bEqual;
	}
	/*
	string CCopyExport::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	//One block testing


	string str;
	switch(i)
	{
	case OUTPUT_FILE_PATH: str = m_outputFilePath; break;
	case USER_NAME: str = m_userName; break;
	case PASSWORD:
	{
	int l = (int)ceil(m_password.GetLength()/16.0)*16;
	CRijndael oRijndael;
	oRijndael.MakeKey("Mingo_La_Fleche5", "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16, 16);
	oRijndael.EncryptBlock((LPCTSTR)m_password, str.GetBufferSetLength(l));
	str.ReleaseBuffer();
	break;
	}
	default: str = CExecutable::GetMember(i, pNode);
	}

	return str;
	}

	void CCopyExport::SetMember(int i, const string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case OUTPUT_FILE_PATH:  m_outputFilePath = str; break;
	case USER_NAME: m_userName = str; break;
	case PASSWORD:
	{
	int l = (int)ceil(str.GetLength()/16.0)*16;
	CRijndael oRijndael;
	oRijndael.MakeKey("Mingo_La_Fleche5", "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16, 16);
	oRijndael.DecryptBlock((LPCTSTR)str, m_password.GetBufferSetLength(l));
	m_password.ReleaseBuffer();
	break;
	}
	default: CExecutable::SetMember(i, str, pNode);
	}

	}
	*/


	//CDimension CCopyExport::GetOutputDimension(const CResultPtr& result)const
	//{
	//	CDimension dimension = result->GetDimension();
	//
	//	//CDimension d;
	//	//d[LOCATION] = m_computation.m_bMeanOverLocation?1:m_window.m_bSelectLocation?m_window.m_locations.GetSize():dimension[LOCATION];
	//	//d[PARAMETER] = m_computation.m_bMeanOverParameterSet?1:dimension[PARAMETER];
	//	//d[REPLICATION] = m_computation.m_bMeanOverReplication?1:dimension[REPLICATION];
	//
	//	//CTTransformation t(result->GetTPeriod(), m_computation.m_TM);
	//	//d[TIME_REF] = m_computation.m_bSelectComputation?t.GetNbCluster():dimension[TIME_REF];
	//	//d[VARIABLE] = m_window.m_bSelectVariable?m_window.m_variables.GetSize():dimension[VARIABLE];
	//	
	//
	//	return result->GetDimension();;
	//}

	ERMsg CCopyExport::GetLocationList(const CFileManager& fileManager, CLocationVector& loc)const
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CCopyExport::GetParameterList(const CFileManager& fileManager, CModelInputVector& parameters)const
	{
		ERMsg msg;
		return msg;
	}

	//int CCopyExport::GetReplication()const
	ERMsg CCopyExport::GetReplication(const CFileManager& fileManager, size_t& nbReplication)const
	{
		assert(m_pParent);

		nbReplication = 1;

		return ERMsg();
	}

	ERMsg CCopyExport::GetDefaultPeriod(const CFileManager& fileManager, CTPeriod& period)const
	{
		assert(m_pParent);

		ERMsg msg;

		CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
		period = CTPeriod(CTRef(0, 0, 0, 0, TM), CTRef(0, 0, 0, 0, TM));

		return msg;
	}

	ERMsg CCopyExport::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	{
		assert(m_pParent);

		ERMsg msg;
		return msg;
	}


	bool CCopyExport::IsLocal()const
	{
		wstring filePath = UTF16(m_outputFilePath);
		int drvNbr = PathGetDriveNumberW(filePath.c_str());

		//	string drive = UtilWin::GetDrive( m_outputFilePath.c_str() );

		return drvNbr >= 0;
	}

	string CCopyExport::GetServerName(string serverName)
	{
		MakeLower(serverName);
		std::replace(serverName.begin(), serverName.end(), '\\', '/');
		ReplaceString(serverName, "ftp://", "");


		string::size_type pos = serverName.find_first_of("\\/");
		if (pos != string::npos)
			serverName = serverName.substr(0, pos);


		return serverName;
	}

	string CCopyExport::GetServerPath(string serverPath)
	{

		MakeLower(serverPath);
		std::replace(serverPath.begin(), serverPath.end(), '\\', '/');
		ReplaceString(serverPath, "ftp://", "");


		string::size_type pos = serverPath.find_first_of("\\/");
		if (pos != string::npos)
		{
			serverPath = serverPath.substr(pos);

			if (!IsPathEndOk(serverPath))
				serverPath += '/';
		}
		else
		{
			serverPath.clear();
		}

		return serverPath;
	}

	ERMsg CCopyExport::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ASSERT(m_pParent);

		ERMsg msg;
		

		string filePathIn = m_pParent->GetExportFilePath(fileManager, CExecutable::EXPORT_CSV);

		if (IsLocal())
		{
			wstring filePath1 = UTF16(filePathIn);
			wstring filePath2 = UTF16(m_outputFilePath);

			::CopyFileW(filePath1.c_str(), filePath2.c_str(), false);
		}
		else
		{
			//FTP copy


			string server = GetServerName(WBSF::GetPath(m_outputFilePath));
			string path = GetServerPath(WBSF::GetPath(m_outputFilePath));
			

			string workingDir = WBSF::GetPath(filePathIn);
			string scriptFilePath = workingDir + GetFileTitle(filePathIn) + "_script.txt";
			WBSF::RemoveFile(scriptFilePath + ".log");


			ofStream stript;
			msg = stript.open(scriptFilePath);
			if (msg)
			{
				stript << "open ftp:"+ m_userName +":"+ m_password +"@" << server << endl;

				stript << "cd \"" << WBSF::GetPath(path) << "\"" << endl;
				stript << "lcd \"" << WBSF::GetPath(filePathIn) << "\"" << endl;
				stript << "put" << " \"" << GetFileName(filePathIn) << "\"" << endl;
				stript << "exit" << endl;
				stript.close();

				UINT show = /*APP_VISIBLE && */m_bShowFTPTransfer ? SW_SHOW : SW_HIDE;
				bool bShow = m_bShowFTPTransfer;
				string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + " /passive=on" + " /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath + "\"";

				DWORD exitCode = 0;
				msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
				if (msg && exitCode != 0)
				{
					msg.ajoute("WinSCP as exit with error code " + ToString((int)exitCode));
					msg.ajoute("See log file: " + scriptFilePath + ".log");
				}

			}

			
			//string command = GetApplicationPath() + "FTPTransfer.exe -Server \"" + server + "\" -Remote \"" + path + "\" -Local \"" + filePathIn + "\" -Passive -Upload";


			//DWORD exitCode = 0;
			//msg = WinExecWait(command.c_str(), GetTempPath().c_str(), m_bShowFTPTransfer);
			//if (msg && exitCode != 0)
//				msg.ajoute("FTPTransfer as exit with error code " + ToString(int(exitCode)));
		}


		if (msg)
		{
			callback.AddMessage(FormatMsg(IDS_BSC_COPY_FILE, filePathIn, m_outputFilePath), 1);
		}

		return msg;
	}


	void CCopyExport::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);

		out[GetMemberName(OUTPUT_FILE_PATH)](m_outputFilePath);
		out[GetMemberName(USER_NAME)](m_userName);
		out[GetMemberName(PASSWORD)](m_password);
	}

	bool CCopyExport::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);

		in[GetMemberName(OUTPUT_FILE_PATH)](m_outputFilePath);
		in[GetMemberName(USER_NAME)](m_userName);
		in[GetMemberName(PASSWORD)](m_password);

		return true;
	}



	//ERMsg CCopyExport::Execute(const CFileManager& fileManager, CCallback& callback)
	//{
	//	ERMsg msg;
	//
	//
	//	//CCSVFile fileIn;
	//	//msg +=fileIn.Load("D:\\Travail\\WorldClim-BioSIM\\Output\\worldclim5.csv");
	//
	//	//CCSVFile fileOut;
	//	//msg +=fileOut.Open("D:\\Travail\\WorldClim-BioSIM\\Output\\worldClim6.csv", CFile::modeCreate|CFile::modeWrite);
	//	//fileOut.WriteString("Name,ID,Latitude,Longitude,Elevation,AltWC,Month,TminWC (°C),TmaxWC (°C),PrecipitationWC (mm)\n");
	//	////fileOut.WriteString("Name,ID,Month,TminWC (°C),TmaxWC (°C),PrecipitationWC (mm)\n");
	//	//if(msg)
	//	//{
	//	//	enum{NAME,ID,LATITUDE,LONGITUDE,ELEVATION,RECNO,POINTNO,LON_EXT,LAT_EXT,ALT,TMIN1,TMAX1,PREC1};
	//	//	//enum{NAME,ID,RECNO,POINTNO,LON_EXT,LAT_EXT,TMIN1,TMAX1,PREC1};
	//
	//	//	for(size_t i=0; i<fileIn.Data().size(); i++)
	//	//	{
	//	//		
	//	//		for(int m=0; m<12; m++)
	//	//		{
	//	//			string line = fileIn[i][NAME]+","+fileIn[i][ID]+","+fileIn[i][LATITUDE]+","+fileIn[i][LONGITUDE]+","+fileIn[i][ELEVATION]+","+fileIn[i][ALT]+",";
	//	//			//string line = fileIn[i][NAME]+","+fileIn[i][ID]+",";
	//	//			line += ToString(m+1) + ",";
	//	//			line += fileIn[i][TMIN1+m*3]+","+fileIn[i][TMAX1+m*3]+","+fileIn[i][PREC1+m*3];
	//	//			fileOut.WriteString(line+"\n");
	//	//		}
	//	//		
	//	//		
	//	//	}
	//
	//	//	fileOut.Close();
	//	//}
	//
	//	//return msg;
	//
	//	switch(m_kind)
	//	{
	//	case LAST_DATE: msg = LastDailyDay(fileManager, callback); break;
	//	case MATCH_STATION_NORMAL: 
	//	case MATCH_STATION_DAILY: msg = MatchStation(fileManager, callback); break;
	//	case XVALIDATION_NORMAL: 
	//	case XVALIDATION_DAILY: msg = XValidation(fileManager, callback); break;
	//	case EXTRACT_NORMAL: msg = ExtractNormal(fileManager, callback); break;
	//	default: ASSERT(false); 
	//	}
	//	return msg;
	//}
	//*******************************************************************************

}