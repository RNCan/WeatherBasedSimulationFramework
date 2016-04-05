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
#include "stdafx.h"   
#include <math.h>
#include <algorithm>

#include "Basic/Statistic.h"
#include "Basic/WeatherCorrection.h"
#include "Basic/NormalsStation.h"
#include "Newmat/Regression.h"
#include "Simulation/WeatherGradient2.h"

#define CREATE_SHORE 0
#if CREATE_SHORE
#include "Geomatic/ShapeFileBase.h"
#endif

using namespace std;
using namespace NEWMAT;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::GRADIENT;


namespace WBSF
{


	const char* GetHemisphereName(size_t i)
	{
		ASSERT(i < NB_HEMISPHERE);
		static const char* HEMISPHERE_NAME[NB_HEMISPHERE] = { "Southern", "Northern" };

		return HEMISPHERE_NAME[i];
	}

	const char* GetGradientName(size_t i)
	{
		ASSERT(i < NB_GRADIENT);

		static const char* GRADIENT_NAME[NB_GRADIENT] = { "Tmin", "Tmax", "Prcp", "Tdew" };
		return GRADIENT_NAME[i];
	}

	const char* GetScaleName(size_t i)
	{
		ASSERT(i < NB_SCALE_GRADIENT);
		static const char* SCALE_NAME[NB_SCALE_GRADIENT] = { "Local", "Regional", "Continental" };

		return SCALE_NAME[i];
	}

	const char* GetSpaceName(size_t i)
	{
		ASSERT(i < NB_SPACE_EX);
		static const char* SPACE_NAME[NB_SPACE_EX] = { "Lon", "Lat", "Elv", "Dsh" };

		return SPACE_NAME[i];
	}

	size_t CWeatherGradient::GetNbSpaces(){ return (m_pShore  && !m_pShore->empty()) ? NB_SPACE_EX : NB_SPACE; }


	CApproximateNearestNeighborPtr CWeatherGradient::m_pShore;

	const double CWeatherGradient::FACTOR_Z = 1000;
	const int CWeatherGradient::NB_STATION_REGRESSION_LOCAL = 25;//23
	const double CWeatherGradient::REGIONAL_FACTOR = 3;
	const double CWeatherGradient::PPT_FACTOR = 1;

	//mean precipitation [mm]
	const CGradientS° CWeatherGradient::GLOBAL_S°[NB_HEMISPHERE][GRADIENT::NB_GRADIENT_EX] =
	{ 
		{//Southern
			{ +16.542, +16.354, +15.253, 13.448, 11.441, +9.698, +9.036, +9.703, 11.273, 13.084, +14.695, +15.897 },
			{ +30.713, +30.298, +29.127, 26.971, 24.433, 22.343, 21.885, 23.281, 25.308, 27.310, +28.826, +29.981 },
			{ 151.782, 140.296, 134.342, 98.112, 72.718, 54.398, 47.419, 43.453, 52.628, 75.942, 100.170, 132.733 },
			{ +16.002, +16.295, +15.324, 13.567, 11.586, +9.838, +8.771, +8.843, 10.040, 11.683, +13.407, +14.906 },
		},
		{//Northern
			{ -6.038, -4.805, -0.978, +4.044, +8.912, 12.928, 15.215, 14.522, 10.942, +5.971, +0.127, -4.434 },
			{ +3.627, +5.579, 10.064, 15.640, 20.794, 24.843, 27.103, 26.310, 22.269, 16.429, +9.681, +4.861 },
			{ 51.805, 45.060, 51.343, 54.667, 69.800, 82.311, 87.544, 84.614, 77.041, 69.883, 63.566, 57.252 },
			{ -4.768, -4.020, -0.823, +3.223, +7.819, 11.967, 14.473, 14.160, 10.863, +6.239, +0.975, -3.173 },
		}
	};

	const size_t CWeatherGradient::NB_S_MAX[NB_HEMISPHERE] = { 1000, 5000 };
	const CGeoRect CWeatherGradient::DEFAULT_RECT[NB_HEMISPHERE] = { CGeoRect(-180, -90, 180, 0, PRJ_WGS_84), CGeoRect(-180, 0, 180, 90, PRJ_WGS_84) };
	

	const double CWeatherGradient::A[NB_SCALE_GRADIENT] = { 2, 4, 8 };
	const double CWeatherGradient::B[NB_SCALE_GRADIENT] = { 4, 8, 16 };
	const double CWeatherGradient::F1[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT_EX] =
	{
		{ 10, 10, 10, 10},	//local
		{ 30, 30, 25, 25},	//Regional
		{ 90, 90, 80, 80},	//continental
	};
	
	const double CWeatherGradient::F2[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT_EX] =
	{
		{ 20, 20, 15, 15 },	//local
		{ 90, 90, 80, 80 },	//Regional
		{ 99, 99, 99, 99 },	//continental
	};

	

	//compute from 5000 stations in North Hemisphere and 1000 station in South Hemisphere
	const double CWeatherGradient::DEFAULT_GRADIENTS[NB_HEMISPHERE][NB_GRADIENT_EX][12][NB_SPACE_EX] =
	{
		{//Southern
			{//Tmin
				{ +0.1501, +2.7114, -4.0720, +0.3883 },
				{ +0.1547, +3.0803, -4.3039, -0.4297 },
				{ +0.1124, +3.7362, -4.4555, -1.5355 },
				{ +0.0730, +4.4044, -4.5744, -2.8112 },
				{ +0.0291, +4.7551, -4.7695, -4.1302 },
				{ -0.0159, +4.9327, -4.8813, -4.7029 },
				{ -0.0338, +5.0313, -4.8209, -4.6392 },
				{ -0.0258, +5.0042, -4.5652, -3.4607 },
				{ +0.0301, +4.8251, -4.3252, -1.9395 },
				{ +0.0667, +4.3891, -4.1228, -0.5486 },
				{ +0.1088, +3.6354, -4.0136, +0.0820 },
				{ +0.1297, +2.9928, -3.9880, +0.4030 },
			},
			{//Tmax
				{ +0.0369, +3.8202, -4.1570, +7.1764 },
				{ +0.0239, +4.1710, -4.3229, +5.9834 },
				{ -0.0029, +4.9182, -4.6038, +5.1487 },
				{ -0.0240, +5.6934, -4.6914, +4.2211 },
				{ -0.0502, +6.2248, -4.7292, +2.7883 },
				{ -0.0874, +6.4998, -4.9496, +2.2888 },
				{ -0.0905, +6.5777, -4.9812, +2.8999 },
				{ -0.0822, +6.4960, -4.6994, +5.0791 },
				{ -0.0200, +6.2426, -4.1919, +6.9073 },
				{ -0.0025, +5.6663, -3.9840, +7.8419 },
				{ +0.0211, +4.8498, -3.8341, +7.6312 },
				{ +0.0244, +4.1330, -3.9240, +7.4345 },
			},
			{//Prcp
				{ -2.2818, +35.6700, -28.2155, +20.3259 },
				{ -1.4871, +37.9047, -33.8860, -01.9099 },
				{ -2.7280, +38.9591, -45.7537, -00.6736 },
				{ -2.6016, +26.3877, -49.9415, -14.4330 },
				{ -1.5897, +13.3773, -41.6551, -34.6773 },
				{ -0.8179, +02.0876, -31.3235, -39.5551 },
				{ -0.6283, -00.6093, -25.5538, -44.2555 },
				{ -0.6912, -02.9476, -21.6993, -28.5165 },
				{ -1.4918, -02.9255, -20.5438, -06.2750 },
				{ -2.2072, +01.4635, -18.8986, +20.9203 },
				{ -2.3195, +11.6999, -17.6462, +38.8327 },
				{ -2.4502, +23.5352, -18.9925, +40.3900 },
				//{0.9852, 1.4709, 0.8855, 1.2805},
				//{0.9883, 1.4757, 0.8493, 1.1053},
				//{0.9734, 1.4776, 0.7614, 0.9491},
				//{0.9627, 1.4079, 0.5193, 0.7781},
				//{0.9699, 1.1318, 0.3657, 0.5123},
				//{0.9796, 0.9265, 0.3685, 0.3382},
				//{0.9839, 0.8596, 0.3751, 0.2363},
				//{0.9630, 0.7488, 0.4402, 0.5328},
				//{0.9541, 0.8107, 0.5651, 1.1056},
				//{0.9600, 0.9326, 0.7746, 1.8316},
				//{0.9713, 1.1577, 0.8622, 1.9517},
				//{0.9806, 1.3785, 0.8812, 1.6007},

			},
			{//Tdew
				{ -0.0728, +3.7361, -5.0168, -0.6015 },
				{ -0.0489, +3.8196, -5.2206, -0.6059 },
				{ -0.1013, +4.2219, -5.3556, -1.6714 },
				{ -0.1061, +4.6777, -5.6728, -2.6143 },
				{ -0.0904, +4.8272, -6.3580, -3.3272 },
				{ -0.0825, +4.9102, -6.7385, -3.4658 },
				{ -0.0919, +4.9284, -6.6081, -4.6105 },
				{ -0.1184, +4.8723, -6.4635, -5.0428 },
				{ -0.0928, +4.8030, -6.2588, -4.5013 },
				{ -0.1186, +4.6173, -5.8566, -2.8315 },
				{ -0.0928, +4.2880, -5.4139, -1.6828 },
				{ -0.0881, +3.9675, -5.1199, -0.8193 },
			},
		},
		{//Northern
			{//Tmin
				{ -0.1648, -6.8418, -2.6711, -7.8028 },
				{ -0.1497, -6.9379, -2.7535, -6.3639 },
				{ -0.0640, -6.4590, -3.2703, -3.2062 },
				{ +0.0332, -5.4912, -4.0628, -0.0867 },
				{ +0.0673, -4.3178, -4.2066, +1.0177 },
				{ +0.0705, -3.3286, -4.2215, +1.7720 },
				{ +0.0695, -2.7634, -3.8330, +1.6409 },
				{ +0.0584, -2.9807, -3.6997, +0.5991 },
				{ +0.0391, -3.6934, -3.6599, -1.1626 },
				{ +0.0151, -4.6773, -3.7395, -2.7160 },
				{ -0.0923, -5.9076, -3.6701, -5.0017 },
				{ -0.1329, -6.5246, -3.1865, -7.3437 },
			},
			{//Tmax
				{ -0.2587, -7.8254, -2.3254, -5.4359 },
				{ -0.2462, -7.8073, -2.5248, -3.6357 },
				{ -0.1757, -7.2857, -3.0735, -0.5145 },
				{ -0.0986, -6.3587, -3.9661, +2.7416 },
				{ -0.0528, -5.1471, -4.0395, +4.2110 },
				{ -0.0474, -4.0136, -3.7432, +4.8364 },
				{ -0.0751, -3.3087, -3.1690, +4.3988 },
				{ -0.0955, -3.6420, -3.1076, +3.5036 },
				{ -0.1255, -4.6555, -3.1623, +1.9634 },
				{ -0.1374, -5.9528, -3.3060, +0.0883 },
				{ -0.1901, -7.1949, -3.2259, -2.7585 },
				{ -0.2150, -7.6082, -2.7562, -5.1985 },
			},
			{//prcp
				{ -1.7343, -03.3299, -02.5330, -38.2934 },
				{ -1.3050, -04.6002, -01.4376, -30.5371 },
				{ -1.1678, -06.8949, -02.4600, -26.2476 },
				{ -0.4802, -11.0763, -05.6527, -13.8631 },
				{ +0.0674, -18.0363, -12.1214, -05.3803 },
				{ +1.1816, -21.2772, -14.6317, -05.9443 },
				{ +2.3223, -20.8544, -11.3473, -06.7425 },
				{ +2.2537, -19.9290, -09.9923, -16.0656 },
				{ +0.8535, -17.9371, -12.7215, -27.2522 },
				{ -0.3647, -12.7339, -13.0829, -35.6856 },
				{ -1.3347, -08.2779, -08.7235, -40.1016 },
				{ -1.6499, -05.3256, -05.1333, -39.9362 },
				//{0.9455, 1.0445, 0.8464, 0.3500},
				//{0.9513, 0.9873, 0.8905, 0.3700},
				//{0.9594, 0.9268, 0.9029, 0.4810},
				//{0.9796, 0.8792, 0.8886, 0.6959},
				//{0.9915, 0.8446, 0.8496, 0.9661},
				//{1.0063, 0.8783, 0.8255, 1.1060},
				//{1.0184, 0.9033, 0.8520, 1.1807},
				//{1.0157, 0.8971, 0.8558, 0.9371},
				//{0.9964, 0.8703, 0.8122, 0.6842},
				//{0.9774, 0.9056, 0.7952, 0.5108},
				//{0.9590, 0.9679, 0.7466, 0.4107},
				//{0.9456, 1.0191, 0.7672, 0.3802},

			},
			{ //Tdew
				{ -0.2935, -5.5482, -3.9213, -8.5562 },
				{ -0.2611, -5.6090, -3.9234, -7.3837 },
				{ -0.1709, -5.2239, -4.4954, -4.6747 },
				{ -0.0600, -4.7142, -5.0108, -2.5893 },
				{ -0.0179, -3.9528, -5.0726, -1.3537 },
				{ +0.0420, -3.1503, -5.2801, +0.3314 },
				{ +0.0997, -2.6322, -5.1298, +0.8237 },
				{ +0.0849, -2.8216, -5.0116, -0.1684 },
				{ +0.0107, -3.4924, -4.8803, -2.2014 },
				{ -0.0714, -4.2382, -4.9142, -3.9638 },
				{ -0.2024, -5.0496, -4.7949, -6.0284 },
				{ -0.2638, -5.4108, -4.2827, -7.9538 },
			},
		},
	};

	CWeatherGradient::CWeatherGradient()
	{
		reset();
	}

	//"D:\\Weather\\Normals\\Shore_z3.shp"
	//"D:\\Weather\\Normals\\Shore_z3.ANN"
	ERMsg CWeatherGradient::Shape2ANN(const string& fielpathIn, const string& filePathOut)
	{
		ERMsg msg;

#if CREATE_SHORE
		CShapeFileBase shape;

		msg = shape.Read(fielpathIn);
		if (msg)
		{
			ofStream stream;
			msg = stream.open(filePathOut, std::ios::binary);
			if (msg)
			{
				CApproximateNearestNeighbor ANN;
				CLocationVector locations(shape.GetNbShape());
				for (size_t i = 0; i < shape.GetNbShape(); i++)
					shape[int(i)].GetShape().GetCentroid(locations[i]);

				ANN.set(locations, false);
				stream << ANN;
				stream.close();
			}
		}
#endif
		return msg;
	}

	ERMsg CWeatherGradient::SetShore(const std::string& filePath)
	{
		ERMsg msg;

		if (filePath.empty())
		{
			m_pShore.reset();
		}
		else
		{
			ifStream stream;
			msg = stream.open(filePath, std::ios::binary);
			if (msg)
			{
				if (m_pShore.get() == NULL)
					m_pShore = make_shared<CApproximateNearestNeighbor>();

				*m_pShore << stream;
				stream.close();

			}
		}

		return msg;
	}


	//"U:\\Geomatique\\Shapefile\\water\\water-polygons-generalized-3857\\water_polygons_z3.shp"
	ERMsg CWeatherGradient::AddShape(const string& filepathIn1, const string& filepathIn2, const string& filePathOut)
	{
		ERMsg msg;
#if CREATE_SHORE
		CShapeFileBase shapeIn;

		msg = shapeIn.Read(filepathIn1);
		if (msg)
		{
			CNormalsDatabase m_normalDB;
			msg = m_normalDB.Open(filepathIn2);
			if (msg)
			{
				CLocationVector locations;
				for (size_t i = 0; i < m_normalDB.size(); i++)
				{

					if (shapeIn.IsInside(m_normalDB[i]))
					{
						double d = shapeIn.GetMinimumDistance(m_normalDB[i]);
						if (d>5000)
							locations.push_back(m_normalDB[i]);
					}
				}

				locations.Save(filePathOut);
			}
		}
#endif
		return msg;
	}

	
	void CWeatherGradient::reset()
	{
		ASSERT(NB_GRADIENT_EX == 4);
		ASSERT(NB_SPACE_EX == 4);

		

		CWeatherCorrections::reset();
		
		m_bForceComputeAllScale = false;

		for (size_t z = 0; z < NB_HEMISPHERE; z++)
			for (size_t g = 0; g < NB_GRADIENT_EX; g++)
				for (size_t m = 0; m < 12; m++)
					m_gradient[z][g][m].fill(0);


	}
	
	double CWeatherGradient::GetShoreDistance(const CLocation& location)
	{
		double d = 0;
		if (m_pShore)
		{
			CSearchResultVector shorePt;
			if (m_pShore->search(location, 1, shorePt) )
			{
				d = shorePt.front().m_distance;
			}
		}

		return d;
	}

	double CWeatherGradient::GetDistance(size_t s, const CLocation& target, const CLocation& station)const
	{
		double d = 0;
		switch (s)
		{
		case X_GR: d = station.GetDistanceXY(target).m_x / 1000; break;//[km]
		case Y_GR: d = station.GetDistanceXY(target).m_y / 1000; break;//[km]
		case Z_GR: d = target.m_z - station.m_z; break; //[m]
		case S_GR: 
			//if (m_lastTarget != target || m_lastStation != station)
			//{
			//	CWeatherGradient& me = const_cast<CWeatherGradient&>(*this);
			//	me.m_lastTarget = target;
			//	me.m_lastStation = station;
			//	me.m_lastDeltaShore = (GetShoreDistance(target) - GetShoreDistance(station)) / 1000; break;//[km]
			//}
			
			/*std::map<CGeoPoint3D, double>::const_iterator Tit = m_shoreCache.find(target);
			std::map<CGeoPoint3D, double>::const_iterator Sit = m_shoreCache.find(station);
			
			if (Tit == m_shoreCache.end())
			{
				me.m_shoreCache.insert(target, GetShoreDistance(target), );
				Tit = m_shoreCache.find(target);
			}
				

			if (Sit == m_shoreCache.end())
			{
				me.m_shoreCache.insert(station, GetShoreDistance(station));
				Sit = m_shoreCache.find(station);
			}
				

			double Td = 0;
			double Sd = 0;	Td = Tit->second;*/
			d = (GetShoreDistance(target) - GetShoreDistance(station)) / 1000; break;//[km]
			
		default: ASSERT(false);
		}

		return d;
	}

	double CWeatherGradient::GetFactor(size_t z, size_t g, size_t s, const CSearchResultVector& results)const
	{
		if (z == CONTINENTAL_GRADIENT)
			return 1;

		if (s == S_GR && !(m_pShore&&!m_pShore->empty()))
			return 1;

		CLocation L°;
		results.GetCentroid(L°);
		//CLocation L°("0", "0", 0, 0, 0);
		

		CStatisticEx M°;
		for (size_t i = 0; i<results.size(); i++)
		{
			const CLocation& st = m_pNormalDB->at(results[i].m_index);
			M° += GetDistance(s, st, L°);
		}

		double mean = GetDistance(s, m_target, L°);
		double mad = M°[MAD];

		//if mad == 0 take half regional and local gradient 
		double ff = mad>0 ? (mean - A[z]*mad) / (B[z]*mad) : 0.5;
		double f = 1 - min(1.0, max(0.0, ff));

		double correction = 1;
		//if (g == PRCP_GR )//&& s == Z_GR
		//{
			//elevation precipitation gradient is invalid if they are ouside in lat lon
			//alternative
			//f = min(m_factor[z][g][X_GR], min(m_factor[z][g][Y_GR], f));

			double f1 = F1[z][g];
			double f2 = F2[z][g];
			
			double Dc = m_target.GetDistance(L°, true) / 1000;// take elevation in distance of centroid km

			correction = 1 - min(1.0, max(0.0, Dc - f1) / f2);
		//}


		f *= correction;

		//for optimisation if the factror is over 95% we take 100 and below 5% we take 0
		if (f > 0.95)
			f = 1;

		if (f < 0.05)
			f = 0;
	
		return f;
	}


	ERMsg CWeatherGradient::CreateDefaultGradient(CCallback& callback)
	{
		ERMsg msg;

		static const double FACTOR_DISTRIBUTE = 0.25;
		callback.PushTask("Create Default Gradient", NB_HEMISPHERE*m_variables.count());


		for (size_t z = 0; z < NB_HEMISPHERE; z++)
		{
			//DEFAULT_RECT[z] = CGeoRect(-180, -90 + double(z)*(180 / NB_HEMISPHERE), 180, -90 + double(z + 1) * (180 / NB_HEMISPHERE), PRJ_WGS_84);

			for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
			{
				if (m_variables[v])
				{
					callback.AddMessage("**************************************************************************************");
					callback.AddMessage(GetVariableTitle(v));

					CSearchResultVector results;

					ERMsg msgTmp = m_pNormalDB->GetStationList(results, v, YEAR_NOT_INIT, true, DEFAULT_RECT[z]);

					if (msgTmp)
						msgTmp = m_pNormalDB->GenerateLOC(results, CWeatherDatabase::WELL_DISTRIBUTED_STATIONS, min(results.size(), NB_S_MAX[z]), v, YEAR_NOT_INIT, true, true, DEFAULT_RECT[z], callback);

					if (msgTmp)
					{
						size_t g = V2G(v);

						msg = ComputeGradient(g, results, m_gradient[z][g], m_R²[z][g], callback);

						
						GetS°(g, results, m_S°[z][g]);
					}
				}
			}
		}

		callback.PopTask();

		return msg;
	}

	
	ERMsg CWeatherGradient::CreateGradient(CCallback& callback)
	{
		ERMsg msg;

		//callback.SetNbStep(NB_SCALE_GRADIENT*m_variables.count()*12);
		callback.PushTask("Create gradient", NB_SCALE_GRADIENT*m_variables.count() * 12, 1);

		size_t e = NORTH_HEMISPHERE;
		if (m_target.m_lat < 0)
			e = SOUTH_HEMISPHERE;

		size_t nbSpaces = GetNbSpaces();

		for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
			for (size_t g = 0; g < NB_GRADIENT_EX; g++)
				m_factor[z][g].fill(0);
		

		for (TVarH v = H_TAIR; v < NB_VAR_H&&msg; v++)
		{
			size_t g = V2G(v);
			if (m_variables[v] && g<NB_GRADIENT_EX)
			{
				for (size_t z = 0; z < NB_SCALE_GRADIENT&&msg; z++)
				{
					double sum=0;
					for (size_t zz = 0; zz < z; zz++)
						for (size_t s = 0; s < nbSpaces; s++)
							sum += m_factor[zz][g][s] / nbSpaces;

					if (sum<1 || m_bForceComputeAllScale)
					{
						CSearchResultVector results;

						if (z < CONTINENTAL_GRADIENT)
						{
							size_t nbStations = size_t(NB_STATION_REGRESSION_LOCAL*pow(3, z));
							ERMsg msgTmp = m_pNormalDB->Search(results, m_target, nbStations, v);

							if (msgTmp)
								msg = ComputeGradient(g, results, m_gradient[z][g], m_R²[z][g], callback);
							else  if (!m_allowDerivedVariables[v])
								msg += msgTmp;
							
							
							GetS°(g, results, m_S°[z][g]);
						}
						else
						{
							for (size_t m = 0; m < 12; m++)
								for (size_t s = 0; s < NB_SPACE_EX; s++)
									m_gradient[z][g][m][s] = DEFAULT_GRADIENTS[e][g][m][s];

							
							m_S°[z][g] = GLOBAL_S°[e][g];
						}
							

						//compute factor
						for (size_t s = 0; s < NB_SPACE_EX; s++)
						{
							double f = GetFactor(z, g, s, results);
							//deduce the factor of finer scale
							for (size_t zz = 0; zz < z; zz++)
								f -= m_factor[zz][g][s];

							m_factor[z][g][s] = max(0.0, min(1.0, f));
						}
					}

					msg += callback.StepIt(0);
				}//for all scale

				for (size_t s = 0; s < NB_SPACE_EX; s++)
				{
					double f = 0;
					for (size_t z = 0; z < NB_SCALE_GRADIENT&&msg; z++)
						f += m_factor[z][g][s];

					ASSERT(f == 1);
				}
			}//if selected variable
		}//all variable
		
		
		callback.PopTask();
		return msg;
	}

	void CWeatherGradient::GetS°(size_t g, const CSearchResultVector& results, CGradientS°& S°)const
	{
		size_t f = G2F(g);
		for (size_t i = 0; i < results.size(); i++)
		{
			CNormalsStation station;
			m_pNormalDB->Get(station, results[i].m_index);
			
			for (size_t m = 0; m < 12; m++)
			{
				ASSERT(!IsMissing(station[m][f]));
				S°[m] += station[m][f];
			}
		}
	}

	ERMsg CWeatherGradient::ComputeGradient(size_t g, CSearchResultVector& results, CGradientYear& Gr, CGradientR²& R², CCallback& callback)
	{
		ERMsg msg;

		//callBack.SetCurrentDescription(GetVariableTitle(v));
		//callback.SetNbStep(12);


		size_t f = G2F(g);

		size_t nbSpaces = GetNbSpaces();
		NEWMAT::Matrix M((int)results.size(), (int)nbSpaces); //lat, lon, elev, Dshore
		array<NEWMAT::ColumnVector, 12> V;//12 months

		for (size_t m = 0; m < 12; m++)
			V[m].ReSize((int)results.size());

		//CLocation L°("0", "0", 0, 0, 0);
		CLocation L°;
		results.GetCentroid(L°);
		CGradientS° S°;
		GetS°(g, results, S°);

		for (int i = 0; i < (int)results.size() && msg; i++)
		{
			CNormalsStation station;
			m_pNormalDB->Get(station, results[i].m_index);

			M[i][X_GR] = GetDistance(X_GR, station, L°) / 1000;	//1000 km
			M[i][Y_GR] = GetDistance(Y_GR, station, L°) / 1000;	//1000 km
			M[i][Z_GR] = GetDistance(Z_GR, station, L°) / 1000;	//1000 m
			
			if (D_SHORE < nbSpaces)
				M[i][S_GR] = GetDistance(S_GR, station, L°) / 1000;	//1000 km
			
			for (size_t m = 0; m < 12; m++)
			{
				//double x = (S°[m][NB_VALUE] * station[m][f] / S°[m][HIGHEST] + 1) / (S°[m][NB_VALUE] + 2);
				//if (g==PRCP_GR)
					//V[m][i] = log(x/(1-x));
				//else 
					V[m][i] = station[m][f];
				ASSERT(!IsMissing(V[m][i]));
			}

			msg += callback.StepIt(0);
		}


		Try
		{
			NEWMAT::ColumnVector fitted; 
			NEWMAT::ColumnVector result;

			for (size_t m = 0; m < 12&&msg; m++)
			{
				R²[m] = DoRegression(M, V[m], result, fitted);
				for (size_t s = 0; s < nbSpaces; s++)
				{
					int i = int(s) + 1;
					
					//if (g == PRCP_GR)
						//Gr[m][s] = exp(result[i]);
					//else
						Gr[m][s] = result[i];
				}

				msg += callback.StepIt();
			}
		}
		Catch(NEWMAT::Exception)
		{
			msg.asgType(ERMsg::ERREUR);

			if (NEWMAT::Exception::what() != NULL)
				msg.ajoute(NEWMAT::Exception::what());
		}

		return msg;
	}

	double CWeatherGradient::GetCorrection(const CLocation& station, size_t m, size_t g, size_t s)const
	{
		ASSERT((m_factor[0][g][s] + m_factor[1][g][s] + m_factor[2][g][s]) == 1);

		double correction = (g == PRCP_GR) ? 1 : 0;

		double delta = GetDistance(s, m_target, station) / 1000;

		if (g == TMIN_GR || g == TMAX_GR)
		{
			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
				correction += delta * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
		}
		else if (g == PRCP_GR)
		{
			double c = 0;
			double S°= 0;

			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
			{
				//c += delta * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
				//S° += (m_factor[z][g][s] * m_S°[z][g][m][MEAN]);/// nbSpaces 

				//double c = m_factor[z][g][s] * pow(m_gradient[z][g][m][s], delta);// / nbSpaces 
				//double cc = c*(m_factor[z][g][s] * m_S°[z][g][m][MEAN]);/// nbSpaces 

				c += delta * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
				S° += (m_factor[z][g][s] * m_S°[z][g][m][MEAN]);/// nbSpaces 
			}


			//le gradient de précipitation son désactiver pour l'instant...bug étrange???
			//correction = (S° + c) / S°;


			correction = 1;
			if (correction < 0.1)
				correction = 0.1;
			else if (correction > 10)
				correction = 10;
		}

		return correction;
	}

	double CWeatherGradient::GetCorrection(const CLocation& station, CTRef TRef, size_t v)const
	{
		double correction = (v == H_PRCP) ? 1 : 0;
		size_t g = V2G(v);
		size_t m = TRef.GetMonth();

		//array < double, NB_SPACE_EX> delta = { 0 };
		//for (size_t s = 0; s <NB_SPACE_EX; s++)
			//delta[s] = GetDistance(s, m_target, station) / 1000;


		if (v == H_PRES)
		{
			correction = GetPressure(station.m_alt) - GetPressure(m_target.m_alt);
		}
		else if (v == H_TAIR || v == H_TRNG)
		{
			for (size_t s = 0; s < NB_SPACE_EX; s++)
				correction += GetCorrection(station, m, g, s);

			if (TRef.GetType() == CTM::HOURLY)
			{
				double p = (sin(2 * PI*TRef.GetHour() / 24.0) + 1) / 2;//full Tmin gradient at 6:00 and full Tmax gradient at 18:00
				assert(p >= 0 && p <= 1);

				if (v == H_TAIR)
					correction *= p;
				else if (H_TRNG)
					correction *= (1 - p);
			}
		}
		else if (v == H_PRCP)
		{
			//size_t nbSpaces = GetNbSpaces();

			//double c = 0;
			//CStatistic S°;
				
			for (size_t s = 0; s < NB_SPACE_EX; s++)
				correction *= GetCorrection(station, m, g, s);
			//{
			//	c += delta[s] * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
				//S° += (m_factor[z][g][s] * m_S°[z][g][m][MEAN]);/// nbSpaces 
			//}

			//double cc = (S°[MEAN]+ c) / S°[MEAN];
			//correction *= cc;
			

			
			/*
			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
			{
				double f = 0;
				for (size_t s = 0; s < nbSpaces; s++)
					f += m_factor[z][g][s] / nbSpaces;

				S° += (f * m_S°[z][g][m][MEAN]);
			}

			ASSERT(S°[SUM]>0);
			correction = max(0.0, (S°[SUM] + correction)) / S°[SUM];*/
		}
		


		//add default gradient (if any)
	/*	for (size_t s = 0; s < nbSpaces; s++)
		{
			double f = 1;
			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
				f -= m_factor[z][g][s];

			if (f > 0)
			{
				ASSERT(f >= 0 && f <= 1);
				f = max(0.0, min(1.0, f));
				f *= delta[s] * DEFAULT_GRADIENTS[e][g][m][s];

				correction += f / PRCP_S°[z][m][MEAN];
			}
			
		}*/
		
		

		return correction;
	}

	//additive correction
	//double CWeatherGradient::GetCorrectionB0(const CLocation& station, CTRef TRef, size_t v)const
	//{
	//	ASSERT(v != SKIP);
	//	ASSERT(v == H_TAIR || v == H_TRNG || v == H_PRCP || v == H_TDEW || v == H_TMIN || v == H_TMAX);
	//	ASSERT(TRef.IsInit());
	//	ASSERT(station.IsInit());

	//	if (v == H_PRCP)
	//		return 0;//no additive correction for precipitation

	//	return GetCorrection(station, TRef, v);
	//}

	////multiplicative correction
	//double CWeatherGradient::GetCorrectionB1(const CLocation& station, CTRef TRef, size_t v)const
	//{
	//	ASSERT(v != SKIP);
	//	ASSERT(v == H_TAIR || v == H_TRNG || v == H_PRCP || v == H_TDEW || v == H_TMIN || v == H_TMAX);
	//	ASSERT(TRef.IsInit());
	//	ASSERT(station.IsInit());


	//	if (v != H_PRCP)
	//		return 1;//no multiplicative correction for other variables than precipitation

	//	
	//	double c = GetCorrection(station, TRef, v);

	//	size_t g = V2G(v);
	//	size_t m = TRef.GetMonth();

	//	CStatistic S°;
	//	size_t nbSpaces = GetNbSpace();
	//	for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
	//	{
	//		double f = 0;
	//		for (size_t s = 0; s < nbSpaces; s++)
	//			f += m_factor[z][g][s] / nbSpaces;

	//		S° += (f * m_prcpS°[z][m][MEAN]);
	//	}
	//		


	//	return (S°[SUM] + c) / S°[SUM];
	//}

	ERMsg CWeatherGradient::Save(const string& filePath)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath);

		if (msg)
		{
			file.write("zone,var,month,R²,X,Y,Z,S\n");

			for (size_t z = 0; z < NB_HEMISPHERE; z++)
			{
				for (size_t g = 0; g < NB_GRADIENT_EX; g++)
				{
					for (size_t m = 0; m < 12; m++)
					{
						string line = FormatA("%d,%d,%02d,%lf", z + 1, g + 1, m + 1,m_R²[z][g][m]); 
						for (size_t s = 0; s < NB_SPACE_EX; s++)
						{
							line += "," + FormatA("%+07.4lf", m_gradient[z][g][m][s]);
						}
							

						file.write(line + "\n");
					}
				}
			}

			for (size_t z = 0; z < NB_HEMISPHERE; z++)
			{
				for (size_t g = 0; g < NB_GRADIENT_EX; g++)
				{
					for (size_t m = 0; m < 12; m++)
					{
						string line = FormatA("%d,%d,%d,", z + 1, g+1, m + 1);
						line += "," + ToString(m_S°[z][g][m], 3);

						file.write(line + "\n");
					}
				}
			}
		}


		return msg;
	}

	ERMsg CWeatherGradient::ExportInput(const string& filePath, size_t v, CSearchResultVector& results)
	{
		ERMsg msg;
		
		ofStream file;

		msg = file.open(filePath);
		if (msg)
		{
			if (results.empty())
				m_pNormalDB->GetStationList(results, (TVarH)v);
				
			file.write("KeyID,Name,Lat,Lon,Alt,DShore,Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec\n");

			size_t f = V2F(v);

			string lines;
			for (size_t i = 0; i < results.size(); i++)
			{
				const CLocation& station = (*m_pNormalDB)[results[i].m_index];
				double D=0;

				if (m_pShore && !m_pShore->empty())
				{
					CSearchResultVector shoreResult;
					m_pShore->search(station, 1, shoreResult);
					ASSERT(shoreResult.size() == 1);

					D = station.GetDistance(shoreResult.front().m_location,false)/1000000;//at 1000 km
				}

				string line = FormatA("%s,%s,%lf,%lf,%lf,%lf",
					station.m_ID.c_str(),
					station.m_name.c_str(),
					station.m_lat,
					station.m_lon,
					station.m_elev,
					D
					);


				const CNormalsData& data = m_pNormalDB->GetData(results[i].m_index);

				//size_t e = station.m_lat < 0 ? 0 : 1;
				for (size_t m = 0; m < 12; m++)
				{
					//double a1 = data[m][f];
					//double a2 = GLOBAL_S°[e][PRCP_GR][m];
					line += FormatA(",%.1lf", data[m][f]);
				}
					


				file.write(line + "\n");
			}

			file.close();
		}

		return msg;
	}

	

	/*void CWeatherGradient::CreateDataset()
	{

		CProjection projection1;

		ERMsg msg = projection1.SetPrjStr("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
		if (msg)
		{
			CProjection projection2(true);

			CGeoPoint xy(-7910615, 5921757);
			xy.Reproject(CProjectionTransformationManager::Get(projection1.GetPrjID(), projection2.GetPrjID()));

		}
	}
*/
}