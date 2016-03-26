#include "StdAfx.h"
#include "UIEnvCanGribForecast.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "Resource.h"

using namespace std;
using namespace CFL;
using namespace stdString;
using namespace UtilWWW;

//*********************************************************************
const char* CUIEnvCanGribForecast::ATTRIBUTE_NAME[NB_ATTRIBUTE]={ "Type"};
const char* CUIEnvCanGribForecast::CLASS_NAME = "EnvCanGribForecast";
const char* CUIEnvCanGribForecast::SERVER_NAME = "dd.weather.gc.ca";




CUIEnvCanGribForecast::CUIEnvCanGribForecast(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CUIEnvCanGribForecast::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString(IDS_SOURCENAME_ENV_CAN_GRIB_FORECAST);
	

	CUpdaterBase::InitClass(option);

	ASSERT( GetParameters().size() < I_NB_ATTRIBUTE);

	StringVector header(IDS_PROPERTIES_ENV_CAN_GRIB_FORECAST, "|;");
	ASSERT( header.size() == NB_ATTRIBUTE);
	StringVector typeList("00|12", "|");

	GetParameters().push_back( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[0], header[0], typeList, "0") );
}

CUIEnvCanGribForecast::~CUIEnvCanGribForecast(void)
{
}


CUIEnvCanGribForecast::CUIEnvCanGribForecast(const CUIEnvCanGribForecast& in)
{
	operator=(in);
}

void CUIEnvCanGribForecast::Reset()
{
	CUpdaterBase::Reset();
//	m_outputPath.clear();
	m_type=0;
}

CUIEnvCanGribForecast& CUIEnvCanGribForecast::operator =(const CUIEnvCanGribForecast& in)
{
	if( &in != this)
	{
		CUpdaterBase::operator =(in);
	//	m_outputPath=in.m_outputPath;
		m_type=in.m_type;
	}

	return *this;
}

bool CUIEnvCanGribForecast::operator ==(const CUIEnvCanGribForecast& in)const
{
	bool bEqual = true;

	if (CUpdaterBase::operator !=(in))bEqual = false;
	//if( m_outputPath!=in.m_outputPath		   )bEqual = false;
	if( m_type!=in.m_type )bEqual = false;
	
	return bEqual;
}

bool CUIEnvCanGribForecast::operator !=(const CUIEnvCanGribForecast& in)const
{
	return !operator ==(in);
}

string CUIEnvCanGribForecast::GetValue(size_t type)const
{
	string str;
	
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	//case I_OUTPUT_PATH: str = m_outputPath; break;
	case I_TYPE: str = ToString(m_type); break;
	default: str = CUpdaterBase::GetValue(type); break;
	}

	return str;
}

void CUIEnvCanGribForecast::SetValue(size_t type, const string& str)
{
	ASSERT( NB_ATTRIBUTE == 1); 
	switch(type)
	{
	//case I_OUTPUT_PATH: m_outputPath = str; break;
	case I_TYPE: m_type= ToInt(str); break;
	default: CUpdaterBase::SetValue(type, str); break;
	}
}



bool CUIEnvCanGribForecast::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIEnvCanGribForecast& info = dynamic_cast<const CUIEnvCanGribForecast&>(in);
	return operator==(info);
}

CParameterBase& CUIEnvCanGribForecast::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CUIEnvCanGribForecast& info = dynamic_cast<const CUIEnvCanGribForecast&>(in);
	return operator=(info);
}

bool CUIEnvCanGribForecast::NeedDownload(const CFileInfo& info, const string& filePath)const
{
	bool bDownload = true;
/*
	CFileStamp fileStamp(filePath);
	CTime lastUpdate = fileStamp.GetTime();
	if( lastUpdate.GetTime()>0 && info.m_mtime < lastUpdate.GetTime())
	{
		bDownload = false;
	}
	*/
	return bDownload;
}


//This table provides the variable name, level, abbreviation, units and a link to additional grib2 encoding information for each parameter encoded in GRIB2 format					
//Number	Variable	Level	Abbreviation	Units	Description
//0-	Surface Pressure	Surface	PRES_SFC_0	Pascal	Sections 0 to 6
//1-	Precipitation rate	Surface	PRATE_SFC_0	kg m-2 sec-1	Sections 0 to 6
//2-	Mean Sea Level Pressure	MSL	PRMSL_MSL_0	Pascal	Sections 0 to 6
//3-	Temperature	2m above ground	TMP_TGL_2m	Kelvin	Sections 0 to 6
//4-	Area-averaged Surface Humidity	Surface	SPFH_SFC_0	kg kg-1	Sections 0 to 6
//5-	Dew Point temperature	Surface	DPT_SFC_0	Kelvin	Sections 0 to 6
//6-	Accumulated Precipitation	Surface	APCP_SFC_0	kg m-2	Sections 0 to 6
//7-	Convective precipitation	Surface	ACPCP_SFC_0	kg m-2	Sections 0 to 6
//8-	Snow Depth	Surface	SNOD_SFC_0	meter	Sections 0 to 6
//10-	Downward incident solar flux	Surface	DSWRF_SFC_0	Joules m-2	Sections 0 to 6
//11-	Downward Long Wave Radiative Flux	Surface	DLWRF_SFC_0	Joules m-2	Sections 0 to 6
//12-	Net Long Wave Radiation at Surface (Accumulated)	Surface	NLWRS_SFC_0	Joules m-2	Sections 0 to 6
//13-	Net Short Wave Radiation at Surface (Accumulated)	Surface	NSWRS_SFC_0	Joules m-2	Sections 0 to 6
//22-	Wind speed - Module	10m above ground	WIND_TGL_10m	metres per second	Sections 0 to 6
//23-	Wind direction	10m above ground	WDIR_TGL_10m	Degrees relative to i,j components of grid	Sections 0 to 6
//30-	Precipitable water	Column	CWAT_EATM_0	kg m-2	Sections 0 to 6
//31-	Upward Long Wave Radiation Flux	Nominal top of atmosphere	ULWRF_NTAT_0	W m-2	Sections 0 to 6
//32-	Albedo	Surface	ALBDO_SFC_0	Percent	Sections 0 to 6
//34-	Soil temperature near the surface	0-10 cm below ground	TSOIL_SFC_0	Kelvin	Sections 0 to 6
//35-	Deep soil temperature	10 cm below ground	TSOIL_DBLL_100	Kelvin	Sections 0 to 6
//36-	Soil moisture	0-10 cm below ground	SOILW_DBLY_10	m3/m3	Sections 0 to 6
//37-	Model topography - Smoothed	Surface	HGT_SFC_0	m2/s2	Sections 0 to 6
//38-	Land cover	Surface	LAND_SFC_0	fraction	Sections 0 to 6
//272-	Water temperature	Surface	WTMP_SFC_0	Kelvin	Sections 0 to 6
//273-	Accumulated Precipitation type - Snow	Surface	WEASN_SFC_0	meter	Sections 0 to 6
//274-	Accumulated Precipitation type - Rain	Surface	WEARN_SFC_0	kg m-2	Sections 0 to 6
//275-	Accumulated Precipitation type - Ice Pellets	Surface	WEAPE_SFC_0	meter	Sections 0 to 6
//276-	Accumulated Precipitation type - Freezing Rain	Surface	WEAFR_SFC_0	meter	Sections 0 to 6

string CUIEnvCanGribForecast::GetRemoteFilePath(int hhh, const string& filetitle)
{
	string str_hhh = FormatA("%03d", hhh);
	string HH = m_type == TYPE_00HOURS ? "00" : "12";

	return  "model_gem_global/25km/grib2/lat_lon/" + HH + "/" + str_hhh.c_str() + "/" + filetitle + ".grib2";
}

string CUIEnvCanGribForecast::GetOutputFilePath(const string& filetitle)
{
	string HH = m_type == TYPE_00HOURS ? "00" : "12";
	return GetWorkingDir() + HH + "\\" + filetitle + ".grib2";
}

//****************************************************************************************************
ERMsg CUIEnvCanGribForecast::Execute(CFL::CCallback& callback)
{
	ERMsg msg;
	string outputPath = GetWorkingDir();

	CInternetSessionPtr pSession;
	CHttpConnectionPtr pConnection;

	msg = GetHttpConnection( SERVER_NAME, pConnection, pSession);
	if( !msg)
		return msg;

	callback.AddMessage(GetString(IDS_UPDATE_DIR ));
	callback.AddMessage(outputPath, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM ) );
	callback.AddMessage(SERVER_NAME, 1 );
	callback.AddMessage("");

	//delete old files
	{
		StringVector filesList = CFL::GetFilesList(GetOutputFilePath("*"));

		callback.SetCurrentDescription("Delete images files");
		callback.SetNbStep((int)filesList.size());
		for (StringVector::const_iterator it = filesList.begin(); it != filesList.end(); it++)
		{
			ERMsg msgTemp = CFL::RemoveFile(*it);
			if (!msg)
				callback.AddMessage(msgTemp);

			msg += callback.StepIt();
		}
	}

	enum TVariables{PRES_SFC_0,TMP_TGL_2,DPT_SFC_0,APCP_SFC_0,SNOD_SFC_0,WIND_TGL_10,WDIR_TGL_10,WEARN_SFC_0, NB_FORECAST_VAR};
	static const char* FORECAST_VAR_NAME[NB_FORECAST_VAR] = {"PRES_SFC_0","TMP_TGL_2","DPT_TGL_2","APCP_SFC_0","SNOD_SFC_0","WIND_TGL_10","WDIR_TGL_10","WEARN_SFC_0"};
	
	
	callback.SetCurrentDescription( "Download images list");
	callback.SetNbStep((int)240/3+1);

	CFileInfoVector fileList;
	for(int hhh=0; hhh<=240&&msg; hhh+=3)
	{
		//string str_hhh = FormatA("%03d", hhh);
		

		string remotePath = CUIEnvCanGribForecast::GetRemoteFilePath(hhh, "*");
		//string path =  "model_gem_global/25km/grib2/lat_lon/" + HH + "/" + str_hhh.c_str() + "/*.grib2" ;
			
		CFileInfoVector fileListTmp;
		msg = FindFiles(pConnection, remotePath, fileListTmp);

		//keep only some variables
		for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end(); it++)
		{
			string varName = GetFileTitle(it->m_filePath);

			int pos=-1;
			for(int j=0; j<NB_FORECAST_VAR&&pos==-1; j++)
			{
				if (Find(varName, FORECAST_VAR_NAME[j]))
					pos=j;
			}

			if(pos>=0)
				fileList.push_back(*it);
		}

		msg += callback.StepIt();
	}

	callback.AddMessage( "Number of images to download: " + ToString(fileList.size()));
	callback.SetCurrentDescription("Download images");
	callback.SetNbStep((int)fileList.size());

	int nbDownload=0;
	for(size_t i=0; i<fileList.size()&&msg; i++)
	{
		string outputFilePath = GetOutputFilePath(GetFileTitle(fileList[i].m_filePath));
		//string outputFilePath = outputPath + HH + "\\" + GetFileName(fileList[i].m_filePath); //"/CMC_glb_"+FORECAST_VAR_NAME[v]+"_latlon.24x.24_"+strDate+HH+"_P225.grib2";
			
		CreateMultipleDir( GetPath(outputFilePath) );
		msg = CopyFile(pConnection, fileList[i].m_filePath, outputFilePath.c_str(), INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
		if(msg)
			nbDownload++;

		msg += callback.StepIt();
	}

	pConnection->Close();
	pSession->Close();

	
	callback.AddMessage( "Number of images downloaded: " + ToString(nbDownload));
			
	return msg;
}

//****************************************************************************************************
ERMsg CUIEnvCanGribForecast::PreProcess(CFL::CCallback& callback)
{
	ERMsg msg;

	//msg = m_DB.Open(GetDatabaseFilePath(), CHourlyDatabase::modeRead, callback);

	//if (msg)
	//{
	//	m_pShapefile.reset(new CShapeFileBase);
	//	msg = m_pShapefile->Read(GetShapefileFilePath());
	//}

	return msg;
}



ERMsg CUIEnvCanGribForecast::GetStationList(StringVector& stationList, CFL::CCallback& callback)
{
	ERMsg msg;
	
	stationList.clear();
	msg.ajoute("Can't extract station from grid");
	msg.ajoute("Can be used only as forecast extraction");

	return msg;
}


//Extraction section

ERMsg CUIEnvCanGribForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CFL::CCallback& callback)
{
	ERMsg msg;

	if (ID.empty())
	{
		//ASSERT(!station.m_ID.empty());
		//ASSERT(station.m_lat != -999);
		//ASSERT(station.m_lon != -999);

		//set<int> years = m_DB.GetYears();

		//bool bInit = false;

		//for (set<int>::const_iterator it = years.begin(); it != years.end() && !bInit; it++)
		//	if (station.IsYearInit(*it))
		//		bInit = true;

		////no forecast are added on old data
		//if (bInit)
		//{

		//	if (msg && !m_DEMFilePath.empty())
		//		msg = dataset.Open(m_DEMFilePath.c_str(), GA_ReadOnly);

		//	if (msg)
		//	{
		//		callback.AddMessage(GetString(IDS_GSOD_OPTIMISATION));
		//		callback.AddMessage(refFilePath, 1);
		//		callback.AddMessage("");

		//		callback.SetCurrentDescription(GetString(IDS_GSOD_OPTIMISATION));
		//		callback.SetNbStep(file.length(), 81 + 1);

		//		elevAllStationFile << "Name,ID,Lat,Lon,Elev,res,ElevLo(SRTM4.1),Elev(SRTM4.1),ElevHi(SRTM4.1)\n";

		//		string line;
		//		while (msg && std::getline(file, line))
		//		{
		//			int res = 0;
		//			double elevLo = -999;
		//			double elev = -999;
		//			double elevHi = -999;


		//			CLocation location;
		//			if (LocationFromLine(line, location))
		//			{
		//				ASSERT(location.m_lat != -999 && location.m_lon != -999);

		//				location.SetSSI("DEMElevation", "0");
		//				if (location.m_elev == -999)// || (location.m_elev == 0 && location.GetSSI("Country")=="BR" ) a lot of brasil station have 0 as elevation
		//				{
		//					res = GetPrecision(line);
		//					double pres = pow(10.0, -res);
		//					CGeoPoint coord1 = location - pres;
		//					CGeoPoint coord2 = location + pres;

		//					GetZoneElevation(dataset, location, coord1, coord2, elevLo, elev, elevHi);
		//					if (fabs(elev - elevLo)<50 && fabs(elev - elevHi)<50)
		//					{
		//						location.m_elev = GetStationElevation(dataset, location, 9, 50);
		//						location.SetSSI("DEMElevation", "1");
		//					}
		//				}


		//				if (location.m_elev != -999)
		//					me[location.m_ID] = location;
		//			}
		//			else
		//			{
		//				if (excludeFile.is_open())
		//					excludeFile << line + "\n";
		//			}

		//			if (elevLo != -999 && elev != -999 && elevHi != -999)
		//			{
		//				CStdString str;
		//				for (int i = 0; i<CLocation::NB_MEMBER; i++)
		//					str += location.GetMember(i) + ",";

		//				str += ToString(res) + ",";
		//				str += ToString(elevLo, 1) + ",";
		//				str += ToString(elev, 1) + ",";
		//				str += ToString(elevHi, 1) + "\n";
		//				elevAllStationFile << str;
		//			}



		//			msg += callback.StepIt();
		//		}

		//		file.close();
		//		excludeFile.close();
		//		elevAllStationFile.close();



		//	int shapeNo = -1;
		//	//double d = m_pShapefile->GetMinimumDistance(station, &shapeNo);
		//	if (m_pShapefile->IsInside(station, &shapeNo))//inside a shape
		//	{
		//		const CDBF3& DBF = m_pShapefile->GetDBF();

		//		int Findex = DBF.GetFieldIndex("PAGEID");
		//		string forecastID = DBF[shapeNo][Findex].GetElement();

		//		CWeatherDatabaseOptimization const& zop = m_DB.GetOptimization();
		//		int index = zop.FindByID(forecastID);
		//		if (index >= 0)
		//		{
		//			CWeatherStation st;
		//			msg = m_DB.Get(st, index);
		//			if (msg)
		//			{
		//				CWVariablesCounter counter = station.GetVariablesCount();
		//				CWVariables varInfo = counter.GetVariables();

		//				//const CWeatherYears& data = st;
		//				CTPeriod p = st.GetVariablesCount().m_period;

		//				CWeatherAccumulator stat(TM);
		//				for (CTRef d = p.Begin(); d <= p.End(); d++)
		//				{
		//					if (stat.TRefIsChanging(d))
		//						if (station[stat.GetTRef()].GetVariablesCount().GetSum() == 0)
		//						{
		//						station[stat.GetTRef()].SetData(stat);
		//						CDay& day = station(d.GetYear())[d.GetMonth()][d.GetDay()];
		//						int i;
		//						i = 0;
		//						}

		//					const CHourlyData& hour1 = st(d.GetYear())[d.GetMonth()][d.GetDay()][d.GetHour()];

		//					for (int v = 0; v <NB_VAR_H; v++)
		//						if (varInfo[v])
		//							if (hour1[v]>-999)
		//								stat.Add(d, v, hour1[v]);

		//				}
		//			}
		//		}
		//	}
		//}
	}
	

	return msg;
}
