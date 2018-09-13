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
	const char* CDailyDatabase::DATABASE_EXT = ".DailyDB";
	const char* CDailyDatabase::OPT_EXT = ".Dzop";
	const char* CDailyDatabase::DATA_EXT = ".csv";
	const char* CDailyDatabase::META_EXT = ".DailyHdr.csv";
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
		string headerFilePath = WBSF::GetPath(inputFilePath) + GetFileTitle(inputFilePath) + CDailyDatabase().GetHeaderExtension();
		string outputDataPath = WBSF::GetPath(outputFilePath) + GetFileTitle(outputFilePath) + "\\";


		//create v3 database
		//ifStream fileIn;
		ofStream fileOut;

		CLocationVector locations;
		msg += locations.Load(headerFilePath);
		if (msg)
			msg += CreateMultipleDir(outputDataPath);

		if (msg)
			msg += fileOut.open(outputFilePath);

		if (!msg)
			return msg;




		try
		{
			//convert header file
			zen::XmlDoc doc("DailyDatabase"); //empty XML document
			doc.root().setAttribute("version", "3");
			doc.setEncoding("Windows-1252");


			writeStruc(locations, doc.root());
			string text = zen::serialize(doc, "\n");

			//ReplaceString(text, "version=\"4\"", "version=\"3\"");
			ReplaceString(text, "<Location>", "<DailyStation>");
			ReplaceString(text, "</Location>", "</DailyStation>");
			ReplaceString(text, "<DataFileName>", "<WeaFile>\n\t\t\t<FileTitle>");
			ReplaceString(text, ".csv</DataFileName>", "</FileTitle>\n\t\t</WeaFile>");

			fileOut.write(text.c_str(), text.length());
		}
		catch (const zen::XmlFileError& e)
		{
			// handle error
			msg.ajoute(WBSF::GetErrorDescription(e.lastError));
		}
		catch (const ERMsg& e)
		{
			// handle error
			msg = e;
		}


		//string text = fileIn.GetText();
		//fileIn.close();



		fileOut.close();

		//convert data file
		CFileInfoVector info;
		GetFilesInfo(inputDataPath + "*.csv", false, info);

		callback.PushTask("Conversion", info.size());
		//callback.SetNbStep(info.size());


		//file.WriteString("DailyDatabase 2\n");
		for (size_t i = 0; i < info.size() && msg; i++)
		{
			//ifStream fileIn;
			//ofStream fileOut;

			//msg += fileIn.open(info[i].m_filePath);
			//msg += fileOut.open(outputDataPath + GetFileTitle(info[i].m_filePath) + ".wea");

			//if (!msg)
			//	return msg;


			////convert data file
			//string text = fileIn.GetText();
			//fileIn.close();

			//ReplaceString(text, ",", "\t");
			//fileOut.write(text.c_str(), text.length());
			//fileOut.close();

			CWeatherYears weather;
			msg += weather.LoadData(info[i].m_filePath);

			CWVariables variables = weather.GetVariables();

			//don't save Tair: problem with BioSIM10
			variables.reset(HOURLY_DATA::H_TAIR);
			variables.reset(HOURLY_DATA::H_SNOW);
			variables.reset(HOURLY_DATA::H_SWE);
			variables.reset(HOURLY_DATA::H_SNDH);
			variables.reset(HOURLY_DATA::H_WNDD);
			weather.CleanUnusedVariable(variables);
			weather.SaveData(outputDataPath + GetFileTitle(info[i].m_filePath) + ".wea", CTM(), '\t');
			msg += callback.StepIt();

		}


		callback.AddMessage(FormatMsg(IDS_CMN_NB_STATIONS_ADDED, ToString(info.size())), 1);
		callback.PopTask();

		return msg;
	}


	ERMsg CDailyDatabase::v3_to_v4(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		string inputDataPath = WBSF::GetPath(inputFilePath) + GetFileTitle(inputFilePath) + "\\";
		string headerFilePath = WBSF::GetPath(outputFilePath) + GetFileTitle(outputFilePath) + CDailyDatabase().GetHeaderExtension();
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


		try
		{
			string text = fileIn.GetText();
			fileIn.close();

			
			ReplaceString(text, "<DailyStation>", "<Location>");
			ReplaceString(text, "</DailyStation>", "</Location>");
			ReplaceString(text, "<WeaFile>\n\t\t\t<FileTitle>", "<DataFileName>");
			ReplaceString(text, "</FileTitle>\n\t\t</WeaFile>", ".csv</DataFileName>");

			zen::XmlDoc doc = zen::parse(text);

			CLocationVector locations;
			readStruc(doc.root(), locations);
			
			msg += locations.Save(headerFilePath);
			
			//
			//zen::XmlIn in(doc.root());
			//for (zen::XmlIn child = in["DailyStation"]; child; child.next())
			//{
			//	
			//	auto iterPair = child->getChildren();
			//	for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
			//	{
			//		string name = iter->getNameAs<string>();
			//		string value;
			//		iter->getValue(value);
			//		location.SetSSI(name, value);
			//	}//for all stations ID
			//
			//
			//}

			
			fileOut << "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" << endl << "<DailyDatabase subdir=\"1\" version=\"4\"/>" << endl;
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
		}
		catch (const zen::XmlFileError& e)
		{
			// handle error
			msg.ajoute(WBSF::GetErrorDescription(e.lastError));
		}
		catch (const ERMsg& e)
		{
			// handle error
			msg = e;
		}
		

		return msg;
	}


	int CDailyDatabase::GetVersion(const std::string& filePath){ return ((CDHDatabaseBase&)CDailyDatabase()).GetVersion(filePath); }
	ERMsg CDailyDatabase::DeleteDatabase(const std::string&  outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CDailyDatabase()).DeleteDatabase(outputFilePath, callback); }
	ERMsg CDailyDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback){ return ((CDHDatabaseBase&)CDailyDatabase()).RenameDatabase(inputFilePath, outputFilePath, callback); }


}