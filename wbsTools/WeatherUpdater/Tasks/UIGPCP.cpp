//Integrated Surface Data - “Lite”
#include "stdafx.h"
#include "UIGPCP.h"

#include <boost\filesystem.hpp>
#include "Basic/FileStamp.h"
#include "Basic/DailyDatabase.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/GDALBasic.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../Resource.h"

using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_srs_api.h"



namespace WBSF
{


	static 	float ReverseFloat(const float inFloat)
	{
		float retVal;
		char *floatToConvert = (char*)& inFloat;
		char *returnFloat = (char*)& retVal;

		// swap the bytes into a temporary buffer
		returnFloat[0] = floatToConvert[3];
		returnFloat[1] = floatToConvert[2];
		returnFloat[2] = floatToConvert[1];
		returnFloat[3] = floatToConvert[0];

		return retVal;
	}



//https://climatedataguide.ucar.edu/climate-data/gpcp-daily-global-precipitation-climatology-project
//ftp ://


	const char* CUIGPCP::SERVER_NAME = "meso.gsfc.nasa.gov";
	const char* CUIGPCP::SERVER_PATH = "/pub/1dd-v1.2/";
	

	//*********************************************************************
	const char* CUIGPCP::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DataType", "FirstYear", "LastYear", "BoundingBox" };
	const size_t CUIGPCP::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_STRING, T_GEORECT };
	const UINT CUIGPCP::ATTRIBUTE_TITLE_ID = IDS_UPDATER_GPCP_P;
	const UINT CUIGPCP::DESCRIPTION_TITLE_ID = ID_TASK_GPCP;

	const char* CUIGPCP::CLASS_NAME(){ static const char* THE_CLASS_NAME = "GPCP";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGPCP::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGPCP::CLASS_NAME(), (createF)CUIGPCP::create);
	

	static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);

	CUIGPCP::CUIGPCP(void)
	{}

	CUIGPCP::~CUIGPCP(void)
	{
	}


	std::string CUIGPCP::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DATA_TYPE: str = "Daily|Monthly"; break;
		};
		return str;
	}

	std::string CUIGPCP::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "GPCP\\"; break;
		case DATA_TYPE: str = "0"; break;
		case FIRST_YEAR:  str = "1996"; break;
		case LAST_YEAR:	str = "2015"; break;
		case BOUNDING_BOX: str = ToString(DEFAULT_BOUDINGBOX); break;
		};

		return str;
	}

	//*****************************************************************************



	ERMsg CUIGPCP::Execute(CCallback& callback)
	{
		ERMsg msg;



		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);
		//string shapeFilePath = TrimConst(Get(SHAPEFILE));
		//if (msg && !shapeFilePath.empty())
		//	msg += shapefile.Read(GetAbsoluteFilePath(shapeFilePath));



		//callback.AddMessage(GetString(IDS_UPDATE_DIR));
		//callback.AddMessage(workingDir, 1);
		//callback.AddMessage(GetString(IDS_UPDATE_FROM));
		//callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		//callback.AddMessage("");

		////load station list
		//CFileInfoVector fileList;
		//msg = UpdateStationHistory();

		//if (msg)
		//	msg = UpdateOptimisationStationFile(GetDir(WORKING_DIR), callback);

		//if (msg)
		//	msg = LoadOptimisation();

		//if (msg)
		//	msg = GetFileList(fileList, callback);

		//if (!msg)
		//	return msg;

		//callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());
		////callback.SetNbStep(fileList.size());

		//int nbRun = 0;
		//int curI = 0;

		//while (curI < fileList.size() && nbRun < 20 && msg)
		//{
		//	nbRun++;

		//	CInternetSessionPtr pSession;
		//	CFtpConnectionPtr pConnection;

		//	ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@hotmail.com", true);
		//	if (msgTmp)
		//	{
		//		
		//		TRY
		//		{
		//			for (int i = curI; i < fileList.size() && msgTmp && msg; i++)
		//			{
		//				msg += callback.StepIt(0);

		//				string fileTitle = GetFileTitle(fileList[i].m_filePath);

		//				string stationID = fileTitle.substr(0, 12);
		//				int year = ToInt(Right(fileTitle, 4));

		//				string zipFilePath = GetOutputFilePath(stationID, year, ".gz");
		//				string extractedFilePath = GetOutputFilePath(stationID, year, "");
		//				string outputFilePath = GetOutputFilePath(stationID, year);
		//				string outputPath = GetPath(outputFilePath);
		//				CreateMultipleDir(outputPath);
		//				msgTmp += CopyFile(pConnection, fileList[i].m_filePath, zipFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

		//				//unzip 
		//				if (msgTmp)
		//				{
		//					

		//					
		//					string command = "External\\7za.exe e \"" + zipFilePath + "\" -y -o\"" + outputPath + "\"";
		//					msg += WinExecWait(command.c_str());
		//					RemoveFile(zipFilePath);

		//					//remove old file
		//					RemoveFile(outputFilePath);
		//					//by default, file don't have extension,
		//					RenameFile(extractedFilePath, outputFilePath);
		//					
		//					//update time to the time of the .gz file
		//					boost::filesystem::path p(outputFilePath);
		//					if (boost::filesystem::exists(p))
		//						boost::filesystem::last_write_time(p, fileList[i].m_time);


		//					ASSERT(FileExists(outputFilePath));
		//					nbRun = 0;
		//					curI++;

		//					msg += callback.StepIt();
		//				}
		//			}
		//		}
		//		CATCH_ALL(e)
		//		{
		//			msgTmp = UtilWin::SYGetMessage(*e);
		//		}
		//		END_CATCH_ALL

		//		//clean connection
		//		pConnection->Close();
		//		pSession->Close();

		//		if (!msgTmp)
		//		{
		//			callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(curI), ToString(fileList.size())));
		//		}
		//	}
		//	else
		//	{
		//		if (nbRun > 1 && nbRun < 20)
		//		{
		//			callback.PushTask("Waiting 30 seconds for server...", 600);
		//			for (size_t i = 0; i < 600 && msg; i++)
		//			{
		//				Sleep(50);//wait 50 milisec
		//				msg += callback.StepIt();
		//			}
		//			callback.PopTask();
		//		}
		//	}
		//}

		//callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
		//callback.PopTask();
		msg.ajoute("not done yet");

		return msg;
	}

	ERMsg CUIGPCP::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		if (type == DATA_DAILY)
			msg = GetStationListD(stationList, callback);
		else if (type == DATA_MONTHLY)
			msg = GetStationListM(stationList, callback);

		return msg;
	}
	
	ERMsg CUIGPCP::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		if (type == DATA_DAILY)
			msg = GetWeatherStationD(ID, TM, station, callback);
		else if (type == DATA_MONTHLY)
			msg = GetWeatherStationM(ID, TM, station, callback);

		return msg;
	}

	ERMsg CUIGPCP::GetStationListD(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		//get all file in the directory 
		StringVector fileList;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		string workingDir = GetDir(WORKING_DIR);
		CGeoRect rect = as<CGeoRect>(BOUNDING_BOX);
		rect.SetPrjID(PRJ_WGS_84);

		string strSearch = workingDir + "Daily\\gpcp_1dd_v1.2_p1d.*";
		StringVector fileListTmp = GetFilesList(strSearch);
		sort(fileListTmp.begin(), fileListTmp.end());

		
		for (size_t i = 0; i < fileListTmp.size() && msg; i++)
		{
			string ext = fileListTmp[i].substr(fileListTmp[i].size() - 6);
			ASSERT(ext.length() == 6);
			int year = ToInt(ext.substr(0, 4));
			size_t m = ToSizeT(ext.substr(4, 2)) - 1;
			if (year >= firstYear && year <= lastYear)
			{
				fileList.push_back(fileListTmp[i]);
				if (!m_firstTRef.IsInit())
					m_firstTRef = CTRef(year, m);

				ASSERT((m_firstTRef + fileList.size() - 1).GetYear() == year && (m_firstTRef + fileList.size() - 1).GetMonth() == m);
			}
		}

		if (msg && !fileList.empty())
		{
			_setmaxstdio(1024);
			m_files.resize(fileList.size());

			//remove file 
			for (size_t i = 0; i < fileList.size() && msg; i++)
				msg += m_files[i].open(fileList[i], ios::in | ios::binary);
			
			stationList.reserve(180 * 360);
			for (size_t i = 0; i < 180; i++)
			{
				for (size_t j = 0; j < 360; j++)
				{
					double lat = 89.5 - i;
					double lon = 0.5 + j;
					if (lon > 180)
						lon -= 360;

					CGeoPoint pt(lon, lat, PRJ_WGS_84);

					if (rect.IsEmpty() || rect.IsInside(pt))
					{
						string title = FormatA("%03d_%03d", i + 1, j + 1);
						stationList.push_back(title);
					}

					msg += callback.StepIt(0);
				}
			}
		}

	//CGeoExtents extents;
	//extents.m_xMin = -180;
	//extents.m_xMax = 180;
	//extents.m_yMin = -90;
	//extents.m_yMax = 90;
	//extents.m_xSize = 360;
	//extents.m_ySize = 180;
	//extents.m_xBlockSize = 360;
	//extents.m_yBlockSize = 1;

	//CBaseOptions options;
	//options.m_prj = PRJ_WGS_84_WKT;
	//options.m_extents = extents;
	//options.m_nbBands = 1;
	//options.m_dstNodata = -999;
	//options.m_outputType = GDT_Float32;
	//options.m_format = "GTIFF";
	//options.m_bOverwrite = true;
	//options.m_bComputeStats = true;
	//options.m_createOptions.push_back(string("COMPRESS=LZW"));
	//options.m_overviewLevels = { { 2, 4, 8, 16 } };




	//ios::pos_type length = m_files.front().length();

	//
	//CGDALDatasetEx grid;
	//msg += grid.CreateImage("c:/tmp/GPCP_D.tiff", options);
	//ASSERT(msg);
	//GDALRasterBand* pBand = grid.GetRasterBand(0);

	//for (size_t i = 0; i < 180; i++)
	//{
	//	array<float, 360> data;
	//	data.fill(0);
	//	for (size_t j = 0; j < 360; j++)
	//	{
	//		for (size_t d = 0; d < m_firstTRef.GetNbDayPerMonth(); d++)
	//		{
	//			m_files.front().seekg(1440 + (d * 180 * 360 + (i * 360 + j)) * 4);

	//			ios::pos_type pos = m_files.front().tellg();
	//			ASSERT(pos < length);

	//			float prcp = 0;
	//			m_files.front().read((char*)(&prcp), sizeof(prcp));
	//			prcp = ReverseFloat(prcp);

	//			if (isnan(prcp) || !isfinite(prcp))
	//				prcp = -999;

	//			if (prcp > 0 && prcp < 0.00001f)
	//				prcp = 0.00001f;

	//			if (prcp < 0)
	//				prcp = -999;

	//			ASSERT(prcp == -999 || (prcp >= 0 && prcp < 316));

	//			if (prcp>0)
	//				data[(j + 180) % 360] += prcp;

	//			msg += callback.StepIt(0);
	//		}
	//	}

	//	pBand->RasterIO(GF_Write, 0, int(i), extents.m_xSize, 1, &(data[0]), extents.m_xSize, 1, GDT_Float32, 0, 0);
	//}




	//grid.Close();


		return msg;
	}




	ERMsg CUIGPCP::GetWeatherStationD(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ASSERT(ID.length()==7);
		ERMsg msg;

		size_t i = WBSF::as<size_t>(ID.substr(0,3))-1;
		size_t j = WBSF::as<size_t>(ID.substr(4, 3))-1;
		double lat = 89.5 - i;
		double lon = 0.5 + j;
		if (lon > 180)
			lon -= 360;

		((CLocation&)station) = CLocation(ID,ID,lat,lon,0);


		int firstYear = m_firstTRef.GetYear();
		int lastYear = (m_firstTRef + m_files.size()).GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);
		
		for (size_t mm = 0; mm < m_files.size() && msg; mm++)
		{
			CTRef TRef = m_firstTRef + mm;
			TRef.Transform(CTM(CTM::DAILY));
			size_t nbDays = TRef.GetNbDayPerMonth();
			for (size_t d = 0; d < nbDays&& msg; d++, TRef++)
			{
				m_files[mm].seekg(1440 + (d*180*360+(i*360+j))*4);
				
				float prcp = 0;
				m_files[mm].read((char*)(&prcp), sizeof(prcp));
				prcp = ReverseFloat(prcp);

				if (isnan(prcp) || !isfinite(prcp))
					prcp = -999;

				if (prcp > 0 && prcp < 0.00001f)
					prcp = 0;

				if (prcp < -999)
					prcp = -999;
			
				ASSERT(prcp == -999 || prcp >= 0);

				station[TRef].SetStat(H_PRCP, prcp);
			}
		}

		
		if (msg && station.HaveData())
			msg = station.IsValid();
		
		return msg;
	}


ERMsg CUIGPCP::GetStationListM(StringVector& stationList, CCallback& callback)
{
	ERMsg msg;


	//get all file in the directory 
	StringVector fileList;
	int firstYear = as<int>(FIRST_YEAR);
	int lastYear = as<int>(LAST_YEAR);
	size_t nbYears = lastYear - firstYear + 1;
	string workingDir = GetDir(WORKING_DIR);
	CGeoRect rect = as<CGeoRect>(BOUNDING_BOX); 
	rect.SetPrjID(PRJ_WGS_84);
	//CGeoRect(-80, 43, -50, 63, PRJ_WGS_84);

	string strSearch = workingDir + "Monthly\\gpcp_v2.2_psg.*";
	StringVector fileListTmp = GetFilesList(strSearch);
	sort(fileListTmp.begin(), fileListTmp.end());

	for (size_t i = 0; i < fileListTmp.size() && msg; i++)
	{
		string ext = fileListTmp[i].substr(fileListTmp[i].size() - 4);
		ASSERT(ext.length() == 4);
		int year = ToInt(ext);

		if (year >= firstYear && year <= lastYear)
		{
			fileList.push_back(fileListTmp[i]);
			if (!m_firstTRef.IsInit())
				m_firstTRef = CTRef(year);


			ASSERT((m_firstTRef.GetYear() + fileList.size() - 1) == year);
		}

	}

	if (msg && !fileList.empty())
	{
		_setmaxstdio(1024);
		m_files.resize(fileList.size());

		//remove file 
		for (size_t i = 0; i < fileList.size() && msg; i++)
			msg += m_files[i].open(fileList[i], ios::in | ios::binary);


		stationList.reserve(144 * 72);
		for (size_t i = 0; i < 72; i++)
		{
			for (size_t j = 0; j < 144; j++)
			{
				double lat = 88.75 - 2.5*i;
				double lon = 1.25 + 2.5*j;
				if (lon > 180)
					lon -= 360;

				CGeoPoint pt(lon, lat, PRJ_WGS_84);

				if (rect.IsEmpty() || rect.IsInside(pt))
				{
					string title = FormatA("%03d_%03d", i + 1, j + 1);
					stationList.push_back(title);
				}

				msg += callback.StepIt(0);
			}
		}
	}



	//CGeoExtents extents;
	//extents.m_xMin = -180;
	//extents.m_xMax = 180;
	//extents.m_yMin = -90;
	//extents.m_yMax = 90;
	//extents.m_xSize = 144;
	//extents.m_ySize = 72;
	//extents.m_xBlockSize = 144;
	//extents.m_yBlockSize = 1;

	//CBaseOptions options;
	//options.m_prj = PRJ_WGS_84_WKT;
	//options.m_extents = extents;
	//options.m_nbBands = 1;
	//options.m_dstNodata = -999;
	//options.m_outputType = GDT_Float32;
	//options.m_format = "GTIFF";
	//options.m_bOverwrite = true;
	//options.m_bComputeStats = true;
	//options.m_createOptions.push_back(string("COMPRESS=LZW"));
	//options.m_overviewLevels = { { 2, 4, 8, 16 } };




	//ios::pos_type length = m_files.front().length();

	//m_files.front().seekg(576);

	//CGDALDatasetEx grid;
	//msg += grid.CreateImage("c:/tmp/GPCP_M.tiff", options);
	//ASSERT(msg);
	//GDALRasterBand* pBand = grid.GetRasterBand(0);

	//for (size_t i = 0; i < 72; i++)
	//{
	//	array<float, 144> data;
	//	data.fill(-999);
	//	for (size_t j = 0; j < 144; j++)
	//	{
	//		//double lat = 89.5 - i;
	//		//double lon = 0.5 + j;
	//		//if (lon > 180)
	//		//	lon -= 360;

	//		//CGeoPoint pt(lon, lat, PRJ_WGS_84);


	//		ios::pos_type pos = m_files.front().tellg();
	//		ASSERT(pos < length);

	//		float prcp = 0;
	//		m_files.front().read((char*)(&prcp), sizeof(prcp));
	//		prcp = ReverseFloat(prcp);

	//		if (isnan(prcp) || !isfinite(prcp))
	//			prcp = -999;

	//		if (prcp > 0 && prcp < 0.00001f)
	//			prcp = 0.00001f;

	//		if (prcp < 0)
	//			prcp = -999;

	//		ASSERT(prcp == -999 || (prcp >= 0 && prcp < 316));

	//		data[(j + 72) % 144] = prcp;

	//		msg += callback.StepIt(0);
	//	}

	//	pBand->RasterIO(GF_Write, 0, int(i), extents.m_xSize, 1, &(data[0]), extents.m_xSize, 1, GDT_Float32, 0, 0);
	//}




	//grid.Close();


	return msg;
}

	ERMsg CUIGPCP::GetWeatherStationM(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ASSERT(ID.length() == 7);
		ERMsg msg;

		size_t i = WBSF::as<size_t>(ID.substr(0, 3)) - 1;
		size_t j = WBSF::as<size_t>(ID.substr(4, 3)) - 1;
		double lat = 88.75 - 2.5*i;
		double lon = 1.25 + 2.5*j;
		if (lon > 180)
			lon -= 360;

		((CLocation&)station) = CLocation(ID, ID, lat, lon, 0);


		int firstYear = m_firstTRef.GetYear();
		int lastYear = (m_firstTRef + m_files.size()).GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		for (size_t y = 0; y < m_files.size() && msg; y++)
		{
			int year = m_firstTRef.GetYear() + int(y);
			for (size_t m= 0; m < 12&& msg; m++)
			{
				m_files[y].seekg(576 + (m * 72 * 144 + (i * 144 + j)) * 4);

				float prcp = 0;
				m_files[y].read((char*)(&prcp), sizeof(prcp));
				prcp = ReverseFloat(prcp);

				if (isnan(prcp) || !isfinite(prcp))
					prcp = -999;

				if (prcp > 0 && prcp < 0.00001f)
					prcp = 0;

				if (prcp < -999)
					prcp = -999;

				ASSERT(prcp == -999 || (prcp >= 0 && prcp < 316));

				for (size_t d = 0; d < WBSF::GetNbDayPerMonth(year, m) && msg; d++)
				{
					CTRef TRef(year, m, d);
					station[TRef].SetStat(H_PRCP, prcp);//prcp is in mm/day
				}
			}
		}


		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}


}