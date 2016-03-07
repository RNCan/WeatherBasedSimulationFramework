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
//****************************************************************************
#pragma once

#include "stdafx.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/NormalsDatabase.h"

using namespace std;

namespace WBSF
{
	bool IsHourlyDB(const std::string& name)
	{
		return IsEqualNoCase(GetFileExtension(name), CHourlyDatabase::DATABASE_EXT);
	}

	bool IsDailyDB(const std::string& name)
	{
		return IsEqualNoCase(GetFileExtension(name), CDailyDatabase::DATABASE_EXT);
	}

	bool IsNormalsDB(const std::string& name)
	{
		return IsEqualNoCase(GetFileExtension(name), CNormalsDatabase::DATABASE_EXT);
	}

	CWeatherDatabasePtr CreateWeatherDatabase(const std::string& filePath)
	{
		CWeatherDatabasePtr ptr;
		if (IsHourlyDB(filePath))
			ptr.reset((CWeatherDatabase*)new CHourlyDatabase);
		else if (IsDailyDB(filePath))
			ptr.reset((CWeatherDatabase*)new CDailyDatabase);
		else if (IsNormalsDB(filePath))
			ptr.reset((CWeatherDatabase*)new CNormalsDatabase);

		return ptr;
	}

	StringVector GetWeatherDBDataFileList(const std::string& filePath, const CWeatherDatabaseOptimization& zop)
	{
		StringVector filesList;

		if (IsHourlyDB(filePath))
		{
			filesList.resize(zop.size());
			for (size_t i = 0; i < zop.size(); i++)
				filesList[i] = CHourlyDatabase().GetDataFilePath(filePath, zop[i].GetDataFileName());
		}
		else if (IsDailyDB(filePath))
		{
			filesList.resize(zop.size());
			for (size_t i = 0; i < zop.size(); i++)
				filesList[i] = CDailyDatabase().GetDataFilePath(filePath, zop[i].GetDataFileName());
		}
		else if (IsNormalsDB(filePath))
		{
			if (CNormalsDatabase::IsExtendedDatabase(filePath))
			{
				filesList.resize(zop.size());
				for (size_t i = 0; i < zop.size(); i++)
					filesList[i] = CNormalsDatabase().GetDataFilePath(filePath, zop[i].GetDataFileName());
			}
			else
			{
				filesList.push_back(CNormalsDatabase::GetNormalsDataFilePath(filePath));
			}

		}

		return filesList;
	}

}