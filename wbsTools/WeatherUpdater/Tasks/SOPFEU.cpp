//usager : MeteoR
//PW: LeSoleilBrille

#include "StdAfx.h"
#include "SOPFEU.h"


#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"

#include "../Resource.h"


//FE	Émetteur Sopfeu
//Identifiant	Station
//Date
//Heure	Temps universel coordonné(ou UTC)

//00. TAi000H	Température horaire(°C)
//01. TAn000H	Température minimale horaire(°C)
//02. TAx000H	Température maximale horaire(°C)
//03. TAm000H	Température moyenne horaire(°C)
//04. PC040H	Précipitation liquide horaire cumulée(mm) : se remet à 0.0 au jour julien 90
//05. PT040H	Précipitation liquide horaire(mm)
//06. PC020H	Précipitation totale horaire cumulée(mm)
//07. PT041H	Poids total en mm
//08. HAi000H	Humidité relative horaire(%)
//09. HAn000H	Humidité relative minimale horaire(%)
//10. HAx000H	Humidité relative maximale horaire(%)
//11. NSi000H	Épaisseur de la neige au sol horaire(cm)
//12. VDi300H	Direction du vent instantanée horaire(°degrés)
//13. VVi300H	Vitesse du vent instantanée horaire(km/h)
//14. VVxi500H	Vitesse de la pointe de vent horaire(km/h)
//15. VDm025B	Direction du vent instantanée horaire à 2,5 mètres(°degrés)
//16. VVm025B	Vitesse du vent instantanée horaire à 2,5 mètres(km / h)
//17. TDi000H	Température du point de rosée horaire(°C)
//18. VB000B	Voltage batterie
//

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	const char* CSOPFEU::SERVER_NAME = "FTP3.sopfeu.qc.ca";
	const char* CSOPFEU::SERVER_PATH = "RMCQ/";

	const char * CSOPFEU::FIELDS_NAME[NB_FIELDS] =
	{ "TAI000H", "TAN000H", "TAX000H", "TAM000H", "PC040H", "PT040H", "PC020H", "PT041H", "HAI000H", "HAN000H", "HAX000H", "NSI000H", "VDI300H", "VVI300H", "VVXI500H", "VDM025B", "VVM025B", "TDI000H", "VB000B" };

	//do not use TAI000H nor TAM000H. 
	const TVarH CSOPFEU::VARIABLE[NB_FIELDS] = { H_SKIP, H_TMIN, H_TMAX, H_SKIP, H_SKIP, H_SKIP, H_ADD1, H_ADD2, H_RELH, H_RELH, H_RELH, H_SNDH, H_WNDD, H_WNDS, H_SKIP, H_SKIP, H_WND2, H_TDEW, H_SKIP };

	TVarH CSOPFEU::GetVariable(std::string str)
	{
		str = MakeUpper(str);
		TVarH v = H_SKIP;
		for (size_t i = 0; i < NB_FIELDS&&v == NOT_INIT; i++)
			if (str == FIELDS_NAME[i])
				v = VARIABLE[i];

		return v;
	}

	//*********************************************************************

	CSOPFEU::CSOPFEU(void)
	{
		m_firstYear = 0;
		m_lastYear = 0;
	}

	CSOPFEU::~CSOPFEU(void)
	{}

	//*****************************************************************************

	string CSOPFEU::GetStationListFilePath()const
	{
		return GetApplicationPath() + "Layers\\QuebecStations.csv";
	}

	string CSOPFEU::GetOutputFilePath(CTRef TRef)const
	{
		return WBSF::FormatA("%s%d/m%4d-%02d-%02d-%02d.csv", m_workingDir.c_str(), TRef.GetYear(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour());
	}

	ERMsg CSOPFEU::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		//size_t nbYears = m_lastYear - m_firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), 1);
		//callback.SetNbStep(nbYears);
		bool bLoaded = false;
		size_t nbTry = 0;

		CFileInfoVector dirList;
		while (!bLoaded && msg)
		{
			nbTry++;

			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;


			msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, m_userName, m_password, true, 5, callback);
			if (msg)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 35000);


				try
				{
					msg = FindFiles(pConnection, string(SERVER_PATH) + "*.*", fileList, true, callback);
					if (msg)
					{
						msg += callback.StepIt();
						bLoaded = true;
						nbTry = 0;
					}
				}
				catch (CException* e)
				{
					//if an error occur: try again
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg = WaitServer(5, callback);

					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}

				pConnection->Close();
				pSession->Close();

			}
		}


		callback.PopTask();

		//remove unwanted file
		if (msg)
		{
			if (!bLoaded)
				callback.AddMessage(GetString(IDS_SERVER_BUSY));

			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()));
			msg = CleanList(fileList, callback);
		}

		return msg;
	}


	static CTRef GetTRef(string& fileTitle)
	{
		int year = 2000 + WBSF::as<int>(fileTitle.substr(1, 2));
		size_t jDay = WBSF::as<size_t>(fileTitle.substr(3, 3)) - 1;
		size_t h = WBSF::as<size_t>(fileTitle.substr(6, 2));

		CJDayRef JDay(year, jDay);
		return CTRef(JDay.GetYear(), JDay.GetMonth(), JDay.GetDay(), h);
	}

	ERMsg CSOPFEU::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = m_workingDir;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(it->m_filePath);
			string stationID = fileTitle.substr(0, 12);
			CTRef TRef = GetTRef(fileTitle);
			string outputFilePath = GetOutputFilePath(TRef);

			if (IsFileUpToDate(*it, outputFilePath, false))
				it = fileList.erase(it);
			else
				it++;


			msg += callback.StepIt();
		}

		callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));
		callback.AddMessage("");

		callback.PopTask();

		return msg;
	}

	ERMsg CSOPFEU::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = m_workingDir;
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");

		//load station list
		CFileInfoVector fileList;
		//msg = UpdateStationHistory();

		if (msg)
			msg = GetFileList(fileList, callback);

		if (msg)
		{
			CreateMultipleDir(m_workingDir);

			callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());

			size_t nbTry = 0;
			size_t curI = 0;

			while (curI < fileList.size() && msg)
			{
				nbTry++;

				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;

				msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, m_userName, m_password, true, 5, callback);
				if (msg)
				{
					try
					{
						while (curI < fileList.size() && msg)
						{
							string fileTitle = GetFileTitle(fileList[curI].m_filePath);

							//string stationID = fileTitle.substr(0, 12);
							CTRef TRef = GetTRef(fileTitle);

							string outputFilePath = GetOutputFilePath(TRef);
							CreateMultipleDir(GetPath(outputFilePath));

							msg += CopyFile(pConnection, fileList[curI].m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true);

							if (msg)
							{
								msg += callback.StepIt();
								curI++;
								nbTry = 0;
							}
						}
					}
					catch (CException* e)
					{
						//if an error occur: try again
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg = WaitServer(5, callback);

						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}


					//clean connection
					pConnection->Close();
					pSession->Close();
				}

			}

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
			callback.PopTask();
		}

		return msg;
	}


	ERMsg CSOPFEU::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		//send all available station
		msg = m_stationsList.Load(GetStationListFilePath());
		if (msg)
		{
			msg = LoadWeatherInMemory(callback);
			if (msg)
			{
				stationList.reserve(m_stations.size());
				for (CWeatherStationMap::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
					stationList.push_back(it->first);
			}
		}



		return msg;


	}

	ERMsg CSOPFEU::LoadWeatherInMemory(CCallback& callback)
	{
		ERMsg msg;

		size_t nbYears = m_lastYear - m_firstYear + 1;


		for (size_t y = 0; y < nbYears&& msg; y++)
		{
			int year = m_firstYear + int(y);

			//on pourrait optimiser en loadant une seul fois tout les fichiers
			StringVector fileList = GetFilesList(m_workingDir + to_string(year) + "\\" + "*.csv");
			callback.PushTask("Load SOPFEU data for year = " + to_string(year), fileList.size());

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				msg += ReadData(fileList[i], m_stations, callback);

				msg += callback.StepIt();
			}

			callback.PopTask();


			//compute hourly temperature and precipitation
			for (auto it = m_stations.begin(); it != m_stations.end(); it++)
			{
				CWeatherStation& station = it->second;
				CWVariables vars = station.GetVariables();

				if (vars[H_TMIN] && vars[H_TMAX])
				{
					CTPeriod p = station.GetEntireTPeriod();
					for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					{
						CHourlyData& data = station.GetHour(TRef);
						if (data[H_TMIN] > -999 && data[H_TMAX] > -999)
						{
							bool bValid = true;
							float Tmin = data[H_TMIN];
							float prevTmin = data.GetPrevious()[H_TMIN];
							float nextTmin = data.GetNext()[H_TMIN];
							if (prevTmin != -999 || nextTmin != -999)
							{
								float diff_prevTmin = prevTmin > -999 ? Tmin - prevTmin : -999;
								float diff_nextTmin = nextTmin > -999 ? Tmin - nextTmin : -999;
								if ((diff_prevTmin == -999 || diff_prevTmin > 8) && (diff_nextTmin == -999 || diff_nextTmin > 8) ||
									(diff_prevTmin == -999 || diff_prevTmin < -8) && (diff_nextTmin == -999 || diff_nextTmin < -8)
									)
								{
									bValid = false;
								}
							}

							float Tmax = data[H_TMAX];
							float prevTmax = data.GetPrevious()[H_TMAX];
							float nextTmax = data.GetNext()[H_TMAX];
							if (prevTmax != -999 || nextTmax != -999)
							{
								float diff_prevTmax = prevTmax > -999 ? Tmax - prevTmax : -999;
								float diff_nextTmax = nextTmax > -999 ? Tmax - nextTmax : -999;
								if ((diff_prevTmax == -999 || diff_prevTmax > 8) && (diff_nextTmax == -999 || diff_nextTmax > 8) ||
									(diff_prevTmax == -999 || diff_prevTmax < -8) && (diff_nextTmax == -999 || diff_nextTmax < -8)
									)
								{
									bValid = false;
								}
							}

							if (!bValid)
							{
								data.SetStat(H_TMIN, -999);
								data.SetStat(H_TAIR, -999);
								data.SetStat(H_TMAX, -999);
								data.SetStat(H_TDEW, -999);
								data.SetStat(H_RELH, -999);
							}
						}
					}
				}


				if (vars[H_ADD1] && vars[H_ADD2])
				{
					float lastPrcp1 = -999;
					float lastPrcp2 = -999;

					CTPeriod p = station.GetEntireTPeriod();
					for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
					{
						CHourlyData& data = station.GetHour(TRef);
						if (data[H_ADD1] > -999 && data[H_ADD2] > -999)
						{
							float hourly_prcp1 = -999;
							float hourly_prcp2 = -999;

							float prcp1 = data[H_ADD1];
							float prcp2 = data[H_ADD2];

							if (lastPrcp1 == -999)
								lastPrcp1 = prcp1;
							
							if (lastPrcp2 == -999)
								lastPrcp2 = prcp2;


							if (prcp1 > 0)
							{
								//verify for speaking
								if (data.HavePrevious() && data.HaveNext())
								{
									float prev1 = data.GetPrevious()[H_ADD1];
									float next1 = data.GetNext()[H_ADD1];
									if (prev1 > -999 && next1 > -999)
									{
										float diff_prev = prcp1 - prev1;
										float diff_next = prcp1 - next1;
										if (!((diff_prev > 2 && diff_next > 2) || (diff_prev < -2 && diff_next < -2)))
										{
											if (prcp1 >= lastPrcp1)
											{
												hourly_prcp1 = prcp1 - lastPrcp1;

											}

											lastPrcp1 = prcp1;
										}
									}
								}
							}

							if (prcp2 > 500)
							{
								//verify for speaking
								if (data.HavePrevious() && data.HaveNext())
								{
									float prev2 = data.GetPrevious()[H_ADD2];
									float next2 = data.GetNext()[H_ADD2];
									if (prev2 > -999 && next2 > -999)
									{
										float diff_prev = prcp2 - prev2;
										float diff_next = prcp2 - next2;
										if (!(( diff_prev > 2 && diff_next > 2) || (diff_prev <-2 && diff_next < -2)) )
										{
											if (prcp2 >= lastPrcp2)
											{
												hourly_prcp2 = prcp2 - lastPrcp2;
											}

											lastPrcp2 = prcp2;
										}
									}
								}
							}

							//if (hourly_prcp1 >= 0)
							//	data.SetStat(H_PRCP, hourly_prcp1);
							if (hourly_prcp2 >= 0)
								data.SetStat(H_PRCP, hourly_prcp2);
						}
					}//for all hours
				}

				//it->second.CleanUnusedVariable("TN T TX P TD H WS WD W2 R Z SD");
			}

		}


		return msg;
	}

	ERMsg CSOPFEU::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		station = m_stations[ID];

		//clean outside period
		for (CWeatherYears::iterator it = station.begin(); it != station.end(); )
		{
			if (it->first >= m_firstYear && it->first <= m_lastYear)
				it++;
			else
				it = station.erase(it);
		}

		if (msg && station.HaveData())
			msg = station.IsValid();

		station.SetSSI("Provider", "SOPFEU");
		station.SetSSI("Network", "SOPFEU");
		station.SetSSI("Country", "CAN");
		station.SetSSI("SubDivision", "QC");


		return msg;
	}

	bool CSOPFEU::IsValid(TVarH v, double value)
	{
		bool bValid = true;
		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX:
		case H_TDEW: bValid = value >= -45 && value <= 40; break;
		case H_PRCP:
		case H_SWE:
		case H_SNDH: bValid = value >= 0; break;
		case H_RELH: bValid = value >= 1 && value <= 100; break;
		case H_WNDD: bValid = value >= 0 && value <= 360; break;
		case H_WNDS:
		case H_WND2: bValid = value >= 0 && value <= 120; break;
		}

		return bValid;
	}

	ERMsg CSOPFEU::ReadData(const string& filePath, CWeatherStationMap& stations, CCallback& callback)const
	{
		ERMsg msg;

		StringVector str(GetFileTitle(filePath), "-");
		ASSERT(str.size() == 4);
		ASSERT(str[0][0] == 'm');
		ASSERT(str[0].size() == 5);
		str[0].erase(str[0].begin());//remove m

		CTRef UTCRef(ToInt(str[0]), ToSizeT(str[1]) - 1, ToSizeT(str[2]) - 1, ToSizeT(str[3]));


		ifStream  file;
		msg = file.open(filePath);
		if (msg)
		{
			enum { C_NETWORK, STA_ID, DATE, F_TIME, FIRST_FILED };
			for (CSVIterator loop(file, ";", false); loop != CSVIterator(); ++loop)
			{
				ASSERT(loop->size() >= FIRST_FILED);
				string ID = (*loop)[STA_ID];
				MakeLower(ID);
				//if (ID != "cqgt")
					//continue;

				size_t pos = m_stationsList.FindByID(ID);
				if (pos != NOT_INIT)
				{
					CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCRef, m_stationsList[pos]);
					for (size_t c = FIRST_FILED; c < loop->size() - 2; c += 3)
					{
						TVarH v = GetVariable((*loop)[c]);
						string flag = (*loop)[c + 1];
						string val = (*loop)[c + 2];
						ASSERT(!val.empty());

						if (v != H_SKIP && flag != "R" && !val.empty())
						{
							float value = ToFloat(val);
							if (stations.find(ID) == stations.end())
							{
								((CLocation&)stations[ID]) = m_stationsList[pos];
								stations[ID].SetHourly(true);
							}

							if (IsValid(v, value))
								stations[ID][TRef].SetStat(v, value);
						}
					}//for all coluns

					//verify temperature
					float Tmin = stations[ID].GetHour(TRef)[H_TMIN];
					float Tmax = stations[ID].GetHour(TRef)[H_TMAX];
					ASSERT(Tmax < 35);

					if (Tmin > -999 && Tmax > -999)
					{
						float Tair = (Tmin + Tmax) / 2;
						stations[ID][TRef].SetStat(H_TAIR, Tair);
					}
					else
					{
						stations[ID][TRef].SetStat(H_TMIN, -999);
						stations[ID][TRef].SetStat(H_TAIR, -999);
						stations[ID][TRef].SetStat(H_TMAX, -999);
					}
				}
				else
				{
					//callback.AddMessage("Undefine ID " + ID);
				}
			}//for all line
		}//if open


		return msg;
	}


}