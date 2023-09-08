//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 24-07-2023	Rémi Saint-Amant	Reintroduce TT_NOON for FWI model
//									Remove hourly growing season.
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




	CGSInfo::CGSInfo(TDirection d, TTemperature TT, char op, double threshold, double nbDays)
	{

		m_d = d;
		m_TT = TT;
		m_op = op;
		m_threshold = threshold;
		m_nbDays = nbDays;
	}

	double CGSInfo::GetGST(const CWeatherDay& data)const
	{
		ASSERT(m_TT >= 0 && m_TT < NB_TT_TEMPERATURE);

		double T = -999;
		//if (data.IsHourly())
	//	{
		switch (m_TT)
		{
		case TT_TMIN:	T = data[H_TMIN][MEAN]; break;
		case TT_TMEAN:	T = data[H_TNTX][MEAN]; break;
		case TT_TMAX:	T = data[H_TMAX][MEAN]; break;
		case TT_TNOON:  ASSERT(data.IsHourly());  T = data[12][H_TAIR]; break;
		default: ASSERT(false);
		}
		ASSERT(T > -999);//hourly values must be computed for TT_TNOON

		return T;
	}

	double CGSInfo::GetGST(const CHourlyData& data)const
	{
		ASSERT(false); //no more used
		ASSERT(m_TT >= 0 && m_TT < NB_TT_TEMPERATURE);

		double T = -999;
		//if (data.IsHourly())
	//	{
		switch (m_TT)
		{
		case TT_TMIN:	T = data[H_TMIN]; break;
		case TT_TMEAN:	T = data[H_TAIR]; break;
		case TT_TMAX:	T = data[H_TMAX]; break;
		case TT_TNOON:  T = data[H_TAIR]; break;
		default: ASSERT(false);
		}
		ASSERT(T > -999);//hourly values must be computed for TT_TNOON

		return T;
	}

	//Get first event
	//look for the first occurrence of n successive days where TT </> threshold
	CTRef CGSInfo::GetFirst(const CWeatherYear& weather, size_t first_month, size_t last_month, int shift)const
	{
		//ASSERT(m_d < NB_DIRECTIONS);
		ASSERT(m_TT < NB_TT_TEMPERATURE);
		ASSERT(m_op == '<' || m_op == '>');
		ASSERT(first_month < 12);
		ASSERT(last_month < 12);


		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);

		p.Begin().m_month = first_month;
		p.Begin().m_day = DAY_01;
		p.End().m_month = last_month;
		p.End().m_day = CTRef::GetNbDayPerMonth(p.End().GetYear(), last_month) - 1;


		CTRef firstTRef;

		size_t nb_valid = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End() && !firstTRef.IsInit(); TRef++)
		{
			/*if (weather.IsHourly())
			{
				const CHourlyData& wData = weather.GetHour(TRef);

				bool bValid = (m_op == '>') ? GetGST(wData) > m_threshold:GetGST(wData) < m_threshold;
				if (bValid)
					nb_valid++;
				else
					nb_valid = 0;

				if (nb_valid/24.0 >= m_nbDays)
					firstTRef = TRef + shift;
			}
			else
			{*/
				const CWeatherDay& wDay = weather.GetDay(TRef);
				bool bValid = (m_op == '>') ? GetGST(wDay) > m_threshold:GetGST(wDay) < m_threshold;
				if (bValid)
					nb_valid++;
				else
					nb_valid = 0;

				if (nb_valid >= m_nbDays)
					firstTRef = TRef + shift;
			//}
		}

		return firstTRef;
	}

	//Get last event
	//look for the last occurrence of n successive days where TT </> threshold
	CTRef CGSInfo::GetLast(const CWeatherYear& weather, size_t first_month, size_t last_month, int shift)const
	{
		//ASSERT(m_d < NB_DIRECTIONS);
		ASSERT(m_TT < NB_TT_TEMPERATURE);
		ASSERT(m_op == '<' || m_op == '>');
		ASSERT(first_month < 12);
		ASSERT(last_month < 12);

		CTRef lastTRef;

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);

		p.Begin().m_month = first_month;
		p.Begin().m_day = DAY_01;

		p.End().m_month = last_month;
		p.End().m_day = CTRef::GetNbDayPerMonth(p.End().GetYear(), last_month) - 1;

		size_t nb_valid = 0;
		for (CTRef TRef = p.End(); TRef >= p.Begin() && !lastTRef.IsInit(); TRef--)
		{
			/*if (weather.IsHourly())
			{
				const CHourlyData& wData = weather.GetHour(TRef);

				bool bValid = (m_op == '>') ? GetGST(wData) > m_threshold:GetGST(wData) < m_threshold;
				if (bValid)
					nb_valid++;
				else
					nb_valid = 0;

				if (nb_valid / 24.0 >= m_nbDays)
					lastTRef = TRef + (int)Round(m_nbDays * 24-1) + shift;
			}
			else
			{*/
				const CWeatherDay& wDay = weather.GetDay(TRef);
				bool bValid = (m_op == '>') ? GetGST(wDay) > m_threshold:GetGST(wDay) < m_threshold;
				if (bValid)
					nb_valid++;
				else
					nb_valid = 0;

				if (nb_valid >= m_nbDays)
					lastTRef = TRef + (int)Round(m_nbDays-1) + shift;
			//}
		}

		return lastTRef;
	}


	//#######################################################################################

	void CEventPeriod::Execute(const CWeatherStation& weather, CModelStatVector& output)const
	{
		if( m_begin.m_TT == CGSInfo::TT_TNOON || m_end.m_TT == CGSInfo::TT_TNOON)
		{
			ASSERT(weather.IsHourly());//For TNOON option, weather must be hourly
		}



		output.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), O_GS_NB_OUTPUTS);

		for (size_t y = 0; y < weather.size(); y++)
		{
			CTPeriod p = GetPeriod(weather[y]);
			output[y][O_GS_BEGIN] = p.Begin().GetRef();
			output[y][O_GS_END] = p.End().GetRef();
			output[y][O_GS_LENGTH] = p.GetLength();
		}
	}


	CTPeriod CEventPeriod::GetPeriod(const CWeatherYear& weather)const
	{
		CTPeriod GS;
		CTPeriod p = weather.GetEntireTPeriod();


		CTRef begin;
		CTRef end;


		if (weather.GetLocation().m_lat >= 0)
		{
			int shift_b = (m_begin.m_d == CGSInfo::GET_FIRST) ? -(int(m_begin.m_nbDays) - 1) : 1;
			begin = m_begin.GetEvent(weather, JANUARY, JUNE, shift_b);
			//if (!begin.IsInit())
				//begin = p.Begin();

			int shift_e = (m_end.m_d == CGSInfo::GET_FIRST) ? -int(m_begin.m_nbDays) : 0;
			end = m_end.GetEvent(weather, JULY, DECEMBER, shift_e);
			//if (!end.IsInit())
				//end = p.End();
		}
		else
		{
			if (!weather.HavePrevious())
				return CTPeriod();//no growing season the first year of southern hemisphere

			int shift_b = (m_begin.m_d == CGSInfo::GET_FIRST) ? -(int(m_begin.m_nbDays) - 1) : 1;
			begin = m_begin.GetEvent(weather.GetPrevious(), JULY, DECEMBER, shift_b);

			int shift_e = (m_end.m_d == CGSInfo::GET_FIRST) ? -int(m_begin.m_nbDays) : 0;
			end = m_end.GetEvent(weather, JANUARY, JUNE, shift_e);
		}

		if (begin.IsInit() && end.IsInit())
			GS = CTPeriod(begin, end);

		return GS;
	}



	//#######################################################################################


	 
	const CGSInfo CGrowingSeason::DEFAULT_BEGIN = { CGSInfo::GET_LAST, CGSInfo::TT_TMIN, '<', 0, 3};
	const CGSInfo CGrowingSeason::DEFAULT_END = { CGSInfo::GET_FIRST, CGSInfo::TT_TMIN, '<', 0, 3 };

	//CTPeriod CGrowingSeason::GetGrowingSeason(const CWeatherYear& weather)const
	//{

	//	CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
	//	CTM TM = p.GetTM();
	//	int year = p.Begin().GetYear();

	//	CTRef TRef1 = CTRef(year, JULY, DAY_01);
	//	CTRef TRef2 = CTRef(year, JULY, DAY_01);



	//	if (weather.GetLocation().m_lat < 0)
	//	{
	//		if (!weather.HavePrevious())
	//			return CTPeriod();//no growing season the first year of southern hemisphere

	//		p.Begin() = CTRef(year - 1, JULY, FIRST_DAY, FIRST_HOUR, weather.GetTM());
	//		p.End() = CTRef(year, JUNE, LAST_DAY, LAST_HOUR, weather.GetTM());

	//		TRef1 = CTRef(year, JANUARY, DAY_01);
	//		TRef2 = CTRef(year, JANUARY, DAY_01);
	//	}


	//	bool bGetIt1 = false;

	//	//Beginning of the growing season
	//	//look backward for the first occurrence of 3 successive days with frost
	//	do
	//	{
	//		TRef1--;
	//		bGetIt1 = true;
	//		for (size_t dd = 0; dd < m_begin.m_nbDays && bGetIt1; dd++)
	//		{
	//			ASSERT((TRef1 - dd).Transform(p.GetTM()) >= p.Begin());

	//			const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef1 - dd]);
	//			if (wDay[H_TMIN].IsInit() && wDay[H_TMAX].IsInit())
	//				bGetIt1 = m_begin.GetGST(wDay) < m_begin.m_threshold;
	//		}
	//	} while (!bGetIt1 && (TRef1 - p.Begin()) > m_begin.m_nbDays);


	//	if (bGetIt1)
	//	{
	//		p.Begin() = TRef1 + 1;
	//		p.Transform(TM);
	//	}



	//	bool bGetIt2 = false;
	//	//End of growing season
	//	do
	//	{
	//		TRef2++;
	//		bGetIt2 = true;
	//		//look for the first occurrence of 3 successive days with frost
	//		for (size_t dd = 0; dd < m_end.m_nbDays && bGetIt2; dd++)
	//		{
	//			ASSERT((TRef2 + dd).Transform(p.GetTM()) <= p.End());

	//			const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef2 + dd]);
	//			if (wDay[H_TMIN].IsInit() && wDay[H_TMAX].IsInit())
	//				bGetIt2 = m_end.GetGST(wDay) < m_end.m_threshold;
	//		}
	//	} while (!bGetIt2 && (p.End() - TRef2) > m_end.m_nbDays);

	//	if (bGetIt2)
	//	{
	//		p.End() = TRef2 - 1;
	//		p.Transform(TM);
	//	}

	//	if (!bGetIt1 && !bGetIt2)
	//	{

	//		CTRef TRef = (weather.GetLocation().m_lat < 0) ? CTRef(year, DECEMBER, DAY_01) : CTRef(year, JULY, DAY_01);

	//		const CWeatherDay& wDay = dynamic_cast<const CWeatherDay&>(weather[TRef]);
	//		if (m_end.GetGST(wDay) < m_end.m_threshold)
	//			p.clear();//no growing season
	//		//else
	//			//p = weather.GetEntireTPeriod(CTM(CTM::DAILY)); //growing season all along the year
	//	}

	//	return p;
	//}


	//CTPeriod CGrowingSeason::GetGrowingSeason2(const CWeatherYear& weather)const
	//{
	//	CTPeriod GS;
	//	CTPeriod p = weather.GetEntireTPeriod();
	//	

	//	CTRef begin;
	//	CTRef end;


	//	if (weather.GetLocation().m_lat >= 0)
	//	{
	//		int shift_b = (m_begin.m_d == CGSInfo::GET_FIRST)? -(int(m_begin.m_nbDays) -1): 1;
	//		begin = m_begin.GetEvent(weather, JANUARY, JUNE, shift_b);
	//		//if (!begin.IsInit())
	//			//begin = p.Begin();

	//		int shift_e = (m_end.m_d == CGSInfo::GET_FIRST) ? -int(m_begin.m_nbDays) : 0;
	//		end = m_end.GetEvent(weather, JULY, DECEMBER, shift_e);
	//		//if (!end.IsInit())
	//			//end = p.End();
	//	}
	//	else
	//	{
	//		if (!weather.HavePrevious())
	//			return CTPeriod();//no growing season the first year of southern hemisphere

	//		int shift_b = (m_begin.m_d == CGSInfo::GET_FIRST) ? -(int(m_begin.m_nbDays) - 1) : 1;
	//		begin = m_begin.GetEvent(weather.GetPrevious(), JULY, DECEMBER, shift_b);

	//		int shift_e = (m_end.m_d == CGSInfo::GET_FIRST) ? -int(m_begin.m_nbDays) : 0;
	//		end = m_end.GetEvent(weather, JANUARY, JUNE, shift_e);
	//	}

	//	if (begin.IsInit() && end.IsInit())
	//		GS = CTPeriod(begin, end);

	//	return GS;
	//}




	//#######################################################################################
	const CGSInfo CFrostFreePeriod::DEFAULT_BEGIN = { CGSInfo::GET_LAST, CGSInfo::TT_TMIN, '<', 0, 1};
	const CGSInfo CFrostFreePeriod::DEFAULT_END = { CGSInfo::GET_FIRST, CGSInfo::TT_TMIN, '<', 0, 1 };


	//FrostFree period: get the longest frost free period of the year
	//CTPeriod CFrostFreePeriod::GetFrostFreePeriod(const CWeatherYear& weather)const
	//{
	//	CTPeriod period = weather.GetEntireTPeriod();
	//	CTPeriod pTmp = period;
	//	CTPeriod p;
	//	bool notInit = true;


	//	for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
	//	{
	//		double T = weather.IsHourly() ? weather[TRef][H_TAIR][MEAN] : weather[TRef][H_TMIN][MEAN];

	//		if (T > 0) //Frost-free period begin or continues
	//		{
	//			if (notInit)
	//			{
	//				pTmp.Begin() = TRef;
	//				notInit = false;
	//			}
	//		}
	//		else
	//		{
	//			if (!notInit)
	//			{
	//				pTmp.End() = TRef - 1;
	//				notInit = true;

	//				//Frost-free period ends
	//				if (pTmp.GetLength() > p.GetLength())
	//					p = pTmp;
	//			}
	//		}

	//		if (TRef == pTmp.End() && !notInit)
	//		{
	//			pTmp.End() = TRef;
	//			if (pTmp.GetLength() > p.GetLength())
	//				p = pTmp;
	//		}
	//	}

	//	return p;
	//}


	//CTPeriod CFrostFreePeriod::GetFrostFreePeriod2(const CWeatherYear& weather)const
	//{
	//	CTPeriod GS;
	//	CTPeriod p = weather.GetEntireTPeriod();
	//	//CTM TM = p.GetTM();
	//	//int year = p.Begin().GetYear();

	//	CTRef begin;
	//	CTRef end;


	//	if (weather.GetLocation().m_lat >= 0)
	//	{
	//		begin = m_begin.GetLast(weather, JANUARY, JUNE, 1);
	//		if (!begin.IsInit())
	//			begin = p.Begin();

	//		end = m_end.GetFirst(weather, JULY, DECEMBER, -1);
	//		if (!end.IsInit())
	//			end = p.End();
	//	}
	//	else
	//	{
	//		if (!weather.HavePrevious())
	//			return CTPeriod();//no growing season the first year of southern hemisphere

	//		begin = m_begin.GetLast(weather.GetPrevious(), JULY, DECEMBER, 1);
	//		end = m_end.GetFirst(weather, JANUARY, JUNE, -1);
	//	}

	//	if (begin.IsInit() && end.IsInit())
	//		GS = CTPeriod(begin, end);

	//	return GS;
	//}
	//	CTRef CGSInfo::GetLast(CWeatherYear& weather, char m_op, bool bJulyDecember)const
	//	{
	//		CTRef lastTRef;
	//
	//		CTPeriod p = weather.GetEntireTPeriod();
	//		if (bJulyDecember)
	//		{
	//			p.Begin().m_month = JULY;
	//			p.Begin().m_day = DAY_01;
	//		}
	//		else
	//		{
	//			p.End().m_month = JUNE;
	//			p.End().m_day = DAY_30;
	//		}
	//
	//
	//
	//		//CTPeriod p = m_period;
	////		p.Begin().m_year = pp.Begin().GetYear();
	//	//	p.End().m_year = pp.End().GetYear();
	//
	//
	//		//Beginning of fire weather 
	//		//look for the first occurrence of 3 successive days Tnoon > T_THRESHOLD
	//		CTRef firstTRef = p.Begin() + (int)m_nbDays;
	//		bool bGetIt = false;
	//		for (CTRef TRef = p.End(); TRef >= firstTRef && !bGetIt; TRef--)
	//		{
	//			bGetIt = true;
	//			for (size_t dd = 0; dd<m_nbDays&&bGetIt; dd--)
	//			{
	//				const CWeatherDay& wDay = weather.GetDay(TRef + dd);
	//				if (m_op == '>')
	//					bGetIt = bGetIt && GetGST(wDay) > m_threshold;
	//				else
	//					bGetIt = bGetIt && GetGST(wDay) < m_threshold;
	//			}
	//
	//
	//			if (bGetIt)
	//			{
	//				lastTRef = TRef - (int)m_nbDays;//end before these 3 days
	//			}
	//		}
	//
	//		return lastTRef;
	//	}


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