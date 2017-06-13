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
#include "Simulation/WeatherGradient.h"

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

		static const char* GRADIENT_NAME[NB_GRADIENT] = { "Tmin", "Tair", "Tmax", "Prcp", "Tdew" };
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

	//size_t CWeatherGradient::GetNbSpaces(){ return NB_SPACE; }
	size_t CWeatherGradient::GetNbSpaces(){ return (m_pShore  && !m_pShore->empty()) ? NB_SPACE_EX : NB_SPACE; }


	CApproximateNearestNeighborPtr CWeatherGradient::m_pShore;

	const double CWeatherGradient::FACTOR_Z = 1000;
	const int CWeatherGradient::NB_STATION_REGRESSION_LOCAL = 25;//23
	const double CWeatherGradient::REGIONAL_FACTOR = 3;
	const double CWeatherGradient::PPT_FACTOR = 1;

	//mean precipitation [mm]
	const CGradientSᵒ CWeatherGradient::GLOBAL_Sᵒ[NB_HEMISPHERE][GRADIENT::NB_GRADIENT] =
	{ 
		{//Southern
			{ { +16.542, +16.354, +15.253, 13.448, 11.441, +9.698, +9.036, +9.703, 11.273, 13.084, +14.695, +15.897 } },
			{ { +23.628, +23.326, +22.190, 20.209, 17.937, 16.020, 15.460, 16.492, 18.291, 20.197, 21.7605, +22.939 } },
			{ { +30.713, +30.298, +29.127, 26.971, 24.433, 22.343, 21.885, 23.281, 25.308, 27.310, +28.826, +29.981 } },
			{ { 151.782, 140.296, 134.342, 98.112, 72.718, 54.398, 47.419, 43.453, 52.628, 75.942, 100.170, 132.733 } },
			{ { +16.002, +16.295, +15.324, 13.567, 11.586, +9.838, +8.771, +8.843, 10.040, 11.683, +13.407, +14.906 } },
		},
		{//Northern
			{ { -6.038, -4.805, -0.978, +4.044, +8.912, 12.928, 15.215, 14.522, 10.942, +5.971, +0.127, -4.434 } },
			{ { -1.206, +0.387, +4.543, +9.842, 14.853, 18.886, 21.159, 20.416, 16.606, 11.200, +4.904, +0.213 } },
			{ { +3.627, +5.579, 10.064, 15.640, 20.794, 24.843, 27.103, 26.310, 22.269, 16.429, +9.681, +4.861 } },
			{ { 51.805, 45.060, 51.343, 54.667, 69.800, 82.311, 87.544, 84.614, 77.041, 69.883, 63.566, 57.252 } },
			{ { -4.768, -4.020, -0.823, +3.223, +7.819, 11.967, 14.473, 14.160, 10.863, +6.239, +0.975, -3.173 } },
		}
	};

	const size_t CWeatherGradient::NB_S_MAX[NB_HEMISPHERE] = { 1000, 5000 };
	const CGeoRect CWeatherGradient::DEFAULT_RECT[NB_HEMISPHERE] = { CGeoRect(-180, -90, 180, 0, PRJ_WGS_84), CGeoRect(-180, 0, 180, 90, PRJ_WGS_84) };
	

	const double CWeatherGradient::A[NB_SCALE_GRADIENT] = { 2, 4, 8 };
	const double CWeatherGradient::B[NB_SCALE_GRADIENT] = { 4, 8, 16 };
	const double CWeatherGradient::F1[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT] =
	{// Tmin Tair Tmax prcp Tdew
		{ 050, 050, 050, 010, 050 },	//local
		{ 150, 150, 150, 035, 150 },	//Regional
		{ 999, 999, 999, 999, 999 },	//continental
	};
	
	const double CWeatherGradient::F2[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT] =
	{// Tmin Tair Tmax prcp Tdew
		{ 050, 050, 050, 025, 050 },	//local
		{ 300, 300, 300, 075, 300 },	//Regional
		{ 999, 999, 999, 999, 999 },	//continental
	};

	

	//compute from 5000 stations in North Hemisphere and 1000 station in South Hemisphere
	const double CWeatherGradient::DEFAULT_GRADIENTS[NB_HEMISPHERE][NB_GRADIENT][12][NB_SPACE_EX] =
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
			{//Tair
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
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
			{//Tair
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
				{ +0.0000, +0.0000, +0.0000, +0.0000 },
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
		ASSERT(NB_GRADIENT == 5);
		ASSERT(NB_SPACE_EX == 4);

		CWeatherCorrections::reset();
		reset_data();
		m_bForceComputeAllScale = false;
	}

	void CWeatherGradient::reset_data()
	{
		for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
		{
			for (size_t g = 0; g < NB_GRADIENT; g++)
			{
				m_factor[z][g].fill(0);
				m_R²[z][g].fill(0);
				for (size_t m = 0; m < 12; m++)
					m_Sᵒ[z][g][m].clear();

				for (size_t m = 0; m < 12; m++)
					m_gradient[z][g][m].fill(0);
			}
		}
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

	double CWeatherGradient::GetDistance(size_t s, const CLocation& target, const CLocation& station)
	{
		double d = 0;
		switch (s)
		{
		case X_GR: d = station.GetDistanceXY(target).m_x / 1000; break;//[km]
		case Y_GR: d = station.GetDistanceXY(target).m_y / 1000; break;//[km]
		case Z_GR: d = target.m_z - station.m_z; break; //[m]
		case S_GR: d = (GetShoreDistance(target) - GetShoreDistance(station)) / 1000; break;//[km]
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

		CLocation Lᵒ;
		results.GetCentroid(Lᵒ);
		

		CStatisticEx Mᵒ;
		for (size_t i = 0; i<results.size(); i++)
		{
			const CLocation& st = m_pNormalDB->at(results[i].m_index);
			Mᵒ += GetDistance(s, st, Lᵒ);
		}

		double mean = GetDistance(s, m_target, Lᵒ);
		double mad = Mᵒ[MAD];

		//if mad == 0 take half regional and local gradient 
		double ff = mad>0 ? (mean - A[z]*mad) / (B[z]*mad) : 0.5;
		double f = 1 - min(1.0, max(0.0, ff));

		double correction = 1;
		

		double f1 = F1[z][g];
		double f2 = F2[z][g];
			
		double Dc = m_target.GetDistance(Lᵒ, true) / 1000;// take elevation in distance of centroid km
		correction = 1 - min(1.0, max(0.0, Dc - f1) / f2);


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

		
		reset_data();
		//static const double FACTOR_DISTRIBUTE = 0.25;
		callback.PushTask("Create Default Gradient", NB_HEMISPHERE*m_variables.count());


		for (size_t e = 0; e < NB_HEMISPHERE&&msg; e++)//warning : here we stok hemispohe instead of scale
		{
			//DEFAULT_RECT[z] = CGeoRect(-180, -90 + double(z)*(180 / NB_HEMISPHERE), 180, -90 + double(z + 1) * (180 / NB_HEMISPHERE), PRJ_WGS_84);

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&msg; v++)
			{
				if (m_variables[v])
				{
					callback.AddMessage("**************************************************************************************");
					callback.AddMessage(GetVariableTitle(v));

					CSearchResultVector results;

					ERMsg msgTmp = m_pNormalDB->GetStationList(results, v, YEAR_NOT_INIT, true, DEFAULT_RECT[e]);

					if (msgTmp)
						msgTmp = m_pNormalDB->GenerateLOC(results, CWeatherDatabase::WELL_DISTRIBUTED_STATIONS, min(results.size(), NB_S_MAX[e]), v, YEAR_NOT_INIT, true, true, DEFAULT_RECT[e], callback);

					if (msgTmp)
					{
						size_t g = V2G(v);

						msg = ComputeGradient(g, results, m_gradient[e][g], m_R²[e][g], callback);

						
						GetSᵒ(g, results, m_Sᵒ[e][g]);
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

		reset_data();

		//commflic in multiThread
		callback.PushTask("Create gradient", NB_SCALE_GRADIENT*m_variables.count() * 12, 1);

		size_t e = NORTH_HEMISPHERE;
		if (m_target.m_lat < 0)
			e = SOUTH_HEMISPHERE;

		size_t nbSpaces = GetNbSpaces();

		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&msg; v++)
		{
			size_t g = V2G(v);
			if (m_variables[v] && g<NB_GRADIENT && g!=1)
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
							ERMsg msgTmp = m_pNormalDB->Search(results, m_target, nbStations, -1, v);

							if (msgTmp)
								msg = ComputeGradient(g, results, m_gradient[z][g], m_R²[z][g], callback);
							else  if (!m_allowDerivedVariables[v])
								msg += msgTmp;
							
							
							GetSᵒ(g, results, m_Sᵒ[z][g]);
						}
						else
						{
							for (size_t m = 0; m < 12; m++)
								for (size_t s = 0; s < GetNbSpaces(); s++)
									m_gradient[z][g][m][s] = DEFAULT_GRADIENTS[e][g][m][s];

							
							m_Sᵒ[z][g] = GLOBAL_Sᵒ[e][g];
						}
							

						//compute factor
						for (size_t s = 0; s < GetNbSpaces(); s++)
						{
							double f = GetFactor(z, g, s, results);
							
							double ff = 1;
							//The factor of this sacle is apply only on the residual of the finer scale
							for (size_t zz = 0; zz < z; zz++)
								ff -= m_factor[zz][g][s];

							m_factor[z][g][s] = max(0.0, min(1.0, f*ff));
						}
					}


					msg += callback.StepIt();
				}//for all scale

				for (size_t s = 0; s < GetNbSpaces(); s++)
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

	void CWeatherGradient::GetSᵒ(size_t g, const CSearchResultVector& results, CGradientSᵒ& Sᵒ)const
	{
		size_t f = G2F(g);
		for (size_t i = 0; i < results.size(); i++)
		{
			CNormalsStation station;
			m_pNormalDB->Get(station, results[i].m_index);
			
			for (size_t m = 0; m < 12; m++)
			{
				ASSERT(!IsMissing(station[m][f]));
				Sᵒ[m] += station[m][f];
			}
		}
	}

	ERMsg CWeatherGradient::ComputeGradient(size_t g, CSearchResultVector& results, CGradientYear& Gr, CGradientR²& R², CCallback& callback)
	{
		ERMsg msg;

		size_t f = G2F(g);

		size_t nbSpaces = GetNbSpaces();
		NEWMAT::Matrix M((int)results.size(), (int)nbSpaces); //lat, lon, elev, Dshore
		array<NEWMAT::ColumnVector, 12> V;//12 months

		for (size_t m = 0; m < 12; m++)
			V[m].ReSize((int)results.size());

		CLocation Lᵒ;
		results.GetCentroid(Lᵒ);
		//CGradientSᵒ Sᵒ;
		//GetSᵒ(g, results, Sᵒ);

		for (int i = 0; i < (int)results.size() && msg; i++)
		{
			CNormalsStation station;
			m_pNormalDB->Get(station, results[i].m_index);


			//M[i][X_GR] = GetDistance(X_GR, station, Lᵒ) / 1000;	//1000 km
			//M[i][Y_GR] = GetDistance(Y_GR, station, Lᵒ) / 1000;	//1000 km
			//M[i][Z_GR] = GetDistance(Z_GR, station, Lᵒ) / 1000;	//1000 m
			//
			//if (D_SHORE < nbSpaces)
			//	M[i][S_GR] = GetDistance(S_GR, station, Lᵒ) / 1000;	//1000 km
			
			M[i][X_GR] = GetDistance(X_GR, station, m_target) / 1000;	//1000 km
			M[i][Y_GR] = GetDistance(Y_GR, station, m_target) / 1000;	//1000 km
			M[i][Z_GR] = GetDistance(Z_GR, station, m_target) / 1000;	//1000 m

			if (D_SHORE < nbSpaces)
				M[i][S_GR] = GetDistance(S_GR, station, m_target) / 1000;	//1000 km


			for (size_t m = 0; m < 12; m++)
			{
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
					if (!_isnan(result[i]) && isfinite(result[i]))
					{
						Gr[m][s] = result[i];
					}
					else
					{

					}
				}

				msg += callback.StepIt(0);
			}
		}
		Catch(NEWMAT::Exception)
		{
			msg.asgType(ERMsg::ERREUR);

			if (NEWMAT::Exception::what() != NULL)
				msg.ajoute(NEWMAT::Exception::what());

			//MessageBox(NULL, L"Matrix exception", L"Error", MB_OK);
		}

		return msg;
	}

	double CWeatherGradient::GetCorrectionII(const CLocation& station, size_t m, size_t g, size_t s)const
	{
		ASSERT(s>=GetNbSpaces() || (m_factor[0][g][s] + m_factor[1][g][s] + m_factor[2][g][s]) == 1);

		double correction = (g == PRCP_GR) ? 1 : 0;

		double delta = GetDistance(s, m_target, station) / 1000; //1000 km or 1000 m

		if (g == TMIN_GR || g == TAIR_GR || g == TMAX_GR)
		{
			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
				correction += delta * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
		}
		else if (g == PRCP_GR)
		{
			//double c = 0;
			//double Sᵒ= 0;

			//for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
			//{
			//	c += delta * m_factor[z][g][s] * m_gradient[z][g][m][s];// / nbSpaces 
			//	Sᵒ += (m_factor[z][g][s] * m_Sᵒ[z][g][m][MEAN]);/// nbSpaces 
			//}
			//
			//ASSERT(Sᵒ > 0);

			//if (Sᵒ>0)
			//	correction = (Sᵒ + c) / Sᵒ;
			////correction = 1;//temporaire pour test

			//if (correction < 0.1)
			//	correction = 0.1;
			//else if (correction > 10)
			//	correction = 10;
		}

		return correction;
	}

	double CWeatherGradient::GetCorrection(const CLocation& station, CTRef TRef, size_t v)const
	{
		double correction = (v == H_PRCP) ? 1 : 0;
		size_t g = V2G(v);
		size_t m = TRef.GetMonth();

		if (v == H_PRES)
		{
			correction = (GetPressure(station.m_alt) - GetPressure(m_target.m_alt)) / 100; //correction in [hPa]
		}
		else if (v == H_TMIN2 || v == H_TAIR2 || v == H_TMAX2)
		{
			if (TRef.GetType() == CTM::HOURLY)
			{
				double cTmin = 0;
				double cTmax = 0;

				for (size_t s = 0; s < GetNbSpaces(); s++)
				{
					cTmin += GetCorrectionII(station, m, TMIN_GR, s);
					cTmax += GetCorrectionII(station, m, TMAX_GR, s);
				}

				double p = (sin(2 * PI*TRef.GetHour() / 24.0) + 1) / 2;//full Tmin gradient at 6:00 and full Tmax gradient at 18:00
				assert(p >= 0 && p <= 1);

				correction = cTmin*p + cTmax*(1 - p);

			}
			else
			{
				if (v == H_TAIR2)
				{
					//pour l'instant je laisse les gradients moyen, mais a changer pour le gradient Tair  : ajouter Tair Pres dans normals
					for (size_t s = 0; s < GetNbSpaces(); s++)
						correction += (GetCorrectionII(station, m, TMIN_GR, s) + GetCorrectionII(station, m, TMAX_GR, s)) / 2;
				}
				else
				{
					for (size_t s = 0; s < GetNbSpaces(); s++)
						correction += GetCorrectionII(station, m, g, s);
				}
			}
		}
		else if (v == H_PRCP)
		{
			for (size_t s = 0; s < GetNbSpaces(); s++)
				correction *= GetCorrectionII(station, m, g, s);
		}
		


		return correction;
	}


	ERMsg CWeatherGradient::Save(const string& filePath)const
	{
		ERMsg msg;

		string filePath1 = GetPath(filePath) + GetFileTitle(filePath) + "_gradient" + GetFileExtension(filePath);
		string filePath2 = GetPath(filePath) + GetFileTitle(filePath) + "_factor" + GetFileExtension(filePath);


		ofStream file1;
		msg = file1.open(filePath1);

		if (msg)
		{
			file1.write("scale,gradient,month,R²,S°,X,Y,Z,S\n");

			for (size_t z = 0; z < NB_SCALE_GRADIENT; z++)
			{
				for (size_t g = 0; g < NB_GRADIENT; g++)
				{
					for (size_t m = 0; m < 12; m++)
					{
						double Sᵒ = 0;
						if (m_Sᵒ[z][g][m].IsInit())
							Sᵒ = m_Sᵒ[z][g][m][MEAN];

						string line = FormatA("%d,%d,%02d,%0.3lf,%0.3lf", z + 1, g + 1, m + 1, m_R²[z][g][m], Sᵒ);
						for (size_t s = 0; s < NB_SPACE_EX; s++)
							line += "," + FormatA("%+07.4lf", m_gradient[z][g][m][s]);

						file1.write(line + "\n");
					}
				}
			}
		}

		file1.close();

		ofStream file2;
		msg = file2.open(filePath2);

		if (msg)
		{
			file2.write("gradient,space,f1,f2,f3\n");

			
			for (size_t g = 0; g < NB_GRADIENT; g++)
			{
				for (size_t s = 0; s < NB_SPACE_EX; s++)
				{
					string line = FormatA("%d,%d,%0.3lf,%0.3lf,%0.3lf", g + 1, s + 1, m_factor[0][g][s], m_factor[1][g][s], m_factor[2][g][s]);
					file2.write(line + "\n");
				}
			}
		}

		file2.close();

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
					//double a2 = GLOBAL_Sᵒ[e][PRCP_GR][m];
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