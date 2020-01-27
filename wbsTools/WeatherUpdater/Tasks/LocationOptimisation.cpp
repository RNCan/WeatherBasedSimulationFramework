#include "StdAfx.h"
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost\serialization\unordered_map.hpp>
#include <boost\serialization\map.hpp>
#include <boost\dynamic_bitset.hpp>
#include "LocationOptimisation.h"
//#include "Geomatic/UtilGDAL.h"
#include "Geomatic/GDALBasic.h"

#include "../resource.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace boost;

namespace WBSF
{


	bool CLocationOptimisation::NeedUpdate(const std::string& refFilePath, const std::string& optFilePath)
	{
		bool bRep = true;

		CFileInfo lastUpdate = GetLastUpdate(optFilePath);
		CFileInfo lastModification = GetFileInfo(refFilePath);

		if (lastUpdate == lastModification)
			bRep = false;

		return bRep;
	}

	CFileInfo CLocationOptimisation::GetLastUpdate(const std::string& filePath)
	{
		ERMsg msg;

		CFileInfo info;

		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::archive::binary_iarchive ar(file);

				__int32 version = 0;
				ar >> version;
				if (version == VERSION)
				{
					ar >> info;
				}
			}
			catch (...)
			{
				//do nothing and recreate optimization
			}

			file.close();
		}

		return info;
	}

	ERMsg CLocationOptimisation::Load(const std::string& filePath)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::archive::binary_iarchive ar(file);

				int version = 0;
				ar >> version;
				if (version == VERSION)
				{
					ar >> m_referenceFileStamp;
					ar >> (*this);
				}
			}
			catch (...)
			{
				//do nothing and recreate optimization
			}

			file.close();
		}

		return msg;
	}

	ERMsg CLocationOptimisation::Save(const std::string& filePath)
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath, ios::out | ios::binary);
		if (msg)
		{
			// write map instance to archive
			boost::archive::binary_oarchive ar(file);
			ar << VERSION;
			ar << m_referenceFileStamp;
			ar << (*this);
		}

		return msg;
	}

	//********************************************************************************************
	//CGHCNStationOptimisation
	double CGHCNStationOptimisation::GetStationElevation(const CGDALDatasetEx& dataset, const CLocation& location, int nbNearest, double deltatElevMax)
	{
		float elev = -999;

		//try to find elevation in the world DEM
		if (dataset.IsOpen())
		{
			CGeoPointIndexVector ptsIndex;
			CGeoExtents extents = dataset.GetExtents();
			if (extents.IsInside(location))
			{
				extents.GetNearestCellPosition(location, nbNearest, ptsIndex);

				vector<double> pixel(ptsIndex.size());

				CStatistic stat;
				for (size_t i = 0; i < ptsIndex.size(); i++)
				{
					pixel[i] = dataset.ReadPixel(0, ptsIndex[i]);
					if (fabs(pixel[i] - dataset.GetNoData(0)) > EPSILON_DATA)
						stat += pixel[i];
				}

				if (stat.IsInit())
				{
					if (fabs(stat[HIGHEST] - pixel[0]) < deltatElevMax &&
						fabs(stat[LOWEST] - pixel[0]) < deltatElevMax)
					{
						elev = float(pixel[0]);
					}
				}
			}//Is inside
		}//Is Open

		return elev;
	}


	void CGHCNStationOptimisation::GetZoneElevation(const CGDALDatasetEx& dataset, const CGeoPoint& pt0, const CGeoPoint& pt1, const CGeoPoint& pt2, double& evelLow, double& elev, double& elevHigh)
	{
		evelLow = -999;
		elev = -999;
		elevHigh = -999;
		CGeoExtents extents = dataset.GetExtents();

		//try to find elevation in the world map
		if (dataset.IsOpen())
		{
			CGeoPointIndex xy0 = extents.CoordToXYPos(pt0);
			CGeoPointIndex xy1 = extents.CoordToXYPos(pt1);
			CGeoPointIndex xy2 = extents.CoordToXYPos(pt2);

			CGeoRectIndex colRow(xy1, xy2);
			colRow.IntersectRect(CGeoRectIndex(0, 0, extents.m_xSize, extents.m_ySize));

			CStatistic stat;
			elev = dataset.ReadPixel(0, xy0);
			if (fabs(elev - dataset.GetNoData(0)) < EPSILON_DATA)
				elev = -999;
			//if( pixel0 > dataset.GetNoData() )
			{
				for (int x = 0; x <= colRow.Width(); x++)
				{
					for (int y = 0; y <= colRow.Height(); y++)
					{

						double pixel = dataset.ReadPixel(0, CGeoPointIndex(colRow.m_x + x, colRow.m_y + y));

						if (fabs(pixel - dataset.GetNoData(0)) > EPSILON_DATA)
							stat += pixel;
					}
				}

				if (stat[NB_VALUE] > 0)
				{
					evelLow = (float)stat[LOWEST];
					elevHigh = (float)stat[HIGHEST];
				}
			}
		}

		//return elev;
	}


	int CGHCNStationOptimisation::GetPrecision(const string& line)
	{
		string lat = line.substr(12, 8);
		Trim(lat, "0");
		size_t latPres = lat.length() - lat.find('.') - 1;

		string lon = line.substr(21, 9);
		Trim(lon, "0");
		size_t lonPres = lon.length() - lon.find('.') - 1;

		ASSERT(max(latPres, lonPres) <= 4);
		return (int)max(latPres, lonPres);
	}
	//Global Historical Climatology Network Database GHCND
	//********************************************************************************888
	ERMsg CGHCNStationOptimisation::Update(const string& refFilePath, CCallback& callback)
	{
		ERMsg msg;

		CGHCNStationOptimisation& me = *this;

		clear();

		
		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<char>());
		

		ifStream file;
		file.imbue(utf8_locale);
		msg = file.open(refFilePath);

		string excludeFilePath(refFilePath);
		SetFileName(excludeFilePath, "ghcnd-exclude-stations.txt");
		ofStream excludeFile;
		if (msg)
			msg += excludeFile.open(excludeFilePath);

		string elevAllStationFilePath(refFilePath);
		SetFileName(elevAllStationFilePath, "ghcnd-stations-all-elevation.csv");
		ofStream elevAllStationFile;
		if (msg)
			msg += elevAllStationFile.open(elevAllStationFilePath);

		//open world DEM for replacing elevation
		CGDALDatasetEx dataset;
		if (msg && !m_DEMFilePath.empty())
			msg = dataset.OpenInputImage(m_DEMFilePath.c_str());

		if (msg)
		{
			callback.AddMessage(GetString(IDS_GSOD_OPTIMISATION));
			callback.AddMessage(refFilePath, 1);
			callback.AddMessage("");

			callback.PushTask(GetString(IDS_GSOD_OPTIMISATION), (size_t)file.length(), size_t(81 + 1));
			//callback.SetNbStep();

			elevAllStationFile << "Name,ID,Lat,Lon,Elev,res,ElevLo(SRTM4.1),Elev(SRTM4.1),ElevHi(SRTM4.1)\n";

			string line;
			while (msg && std::getline(file, line))
			{
				//int res = 0;
				//double elevLo = -999;
				double elev = -999;
				//double elevHi = -999;


				CLocation location;
				if (LocationFromLine(line, location))
				{
					ASSERT(location.m_lat != -999 && location.m_lon != -999);

					location.SetSSI("DEMElevation", "0");
					if (location.m_elev == -999)// || (location.m_elev == 0 && location.GetSSI("Country")=="BR" ) a lot of brasil station have 0 as elevation
					{
						double elev = GetStationElevation(dataset, location, 9, 50);
						if (elev != -999)
						{
							location.m_elev = elev;
							location.SetSSI("DEMElevation", "1");
						}
					}

					if (location.m_elev != -999)
						me[location.m_ID] = location;
				}
				else
				{
					if (excludeFile.is_open())
						excludeFile << line + "\n";
				}

				if (elev != -999)
				{
					string str;
					for (int i = 0; i < CLocation::NB_MEMBER; i++)
						str += location.GetMember(i) + ",";

					//str += ToString(res)+ ",";
					//str += ToString(elevLo, 1)+ ",";
					str += ToString(elev, 1) + ",";
					//str += ToString(elevHi, 1)+ "\n";
					elevAllStationFile << str;
				}



				msg += callback.StepIt();
			}

			file.close();
			excludeFile.close();
			elevAllStationFile.close();


			if (msg)
				m_referenceFileStamp = GetFileInfo(refFilePath);
		}

		callback.PopTask();

		return msg;
	}



	bool CGHCNStationOptimisation::LocationFromLine(std::string line, CLocation& location)
	{
		bool bRep = false;

		location.clear();

		Trim(line);
		if (!line.empty())
		{
			string tmp;

			string country = TrimConst(line.substr(0, 2));
			string state = TrimConst(line.substr(38, 2));
			if (country.empty())
				country = "UN";//Unknown

			location.SetSSI("Country", country);
			location.SetSSI("State", state);

			location.m_ID = Trim(line.substr(0, 11));
			location.m_name = UppercaseFirstLetter(Trim(line.substr(41, 29)));
			

			tmp = Trim(line.substr(12, 8));
			if (!tmp.empty())
			{
				double lat = ToDouble(tmp);
				if (lat > -90 && lat<90)
					location.m_lat = lat;
			}

			tmp = Trim(line.substr(21, 9));
			if (!tmp.empty())
			{
				double lon = ToDouble(tmp);
				if (lon>-360 && lon < 360)
					location.m_lon = lon;
			}

			tmp = Trim(line.substr(31, 6));
			if (!tmp.empty())
			{
				double elev = ToDouble(tmp);
				if (elev > -999)
					location.m_elev = elev;
			}

			if (location.m_lat != -999 && location.m_lon != -999)
				bRep = true;
		}


		return bRep;
	}



}