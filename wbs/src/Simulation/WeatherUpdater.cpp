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
#include "Basic/Registry.h"
#include "Basic/GoogleDrive.h"
#include "FileManager/FileManager.h"
#include "Simulation/WeatherUpdater.h"
#include "Simulation/ExecutableFactory.h"


using namespace std;

namespace WBSF
{
	//*******************************************************************************
	const char* CWeatherUpdate::XML_FLAG = "WeatherUpdater";
	const char* CWeatherUpdate::MEMBERS_NAME[NB_MEMBERS_EX] = { "UpdaterFile", "ShowApp" };
	const int CWeatherUpdate::CLASS_NUMBER = CExecutableFactory::RegisterClass(CWeatherUpdate::GetXMLFlag(), &CWeatherUpdate::CreateObject);

	CWeatherUpdate::CWeatherUpdate()
	{
		ClassReset();
	}

	CWeatherUpdate::CWeatherUpdate(const CWeatherUpdate& in)
	{
		operator=(in);
	}

	CWeatherUpdate::~CWeatherUpdate() 
	{}

	void CWeatherUpdate::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CWeatherUpdate::ClassReset()
	{
		m_name = "WeatherUpdater";
		m_fileTitle.clear();
		m_bShowApp = false;
	}

	CWeatherUpdate& CWeatherUpdate::operator =(const CWeatherUpdate& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_fileTitle = in.m_fileTitle;
			m_bShowApp = in.m_bShowApp;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CWeatherUpdate::operator == (const CWeatherUpdate& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_fileTitle != in.m_fileTitle)bEqual = false;
		if (m_bShowApp != in.m_bShowApp)bEqual = false;

		return bEqual;
	}

	void CWeatherUpdate::GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)
	{
		info.SetLocations(pResult->GetMetadata().GetLocations());
		info.SetNbReplications(pResult->GetMetadata().GetNbReplications());
	}


	ERMsg CWeatherUpdate::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	{
		GetOutputDefinition(outputVar);

		return ERMsg();
	}

	void CWeatherUpdate::GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const
	{
		outputVar.clear();
	}


	void CWeatherUpdate::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(SCRIPT_TITLE)](m_fileTitle);
		out[GetMemberName(SHOW_APP)](m_bShowApp);

	}

	bool CWeatherUpdate::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(SCRIPT_TITLE)](m_fileTitle);
		in[GetMemberName(SHOW_APP)](m_bShowApp);

		return true;
	}


	ERMsg CWeatherUpdate::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		string filePath;
		msg = GetFM().WeatherUpdate().GetFilePath(m_fileTitle, filePath);
		if (msg)
		{
			//try to open log
			string logFilePath(filePath);
			SetFileExtension(logFilePath, ".log");

			callback.PushTask("Call WeatherUpdater: "+GetFileName(filePath), NOT_INIT);
			msg = CallApplication(CRegistry::WEATHER_UPDATER, "\"" + filePath + "\" -e -l \"" + logFilePath + (m_bShowApp ? "\" -Show" : "\""), NULL, m_bShowApp ? SW_SHOW : SW_HIDE, false, true);

			if (msg)
			{
				ifStream log;
				if (log.open(logFilePath))
					callback.AddMessage(log.GetText());
			}

			callback.PopTask();
		}


		return msg;
	}


	ERMsg CWeatherUpdate::GenerateWUProject(const string& WU_file_path, const string& file_id, const string& weather_path)
	{
		ERMsg msg;

		
		string project_path = WBSF::GetPath(WU_file_path);
		string download_path = project_path + "tmp\\";
		//string zip_filepath = download_path + CGoogleDrive::GetFileName(file_id);
	//	string weather_path = project_path + "..\\Weather\\";
		//string file_name = GetFileName(file_path);


		ofStream file;
		msg = file.open(WU_file_path);
		if (msg)
		{
			
			file << "<?xml version=\"1.0\" encoding=\"Windows - 1252\"?>" << endl;
			file << "<WeatherUpdater version=\"2\">" << endl;
			file << "\t<Tasks type=\"Tools\">" << endl;
			file << "\t\t<Task execute=\"true\" name=\"DownloadFile\" type=\"GoogleDrive\">" << endl;
			file << "\t\t\t<Parameters name=\"GoogleDriveLink\">" << file_id << "</Parameters>" << endl;
			file << "\t\t\t<Parameters name=\"DownloadPath\">" << download_path << "</Parameters>" << endl;
			file << "\t\t\t<Parameters name=\"OutputPath\">" << weather_path << "</Parameters>" << endl;
			file << "\t\t\t<Parameters name=\"ShowProgress\">1</Parameters>" << endl;
			file << "\t\t\t<Parameters name=\"UnzipFile\">1</Parameters>" << endl;
			file << "\t\t</Task>" << endl;
			//file << "\t\t<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
			//file << "\t\t\t<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
			//file << "\t\t\t<Parameters name=\"Command\">1</Parameters>" << endl;
			//file << "\t\t\t<Parameters name=\"Directory\">"<< weather_path <<"</Parameters>" << endl;
			//file << "\t\t\t<Parameters name=\"Filter\">*.*</Parameters>" << endl;
			//file << "\t\t\t<Parameters name=\"ZipFilepath\">" << zip_filepath << "</Parameters>" << endl;
			//file << "\t\t\t<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
			//file << "\t\t</Task>" << endl;
			file << "\t</Tasks>" << endl;
			file << "</WeatherUpdater>" << endl;

			file.close();
			
		}
		
		return msg;
}


}