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

	CWeatherUpdate::~CWeatherUbpdate()
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
		//outputVar.push_back(CModelOutputVariableDef("Latitude", "Latitude", "Current position", CTM(CTM::HOURLY)));
		//outputVar.push_back(CModelOutputVariableDef("Longitude", "Longitude", "Current position", CTM(CTM::HOURLY)));
		//outputVar.push_back(CModelOutputVariableDef("Height", "Height", "Current position", CTM(CTM::HOURLY)));
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

			callback.PushTask("Call WeatherUpdater...", NOT_INIT);
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


	ERMsg CWeatherUpdate::GenerateWUProject(const string& WU_file_path, const string& server, const string& file_path)
	{
		ERMsg msg;

		
		string file_name = GetFileName(file_path);


		ofStream file;
		msg = file.open(WU_file_path);
		if (msg)
		{
			//if (bNormals)
			//{
			file << "<?xml version=\"1.0\" encoding=\"Windows - 1252\"?>" << endl;
			file << "<WeatherUpdater version=\"2\">" << endl;
			file << "<Tasks type=\"Tools\">" << endl;
			file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
			file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
			file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
			file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
			file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
			file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
			file << "<Parameters name=\"Local\">tmp\\" << file_name << "</Parameters>" << endl;
			file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
			file << "<Parameters name=\"Password\"/>" << endl;
			file << "<Parameters name=\"Proxy\"/>" << endl;
			file << "<Parameters name=\"Remote\">" << file_path << "</Parameters>" << endl;
			file << "<Parameters name=\"Server\">" << server << "</Parameters>" << endl;
			file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
			file << "<Parameters name=\"UserName\"/>" << endl;
			file << "</Task>" << endl;
			file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
			file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
			file << "<Parameters name=\"Command\">1</Parameters>" << endl;
			file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
			file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
			file << "<Parameters name=\"ZipFilepath\">tmp\\" << file_name << "</Parameters>" << endl;
			file << "</Task>" << endl;
			file << "</Tasks>" << endl;
			file << "</WeatherUpdater>" << endl;

			file.close();
		}
		
		return msg;
}


}