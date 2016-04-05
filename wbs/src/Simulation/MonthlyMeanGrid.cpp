//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "StdAfx.h"
#pragma warning(disable: 4275 4251)
#include "GDAL_priv.h"
#include "ogr_srs_api.h"


#include "Basic/WeatherStation.h"
#include "Basic/UtilTime.h"
#include "Basic/DailyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Simulation/AdvancedNormalStation.h"
#include "Simulation/MonthlyMeanGrid.h"


using namespace std;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	static const int FIRST_YEAR_OF_FIRST_PERIOD = 1961;

	const char* CMonthlyMeanGrid::FILE_NAME[NB_FIELDS] = { "Tmin.tif", "Tmax.tif", "TminTmax.tif", "Tmin_SD.tif", "Tmax_SD.tif", "A1.tif", "A2.tif", "B1.tif", "B2.tif", "Prcp.tif", "Prcp_SD.tif", "SpeH.tif", "RelH.tif", "RelH_SD.tif", "WndS.tif", "WndS_SD.tif" };

	const char* CMonthlyMeanGrid::XML_FLAG = "MontlyMeanGrids";
	const char* CMonthlyMeanGrid::MEMBER_NAME[NB_MEMBER] = { "FirstYear", "LastYear", "Variables" };
	std::string CMonthlyMeanGrid::GetMember(size_t i)const
	{
		std::string str;

		switch (i)
		{
		case FIRST_YEAR: str = ToString(m_firstYear); break;
		case LAST_YEAR: str = ToString(m_lastYear); break;
		case VARIABLES_USED: str = GetVariablesUsed(); break;
		default: ASSERT(false);
		}

		return str;
	}

	void CMonthlyMeanGrid::SetMember(size_t i, const std::string& str)
	{
		switch (i)
		{
		case FIRST_YEAR: m_firstYear = ToInt(str); break;
		case LAST_YEAR: m_lastYear = ToInt(str); break;
		case VARIABLES_USED: SetVariablesUsed(str); break;
		default: ASSERT(false);
		}
	}

	std::string CMonthlyMeanGrid::GetVariablesUsed()const
	{
		//CDailyDatabaseFormat format;
		std::vector<int> var;
		for (int i = 0; i < NB_FIELDS; i++)
		{
			if (m_supportedVariables[i])
				var.push_back(i);
		}


		//std::string str = (format.GetString(",", true));
		//str.Replace("Add1","SpcH");
		return ToString(var);
	}

	void CMonthlyMeanGrid::SetVariablesUsed(std::string str)
	{
		//str.MakeLower();
		//str.Replace("spch","add1");
		for (int i = 0; i < NB_FIELDS; i++)
			m_supportedVariables[i] = false;

		std::vector<int> var = ToVector<int>(str);

		for (int i = 0; i < var.size(); i++)
			m_supportedVariables[var[i]] = true;

	}

	ERMsg CMonthlyMeanGrid::Open(std::string filePath)
	{
		ERMsg msg;
		ASSERT(false); //todo
		//msg = Load(filePath);
		if (msg)
		{
			for (int v = 0; v < NB_FIELDS; v++)
			{
				std::string filePath = GetFilePath(v);
				if (!filePath.empty())
					msg += m_grid[v].OpenInputImage(filePath);
			}
		}

		return msg;
	}

	void CMonthlyMeanGrid::Close()
	{
		ERMsg msg;
		for (int v = 0; v < NB_FIELDS; v++)
		{
			if (m_grid[v].IsOpen())
				m_grid[v].Close();
		}
	}

	std::string CMonthlyMeanGrid::GetFilePath(int v)
	{
		std::string filePath;
		if (m_supportedVariables[v])
		{
			filePath = GetPath(m_filePath) + GetFileTitle(m_filePath) + "_" + FILE_NAME[v];
			//if(v==RELH&&m_bSpecificHumidity)
			//filePath=GetPath(m_filePath)+"SpcH.bil";
		}

		return filePath;
	}


	float CMonthlyMeanGrid::GetMonthlyMean(short v, short year, short m, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d)
	{
		ASSERT(m_firstYear != -1);
		ASSERT(pts.size() == d.size());

		if (!m_grid[v].IsOpen())
			return MISSING;


		int band = (year - m_firstYear) * 12 + m + 1;
		return (float)m_grid[v].GetWindowMean(band, nbNeighbor, power, pts, d);


	}


	bool CMonthlyMeanGrid::GetMonthlyMean(short firstYear, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, double monthlyMean[12][NB_FIELDS])
	{
		ASSERT(firstYear >= m_firstYear && firstYear <= m_lastYear);

		bool bRep = true;
		CStatistic MMStat[12][NB_FIELDS];

		for (int y = 0; y < 30; y++)
		{
			short year = firstYear + y;
			for (int m = 0; m < 12; m++)
			{
				for (int v = 0; v < NB_FIELDS; v++)
				{
					int vv = v;
					//there are never mmg for PRCP_SD, we use the value of PRCP_TT
					if (v == PRCP_SD)
						vv = PRCP_TT;

					float value = GetMonthlyMean(vv, year, m, nbNeighbor, power, pts, d);
					if (!IsMissing(value))
						MMStat[m][v] += value;
				}
			}
		}

		for (int m = 0; m < 12; m++)
		{
			for (int v = 0; v < NB_FIELDS; v++)
			{
				if (MMStat[m][v][NB_VALUE] >= 10)//Need at least 10 years of data
				{
					if (v == PRCP_SD)
						monthlyMean[m][v] = MMStat[m][v][COEF_VAR];//use coefficient of variation for precipitation
					else monthlyMean[m][v] = MMStat[m][v][MEAN];
				}
				else
				{
					monthlyMean[m][v] = MISSING;
					if (m_grid[v].IsOpen())
						bRep = false;
				}
			}
		}

		return bRep;
	}


	//int N2D(int f)
	//{
	//	int v=-1;
	//	switch(f)
	//	{
	//	case TMIN_MN: v=DAILY_DATA::TMIN; break;
	//	case TMAX_MN: v=DAILY_DATA::TMAX; break;
	//	case PRCP_TT: v=DAILY_DATA::PRCP; break;
	//	case TDEW_MN: v=DAILY_DATA::TDEW; break;
	//	case RELH_MN: v=DAILY_DATA::RELH; break;
	//	case WNDS_MN: v=DAILY_DATA::WNDS; break;
	//	default : v=-1;
	//	}
	//	return v;
	//}

	//
	//int N2H(int f)
	//{
	//	int v = -1;
	//	switch (f)
	//	{
	//	case TMIN_MN: v = H_TAIR; break;
	//	case TMAX_MN: v = H_TAIR; break;
	//	case PRCP_TT: v = H_PRCP; break;
	//	case TDEW_MN: v = H_TDEW; break;
	//	case RELH_MN: v = H_RELH; break;
	//	case WNDS_MN: v = H_WNDS; break;
	//	default: v = -1;
	//	}
	//	return v;
	//}
	//
	//
	//short H2N(short v)
	//{
	//	ASSERT(v >= 0 && v<NB_VAR_H);
	//
	//	short n = -1;
	//	switch (v)
	//	{
	//	case H_TAIR: n = TMIN_MN; break;
	//	case H_PRCP: n = PRCP_TT; break;
	//	case H_TDEW: n = SPEH_MN; break;
	//	case H_RELH: n = RELH_MN; break;
	//	case H_WNDS: n = WNDS_MN; break;
	//	default: break;
	//	}
	//
	//	return n;
	//}


	bool CMonthlyMeanGrid::UpdateData(short firstRefYear, short firstCCYear, short nbNeighbor, int maxDistance, double power, CWeatherStation& station)
	{
		ASSERT(m_grid[TMIN_MN].IsOpen());

		//ca ne fonctionne pas correctement pour la precipitation et wind speed?
		const CMonthlyMeanGrid& me = *this;


		CGeoPoint pt(station);
		if (pt.GetPrjID() != m_grid[TMIN_MN].GetPrjID())
		{
			pt.Reproject(CProjectionTransformationManager::Get(pt.GetPrjID(), m_grid[TMIN_MN].GetPrjID()));
		}


		CGeoPointIndex index = m_grid[TMIN_MN].GetExtents().CoordToXYPos(pt);

		if (index.m_x < 0 || index.m_x >= m_grid[TMIN_MN]->GetRasterXSize() || index.m_y < 0 || index.m_y >= m_grid[TMIN_MN]->GetRasterYSize())
			return false;


		CGeoPointIndexVector pts;
		CGeoExtents extents = m_grid[TMIN_MN].GetExtents();

		int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		extents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);

		std::vector<double> d;
		for (size_t i = 0; i < pts.size(); i++)
		{
			CGeoPoint pti = extents.XYPosToCoord(pts[i]);
			double di = max(0.000001, pt.GetDistance(pti));
			if (di < maxDistance)
				d.push_back(di);
		}

		pts.erase(pts.begin() + d.size(), pts.end());

		if (pts.empty())
			return false;


		double refMonthlyMean[12][NB_FIELDS] = { 0 };
		double ccMonthlyMean[12][NB_FIELDS] = { 0 };

		if (!GetMonthlyMean(firstRefYear, nbNeighbor, power, pts, d, refMonthlyMean))
			return false;

		if (!GetMonthlyMean(firstCCYear, nbNeighbor, power, pts, d, ccMonthlyMean))
			return false;


		CWeatherStation stationII;
		((CLocation&)stationII) = station;
		//stationII.SetFirstYear(firstCCYear);
		stationII.CreateYears(firstCCYear, station.size());

		for (size_t y = 0; y < station.size(); y++)
		{
			int year = station[y].GetTRef().GetYear();

			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < station[y][m].size(); d++)
				{
					for (TVarH v = H_TAIR; v<NB_VAR_H; ((int&)v)++)
					{
						size_t f = V2F(v);
						//if (v == H_TDEW)
						//v = H_RELH;//pass pass: try to convert relative humidity from specific humidity ratio. f is specific humidity

						if (f>-1 && station[y][m][d][v].IsInit() && !IsMissing(ccMonthlyMean[m][f]) && !IsMissing(refMonthlyMean[m][f]))
						{
							if (v == HOURLY_DATA::H_TAIR)
							{
								const CStatistic& statIn = station[y][m][d][v];
								CStatistic statOut;
								statOut += statIn[LOWEST] + (ccMonthlyMean[m][TMIN_MN] - refMonthlyMean[m][TMIN_MN]);
								statOut += statIn[HIGHEST] + (ccMonthlyMean[m][TMAX_MN] - refMonthlyMean[m][TMAX_MN]);
								stationII[y][m][d][v] = statOut;
							}
							else if (v == HOURLY_DATA::H_PRCP)
							{
								double prcp = station[y][m][d][v][SUM];
								prcp *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);
								stationII[y][m][d][v] = prcp;
							}
							else if (v == HOURLY_DATA::H_TDEW)
							{
								if (IsMissing(refMonthlyMean[m][RELH_MN]))//no relative humidity. then take specific humidity
								{
									ASSERT(refMonthlyMean[m][f] < 20);//ccMonthlyMean must be specyfic humidity g[H2O]/kg[air]
									if (station[y][m][d][H_RELH].IsInit() && station[y][m][d][H_TAIR].IsInit())
									{
										ASSERT(station[y][m][d][H_TAIR][NB_VALUE] >= 2);

										//convert Hr to Hs with station temperature
										double Tmin = station[y][m][d][H_TAIR][LOWEST];
										double Tmax = station[y][m][d][H_TAIR][HIGHEST];
										double Hr = station[y][m][d][H_RELH][MEAN];
										double Hs = Hr2Hs(Tmin, Tmax, Hr);
										Hs *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);//specific humidity ratio

										//convert back Hs to Hr with the new station temperature
										double TminII = stationII[y][m][d][H_TAIR][LOWEST];
										double TmaxII = stationII[y][m][d][H_TAIR][HIGHEST];
										Hr = Hs2Hr(TminII, TmaxII, Hs);

										stationII[y][m][d][H_RELH] = Hr;
									}
								}


								//	double Tdew = station[y][m][d][v][MEAN];
								//	Tdew = Min(stationII[y][m][d][H_TAIR][LOWEST], Tdew + (ccMonthlyMean[m][f] - refMonthlyMean[m][f]));
								//	stationII[y][m][d][v] = Tdew;
								//}
							}
							else if (v == HOURLY_DATA::H_RELH)
							{
								ASSERT(refMonthlyMean[m][f] >= 0 && refMonthlyMean[m][f] <= 100);//ccMonthlyMean must be relative humidity [%]

								//convert Hr to Hs with station temperature
								double Hr = station[y][m][d][H_RELH][MEAN];
								Hr = max(0.0, min(100.0, (Hr*ccMonthlyMean[m][f] / refMonthlyMean[m][f])));
								stationII[y][m][d][H_RELH] = Hr;
							}
							else if (v == HOURLY_DATA::H_WNDS)
							{
								double wndS = station[y][m][d][v][MEAN];
								wndS *= (ccMonthlyMean[m][f] / refMonthlyMean[m][f]);
								stationII[y][m][d][v] = wndS;
							}
						}//if is valid fields
					}//for all fields

					//complete humidity
					if (stationII[y][m][d][H_TAIR].IsInit())
					{
						if (stationII[y][m][d][H_TDEW].IsInit() &&
							!stationII[y][m][d][H_RELH].IsInit())
						{
							double RH = Td2Hr(stationII[y][m][d][H_TAIR][MEAN], stationII[y][m][d][H_TDEW][MEAN]);
							stationII[y][m][d][H_RELH] = RH;
						}
						else if (stationII[y][m][d][H_RELH].IsInit() &&
							!stationII[y][m][d][H_TDEW].IsInit())
						{
							double TDew = Hr2Td(stationII[y][m][d][H_TAIR][MEAN], stationII[y][m][d][H_RELH][MEAN]);
							stationII[y][m][d][H_TDEW] = TDew;
						}
					}
				}//for all days
			}//for all month
		}//for all years

		station = stationII;
		ASSERT(stationII.IsValid());

		return true;
	}

	//after created the normal station from daily station, update normal standard deviation when they exist
	bool CMonthlyMeanGrid::UpdateStandardDeviation(short firstRefYear, short firstccYear, short nbNeighbor, int maxDistance, double power, CNormalsStation& station)
	{
		ASSERT(m_grid[TMIN_MN].IsOpen());

		if (!m_grid[DEL_STD].IsOpen() && !m_grid[EPS_STD].IsOpen() && !m_grid[RELH_SD].IsOpen() && !m_grid[WNDS_SD].IsOpen())
			return true;

		CProjectionTransformation PT(CProjectionManager::GetPrj(PRJ_WGS_84), CProjectionManager::GetPrj(m_grid[TMIN_MN]->GetProjectionRef()));
		const CMonthlyMeanGrid& me = *this;

		CGeoPoint pt(station.m_lon, station.m_lat, PRJ_WGS_84);
		pt.Reproject(PT);

		CGeoPointIndex index = m_grid[TMIN_MN].GetExtents().CoordToXYPos(pt);

		if (index.m_x < 0 || index.m_x >= m_grid[TMIN_MN]->GetRasterXSize() || index.m_y < 0 || index.m_y >= m_grid[TMIN_MN]->GetRasterYSize())
			return false;


		CGeoPointIndexVector pts;
		CGeoExtents extents = m_grid[TMIN_MN].GetExtents();

		int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		extents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);

		std::vector<double> d;
		for (size_t i = 0; i < pts.size(); i++)
		{
			CGeoPoint pti = extents.XYPosToCoord(pts[i]);
			double di = max(0.000001, pt.GetDistance(pti));
			if (di < maxDistance)
				d.push_back(di);
		}

		pts.erase(pts.begin() + d.size(), pts.end());
		if (pts.empty())
			return false;

		double refMonthlyMean[12][NB_FIELDS] = { 0 };
		double ccMonthlyMean[12][NB_FIELDS] = { 0 };

		if (!GetMonthlyMean(firstRefYear, nbNeighbor, power, pts, d, refMonthlyMean))
			return false;

		if (!GetMonthlyMean(firstccYear, nbNeighbor, power, pts, d, ccMonthlyMean))
			return false;


		CNormalsData data = station;
		for (int m = 0; m < 12; m++)
		{
			for (int v = 0; v < NB_FIELDS; v++)
			{
				if (!IsMissing(data[m][v]))
				{
					if (!IsMissing(ccMonthlyMean[m][v]) && !IsMissing(refMonthlyMean[m][v]))
					{
						if (v == DEL_STD || v == EPS_STD || v == PRCP_SD || v == RELH_SD)
						{
							data[m][v] *= float(ccMonthlyMean[m][v] / refMonthlyMean[m][v]);
							ASSERT(data[m][v] < 2000);
						}
						else if (v == WNDS_SD)
						{
							data[m][v] += float(log(ccMonthlyMean[m][v]) - log(refMonthlyMean[m][v]));
							if (data[m][v] < 0.0001)//sometime, variance are negative, in this case we take the variance of future period...
								data[m][v] = (float)log(ccMonthlyMean[m][v]);

							ASSERT(data[m][v] > 0);
						}
					}
				}
			}
		}

		((CNormalsData&)station) = data;

		return true;
	}

	bool CMonthlyMeanGrid::UpdateData(short firstRefYear, short firstccYear, short nbNeighbor, int maxDistance, double power, CNormalsStation& station)
	{
		ASSERT(m_grid[TMIN_MN].IsOpen());

		CProjectionTransformation PT(CProjectionManager::GetPrj(PRJ_WGS_84), CProjectionManager::GetPrj(m_grid[TMIN_MN]->GetProjectionRef()));
		const CMonthlyMeanGrid& me = *this;

		CGeoPoint pt(station.m_lon, station.m_lat, PRJ_WGS_84);
		pt.Reproject(PT);

		CGeoPointIndex index = m_grid[TMIN_MN].GetExtents().CoordToXYPos(pt);

		if (index.m_x < 0 || index.m_x >= m_grid[TMIN_MN]->GetRasterXSize() || index.m_y < 0 || index.m_y >= m_grid[TMIN_MN]->GetRasterYSize())
			return false;


		CGeoPointIndexVector pts;
		CGeoExtents extents = m_grid[TMIN_MN].GetExtents();

		int level = (int)ceil((sqrt((double)nbNeighbor) - 1) / 2);
		extents.GetNearestCellPosition(pt, Square((level + 1) * 2 + 1), pts);

		std::vector<double> d;
		for (size_t i = 0; i < pts.size(); i++)
		{
			CGeoPoint pti = extents.XYPosToCoord(pts[i]);
			double di = max(0.000001, pt.GetDistance(pti));
			if (di < maxDistance)
				d.push_back(di);
		}

		pts.erase(pts.begin() + d.size(), pts.end());
		if (pts.empty())
			return false;

		double refMonthlyMean[12][NB_FIELDS] = { 0 };
		double ccMonthlyMean[12][NB_FIELDS] = { 0 };

		if (!GetMonthlyMean(firstRefYear, nbNeighbor, power, pts, d, refMonthlyMean))
			return false;

		if (!GetMonthlyMean(firstccYear, nbNeighbor, power, pts, d, ccMonthlyMean))
			return false;


		CNormalsData data = station;
		for (int m = 0; m < 12; m++)
		{
			for (int v = 0; v < NB_FIELDS; v++)
			{
				if (!IsMissing(data[m][v]))
				{
					if (!IsMissing(ccMonthlyMean[m][v]) && !IsMissing(refMonthlyMean[m][v]))
					{
						if (v == TMIN_MN || v == TMAX_MN)
						{
							data[m][v] += float(ccMonthlyMean[m][v] - refMonthlyMean[m][v]);
						}
						else if (v == DEL_STD || v == EPS_STD || /*v==TMNMX_R ||*/
							v == PRCP_TT || v == PRCP_SD || v == RELH_SD)//|| v == PRCP_SD 
						{
							data[m][v] *= float(ccMonthlyMean[m][v] / refMonthlyMean[m][v]);
							ASSERT(data[m][v] < 2000);
						}
						else if (v == WNDS_MN)
						{
							//ccMonthlyMean and refMonthlyMean are not in log:grid must not be logged
							data[m][v] += float(log(ccMonthlyMean[m][v]) - log(refMonthlyMean[m][v]));
						}
						else if (v == WNDS_SD)
						{
							data[m][v] += float(log(ccMonthlyMean[m][v]) - log(refMonthlyMean[m][v]));
							if (data[m][v] < 0.0001)//sometime, variance are negative, in this case we take the variance of future period...
								data[m][v] = (float)log(ccMonthlyMean[m][v]);

							ASSERT(data[m][v] > 0);
						}
						else if (v == RELH_MN)
						{
							data[m][v] *= float(ccMonthlyMean[m][v] / refMonthlyMean[m][v]);
							data[m][v] = min(100.0f, max(0.0f, data[m][v]));
							//dew point is lost in convertion. Not mather because they never hused
						}
						else if (v == SPEH_MN)
						{
							//It's not possible to convert monthly specific humidity, then do nothing. user must used daily data instead
						}
					}
					else
					{
						if (v == TMIN_MN || v == TMAX_MN || v == PRCP_TT || v == WNDS_MN)
						{
							data[m][v] = MISSING;
						}
						else if (v == RELH_MN)
						{
							data[m][SPEH_MN] = MISSING;
							data[m][RELH_MN] = MISSING;
							data[m][RELH_SD] = MISSING;
							//dew point is lost in convertion. Not mather because they never hused
						}
					}
				}
			}
		}

		((CNormalsData&)station) = data;

		return true;
	}


	ERMsg CMonthlyMeanGrid::ExportMonthlyValue(short firstRefYear, short firstCCYear, short nbNeighbor, CWeatherStation& station, const std::string& filePath, CCallback& callback)
	{
		ASSERT(m_grid[TMIN_MN].IsOpen());

		ERMsg msg;
		/*
			static char* VAR_NAME[NB_VAR] = {"Tmin(°C)","Tmax(°C)","Prcp(mm)","Tdew(°C)","RelH(%)","WndS(km/h)","Snow","snow","Srad","Add1","Add2","Add3","Add4"};
			//std::string filePath2(filePath);
			//SetFileTitle( filePath2, GetFileTitle( filePath ) + "_equation");

			const CMonthlyMeanGrid& me = *this;
			CGeoPointWP pt( station.GetCoord(), CProjection(CProjection::GEO));

			CGeoPointIndex index;
			pt.Reproject(m_grid[TMIN].GetPrj());
			m_grid[TMIN].CartoCoordToColRow(pt, index,true);

			if( index.x==-1 || index.y==-1)
			{
			msg.ajoute( "point for station " + station.GetName() +" not in the OURANOS database");
			return msg;
			}

			CStdioFileEx file1;
			msg += file1.Open(filePath, CFile::modeCreate|CFile::modeWrite);

			if(!msg)
			return msg;


			std::string tmp;
			tmp.Format("refYear,ccYear,month,Var,OURANOS (%d-%d),OURANOS (%d-%d),Data (%d-%d),CC data (%d-%d),Delta\n", firstRefYear, firstRefYear+29, firstccYear, firstccYear+29, firstRefYear, firstRefYear+29, firstccYear, firstccYear+29);
			file1.WriteString(tmp);

			//ASSERT( lastccYear-firstccYear+1==30);
			double refMonthlyMean[12][NB_VAR] = {0};
			double ccMonthlyMean[12][NB_VAR] = {0};

			GetMonthlyMean(firstRefYear, nbNeighbor, pt, refMonthlyMean );
			GetMonthlyMean(firstccYear, nbNeighbor, pt, ccMonthlyMean );

			for(int y=0; y<30; y++)
			{
			short refYear = firstRefYear+y;
			short ccYear = firstccYear+y;

			//	for(int year=firstccYear; year<=lastccYear; year++)
			//	{

			//		int refYear = firstRefYear+year-firstccYear;
			if( !station.YearExist(refYear) || ccYear <firstYearApplyCC)
			continue;

			//		int y = refYear-firstRefYear;
			for(int m=0; m<12; m++)
			{
			//daily data
			CStatistic refMonthlyMeanF[NB_VAR];
			CStatistic ccMonthlyMeanF[NB_VAR];

			int jd=GetJDay(refYear, m, 0);
			for(int d=0; d<GetNbDayPerMonth(refYear, m); d++)
			{
			for(int v=0; v<NB_VAR; v++)
			{
			float value = station.GetYear(refYear).GetData()[jd+d][v];
			if( value > -999)
			{
			if( v==TMIN || v==TMAX || v==TDEW)
			{
			double ccValue = value + ccMonthlyMean[m][v]-refMonthlyMean[m][v];
			ccMonthlyMeanF[v] += ccValue;
			}
			else if( v==PRCP || v==WNDS)
			{
			double ccValue = value * ccMonthlyMean[m][v]/refMonthlyMean[m][v];
			ccMonthlyMeanF[v] += ccValue;
			}
			else if( v==RELH )
			{
			//apply daily cc here
			double Tmin = station.GetYear(refYear).GetData()[jd+d][TMIN];
			double Tmax = station.GetYear(refYear).GetData()[jd+d][TMAX];
			if( Tmin>-999 && Tmax>-999)
			{
			double Tmean = (Tmin+Tmax)/2;
			double ccTmin = Tmin + ccMonthlyMean[m][TMIN]-refMonthlyMean[m][TMIN];
			double ccTmax = Tmax + ccMonthlyMean[m][TMAX]-refMonthlyMean[m][TMAX];
			double ccTmean = (ccTmin+ccTmax)/2;


			double Hs = Hr2Hs(Tmean, value);
			double ccHs = Hs*ccMonthlyMean[m][RELH]/refMonthlyMean[m][RELH];//specific humidity
			double ccRelH = Hs2Hr(ccTmean, ccHs);
			ccMonthlyMeanF[RELH] += ccRelH;
			ccMonthlyMeanF[TDEW] += Hr2Td(ccTmin, ccTmax, ccRelH);
			}
			}

			refMonthlyMeanF[v] += value;
			}
			}
			}

			for(int v=0; v<NB_VAR; v++)
			{
			std::string tmp;

			short s = (v==PRCP)?SUM:MEAN;
			if( v==PRCP || v==WNDS || v==RELH)
			tmp.Format("%4d,%4d,%2d,%s,%.6lg,%.6lg,%.1lf,%.1lf,%.4lf\n", refYear, ccYear, m+1, VAR_NAME[v], refMonthlyMean[m][v], ccMonthlyMean[m][v], refMonthlyMeanF[v][s], ccMonthlyMeanF[v][s], ccMonthlyMeanF[v][s]/refMonthlyMeanF[v][s]);
			else tmp.Format("%4d,%4d,%2d,%s,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf\n", refYear, ccYear, m+1, VAR_NAME[v], refMonthlyMean[m][v], ccMonthlyMean[m][v], refMonthlyMeanF[v][s], ccMonthlyMeanF[v][s], ccMonthlyMeanF[v][s]-refMonthlyMeanF[v][s]);

			file1.WriteString(tmp);
			}
			}
			}

			file1.Close();
			*/
		return msg;
	}
	//**********************************************************************************
	const char* CNormalFromDaily::XML_FLAG = "Daily2Normal";
	const char* CNormalFromDaily::MEMBER_NAME[NB_MEMBER] = { "InputFilePath", "FirstYear", "LastYear", "MinimumYears", "nbNeighbor", "OuputFilePath", "ApplyCC", "MMGFilePath", "refPeriodIndex", "CCPeriodIndex", "CreateAll" };

	CNormalFromDaily::CNormalFromDaily()
	{
		Reset();
	}

	void CNormalFromDaily::Reset()
	{
		m_inputDBFilePath.empty();
		m_outputDBFilePath.empty();

		m_firstYear = 1971;
		m_lastYear = 2000;
		m_nbYearMin = 10;
		m_nbNeighbor = 3;

		//climatic change section
		m_bApplyCC = false;
		m_inputMMGFilePath.empty();
		m_refPeriodIndex = 0;
		m_CCPeriodIndex = 11;
		m_bCreateAll = false;
	}

	std::string CNormalFromDaily::GetMember(size_t i)const
	{
		string str;
		string path = GetPath(m_filePath);

		switch (i)
		{
		case INPUT_DB: str = GetRelativePath(path, m_inputDBFilePath); break;
		case FIRST_YEAR: str = ToString(m_firstYear); break;
		case LAST_YEAR: str = ToString(m_lastYear); break;
		case MINIMUM_YEARS: str = ToString(m_nbYearMin); break;
		case NB_NEIGHBOR: str = ToString(m_nbNeighbor); break;
		case OUPUT_DB: str = GetRelativePath(path, m_outputDBFilePath.c_str()); break;
		case APPLY_CC: str = ToString(m_bApplyCC); break;
		case INPUT_MMG: str = GetRelativePath(path, m_inputMMGFilePath.c_str()); break;
		case REF_PERIOD_INDEX: str = ToString(m_refPeriodIndex); break;
		case CCPERIOD_INDEX: str = ToString(m_CCPeriodIndex); break;
		case CREATE_ALL: str = ToString(m_bCreateAll); break;
		default: ASSERT(false);
		}

		return str;
	}

	void CNormalFromDaily::SetMember(size_t  i, const std::string& str)
	{
		std::string path = GetPath(m_filePath);

		switch (i)
		{
		case INPUT_DB: m_inputDBFilePath = GetAbsolutePath(path, str); break;
		case FIRST_YEAR: m_firstYear = ToInt(str); break;
		case LAST_YEAR: m_lastYear = ToInt(str); break;
		case MINIMUM_YEARS: m_nbYearMin = ToInt(str); break;
		case NB_NEIGHBOR: m_nbNeighbor = ToInt(str); break;
		case OUPUT_DB: m_outputDBFilePath = GetAbsolutePath(path, str); break;
		case APPLY_CC: m_bApplyCC = ToBool(str); break;
		case INPUT_MMG: m_inputMMGFilePath = GetAbsolutePath(path, str); break;
		case REF_PERIOD_INDEX: m_refPeriodIndex = ToInt(str); break;
		case CCPERIOD_INDEX: m_CCPeriodIndex = ToInt(str); break;
			//	case FIRST_APPLYED_CCYEAR: m_firstApplyedCCYear = ToInt(str); break;
		case CREATE_ALL: m_bCreateAll = ToBool(str); break;
		default: ASSERT(false);
		}
	}

	int CNormalFromDaily::GetNbDBCreate()
	{
		return (m_bApplyCC&&m_bCreateAll) ? 12 - m_CCPeriodIndex : 1;
	}

	int CNormalFromDaily::GetFirstYear(int i)
	{
		ASSERT(i >= 0 && i < GetNbDBCreate());
		return m_bApplyCC ? FIRST_YEAR_OF_FIRST_PERIOD + (m_CCPeriodIndex + i) * 10 : m_firstYear;
	}

	int CNormalFromDaily::GetLastYear(int i)
	{
		ASSERT(i >= 0 && i < GetNbDBCreate());
		return m_bApplyCC ? 1990 + (m_CCPeriodIndex + i) * 10 : m_lastYear;
	}

	std::string CNormalFromDaily::GetOutputFilePath(int i)
	{
		ASSERT(i >= 0 && i < GetNbDBCreate());
		string outputDBFilePath = m_outputDBFilePath;
		if (m_bCreateAll)
			outputDBFilePath = FormatA("%s %d-%d%s", GetPath(m_outputDBFilePath) + GetFileTitle(m_outputDBFilePath), GetFirstYear(i), GetLastYear(i), GetFileExtension(m_outputDBFilePath));

		return outputDBFilePath;
	}

	ERMsg CNormalFromDaily::CreateNormalDatabase(CCallback& callback)
	{
		ERMsg msg;

		CDailyDatabase inputDB;
		msg += inputDB.Open(m_inputDBFilePath, CDailyDatabase::modeRead, callback);

		CMonthlyMeanGrid MMG;
		if (m_bApplyCC)
			msg += MMG.Open(m_inputMMGFilePath);


		if (!msg)
			return msg;


		callback.PushTask("Create all MMG", GetNbDBCreate());
		for (int p = 0; p < GetNbDBCreate(); p++)
		{
			int firstYear = GetFirstYear(p);
			int lastYear = GetLastYear(p);
			std::string outputDBFilePath = GetOutputFilePath(p);


			//Open daily(read) and normal(write) database

			CNormalsDatabase outputDB;


			//		outputDB.SetBeginYear(firstYear);
			//		outputDB.SetEndYear(lastYear);
			msg += outputDB.Open(outputDBFilePath, CNormalsDatabase::modeEdit);


			if (!msg)
				return msg;


			callback.PushTask("Create " + GetFileTitle(outputDBFilePath), inputDB.size());
			callback.AddMessage("Create " + GetFileTitle(outputDBFilePath));
			//callback.SetNbStep(inputDB.size());


			//for all stations in daily database
			int nbStationAdded = 0;
			for (int i = 0; i < inputDB.size() && msg; i++)
			{
				CWeatherStation dailyStation;

				msg = inputDB.Get(dailyStation, i);

				if (m_bApplyCC)
				{
					//adjust daily data to reflect climatic change
					msg += ApplyClimaticChange(dailyStation, MMG, callback);
				}

				if (msg)
				{
					//remove years not in the period
					CleanUpYears(dailyStation, firstYear, lastYear);

					//if the station have enough years
					if (dailyStation.size() >= m_nbYearMin)
					{
						//create normal
						CAdvancedNormalStation station;
						//ERMsg messageTmp = station.FromDaily(dailyStation, m_nbYearMin);

						if (station.FromDaily(dailyStation, m_nbYearMin))
						{
							if (m_bApplyCC)
							{
								//now adjust standard deviation if they are present
								MMG.UpdateStandardDeviation(FIRST_YEAR_OF_FIRST_PERIOD + m_refPeriodIndex * 10, FIRST_YEAR_OF_FIRST_PERIOD + m_CCPeriodIndex * 10, m_nbNeighbor, m_maxDistance, m_power, station);
							}

							//add normal to database
							ERMsg messageTmp = outputDB.Add(station);
							if (messageTmp)
							{
								nbStationAdded++;
							}

							//TestWG(station);

							//messageTmp = outputDB.Add(station);
							//if (messageTmp)
							//{
							//	nbStationAdded++;
							//}

							if (!messageTmp)
								callback.AddMessage(messageTmp, 1);
						}
					}
				}

				msg += callback.StepIt();
			}

			callback.PopTask();
			outputDB.Close();



			if (msg)
			{
				callback.AddMessage(FormatMsg("Nb stations added = %1", ToString(nbStationAdded)), 1);

				//open the file to create optimization 
				msg = outputDB.Open(outputDBFilePath);

				//std::string locFilePath( m_outputFilePath );
				//UtilWin::SetFileExtension( locFilePath, ".loc");
				//CLocArray locArray;
				//CGeoRect bBox(-180, -90, 180, 90, CProjection::GEO);
				//msg += normalDB.GenerateLOC(bBox, false, 0, locArray, true );
				//msg += locArray.Save(locFilePath);
			}
		}


		inputDB.Close();
		MMG.Close();
		callback.PopTask();


		return msg;
	}

	void CNormalFromDaily::CleanUpYears(CWeatherStation& dailyStation, short firstYear, short lastYear)
	{
		//for (size_t y = 0; y < dailyStation.size(); y++)
		for (auto it = dailyStation.begin(); it != dailyStation.end();)
		{
			int year = it->first;
			if (year >= firstYear || year <= lastYear)
			{
				//CTRef TRef = dailyStation[y].GetTRef();
				if (!it->second->HaveData())
					it = dailyStation.erase(it);
				else
					it++;
			}
			else
			{
				it = dailyStation.erase(it);
			}
		}
	}

	ERMsg CNormalFromDaily::ApplyClimaticChange(CWeatherStation& dailyStation, CMonthlyMeanGrid& mmg, CCallback& callback)
	{
		ERMsg msg;


		CleanUpYears(dailyStation, m_firstYear, m_lastYear);
		if (dailyStation.size() >= m_nbYearMin)
		{
			//coord of the station
			if (!mmg.UpdateData(FIRST_YEAR_OF_FIRST_PERIOD + m_refPeriodIndex * 10, FIRST_YEAR_OF_FIRST_PERIOD + m_CCPeriodIndex * 10, m_nbNeighbor, m_maxDistance, m_power, dailyStation))
			{
				dailyStation.Reset();
			}
		}

		return msg;
	}

}