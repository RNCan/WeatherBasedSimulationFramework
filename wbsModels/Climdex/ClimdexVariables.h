#pragma once

#include "Basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "Basic/WeatherDefine.h"

namespace WBSF
{
	
	enum TTPecentil{ P10, P50, P90, NB_T_PERCENTILS };
	enum TPPecentil{ P95, P99, NB_P_PERCENTILS };
	enum TPecentilVar{ PVAR_TMIN, PVAR_TMAX, NB_PERCENTILS_VAR };
	typedef std::array<std::array<std::array<double, NB_T_PERCENTILS>, NB_PERCENTILS_VAR>, 365> CTPercentil;
	typedef std::array<double, NB_P_PERCENTILS>  CPPercentil;
	typedef std::array<CPPercentil, 12>  CPPercentilM;
	
	

	class CClimdexNormals
	{
	public:

		static const HOURLY_DATA::TVarH VARIABLES[NB_PERCENTILS_VAR];
		CClimdexNormals();

		bool m_bUseBootstrap;
		CTPeriod m_period;

		void clear();
		void Compute(const CWeatherYears& weather);

		
		double GetEventPercent(const CWeatherDay& day, TPecentilVar v, TTPecentil p)const;
		double GetTThreshold(size_t jd, TPecentilVar v, TTPecentil p)const{ return m_Tpercentil[0][0][jd][v][p];}
		double GetPThreshold(size_t m, TPPecentil p)const{ return m_PpercentilM[m][p]; }
		double GetPThreshold(TPPecentil p)const{ return m_PpercentilA[p]; }

	protected:

		
		static void Tthreshold(const CWeatherYears& weather, CTPeriod period, bool bUseBootstrap, std::vector<std::vector<CTPercentil>>& Tpercentil);
		static void Pthreshold(const CWeatherYears& weather, CTPeriod period, CPPercentil& percentilA, CPPercentilM& percentilM);

		
		std::vector<std::vector<CTPercentil>> m_Tpercentil;
		CPPercentil m_PpercentilA;
		CPPercentilM m_PpercentilM;
		
	};

	class CClimdexVariables
	{
	public:

		
		enum TClimdex { FD, SU, ID, TR, GSL, TXX, TNX, TXN, TNN, TN10, TX10, TN90, TX90, WSDI, CSDI, DTR, RX1D, RX5D, SDII, R10MM, R20MM, RNNMM, CDD, CWD, R95P, R99P, PRCP, NB_VARIABLES };
		
		CClimdexVariables();

		
		CTPeriod m_basePeriod;
		double m_nn;  //user define precipitation threshold;
		bool m_bUseBootstrap;

		ERMsg Execute(CTM TM, const CWeatherStation& weather, CModelStatVector& output);
		double Get(size_t v, const CWeatherYear& weather, bool bForce = false);
		double Get(size_t v, const CWeatherMonth& weather, bool bForce = false);


		static double GetNumber(const CWeatherDay& weather, size_t index);
		static double GetNumber(const CWeatherMonth& weather, size_t index);
		static double GetTXX(const CWeatherMonth& weather);
		static double GetTNX(const CWeatherMonth& weather);
		static double GetTXN(const CWeatherMonth& weather);
		static double GetTNN(const CWeatherMonth& weather);
		
		
		
		
		static double GetTP(const CWeatherMonth& weather, const CClimdexNormals& N, TPecentilVar v, TTPecentil p);
		static double GetTN10p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetTP(weather, N, PVAR_TMIN, P10); }
		static double GetTX10p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetTP(weather, N, PVAR_TMAX, P10); }
		static double GetTN90p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetTP(weather, N, PVAR_TMIN, P90); }
		static double GetTX90p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetTP(weather, N, PVAR_TMAX, P90); }
		static double GetDTR(const CWeatherMonth& weather);
		static double GetGSL(const CWeatherYear& weather, std::array<double, 12> &gsl);

		
		static double GetRX1DAY(const CWeatherMonth& weather);
		static double GetRX5DAY(const CWeatherMonth& weather);
		static CStatistic GetRnnmm(const CWeatherMonth& weather, double nn);
		static double GetSDII(const CWeatherMonth& weather){ CStatistic stat = GetRnnmm(weather, 1.0);  return stat.IsInit() ? stat[MEAN] : -999; }
		static double GetR10mm(const CWeatherMonth& weather){ CStatistic stat = GetRnnmm(weather, 10);  return stat[NB_VALUE]; }
		static double GetR20mm(const CWeatherMonth& weather){ CStatistic stat = GetRnnmm(weather, 20);  return stat[NB_VALUE]; }

		static double GetRp(const CWeatherMonth& weather, const CClimdexNormals& N, TPPecentil p);
		static double GetR95p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetRp(weather, N, P95); }
		static double GetR99p(const CWeatherMonth& weather, const CClimdexNormals& N){ return GetRp(weather, N, P99); }
		static double GetRp(const CWeatherYear& weather, const CClimdexNormals& N, TPPecentil p);
		static double GetR95p(const CWeatherYear& weather, const CClimdexNormals& N){ return GetRp(weather, N, P95); }
		static double GetR99p(const CWeatherYear& weather, const CClimdexNormals& N){ return GetRp(weather, N, P99); }
		static double GetPRCP(const CWeatherMonth& weather);
		

		static size_t GetPreviousCnt(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p);
		static double GetSDI(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar v, TTPecentil p, std::array<double, 12> &sdi);
		static double GetWSDI(const CWeatherYear& weather, const CClimdexNormals& N, std::array<double, 12> &sdi){ return GetSDI(weather, N, PVAR_TMAX, P90, sdi); }
		static double GetCSDI(const CWeatherYear& weather, const CClimdexNormals& N, std::array<double, 12> &sdi){ return GetSDI(weather, N, PVAR_TMIN, P10, sdi); }

		static size_t GetPreviousCD(const CWeatherYear& weather, bool bWet);
		static size_t GetNextCD(const CWeatherYear& weatherIn, bool bWet, size_t cnt);
		static double GetCD(const CWeatherYear& weather, bool bWet, std::array<double, 12> &cd);
		static double GetCDD(const CWeatherYear& weather, std::array<double, 12> &cd){ return GetCD(weather, false, cd); }
		static double GetCWD(const CWeatherYear& weather, std::array<double, 12> &cd){ return GetCD(weather, true, cd); }



		static double GetTXX(const CWeatherYear& weather);
		static double GetTNX(const CWeatherYear& weather);
		static double GetTXN(const CWeatherYear& weather);
		static double GetTNN(const CWeatherYear& weather);
		static double GetDTR(const CWeatherYear& weather);
		static double GetRX1DAY(const CWeatherYear& weather);
		static double GetPRCP(const CWeatherYear& weather);


		static double GetRX5DAY_old(const CWeatherYear& weather);

	protected:

		CClimdexNormals m_N;
	};

}