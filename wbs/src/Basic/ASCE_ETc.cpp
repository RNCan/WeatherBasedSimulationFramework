//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
//The Dual Kc Method : Incorporating Specific Wet Soil Effects
//The dual Kc method introduced in this section, based on FAO - 56, is applicable to
//both Kc based on ETo(Kco) and Kc based on ETr(Kcr), with differences only in the
//value for Kc max. The Ke component describes the evaporation component of ETc. Because
//the dual Kc method incorporates the effects of specific wetting patterns and frequencies
//that may be unique to a single field, this method can provide more accurate
//estimates of evaporation components and total ET on an individual field basis.
//When the soil surface layer is wet, following rain or irrigation, Ke is at a maximum,
//and when the soil surface layer is dry, Ke is small, even zero. When the soil is wet,
//evaporation occurs at some maximum rate and Kcb + Ke is limited by a maximum
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"
#include <assert.h>
#include "basic/UtilStd.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/ASCE_ETc.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{



	//Coarse-textured soils include sands and loamy sand textured soils. 
	//Medium-textured soils include sandy loam, loam, silt loam, silt textured soils.
	//Fine-textured soils include silty clay loam, silty clay, and clay textured soils.
	TGranularity CSoil::GetDefaultGranularity(TSoil s)
	{
		TGranularity g;
		if (s >= SAND && s <= LOAMY_SAND)
			g = COARSE;
		else if (s >= SANDY_LOAM && s <= SILT)
			g = MEDIUM;
		else if (s >= SILTY_CLAY_LOAM && s <= CLAY)
			g = FINE;

		return g;
	}

	double CSoil::GetZe(TGranularity g)
	{
		ASSERT(g >= COARSE && g < NB_GRANULARITY);
		static const double ZE[NB_GRANULARITY] = { 0.100, 0.125, 0.150 };
		return ZE[g];
	}

	const SSoilInfo CSoil::SOIL_INFO[NB_SOIL]
	{

		{ "Sand", 0.07, 0.17, 0.02, 0.07, 0.05, 0.11, 2, 7},
		{ "Loamy sand", 0.11, 0.19, 0.03, 0.10, 0.06, 0.12, 4, 8 },
		{ "Sandy loam", 0.18, 0.28, 0.06, 0.16, 0.11, 0.15, 6, 10 },
		{ "Loam", 0.20, 0.30, 0.07, 0.17, 0.13, 0.18, 8, 10 },
		{ "Silt loam", 0.22, 0.36, 0.09, 0.21, 0.13, 0.19, 8, 11 },
		{ "Silt", 0.28, 0.36, 0.12, 0.22, 0.16, 0.20, 8, 11 },
		{ "Silt clay loam", 0.30, 0.37, 0.17, 0.24, 0.13, 0.18, 8, 11 },
		{ "Silty clay", 0.30, 0.42, 0.17, 0.29, 0.13, 0.19, 8, 12 },
		{ "Clay", 0.32, 0.40, 0.20, 0.24, 0.12, 0.20, 8, 12 },
	};


	double CSoil::GetTEW(TSoil s, double Ze)
	{
		double θFC = (SOIL_INFO[s].θFC[0] + SOIL_INFO[s].θFC[1]) / 2;
		double θWP = (SOIL_INFO[s].θWP[0] + SOIL_INFO[s].θWP[1]) / 2;
		double TEW = 1000 * (θFC - 0.5*θWP)*Ze;
		return TEW;
	}

	double CSoil::GetREW(TSoil s)
	{
		double REW = (SOIL_INFO[s].REW[0] + SOIL_INFO[s].REW[1]) / 2;
		return REW;
	}


	double CSoil::GetTAW(TSoil s, double Zr)
	{
		double TAW = 1000 * SOIL_INFO[s].θFC–θWP[1] * Zr;	//total available water in the root zone [mm]
		return TAW;
	}

	double CSoil::GetRAW(double p, double TAW)
	{
		double RAW = p*TAW;//readily available water in the root zone [mm]
		return RAW;
	}
	
	//***************************************************************************************
	const SCropInfo CASCE_ETc::CROP_INFO[118] =
	{
		//Species,Height,rootLo,rootHi,P,Crop,Kcini,Kcmid,Kcend,Kcbini,Kcbmid,Kcbend
		//a.Small Vegetables
		{ "Broccoli", 0.3, 0.4, 0.6, 0.45, 0.7, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Brussels sprouts", 0.4, 0.4, 0.6, 0.45, 0.7, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Cabbage", 0.4, 0.5, 0.8, 0.45, 0.7, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Carrots", 0.3, 0.5, 1, 0.35, 0.7, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Cauliflower", 0.4, 0.4, 0.7, 0.45, 0.7, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Celery", 0.6, 0.3, 0.5, 0.2, 0.7, 1.05, 1, 0.15, 0.95, 0.9 },
		{ "Garlic", 0.3, 0.3, 0.5, 0.3, 0.7, 1, 0.7, 0.15, 0.9, 0.6 },
		{ "Lettuce", 0.3, 0.3, 0.5, 0.3, 0.7, 1, 0.95, 0.15, 0.9, 0.9 },
		{ "Onions dry", 0.4, 0.3, 0.6, 0.3, 0.7, 1.05, 0.75, 0.15, 0.95, 0.65 },
		{ "Onions green", 0.3, 0.3, 0.6, 0.3, 0.7, 1, 1, 0.15, 0.9, 0.9 },
		{ "Onions seed", 0.5, 0.3, 0.6, 0.35, 0.7, 1.05, 0.8, 0.15, 1.05, 0.7 },
		{ "Spinach", 0.3, 0.3, 0.5, 0.2, 0.7, 1, 0.95, 0.15, 0.9, 0.85 },
		{ "Radishes", 0.3, 0.3, 0.5, 0.3, 0.7, 0.9, 0.85, 0.15, 0.85, 0.75 },
		//b.Vegetables,Solanum,Family(Solanaceae)
		{ "Eggplant", 0.8, 0.7, 1.2, 0.45, 0.6, 1.05, 0.9, 0.15, 1, 0.8 },
		{ "Sweet peppers(bell)", 0.7, 0.5, 1, 0.3, 0.6, 1.05, 0.9, 0.15, 1, 0.8 },
		{ "Tomato", 0.6, 0.7, 1.5, 0.4, 1.15, 0.6, 0.80, 0.15, 1.1, 0.70 },
		//c.Vegetables, Cucumber Family(Cucurbitaceae)
		{ "Cantaloupe", 0.3, 0.9, 1.5, 0.45, 0.5, 0.85, 0.6, 0.15, 0.75, 0.5 },
		{ "Cucumber fresh market", 0.3, 0.7, 1.2, 0.5, 0.6, 1, 0.75, 0.15, 0.95, 0.7 },
		{ "Cucumber machine harvest", 0.3, 0.7, 1.2, 0.5, 0.5, 1, 0.9, 0.15, 0.95, 0.8 },
		{ "Pumpkin winter squash", 0.4, 1, 1.5, 0.35, 0.5, 1, 0.8, 0.15, 0.95, 0.7 },
		{ "Squash zucchini", 0.3, 0.6, 1, 0.5, 0.5, 0.95, 0.75, 0.15, 0.9, 0.7 },
		{ "Sweet melons", 0.4, 0.8, 1.5, 0.4, 0.5, 1.05, 0.75, 0.15, 1, 0.7 },
		{ "Watermelon", 0.4, 0.8, 1.5, 0.4, 0.4, 1, 0.75, 0.15, 0.95, 0.7 },
		//d.Roots and Tubers
		{ "Beets table", 0.4, 0.6, 1, 0.5, 0.5, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Cassava year 1", 1, 0.5, 0.8, 0.35, 0.3, 0.80, 0.3, 0.70, 0.2 },
		{ "Cassava year 2", 1.5, 0.7, 1, 0.4, 0.3, 1.1, 0.5, 0.15, 1, 0.45 },
		{ "Parsnip", 0.4, 0.5, 1, 0.4, 0.5, 1.05, 0.95, 0.15, 0.95, 0.85 },
		{ "Potato", 0.6, 0.4, 0.6, 0.35, 0.5, 1.15, 0.75, 0.15, 1.1, 0.65 },
		{ "Sweet potato", 0.4, 1, 1.5, 0.65, 0.5, 1.15, 0.65, 0.15, 1.1, 0.55 },
		{ "Turnip and rutabaga", 0.6, 0.5, 1, 0.5, 0.5, 1.1, 0.95, 0.15, 1, 0.85 },
		{ "Sugar beet", 0.5, 0.7, 1.2, 0.55, 0.35, 1.2, 0.7, 0.15, 1.15, 0.5 },
		//e.Legumes(Leguminosae)
		{ "Beans green", 0.4, 0.5, 0.7, 0.45, 0.5, 1.05, 0.9, 0.15, 1, 0.8 },
		{ "Beans dry and pulses", 0.4, 0.6, 0.9, 0.45, 0.4, 1.15, 0.35, 0.15, 1.1, 0.25 },
		{ "Beans lima large vines", 0.4, 0.8, 1.2, 0.45, 0.4, 1.15, 0.35, 0.15, 1.1, 0.25 },
		{ "Chick pea", 0.4, 0.6, 1, 0.5, 0.4, 1, 0.35, 0.15, 0.95, 0.25 },
		{ "Faba bean(broadbean) fresh", 0.8, 0.5, 0.7, 0.45, 0.5, 1.15, 1.1, 0.15, 1.1, 1.05 },
		{ "Faba bean(broadbean) dry/seed", 0.8, 0.5, 0.7, 0.45, 0.5, 1.15, 0.3, 0.15, 1.1, 0.2 },
		{ "Garbanzo", 0.8, 0.6, 1, 0.45, 0.4, 1.15, 0.35, 0.15, 1.05, 0.25 },
		{ "Green  gram and cowpeas freash", 0.4, 0.6, 1, 0.45, 0.4, 1.05, 0.60, 0.15, 1, 0.55 },
		{ "Green  gram and cowpeas dry", 0.4, 0.6, 1, 0.45, 0.4, 1.05, 0.35, 0.15, 1, 0.25 },
		{ "Groundnut(peanut)", 0.4, 0.5, 1, 0.5, 0.4, 1.15, 0.6, 0.15, 1.1, 0.5 },
		{ "Lentil", 0.5, 0.6, 0.8, 0.5, 0.4, 1.1, 0.3, 0.15, 1.05, 0.2 },
		{ "Peas fresh", 0.5, 0.6, 1, 0.35, 0.5, 1.15, 1.1, 0.15, 1.1, 1.05 },
		{ "Peas dry/seed", 0.5, 0.6, 1, 0.4, 0.4, 1.15, 0.3, 0.15, 1.1, 0.2 },
		{ "Soybeans short", 0.5, 0.6, 1.3, 0.5, 0.4, 1.15, 0.5, 0.15, 1.1, 0.3 },
		{ "Soybeans tall", 1, 0.6, 1.3, 0.5, 0.4, 1.15, 0.5, 0.15, 1.1, 0.3 },
		//f.Perennial,Vegetables(with winter dormancy and initially bare or mulched soil)
		{ "Artichokes", 0.7, 0.6, 0.9, 0.45, 0.5, 1, 0.95, 0.15, 0.95, 0.9 },
		{ "Asparagus short", 0.2, 1.2, 1.8, 0.45, 0.5, 0.95, 0.3, 0.15, 0.9, 0.2 },
		{ "Asparagus tall", 0.8, 1.2, 1.8, 0.45, 0.5, 0.95, 0.3, 0.15, 0.9, 0.2 },
		{ "Mint short", 0.6, 0.4, 0.8, 0.4, 0.6, 1.15, 1.1, 0.4, 1.1, 1.05 },
		{ "Mint tall", 0.8, 0.4, 0.8, 0.4, 0.6, 1.15, 1.1, 0.4, 1.1, 1.05 },
		{ "Strawberries", 0.2, 0.2, 0.3, 0.2, 0.4, 0.85, 0.75, 0.3, 0.8, 0.7 },
		//g.Fiber,Crops
		{ "Cotton short", 1.2, 1, 1.7, 0.65, 0.35, 1.20, 0.50, 0.15, 1.15, 0.40 },
		{ "Cotton tall", 1.5, 1, 1.7, 0.65, 0.35, 1.15, 0.70, 0.15, 1.10, 0.50 },
		{ "Flax", 1.2, 1, 1.5, 0.5, 0.35, 1.1, 0.25, 0.15, 1.05, 0.2 },
		{ "Sisal", 1.5, 0.5, 1, 0.8, 0.35, 0.55, 0.55, 0.15, 0.55, 0.55 },
		//h.Oil,Crops
		{ "Castor bean(Ricinus)", 0.3, 1, 2, 0.5, 0.35, 1.15, 0.55, 0.15, 1.1, 0.45 },
		{ "Rapeseed canola", 0.6, 1, 1.5, 0.6, 0.35, 1.07, 0.35, 0.15, 1.03, 0.25 },
		{ "Safflower", 0.8, 1, 2, 0.6, 0.35, 1.07, 0.25, 0.15, 1.03, 0.2 },
		{ "Sesame", 1, 1, 1.5, 0.6, 0.35, 1.1, 0.25, 0.15, 1.05, 0.2 },
		{ "Sunflower", 2, 0.8, 1.5, 0.45, 0.35, 1.07, 0.35, 0.15, 1.03, 0.25 },
		//i.Cereals
		{ "Barley", 1, 1, 1.5, 0.55, 0.3, 1.15, 0.25, 0.15, 1.1, 0.15 },
		{ "Oats", 1, 1, 1.5, 0.55, 0.3, 1.15, 0.33, 0.15, 1.1, 0.23 },
		{ "Spring wheat", 1, 1, 1.5, 0.55, 0.4, 1.15, 0.33, 0.33, 1.1, 0.23 },
		{ "Winter wheat", 1, 1.5, 1.8, 0.55, 0.7, 1.15, 0.33 },
		{ "Maize field(grain)(field corn)", 2, 1, 1.7, 0.55, 0.3, 1.2, 0.35, 0.15, 1.15, 0.15 },
		{ "Maize sweet(sweet corn)", 1.5, 0.8, 1.2, 0.5, 0.3, 1.15, 1.05, 0.15, 1.1, 1 },
		{ "Millet", 1.5, 1, 2, 0.55, 0.3, 1, 0.3, 0.15, 0.95, 0.2 },
		{ "Sorghum grain short", 1, 1, 2, 0.55, 0.3, 1.05, 0.55, 0.15, 1.00, 0.35 },
		{ "Sorghum grain tall", 2, 1, 2, 0.55, 0.3, 1.05, 0.55, 0.15, 1.00, 0.35 },
		{ "Sorghum sweet short", 2, 1, 2, 0.5, 0.3, 1.2, 1.05, 0.15, 1.15, 1 },
		{ "Sorghum sweet tall", 4, 1, 2, 0.5, 0.3, 1.2, 1.05, 0.15, 1.15, 1 },
		{ "Rice", 1, 0.5, 1, 0.2, 1.05, 1.2, 0.75, 1, 1.15, 0.57 },
		//j.Forages
		{ "Alfalfa for hay", 0.7, 1, 2, 0.55, 0.4, 1.2, 1.15, 0.3, 1.15, 1.1 },
		{ "Alfalfa for seed", 0.7, 1, 3, 0.6, 0.4, 0.5, 0.5, 0.3, 0.45, 0.45 },
		{ "Bermuda for hay", 0.35, 1, 1.5, 0.55, 0.55, 1, 0.85, 0.5, 0.95, 0.8 },
		{ "Bermuda spring crop for seed", 0.4, 1, 1.5, 0.6, 0.35, 0.9, 0.65, 0.15, 0.85, 0.6 },
		{ "Clover hay berseem", 0.6, 0.6, 0.9, 0.5, 0.4, 1.15, 1.1, 0.3, 1.1, 1.05 },
		{ "Ryegrass hay", 0.3, 0.6, 1, 0.6, 0.95, 1.05, 1, 0.85, 1, 0.95 },
		{ "Sudan grass hay(annual)", 1.2, 1, 1.5, 0.55, 0.5, 1.15, 1.1, 0.3, 1.1, 1.05 },
		{ "Grazing pasture-rotated grazing short", 0.15, 0.5, 1.5, 0.6, 0.4, 0.95, 0.85, 0.3, 0.90, 0.8 },
		{ "Grazing pasture-rotated grazing tall", 0.3, 0.5, 1.5, 0.6, 0.4, 0.95, 0.85, 0.3, 0.90, 0.8 },
		{ "Grazing pasture-extensive grazing", 0.1, 0.5, 1.5, 0.6, 0.3, 0.75, 0.75, 0.3, 0.7, 0.7 },
		{ "Turf grass cool season", 0.1, 0.5, 1, 0.4, 0.9, 0.9, 0.9, 0.8, 0.85, 0.85 },
		{ "Turf grass warm season", 0.1, 0.5, 1, 0.5, 0.85, 0.9, 0.9, 0.75, 0.8, 0.8 },
		//k.Sugar Cane
		{ "Sugar Cane", 3, 1.2, 2, 0.65, 0.4, 1.25, 0.75, 0.15, 1.2, 0.7 },
		//l.Tropical Fruits and Trees
		{ "Banana 1st year", 3, 0.5, 0.9, 0.35, 0.5, 1.1, 1, 0.15, 1.05, 0.9 },
		{ "Banana 2nd year", 4, 0.5, 0.9, 0.35, 1, 1.2, 1.1, 0.6, 1.1, 1.05 },
		{ "Cacao", 3, 0.7, 1, 0.3, 1, 1.05, 1.05, 0.9, 1, 1 },
		{ "Coffee short", 2, 0.9, 1.5, 0.4, 0.9, 0.95, 0.95, 0.8, 0.9, 0.9 },
		{ "Coffee tall", 3, 0.9, 1.5, 0.4, 1.05, 1.1, 1.1, 1, 1.05, 1.05 },
		{ "Date palms", 8, 1.5, 2.5, 0.5, 0.9, 0.95, 0.95, 0.8, 0.85, 0.85 },
		{ "Palm trees", 8, 0.7, 1.1, 0.65, 0.95, 1, 1, 0.85, 0.9, 0.9 },
		{ "Pineapple short", 0.6, 0.3, 0.6, 0.5, 0.5, 0.3, 0.3, 0.15, 0.25, 0.25 },
		{ "Pineapple tall", 1.2, 0.3, 0.6, 0.5, 0.5, 0.5, 0.5, 0.3, 0.45, 0.45 },
		{ "Rubber trees", 10, 1, 1.5, 0.4, 0.95, 1, 1, 0.85, 0.9, 0.9 },
		{ "Tea non-shaded", 1.5, 0.9, 1.5, 0.4, 0.95, 1, 1, 0.9, 0.95, 0.9 },
		{ "Tea shaded", 2, 0.9, 1.5, 0.45, 1.1, 1.15, 1.15, 1, 1.1, 1.05 },
		//m. Berries and Hops
		{ "Grapes and Berries Berries(bushes)", 1.5, 0.6, 1.2, 0.5, 0.3, 1.05, 0.5, 0.2, 1, 0.4 },
		{ "Grapes table or raisin", 2, 1, 2, 0.35, 0.2, 1.15, 0.9, 0.15, 0.7, 0.7 },
		{ "Grapes wine short", 1.5, 1, 2, 0.45, 0.2, 0.8, 0.6, 0.15, 0.7, 0.7 },
		{ "Grapes wine tall", 2, 1, 2, 0.45, 0.2, 0.8, 0.6, 0.15, 0.7, 0.7 },
		{ "Hops", 5, 1, 1.2, 0.5, 0.3, 1.05, 0.85, 0.15, 1, 0.8 },
		//n.Fruit Trees
		{ "Almonds", 5, 1, 2, 0.4, 0.2, 1, 0.7, 0.15, 0.75, 0.8 },
		{ "Apples cherries pears", 4, 1, 2, 0.5, 0.3, 1.15, 0.8, 0.15, 0.4, 0.8 },
		{ "Apricots peaches stone fruit", 3, 1, 2, 0.5, 0.3, 1.2, 0.8, 0.15, 0.7, 0.8 },
		{ "Avocado", 3, 0.5, 1, 0.7, 0.3, 1, 0.9, 0.15, 0.75, 0.8 },
		{ "Citrus 70% canopy", 4, 1.2, 1.5, 0.5, 0.8, 0.8, 0.8, 0.15, 0.75, 0.8 },
		{ "Citrus 50% canopy", 3, 1.1, 1.5, 0.5, 0.8, 0.8, 0.8, 0.15, 0.75, 0.8 },
		{ "Citrus 20% canopy", 2, 0.8, 1.1, 0.5, 0.8, 0.8, 0.8, 0.15, 0.75, 0.8 },
		{ "Conifer trees", 10, 1, 1.5, 0.7, 1, 1, 1, 0.95, 0.95, 0.95 },
		{ "Kiwi", 3, 0.7, 1.3, 0.35, 0.4, 1.05, 1.05, 0.2, 1, 1 },
		{ "Olives short(40% to 60% groundcoverage by canopy)", 3, 1.2, 1.7, 0.65, 0.6, 0.7, 0.6, 0.15, 0.7, 0.7 },
		{ "Olives tall(40% to 60% groundcoverage by canopy)", 5, 1.2, 1.7, 0.65, 0.6, 0.7, 0.6, 0.15, 0.7, 0.7 },
		{ "Pistachios short", 3, 1, 1.5, 0.4, 0.3, 1, 0.7, 0.15, 0.7, 0.7 },
		{ "Pistachios tall", 5, 1, 1.5, 0.4, 0.3, 1, 0.7, 0.15, 0.7, 0.7 },
		{ "Walnut orchard short", 4, 1.7, 2.4, 0.5, 0.4, 1.1, 0.65, 0.15, 0.75, 0.8 },
		{ "Walnut orchard tall", 5, 1.7, 2.4, 0.5, 0.4, 1.1, 0.65, 0.15, 0.75, 0.8 },
	};
	
	CASCE_ETc::CASCE_ETc(const CMonthDay& Jplant, TCrop c, TSoil s, TGranularity g, TWettingEvent i, bool bExtended)
	{
		m_Jplant = Jplant;
		m_crop = c;
		m_soil = s;
		m_granularity = g;
		m_irrigation=i;
		m_bExtended = bExtended;
	}

	double CASCE_ETc::Get_p(TCrop c, double ETc)
	{
		double p = CROP_INFO[c].p + 0.04* (5 - ETc);
		return max(0.0, min(1.0, p));
	}

	//Kcbo and Kcbr are the basal Kc for the ETo and ETr bases
	//c	: crop type
	double CASCE_ETc::Get_h(TCrop c)
	{
		return CROP_INFO[c].h;
	}

	//Zr_max: maximum effective rooting depth [m]
	double CASCE_ETc::GetZr_max(TCrop c)
	{
		//The larger values for Zr are for soils having no significant layering or other characteristics that can restrict
		//rooting depth. The smaller values for Zr may be used for irrigation scheduling and the larger values
		//for modeling soil water stress or for rainfed conditions.

		return CROP_INFO[c].Zr_max[1];
	}

	//u2	: horizontal wind speed at 2 m above the ground surface [m/s]
	//RHmin	: minimum daily relative humidity [%]
	//h		: height of vegetation [m]
	double CASCE_ETc::GetKAdjustement(double u2, double RHmin, double h)
	{
		//u2 = if(u2 >= 1 && u2 <= 6 && RHmin >= 20 && RHmin <= 80)
		return (0.04*(u2 - 2) - 0.004*(RHmin - 45))*pow(h / 3, 0.3);
	}

	
	//Kcb	: basal crop coefficient, 0 to 1.4 for Kco and 0 to ~1.0 for Kcr
	//c		: crop type
	//k		: season point
	//u2	: mean horizontal wind speed at 2 m above the ground surface [m/s]
	//RHmin	: minimum daily relative humidity [%]
	double CASCE_ETc::GetKcb(TCrop c, TKInfo k, double u2, double RHmin)
	{
		ASSERT(k == K_INI || k == K_MID || k == K_END);

		double h = Get_h(c);

		double Kc = CROP_INFO[c].Kcb[k];
		if (k == K_MID)
			Kc += GetKAdjustement(u2, RHmin, h);

		if (k == K_END && Kc >= 0.45)
			Kc += GetKAdjustement(u2, RHmin, h);

		//basal crop coefficient, soil water not limiting transpiration, but the soil surface is visually dry
		return Kc;
	}


	//fw	: fraction of the soil surface wetted by irrigation and/or precipitation
	//e		: wetting event
	double CASCE_ETc::Getfw(TWettingEvent e)
	{
		ASSERT(e >= I_PRECIPITATION && e<NB_WETTING);
		static const SWettingEvent WETTING[NB_WETTING] =
		{
			{ "Precipitation", 1, 1 },
			{ "Sprinkler irrigation, field crops", 1, 1 },
			{ "Sprinkler irrigation, orchards,", 0.7, 1 },
			{ "Basin irrigation", 1, 1 },
			{ "Border irrigation", 1, 1 },
			{ "Furrow irrigation(every furrow), narrow bed", 0.6, 1 },
			{ "Furrow irrigation(every furrow), wide bed", 0.4, 0.6 },
			{ "Furrow irrigation(alternated furrows)", 0.3, 0.5 },
			{ "Microspray irrigation, orchards", 0.5, 0.8 },
			{ "Trickle(drip) irrigation", 0.3, 0.4 },
		};

		return (WETTING[e].fw[0] + WETTING[e].fw[1]) / 2;
	}

	//*******************************************************************************

	//u2	: mean of horizontal wind speed at 2 m above the ground surface [m/s]
	//begin	: begin of the period 
	//end	: end of the period
	double CASCE_ETc::Getu2(const CWeatherStation& weather, const CTRef& begin, const CTRef& end)
	{
		CStatistic u2;
		 
		for (CTRef TRef = begin; TRef <= end; TRef++)
			u2 += weather[TRef][H_WND2][MEAN] * 1000 / 3600;

		return u2[MEAN];
	}

	//RHmin	: daily minimum relative humidity for a date [%]
	//Tref	: date
	double CASCE_ETc::GetRHmin(const CWeatherStation& weather, const CTRef& TRef)
	{
		//lowest daily RH from hourly value
		if (weather.IsHourly())
			return weather[TRef][H_RELH][LOWEST];

		double E°Tdew = e°(weather[TRef][H_TDEW][MEAN]) * 1000;
		double E°Tmax = e°(weather[TRef][H_TMAX][MEAN]) * 1000;
		double RHmin = max(1.0, min(100.0, E°Tdew / E°Tmax * 100));
		return RHmin;
	}

	//RHmin	: mean of daily minimum relative humidity for a period [%]
	//begin	: begin of the period 
	//end	: end of the period
	double CASCE_ETc::GetRHmin(const CWeatherStation& weather, const CTRef& begin, const CTRef& end)
	{
		CStatistic RHmin;
		
		for (CTRef TRef = begin; TRef <= end; TRef++)
			RHmin += GetRHmin(weather, TRef);

		return RHmin[MEAN];
	}


	//Kr	: evaporation reduction coefficient is dependent on the cumulative depth of water depleted [dimensionless]
	//De	: cumulative depletion from the soil surface layer [mm]
	//REW	: readily evaporable water from the surface soil layer [mm]
	//TEW	: total evaporable water [mm]
	double CASCE_ETc::GetKr(double De, double REW, double TEW)
	{
		double Kr = (De > REW) ? (TEW - De) / (TEW - REW) : 1;
		return max(0.0, Kr);
	}

	//Ke	: estimated amount of energy available at the exposed soil fraction
	//Kcb	: basal crop coefficient, 0 to 1.4 for Kco and 0 to ~1.0 for Kcr
	//Kc_max: maximum value of Kc following rain or irrigation
	//Kr	: dimensionless evaporation reduction coefficient(defined later) and is dependent on the cumulative depth of water depleted(evaporated)
	//few	: fraction of the soil that is both exposed to solar radiation and that is wetted.The evaporation rate is restricted by
	double CASCE_ETc::GetKe(double Kcb, double Kc_max, double Kr, double few)
	{
		//the estimated amount of energy available at the exposed soil fraction, i.e., Ke cannot
		//exceed few Kc_max. Kc_max for the ETo basis(Kco_max) ranges from about 1.05 to 1.30:
		double Ke = min(Kr*(Kc_max - Kcb), few*Kc_max);
		return Ke;
	}

	//Kcb	: basal crop coefficient, 0 to 1.4 for Kco and 0 to ~1.0 for Kcr
	//TRef	: acttual date
	//Jdev	: begin of development period of crop growing season,
	//Jmid	: begin of midseason period of crop growing season,
	//Jlate	: begin of late season period of crop growing season,
	//Jharv	: date of havest
	//Kcb_ini: initial basal crop coeficient
	//Kcb_mid: mean midseason basal crop coeficient
	//Kcb_end: basal crop coeficient at harvest
	double CASCE_ETc::GetKcb(CTRef TRef, CTRef Jdev, CTRef Jmid, CTRef Jlate, CTRef Jharv, double Kcb_ini, double Kcb_mid, double Kcb_end)
	{
		double Kcb = 0;

		if (TRef < Jdev)
			Kcb = Kcb_ini;
		else if (TRef < Jmid)
			Kcb = Kcb_ini + (Kcb_mid - Kcb_ini)*(TRef - Jdev) / (Jmid - Jdev);
		else if (TRef < Jlate)
			Kcb = Kcb_mid;
		else if (TRef < Jharv)
			Kcb = Kcb_mid + (Kcb_end - Kcb_mid)*(TRef - Jlate) / (Jharv - Jlate);
		else
			Kcb = Kcb_ini;

		ASSERT(Kcb >= Kcb_ini && Kcb <= Kcb_mid);
		return Kcb;
	}

	//Ks	: soil water stress coefficient computed for the root zone(range of 0 to 1)
	//Dr	: cumulative depletion from the root zone, including De [mm]
	//RAW	: water that can be extract from the root zone without suffering water stress [mm]
	//TAW	: total available water in the root zone  [mm]
	double CASCE_ETc::GetKs(double Dr, double RAW, double TAW)
	{
		double Ks = (Dr>RAW) ? (TAW - Dr) / (TAW - RAW) : 1;
		return max(0.0, min(1.0, Ks));
	}

	//Kt	: proportion of basal ET (Kcb ETref) extracted as transpiration from the few fraction
	//De	: cumulative depletion depth at the ends of previous days [mm]
	//TEW	: total evaporable water that can be evaporated from the surface soil layer [mm]
	//Dr	: current depletion from the effective root zone [mm]
	//TAW	: the total available water in the root zone [mm]
	double CASCE_ETc::GetKt(double De, double TEW, double Dr, double TAW)
	{
		//8.68
		//	The water stress function is described here. Mean water content of the root
		//	zone is expressed as root zone depletion, Dr, defined as water shortage relative to field
		//	capacity.At field capacity, Dr = 0. The degree of stress is presumed to progressively
		//	increase as Dr increases past RAW, the depth of readily available water in the root
		//	zone.For Dr > RAW, Ks is :
		double Kt = (1 - (De / TEW)) / (1 - (Dr / TAW));
		return max(0.0, min(1.0, Kt));
	}

	//Kc_max: maximum value of Kc following rain or irrigation
	//Kcb	: basal crop coefficient, 0 to 1.4 for Kco and 0 to ~1.0 for Kcr
	//u2	: horizontal wind speed at 2 m above the ground surface [m/s]
	//RHmin	: minimum daily relative humidity [%]
	//h		: height of vegetation [m]
	double CASCE_ETc::GetKc_max(double Kcb, double u2, double RHmin, double h)
	{
		double Kc_max = max(1.2 + GetKAdjustement(u2, RHmin, h), Kcb + 0.05);
		ASSERT(Kc_max > 0);
		return Kc_max;
	}



	
	//Kc_adj: ratio of ETc to ETref when the soil surface layer is dry, but where the average soil water content of the root zone is adequate to sustain
	//Ks	: stress reduction coefficient, 0 to 1
	//Kcb	: basal crop coefficient, 0 to 1.4 for Kco and 0 to ~1.0 for Kcr
	//Ke	: soil water evaporation coefficient
	double CASCE_ETc::GetKc_adj(double Ks, double Kcb, double Ke)
	{
		
		double Kc_adj = Ks*Kcb + Ke;
		return Kc_adj;
	}


	//DPe	: Downward drainage (percolation) of water from the surface layer [mm]
	//P		: depth of rainfall during the event[mm].
	//RO	: depth of surface runoff during the event [mm]
	//I		: irrigation depth that infiltrates the soil [mm]
	//fw	:
	//De	: cumulative depletion from the soil surface layer [mm]
	double CASCE_ETc::GetDPe(double P, double RO, double I, double fw, double De)
	{
		//8.69
		double DPe = (P - RO) + I / fw - De;
		return max(0.0, DPe);
	}

	
	//is the maximum depth of water that can be retained as infiltration and canopy
	//interception during a single precipitation event(in mm).S is calculated as
	//g		: granularity of soil
	//RO	: depth of surface runoff during the event [mm]
	//P		: depth of rainfall during the event[mm].
	//REW	: readily evaporable water from the surface soil layer [mm]
	//TEW	: total evaporable water that can be evaporated from the surface soil layer [mm]
	//De	: cumulative depletion from the soil surface layer [mm]
	double CASCE_ETc::GetRO(TGranularity g, double P, double REW, double TEW, double De)
	{
		double RO = 0;
		if (P == 0)
			return RO;

		//Table 8.15.Typical curve numbers for general crops for antecedent soil water condition(AWC) II from USDA - SCS(1972) and Allen(1988).
		//Suggested defaults
		static const double AWC_II[NB_GRANULARITY] = { 65, 72, 82 };

		double CNɪɪ = AWC_II[g];

		//Hawkins et al. (1985) expressed tabular relationships in USDA - SCS(1972) in the
		//form of equations relating CN for AWC I and AWC III to CN for AWC II :
		double CNɪ = CNɪɪ / (2.281 - 0.01281*CNɪɪ);
		double CNɪɪɪ = CNɪɪ / (0.427 + 0.00573*CNɪɪ);

		//where CNI is the curve number associated with AWC I(dry)[0 - 100], CNII is the
		//curve number associated with AWC II(average condition)[0 - 100], and CNIII is the
		//	curve number associated with AWC III(wet)[0 - 100].
		//	The soil surface layer water balance associated with the dual Kc procedure(Equation
		//	8.66) can be used to estimate the AWC condition.An approximation for the depletion
		//	of the soil surface layer at AWC III(wet) is when De = 0.5 REW, i.e., when the
		//	evaporation process is halfway through stage 1 drying.This point will normally be
		//	when approximately 5 mm or less have evaporated from the top 150 mm of soil since
		//	the time it was last completely wetted.Thus, the relationship :
		//(8.91)

		double De_AWCɪɪɪ = 0.5* REW;
		//where De - AWC III is the depletion of the evaporative layer at AWC III.AWC I can be
		//estimated to occur when 15 to 20 mm of water have evaporated from the top 150 mm
		//	of soil from the time it was last completely wetted.This is equivalent to when the
		//	evaporation layer has dried to the point at which De exceeds 30 % of the total evaporable
		//	water in the surface layer beyond REW.This depletion

		double De_AWCɪ = 0.7* REW + 0.3*TEW;
		//where TEW is the cumulative evaporation from the surface soil layer at the end of
		//stage 2 drying.When De is in between these two extremes, i.e., 0.5 REW < De < 0.7
		//REW + 0.3 TEW, then the AWC is in the AWC II condition and the CN value is linearly
		//interpolated between CNI and CNIII.In equation form :
		//


		double CN = 0;
		if (De < De_AWCɪɪɪ)
			CN = CNɪɪɪ;
		else if (De >= De_AWCɪ)
			CN = CNɪ;
		else
			CN = ((De - De_AWCɪɪɪ)*CNɪ + (De_AWCɪ - De)*CNɪɪɪ) / (0.2*REW + 0.3*TEW);

		//If P < 0.2 S, then RO =
		//0.0.In addition, RO ≤ P applies.
		double S = 250 * (100 / CN - 1);
		if (P > 0.2*S)
			RO = Square(P - 0.2*S) / (P + 0.8*S);

		return min(RO, P);
	}



	ERMsg CASCE_ETc::Execute(const CWeatherStation& weather, CModelStatVector& output)
	{
		ERMsg msg;

		if (m_granularity == FROM_SOIL)
			m_granularity = CSoil::GetDefaultGranularity(m_soil);

		//Init output 
		output.Init(weather.GetEntireTPeriod(CTM(CTM::DAILY)), m_bExtended ? NB_STATS_EXTENDED : NB_STATS);

		//compute evapotranpiration of reference

		CASCE_ETsz ASCE2005;
		CModelStatVector Etsz;
		ASCE2005.Execute(weather, Etsz);
		Etsz.Transform(CTM(CTM::DAILY), SUM);


		//Compute DegreeDays 5°C
		//CDegreeDays DD(m_weather.IsHourly() ? CDegreeDays::AT_HOURLY : CDegreeDays::AT_DAILY, CDegreeDays::DAILY_AVERAGE, 5);
		//CModelStatVector DD5;
		//DD.Execute(m_weather, DD5);


		//developement length
		size_t Lini = 25;
		size_t Ldev = 25;
		size_t Lmid = 30;
		size_t Llate = 20;

		//compute all
		for (size_t y=0; y < weather.size(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

			CTRef Jplant(p.Begin().GetYear(), m_Jplant.m_month, m_Jplant.m_day);//input
			CTRef Jdev = Jplant + Lini;
			CTRef Jmid = Jdev + Ldev;
			CTRef Jlate = Jmid + Lmid;
			CTRef Jlate_harv = Jmid + (Lmid + Llate) / 2;
			CTRef Jharv = Jlate + Llate;

			//The adjustments to Kc for climate are generally made using mean values
			//for u2 and RHmin for the entire midseason period.
			double u2_mid = Getu2(weather, Jmid, Jlate);		//mean wind speed at 2 meters [m/s]
			double RHmin_mid = GetRHmin(weather, Jmid, Jlate);//[%]

			//root 
			double Ze = CSoil::GetZe(m_granularity);

			double h = 0;
			double h_max = Get_h(m_crop);
			double Kcb_ini = GetKcb(m_crop, K_INI, u2_mid, RHmin_mid);
			double Kcb_mid = GetKcb(m_crop, K_MID, u2_mid, RHmin_mid);
			double Kcb_end = GetKcb(m_crop, K_END, u2_mid, RHmin_mid);
			double Kc_min = Kcb_ini;
			double Kc_max = Kcb_mid;

			const double REW = CSoil::GetREW(m_soil);
			const double TEW = CSoil::GetTEW(m_soil, Ze);

			const double Zr_min = 0.2;
			const double Zr_max = GetZr_max(m_crop);

			double De = 0;//cumulative depletion from the soil surface layer, begin at the end of the winter at zero [mm]
			double Dr = 0;//cumulative depletion from the root zone, including De [mm]
			double DPe = 0;//deep percolation from the few fraction of the soil surface [mm]

			double Zr = 0;
			double RAW = 0;
			double Dr_adj = 0;
			double IRn = 0;			//net irrigation water requirement

			double last_p = 0;//last amout of precipitation
			double last_i = 0;//last amout of irigation
			double fw¯¹ = 1;


			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				if (TRef.GetJDay() == 135)
				{
					int i;
					i = 0;
				}
				
				//get weather variables
				double RHmin = GetRHmin(weather, TRef);//[%]
				double u2 = weather[TRef][H_WND2][MEAN] * 1000 / 3600; //u2 [m/s]
				double P = weather[TRef][H_PRCP][SUM]; //[mm]
				double ETo = Etsz[TRef][ETInterface::S_ET];//[mm]
				double RO = GetRO(m_granularity, P, REW, TEW, De);//[mm]

				//get current basal crop
				double Kcb = GetKcb(TRef, Jdev, Jmid, Jlate, Jharv, Kcb_ini, Kcb_mid, Kcb_end);
				h = max(h, Kcb / Kc_max*h_max);			//update crop height [m]
				Kc_max = GetKc_max(Kcb, u2, RHmin, h);	//update max crop coef

				//compute fraction of soil covered/uncovered by vegetation
				double I = IRn; //irrigation
				double fc = max(0.01, min(0.99, pow((Kcb - Kc_min) / (Kc_max - Kc_min), 1 + 0.5*h)));
				//determine effective fw from the last maine event between P and I
				//double fw = Getfw(last_p > last_i ? I_PRECIPITATION : m_irrigation);
				double fw = I>0 ? Getfw(m_irrigation) : P>0 ? 1 : fw¯¹;
				double few = min(1.0 - fc, fw); //fraction of the soil that is both exposed to solar radiation and that is wetted

				//compute coeficient
				double Kr = GetKr(De, REW, TEW);
				double Ke = GetKe(Kcb, Kc_max, Kr, few);
				double E = ETo*Ke;	//evaporation [mm]

				double DPe = max(0.0, (P - RO) + I / fw - De);//deep percolation from the few fraction of the soil surface layer if soil water content exceeds field capacity [mm]
				De = max(0.0, min(TEW, De - (P - RO) - I / fw + E / few - DPe));//update cumulative depletion from the soil surface layer [mm]

				//first estimate of Kc
				double Kc = Ke + Kcb;	//first estimate of Kc
				double ETc = Kc*ETo;	//first estimate of ETc


				Zr = max(Zr, Zr_min + (Kcb - Kc_min) / (Kc_max - Kc_min)*(Zr_max - Zr_min));//update root deep [m]
				double TAW = CSoil::GetTAW(m_soil, Zr);	//total available water in the root zone [mm]
				double p = Get_p(m_crop, ETc);
				double RAW = p*TAW;//readily available water in the root zone, mm

				double Dr = max(0.0, min(TAW, Dr_adj - (P - RO) - I + ETc));	//depletion
				double DP = max(0.0, (P - RO) + I - ETc - Dr_adj);	//Drainage
				double Ks = GetKs(Dr, RAW, TAW);	//stress coeficient
				double Kc_adj = Ke + Kcb*Ks;

				//compute irrigation if needed
				if (m_irrigation != I_PRECIPITATION)
					IRn = ((TRef >= Jplant) && (TRef<Jlate_harv) && (Dr>RAW)) ? Dr : 0;


				//update Dr with Kc_adj
				Dr_adj = max(0.0, min(TAW, Dr_adj - (P - RO) - I + Kc_adj*ETo + DP));

				//if (P > 0)
				last_p = P;
//
				//if (IRn > 0)
				last_i = I / fw;
			
				fw¯¹ = fw;

				//save output
				output[TRef][S_ETc] = Kc_adj*ETo;
				output[TRef][S_ETsz] = ETo;
				output[TRef][S_Kc_adj] = Kc_adj;
				output[TRef][S_Kcb] = Kcb;
				output[TRef][S_h] = h;
				output[TRef][S_Zr] = Zr;
				output[TRef][S_I] = I;

				if (m_bExtended)
				{
					output[TRef][S_Ks] = Ks;
					output[TRef][S_Ke] = Ke;
					output[TRef][S_Kr] = Kr;
					output[TRef][S_De] = De;
					output[TRef][S_Dr] = Dr_adj;
					output[TRef][S_RAW] = RAW;
					output[TRef][S_TAW] = TAW;
				}
			}
		}

		return msg;
	}



	//************************************************************************************

	CASCE_ET_Land::CASCE_ET_Land(TLVegetation v, double h)
	{
		m_vegetation=v;
		m_height = h;
	}

	//fc_eff: effective fraction of ground covered or shaded by vegetation(0 to 1.0) near solar noon
	//h		: mean height of the vegetation [m]
	double CASCE_ET_Land::GetKd(double fc_eff, double h)
	{
		//ML	:	multiplier on fc_eff to impose an upper limit on relative transpiration per unit ground area
		//Parameter ML is expected to range from 1.5 to 2.0,
		double ML = 1.5;

		//(8.80)
		double Kd = min(ML*fc_eff, pow(fc_eff, 1.0 / (1.0 + h)));
		return min(1.0, Kd);
	}

	//Kv	: vegetation species factor
	//Kd	: vegetation density factor
	//Kmc	: microclimate factor
	//Ksm	: managed stress factor.
	//Kv	: can be considered to be the ratio of ETL to ETo for a specific single or mixture of plant species under full or nearly
	//	full ground cover and full soil water supply.
	double CASCE_ET_Land::GetKL(TLVegetation v, TStress s, double fc_eff, double h)
	{
		//Table 8.10. Vegetation factors(Kv) for general plant types.
		//Table 8.12. Microclimate factor, Kmc, for different plant types(from Irrigation Association, 2003).
		//Table 8.13. Managed stress factors(Ksm) for general plant types
		//and the general values of depletion fraction for no stress.
		static const CKLInfo INFO[NB_VEGETATIONS] =
		{
			{ "Trees", 1.15, 1.4, 1.0, 0.5, 0.4, 0.6, 0.8, 0.6 },
			{ "Shrubs desert species", 0.70, 1.3, 1.0, 0.5, 0.3, 0.4, 0.6, 0.6 },
			{ "Shrubs nondesert species", 0.80, 1.3, 1.0, 0.5, 0.4, 0.6, 0.8, 0.6 },
			{ "Groundcover", 1.0, 1.2, 1.0, 0.5, 0.3, 0.5, 0.8, 0.5 },
			{ "Annuals(flowers)", 0.90, 1.2, 1.0, 0.5, 0.5, 0.7, 0.8, 0.4 },
			{ "Mixture of trees, shrubs, and groundcover", 1.20, 1.4, 1.0, 0.5, 0.4, 0.6, 0.8, 0.6 },
			{ "Cool season turfgrass", 0.90, 1.2, 1.0, 0.8, 0.7, 0.8, 0.9, 0.4 },
			{ "Warm season turfgrass", 0.90, 1.2, 1.0, 0.8, 0.6, 0.7, 0.8, 0.5 },
		};

		enum TKmc{ KMC_Hi, KMC_AVERAGE, KMC_LO };

		double Kd = GetKd(fc_eff, h);
		double KL = INFO[v].Kv * Kd*INFO[v].Kmc[KMC_AVERAGE] * INFO[v].Ksm[s];

		return KL;
	}

	ERMsg CASCE_ET_Land::Execute(const CWeatherStation& weather, CModelStatVector& stats)
	{
		ERMsg msg;

		return msg;
	}
	//"Trees;Shrubs desert species;Shrubs nondesert species;Groundcover;Annuals(flowers);Mixture of trees, shrubs, and groundcover;Cool season turfgrass;Warm season turfgrass"


}