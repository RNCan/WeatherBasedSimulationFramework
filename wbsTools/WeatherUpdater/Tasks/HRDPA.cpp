#include "StdAfx.h"
#include "HRDPA.h"
#include "Basic/CallCurl.h"
#include "Basic/FileStamp.h"
#include "Geomatic/gdalbasic.h"
#include "UI/Common/SYShowMessage.h"

#include "../Resource.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{



	//http://climate.weather.gc.ca/radar/image.php?time=04-MAR-14%2005.21.06.293480%20PM&site=WBI
	////HRDPA:
	//http://dd.weather.gc.ca/analysis/precip/hrdpa/grib2/polar_stereographic/24/


	//*********************************************************************
	const char* CHRDPA::SERVER_NAME = "dd.weather.gc.ca";
	const char* CHRDPA::SERVER_PATH = "analysis/precip/%1/grib2/polar_stereographic/";


	CHRDPA::CHRDPA(void)
	{
		m_bByHRDPS = false;
		m_max_hours = 24 * 32;
		m_type = CHRDPA::TYPE_06HOURS;
		m_product = CHRDPA::HRDPA;

	}

	CTRef CHRDPA::GetTRef(string file_path)
	{
		string title = GetFileTitle(file_path);
		title = Right(title, 14);
		int year = WBSF::as<int>(title.substr(0, 4));
		size_t m = WBSF::as<int>(title.substr(4, 2)) - 1;
		size_t d = WBSF::as<int>(title.substr(6, 2)) - 1;
		size_t h = WBSF::as<int>(title.substr(8, 2));
		//size_t hh = WBSF::as<int>(title.substr(10, 3));

		return CTRef(year, m, d, h);
	}


	bool CHRDPA::NeedDownload(const CFileInfo& info, const string& filePathIn)const
	{
		bool bDownload = true;

		//grib is convert into .tif with HRDPS projection		
		string filePath = filePathIn;
		SetFileExtension(filePath, ".tif");

		//is it a valid GeoTIFF
		CGDALDatasetEx DS;
		if (DS.OpenInputImage(filePath))
		{
			DS.Close();
			bDownload = false;
		}
		/*else
		{
			CTRef TRef = GetTRef(filePath);
			CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
			if (now - TRef > m_max_hours)
				bDownload = false;
		}*/


		return bDownload;
	}

	string CHRDPA::GetOutputFilePath(const string& fileName)const
	{
		string file_path;
		if (m_bByHRDPS)
		{
			ASSERT(TYPE_06HOURS == m_type);
			ASSERT(HRDPA == m_product);
			CTRef TRef = GetTRef(fileName) - 6;//remove 6 hours because the HRDPA is at the end of the period of 6 hours
			file_path = FormatA("%s%d\\%02d\\%02d\\%02d\\%s", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), fileName.c_str());
		}
		else
		{
			string year = Right(fileName, 20).substr(0, 4);
			string type = m_type == TYPE_06HOURS ? "06" : "24";

			file_path = m_workingDir + type + "\\" + year + "\\" + fileName;
		}

		return file_path;
	}



	ERMsg CHRDPA::Execute(CCallback& callback, StringVector& HRDPAFiles)
	{
		ERMsg msg;

		//CInternetSessionPtr pSession;
		//CHttpConnectionPtr pConnection;

		//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		//if (!msg)
			//return msg;

		callback.AddMessage("Updating HRDPA/RDPA");
		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		string type = m_type == TYPE_06HOURS ? "06" : "24";
		string product = m_product == RDPA ? "RDPA" : "HRDPA";

		string URL = string("https://") + SERVER_NAME + "/" + SERVER_PATH + type + "/*.grib2";
		ReplaceString(URL, "%1", product);
		WBSF::MakeLower(URL);

		CFileInfoVector fileListTmp;

		//msg = UtilWWW::FindFiles(pConnection, URL, fileListTmp);
		msg = UtilWWW::FindFilesCurl(URL, fileListTmp);

		//pConnection->Close();
		//pSession->Close();

		CFileInfoVector fileList;
		for (CFileInfoVector::const_iterator it = fileListTmp.begin(); it != fileListTmp.end(); it++)
		{
			if (m_product == HRDPA)
			{
				if (m_type == TYPE_24HOURS)
				{
					CTRef TRef = CHRDPA::GetTRef(it->m_filePath);
					if (TRef.GetHour() == 12)//download prcp at noon only
						fileList.push_back(*it);
				}
				else if (m_type == TYPE_06HOURS)
				{
					string cutoff = GetFileTitle(it->m_filePath).substr(19, 4);
					if (cutoff == "0700")//download prcp at cutoff 7 only
						fileList.push_back(*it);
				}
			}
		}

		callback.AddMessage("Number of images found: " + ToString(fileList.size()));
		//keep only 10km grid
		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end();)
		{
			string fileName = GetFileName(it->m_filePath);
			string filePath = GetOutputFilePath(fileName);

			if (!NeedDownload(*it, filePath))
				it = fileList.erase(it);
			else
				it++;

			msg += callback.StepIt(0);
		}

		//CCallcURL cURL;


		callback.AddMessage("Number of images to download after clearing: " + ToString(fileList.size()));
		callback.PushTask("Download " + product + " precipitation images (" + ToString(fileList.size()) + ")", fileList.size());

		size_t posI = 0;
		size_t nbDownload = 0;
		///size_t nbTry = 0;
		for (size_t i = posI; i < fileList.size() && msg; i++)
		{
			//nbTry++;

			//CInternetSessionPtr pSession;
			//CHttpConnectionPtr pConnection;
			//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			//if (msg)
			{
				//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
				//pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 15000);

				//try
				//{
				string filePath = GetOutputFilePath(GetFileName(fileList[i].m_filePath));
				CreateMultipleDir(GetPath(filePath));

				
				msg = CopyFileCurl(fileList[i].m_filePath, filePath);
				if (msg && GoodGrib(filePath))
				{
					string filePathOut = filePath;
					SetFileExtension(filePathOut, ".tif");

					nbDownload++;
					posI++;
					//convert grib into tif and use the same projection as HRDPS
					string argument = "-ot Float32 -dstnodata 9999 -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";

					//remove +k=90 : no longer supported by PROJ4, RSA 02-08-2022
					string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
					string projlib_path = GetApplicationPath() + "External\\projlib";
					string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\"";
					argument += " -t_srs \"+proj=stere +lat_0=90 +lat_ts=60 +lon_0=252 +x_0=0 +y_0=0 +a=6371229 +b=6371229 +units=m +no_defs\"";
					string command = "\"" + GetApplicationPath() + "External\\GDALWarp.exe\" " + option + " " + argument + " \"" + filePath + "\" \"" + filePathOut + "\"";;
					msg += WinExecWait(command);
					if (msg)
					{
						ASSERT(FileExists(filePathOut));
						msg += RemoveFile(filePath);
						HRDPAFiles.push_back(filePathOut);
					}

				}
				else
				{
					callback.AddMessage("corrupt file, remove: " + filePath);
					msg = WBSF::RemoveFile(filePath);
				}

				msg += callback.StepIt();

				/*}
				catch (CException* e)
				{
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg += WaitServer(30, callback);
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}

				pConnection->Close();
				pSession->Close();*/
			}
		}



		callback.AddMessage("Number of images downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;
	}


}