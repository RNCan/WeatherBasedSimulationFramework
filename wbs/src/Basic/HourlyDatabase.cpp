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
#include "stdafx.h"
#include "Basic/HourlyDatabase.h"

namespace WBSF
{

	const int CHourlyDatabase::VERSION = 1;
	const char* CHourlyDatabase::XML_FLAG = "HourlyDatabase";
	const char* CHourlyDatabase::DATABASE_EXT = ".HourlyStations";
	const char* CHourlyDatabase::OPT_EXT = ".Hzop";
	const char* CHourlyDatabase::DATA_EXT = ".csv";
	const CTM CHourlyDatabase::DATA_TM = CTM(CTM::HOURLY);

	ERMsg CHourlyDatabase::CreateDatabase(const std::string& filePath)
	{
		ERMsg msg;

		CHourlyDatabase DB;
		msg = DB.Open(filePath, modeWrite);
		if (msg)
		{
			DB.Close();
			ASSERT(FileExists(filePath));
		}

		return msg;
	}


	//stategy to get static method
	int CHourlyDatabase::GetVersion(const std::string& filePath){ return ((CDHDatabaseBase&)CHourlyDatabase()).GetVersion(filePath); }
	ERMsg CHourlyDatabase::DeleteDatabase(const std::string&  outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CHourlyDatabase()).DeleteDatabase(outputFilePath, callback); }
	ERMsg CHourlyDatabase::AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback){ return ((CDHDatabaseBase&)CHourlyDatabase()).AppendDatabase(inputFilePath1, inputFilePath2, callback); }
	ERMsg CHourlyDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CHourlyDatabase()).RenameDatabase(inputFilePath, outputFilePath, callback); }

}