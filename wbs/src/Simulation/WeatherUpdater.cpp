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
	const char* CWeatherUpdate::MEMBERS_NAME[NB_MEMBERS_EX] = { "Parameters" };
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
	}

	CWeatherUpdate& CWeatherUpdate::operator =(const CWeatherUpdate& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_fileTitle = in.m_fileTitle;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CWeatherUpdate::operator == (const CWeatherUpdate& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_fileTitle != in.m_fileTitle)bEqual = false;

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
	}

	bool CWeatherUpdate::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(SCRIPT_TITLE)](m_fileTitle);

		return true;
	}


	ERMsg CWeatherUpdate::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		string filePath;
		msg = GetFM().WeatherUpdate().GetFilePath(m_fileTitle, filePath);
		if (msg)
		{
			msg = CallApplication(CRegistry::WEATHER_UPDATER, "\"" + filePath + "\" /EXEC", NULL, SW_SHOW, false, true);

			//copy log into biosim component log
		}

		return msg;
	}
}