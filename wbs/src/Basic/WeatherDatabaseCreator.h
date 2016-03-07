//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "Basic/WeatherDatabase.h"

namespace WBSF
{

	bool IsHourlyDB(const std::string& name);
	bool IsDailyDB(const std::string& name);
	bool IsNormalsDB(const std::string& name);
	CWeatherDatabasePtr CreateWeatherDatabase(const std::string& filePath1);
	StringVector GetWeatherDBDataFileList(const std::string& filePath, const CWeatherDatabaseOptimization& zop);
}