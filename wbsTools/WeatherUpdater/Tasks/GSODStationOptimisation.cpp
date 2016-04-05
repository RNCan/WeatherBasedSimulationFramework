#include "stdafx.h"
#include "GSODStationOptimisation.h"
#include "Basic/UtilStd.h"

#include "../Resource.h"

using namespace std;
using namespace WBSF::WEATHER;

namespace WBSF
{


	//********************************************************************************
	ERMsg CGSODStationOptimisation::Update(const string& referencedFilePath, CCallback& callback)
	{
		ERMsg msg;

		clear();
		m_nbEmptyName = 0;

		ifStream file;
		msg = file.open(referencedFilePath);
		if (msg)
		{
			callback.AddMessage(GetString(IDS_GSOD_OPTIMISATION));
			callback.AddMessage(referencedFilePath, 1);
			callback.AddMessage("");

			callback.PushTask(GetString(IDS_GSOD_OPTIMISATION), (size_t)file.length(), (size_t)81);
			//callback.SetNbStep((size_t)file.length(), (size_t)81);


			CGSODStationOptimisation& me = *this;
			string tmp;

			while (tmp.substr(0, 11) != "USAF   WBAN")
				std::getline(file, tmp);

			//read empty line
			std::getline(file, tmp);

			CGSODStation station;
			while (msg && ReadStation(file, station))//std::getline(file, tmp))
			{
				if (station.m_USAF != "999999" && station.m_WBAN != "99999")
				{
					ASSERT(!station.m_name.empty());
					me[station.m_ID] = station;
				}

				msg += callback.StepIt();
			}

			file.close();

			m_referenceFileStamp = GetFileInfo(referencedFilePath);

			callback.AddMessage("station listed in history file: " + to_string(me.size()));
			callback.PopTask();
		}

		return msg;
	}

	CTPeriod CGSODStationOptimisation::GetPeriod(const string& line)const
	{
		CTPeriod p;
		string begin = line.substr(82, 8);
		string end = line.substr(91, 8);
		if (begin != "NO DATA " && end != "NO DATA ")
		{
			p.Begin() = CTRef(ToInt(begin.substr(0, 4)), ToInt(begin.substr(4, 2)) - 1, ToInt(begin.substr(6, 2)) - 1);
			p.End() = CTRef(ToInt(end.substr(0, 4)), ToInt(end.substr(4, 2)) - 1, ToInt(end.substr(6, 2)) - 1);
		}

		return p;
	}

	bool CGSODStationOptimisation::ReadStation(istream& stream, CGSODStation& station)
	{
		bool bRep = false;
		int nbNoName = 0;
		station.m_USAF = "999999";
		station.m_WBAN = "99999";

		//stationID = -1;

		station.Reset();
		string line;
		if (std::getline(stream, line))
		{
			bRep = true;
			if (line.length() >= 93)
			{
				//Get station name
				string name = PurgeFileName(line.substr(13, 29));

				if (name.empty())
				{
					name = PurgeFileName(line.substr(0, 12));
					if (name.empty())
					{
						m_nbEmptyName++;
						name = "EmptyName" + ToString(m_nbEmptyName);
					}
				}


				//Get station Coordinate
				double lat = -999;
				double lon = -999;
				double elev = -999;

				string strLat = line.substr(57, 7);
				string strLon = line.substr(65, 8);
				string strAlt = line.substr(74, 6);

				if (strLat != "      " && strLat != "+00.000" &&
					strLon != "       " && strLon != "+000.000" &&
					strAlt != "      " && strAlt != "-0999.0" && strAlt != "-0999.9")
				{
					lat = ToDouble(strLat);
					lon = ToDouble(strLon);
					elev = ToDouble(strAlt);        //elevation in meter


					if (line.substr(0, 6) == "716920")//coordinate error over MARTICOT ISLAND
						lon = -54.583;
					if (line.substr(7, 5) == "12848")//coordinate error over Dinner Key NAF
						lon = -80.2333;



					if (name.find("BOGUS") != -1 ||
						lat < -90 || lat>90 ||
						lon < -180 || lon>180 ||
						elev < -400)
					{
						lat = -999;
						lon = -999;
						elev = -999;
					}
				}


				if (lat != -999 || lon != -999)
				{
					station.m_USAF = "USAF" + line.substr(0, 6);
					station.m_WBAN = "WBAN" + line.substr(7, 5);
					station.m_country = TrimConst(line.substr(43, 2));
					ReplaceString(station.m_country, "DL", "GM");
					if (station.m_country.empty())
						station.m_country = "UN";//Unknown

					station.m_state = line.substr(48, 2);
					//station.m_CALL = line.substr(52, 4); 
					station.m_period = GetPeriod(line);
					ASSERT(station.m_USAF != "999999" || station.m_WBAN != "99999");

					station.m_name = UppercaseFirstLetter(name);
					ASSERT(!station.m_name.empty());

					//if( station.m_USAF != 999999)
					//station.m_ID = "USAF" + ToString(station.m_USAF);
					//else station.m_ID = "WBAN" + ToString(station.m_WBAN);
					station.m_ID = line.substr(0, 6) + "-" + line.substr(7, 5);

					station.m_lat = lat;
					station.m_lon = lon;
					station.m_elev = elev;

					station.SetToSSI();
				}
			}
		}

		return bRep;
	}


}