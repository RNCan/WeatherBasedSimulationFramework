//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"
//#include <math.h>
//#include <fstream>
//#include <ostream>

#include "Basic/GrowingSeason.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	//NOTE: Begin and END are ZERO-BASED Julian dates
	//Source:
	//Boughner, C.C. 1964. Distribution of growing degree days in Canada. 
	//Can. Met. Memoirs 17. Met. Br., Dept. of Transport. 40 p.
	/*CTPeriod CWeatherYear::GetGrowingSeason(bool bAlwaysFillPeriod)const
	{
	int day = 200; //(Mid-July)
	bool frost=false;
	CTPeriod p( GetFirstTRef(), GetLastTRef());


	//Beginning of the growing season
	//look backward for the first occurrence of 3 successive days with frost
	do
	{

	frost = GetDay(day).GetTMin() < 0 &&
	GetDay(day-1).GetTMin() < 0 &&
	GetDay(day-2).GetTMin() < 0;

	day--;

	} while(!frost && day > 1);


	if(day>1 )
	{
	p.Begin().SetJDay(m_year, day+2);
	}


	//End of growing season
	day = 200;
	do
	{
	//look for the first occurrence of 3 successive days with frost
	frost = GetDay(day).GetTMin() < 0 &&
	GetDay(day+1).GetTMin() < 0 &&
	GetDay(day+2).GetTMin() < 0;
	day++;
	} while(!frost && day < GetLastDay()-2);

	if(day<GetLastDay()-2)
	{
	p.End().SetJDay( m_year, day-2);
	}

	if( p.End() < p.Begin() )
	{
	if( bAlwaysFillPeriod )
	p.End() = p.Begin() = CTRef(m_year, JULY, 14);
	else
	p.Reset();
	}

	return p;
	}
	*/

	double CGSInfo::GetGST(const CWeatherDay& data)const
	{
		ASSERT(m_type >= 0 && m_type<NB_TT_TEMPERATURE);

		double T = -999;
		//if (data.IsHourly())
	//	{
		switch (m_type)
		{
		case TT_TMIN:	T = data[H_TMIN2][MEAN]; break;
		case TT_TMEAN:	T = data[H_TAIR2][MEAN]; break;
		case TT_TMAX:	T = data[H_TMAX2][MEAN]; break;
		case TT_TNOON:  T = data[12][H_TAIR2]; break;
		default: ASSERT(false);
		}
		ASSERT(T > -999);//hourly values must be computed for TT_TNOON
		//}
		//else
		//{
		//	switch (m_type)
		//	{
		//	case TT_TMIN:	T = data[H_TMIN][MEAN]; break;
		//	case TT_TMEAN:	T = data[H_TAIR][MEAN]; break;
		//	case TT_TMAX:	T = data[H_TMAX][MEAN]; break;
		//	case TT_TNOON:  ASSERT(false); break; //hourly values must be computed
		//	default: ASSERT(false);
		//	}
		//}
		return T;
	}

	void CGrowingSeason::Execute(const CWeatherStation& weather, CModelStatVector& output)const
	{
		output.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), 2);

		for (size_t y = 0; y < weather.size(); y++)
		{
			CTPeriod p = GetGrowingSeason(weather[y]);
			output[y][0] = p.Begin().GetRef();
			output[y][1] = p.End().GetRef();
		}
	}
	
	CTPeriod CGrowingSeason::GetGrowingSeason(const CWeatherYear& weather)const
	{
		
		CTPeriod p = weather.GetEntireTPeriod();
		CTM TM = p.GetTM();
		int year = p.Begin().GetYear();

		CTRef TRef1 = CTRef(year, JULY, DAY_15);
		CTRef TRef2 = CTRef(year, JULY, DAY_15);
		
		bool bGetIt1 = false;

		//Beginning of the growing season
		//look backward for the first occurrence of 3 successive days with frost
		do
		{
			TRef1--;
			bGetIt1 = true;
			for (size_t dd = 0; dd<m_begin.m_nbDays&&bGetIt1; dd++)
			{
				ASSERT((TRef1 - dd).Transform(p.GetTM()) >= p.Begin());

				const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef1 - dd]);
				bGetIt1 = m_begin.GetGST(wDay) < m_begin.m_threshold;
			}
		} while (!bGetIt1 && (TRef1 - p.Begin())>m_begin.m_nbDays);


		if (bGetIt1)
		{
			p.Begin() = TRef1 + 1;
			p.Transform(TM);
		}
			


		bool bGetIt2 = false;
		//End of growing season
		do
		{
			TRef2++;
			bGetIt2 = true;
			//look for the first occurrence of 3 successive days with frost
			for (size_t dd = 0; dd<m_end.m_nbDays&&bGetIt2; dd++)
			{
				ASSERT((TRef2 + dd).Transform(p.GetTM())<=p.End());

				const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef2 + dd]);
				bGetIt2 = m_end.GetGST(wDay) < m_end.m_threshold;
			}
		} while (!bGetIt2 && (p.End()-TRef2)>m_end.m_nbDays);

		if (bGetIt2)
		{
			p.End() = TRef2 - 1;
			p.Transform(TM);
		}
		
		if (!bGetIt1 && !bGetIt2)
		{
			const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[CTRef(year, JULY, DAY_15)]);
			if (m_end.GetGST(wDay) < m_end.m_threshold)
				p.clear();//no growing season
			//else
				//p = weather.GetEntireTPeriod(CTM(CTM::DAILY)); //growing season all along the year
		}

		return p;
	}


	//FrostFree period: get the longest frost free period of the year
	CTPeriod CGrowingSeason::GetFrostFreePeriod(const CWeatherYear& weather)const
	{
		//CTPeriod p = 
		//int year = p.Begin().GetYear();
		CTPeriod period = weather.GetEntireTPeriod();
		CTPeriod pTmp = period;
		CTPeriod p;
		bool notInit = true;

		
		for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
		{
			//const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef]);
			double T = weather.IsHourly() ? weather[TRef][H_TAIR2][MEAN] : weather[TRef][H_TMIN2][MEAN];

			if (T>0) //Frost-free period begin or continues
			{
				if (notInit)
				{
					pTmp.Begin() = TRef;
					notInit = false;
				}
			}
			else
			{
				if (!notInit)
				{
					pTmp.End() = TRef - 1;
					notInit = true;

					//Frost-free period ends
					if (pTmp.GetLength() > p.GetLength())
						p = pTmp;
				}
			}

			if (TRef == pTmp.End() && !notInit)
			{
				pTmp.End() = TRef;
				if (pTmp.GetLength() > p.GetLength())
					p = pTmp;
			}
		}

		return p;
	}

	CTRef CGSInfo::GetFirst(CWeatherYear& weather, char sign)const
	{
		CTPeriod p = weather.GetEntireTPeriod();
		//int year = p.Begin().GetYear();

		CTRef firstTRef;

		//Beginning of fire weather 
		//look for the first occurrence of 3 successive days T > T_THRESHOLD
		CTRef lastTRef = p.End() - (int)m_nbDays;
		bool bGetIt = false;
		for (CTRef TRef = p.Begin(); TRef <= lastTRef && !bGetIt; TRef++)
		{
			bGetIt = true;
			for (size_t dd = 0; dd<m_nbDays&&bGetIt; dd++)
			{
				//const CDataInterface& data = weather[TRef + dd];
				const CWeatherDay& wDay = weather.GetDay(TRef + dd);
				if (sign == '>')
					bGetIt = GetGST(wDay) > m_threshold;
				else
					bGetIt = GetGST(wDay) < m_threshold;
			}

			if (bGetIt)
			{
				firstTRef = TRef + (int)m_nbDays;//begin after these 3 days
			}
		}

		return firstTRef;
	}

	CTRef CGSInfo::GetLast(CWeatherYear& weather, char sign)const
	{
		CTRef lastTRef;

		CTPeriod p = weather.GetEntireTPeriod();
		//Beginning of fire weather 
		//look for the first occurrence of 3 successive days Tnoon > T_THRESHOLD
		CTRef firstTRef = p.Begin() + (int)m_nbDays;
		bool bGetIt = false;
		for (CTRef TRef = p.End(); TRef >= firstTRef && !bGetIt; TRef--)
		{
			bGetIt = true;
			for (size_t dd = 0; dd<m_nbDays&&bGetIt; dd--)
			{
				//const CWeatherDay& Wday = GetDay(d - dd);
				//const CDataInterface& data = weather[TRef - dd];
				const CWeatherDay& wDay = weather.GetDay(TRef + dd);
				if (sign == '>')
					bGetIt = bGetIt && GetGST(wDay) > m_threshold;
				else
					bGetIt = bGetIt && GetGST(wDay) < m_threshold;
			}


			if (bGetIt)
			{
				lastTRef = TRef - (int)m_nbDays;//end before these 3 days
			}
		}

		return lastTRef;
	}

	
	//CTRef CWeatherYear::GetLastSnowDay(int MINIMUM_SNOW_DEPTH, int NB_DAY_MIN)const
	//{
	//	const CWeatherYear& me = *this;

	//	CTRef firstDay;
	//	short nbDay = 0;

	//	CTRef begin(GetYear(), JULY, 14);
	//	CTRef end(GetYear(), JANUARY, FIRST_DAY);


	//	for (CTRef d = begin; d >= end&&!firstDay.IsInit(); d--)
	//	{
	//		if (me[d][SNDH] > MINIMUM_SNOW_DEPTH)//more than 2 cm
	//		{
	//			nbDay++;
	//			if (nbDay>NB_DAY_MIN && !firstDay.IsInit())
	//			{
	//				firstDay = d + NB_DAY_MIN;
	//			}
	//		}
	//		else nbDay = 0;
	//	}

	//	return firstDay;
	//}

	//CTRef CWeatherYear::GetFirstSnowDay(int MINIMUM_SNOW_DEPTH, int NB_DAY_MIN)const
	//{
	//	//static const long NB_DAY_MIN=7;
	//	const CWeatherYear& me = *this;

	//	CTRef firstDay;
	//	short nbDay = 0;

	//	CTRef begin(GetYear(), JULY, 14);
	//	CTRef end(GetYear(), DECEMBER, LAST_DAY);


	//	for (CTRef d = begin; d <= end&&!firstDay.IsInit(); d++)
	//	{
	//		if (me[d][SNDH] > MINIMUM_SNOW_DEPTH)
	//		{
	//			nbDay++;
	//			if (nbDay>NB_DAY_MIN && !firstDay.IsInit())
	//			{
	//				firstDay = d - NB_DAY_MIN;
	//			}
	//		}
	//		else nbDay = 0;
	//	}

	//	return firstDay;
	//}

	//return Water Deficit en mm of wather
	//double CWeatherYear::GetWaterDeficit()const
	//{
	//	CThornthwaitePET TPET(m_weather[y], 0, CThornthwaitePET::POTENTIEL_STANDARD);
	//	return TPET.GetWaterDeficit(m_weather[y]);//in mm 
	/*
	_ASSERTE( m_months[0][0].GetPpt() >= 0);
	//est-ce qu'on devrait utiliser cette équation à la place???
	//return max( 0, GetStat(PET, SUM) -	GetStat(PPT, SUM));

	CThornthwaitePET PET(*this, 0, CThornthwaitePET::POTENTIEL_STANDARD);
	double A = 0;
	//calculer Et pour le mois et A
	for(int m=0; m<12; m++)
	{
	if(m_months[m].GetStat(STAT_T_MN, MEAN)>0.)
	{
	//precipitation in mm
	double A_tmp=(PET.Get(m)-m_months[m].GetStat(STAT_PRCP, SUM));
	if(A_tmp>0.)
	A += A_tmp;
	}
	}

	return(A);
	*/
	//}

	/*
	old VaporPressureDeficit replaced by calculation with radiation
	be careful, in this vapor pressure, only day between 151 and 243 are used
	double CWeatherYear::GetVaporPressureDeficit()const
	{
	_ASSERTE( GetNbDay() > 243);
	//	_ASSERTE( false);//à vérifier et a enlever


	double dpv=0;

	for(int day=151; day<243; ++day)
	{
	const CWeatherDay& d = GetDay(day);

	double T1=7.5*d.GetTMax()/(237.3+d.GetTMax());
	double T2=7.5*d.GetTMin()/(237.3+d.GetTMin());
	dpv += pow(10.,T1) - pow(10.,T2);
	}

	dpv *= 6.108;

	return dpv;
	}
	*/

	/*void CWeatherYear::InitializeBeginDate(CDate& begin)const
	{
	if( begin.GetYear() == -999)
	begin = CDate(m_year, 0);
	else
	{
	if( begin.GetMonth() == MONTH_NOT_INIT )
	{
	_ASSERTE( begin.GetDay() == DAY_NOT_INIT);
	begin.SetMonth(FIRST_MONTH);
	begin.SetDay(FIRST_DAY);
	}
	else
	{
	if( begin.GetDay() == DAY_NOT_INIT)
	begin.SetDay(FIRST_DAY);
	}
	}
	}

	void CWeatherYear::InitializeEndDate(CDate& end)const
	{
	if( end.GetYear() == YEAR_NOT_INIT)
	end = CDate(m_year, LAST_DAY);
	else
	{
	if( end.GetMonth() == MONTH_NOT_INIT )
	{
	_ASSERTE( end.GetDay() == DAY_NOT_INIT);
	end.SetMonth(LAST_MONTH);
	end.SetDay(LAST_DAY);
	}
	else
	{
	if( end.GetDay() == DAY_NOT_INIT)
	end.SetDay( LAST_DAY );
	}
	}
	}
	*/

}//namespace WBSF