#pragma once
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	extern const char WETNESS_DURATION_HEADER[];
	enum TWDAnnualStat{ WETNESS_DURATION, NB_WETNESS_DURATION_STATS };


	typedef CModelStatVectorTemplate<NB_WETNESS_DURATION_STATS, WETNESS_DURATION_HEADER> CWetnessDurationStat;
	typedef CModelStatVectorTemplate<1> CObsStatVector;

	class CDewDuration;
	class CSWEB;

	class CWetnessDurationModel : public CBioSIMModelBase
	{
	public:

		enum ModelType{ DAILY_SINUS, EXTENDED_TRESHOLD, FIXED_THRESHOLD, DPD, CART, PM, SWEB, NB_MODEL };
		enum WetnessDurationType{ DP_09, DP_15, DP_MN, DP_HI, NB_DP_TYPE };
		CWetnessDurationModel();
		virtual ~CWetnessDurationModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		//virtual ERMsg OnExecuteAnnual();
		//virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();

		virtual void AddHourlyResult(const StringVector& header, const StringVector& data);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void AddMonthlyResult(const StringVector& header, const StringVector& data);
		virtual bool GetFValueHourly(CStatisticXY& stat);
		virtual bool GetFValueDaily(CStatisticXY& stat);
		virtual bool GetFValueMonthly(CStatisticXY& stat);
		
		void ExecuteDaily(CWetnessDurationStat& dailyStat);
		void ExecuteHourly(CWetnessDurationStat& stat);

		static CBioSIMModelBase* CreateObject(){ return new CWetnessDurationModel; }

		static void Hourly2Daily(const CWetnessDurationStat& hStat, CWetnessDurationStat& stat);

	protected:

		double GetWetnessDuration( CWeatherStation& weather, CTRef ref);
		double GetWetnessDurationSinus( CWeatherDay& wDay)const;
		double GetWetnessDurationExtT( CDay& hourlyDay)const;
		double GetWetnessDurationFixT( CDay& hourlyDay)const;
		double GetWetnessDurationDPD( CDay& hourlyDay)const;
		double GetWetnessDurationCART( CDay& hourlyDay)const;
		double GetWetnessDurationPM( CWeatherStation& weather, CTRef ref)const;
		double GetWetnessDurationSWEB( CWeatherStation& weather, CTRef ref)const;

		void GetObsStat(CObsStatVector& stat)const;
		void GetObsStatH(CObsStatVector& stat)const;

		int m_model;

		//simus model
		//int m_DPType;

		//temporal computed for Extended threshold,
		double m_previousRH;
		bool m_bWetPrevious;

		//Fixed threshold
		int m_RHThreshold;

		//PM
		int m_canopy;
		int m_exposure;

		//SWEB
		double m_LAI;	//Leaf Area Index
		double m_Zc;	//Canopy Height (m)


		//tmp
		CWetnessDurationStat m_PMStat;

		CWetnessDurationStat m_SWEBStat;
		//CWeatherStation m_weather;
		//CWeatherStation m_obs;
		//CWeatherStation m_dailyData;

		//CDewDuration m_PM;
		//CSWEB m_SWEB;

		double m_x0;
		double m_x1;
		double m_x2;
		double m_x3;
		double m_x4;

		//CObsStatVector m_obsD;
		CWeatherStation m_obsWeather;

		enum TInputWDHourly{ H_T, H_PRES, H_PRCP, H_TDEW, H_RH, H_WIND_SPEED, H_SRAD, H_NRAD1, H_NRAD2, H_WETNESS, H_VPD, NB_INPUT_HOURLY };
		enum TInputWDDaily{ D_TMIN, D_TMAX, D_PRES, D_PRCP, D_TDEW, D_RH, D_WIND_SPEED, D_SRAD, D_WETNESS_DURATION, D_VPD, NB_INPUT_DAILY };
		enum TInputWDMonthly{ M_TMIN, M_TMAX, M_PRES, M_PRCP, M_TDEW, M_RH, M_WIND_SPEED, M_SRAD, M_WETNESS_DURATION, M_VPD, NB_INPUT_MONTHY };
		//enum {SWEB_T,SWEB_PRCP,SWEB_RH,SWEB_WIND,SWEB_SRAD,SWEB_RAD1,SWEB_RAD2, SWEB_W, NB_SWEB};
	};



	class CDewDuration
	{
	public:



		int m_canopy;
		int m_exposure;
		double m_cropHeight;
		double m_x0;
		double m_x1;
		double m_x2;
		double m_x3;
		double m_x4;




		CLocation m_loc;


		enum TCanopy { CORN, SOYBEAN, APPLE, NB_CANOPY };
		enum TExposure { EXPOSED_LEAF, SHADED_LEAF, NB_EXPOSURE };

		CDewDuration();
		void Execute(const CWeatherStation& weather, CWetnessDurationStat& stat);
		//void Execute(const CWeatherStation& hourlyData, CWetnessDurationStat& stat);



		static const double σ;

		double GetΔT(double Rs, double RL, double Ta, double hc, double hw, double P, double esa, double e);
		double GetTl(double Ta, double ΔT);
		double GetLE(double hw, double P, double esl, double e);
		double Getδ(double jDay);
		//double GetTn(double λ, double Ω, int year, int m, int d);
		double GetRst(double λ, double δ, double t, double tn);
		double GetRsc(double Rst, double c);
		double GetRsd(double λ, double δ, double t, double tn);
		double GetRcd(double λ, double δ, double t, double tn);
		double GetRd(double Rsd, double Rcd, double c);
		double GetRL(double Tws, double ews, double c);
		double GetRLt(double RL, double Ta);
		double GetTa(double Tws, double Rn);
		double GetRn(double Rs, double RL, double Tws);
		double GetUc(double Zc, double Zws, double Uws, double Rn);
		double GetUh(double Uc, double Zh, double Zc);
		double Gethc(double U);
		double Gethw(double L, double Cp, double hc);


	};


	//Simulation of surface wetness with a water budget and energy balance approach
	//R.D. Magarey a,*, J.M. Russo b, R.C. Seem a
	//a Department of Plant Pathology, NYSAES, Geneva, NY 14456, United States
	//b ZedX Inc., 369 Rolling Ridge Drive, Bellefonte, PA 16823, United States
	//Received 11 April 2006; accepted 23 August 2006
	class CSWEB
	{
	public:

		enum TStat{ WETNESS_DURATION, NB_WETNESS_DURATION_STATS };


		double m_LAI;	//Leaf Area Index
		double m_Zc;	//Canopy Height (m)


		double m_x0;
		double m_x1;
		double m_x2;
		double m_x3;
		double m_x4;

		CSWEB()
		{
			m_LAI = 1;
			m_Zc = 0.1;
		}


		void Execute(const CWeatherStation& weather, CModelStatVector& stat);

		static int GetSW(double Wind);
		static double GetWind(double S, double C);
		static double GetW(double Wind, double Wmax);
		static double GetWmax();
		static double GetC(double LAI, double Cl);
		static double GetS(double I, double Dp, double E);
		static double GetI(double LAI, double P);
		static double GetEp(double Δ, double λ, double δ, double p, double Cp, double h, double Ea, double Es);
		static double GetE(double Ep, double W);
		static double GetDp(double Δ, double λ, double δ, double Rnc);
		static double Geth(double c, double Uc);
		static double Getc(double W);
		static double GetUc(double Uz, double Z, double Zc);

	
	};

}