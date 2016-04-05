//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "Basic/DailyDatabase.h"
#include "Basic/UtilStd.h"

#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{

	const int CDailyDatabase::VERSION = 4;
	const char* CDailyDatabase::XML_FLAG = "DailyDatabase";
	const char* CDailyDatabase::DATABASE_EXT = ".DailyStations";
	const char* CDailyDatabase::OPT_EXT = ".Dzop";
	const char* CDailyDatabase::DATA_EXT = ".csv";
	const CTM CDailyDatabase::DATA_TM = CTM(CTM::DAILY);

	ERMsg CDailyDatabase::CreateDatabase(const std::string& filePath)
	{
		ERMsg msg;

		CDailyDatabase DB;
		msg = DB.Open(filePath, modeWrite);
		if (msg)
		{
			DB.Close();
			ASSERT(FileExists(filePath));
		}



		return msg;
	}

	ERMsg CDailyDatabase::v4_to_v3(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		string inputDataPath = CDailyDatabase().GetDataPath(inputFilePath);
		string outputDataPath = CDailyDatabase().GetDataPath(outputFilePath);

		//create v3 database
		ifStream fileIn;
		ofStream fileOut;

		msg += fileIn.open(inputFilePath);
		if (msg)
			msg += CreateMultipleDir(outputDataPath);

		if (msg)
			msg += fileOut.open(outputFilePath);

		if (!msg)
			return msg;



		//convert header file
		string text = fileIn.GetText();
		fileIn.close();

		ReplaceString(text, "version=\"4\"", "version=\"3\"");
		ReplaceString(text, "<Location>", "<DailyStation>");
		ReplaceString(text, "</Location>", "</DailyStation>");
		ReplaceString(text, "<DataFileName>", "<WeaFile>\n\t\t\t<FileTitle>");
		ReplaceString(text, ".csv</DataFileName>", "</FileTitle>\n\t\t</WeaFile>");

		fileOut.write(text.c_str(), text.length());
		fileOut.close();

		//convert data file
		CFileInfoVector info;
		GetFilesInfo(inputDataPath + "*.csv", false, info);

		callback.PushTask("Conversion", info.size());
		//callback.SetNbStep(info.size());


		//file.WriteString("DailyDatabase 2\n");
		for (int i = 0; i < info.size() && msg; i++)
		{
			ifStream fileIn;
			ofStream fileOut;

			msg += fileIn.open(info[i].m_filePath);
			msg += fileOut.open(outputDataPath + GetFileTitle(info[i].m_filePath) + ".wea");

			if (!msg)
				return msg;


			//convert data file
			string text = fileIn.GetText();
			fileIn.close();

			ReplaceString(text, ",", "\t");
			fileOut.write(text.c_str(), text.length());
			fileOut.close();

			msg += callback.StepIt();

		}


		callback.AddMessage(FormatMsg(IDS_CMN_NB_STATIONS_ADDED, ToString(info.size())), 1);
		callback.PopTask();

		return msg;
	}


	ERMsg CDailyDatabase::v3_to_v4(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		string inputDataPath = CDailyDatabase().GetDataPath(inputFilePath);
		string outputDataPath = CDailyDatabase().GetDataPath(outputFilePath);

		//create v4 database
		ifStream fileIn;
		ofStream fileOut;

		msg += fileIn.open(inputFilePath);
		if (msg)
			msg += CreateMultipleDir(outputDataPath);

		if (msg)
			msg += fileOut.open(outputFilePath);

		if (!msg)
			return msg;



		//convert header file
		string text = fileIn.GetText();
		fileIn.close();

		ReplaceString(text, "version=\"3\"", "version=\"4\"");
		ReplaceString(text, "<DailyStation>", "<Location>");
		ReplaceString(text, "</DailyStation>", "</Location>");
		ReplaceString(text, "<WeaFile>\n\t\t\t<FileTitle>", "<DataFileName>");
		ReplaceString(text, "</FileTitle>\n\t\t</WeaFile>", ".csv</DataFileName>");

		fileOut.write(text.c_str(), text.length());
		fileOut.close();

		//convert data file
		CFileInfoVector info;
		GetFilesInfo(inputDataPath + "*.wea", false, info);

		callback.PushTask("Conversion", info.size());
		//callback.SetNbStep(info.size());


		//file.WriteString("DailyDatabase 2\n");
		for (int i = 0; i < info.size() && msg; i++)
		{
			ifStream fileIn;
			ofStream fileOut;

			msg += fileIn.open(info[i].m_filePath);
			msg += fileOut.open(outputDataPath + GetFileTitle(info[i].m_filePath) + ".csv");

			if (!msg)
				return msg;


			//convert data file
			string text = fileIn.GetText();
			fileIn.close();
			ReplaceString(text, "\t ", ",");
			fileOut.write(text.c_str(), text.length());
			fileOut.close();

			msg += callback.StepIt();

		}


		callback.AddMessage(FormatMsg(IDS_CMN_NB_STATIONS_ADDED, ToString(info.size())), 1);
		callback.PopTask();

		return msg;
	}


	int CDailyDatabase::GetVersion(const std::string& filePath){ return ((CDHDatabaseBase&)CDailyDatabase()).GetVersion(filePath); }
	ERMsg CDailyDatabase::DeleteDatabase(const std::string&  outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CDailyDatabase()).DeleteDatabase(outputFilePath, callback); }
	//ERMsg CDailyDatabase::AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback){ return ((CDHDatabaseBase&)CDailyDatabase()).AppendDatabase(inputFilePath1, inputFilePath2, callback); }
	ERMsg CDailyDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CDailyDatabase()).RenameDatabase(inputFilePath, outputFilePath, callback); }


}