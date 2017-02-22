//*********************************************************************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//*********************************************************************************************************************************

#pragma once

#include <map>
#include <vector>
#include <math.h>
#include <assert.h>

#include "Basic/UtilTime.h"
#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"

namespace WBSF
{
	

	//*********************************************************************************************************************************
	//CETOptions
	extern const char ET_HEADER[];

	class CETOptions : public std::map < std::string, std::string >
	{
	public:

		//options for all ET type

		CETOptions();

		bool OptionExist(std::string name)const
		{
			return find(name) != end();
		}

		std::string GetOption(std::string name)const
		{
			std::string option;
			CETOptions::const_iterator it = find(name);
			if (it != end())
				option = it->second;

			return option;
		}
	};


	//*********************************************************************************************************************************
	//ETInterface
	struct ETInterface
	{
	public:

		enum TETHourlyStat{ S_ET, NB_ET_STATS };
		typedef CModelStatVectorTemplate<NB_ET_STATS, ET_HEADER> CETVector;


		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats) = 0;
		virtual ERMsg SetOptions(const CETOptions& options) = 0;
		
	};


	typedef std::shared_ptr<ETInterface> CETPtr;
	typedef ETInterface* (__stdcall *CreateETFn)(void);


	//*********************************************************************************************************************************
	//CETFactory
	// Factory for creating instances of IET
	class CETFactory
	{
	public:
		~CETFactory() { m_factoryMap.clear(); }

		static CETFactory& Get()
		{
			static CETFactory INSTANCE;
			return INSTANCE;
		}


		static bool Register(const std::string &name, CreateETFn pfnCreate);
		static bool IsRegistered(const std::string &name);
		static CETPtr CreateET(const std::string &name);

	protected:

		CETFactory();
		CETFactory(const CETFactory &) { }
		CETFactory &operator=(const CETFactory &) { return *this; }

		typedef std::map<std::string, CreateETFn> FactoryMap;
		FactoryMap m_factoryMap;
		CCriticalSection m_CS;

	};


	//*********************************************************************************************************************************
	//CETBase
	class CETBase : public ETInterface
	{
	public:
		
		virtual ERMsg SetOptions(const CETOptions& options){ return ERMsg(); }//do nothing
	};



	//*********************************************************************************************************************************
	//CThornthwaiteET : this class computes the Thornthwaite potential evapotranspiration
	//Variable need : TAir
	class CThornthwaiteET : public CETBase
	{
	public:

		enum TType{ POTENTIEL_STANDARD, POTENTIEL_ADJUSTED };
		static ETInterface* __stdcall Create(){ return new CThornthwaiteET; }

		//options
		size_t m_type;

		CThornthwaiteET(size_t type = POTENTIEL_STANDARD);

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);


		double GetI(const CWeatherYear& weather);
		double GetAlpha(double I);
		double GetET(const CWeatherMonth& weather, double I);

	protected:

		static double GetCorrectionFactor(double lat, size_t m);
		static const bool AUTO_REGISTER;

	};




	//*********************************************************************************************************************************
	//CBlaneyCriddleET : this class computes the Blaney-Criddle potential evapotranspiration
	//Variable need : TAir
	class CBlaneyCriddleET : public CETBase
	{
	public:

		enum TCrop { PASTURE_GRASS, ALFALFA, GRAPES, DECIDOUS_ORCHARD, NB_TYPE };
		static ETInterface* __stdcall Create(){ return new CBlaneyCriddleET; }

		//options
		TCrop m_cropType;						//type of crop

		CBlaneyCriddleET();
		CBlaneyCriddleET(const CWeatherYear& weather, double lat, short type = PASTURE_GRASS);

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:


		static double GetCorrectionFactor(double lat, size_t month);
		class CCorrectionFactorsMonth
		{
		public:

			double m_latitude;		//latitude in degree
			double m_cf[12];		//correction factor
		};

		double CBlaneyCriddleET::GetCropFactor(TCrop type, size_t month)
		{
			_ASSERTE(type >= 0 && type < NB_TYPE);
			_ASSERTE(month >= 0 && month < 12);

			return CropFactors[type].m_cf[month];
		}

	protected:

		class CCropFactorsMonth
		{
		public:

			char* m_name;
			size_t m_begin;
			size_t m_end;
			size_t m_length;
			double m_cf[12];		//correction factor
		};


		static const CCorrectionFactorsMonth CorrectionFactors[10];
		static const CCropFactorsMonth CropFactors[NB_TYPE];
		static const bool AUTO_REGISTER;
	};



	//*********************************************************************************************************************************
	//CHamonET : Hamon potential evapotranspiration
	//Variable need : Tair


	class CHamonET : public CETBase
	{
	public:

		static const double KPEC;
		static ETInterface* __stdcall Create(){ return new CHamonET; }

		CHamonET();


		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:

		static const bool AUTO_REGISTER;
	};

	//*********************************************************************************************************************************
	//ModifiedHamonET : Modified Hamon potential evapotranspiration
	//Variable need : Tair

	class CModifiedHamonET : public CETBase
	{
	public:

		static const double KPEC;
		static ETInterface* __stdcall Create(){ return new CModifiedHamonET; }


		CModifiedHamonET();

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:

		static const bool AUTO_REGISTER;
	};

	//*********************************************************************************************************************************
	//ModifiedHamonET : Modified Hamon potential evapotranspiration
	//Variable need : Tair

	class CHargreavesET : public CETBase
	{
	public:

		static const double KPEC;
		static ETInterface* __stdcall Create(){ return new CHargreavesET; }

		CHargreavesET();


		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:

		static const bool AUTO_REGISTER;
	};



	//*********************************************************************************************************************************
	//CTurcET : turk evapotranspiration
	// not validated
	//Variable need : Tair, Humidity, Solar Radiation
	class CTurcET : public CETBase
	{
	public:

		static ETInterface* __stdcall Create(){ return new CTurcET; }

		CTurcET();
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:

		static const bool AUTO_REGISTER;
	};

	//*********************************************************************************************************************************
	//CPriestleyTaylorET : Priestley-Taylor potential evapotranspiration
	// 
	//Variable need : Tair, Humidity, Wind Speed, Solar Radiation
	class CPriestleyTaylorET : public CETBase
	{
	public:

		class CAlpha
		{
		public:
			double	m_α;
			char*	m_name;
			char*	m_ref;
		};


		enum TPriestleyTaylorFactor{
			STRONGLY_ADVECTIVE_CONDITIONS, GRASS_SOIL_AT_FIELD_CAPACITY, IRRIGATED_RYEGRASS, SATURATED_SURFACE, OPEN_WATER_SURFACE, WET_MEADOW,
			WET_DOUGLAS_FIR_FOREST, SHORT_GRASS, BOREAL_BROAD_LEAVED, DOUGLAS_FIR_FOREST, BARE_SOIL_SURFACE, MIXED_REFORESTATION_WATER_LIMITED, PONDEROSA_PINE_WATER_LIMITED_DAYTIME,
			TROPICAL_BROAD_LEAVED, DOUGLAS_FIR_FOREST_UNTHINNED, BROAD_LEAVED, TEMPERATE_BROAD_LEAVED, DOUGLAS_FIR_FOREST_THINNED,
			DOUGLAS_FIR_FORES_DAYTIME, SPRUCE_FOREST_DAYTIME, TEMPERATE_CONIFEROUS, CONIFEROUS, BOREAL_CONIFEROUS, NB_ALPHA
		};

		static double GetAlpha(TPriestleyTaylorFactor f){ return PRE_DEFINE_ALHA[f].m_α; }
		static ETInterface* __stdcall Create(){ return new CPriestleyTaylorET; }

		//options
		double m_α;	//Priestley-Taylor correction factor

		CPriestleyTaylorET();

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);


	protected:

		double calc_pet(double rad, double ta, double pa, double dayl);
		//double atm_pres(double alt);
		static const CAlpha PRE_DEFINE_ALHA[NB_ALPHA];
		static const bool AUTO_REGISTER;

	};


	//*********************************************************************************************************************************
	//CModifiedPriestleyTaylorET : this class computes the Thornthwaite potential evapotranspiration
	//Variable need : Tair, Humidity, Wind Speed, Solar Radiation
	class CModifiedPriestleyTaylorET : public CETBase
	{

	public:


		enum TMethod{ HOLTSLAG_VAN_ULDEN, ALPHA_OMEGA, NB_METHOD };
		static ETInterface* __stdcall Create(){ return new CModifiedPriestleyTaylorET; }

		//options
		size_t m_method;	//Priestley-Taylor correction factor type
		CModifiedPriestleyTaylorET();

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

		double GetET(double rad, double ta, double pa, double dayl);
	protected:


		static const bool AUTO_REGISTER;

	};

	//*********************************************************************************************************************************
	//Penman-Monteith
	//Variable need : Tair, Humidity, Wind Speed, Solar Radiation
	class CPenmanMonteithET : public CETBase
	{
	public:

		static ETInterface* __stdcall Create(){ return new CPenmanMonteithET; }

		CPenmanMonteithET();

		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

	protected:

		static const bool AUTO_REGISTER;
	};
	
	//*********************************************************************************************************************************
	//ASCE_ETsz
	//Variable need : Tair, Humidity, Wind Speed, Solar Radiation
	class CASCE_ETsz : public CETBase
	{
	public:

		enum TReference { SHORT_REF, GRASS = SHORT_REF, LONG_REF, ALFALFA = LONG_REF, NB_REF = 2 };
		enum TExtendedStat{ S_RA = NB_ET_STATS, S_RSO, S_FCD, S_RNL, S_RNS, S_RN, NB_EXTENDED_STATS };
		static ETInterface* __stdcall Create(){ return new CASCE_ETsz; }

		//SHORT_REF is equivalent to grass
		//LONG_REF is equivalent to alfalfa
		//option
		TReference m_referenceType;
		//bool m_bExtended;
		
		CASCE_ETsz(size_t referenceType = SHORT_REF, bool extended = false);

		virtual ERMsg SetOptions(const CETOptions& options);
		virtual void Execute(const CWeatherStation& weather, CModelStatVector& stats);

		static double GetETsz(const double& Rn, const double& G, const double& T, const double& u2, const double& Ea, const double& Es, const double& Δ, const double& γ, const double& Cn, const double& Cd);
		static double GetMeanAirTemperature(double Tmin, double Tmax);
		static double GetAtmosphericPressure(double z);
		static double GetPsychrometricConstant(double P);
		static double GetSlopeOfSaturationVaporPressure(double T);
		static double e°(double T);
		static double GetSaturationVaporPressure(double Tmin, double Tmax);
		static double GetActualVaporPressure(double Tdew);
		static double GetActualVaporPressure(double RHmean, double Tmin, double Tmax);
		static double GetNetRadiation(double Rns, double Rnl);
		static double GetNetShortWaveRadiation(double Rs);
		static double GetNetLongWaveRadiation(double Tmin, double Tmax, double Ea, double Fcd);
		//static double GetCloudiness(CTRef TRef, const CLocation& loc, double Rs){ return GetCloudiness(TRef, loc.m_lat, loc.m_lon, loc.m_alt, Rs); }
		//static double GetCloudiness(double Ra, double Rs);
		static double GetCloudinessFunction(double Rs, double Rso);
		static double GetClearSkySolarRadiation(double Ra, double z);
		static double GetInverseRelativeDistanceFactor(int J);
		static double GetSolarDeclination(int J);
		static double GetSunsetHourAngle(double ϕ, double δ);
		static double GetExtraterrestrialRadiation(double lat, int J);
		static double GetExtraterrestrialRadiationH(double lat, int J);
		static double GetWindProfileRelationship(double uz, double zw);
		static double GetCn(int type){ _ASSERTE(type >= 0 && type < NB_REF); return CorrectionFactors[type].m_Cn; }
		static double GetCd(int type){ _ASSERTE(type >= 0 && type < NB_REF); return CorrectionFactors[type].m_Cd; }

		static double GetCnH(int type, bool bDaytime){ _ASSERTE(type >= 0 && type < NB_REF); return CorrectionFactorsH[type][bDaytime ? 0 : 1].m_Cn; }
		static double GetCdH(int type, bool bDaytime){ _ASSERTE(type >= 0 && type < NB_REF); return CorrectionFactorsH[type][bDaytime ? 0 : 1].m_Cd; }
		static double GetGH(int type, double Rn);

		//specific hourly method
		static double GetNetLongWaveRadiationH(double T, double Ea, double Fcd);
		static double GetSeasonalCorrection(double J);
		static double GetBeginSolarTimeAngle(double ω);
		static double GetEndSolarTimeAngle(double ω);
		static double GetMidSolarTimeAngle(double t, double Lz, double Lm, double Sc);
		static void	  AdjustSolarTimeAngle(double& ω1, double& ω2, double ωs);
		static double GetCenterLocalTimeZone(double Lm);
		static double GetLm(double lon);
		static double GetAngleOfSunAboveHorizon(double ϕ, double δ, double ω);
		static double GetExtraterrestrialRadiationH(CTRef TRef, double lat, double lon, double alt);
		static double GetExtraterrestrialRadiationH(double ϕ, double δ, double dr, double ω1, double ω2);


	protected:

		void UnitTest();


		class CCorrectionFactors
		{
		public:
			//numerator constant that changes with reference type and calculation time step (K mm s3 Mg-1 d-1 or K mm s3 Mg-1 h-1)
			double m_Cn;
			//denominator constant that changes with reference type and calculation time step (s m-1).
			double m_Cd;
		};

		static const CCorrectionFactors CorrectionFactors[NB_REF];
		static const CCorrectionFactors CorrectionFactorsH[NB_REF][2];
		static const bool AUTO_REGISTER;
	};

	

	//*********************************************************************************************************************************
	//From AIMM
	class CKropCoeficient
	{
	public:

		enum TCoeficient{ A, B, C, D, E, NB_COEFICIENT };
		enum TKrop {
			ALFALFA_HAY, BARLEY, BROME_HAY, CANARY_SEED, CANOLA, DRY_BEAN, DRY_PEAS, FLAX, FRESH_CORN_SWEET, FRESH_PEAS,
			GRAIN_CORN, GRASS_HAY, GREEN_FEED, HARD_RED_SPRING_WEAT, LENTIL, MALT_BARLEY, MILK_VETCH, MONARDA, MUSTARD,
			NATIVE_PASTURE, OAT, OAT_SILAGE, ONIONS, POTATO, RYE, SAFFLOWER, SMALL_FRUIT, SOFT_WEAT, SUGAR_BEETS,
			SUNFLOWER, TIMOTHY_HAY, TRITICAL, TURN_SOD, WINTER_WEAT, NB_CROP
		};

		static const double KROP_COEFICIENT[NB_CROP][NB_COEFICIENT];

		//x is cumulative DD with a 5° C
		static double GetKc(size_t type, double x);


	};


}