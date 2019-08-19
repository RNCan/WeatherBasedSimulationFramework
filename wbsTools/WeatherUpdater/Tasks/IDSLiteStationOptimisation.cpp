#include "stdafx.h"
#include "IDSLiteStationOptimisation.h"
#include "Basic/CSV.h"
#include "Geomatic/ShapeFileBase.h"


#include "../Resource.h"

using namespace std;
using namespace WBSF::WEATHER;

namespace WBSF
{

	static const char* TYPE_NAME[2] = { "USAF", "WBAN" };

	bool CIDSLiteStation::operator>(const CIDSLiteStation& in)const
	{
		bool bRep = m_ID > in.m_ID;
		if (m_ID == in.m_ID)
			bRep = m_period.Begin() > in.m_period.Begin();

		return bRep;
	}



	bool CIDSLiteStation::ReadStation(const StringVector& line)
	{
		ASSERT(line.size() == NB_COLUMNS);

		bool bRep = false;
		static int nbEmptyName = 0;

		Reset();

		if (!line[C_LAT].empty() && !line[C_LON].empty() && !line[C_ELEV].empty() &&
			line[C_LAT] != "+00.000" && line[C_LON] != "+000.000" &&
			line[C_ELEV] != "-0999.9" && line[C_ELEV] != "-0999.0")
		{
			ASSERT(line[C_LAT] != "-99.999" && line[C_LON] != "-999.999");

			bRep = true;

			string name = TrimConst(line[C_STATION_NAME]);

			if (name.empty())
			{
				nbEmptyName++;
				name = "EmptyName" + ToString(nbEmptyName);
			}
			ASSERT(!name.empty());
			m_name = UppercaseFirstLetter(name);

			m_country = TrimConst(line[C_CTRY]);
			m_state = TrimConst(line[C_STATE]);
			if (m_country.empty())
				m_country = "UN";//Unknown

			ASSERT(!line[C_USAF].empty());
			ASSERT(!line[C_WBAN].empty());
			m_USAF = "USAF" + line[C_USAF];
			m_WBAN = "WBAN" + line[C_WBAN];

			m_ID = line[C_USAF] + "-" + line[C_WBAN];
			ASSERT(!m_ID.empty());

			m_lat = ToDouble(line[C_LAT]);
			m_lon = ToDouble(line[C_LON]);
			m_elev = ToDouble(line[C_ELEV]);

			if (line[C_USAF] == "716920")//coordinate error over MARTICOT ISLAND
				m_lon = -54.583;
			if (line[C_WBAN] == "12848")//coordinate error over Dinner Key NAF
				m_lon = -80.2333;

			ASSERT(m_lat >= -90 && m_lat < 90);
			ASSERT(m_lon >= -180 && m_lon < 180);

			if (!line[C_BEGIN].empty() && !line[C_END].empty())
			{
				m_period.Begin() = CTRef(ToInt(line[C_BEGIN].substr(0, 4)), ToInt(line[C_BEGIN].substr(4, 2)) - 1, ToInt(line[C_BEGIN].substr(6, 2)) - 1);
				m_period.End() = CTRef(ToInt(line[C_END].substr(0, 4)), ToInt(line[C_END].substr(4, 2)) - 1, ToInt(line[C_END].substr(6, 2)) - 1);
			}

			SetToSSI();
		}

		return bRep;
	}

	//****************************************************************

	ERMsg CIDSLiteStationOptimisation::Update(const string& referencedFilePath, CCallback& callback)
	{
		ERMsg msg;

		clear();

		CIDSLiteStationOptimisation& me = *this;
		ifStream file;
		msg = file.open(referencedFilePath);

		if (msg)
		{

			std::string provinces_file_path = GetApplicationPath() + "Layers\\Canada.shp";


			m_pShapefile = new CShapeFileBase;
			if (!m_pShapefile->Read(provinces_file_path))
			{
				delete m_pShapefile;
				m_pShapefile = NULL;
			}



			callback.AddMessage(GetString(IDS_GSOD_OPTIMISATION));
			callback.AddMessage(referencedFilePath, 1);
			callback.AddMessage("");

			callback.PushTask(GetString(IDS_GSOD_OPTIMISATION), (int)file.length());
			//callback.SetNbStep((int)file.length());

			for (CSVIterator loop(file, ",", true, true); loop != CSVIterator(); ++loop)
			{
				CIDSLiteStation station;
				bool bValid = false;
				bValid = station.ReadStation(*loop);

				if (bValid)
				{
					if (station.m_country == "CA")
					{
						if (m_pShapefile && station.m_state.empty())
						{
							station.m_state = GetProvince(station.m_lat, station.m_lon);
							station.SetToSSI();
						}
					}

					me[station.m_ID] = station;

				}


				msg += callback.StepIt(double(loop->GetLastLine().length() + 1));
			}

			callback.AddMessage("station listed in history file: " + to_string(me.size()));
			callback.PopTask();


			if (m_pShapefile)
			{
				delete m_pShapefile;
				m_pShapefile = NULL;
			}


		}

		if (msg)
			m_referenceFileStamp = GetFileInfo(referencedFilePath);


		return msg;
	}


	std::string CIDSLiteStationOptimisation::GetProvince(double lat, double lon)const
	{
		ASSERT(m_pShapefile);

		std::string prov;

		const CDBF3& DBF = m_pShapefile->GetDBF();
		int Findex = DBF.GetFieldIndex("STATE_ID");
		ASSERT(Findex >= 0 && Findex < DBF.GetNbField());

		CGeoPoint pt(lon, lat, PRJ_WGS_84);
		int shapeNo = -1;

		//m_pShapefile->
		if (m_pShapefile->IsInside(pt, &shapeNo))//inside a shape
		{
			prov = DBF[shapeNo][Findex].GetElement();
		}
		else
		{
			double d = m_pShapefile->GetMinimumDistance(pt, &shapeNo);
			ASSERT(shapeNo >= 0 && shapeNo < DBF.GetNbRecord());
			prov = DBF[shapeNo][Findex].GetElement();
		}

		return prov;
	}
}
