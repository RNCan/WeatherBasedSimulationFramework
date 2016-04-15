#include "StdAfx.h"

#include "UIGHCN.h"
#include <boost\filesystem.hpp>
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../Resource.h"

#include "CountrySelection.h"
#include "StateSelection.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{


using namespace boost;


const int CUIGHCND::GHCN_VARIABLES[NB_VARIABLES] = { TMIN, TMAX, PRCP, AWND , WESF, SNWD, WESD };


//********************************************************************************************
const char* CUIGHCND::ELEM_CODE[NB_ELEMENT] =
{
	"ASMM", "ASSS", "AWND", "CLDG", "DPNT", "DPTP", "DYSW", "DYVC",
	"F2MN", "F5SC", "FMTM", "FRGB", "FRGT", "FRTH", "FSIN", "FSMI",
	"FSMN", "GAHT", "HTDG", "MNRH", "MNTP", "MXRH", "PGTM", "PKGS",
	"PRCP", "PRES", "PSUN", "RDIR", "RWND", "SAMM", "SASS", "SCMM",
	"SCSS", "SGMM", "SGSS", "SLVP", "SMMM", "SMSS", "SNOW", "SNWD",
	"WESF", "WESD",
	"STMM", "STSS", "THIC", "TMAX", "TMIN", "TMPW", "TSUN", "WTEQ",
	"EVAP", "MNPN", "MXPN", "TOBS", "WDMV"
};

short CUIGHCND::GetElementType(const char* type)
{
	short pos = -1;
    for(int i=0; i<NB_ELEMENT&&pos==-1; i++)
		if( IsEqualNoCase(type, ELEM_CODE[i]) )
            pos = i;

    return pos;
}

//Global Climate Observing System (GCOS) Surface Network (GSN)
const char* CUIGHCND::SERVER_NAME = "ftp.ncdc.noaa.gov";
const char* CUIGHCND::SERVER_PATH = "pub/data/ghcn/daily/";

//*********************************************************************
const char* CUIGHCND::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States", "ShowProgress" };
const size_t CUIGHCND::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_BROWSE, T_STRING_BROWSE, T_BOOL };
const UINT CUIGHCND::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_GHCND_P;
const UINT CUIGHCND::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_GHCND;

const char* CUIGHCND::CLASS_NAME(){ static const char* THE_CLASS_NAME = "GHCND";  return THE_CLASS_NAME; }
CTaskBase::TType CUIGHCND::ClassType()const { return CTaskBase::UPDATER; }
static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGHCND::CLASS_NAME(), (createF)CUIGHCND::create);



CUIGHCND::CUIGHCND(void)
{}

CUIGHCND::~CUIGHCND(void)
{}


std::string CUIGHCND::Option(size_t i)const
{
	string str;
	switch (i)
	{
	case COUNTRIES:	str = CCountrySelection::GetAllPossibleValue(); break;
	case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
	};
	return str;
}

std::string CUIGHCND::Default(size_t i)const
{
	string str;

	switch (i)
	{
	case FIRST_YEAR:
	case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
	};

	return str;
}

//****************************************************


ERMsg CUIGHCND::UpdateStationHistory(CCallback& callback)
{
	ERMsg msg;

	

	CInternetSessionPtr pSession;
	CFtpConnectionPtr pConnection;
	
	msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@hotmail.com", true);
	if (msg)
	{
		string path = GetStationFilePath(false);

		CFileInfoVector fileList;
		msg = FindFiles(pConnection, path, fileList, callback);
		
		pConnection->Close();
		pSession->Close();

		if( msg)
		{
			ASSERT( fileList.size() == 1);

			string outputFilePath = GetStationFilePath(true);
			if( !IsFileUpToDate(fileList[0], outputFilePath) )
			{
				msg = FTPDownload(SERVER_NAME, path, outputFilePath, callback);
			}
		}
	}

	return msg;
}

ERMsg CUIGHCND::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
{
	ERMsg msg;

	fileList.clear();

	///int nbYears = m_lastYear-m_firstYear+1;

	callback.PushTask(GetString(IDS_LOAD_FILE_LIST), 1);
	//callback.SetNbStep(1);

	//open a connection on the server
	CInternetSessionPtr pSession;
	CFtpConnectionPtr pConnection;
		
	msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@hotmail.com", true);
	if (msg)
	{
		//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);
		string filter = string(SERVER_PATH) + "by_year/*.gz";
		msg = FindFiles(pConnection, filter, fileList, callback);	

		pConnection->Close();
		pSession->Close();
	}
	
	callback.PopTask();

	
	//remove unwanted file
	if(msg)
	{
		callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()), 1);
		msg = CleanList(fileList, callback);
	}

	return msg;
}

ERMsg CUIGHCND::FTPDownload(const string& server, const string& inputFilePath, const string& outputFilePath, CCallback& callback)
{
	ERMsg msg;

	callback.PushTask("FTPTransfer...", NOT_INIT);

	string command = "\"" + GetApplicationPath() + "External\\FTPTransfer.exe\" -Server \"" + server + "\" -Remote \"" + inputFilePath + "\" -Local \"" + outputFilePath + "\" -Passive -Download";

	UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;

	DWORD exitCode=0;
	msg = WinExecWait(command, GetTempPath(), show, &exitCode);
	if( msg && exitCode!=0)
		msg.ajoute("FTPTransfer as exit with error code " + ToString(int(exitCode)) );
	
	callback.PopTask();

	return msg;
}

ERMsg CUIGHCND::sevenZ(const string& filePathZip, const string& workingDir, CCallback& callback)
{
	ERMsg msg;
	
	callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

	string command = GetApplicationPath() + "External\\7z.exe x \"" + filePathZip+ "\" -y";
	//UINT show = as<bool>(SHOW_APP) ? SW_SHOW : SW_HIDE;

	DWORD exitCode=0;
	msg = WinExecWait(command, workingDir, SW_HIDE, &exitCode);
	if( msg && exitCode!=0)
		msg.ajoute("7z.exe as exit with error code " + ToString(int(exitCode)));
	

	callback.PopTask();


	return msg;
}

ERMsg CUIGHCND::Execute(CCallback& callback)
{
	ERMsg msg;

	string workingDir = GetDir(WORKING_DIR); 
	CreateMultipleDir( workingDir );

	string tarFilePath = workingDir + "ghcnd_all.tar";
	string gzFilePath = tarFilePath + ".gz";
	string InputFilePath = string(SERVER_PATH) + "ghcnd_all.tar.gz";

	callback.AddMessage(GetString(IDS_UPDATE_DIR ));
	callback.AddMessage(workingDir, 1);
	callback.AddMessage(GetString(IDS_UPDATE_FROM ) );
	callback.AddMessage(string(SERVER_NAME) +  "/" + SERVER_PATH, 1 );
	callback.AddMessage("");
		
	
	msg = UpdateStationHistory(callback);
	if(!msg)
		return msg;
	
	CFileInfoVector fileList;
	msg = GetFileList(fileList, callback);

	if( !msg)
		return msg;

	callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(fileList.size()), 1);
	callback.AddMessage("");
	
	callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());

	int nbRun = 0;
	size_t curI = 0;

	while(curI<fileList.size() && nbRun<3 && msg)
	{
		nbRun++;
			
		//ERMsg msgTmp;
		for (size_t i = curI; i<fileList.size() && msg; i++)
		{
			msg += callback.StepIt(0);
			if (msg)
			{
				string outputFilePath = GetOutputFilePath(fileList[i]) + ".gz";

				CreateMultipleDir(GetPath(outputFilePath));
				callback.AddMessage("Download " + GetFileName(outputFilePath) + " ...");

				msg = FTPDownload(SERVER_NAME, fileList[i].m_filePath, outputFilePath.c_str(), callback);

				//unzip it
				if (msg)
				{
					callback.AddMessage("Unzip " + GetFileName(outputFilePath) + " ...");
					msg = sevenZ(outputFilePath.c_str(), GetPath(outputFilePath).c_str(), callback);
					RemoveFile(outputFilePath);

					//update time to the time of the .gz file
					boost::filesystem::path p(outputFilePath);
					if (boost::filesystem::exists(p))
						boost::filesystem::last_write_time(p, fileList[i].m_time);


					if (msg)
						curI++;
				}
			}
		}
	}

	callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(curI), ToString(fileList.size())));
	callback.PopTask();

	return msg;
}
string CUIGHCND::GetOutputFilePath(const string& fileTitle)const
{
	return GetDir(WORKING_DIR) + "by_year\\" + fileTitle + ".csv";
}

string CUIGHCND::GetOutputFilePath(const CFileInfo& info)const
{
	return GetOutputFilePath( GetFileTitle(info.m_filePath).c_str() );
}

bool CUIGHCND::IsFileInclude(const string& fileTitle)const
{
	bool bRep = false;

	if( StationExist(fileTitle) )
	{
		CLocation location;
		GetStationInformation(fileTitle, location);

		bRep = IsStationInclude( location.m_ID );
	}

	return bRep;
}

bool CUIGHCND::IsStationInclude(const string& ID)const
{
	bool bRep = false;

	if( StationExist(ID.c_str()) )
	{
		CLocation location;
		GetStationInformation(ID, location);
		bool bDEMElev = ToBool(location.GetSSI("DEMElevation"));
		if( !bDEMElev || false )
		{
			CCountrySelection countries(Get(COUNTRIES));
			//CGeoRect boundingBox;
			size_t country = CCountrySelection::GetCountry(location.GetSSI("Country").c_str());
			ASSERT( country != NOT_INIT);

			if( countries.none() || countries.at(country) )
			{
				//if( boundingBox.IsRectEmpty() ||
					//boundingBox.PtInRect( location ) )
				//{
					if( IsEqualNoCase( location.GetSSI("Country"), "US") )
					{
						CStateSelection states(Get(STATES));
						size_t state = CStateSelection::GetState(location.GetSSI("State").c_str());
						if (states.none() || (state<states.size() && states.at(state)))
							bRep=true;
					}
					else 
					{
						bRep=true;
					}//use this state if USA
				//}//in the bounding extents
			}//use this country
		}//use DEM elevation
	}//station exist

	return bRep;
}

ERMsg CUIGHCND::CleanList(StringVector& fileList, CCallback& callback)const
{
	ERMsg msg;

	callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
	//callback.SetNbStep(fileList.size());
	

	for (StringVector::iterator it = fileList.begin(); it != fileList.end() && msg;)
	{
		string fileTitle = GetFileTitle(*it);

		if (!IsFileInclude(fileTitle))
			it = fileList.erase(it);
		else
			it++;

		msg += callback.StepIt();
	}

	callback.PopTask();

	return msg;
}

ERMsg CUIGHCND::CleanList(CFileInfoVector& fileList, CCallback& callback)const
{
	ERMsg msg;

	string workingDir = GetDir(WORKING_DIR); 
	int firstYear = as<int>(FIRST_YEAR);
	int lastYear = as<int>(LAST_YEAR);

	//CStdioFile excludedFile;
	//excludedFile.Open( workingDir + "FileWithoutInfo.txt", CStdFile::modeWrite|CStdFile::modeCreate);

	callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
	//callback.SetNbStep(fileList.size());
	

	for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end()&&msg; )
	{
		int year = ToInt(GetFileTitle(it->m_filePath));
		string outputFilePath = GetOutputFilePath(*it);

		if (year<firstYear || year>lastYear || IsFileUpToDate(*it, outputFilePath, false))
			it = fileList.erase(it);
		else
			it++;


		msg += callback.StepIt();
	}			
	
	callback.PopTask();
	//excludedFile.Close();
	//CStdioFile file("d:\\travail\\anomalie.txt", CStdFile::modeWrite|CStdFile::modeCreate);
	//for( int i=0; i<missStationArray.size(); i++)
	//{
	//	file.WriteString(missStationArray[i]);
	//	file.WriteString("\n");
	//}
	//file.Close();

	return msg;
}

ERMsg CUIGHCND::LoadOptimisation()
{
	//load station list in memory for optimization
	ERMsg msg;
	string filePath = GetStationFilePath();
	
	msg = m_optFile.Load(GetOptFilePath());
	return msg;
}	


string CUIGHCND::GetOptFilePath()const
{
	string optFilePath(GetStationFilePath());
	SetFileExtension(optFilePath, ".GHCNopt");

	return optFilePath;
}


bool CUIGHCND::StationExist(const string& fileTitle)const
{
	return m_optFile.KeyExists(fileTitle);
}

void CUIGHCND::GetStationInformation(const string& ID, CLocation& location)const
{
	CLocationOptimisation::const_iterator it = m_optFile.find(ID);
	ASSERT( it != m_optFile.end() );

	if( it != m_optFile.end() )
		location = it->second;
	
}

ERMsg CUIGHCND::UpdateOptimisationStationFile(const string& workingDir, CCallback& callback)const
{
    ERMsg msg;

	//CreateMultipleDir(workingDir+ "Opt\\");
	
	string refFilePath = GetStationFilePath();
	string optFilePath = GetOptFilePath();
	
	if( CLocationOptimisation::NeedUpdate(refFilePath, optFilePath) )
	{
		CGHCNStationOptimisation optFile;
		//if (bUseDEMStation)
			//optFile.m_DEMFilePath = GetAbsoluteFilePath(m_DEMFilePath);

		msg = optFile.Update(refFilePath, callback);
		if( msg )
			msg = optFile.Save( optFilePath );
	}

	return msg;
}

ERMsg CUIGHCND::PreProcess(CCallback& callback)
{
	ERMsg msg;

	m_loadedData.clear();
	int firstYear = as<int>(FIRST_YEAR);
	int lastYear = as<int>(LAST_YEAR);


	string path = GetDir(WORKING_DIR);
	msg = UpdateOptimisationStationFile(path, callback);
	if (msg)
	{
		string workingDir = GetDir(WORKING_DIR); 
		
		msg = LoadOptimisation();
		if (msg)
		{
			size_t nbYears = lastYear - firstYear + 1;
			//callback.AddTask(nbYears);

			for (size_t y = 0; y<nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				string filePath = path + "by_year\\" + ToString(year) + ".csv";
				msg = LoadData(filePath, m_loadedData, callback);
			}
		}
	}

	return msg;
}

ERMsg CUIGHCND::GetStationList(StringVector& list, CCallback& callback)
{
	ERMsg msg; 

	msg = PreProcess(callback);
	if (!msg)
		return msg;

	list.clear();
		
	for (SimpleDataMap::const_iterator it=m_loadedData.begin(); it != m_loadedData.end(); it++)
	{
		list.push_back(it->first);
	}


	return msg;
}


static std::string TraitFileName(std::string name)
{
	size_t begin = name.find('(');
	size_t end = name.find(')');
	if( begin!=std::string::npos && end!=std::string::npos)
		name.erase(begin, end);

	ReplaceString(name, "&amp;", "&");
	std::replace(name.begin(), name.end(), ',', '-');
	std::replace(name.begin(), name.end(), '.', '·');
	
	//std::replace(name.begin(), name.end(), ',', '-');
	
	Trim(name);
	
	return UppercaseFirstLetter(name);
}

ERMsg CUIGHCND::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback )
{
	ERMsg msg;
	

	GetStationInformation(ID, station);
	station.m_name = PurgeFileName(station.m_name);

	const SimpleDataYears& data = m_loadedData.at(ID);

	for (SimpleDataYears::const_iterator it = data.begin(); it != data.end()&&msg; it++)
	{
		const SimpleDataYear& dataYear = it->second;
		for (size_t jd = 0; jd < dataYear.size()&&msg; jd++)
		{
			CJDayRef TRef(it->first, jd);

			CDay& day = (CDay&)station[TRef];
			
			 
			if (dataYear[jd][V_TMIN]>-999 && dataYear[jd][V_TMAX]>-999)
			{
				double Tmin = (double)dataYear[jd][V_TMIN];
				double Tmax = (double)dataYear[jd][V_TMAX];
				assert(Tmin<Tmax);
				if (Tmin > Tmax)
					Switch(Tmin, Tmax);

				day.SetStat(H_TAIR, (Tmin + Tmax)/2);
				day.SetStat(H_TRNG, Tmax - Tmin);
			}
			
			if (dataYear[jd][V_PRCP]>-999 )
				day.SetStat(H_PRCP, dataYear[jd][V_PRCP]);
			
			if (dataYear[jd][V_AWND]>-999)
				day.SetStat(H_WNDS, dataYear[jd][V_AWND]);

		}
	}

	if (msg && station.HaveData())
	{
		//verify station is valid
		msg = station.IsValid();
	}

	return msg;
}

//PRCP = Precipitation (tenths of mm)
//SNOW = Snowfall (mm)
//SNWD = Snow depth (mm)
//TMAX = Maximum temperature (tenths of degrees C)
//TMIN = Minimum temperature (tenths of degrees C)
//	   
//The other elements are:
//	   
//ACMC = Average cloudiness midnight to midnight from 30-second ceilometer data (percent)
//ACMH = Average cloudiness midnight to midnight from manual observations (percent)
//ACSC = Average cloudiness sunrise to sunset from 30-second ceilometer data (percent)
//ACSH = Average cloudiness sunrise to sunset from manual observations (percent)
//AWND = Average daily wind speed (tenths of meters per second)
//DAEV = Number of days included in the multiday evaporation total (MDEV)
//DAPR = Number of days included in the multiday precipiation total (MDPR)
//DASF = Number of days included in the multiday snowfall total (MDSF)		  
//DATN = Number of days included in the multiday minimum temperature (MDTN)
//DATX = Number of days included in the multiday maximum temperature (MDTX)
//DAWM = Number of days included in the multiday wind movement (MDWM)
//DWPR = Number of days with non-zero precipitation included in multiday precipitation total (MDPR)
//EVAP = Evaporation of water from evaporation pan (tenths of mm) 
//FMTM = Time of fastest mile or fastest 1-minute wind (hours and minutes, i.e., HHMM)
//FRGB = Base of frozen ground layer (cm)
//FRGT = Top of frozen ground layer (cm)
//FRTH = Thickness of frozen ground layer (cm)
//GAHT = Difference between river and gauge height (cm)
//MDEV = Multiday evaporation total (tenths of mm; use with DAEV)
//MDPR = Multiday precipitation total (tenths of mm; use with DAPR and DWPR, if available)
//MDSF = Multiday snowfall total 
//MDTN = Multiday minimum temperature (tenths of degrees C; use with DATN)
//MDTX = Multiday maximum temperature (tenths of degress C; use with DATX)
//MDWM = Multiday wind movement (km)
//MNPN = Daily minimum temperature of water in an evaporation pan (tenths of degrees C)
//MXPN = Daily maximum temperature of water in an evaporation pan (tenths of degrees C)
//PGTM = Peak gust time (hours and minutes, i.e., HHMM)
//PSUN = Daily percent of possible sunshine (percent)
//SN*# = Minimum soil temperature (tenths of degrees C) where * corresponds to a code for ground cover and # corresponds to a code for soil depth.  
//	Ground cover codes include the following:
//		0 = unknown
//		1 = grass
//		2 = fallow
//		3 = bare ground
//		4 = brome grass
//		5 = sod
//		6 = straw multch
//		7 = grass muck
//		8 = bare muck
//	Depth codes include the following:
//		1 = 5 cm
//		2 = 10 cm
//		3 = 20 cm
//		4 = 50 cm
//		5 = 100 cm
//		6 = 150 cm
//		7 = 180 cm
//SX*# = Maximum soil temperature (tenths of degrees C) where * corresponds to a code for ground cover and # corresponds to a code for soil depth. See SN*# for ground cover and depth codes. 
//THIC = Thickness of ice on water (tenths of mm)	
//TOBS = Temperature at the time of observation (tenths of degrees C)
//TSUN = Daily total sunshine (minutes)
//WDF1 = Direction of fastest 1-minute wind (degrees)
//WDF2 = Direction of fastest 2-minute wind (degrees)
//WDF5 = Direction of fastest 5-second wind (degrees)
//WDFG = Direction of peak wind gust (degrees)
//WDFI = Direction of highest instantaneous wind (degrees)
//WDFM = Fastest mile wind direction (degrees)
//WDMV = 24-hour wind movement (km)	   
//WESD = Water equivalent of snow on the ground (tenths of mm)
//WESF = Water equivalent of snowfall (tenths of mm)
//WSF1 = Fastest 1-minute wind speed (tenths of meters per second)
//WSF2 = Fastest 2-minute wind speed (tenths of meters per second)
//WSF5 = Fastest 5-second wind speed (tenths of meters per second)
//WSFG = Peak guest wind speed (tenths of meters per second)
//WSFI = Highest instantaneous wind speed (tenths of meters per second)
//WSFM = Fastest mile wind speed (tenths of meters per second)
//WT** = Weather Type where ** has one of the following values:
//	01 = Fog, ice fog, or freezing fog (may include heavy fog)
//	02 = Heavy fog or heaving freezing fog (not always distinguished from fog)
//	03 = Thunder
//	04 = Ice pellets, sleet, snow pellets, or small hail 
//	05 = Hail (may include small hail)
//	06 = Glaze or rime 
//	07 = Dust, volcanic ash, blowing dust, blowing sand, or blowing obstruction
//	08 = Smoke or haze 
//	09 = Blowing or drifting snow
//	10 = Tornado, waterspout, or funnel cloud 
//	11 = High or damaging winds
//	12 = Blowing spray
//	13 = Mist
//	14 = Drizzle
//	15 = Freezing drizzle 
//	16 = Rain (may include freezing rain, drizzle, and freezing drizzle) 
//	17 = Freezing rain 
//	18 = Snow, snow pellets, snow grains, or ice crystals
//	19 = Unknown source of precipitation 
//	21 = Ground fog 
//	22 = Ice fog or freezing fog
//WV** = Weather in the Vicinity where ** has one of the following values:
//	01 = Fog, ice fog, or freezing fog (may include heavy fog)
//	03 = Thunder
//	07 = Ash, dust, sand, or other blowing obstruction
//	18 = Snow or ice crystals
//	20 = Rain or snow shower

//MFLAG1     is the measurement flag.  There are ten possible values:
//	Blank = no measurement information applicable
//	B     = precipitation total formed from two 12-hour totals
//	D     = precipitation total formed from four six-hour totals
//	H     = represents highest or lowest hourly temperature
//	K     = converted from knots 
//	L     = temperature appears to be lagged with respect to reported hour of observation
//	O     = converted from oktas 
//	P     = identified as "missing presumed zero" in DSI 3200 and 3206
//	T     = trace of precipitation, snowfall, or snow depth
//	W     = converted from 16-point WBAN code (for wind direction)
//
//QFLAG1     is the quality flag.  There are fourteen possible values:
//	Blank = did not fail any quality assurance check
//	D     = failed duplicate check
//	G     = failed gap check
//	I     = failed internal consistency check
//	K     = failed streak/frequent-value check
//	L     = failed check on length of multiday period 
//	M     = failed megaconsistency check
//	N     = failed naught check
//	O     = failed climatological outlier check
//	R     = failed lagged range check
//	S     = failed spatial consistency check
//	T     = failed temporal consistency check
//	W     = temperature too warm for snow
//	X     = failed bounds check
//	Z     = flagged as a result of an official Datzilla investigation
//
//SFLAG1     is the source flag.  There are twenty eight possible values (including blank, upper and lower case letters):
//	Blank = No source (i.e., data value missing)
//	0     = U.S. Cooperative Summary of the Day (NCDC DSI-3200)
//	6     = CDMP Cooperative Summary of the Day (NCDC DSI-3206)
//	7     = U.S. Cooperative Summary of the Day -- Transmitted via WxCoder3 (NCDC DSI-3207)
//	A     = U.S. Automated Surface Observing System (ASOS) real-time data (since January 1, 2006)
//	a     = Australian data from the Australian Bureau of Meteorology
//	B     = U.S. ASOS data for October 2000-December 2005 (NCDC DSI-3211)
//	b     = Belarus update
//	E     = European Climate Assessment and Dataset (Klein Tank et al., 2002)	   
//	F     = U.S. Fort data 
//	G     = Official Global Climate Observing System (GCOS) or other government-supplied data
//	H     = High Plains Regional Climate Center real-time data
//	I     = International collection (non U.S. data received through  personal contacts)
//	K     = U.S. Cooperative Summary of the Day data digitized from paper observer forms (from 2011 to present)
//	M     = Monthly METAR Extract (additional ASOS data)
//	N     = Community Collaborative Rain, Hail,and Snow (CoCoRaHS)
//	Q     = Data from several African countries that had been "quarantined", that is, withheld from public release until permission was granted from the respective meteorological services
//	R     = NCDC Reference Network Database (Climate Reference Network and Historical Climatology Network-Modernized)
//	r     = All-Russian Research Institute of Hydrometeorological Information-World Data Center
//	S     = Global Summary of the Day (NCDC DSI-9618)
//		NOTE: "S" values are derived from hourly synoptic reports exchanged on the Global Telecommunications System (GTS).
//		Daily values derived in this fashion may differ significantly from "true" daily data, particularly for precipitation (i.e., use with caution).
//	s     = China Meteorological Administration/National Meteorological Information Center/Climatic Data Center (http://cdc.cma.gov.cn)
//	T     = SNOwpack TELemtry (SNOTEL) data obtained from the Western  Regional Climate Center
//	U     = Remote Automatic Weather Station (RAWS) data obtained from the Western Regional Climate Center	   
//	u     = Ukraine update	   
//	W     = WBAN/ASOS Summary of the Day from NCDC's Integrated Surface Data (ISD).  
//	X     = U.S. First-Order Summary of the Day (NCDC DSI-3210)
//	Z     = Datzilla official additions or replacements 
//	z     = Uzbekistan update
//
//	When data are available for the same time from more than one source, the highest priority source is chosen according to the following
//	priority order (from highest to lowest):   Z,R,0,6,X,W,K,7,F,B,M,r,E,z,u,b,a,s,G,Q,I,A,N,T,U,H,S





ERMsg CUIGHCND::LoadData(const string& filePath, SimpleDataMap& data, CCallback& callback)const
{
	ASSERT(FileExists(filePath));


	//ID = 11 character station identification code
	//YEAR/MONTH/DAY = 8 character date in YYYYMMDD format (e.g. 19860529 = May 29, 1986)
	//ELEMENT = 4 character indicator of element type 
	//DATA VALUE = 5 character data value for ELEMENT 
	//M-FLAG = 1 character Measurement Flag 
	//Q-FLAG = 1 character Quality Flag 
	//S-FLAG = 1 character Source Flag 
	//OBS-TIME = 4-character time of observation in hour-minute format (i.e. 0700 =7:00 am)

	enum TFields{ GHCN_ID, GHCN_DATE, GHCN_ELEMENT, GHCN_DATA, GHCN_M, GHCN_Q, GHCN_S };


	ERMsg msg;

	ifStream file;
	msg = file.open(filePath);

	if (msg)
	{
		file.seekg(0, std::istream::end);
		std::istream::pos_type length = file.tellg();
		file.seekg(0);

		callback.PushTask("Load in memory " + GetFileName(filePath), length);
		//callback.SetNbStep(length);

		short year = ToShort(GetFileTitle(filePath));
		//CWeatherStationMap::iterator it = stations.end();
		//SimpleDataMap::iterator it = data.end();

		bool bInclude = true;
		string lastID;
		for (CSVIterator loop(file); loop != CSVIterator()&&msg; ++loop)
		{
			string ID = (*loop)[GHCN_ID];
			//StationID ID11 = ID;
			

			if (ID != lastID)
			{
				if (m_included.find(ID) != m_included.end())
				{
					bInclude = true;
				}
				else if (m_rejected.find(ID) != m_rejected.end())
				{
					bInclude = false;
				}
				else
				{
					bInclude = IsStationInclude(ID);
					if (bInclude)
						const_cast<CUIGHCND*>(this)->m_included.insert(ID);
					else
						const_cast<CUIGHCND*>(this)->m_rejected.insert(ID);
				}
				
				lastID = ID;
			}

			if (bInclude)
			{
				const string& date = (*loop)[GHCN_DATE];
				ASSERT(date.length() == 8);

				int year = ToValue<int>(date.substr(0, 4)); ASSERT(year >= 1800 && year<2020);
				int month = ToValue<int>(date.substr(4, 2)) - 1; ASSERT(month >= 0 && month<12);
				int day = ToValue<int>(date.substr(6, 2)) - 1; ASSERT(day >= 0 && day<GetNbDayPerMonth(year, month));
				ASSERT(day >= 0 && day<GetNbDayPerMonth(year, month));

				if (day >= 0 && day<GetNbDayPerMonth(year, month))
				{
					CTRef TRef(year, month, day);

					int type = GetElementType((*loop)[GHCN_ELEMENT].c_str());
					string strValue = TrimConst((*loop)[GHCN_DATA]);
					char Mf1 = (*loop)[GHCN_M].empty() ? ' ' : ToValue<char>((*loop)[GHCN_M]);
					char Qf2 = (*loop)[GHCN_Q].empty() ? ' ' : ToValue<char>((*loop)[GHCN_Q]);
					char Sf3 = (*loop)[GHCN_S].empty() ? ' ' : ToValue<char>((*loop)[GHCN_S]);
					

					float value = ToFloat(strValue);

					if (!strValue.empty() && Qf2 == ' ')//is valid
					{
						switch (type)
						{

						case TMIN:
							ASSERT(value>-999 && value<999 || value == -9999);
							if (value > -999 && value < 999)
							{
								//10e of °C --> °C
								data[ID][year][TRef.GetJDay()][V_TMIN] = value / 10;
							}
							break;

						case TMAX:
							ASSERT(value > -999 && value < 999 || value == -9999);
							if (value > -999 && value < 999)
							{
								//10e of °C --> °C
								data[ID][year][TRef.GetJDay()][V_TMAX] = value / 10;
							}
							break;

						
						case PRCP:
							ASSERT((int)value >= 0 && value < 9999 || value == -9999);
							if ((int)value >= 0 && value < 9999)
							{
								//10e of mm --> mm
								data[ID][year][TRef.GetJDay()][V_PRCP] = value / 10;
							}
							break;

						case AWND://Wind speed
							ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
							if ((int)value >= 0 && value < 9999)
							{
								//10e of m/s --> km/h
								data[ID][year][TRef.GetJDay()][V_AWND] = (value / 10) * 3600 / 1000;
							}
							break;

						case WESF: //Water equivalent of snowfall
							ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
							if ((int)value >= 0 && value < 9999)
							{
								//10e of mm --> mm
								data[ID][year][TRef.GetJDay()][V_WESF] = (value / 10);
							}
							break;
						case SNWD://snow depth
							ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
							if ((int)value >= 0 && value < 9999)
							{
								//mm --> cm
								data[ID][year][TRef.GetJDay()][V_SNWD] = (value / 10);
							}
							break;

						case WESD://Water equivalent of snow on the ground
							ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
							if ((int)value >= 0 && value < 9999)
							{
								//10e of mm --> mm
								data[ID][year][TRef.GetJDay()][V_WESD] = (value / 10);
							}
							break;

						

							//case DPTP:
							//	ASSERT( value > -999 && value < 999 || value==99999 );
							//	//ASSERT( unit == "TF");
							//	if( value > -999 && value < 999 )
							//	{
							//		//test
							//		value = (value/10.0f-32)*5.f/9.f;//10e of Fahrenheit - > °C
							//		data(year)[month][day].SetData(H_TDEW, value);
							//	}
							//	break;
							//case MNRH:
							//	ASSERT( (int)value >=0 && (int)value<=100 || value==99999);
							//	//ASSERT( unit == "P ");
							//	if( (int)value>=0 && (int)value<=100)
							//	{
							//		CStatistic stat = data(year)[month][day].GetData(H_RELH);
							//		stat += value;
							//		data(year)[month][day].SetData(H_RELH, stat);
							//	}
							//	break;
							//case MXRH:
							//	ASSERT( (int)value >=0 && (int)value<=100 || value==99999);
							//	//ASSERT( unit == "P ");
							//	if( (int)value>=0 && (int)value<=100)
							//	{
							//		CStatistic stat = data(year)[month][day].GetData(H_RELH);
							//		stat += value;
							//		data(year)[month][day].SetData(H_RELH, stat);
							//	}
							//	break;
						//case -1:
						//case SNOW:
						//case EVAP:
						//case MNPN:
						//case MXPN:
						//case PGTM:
						//case TOBS:
						//case WDMV:
						//case FMTM: break;
						//default: ASSERT(false);
						}
					}//end if flag
					else
					{
						int i;
						i = 0;
						//return msg;
					}//data is valid
				}//day is valid
			}//valid stations

			msg += callback.SetCurrentStepPos((size_t)file.tellg());
		}// for all lines

		callback.PopTask();
	}

	return msg;
}


}