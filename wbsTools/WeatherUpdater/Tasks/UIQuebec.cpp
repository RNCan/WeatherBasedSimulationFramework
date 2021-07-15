#include "StdAfx.h"
#include "UIQuebec.h" 

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "Geomatic/TimeZones.h" 
#include "mosa.h" 

#include "basic/zenXml.h"
#include "Basic/WeatherStation.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/Utilstd.h"


using namespace WBSF::HOURLY_DATA;
using namespace Mosa;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{

	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIQuebec::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", "DataType", "UpdateUntil", "UpdateStationsList", "UserNameSOPFEU", "PasswordSOPFEU", "UserNameMFFP", "PasswordMFFP" };
	const size_t CUIQuebec::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX, T_STRING, T_BOOL, T_STRING, T_PASSWORD, T_STRING, T_PASSWORD };
	const UINT CUIQuebec::ATTRIBUTE_TITLE_ID = IDS_UPDATER_QUEBEC_P;
	const UINT CUIQuebec::DESCRIPTION_TITLE_ID = ID_TASK_QUEBEC;

	const char* CUIQuebec::CLASS_NAME() { static const char* THE_CLASS_NAME = "Quebec";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIQuebec::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIQuebec::CLASS_NAME(), (createF)CUIQuebec::create);

	const std::array<const char*, CUIQuebec::NB_NETWORKS> CUIQuebec::PROVIDER = { { "SOPFEU", "Québec", "SolutionMesonet", "SolutionMesonet", "SolutionMesonet", "SolutionMesonet", "SolutionMesonet" } };
	const std::array<const char*, CUIQuebec::NB_NETWORKS> CUIQuebec::SERVER_NAME = { { "FTP3.sopfeu.qc.ca", "www.mddelcc.gouv.qc.ca", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org" } };
	const std::array<const char*, CUIQuebec::NB_NETWORKS> CUIQuebec::NETWORK_NAME = { { "SOPFEU", "MDDELCC", "HYDRO", "MFFP", "ALCAN", "FADQ", "SM" } };


	std::bitset<CUIQuebec::NB_NETWORKS> CUIQuebec::GetNetwork()const
	{
		StringVector networkV(Get(NETWORK), "|;,");
		bitset<NB_NETWORKS> networks;

		for (size_t n = 0; n < NB_NETWORKS; n++)
			if (networkV.empty() || networkV.Find(ToString(NETWORK_NAME[n])) != NOT_INIT)
				networks.set(n);

		return networks;
	}




	CUIQuebec::CUIQuebec(void)
	{

	}

	CUIQuebec::~CUIQuebec(void)
	{}


	std::string CUIQuebec::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK: str = "SOPFEU=SOPFEU|MDDELCC=MDDELCC (daily)|HYDRO=Hydro-Quebec|MFFP=MFFP|ALCAN=Alcan|FADQ=Financière Agricole|SM=Solution-Mesonet"; break;
		case DATA_TYPE: str = "Hourly|Daily"; break;
		};

		return str;
	}

	std::string CUIQuebec::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Quebec\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_UNTIL: str = "7"; break;
		case UPDATE_STATIONS_LIST: str = "0"; break;
		};

		return str;
	}

	//*************************************************************************************************

	ERMsg CUIQuebec::Execute(CCallback& callback)
	{
		//return CreateWeatherStationQuebec(callback);


		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		bitset<NB_NETWORKS> network = GetNetwork();
		size_t dataType = as<size_t>(DATA_TYPE);

		callback.PushTask(GetString(IDS_UPDATE_FILE) + " Quebec " + (dataType == HOURLY_WEATHER ? "hourly" : "daily") + " (" + to_string(network.count()) + ")", network.count());

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case SOPFEU:  InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.Execute(callback); break;
				case MDDELCC: InitMDDELCC(m_MDDELCC); msg += m_MDDELCC.Execute(callback); break;
				case MFFP:
				{
					if (InitMFFP(m_MFFP))
					{
						msg += m_MFFP.Execute(callback);
					}
					else
					{
						Init(n); msg += Execute(n, callback);
					}
					break;
				}
				default: Init(n); msg += Execute(n, callback);
				}

				msg += callback.StepIt();
			}

			if (callback.GetUserCancel())
				break;
		}

		callback.PopTask();
		return msg;
	}


	ERMsg CUIQuebec::GetStationList(size_t n, StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string filePath = GetApplicationPath() + "Layers\\QuebecStations.csv";
		msg = m_stations.Load(filePath);

		for (size_t i = 0; i < m_stations.size(); i++)
		{
			if (m_stations[i].GetSSI("Network") == NETWORK_NAME[n])
			{
				stationList.push_back(m_stations[i].m_ID);
			}
		}

		return msg;
	}

	ERMsg CUIQuebec::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		bitset<NB_NETWORKS> network = GetNetwork();
		size_t dataType = as<size_t>(DATA_TYPE);

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				StringVector station;

				switch (n)
				{
				case SOPFEU: InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.GetStationList(station, callback); break;
				case MDDELCC:InitMDDELCC(m_MDDELCC);  msg += m_MDDELCC.GetStationList(station, callback); break;
				case MFFP:    if (InitMFFP(m_MFFP)) { msg += m_MFFP.GetStationList(station, callback); break; }
				default: Init(n); msg += GetStationList(n, station, callback); break;
				}

				if (msg)
				{
					stationList.reserve(stationList.size() + station.size());
					for (StringVector::const_iterator it = station.begin(); it != station.end(); it++)
						stationList.push_back(ToString(n) + *it);
				}
			}
		}


		return msg;
	}


	ERMsg CUIQuebec::Execute(size_t n, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		size_t dataType = as<size_t>(DATA_TYPE);
		if (dataType == HOURLY_WEATHER)
		{
			callback.AddMessage(GetString(IDS_UPDATE_DIR));
			callback.AddMessage(workingDir, 1);
			callback.AddMessage("");

			string userName = Get(USER_NAME_MFFP);
			string password = Get(PASSWORD_MFFP);
			int firstYear = as<int>(FIRST_YEAR);
			int lastYear = as<int>(LAST_YEAR);
			int updateUntil = as<int>(UPDATE_UNTIL);
			msg = ExecuteFTP(workingDir, n, userName, password, firstYear, lastYear, updateUntil, callback);
		}

		return msg;
	}

	string CUIQuebec::GetWorkingDir(size_t n)const
	{
		return GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\";
	}

	string CUIQuebec::GetOutputFilePath(size_t n, string id, int year)const
	{
		return GetWorkingDir(n) + ToString(year) + "\\" + id + ".csv";
	}

	void CUIQuebec::Init(size_t n)
	{
		m_firstYear = as<int>(FIRST_YEAR);
		m_lastYear = as<int>(LAST_YEAR);


	}

	void CUIQuebec::InitSOPFEU(CSOPFEU& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "SOPFEU\\";
		obj.m_userName = Get(USER_NAME_SOPFEU);
		obj.m_password = Get(PASSWORD_SOPFEU);
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);
	}

	void CUIQuebec::InitMDDELCC(CMDDELCC& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "MDDELCC\\";
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);
		obj.bForceUpdateList = as<bool>(UPDATE_STATIONS_LIST);
		obj.m_updateUntil = as<int>(UPDATE_UNTIL);
	}

	bool CUIQuebec::InitMFFP(CMFFP& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "MFFP\\";
		obj.m_userName = Get(USER_NAME_MFFP);
		obj.m_password = Get(PASSWORD_MFFP);
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		return obj.m_userName != "MFFP" ? GetFtpConnection(SERVER_NAME[MFFP], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, obj.m_userName, obj.m_password, true, 2) : false;
	}

	ERMsg CUIQuebec::GetWeatherStation(const string& name, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		size_t n = WBSF::as<size_t>(name.substr(0, 1));
		string ID = name.substr(1);



		switch (n)
		{
		case SOPFEU: msg += m_SOPFEU.GetWeatherStation(ID, TM, station, callback); break;
		case MDDELCC:  msg += m_MDDELCC.GetWeatherStation(ID, TM, station, callback); break;
		case MFFP:    if (InitMFFP(m_MFFP)) { msg += m_MFFP.GetWeatherStation(ID, TM, station, callback); break; }
		default:  msg += GetWeatherStation(n, ID, TM, station, callback); break;
		}

		station.SetSSI("Provider", PROVIDER[n]);


		return msg;
	}

	ERMsg CUIQuebec::GetWeatherStation(size_t n, const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		//string workingDir = GetWorkingDir(n);

		size_t pos = m_stations.FindByID(ID);
		ASSERT(pos != NOT_INIT);

		((CLocation&)station) = m_stations[pos];
		station.m_name = PurgeFileName(station.m_name);

		station.SetHourly(true);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

			msg += callback.StepIt(0);
		}

		station.CleanUnusedVariable("TN T TX P TD H WS WD R Z W2");

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}



		/*string network = station.GetSSI("Network");
		string country = station.GetSSI("Country");
		string subDivisions = station.GetSSI("SubDivision");
		station.m_siteSpeceficInformation.clear();
		station.SetSSI("Network", network);
		station.SetSSI("Country", country);
		station.SetSSI("SubDivision", subDivisions);
*/

		return msg;
	}





	ERMsg CUIQuebec::CreateWeatherStationQuebec(CCallback& callback)
	{
		ERMsg msg;

		string working_dir = "G:\\Hydro-Québec\\";

		CLocationVector locations;
		msg += locations.Load(working_dir + "Liste.csv");
		CLocationVector locations2;


		static const char* VAR_DIR_H[8] = { "Températures minimales horaires", "", "Températures maximales horaires","Précipitations horaires","","Humidité relative horaires","Vitesse vent horaires","Direction vent horaires" };
		static const char* VAR_DIR_D[8] = { "Températures minimales journalières", "", "Températures maximales journalières","Précipitations journalières","","","","" };

		string outputFilePathD = working_dir + "CoteNord2.DailyDB";
		string outputFilePathH = working_dir + "CoteNord2.HourlyDB";


		size_t t = 0;

		CDailyDatabase dbD;
		CHourlyDatabase dbH;

		if (t == 0)
		{

			msg += CHourlyDatabase::DeleteDatabase(outputFilePathH, callback);
			msg += dbH.Open(outputFilePathH, CHourlyDatabase::modeWrite, callback);
		}
		else
		{
			msg += CDailyDatabase::DeleteDatabase(outputFilePathD, callback);
			msg += dbD.Open(outputFilePathD, CDailyDatabase::modeWrite, callback);
		}



		std::map<string, CWeatherStation> data;

		callback.PushTask("Create database", 9);

		for (size_t v = 0; v < 8; v++)
		{


			CTM TM((t == 0) ? CTM::HOURLY : CTM::DAILY);
			string type = (t == 0) ? VAR_DIR_H[v] : VAR_DIR_D[v];
			if (!type.empty())
			{
				ASSERT(DirectoryExists(working_dir + "Donnees_meteo\\" + type + "\\"));
				string filter = working_dir + "Donnees_meteo\\" + type + "\\*.csv";
				StringVector list = WBSF::GetFilesList(filter);
				callback.PushTask(string("Add var ") + type, list.size());

				for (size_t f = 0; f < list.size(); f++)
				{
					CWeatherAccumulator stat(TM);

					ifStream file;
					msg = file.open(list[f]);
					if (msg)
					{
						string ID;
						StringVector lines;
						StringVector columns;
						CLocation coord;
						//for (size_t i = 0; i < 27; i++)
						while (columns.empty() || columns[0] != "Dateheure")
						{
							string line;
							std::getline(file, line);

							columns.Tokenize(line, ";", false);

							if (!columns.empty())
							{
								if (columns[0] == "DSHÉ")
								{
									ID = columns[1];
									coord.m_ID = columns[1];
								}
								else if (columns[0] == "Nom")
								{
									coord.m_name = columns[1];
								}
								else if (columns[0].substr(0, 6) == "XCOORD")
								{
									coord.m_lon = ToDouble(columns[1]);
								}
								else if (columns[0].substr(0, 6) == "YCOORD")
								{
									coord.m_lat = ToDouble(columns[1]);
								}
								else if (columns[0].substr(0, 6) == "ZCOORD")
								{
									if (!columns[1].empty())
										coord.m_elev = ToDouble(columns[1]);
								}
								else if (columns[0] == "Ouverture")
								{
									coord.SetSSI("debut", columns[1]);
								}
								else if (columns[0] == "Fermeture")
								{
									coord.SetSSI("fin", columns[1]);
								}
								else if (columns[0] == "Type")
								{
									coord.SetSSI("type", columns[1]);
								}
								else if (columns[0] == "Proprio")
								{
									lines.push_back(line);
									std::getline(file, line);
									columns.Tokenize(line, ";", false);
									coord.SetSSI("proprio", columns[0]);
								}
								else if (columns[0] == "SMC")
								{
									coord.SetSSI("SMC", columns[1]);
								}
								else if (columns[0] == "RMCQ")
								{
									coord.SetSSI("RMCQ", columns[1]);
								}

							}
							lines.push_back(line);
						}

						if (locations2.FindByID(coord.m_ID) == NOT_INIT)
							locations2.push_back(coord);

						ASSERT(!ID.empty());
						size_t loc_pos = locations.FindByID(ID);

						if (loc_pos == NOT_INIT)
						{
							auto it = locations.FindBySSI("SMC", coord.GetSSI("SMC"), false);
							if (it != locations.end())
								loc_pos = std::distance(locations.begin(), it);
						}

						if (loc_pos == NOT_INIT)
						{
							auto it = locations.FindBySSI("RMCQ", coord.GetSSI("RMCQ"), false);
							if (it != locations.end())
								loc_pos = std::distance(locations.begin(), it);
						}

						ASSERT(loc_pos < locations.size());

						ID = locations[loc_pos].m_ID;
						if (ID.empty())
							callback.AddMessage(list[f]);

						for (CSVIterator loop(file, ";", false); loop != CSVIterator() && msg; ++loop)
						{
							if (!loop->empty())
							{
								enum TColName { C_DATEHEURE, C_VALEUR, C_QUALITE, C_STATUT, NB_COLUMNS };
								ASSERT((*loop).size() == NB_COLUMNS);

								StringVector time((*loop)[C_DATEHEURE], "-: T");
								ASSERT(time.size() == 5);

								int year = ToInt(time[0]);
								size_t month = ToInt(time[1]) - 1;
								size_t day = ToInt(time[2]) - 1;
								size_t hour = ToInt(time[3]);


								ASSERT(month >= 0 && month < 12);
								ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
								ASSERT(hour >= 0 && hour < 24);



								if (data.find(ID) == data.end())
								{
									data[ID] = CWeatherStation(TM.Type() == CTM::HOURLY);

									ASSERT(loc_pos != NOT_INIT);
									((CLocation&)(data[ID])) = locations[loc_pos];
								}

								CTM TMtmp = t == 0 ? CTM::HOURLY : CTM::DAILY;
								CTRef UTCTRef = CTRef(year, month, day, hour, TMtmp);
								CTRef TRef = t == 0 ? CTimeZones::UTCTRef2LocalTRef(UTCTRef, data[ID]) : UTCTRef;

								if (stat.TRefIsChanging(TRef))
								{
									data[ID][stat.GetTRef()].SetStat((TVarH)v, stat.GetStat(v));
								}

								string tmp = (*loop)[C_VALEUR];
								if (!tmp.empty())
								{
									ReplaceString(tmp, ",", ".");
									bool bValid = true;
									double value = ToDouble(tmp);
									//eliminate all suspect values
									switch (v)
									{
									case H_TMIN:
									case H_TAIR:
									case H_TMAX:
									case H_TDEW: bValid = value > -60 && value < 60; break;
									case H_PRCP: bValid = value >= 0 && value < 150; break;
									case H_RELH:
										if (value > 100 && value < 105)//let valid humidity up to 105%
											value = 100;
										bValid = value > 1 && value <= 100; break;
									case H_WNDS: bValid = value >= 0 && value < 80; break;
									case H_WNDD: bValid = value >= 0 && value <= 360; break;
									}

									if (bValid)
									{
										stat.Add(TRef, v, value);

										if (TM == CTM::HOURLY)
										{
											if (v == H_TMAX)
											{
												double Tmin = data[ID].GetHour(TRef).at(H_TMIN);
												if (Tmin > -999)
													data[ID][TRef].SetStat(H_TAIR, (Tmin + value) / 2);
											}

											if (v == H_RELH)
											{
												double T = data[ID].GetHour(TRef).at(H_TAIR);
												if (T > -999)
												{
													data[ID][TRef].SetStat(H_TDEW, Hr2Td(T, value));
												}
											}

										}//Hourly
										else
										{
											if (v == H_RELH)
											{
												double Tmin = data[ID].GetHour(TRef).at(H_TMIN);
												double Tmax = data[ID].GetHour(TRef).at(H_TMAX);
												if (Tmin > -999 && Tmax > -999)
												{
													//approximation of Tdew
													data[ID][TRef].SetStat(H_TDEW, Hr2Td((Tmin + Tmax) / 2, value));
												}
											}
										}
									}//is valid
								}//empty
							}//no empty line

							//msg += callback.StepIt(loop->GetLastLine().length() + 2);
							msg += callback.StepIt(0);
						}//for all line (


						if (stat.GetTRef().IsInit() && data.find(ID) != data.end())
							data[ID][stat.GetTRef()].SetStat((TVarH)v, stat.GetStat(v));
						//data[ID][stat.GetTRef()].SetData(stat);


					}//if msg

					msg += callback.StepIt();
				}//fo all files

				callback.PopTask();
				msg += callback.StepIt();
			}//if valid var


		}//for all variables


		if (msg)
		{
			//save data
			for (auto it1 = data.begin(); it1 != data.end(); it1++)
			{
				if (it1->second.m_name.empty())
				{
					callback.AddMessage("empty station: " + ToString(distance(data.begin(), it1)));
				}

				CWVariables vars;
				vars.set();

				CWVariablesCounter  couter = it1->second.GetVariablesCount();
				for (size_t v = 0; v < 8; v++)
				{
					if (couter[v].first < 10)
						vars.reset(v);
				}


				it1->second.CleanUnusedVariable(vars);


				//
				if (t == 0)
					dbH.Add(it1->second);
				else
					dbD.Add(it1->second);
			}
		}//if msg


		//
		if (t == 0)
			dbH.Close(true, callback);
		else
			dbD.Close(true, callback);

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(data.size()), 1);
		callback.PopTask();


		msg += locations2.Save(working_dir + "Liste_generate.csv");


		return msg;
	}
}

