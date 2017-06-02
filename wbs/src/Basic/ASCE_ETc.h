//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <sstream>
#include "basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "ModelBase/InputParam.h"



namespace WBSF
{

	enum TSoil { SAND, LOAMY_SAND, SANDY_LOAM, LOAM, SILT_LOAM, SILT, SILTY_CLAY_LOAM, SILTY_CLAY, CLAY, NB_SOIL };
	enum TGranularity { FROM_SOIL=-1, COARSE, MEDIUM, FINE, NB_GRANULARITY };

	//after Allen 2005 Table1 TEWmax[low]*10
	struct SSoilInfo
	{
		char* name;
		double θFC[2];		//volumetric soil water content at field capacity
		double θWP[2];		//volumetric soil water content at wilting point
		double θFC_θWP[2];	//
		double REW[2];
	};

	class CSoil
	{
	public:
		
		static const SSoilInfo SOIL_INFO[NB_SOIL];
		
		static TGranularity GetDefaultGranularity(TSoil s);
		static double GetZe(TGranularity g);

		static double GetREW(TSoil s);
		static double GetTEW(TSoil s, double Ze);
		static double GetRAW(double p, double TAW);
		static double GetTAW(TSoil s, double Zr);
	};

	//Table 8.4.Typical crop height, ranges of maximum effective rooting depth(Zr), and
	//soil water depletion fraction for no stress(p), for common crops(from FAO - 56).
	//Crop Maximum Crop Height(h) (m) Maximum Root Depth[1](m) Depletion Fraction[2](p) for ETc = 5 mm d - 1
	//[1] The larger values for Zr are for soils having no significant layering or other characteristics that can restrict
	//rooting depth.The smaller values for Zr may be used for irrigation scheduling and the larger values
	//for modeling soil water stress or for rainfed conditions.
	//[2] The tabled values for p apply for ETc ≈ 5 mm / day.The value for p can be adjusted for different ETc
	//according to p = p + 0.04 (5 _ ETc) where p is expressed as a fraction and ETc as mm / day.

	enum TKInfo{ K_INI, K_MID, K_END, NB_K };
	struct SCropInfo
	{
		char* name;
		double h;
		double Zr_max[2];
		double p;
		double Kc[NB_K];
		double Kcb[NB_K];
	};

	enum TCrop 
	{
		BROCCOLI, BRUSSELS_SPROUTS, CABBAGE, CARROTS, CAULIFLOWER, CELERY, GARLIC, LETTUCE, ONIONS_DRY, ONIONS_GREEN, ONIONS_SEED, SPINACH, RADISHES,
		EGGPLANT, SWEET_PEPPERS_BELL, TOMATO, CANTALOUPE, CUCUMBER_FRESH_MARKET, CUCUMBER_MACHINE_HARVEST, PUMPKIN_WINTER_SQUASH, SQUASH_ZUCCHINI, SWEET_MELONS, WATERMELON,
		BEETS_TABLE, CASSAVA_YEAR_1, CASSAVA_YEAR_2, PARSNIP, POTATO, SWEET_POTATO, TURNIP_AND_RUTABAGA, SUGAR_BEET,
		BEANS_GREEN, BEANS_DRY_AND_PULSES, BEANS_LIMA_LARGE_VINES, CHICK_PEA, FABA_BEAN, FRESH_BEAN, FABA_BEAN_DRY, GARBANZO, COWPEAS_FRESH, COWPEAS_DRY, PEANUT, LENTIL, PEAS_FRESH, PEAS_DRY,
		SOYBEANS_SHORT, SOYBEANS_TALL, ARTICHOKES, ASPARAGUS_SHORT, ASPARAGUS_TALL, MINT_SHORT, MINT_TALL, STRAWBERRIES, COTTON_SHORT, COTTON_TALL,
		FLAX, SISAL, CASTOR_BEAN, RAPESEED_CANOLA, SAFFLOWER, SESAME, SUNFLOWER, BARLEY, OATS, SPRING_WHEAT, WINTER_WHEAT, FIELD_CORN, SWEET_CORN, MILLET, SORGHUM_GRAIN_SHORT, SORGHUM_GRAIN_TALL, SORGHUM_SWEET_SHORT, SORGHUM_SWEET_TALL, RICE,
		ALFALFA_FOR_HAY, ALFALFA_FOR_SEED, BERMUDA_FOR_HAY, BERMUDA_SPRING_CROP_FOR_SEED, CLOVER_HAY_BERSEEM, RYEGRASS_HAY, SUDAN_GRASS_HAY, GRAZING_PASTURE_SHORT, GRAZING_PASTURE_TALL, GRAZING_PASTURE_EXTENSIVE, TURF_GRASS_COOL_SEASON, TURF_GRASS_WARM_SEASON,
		SUGAR_CANE,
		BANANA_1ST_YEAR, BANANA_2ND_YEAR, CACAO, COFFEE_SHORT, COFFEE_TALL, DATE_PALMS, PALM_TREES, PINEAPPLE_SHORT, PINEAPPLE_TALL, RUBBER_TREES, TEA_NON_SHADED, TEA_SHADED, GRAPES, GRAPES_TABLE, GRAPES_WINE_SHORT, GRAPES_WINE_TALL, HOPS,
		ALMONDS, APPLES_CHERRIES_PEARS, APRICOTS_PEACHES, AVOCADO, CITRUS_70, CITRUS_50, CITRUS_20, CONIFER_TREES, KIWI,
		OLIVES_SHORT, OLIVES_TALL, PISTACHIOS_SHORT, PISTACHIOS_TALL, WALNUT_ORCHARD_SHORT, WALNUT_ORCHARD_TALL,
		NB_CROPS
	};



	enum TWettingEvent{ I_PRECIPITATION, I_SPRINKLER_FIELD, I_SPRINKLER_ORCHARDS, I_BASIN, I_BORDER, I_FURROW_NARROW, I_FURROW_WIDE, I_FURROW_ALTERNATED, I_MICROSPRAY_ORCHARDS, I_TRICKLE, NB_WETTING };
	struct SWettingEvent
	{
		char* name;
		double fw[2];
	};

	class CASCE_ETc
	{
	public:

		enum TStats { S_ETc, S_ETsz, S_Kc_adj, S_Kcb, S_h, S_Zr, S_I, NB_STATS };
		enum TStatsEx { S_Ks = NB_STATS, S_Ke, S_Kr, S_De, S_Dr, S_RAW, S_TAW, NB_STATS_EXTENDED };
		static const SCropInfo CROP_INFO[118];

		CASCE_ETc(const CMonthDay& Jplant, TCrop c = BROCCOLI, TSoil s = SILT_LOAM, TGranularity g = FROM_SOIL, TWettingEvent i = I_PRECIPITATION, bool bExtended = false);
		ERMsg Execute(const CWeatherStation& weather, CModelStatVector& stats);


		//member
		CMonthDay m_Jplant;
		TCrop m_crop;
		TSoil m_soil;
		TGranularity m_granularity;
		TWettingEvent m_irrigation;
		bool m_bExtended;
		
		static double Get_h(TCrop c);
		static double Get_p(TCrop c, double ETc=5);
		static double GetZr_max(TCrop c);
		static double GetKAdjustement(double u2, double RHmin, double h);
		static double GetKcb(TCrop c, TKInfo k, double u2 = 2, double RHmin = 45);
		static double Getfw(TWettingEvent e);

		static double Getu2(const CWeatherStation& weather, const CTRef& Jmid, const CTRef& Jlate);
		static double GetRHmin(const CWeatherStation& weather, const CTRef& TRef);
		static double GetRHmin(const CWeatherStation& weather, const CTRef& Jmid, const CTRef& Jlate);
		static double GetKr(double De, double TEW, double REW);
		static double GetKe(double Kcb, double Kc_max, double Kr, double few);
		static double GetKcb(CTRef TRef, CTRef Jdev, CTRef Jmid, CTRef Jlate, CTRef Jharv, double Kcb_ini, double Kcb_mid, double Kcb_end);
		static double GetKs(double Dr, double RAW, double TAW);
		static double GetKt(double De, double TEW, double Dr, double TAW);
		static double GetKc_max(double Kcb, double u2, double RHmin, double h);
		static double GetKc_adj(double Ks, double Kcb, double Ke);
		static double GetDPe(double P, double RO, double I, double fw, double De);
		static double GetRO(TGranularity g, double P, double REW, double TEW, double De);
		
	};


	//The typical Kv values in Table 8.10 represent nearly full effective ground cover(fc> ~0.7) (see Section 8.5.3),
	enum TLVegetation { TREES, SHRUBS_DESERT, SHRUBS_NONDESERT, GROUNDCOVER, FLOWERS, TREES_SHRUBS_GROUNDCOVER, COOL_SEASON_TURFGRASS, WARM_SEASON_TURFGRASS, NB_VEGETATIONS };

	struct CKLInfo
	{
		char* name;
		double Kv;
		double Kmc[3];
		double Ksm[3];
		double p;
	};

	enum TStress{ S_HIGHT, S_MEDIUM, S_LOW, NB_STRESS };

	//landscape cvrop coeficient
	class CASCE_ET_Land
	{
	public:

		
		TLVegetation m_vegetation;
		double m_height;

		CASCE_ET_Land(TLVegetation v = TREES, double h=7);
		ERMsg Execute(const CWeatherStation& weather, CModelStatVector& stats);

		static double GetKd(double fc_eff, double h);
		static double GetKL(TLVegetation v, TStress s, double fc_eff, double h);
	};
}


