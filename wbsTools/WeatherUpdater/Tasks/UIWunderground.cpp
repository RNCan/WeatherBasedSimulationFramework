#include "StdAfx.h"
#include "UIWunderground.h"

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{
	//to update data in csv file
	//https://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=IQUEBECS44&day=1&month=1&year=2015&dayend=31&monthend=12&yearend=2015&graphspan=custom&format=1
	
	

	//pour la liste des stations actives:
	//http://www.wunderground.com/weatherstation/ListStations.asp?showall=&start=20&selectedState=
	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIWunderground::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Type", "UpdateStationList" };
	const size_t CUIWunderground::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_COMBO_INDEX, T_BOOL };
	const UINT CUIWunderground::ATTRIBUTE_TITLE_ID = IDS_UPDATER_BC_P;
	const UINT CUIWunderground::DESCRIPTION_TITLE_ID = ID_TASK_BC;

	const char* CUIWunderground::CLASS_NAME(){ static const char* THE_CLASS_NAME = "WeatherUnderground";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIWunderground::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIWunderground::CLASS_NAME(), (createF)CUIWunderground::create);


	const char* CUIWunderground::SERVER_NAME = "www.wunderground.com";


	static const char pageFormat1[] = "weatherstation/ListStations.asp?showall=&start=%s&selectedState=%s";
	static const char pageFormat2[] = "weatherstation/ListStations.asp?selectedCountry=%s";
	static const short SEL_ROW_PER_PAGE = 40;

	static const char pageDataFormat[] =
	{
		"weatherstation/WXDailyHistory.asp?"
		"ID=%s&"
		"graphspan=custom&"
		"month=%d&"
		"Day=%d"
		"Year=%d&"
		"monthEnd=%d&"
		"DayEnd=%d"
		"YearEnd=%d&"
		"format=1"
	};





	CUIWunderground::CUIWunderground(void)
	{}


	CUIWunderground::~CUIWunderground(void)
	{}


	std::string CUIWunderground::Option(size_t i)const
	{
		string str;
	/*	switch (i)
		{
		case DATA_TYPE:	str = GetString(IDS_STR_WDATA_TYPE); break;
		};*/
		return str;
	}

	std::string CUIWunderground::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "1"; break;
		case UPDATE_STATION_LIST:	str = "1"; break;
		};
		return str;
	}

	std::string CUIWunderground::GetOutputFilePath(const std::string& ID, int year)
	{
		string workingDir = GetDir(WORKING_DIR);
		string ouputPath = workingDir + "\\" + ToString(year) + "\\";
		return ouputPath + ID + ".csv";
	}

	ERMsg CUIWunderground::DownloadStationList(CLocationVector& stationList, CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		//callback.PushTask(GetString(IDS_LOAD_STATION_LIST), m_selection.GetNbSelection());

		////loop on province
		//for (int i = 0; i < CWUCountrySelection::NB_COUNTRY&&msg; i++)
		//{
		//	if (!m_selection.IsUsed(i))
		//		continue;

		//	string countryName = m_selection.GetName(i);
		//	replace(countryName.begin(), countryName.end(), ' ', '+');
		//	string pageURL = FormatA(pageFormat2, countryName.c_str());

		//	msg = GetStationListPage(pConnection, pageURL, stationList);
		//	msg += callback.StepIt();
		//}

		pConnection->Close();
		pSession->Close();

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(stationList.size()));

		callback.PopTask();


		return msg;
	}


	//******************************************************
	ERMsg CUIWunderground::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		if (as<bool>(UPDATE_STATION_LIST))
		{
			//UpdateStationList(pConnection, callback);
		}

		//Get station
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		size_t nbYears = lastYear - firstYear + 1;

		//callback.AddMessage(GetString(IDS_UPDATE_FILE));
		int nbDownload = 0;


		//callback.SetNbStep(1);
		//Get the stations list
		//CLocationVector stationList;
		//msg = GetStationList(stationList, callback);
		StringVector stationList;
		stationList.push_back("IQUEBECS44");

		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			//create LOC from station list
			CLocationVector loc;
			for (size_t i = 0; i < stationList.size(); i++)
			{
				static const char* URL_FORMAT = "weatherstation/WXDailyHistory.asp?ID=%s&day=1&month=1&year=%d&dayend=31&monthend=12&yearend=%d&graphspan=custom&format=1";
				string URL = FormatA(URL_FORMAT, stationList[i].c_str(), year, year);
				string ouputPath = workingDir + "\\" + ToString(year) + "\\";// +NETWORK_NAME[n] + "\\";
				CreateMultipleDir(ouputPath);

				string source;
				msg = GetPageText(pConnection, URL, source, false, FLAGS);

				if (!source.empty())
				{
					ofStream file;
					msg = file.open(ouputPath + stationList[i] + ".csv");
					if (msg)
					{
						file << source;
						file.close();
					}
				}
			}
		}
//		msg += loc.Save("d:\\travail\\WUnderground\\allStation.csv");

		callback.PopTask();
		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 2);


		pConnection->Close();
		pSession->Close();


		return msg;
	}


	ERMsg CUIWunderground::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		stationList.push_back("IQUEBECS44");

		return msg;
	}
	

	ERMsg CUIWunderground::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t pos = m_stations.FindByID(ID);
		if (pos == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}


		((CLocation&)station) = m_stations[pos];

		station.m_name = station.m_name;
		station.m_ID;// += "H";//add a "H" for hourly data

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = size_t(lastYear - firstYear + 1);
		station.CreateYears(firstYear, nbYears);

		//if (nbYears > 10)
		//callback.PushTask();

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				string filePath = GetOutputFilePath(ID, year);
				if (FileExists(filePath))
				{
					CWeatherYears data;
					msg = data.LoadData(filePath);
					if (msg)
						station[year][m] = data[year][m];

					msg += callback.StepIt(0);
				}
			}
		}

		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
			{
				msg = station.IsValid();
			}
		}

		
		return msg;
	}

	ERMsg CUIWunderground::ReadData(const string& filePath, CTM TM, CWeatherStation& data, CCallback& callback)const
	{
		ERMsg msg;

		string path = GetPath(filePath);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		//now extact data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{/*
				ASSERT(timePos != NOT_INIT);
				string dateTimeStr = (*loop)[timePos];
				StringVector dateTime(dateTimeStr, " -:");

				int year = stoi(dateTime[0]);
				size_t month = stoi(dateTime[1]) - 1;
				size_t day = stoi(dateTime[2]) - 1;
				int hour = stoi(dateTime[3]);

				ASSERT(year >= firstYear - 1 && year <= lastYear);
				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				ASSERT(hour >= 0 && hour < 24);


				CTRef TRef = CTRef(year, month, day, hour, TM);

				if (stat.TRefIsChanging(TRef))
					data[stat.GetTRef()].SetData(stat);

				double Tmin = -DBL_MAX;
				double Tmax = -DBL_MAX;
				for (size_t c = 0; c<loop->size(); c++)
				{
					if (col_pos[c] != var_map.end())
					{
						string str = TrimConst((*loop)[c]);
						if (str != "None")
						{
							double value = stod(str);
							value = Normalize(value, col_pos[c]->first, col_pos[c]->second.units, data.m_z);

							if (col_pos[c]->second.v < H_ADD1)
								stat.Add(TRef, col_pos[c]->second.v, value);
							else if (col_pos[c]->second.v == H_ADD1)
								Tmin = value;
							else if (col_pos[c]->second.v == H_ADD2)
								Tmax = value;
						}
					}
				}

				if (Tmin != -DBL_MAX && Tmax != -DBL_MAX)
				{
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);

					stat.Add(TRef, H_TAIR, (Tmin + Tmax) / 2);
					stat.Add(TRef, H_TRNG, Tmax - Tmin);
				}
*/
				msg += callback.StepIt(0);
			}//for all line


			//if (stat.GetTRef().IsInit())
				//data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}




	//using namespace CFL;
	////using namespace DAILY_DATA;
	//using namespace UtilWWW;
	//using namespace std; using namespace stdString; using namespace CFL;
	//
	//const string CWUCountrySelection::COUNTRAY_NAME[NB_TAG]=
	//{
	//  "Afghanistan", 
	//  "Albania", 
	//  "Alberta", 
	//  "Algeria", 
	//  "Angola", 
	//  "Anguilla", 
	//  "Argentina", 
	//  "Armenia", 
	//  "Australia", 
	//  "Austria", 
	//  "Azerbaijan", 
	//  "Bahamas", 
	//  "Bahrain", 
	//  "Bangladesh", 
	//  "Barbados", 
	//  "Beijing", 
	//  "Belarus", 
	//  "Belgium", 
	//  "Belize", 
	//  "Benin", 
	//  "Bermuda", 
	//  "Bolivia", 
	//  "Bosnia", 
	//  "Botswana", 
	//  "Bouvet Island", 
	//  "Brazil", 
	//  "British Columbia", 
	//  "British Indian Ocean", 
	//  "British Virgin Islands", 
	//  "Brunei", 
	//  "Bulgaria", 
	//  "Burkina Faso", 
	//  "Burma/Myanmar", 
	//  "Burundi", 
	//  "Cambodia", 
	//  "Cameroon", 
	//  "Canada", 
	//  "Canary Islands", 
	//  "Canton Island", 
	//  "Cape Verde", 
	//  "Capital Territory", 
	//  "Caroline Islands", 
	//  "Cayman Island", 
	//  "Central African Republic", 
	//  "Chad", 
	//  "Cheng-Du", 
	//  "Chile", 
	//  "China", 
	//  "Colombia", 
	//  "Comoros", 
	//  "Congo", 
	//  "Cook Islands", 
	//  "Costa Rica", 
	//  "Croatia", 
	//  "Cuba", 
	//  "Cyprus", 
	//  "Czech Republic", 
	//  "Democratic Yemen", 
	//  "Denmark", 
	//  "Djibouti", 
	//  "Dominica", 
	//  "Dominican Republic", 
	//  "East Timor", 
	//  "Ecuador", 
	//  "Egypt", 
	//  "El Salvador", 
	//  "Equatorial Guinea", 
	//  "Eritrea", 
	//  "Estonia", 
	//  "Ethiopia", 
	//  "Falkland Islands", 
	//  "Fiji", 
	//  "Finland", 
	//  "France", 
	//  "French Guiana", 
	//  "French Polynesia", 
	//  "Gabon", 
	//  "Gambia", 
	//  "Germany", 
	//  "Ghana", 
	//  "Gibraltar", 
	//  "Greece", 
	//  "Greenland", 
	//  "Grenada", 
	//  "Guam", 
	//  "Guang-Zhou", 
	//  "Guatemala", 
	//  "Guinea", 
	//  "Guinea-Bissau", 
	//  "Guyana", 
	//  "Haiti", 
	//  "Han-Kou", 
	//  "Hawaii", 
	//  "Honduras", 
	//  "Hong Kong", 
	//  "Hungary", 
	//  "Iceland", 
	//  "India", 
	//  "Indonesia", 
	//  "Iran", 
	//  "Iraq", 
	//  "Ireland", 
	//  "Israel", 
	//  "Italy", 
	//  "Ivory Coast", 
	//  "Jamaica", 
	//  "Japan", 
	//  "Jordan", 
	//  "Kazakhstan", 
	//  "Kenya", 
	//  "Kiribati", 
	//  "Kuwait", 
	//  "Kyrgyzstan", 
	//  "Lan-Zhou", 
	//  "Lao Peoples Republic", 
	//  "Latvia", 
	//  "Lebanon", 
	//  "Lesotho", 
	//  "Liberia", 
	//  "Libya", 
	//  "Luxembourg", 
	//  "Macao", 
	//  "Macedonia", 
	//  "Madagascar", 
	//  "Madeira Islands", 
	//  "Malawi", 
	//  "Malaysia", 
	//  "Maldives", 
	//  "Mali", 
	//  "Malta", 
	//  "Manitoba", 
	//  "Mariana Islands", 
	//  "Marshall Islands", 
	//  "Martinique", 
	//  "Maryland", 
	//  "Mauritania", 
	//  "Mauritius", 
	//  "Mexico", 
	//  "Mongolia", 
	//  "Montana", 
	//  "Morocco", 
	//  "Mozambique", 
	//  "Namibia", 
	//  "Nauru", 
	//  "Nepal", 
	//  "Netherlands", 
	//  "New Brunswick", 
	//  "New Caledonia", 
	//  "New South Wales", 
	//  "New Zealand", 
	//  "Newfoundland", 
	//  "Nicaragua", 
	//  "Niger", 
	//  "Nigeria", 
	//  "North Korea", 
	//  "North Pacific Islands", 
	//  "Norway", 
	//  "Nova Scotia", 
	//  "Oman", 
	//  "Pakistan", 
	//  "Panama", 
	//  "Papua New Guinea", 
	//  "Paraguay", 
	//  "Peru", 
	//  "Philippines", 
	//  "Poland", 
	//  "Portugal", 
	//  "Prince Edward Island", 
	//  "Puerto Rico", 
	//  "Qatar", 
	//  "Republic of Moldova", 
	//  "Reunion Island", 
	//  "Romania", 
	//  "Russia", 
	//  "Rwanda", 
	//  "Saudi Arabia", 
	//  "Senegal", 
	//  "Seychelles", 
	//  "Shang-Hai", 
	//  "Shen-Yang", 
	//  "Sierra Leone", 
	//  "Singapore", 
	//  "Slovakia", 
	//  "Slovenia", 
	//  "Solomon Islands", 
	//  "Somalia", 
	//  "South Africa", 
	//  "South Korea", 
	//  "Southern Line Islands", 
	//  "Spain", 
	//  "Sri Lanka", 
	//  "Sudan", 
	//  "Suriname", 
	//  "Swaziland", 
	//  "Sweden", 
	//  "Switzerland", 
	//  "Syria", 
	//  "Taiwan", 
	//  "Tajikistan", 
	//  "Tanzania", 
	//  "Tasmania", 
	//  "Thailand", 
	//  "Togo", 
	//  "Tokelau Island", 
	//  "Tonga", 
	//  "Trinidad And Tobago", 
	//  "Tunisia", 
	//  "Turkey", 
	//  "Turks Islands", 
	//  "Tuvalu", 
	//  "United States", 
	//  "Uganda", 
	//  "Ukraine", 
	//  "United Arab Emirates", 
	//  "United Kingdom", 
	//  "Uruguay", 
	//  "Urum-Qui", 
	//  "Uzbekistan", 
	//  "Vanuata", 
	//  "Venezuela", 
	//  "Victoria", 
	//  "Viet Nam", 
	//  "Virgin Islands", 
	//  "Virginia", 
	//  "Wake Island", 
	//  "Wallis And Futuna Island", 
	//  "Western Sahara", 
	//  "Western Samoa", 
	//  "Yemen", 
	//  "Yugoslavia", 
	//  "Yukon Territory", 
	//  "Zambia", 
	//  "Zimbabwe",
	//  "All"
	//};
	//
	//CWUCountrySelection::CWUCountrySelection()
	//{
	//	Reset();
	//}
	//
	//string CWUCountrySelection::GetName(short i)
	//{
	//	ASSERT( i>= 0 && i<NB_COUNTRY);
	//	return COUNTRAY_NAME[i];
	//}
	//
	//string CWUCountrySelection::ToString()const
	//{
	//	string str;
	//	if( IsUsedAll() )
	//	{
	//		str = COUNTRAY_NAME[ALL];
	//	}
	//	else
	//	{
	//		for(int i=0; i<NB_COUNTRY; i++)
	//		{
	//			if( IsUsed(i) )
	//			{
	//				str += COUNTRAY_NAME[i];
	//				str += ';';
	//			}
	//		}
	//	}
	//	return str;
	//}
	//
	//ERMsg CWUCountrySelection::FromString(const string& in)
	//{
	//	ERMsg msg;
	//
	//	Reset();
	//
	//	string::size_type start=0;
	//	string tmp = Tokenize(in, ";", start);
	//	while(!tmp.empty() )
	//	{
	//		msg+=SetUsed(tmp);
	//		tmp = Tokenize(in, ";", start);
	//	}
	//
	//	return msg;
	//}
	//
	//short CWUCountrySelection::GetCountry(const string& in)//by abr
	//{
	//	short country = -1;
	//	string tmp(in);
	//	if(tmp.length() != 2)
	//		Trim(tmp);
	//
	//	MakeUpper(tmp);
	//	for(int i=0; i<NB_TAG; i++)
	//	{
	//		if (IsEqualNoCase(tmp, COUNTRAY_NAME[i]))
	//		{
	//			country = i;
	//			break;
	//		}
	//	}
	//
	//	return country;
	//}
	//


}