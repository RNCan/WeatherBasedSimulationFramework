//**********************************************************************
// 20/09/2016	1.2.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 28/05/2015 	1.0.0   Rémi Saint-Amant	Incorporate in BioSIMModelBase
//**********************************************************************
//variables :
//1			annual_tmin			Minimum annual temperature(°C)
//2		*	annual_temp_mean	Mean annual temperature(°C)
//3			annual_tmax			Maximum annual temperature(°C)
//4		*	annual_precip		Total annual precipitation(mm)
//5			Warm_Prep			Warmest *quarter* total precipitation(mm)
//6		*	Warm_tmean			Warmest quarter mean temperature(°C)
//7			Cold_Prep			Coldest quarter total precipitation(mm)
//8			Cold_tmean			Coldest quarter mean temperature(°C)
//9			Wet_Prep			Wettest quarter total precipitation(mm)
//10		Wet_tmean			Wettest quarter mean temperature(°C)
//11		Dry_Prep			Driest quarter total precipitation(mm)
//12		Dry_tmean			Driest quarter mean temperature(°C)
//13		ann_arid			Annual aridity index(mm)
//14		WarmQ_AR			Warmest quarter aridity index(mm)
//15		ColdQ_AR			Coldest quarter aridity index(mm)
//16		WetQ_AR				Wettest quarter aridity index(mm)
//17	*	DryQ_AR				Driest quarter aridity index(mm)
//18		WM_tmean			Warmest month mean temperature(°C)
//19		CM_tmean			Coldest month mean temperature(°C)
//20		Maxprep_WeM			Total precipitation in the wettest month(mm)
//21		Minprep_DM			Total precipitation in the driest month(mm)
//22		sum_dd_5			Degree day accumulation >5°C between 1 April and 31 August
//*  = limited model (varaibles correletion)


#include <functional>   // std::greater
#include <algorithm>    // std::sort
#include "Basic/UtilMath.h"
#include "Basic/DegreeDays.h"
#include "Basic/Evapotranspiration.h"
#include "BlueStainVariables.h"

 
using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	#define extremVar(e) (e<2?H_PRCP:H_TNTX)

	extern const char * pVars = "Minimum annual temperature(°C)|Mean annual temperature(°C)|Maximum annual temperature(°C)|Total annual precipitation(mm)|Warmest *quarter* total precipitation(mm)|Warmest quarter mean temperature(°C)|Coldest quarter total precipitation(mm)|Coldest quarter mean temperature(°C)|Wettest quarter total precipitation(mm)|Wettest quarter mean temperature(°C)|Driest quarter total precipitation(mm)|Driest quarter mean temperature(°C)|Annual aridity index(mm)|Warmest quarter aridity index(mm)|Coldest quarter aridity index(mm)|Wettest quarter aridity index(mm)|Driest quarter aridity index(mm)|Warmest month mean temperature(°C)|Coldest month mean temperature(°C)|Total precipitation in the wettest month(mm)|Total precipitation in the driest month(mm)|Degree day accumulation >5°C between 1 April and 31 August";

	class SortExtrem
	{
	public:
		SortExtrem(CBlueStainVariables::TExtrem e) :
			m_e(e)
		{}

	

		inline bool operator() (const pair<CStatistic, CTPeriod>& struct1, const pair<CStatistic, CTPeriod>& struct2)
		{
			ASSERT(struct1.first.IsInit() && struct2.first.IsInit());

			bool bRep=false;
			
			if (m_e == CBlueStainVariables::DRIEST || m_e == CBlueStainVariables::COLDEST)
				bRep = (struct1.first[MEAN] < struct2.first[MEAN]);
			else
				bRep = (struct1.first[MEAN] > struct2.first[MEAN]);
			
			return bRep;
		}

		inline bool operator() (const pair<double, size_t> & struct1, const pair<double, size_t>& struct2)
		{
			bool bRep = false;
			if (m_e == CBlueStainVariables::DRIEST || m_e == CBlueStainVariables::COLDEST)
				bRep = (struct1.first < struct2.first);
			else
				bRep = (struct1.first > struct2.first);

			return bRep;
		}


	protected:

		CBlueStainVariables::TExtrem m_e;
	};


	CStatistic CBlueStainVariables::GetNormalStat(const CWeatherStation& weather, size_t m, TExtrem e)
	{
		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
			stat += (e < WARMEST ? weather[y][m][H_PRCP] : weather[y][m][H_TNTX]);

		return stat;
	}



	CTPeriod CBlueStainVariables::GetExtremQuarter(const CWeatherYear& weather, TExtrem e, bool bLoop)
	{
		int year = weather.GetTRef().GetYear();
		
		CTM TM = weather.GetTM();

		size_t max_m = bLoop ? 12 : 9;
		vector<pair<CStatistic, CTPeriod>> quarter(max_m);

		
		for (size_t m = 0; m < max_m; m++)
		{
			size_t mm = (m + 2);
			CTPeriod p(CTRef(year, m, 0, 0, TM), CTRef(year+int(mm/12), mm % 12, 0, 0, TM), CTPeriod::YEAR_BY_YEAR);
			quarter[m] = make_pair(CStatistic(), p);
		}

		for (size_t m = 0; m<quarter.size(); m++)
		{
			for (size_t i = 0; i < 3; i++)
			{
				size_t mm = m + i;
				CTRef TRef(weather.GetTRef().GetYear()+int(mm/12), mm % 12);

				if (weather.GetEntireTPeriod(CTM(CTM::MONTHLY)).IsInside(TRef))
					quarter[m].first += e<WARMEST ? weather[TRef][H_PRCP][MEAN] : weather[TRef][H_TNTX][MEAN];
			}
		}

		sort(quarter.begin(), quarter.end(), SortExtrem(e));

		return quarter.front().second;
	}



	size_t CBlueStainVariables::GetExtremMonth(const CWeatherStation& weather, TExtrem e)
	{
		array <pair<double, size_t>, 12> normals;
		
		for (size_t m = 0; m < 12; m++)
			normals[m] = make_pair(GetNormalStat(weather, m, e)[MEAN], m);
		

		sort(normals.begin(), normals.end(), SortExtrem(e));
		
		return normals.front().second;
	}


	size_t CBlueStainVariables::GetExtremMonth(const CWeatherYear& weather, TExtrem e)
	{
		array <pair<double, size_t>, 12> normals;
		
		for (size_t m = 0; m < 12; m++)
			normals[m] = make_pair(e<WARMEST ? weather[m][H_PRCP][MEAN] : weather[m][H_TNTX][MEAN], m);
		
		sort(normals.begin(), normals.end(), SortExtrem(e));

		return normals.front().second;
	}
	void CBlueStainVariables::GetSummerDD5(CWeatherStation& weather, CModelStatVector& ouptut)
	{
		CDegreeDays DD5(CDegreeDays::DAILY_AVERAGE, 5);
	
		ouptut.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), CDegreeDays::NB_OUTPUT, 0, CDegreeDays::HEADER);

		CModelStatVector tmp;
		DD5.Execute(weather, tmp);

		CTM TM = weather.GetTM();

		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
		{
			int year = weather.GetFirstYear() + int(y);

			CTPeriod p(CTRef(year, APRIL, 0, 0, TM), CTRef(year, AUGUST, LAST_DAY, LAST_HOUR, TM), CTPeriod::YEAR_BY_YEAR);
			ouptut[y][0] = tmp.GetStat(CDegreeDays::S_DD, p)[SUM];
		}
	}

	
	void CBlueStainVariables::GetWaterDeficit(const CWeatherStation& weather, CModelStatVector& output )
	{
		CThornthwaiteET Thornthwaite(CThornthwaiteET::POTENTIEL_STANDARD);

		CModelStatVector ET1;
		Thornthwaite.Execute(weather, ET1);
		
		CTStatMatrix ET2(ET1, CTM(CTM::MONTHLY));

		output.Init(ET2.m_period, 1);

		for (size_t y = 0; y<weather.size(); y++)
		{
			CStatistic statM;
			for (size_t m = 0; m < weather[y].size(); m++)
			{
				CTRef TRef(weather[y][m].GetTRef());
			
				const CStatistic& prcp = weather[y][m](H_PRCP);
				const CStatistic& ET = ET2(TRef, ETInterface::S_ET);

				double aridity = ET[SUM] - prcp[SUM];//allow negative values
	
				output[TRef][0] = aridity;
			}
		}
	}

	double CBlueStainVariables::GetAridity(const CModelStatVector& AR, CTPeriod p, bool bLimitToZero)
	{
		if (!p.IsInit())
			p = AR.GetTPeriod();

		p.Transform(CTM(CTM::MONTHLY));

		CStatistic statA;
		for (size_t y = 0; y<p.GetNbYears(); y++)
		{
			int year = p.GetFirstYear() + int(y);

			CStatistic statM;
			for (size_t m = 0; m < 12; m++)
			{
				CTRef TRef(year, m);
				if (p.IsInside(TRef))
				{
					double aridity = AR[TRef][0];
					
					if (bLimitToZero)
						aridity = max(0.0, aridity);

					statM += aridity;
				}
					
			}

			statA += statM[SUM];
		}

		return statA[MEAN];
	}

	void CBlueStainVariables::Execute(CWeatherStation& weather, CModelStatVector& output)
	{
		output.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), CBlueStainVariables::NB_VARIABLES, -999);

		CModelStatVector DD5;
		CModelStatVector WD;

		GetSummerDD5(weather, DD5);
		GetWaterDeficit(weather, WD);

	
		for (size_t y = 0; y < weather.size(); y++)
		{
			array < CTPeriod, NB_EXTREM> Q;	//quarter extrem
			array < size_t, NB_EXTREM> M;		//monthly extrem

			for (size_t e = 0; e < NB_EXTREM; e++)
				Q[e] = GetExtremQuarter(weather[y], TExtrem(e), false);

			for (size_t e = 0; e < NB_EXTREM; e++)
				M[e] = GetExtremMonth(weather[y], TExtrem(e));




			int year = weather.GetFirstYear() + int(y);
			for (size_t v = 0; v < CBlueStainVariables::NB_VARIABLES; v++)
			{
				switch (v)
				{
				case V_TMIN_EXT:	output[y][v] = weather[y][H_TMIN2][MEAN]; break;
				case V_TMEAN:		output[y][v] = weather[y][H_TNTX][MEAN]; break;
				case V_TMAX_EXT:	output[y][v] = weather[y][H_TMAX2][MEAN]; break;
				case V_PRCP:		output[y][v] = weather[y](H_PRCP)[SUM]; break;
				case V_WARMQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[WARMEST])[MEAN]; break;
				case V_COLDQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[COLDEST])[MEAN]; break;
				case V_WETQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[WETTEST])[MEAN]; break;
				case V_DRYQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[DRIEST])[MEAN]; break;
				case V_WARMQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[WARMEST])[SUM]; break;
				case V_COLDQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[COLDEST])[SUM]; break;
				case V_WETQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[WETTEST])[SUM]; break;
				case V_DRYQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[DRIEST])[SUM]; break;
				case V_AI:			output[y][v] = GetAridity(WD, CTPeriod(CTRef(year, FIRST_MONTH), CTRef(year, LAST_MONTH)), false); break;
				case V_WARMQ_AI:	output[y][v] = GetAridity(WD, Q[WARMEST], false); break;
				case V_COLDQ_AI:	output[y][v] = GetAridity(WD, Q[COLDEST], false); break;
				case V_WETQ_AI:		output[y][v] = GetAridity(WD, Q[WETTEST], false); break;
				case V_DRYQ_AI:		output[y][v] = GetAridity(WD, Q[DRIEST], false); break;
				case V_WARMM_TMEAN:	output[y][v] = weather[CTRef(year, M[WARMEST])][H_TNTX][MEAN]; break;
				case V_COLDM_TMEAN:	output[y][v] = weather[CTRef(year, M[COLDEST])][H_TNTX][MEAN]; break;
				case V_WETM_PRCP:	output[y][v] = weather[CTRef(year, M[WETTEST])][H_PRCP][SUM]; break;
				case V_DRYM_PRCP:	output[y][v] = weather[CTRef(year, M[DRIEST])][H_PRCP][SUM]; break;
				case V_SUMMER_DD5:	output[y][v] = DD5[y][0]; break;
				default: ASSERT(false);
				}
			}
		}
	}


}