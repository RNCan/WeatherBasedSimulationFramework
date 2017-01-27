//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 13-09-2016	Rémi Saint-Amant	Change Tair and Trng by Tmin and Tmax
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 18-11-2015	Rémi Saint-Amant	Change in the daily accumulation
// 08-12-2014	Rémi Saint-Amant	Integration of CWeather into CWeatherStation
// 08-04-2013	Rémi Saint-Amant	Initial version from old code
//****************************************************************************


//todo: add the time zone to the computation of hourly value. a delta zone must compoute between 
//the time zone of the simulation point and the time zone of the weather station

#include "stdafx.h"
#include <locale>
#include <mutex>

#include "Basic/WeatherStation.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/WeatherCorrection.h"

#include "WeatherBasedSimulationString.h"
#include "OpenMP.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::GRADIENT;



static std::mutex STATISTIC_MUTEX;



namespace WBSF
{
	

	//from : http://agsys.cra-cin.it/tools/evapotranspiration/help/Soil_heat_flux.html
	//The soil heat flux, G, is the energy that is utilized in heating the soil. G is positive when the soil is warming and negative when the soil is cooling. 
	// LAI [in]:	crop leaf area index [m2 m-2]
	// G [out]:		soil heat flux energy [MJ m-2 hr-1]
	double G(double Rn, double LAI)
	{
		
		//G beneath a dense cover of grass does not correlate well with air temperature. 
		//where Rn hourly (MJ m-2 h-1) net radiation, KG=0.4 during daytime (Rn>0), KG=1.6-2.0 during nighttime (Rn<0)

		double Ghr = 0;
		if (LAI >= 0)
		{
			double Kg = (Rn > 0) ? 0.4 : 1.8;
			Ghr = Kg*Rn*exp(-0.5*LAI);
		}
		else
		{
			//Coefficients cd and cn are daytime and nighttime factors, 
			//respectively (cd=0.1, cn=0.5 for short grass; cd=0.04, cn=0.2 for tall grass).
			//We only use short grass here
			static const double Cd = 0.1;
			static const double Cn = 0.5;
			double C = (Rn > 0) ? Cd : Cn;
			Ghr = C*Rn;
		}

		return Ghr;
	}


CStatistic GetDailyStat(size_t v, CWeatherDay& weather)
{
	ASSERT(weather.IsHourly());
	
	CWeatherAccumulator accumulator(CTM(CTM::DAILY, CTM::FOR_EACH_YEAR));
	if (weather.GetParent() != NULL)
	{
		//Compute noon-noon from the previous and current day
		const CWeatherDay& previousDay = weather.GetPrevious();

		for (size_t h = 12; h<24; h++)
			accumulator.Add(previousDay[h].GetTRef(), v, previousDay[h][v]);

	}

	for (size_t h = 0; h<24; h++)
		accumulator.Add(weather[h].GetTRef(), v, weather[h][v]);

	return accumulator[v];
}

//**************************************************************************************************************
//COverheat

double COverheat::GetTmin(const CWeatherDay& weather)const
{
	return weather[H_TMIN2][MEAN];
}

double COverheat::GetTmax(const CWeatherDay& weather)const
{
	if (m_overheat == 0)//by optimisation, avoid compute statistic
		return weather[H_TMAX2][MEAN];

	return weather[H_TMAX2][MEAN] + weather[H_TRNG2][MEAN] * m_overheat;
}

double COverheat::GetOverheat(const CWeatherDay& weather, size_t h, size_t hourTmax)const
{ 
	double OH = 0;
	if (m_overheat!= 0 && weather[H_TMIN2].IsInit() && weather[H_TMAX2].IsInit())
	{
		double Fo = 0.5*(1 + cos((double(hourTmax) - h) / 12.0*PI));
		double maxOverheat = weather[H_TRNG2][MEAN] * m_overheat;
		OH = maxOverheat* Fo;
	}

	return OH;
}

double COverheat::GetT(const CWeatherDay& weather, size_t h, size_t hourTmax)const
{
	double T = -999;
	if (weather[H_TMIN2].IsInit() && weather[H_TMAX2].IsInit())
	{
		const CWeatherDay& d1 = weather.GetPrevious();
		const CWeatherDay& d2 = weather;
		const CWeatherDay& d3 = weather.GetNext();
		T = WBSF::GetAllenT(GetTmin(d1), GetTmax(d1), GetTmin(d2), GetTmax(d2), GetTmin(d3), GetTmax(d3), h, hourTmax);
	}

	return T;
}
//**************************************************************************************************************
//CWeatherAccumulator

const CStatistic CWeatherAccumulator::EMPTY_STAT;
CWeatherAccumulator::CWeatherAccumulator(const CTM& TM)
{
	m_TM = TM;

	m_deltaHourMin = 20;
	m_minimumHours.fill(6);
	m_minimumHours[HOURLY_DATA::H_PRCP] = 20;
	m_minimumHours[HOURLY_DATA::H_SRAD2] = 20;
	m_minimumHours[HOURLY_DATA::H_SNDH] = 1;
	m_minimumHours[HOURLY_DATA::H_SWE] = 1;
	m_minimumDays.fill(22);

	m_noonVariablesTmp.fill(CStatistic());
	for (size_t h = 0; h<m_noonTRefMatrixTmp.size(); h++)
		m_noonTRefMatrixTmp[h].fill(0);

	ResetStat();
	ResetMidnight();
	ResetNoon();
}
ERMsg CWeatherAccumulator::Add(const StringVector& data, const CWeatherFormat& format)
{
	ERMsg msg;


	CTRef Tref = format.GetTRef(data);

	if (!Tref.IsInit())
	{
		msg.ajoute(GetString(IDS_BSC_INVALID_TIME_REF));
		return msg;
	}

	//CStatistic Tair;
	
	for (size_t i = 0; i<(int)format.size() && i<data.size(); i++)
	{
		if (IsVariable(format[i].m_var))
		{
			if (!data[i].empty())
			{
				double value = ToDouble(data[i]);

				CStatistic stat;
				if (value > format.GetNoData())
					stat += value;
				
				//if (format[i].m_var == H_TAIR2 && format[i].m_stat == LOWEST)
				//{
				//	Tair += stat;
				//	//Add(Tref, H_TMIN, stat);//always add stat to reset day
				//}
				//else if (format[i].m_var == H_TAIR2 && format[i].m_stat == HIGHEST)
				//{
				//	Tair += stat;
				//	//Add(Tref, H_TMAX, stat);//always add stat to reset day
				//}
				//else
				//{
				Add(Tref, format[i].m_var, stat);//always add stat to reset day
				//}
			}//element is an non empty variable
		}
	}//for all element

	/*if (Tair.IsInit()  )
	{
		Add(Tref, H_TAIR, Tair[MEAN]);
		Add(Tref, H_TRNG, Tair[RANGE]);
	}
	else
	{
		Add(Tref, H_TAIR, Tair);
		Add(Tref, H_TRNG, Tair);
	}
*/
	return msg;
}

void CWeatherAccumulator::Add(CTRef Tref, size_t v, const CStatistic& value)
{
	assert(Tref.GetTM().Type() == CTM::HOURLY || Tref.GetTM().Type() == CTM::DAILY);
	
	if (Tref.GetTM().Type() == CTM::HOURLY)
	{
		if (m_bStatComputed)
			ResetStat();

		if (TRefIsChanging(Tref))//, MIDNIGHT_MIDNIGHT))
			ResetMidnight();

		if (TRefIsChanging(Tref,12))//, NOON_NOON))
			ResetNoon();

		//if (v == H_TMIN || v == H_TMAX)
			//v = H_TAIR;

		m_midnightTRefMatrix[Tref.GetHour()][v] += (int)value[NB_VALUE];
		m_midnightVariables.m_bInit = true;
		m_midnightVariables[v] += value;
		m_midnightVariables.m_period += Tref;


		
		m_noonTRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
		m_noonVariablesTmp.m_bInit = true;
		m_noonVariablesTmp[v] += value;
		m_noonVariablesTmp.m_period += Tref;
		m_lastTRef = Tref;
	}
	else if (Tref.GetTM().Type() == CTM::DAILY)
	{
		ASSERT(Tref.GetTM().Type() == CTM::DAILY);

		if (TRefIsChanging(Tref))
		{
			ResetStat();
			ResetMidnight();
			ResetNoon();
		}

		//if (v == H_TMIN)
		//{
		//	assert(false);// a faire
		//	m_noonVariablesTmp.m_bInit = true;
		//	m_noonVariablesTmp[H_TAIR] += value;
		//	m_noonVariablesTmp.m_period += Tref;
		//}
		//else if (v == H_TMAX)
		//{
		//	assert(false);// a faire
		//	m_midnightVariables.m_bInit = true;
		//	m_midnightVariables[H_TAIR] += value;
		//	m_midnightVariables.m_period += Tref;
		//}
		//else
		//{
		m_variables.m_bInit = true;
		m_variables[v] += value;
		m_variables.m_period += Tref;
		m_lastTRef = Tref;
		//}
	}
}

const CStatistic& CWeatherAccumulator::GetStat(size_t v)const
{
	if (m_TM.Type() != CTM::HOURLY)
	{
		ComputeStatistic();
		return m_variables[v];
	}


	return m_midnightVariables[v];
}

void CWeatherAccumulator::ComputeStatistic()const
{
	assert(m_TM.Type() != CTM::HOURLY);

	if (!m_bStatComputed)
	{
		CWeatherAccumulator& me = const_cast<CWeatherAccumulator&>(*this);

		if (m_TM.Type() == CTM::DAILY && GTRef().GetTM().Type() == CTM::HOURLY)
		{
			for (size_t v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				me.m_variables[v].clear();//reset var
					
				//continue;
				CStatistic stat = (v == H_TMIN2) ? GetStat(v, NOON_NOON) : GetStat(v, MIDNIGHT_MIDNIGHT);
				if (stat.IsInit())
				{
					switch (v)
					{
					case H_TMIN2:	me.m_variables[v] = stat[LOWEST]; break;
					case H_TMAX2:	me.m_variables[v] = stat[HIGHEST]; break;
					case H_PRCP:	me.m_variables[v] = stat[SUM]; break;
					default:		me.m_variables[v] = stat[MEAN];
					}
					
				}
					
				
			}
				
			if (!me.m_variables[H_TMIN2].IsInit() || !me.m_variables[H_TMAX2].IsInit())
			{
				me.m_variables[H_TMIN2].clear();//reset var
				me.m_variables[H_TMAX2].clear();//reset var

				CStatistic midnight = GetStat(H_TAIR2, MIDNIGHT_MIDNIGHT);//compute midnight-midnight statistics
				CStatistic noon = GetStat(H_TAIR2, NOON_NOON);//compute noon-noon statistics
				if (midnight.IsInit() && noon.IsInit())
				{
					me.m_variables[H_TMIN2] = noon[LOWEST];
					me.m_variables[H_TMAX2] = midnight[HIGHEST];
				}
			}
		}//for all variable	
			//else if (GTRef().GetTM().Type() == CTM::DAILY)
			//{
				//if (m_variables[H_TAIR][NB_VALUE] >= 1 && m_variables[H_TRNG].IsInit())
				//{
				//	assert(m_variables[H_TAIR][NB_VALUE] == m_variables[H_TRNG][NB_VALUE]);
				//	double Tmean = m_variables[H_TAIR][MEAN];
				//	double Trange = m_variables[H_TRNG][MEAN];

				//	me.m_variables[H_TAIR] = Tmean;
				//	me.m_variables[H_TRNG] = Trange;
				//}
				//else if (m_variables[H_TAIR][NB_VALUE] >= 2 && !m_variables[H_TRNG].IsInit())
				//{
				//	//Tair from Tmin and Tmax : reset Tair and int Trng
				//	double Tmean = m_variables[H_TAIR][MEAN];
				//	double Trange = m_variables[H_TAIR][RANGE];

				//	me.m_variables[H_TAIR] = Tmean;
				//	me.m_variables[H_TRNG] = Trange;
				//}
				//else if (m_variables[H_TAIR].IsInit())
				//{
				//	me.m_variables[H_TAIR].clear();
				//	me.m_variables[H_TRNG].clear();
				//}

			//}
		//}
		
		me.m_variables.m_bInit = true;
		me.m_bStatComputed = true;
	}

}
const CStatistic& CWeatherAccumulator::GetStat(size_t v, int sourcePeriod)const
{
	assert(m_TM.Type() == CTM::DAILY && GTRef().GetTM().Type() == CTM::HOURLY);

	bool bValid = true;
	//if ( v == H_TMIN2 || v == H_TMAX2 )
		//v = H_TAIR2;//daily diurne range is base on hourly temperature

	if (sourcePeriod == MIDNIGHT_MIDNIGHT)
	{
		ASSERT(m_midnightTRefMatrix.size() == 24);
		CStatistic NbTRef;
		for (size_t h = 0; h<m_midnightTRefMatrix.size(); h++)
			if (m_midnightTRefMatrix[h][v]>0)
				NbTRef += (int)h;

		if (/*v == H_TAIR2 ||*/ v == H_TMAX2)
		{
			bValid = false;
			for (size_t h = 14; h <= 16 && !bValid; h++)
				if (m_midnightTRefMatrix[h][v]>0)
					bValid = true;

			if (!bValid)
			{
				if (m_noonVariables[H_TAIR2][NB_VALUE] >= m_minimumHours[v])
				{
					for (size_t h = 3; h <= 6 && !bValid; h++)
						if (m_noonTRefMatrix[h][H_TAIR2] > 0)
							bValid = true;

					if (bValid)
						v = H_TAIR2; //use Tair stat instead of Tmax
				}
			}
		}
		//else if( v==HOURLY_DATA::H_WNDD )
		//{a revoir...
		//}
		else
		{
			if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
				m_midnightVariables[v][NB_VALUE] < m_minimumHours[v])
			{
				bValid = false;
			}
		}

		if (bValid )//|| (GTRef().GetTM().Type() != CTM::HOURLY)
			return m_midnightVariables[v];
	}
	else
	{
		ASSERT(m_noonTRefMatrix.size() == 24);

		CStatistic NbTRef;
		for (size_t h = 0; h<m_noonTRefMatrix.size(); h++)
			if (m_noonTRefMatrix[h][v]>0)
				NbTRef += (int)h;


		if (/*v == H_TAIR2 || */v == H_TMIN2)
		{
			bValid = false;
			if (m_noonVariables[v][NB_VALUE] >= m_minimumHours[v])
			{
				for (size_t h = 3; h <= 6 && !bValid; h++)
					if (m_noonTRefMatrix[h][v]>0)
						bValid = true;
			}


			//try with Tair 
			if (!bValid)
			{
				if (m_noonVariables[H_TAIR2][NB_VALUE] >= m_minimumHours[v])
				{
					for (size_t h = 3; h <= 6 && !bValid; h++)
						if (m_noonTRefMatrix[h][H_TAIR2] > 0)
							bValid = true;

					if (bValid)
						v = H_TAIR2; //use Tair stat instead of Tmin
				}
			}
		}
		else
		{
			if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
				m_noonVariables[v][NB_VALUE] < m_minimumHours[v])
			{
				bValid = false;
			}
		}

		if (bValid)
			return m_noonVariables[v];
	}


	return EMPTY_STAT;
}
//**************************************************************************************************************
//CDataInterface
void CDataInterface::SetData(const CWeatherAccumulator& weatherStat)
{
	for (TVarH v = H_FIRST_VAR; v<NB_VAR_H; v++)
		SetStat(v, weatherStat.GetStat(v));
}

//***********************************************************************************************************


CHourlyData& CHourlyData::operator=(const CHourlyData& in)
{
	if( &in!=this)
	{
		CWeatherVariables::operator=(in);
		
		if( m_pParent== NULL )
			m_TRef = in.m_TRef;
	}

	return *this;
}


const CStatistic& CHourlyData::GetData(HOURLY_DATA::TVarH v)const
{
	static CStatistic STATISTIC_TMP[256];

	STATISTIC_TMP[omp_get_thread_num() % 256].Reset();
	if (at(v)>WEATHER::MISSING)
		STATISTIC_TMP[omp_get_thread_num() % 256] = CWeatherVariables::at(v);

	return STATISTIC_TMP[omp_get_thread_num() % 256];
}

CStatistic& CHourlyData::GetData(HOURLY_DATA::TVarH v)
{
	static CStatistic STATISTIC_TMP[256];
	//assert(false);//not thread safe

	STATISTIC_TMP[omp_get_thread_num()%256].Reset();
	
	if (!IsMissing(at(v)))
		STATISTIC_TMP[omp_get_thread_num() % 256] = CWeatherVariables::at(v);

	return STATISTIC_TMP[omp_get_thread_num() % 256];
}

bool CHourlyData::GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const
{ 
	if (!IsMissing(at(v)))
		stat = at(v);
	return stat.IsInit(); 
}

void CHourlyData::SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat)
{
	if( stat.IsInit() )
	{
		switch(v)
		{
		case H_SNOW:
		case H_PRCP:	at(v) = (float)stat[SUM]; break;
		default:		at(v) = (float)stat[MEAN];
		}
	}
	else
	{
		at(v) = WEATHER::MISSING;
	}
}


double CHourlyData::GetExtraterrestrialRadiation()const
{
	const CLocation& loc = GetLocation();
	return CASCE_ETsz::GetExtraterrestrialRadiationH(GetTRef(), loc.m_lat, loc.m_lon, loc.m_alt);
}


//Fcd[In]	: Fcd of previous time step
//Fcd[In]	: Fcd of current time step	
double CHourlyData::GetNetRadiation(double& Fcd)const
{
	const CLocation& loc = GetLocation();
	double Ra = CASCE_ETsz::GetExtraterrestrialRadiationH(GetTRef(), loc.m_lat, loc.m_lon, loc.m_alt);
	if (Ra >= 0)//if daytime update Fcd
	{
		double Rs = at(H_SRMJ);
		double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, loc.m_alt);
		Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);
	}
	
	//compute new Fcd
	double Rnl = CASCE_ETsz::GetNetLongWaveRadiationH(at(H_TAIR2), at(H_EA2)/1000, Fcd);
	double Rns = CASCE_ETsz::GetNetShortWaveRadiation(at(H_SRMJ));

	return  CASCE_ETsz::GetNetRadiation(Rns, Rnl);// hourly incoming radiation [MJ/(m²·h)]
}


//l[out]: latent heat of vaporization of water[MJ kg - 1]
double CHourlyData::GetLatentHeatOfVaporization()const
{
	return 2.5023 - 0.00243054 * at(H_TAIR2);
}

CStatistic CHourlyData::GetVarEx(HOURLY_DATA::TVarEx v)const
{
	const CHourlyData& me = *this;
	const CLocation& loc = GetLocation();
	CTRef TRef = GetTRef();

	CStatistic stat;
	switch (v)
	{
	//case H_TMIN:	stat = !WEATHER::IsMissing(at(H_TAIR)) && !WEATHER::IsMissing(at(H_TRNG)) ? at(H_TAIR) - at(H_TRNG) / 2 : !WEATHER::IsMissing(at(H_TAIR))?at(H_TAIR): WEATHER::MISSING; break;
	//case H_TMAX:	stat = !WEATHER::IsMissing(at(H_TAIR)) && !WEATHER::IsMissing(at(H_TRNG)) ? at(H_TAIR) + at(H_TRNG) / 2 : !WEATHER::IsMissing(at(H_TAIR))?at(H_TAIR): WEATHER::MISSING; break;
	case H_KELV:	stat = K(); break;	//temperature in kelvin
	case H_PSYC:	stat = !WEATHER::IsMissing(at(H_PRES)) ? CASCE_ETsz::GetPsychrometricConstant(at(H_PRES) / 10) : WEATHER::MISSING; break;
	case H_SSVP:	stat = !WEATHER::IsMissing(at(H_TAIR2)) ? CASCE_ETsz::GetSlopeOfSaturationVaporPressure(at(H_TAIR2)) : WEATHER::MISSING; break;
	case H_LHVW:	stat = GetLatentHeatOfVaporization(); break;	// latent heat of vaporization of water [MJ kg-1]
	case H_FNCD:	stat = !WEATHER::IsMissing(at(H_SRAD2)) ? CASCE_ETsz::GetCloudinessFunction(me[H_SRMJ], me[H_CSRA]) : WEATHER::MISSING; break;
	case H_CSRA:	stat = CASCE_ETsz::GetClearSkySolarRadiation(GetExtraterrestrialRadiation(), loc.m_alt); break;
	case H_EXRA:	stat = GetExtraterrestrialRadiation(); break;
	case H_SWRA:	stat = !WEATHER::IsMissing(at(H_SRAD2)) ? CASCE_ETsz::GetNetShortWaveRadiation(me[H_SRMJ]) : WEATHER::MISSING; break;
	case H_ES2:		stat = !WEATHER::IsMissing(at(H_TAIR2)) ? e°(at(H_TAIR2)) : WEATHER::MISSING; break;
	case H_EA2:		stat = !WEATHER::IsMissing(at(H_TDEW)) ? e°(at(H_TDEW)) : WEATHER::MISSING; break;
	case H_VPD2:	stat = !WEATHER::IsMissing(at(H_TAIR2)) && !WEATHER::IsMissing(at(H_TDEW)) ? max(0.0, e°(at(H_TAIR2)) - e°(at(H_TDEW))) : WEATHER::MISSING; break;
	case H_TNTX:	stat = !WEATHER::IsMissing(at(H_TAIR2)) ? at(H_TAIR2) : WEATHER::MISSING; break; //!WEATHER::IsMissing(at(H_TMIN2)) && !WEATHER::IsMissing(at(H_TMAX2)) ? (at(H_TMIN2) + at(H_TMAX2)) / 2 : WEATHER::MISSING; break;
	case H_TRNG2:	stat = !WEATHER::IsMissing(at(H_TAIR2)) ? 0 : WEATHER::MISSING; break;
	case H_SRMJ:	stat = !WEATHER::IsMissing(at(H_SRAD2)) ? at(H_SRAD2)*3600.0/1000000 : WEATHER::MISSING; break;
	default:ASSERT(false);
	}


	return stat;
}



CDailyWaveVector& CHourlyData::GetAllenWave(CDailyWaveVector& t, size_t hourTmax, size_t step, const COverheat& overheat) const
{
	assert(false);
	//don't call GetAllenWave on hourly data
	return t;
}

void CHourlyData::WriteStream(ostream& stream, const CWVariables& variable)const
{
	const CHourlyData& me = *this;
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			write_value(stream, me[v]);
}

void CHourlyData::ReadStream(istream& stream, const CWVariables& variable)
{
	const CHourlyData& me = *this;
	for (size_t v=0; v<variable.size(); v++)
		if (variable[v])
			read_value(stream, me[v]);
}


//****************************************************************************************************************
CWeatherDay& CWeatherDay::operator=(const CWeatherDay& in)
{
	if( &in!=this)
	{
		if (in.m_pHourlyData.get())
		{
			assert(IsHourly());
			ManageHourlyData();
			*m_pHourlyData = *in.m_pHourlyData;
		}

		m_dailyStat = in.m_dailyStat;
	}

	return *this;
}

bool CWeatherDay::operator==(const CWeatherDay& in)const
{
	bool bEqual = true;// IsHourly() == in.IsHourly();
	if (IsHourly() && in.IsHourly())
	{
		if (m_pHourlyData.get() && in.m_pHourlyData.get())
			if (*m_pHourlyData != *in.m_pHourlyData)bEqual = false;
	}
	else
	{
		if (m_dailyStat != in.m_dailyStat)bEqual = false;
	}

	
	return bEqual;
}

bool CWeatherDay::GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const
{
	if (HourlyDataExist())
		CompileDailyStat();

	stat = m_dailyStat[v];
	return stat.IsInit();
}


void CWeatherDay::SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat)
{ 
	m_dailyStat[v] = stat;
	m_dailyStat.m_bInit = false;
}


//does we have to elimininate not enaugh data here ????
void CWeatherDay::CompileDailyStat(bool bFoceCompile)const
{
	//STATISTIC_MUTEX.lock();
	if (!m_dailyStat.m_bInit || bFoceCompile)
	{
		CWeatherDay& me = const_cast<CWeatherDay&>(*this);

		if (IsHourly())
		{
			bool bHourlyComputed = GetWeatherStation()->IsHourlyComputed();
			bool bIsCompilingHourly = GetWeatherStation()->IsCompilingHourly();
			if (!bHourlyComputed && !bIsCompilingHourly)
			{


				CWeatherAccumulator accumulator(CTM(CTM::DAILY, CTM::FOR_EACH_YEAR));
				accumulator.m_minimumHours.fill(0);
				accumulator.m_minimumDays.fill(0);

				if( m_pParent != NULL)
				{
					//Compute noon-noon from the previous and current day
					const CWeatherDay& previousDay = me.GetPrevious();
					for (size_t h = 12; h<24; h++)
						accumulator.Add(previousDay[h].GetTRef(), H_TAIR2, previousDay[h][H_TAIR2]);
				}

				me.m_dailyStat.clear();
				for (size_t h = 0; h < 24; h++)
				{
					if (me[h].HaveData())
					{
						if (IsMissing(me[h][H_RELH]) && !IsMissing(me[h][H_TAIR2]) && !IsMissing(me[h][H_TDEW]))
							me[h][H_RELH] = (float)Td2Hr(me[h][H_TAIR2], me[h][H_TDEW]);

						if (IsMissing(me[h][H_TDEW]) && !IsMissing(me[h][H_TAIR2]) && !IsMissing(me[h][H_RELH]))
							me[h][H_TDEW] = (float)Hr2Td(me[h][H_TAIR2], me[h][H_RELH]);

						for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
						{
							if (!IsMissing(me[h][v]))
							{
								accumulator.Add(me[h].GetTRef(), v, me[h][v]);

								if (v != H_TMIN2 && v != H_TMAX2)
									me.m_dailyStat[v] += me[h][v];
							}
						}

						me.m_dailyStat.m_period += me[h].GetTRef();
					}
				}

				ASSERT(!m_dailyStat[H_TAIR2].IsInit() || (m_dailyStat[H_TAIR2][MEAN] >= -90 && m_dailyStat[H_TAIR2][MEAN] < 90));
				ASSERT(!m_dailyStat[H_TMIN2].IsInit() || (m_dailyStat[H_TMIN2][MEAN] >= -90 && m_dailyStat[H_TMIN2][MEAN] < 90));
				ASSERT(!m_dailyStat[H_TMAX2].IsInit() || (m_dailyStat[H_TMAX2][MEAN] >= -90 && m_dailyStat[H_TMAX2][MEAN] < 90));

				CStatistic Tmin = accumulator.GetStat(H_TMIN2);
				CStatistic Tmax = accumulator.GetStat(H_TMAX2);
				if (Tmin.IsInit() && Tmax.IsInit())
				{
					me.m_dailyStat[H_TMIN2] = Tmin;
					me.m_dailyStat[H_TMAX2] = Tmax;
				}
				//if (m_dailyStat[H_TAIR2].IsInit())
					//me.m_dailyStat[H_TRNG] = m_dailyStat[H_TAIR][RANGE];
			}

			
		}
		else
		{
			me.m_dailyStat.m_period = CTPeriod(m_TRef, m_TRef);
		}


		me.m_dailyStat.m_bInit = true;
	}

	//STATISTIC_MUTEX.unlock();
}


static double GetTdaylightEstimate(double Tmax, double Tmean)
{
	_ASSERTE(!IsMissing(Tmax) && !IsMissing(Tmean));

	static const double TDAYCOEF = 0.45;  // (dim) daylight air temperature coefficient (dim) 
	return ((Tmax - Tmean)*TDAYCOEF) + Tmean;
}

double CWeatherDay::GetTdaylight()const
{
	const CWeatherDay& me = (const CWeatherDay&)(*this);
	CStatistic Tdaylight;
	if (IsHourly())
	{
		const CLocation& loc = GetLocation();
		CSun sun(loc.m_lat, loc.m_lon);
		size_t sunrise = Round(sun.GetSunrise(GetTRef()));
		size_t sunset = min(23ll, Round(sun.GetSunset(GetTRef())));
		
		for(size_t h=sunrise; h<=sunset; h++)
			Tdaylight+=me[h][H_TAIR2];
	}
	else
	{
		Tdaylight = GetTdaylightEstimate(me[H_TMAX2][MEAN], me[H_TAIR2][MEAN]);
	}


	return Tdaylight[MEAN];
}
//
//double CWeatherDay::GetCloudiness()const
//{
//	const CWeatherDay& me = *this;
//
//	//Julian day, in one base
//	int J = int(GetTRef().GetJDay()+1);
//
//	double Rs = max(0.0, me[H_SRAD][SUM]);
//	double Ra = CASCE_ETsz::GetExtraterrestrialRadiation(lat, J);
//	double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, z);
//	double Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);
//
//	return Fcd;
//}
//
//double CWeatherDay::GetNetLongWaveRadiation(double Fcd)const
//{
//	const CWeatherDay& me = *this;
//
//	double Rnl = 0;
//	if (IsHourly())
//	{
//		ASSERT(false);
//		//for (size_t h = 0; h < 24; h++)
//		//{ 
//		//	//input Fcd is not use here
//		//	double Fcd = me[h].GetCloudiness();
//		//	Rnl += me[h].GetNetLongWaveRadiation(Fcd);
//		//}
//	}
//	else
//	{
//		//double Fcd = GetCloudiness();
//		double Tmin = me[H_TAIR][LOWEST];
//		double Tmax = me[H_TAIR][HIGHEST];
//		double Ea = me[H_EA][MEAN];
//		Rnl = CASCE_ETsz::GetNetLongWaveRadiation(Tmin, Tmax, Ea, Fcd);
//
//	}
//
//	return Rnl;
//}
//
//double CWeatherDay::GetNetShortWaveRadiation()const
//{
//	const CWeatherDay& me = *this;
//
//	double Rns = 0;
//	if (IsHourly())
//	{
//		for (size_t h = 0; h < 24; h++)
//			Rns += me[h].GetNetShortWaveRadiation();
//	}
//	else
//	{
//		double Rs = me[H_SRAD][MEAN];
//		Rns = CASCE_ETsz::GetNetShortWaveRadiation(Rs);
//	}
//
//	return Rns;
//}

double CWeatherDay::GetNetRadiation(double& Fcd)const
{
	const CWeatherDay& me = *this;

	double Rn = 0;
	if (IsHourly())
	{
		for (size_t h = 0; h < 24; h++)
		{
			Rn += me[h].GetNetRadiation(Fcd);
		}
	}
	else
	{
		const CLocation& loc = GetLocation();
		double Tmin = me[H_TMIN2][MEAN];
		double Tmax = me[H_TMAX2][MEAN];
		double Ea = me[H_EA2][MEAN] / 1000;	//vapor pressure [kPa]
		double Rs = me[H_SRMJ][SUM];		//net radiation in MJ/m²
		int J = int(GetTRef().GetJDay() + 1);

		double Ra = CASCE_ETsz::GetExtraterrestrialRadiation(loc.m_lat, J);
		double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, loc.m_alt);
		Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);//compute new Fcd
		
		double Rnl = CASCE_ETsz::GetNetLongWaveRadiation(Tmin, Tmax, Ea, Fcd);
		double Rns = CASCE_ETsz::GetNetShortWaveRadiation(Rs);
		Rn = CASCE_ETsz::GetNetRadiation(Rns, Rnl);// daily incoming radiation [MJ/(m²·h)]
	}

	return Rn;
}



CStatistic CWeatherDay::GetVarEx(HOURLY_DATA::TVarEx v)const
{
	
	ASSERT(m_pParent);
	const CWeatherDay& me = *this;
	const CLocation& loc = GetLocation();
	CTRef TRef = GetTRef();

	CStatistic stat;

	
	if (v == H_TNTX || v == H_TRNG2)
	{
		//ASSERT(m_dailyStat.m_bInit);
		const CStatistic& Tmin = GetStat(H_TMIN2);
		const CStatistic& Tmax = GetStat(H_TMAX2);
		
		if (Tmin.IsInit() && Tmax.IsInit())
		{
			ASSERT(Tmax[MEAN] >= Tmin[MEAN]);

			if (v == H_TNTX)
				stat = (Tmax[MEAN] + Tmin[MEAN])/2;
			else if (v == H_TRNG2)			
				stat = Tmax[MEAN] - Tmin[MEAN];
		}
	}
	else
	{
		if (IsHourly())
		{
			for (size_t h = 0; h < 24; h++)
				stat += me[h].GetVarEx(v);
		}
		else
		{
			switch (v)
			{
			case H_KELV:	stat = K(); break;
			case H_PSYC:	stat = me[H_PRES].IsInit() ? CASCE_ETsz::GetPsychrometricConstant(me[H_PRES][MEAN] / 10) : WEATHER::MISSING; break;
			case H_SSVP:	stat = me[H_TAIR2].IsInit() ? CASCE_ETsz::GetSlopeOfSaturationVaporPressure(me[H_TAIR2][MEAN]) : WEATHER::MISSING; break;
			case H_LHVW:	stat = me[H_TAIR2].IsInit() ? 2.5023 - 0.00243054 * me[H_TAIR2][MEAN] : WEATHER::MISSING; break;	// latent heat of vaporization of water [MJ kg-1]
			case H_FNCD:	stat = me[H_SRAD2].IsInit() ? CASCE_ETsz::GetCloudinessFunction(me[H_SRMJ][SUM], CWeatherDay::GetVarEx(H_CSRA)[SUM]) : WEATHER::MISSING; break;
			case H_CSRA:	CASCE_ETsz::GetClearSkySolarRadiation(CWeatherDay::GetVarEx(H_EXRA), loc.m_z); break;
			case H_EXRA:	CASCE_ETsz::GetExtraterrestrialRadiation(loc.m_lat, int(TRef.GetJDay() + 1)); break;
			case H_SWRA:	stat = me[H_SRAD2].IsInit() ? CASCE_ETsz::GetNetShortWaveRadiation(me[H_SRMJ][SUM]) : WEATHER::MISSING; break;
			case H_ES2:		stat = stat = me[H_TMIN2].IsInit() && me[H_TMAX2].IsInit() ? e°(me[H_TMIN2], me[H_TMAX2]) : WEATHER::MISSING; break;
			case H_EA2:		stat = me[H_TDEW].IsInit() ? e°(me[H_TDEW][LOWEST], me[H_TDEW][HIGHEST]) : WEATHER::MISSING; break;
			case H_VPD2:	
			{
				CStatistic Ea = me[H_EA2];
				CStatistic Es = me[H_ES2];
				stat = (Ea.IsInit() && Es.IsInit()) ? max(0.0, Es[MEAN] - Ea[MEAN]) : WEATHER::MISSING; break;
			}
			
			case H_SRMJ:	stat = me[H_SRAD2].IsInit() ? me[H_SRAD2][MEAN]*24*3600/1000000 : WEATHER::MISSING; break;
			case H_TNTX:	//case H_TNTX:	stat = !WEATHER::IsMissing(at(H_TAIR2)) ? at(H_TAIR2) : WEATHER::MISSING; break; //!WEATHER::IsMissing(at(H_TMIN2)) && !WEATHER::IsMissing(at(H_TMAX2)) ? (at(H_TMIN2) + at(H_TMAX2)) / 2 : WEATHER::MISSING; break;
			case H_TRNG2:	//stat = me[H_TMIN2].IsInit() && me[H_TMAX2].IsInit() ? me[H_TMAX2][MEAN] - me[H_TMIN2][MEAN] : WEATHER::MISSING; break;
			default:ASSERT(false);
			}
		}
	}

	return stat;
}

//h : hour
//double CWeatherDay::GetOverheat(const COverheat& overheat)const
//{
//	_ASSERTE(m_pParent);
//	assert(fmod(24.0, step) == 0);
//	
//	//_ASSERTE(hourTmax >= 12 && hourTmax< 24);
//	//_ASSERTE(step>0 && step <= 24);
//
//	if (IsHourly())
//	{
//		size_t nbStep = size_t(24.0 / step);
//		for (size_t s = 0; s<nbStep; s++)
//		{
//			size_t h = size_t(s*step);
//			double t = at(h).at(H_TAIR) + overheat.GetValue(h);
//			t.push_back(t);
//		}
//	}
//	else
//	{
//		static const int time_factor = (int)hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
//		static const double r_hour = 3.14159 / 12;
//
//
//		CWeatherDay days[3] = { GetPrevious(), *this, GetNext() };
//
//		for (int d = 0; d < 3; d++)
//			overheat.TransformWeather(days[d]);
//
//		int nbStep = int(24.0 / step);
//		for (int s = 0; s<nbStep; s++)
//		{
//			double h = double(s*step);
//			double  mean = h<hourTmax - 12 ? days[0][H_TAIR][MEAN] : h<hourTmax ? days[1][H_TAIR][MEAN] : days[2][H_TAIR][MEAN];
//			double range = h < hourTmax - 12 ? days[0][H_TAIR][RANGE] : h < hourTmax ? days[1][H_TAIR][RANGE] : days[2][H_TAIR][RANGE];
//
//			double theta = (h - time_factor)*r_hour;
//			t.push_back(float(mean + range / 2 * sin(theta)));
//		}
//	}
//
//	return t;
//}

double CWeatherDay::GetAllenT(size_t h, size_t hourTmax)const
{
	const CWeatherDay& d1 = GetPrevious();
	const CWeatherDay& d2 = *this;
	const CWeatherDay& d3 = GetNext();

	return WBSF::GetAllenT(d1[H_TMIN2][MEAN], d1[H_TMAX2][MEAN], d2[H_TMIN2][MEAN], d2[H_TMAX2][MEAN], d3[H_TMIN2][MEAN], d3[H_TMAX2][MEAN], h, hourTmax);
}

CDailyWaveVector& CWeatherDay::GetAllenWave(CDailyWaveVector& t, size_t hourTmax, size_t step, const COverheat& overheat) const
{
	_ASSERTE(m_pParent);
	assert(fmod(24.0, step) == 0);
	_ASSERTE(hourTmax >= 12 && hourTmax< 24);
	_ASSERTE(step>0 && step <= 24);
	
	
	if (!t.IsInit())
	{
		t.m_period = CTPeriod(GetTRef(), GetTRef());
	}
	else
	{
		t.m_period.End() = GetTRef();
	}

	if (IsHourly())
	{
		size_t nbStep = size_t(24.0 / step);
		for (size_t s = 0; s<nbStep; s++)
		{
			size_t h = size_t(s*step);
			double T = overheat.GetT(*this, h, hourTmax);
			t.push_back((float)T);
		}
	}
	else
	{
		assert(m_dailyStat[H_TMIN2].IsInit());
		assert(m_dailyStat[H_TMAX2].IsInit());

		int time_factor = (int)hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
		static const double r_hour = 3.14159 / 12;
		
		double Tmin[3] = { overheat.GetTmin(GetPrevious()), overheat.GetTmin(*this), overheat.GetTmin(GetNext()) };
		double Tmax[3] = { overheat.GetTmax(GetPrevious()), overheat.GetTmax(*this), overheat.GetTmax(GetNext()) };

		int nbStep = int(24.0 / step);
		for (int s = 0; s<nbStep; s++)
		{
			double h = double(s*step);
			size_t i = h<hourTmax ? 1 : 2;
			size_t ii = h<hourTmax - 12 ? 0 : 1;
			double Tmin² = Tmin[i];
			double Tmax² = Tmax[ii];

			double  mean = (Tmin² + Tmax²) / 2;
			double range = Tmax² - Tmin²;

			double theta = (h - time_factor)*r_hour;
			t.push_back( float(mean + range / 2 * sin(theta)));
		}
	}

	return t;
}


void CWeatherDay::WriteStream(ostream& stream, const CWVariables& variable)const
{
	assert(!IsHourly());

	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			write_value(stream, m_dailyStat[v]);
}


void CWeatherDay::ReadStream(istream& stream, const CWVariables& variable)
{
	assert(!IsHourly());

	for (size_t v = 0, u = 0; v < variable.size() && u < variable.count(); v++)
		if (variable[v])
			read_value(stream, m_dailyStat[v]), u++;

	if (variable[H_TMIN2] && variable[H_TMAX2])
	{
		ASSERT(m_dailyStat[H_TMIN2].IsInit());
		ASSERT(m_dailyStat[H_TMAX2].IsInit());
	}
}



//*****************************************************************************************
//Temperature
//Carla Cesaraccio · Donatella Spano · Pierpaolo Duce Richard L. Snyder
//An improved model for determining degree-day values from daily temperature data
//Allen Wave Allen, 1976 J.C. Allen
//A modified sine wave method for calculating degree-days Environ. Entomol., 5 (1976), pp. 388–396
void CWeatherDay::ComputeHourlyTair()
{
	_ASSERTE(m_pParent);

	
	const CLocation& loc = GetLocation();

	static const double c = 0.253;//From 10 station in North America 2010
	
	CWeatherDay& me = *this;
	const CWeatherDay& dp = GetPrevious();
	const CWeatherDay& dn = GetNext();
	if (!dp[H_TMIN2].IsInit() || !me[H_TMIN2].IsInit() || !dn[H_TMIN2].IsInit() ||
		!dp[H_TMAX2].IsInit() || !me[H_TMAX2].IsInit() || !dn[H_TMAX2].IsInit())
		return;

	CSun sun(loc.m_lat, loc.m_lon);
	double Tmin[3] = { dp[H_TMIN2][MEAN], me[H_TMIN2][MEAN], dn[H_TMIN2][MEAN] };
	double Tmax[3] = { dp[H_TMAX2][MEAN], me[H_TMAX2][MEAN], dn[H_TMAX2][MEAN] };
	
	ASSERT(Tmin[0]>-999 && Tmax[0]>-999);
	ASSERT(Tmin[1]>-999 && Tmax[1]>-999);
	ASSERT(Tmin[2]>-999 && Tmax[2]>-999);

	CTRef TRef = GetTRef();
	double hourTmax = max(12.0, min(23.0, 1.00258*sun.GetSolarNoon(TRef) + 2.93458));

	int Hn = (int)Round(sun.GetSunrise(TRef));
	int Hp = Hn + 24;
	int Ho = (int)Round(sun.GetSunset(TRef));
	int Hx = Ho - 4;

	double Tn1 = Tmin[0];
	double Tx1 = Tmax[0];
	double Tp1 = Tmin[1];
	double To1 = Tx1 - c*(Tx1 - Tp1);

	double b1 = (Tp1 - To1) / sqrt((double)Hp - Ho);

	double Tn2 = Tmin[1];
	double Tx2 = Tmax[1];
	double Tp2 = Tmin[2];
	double To2 = Tx2 - c*(Tx2 - Tp2);

	double α = Tx2 - Tn2;
	double R = Tx2 - To2;
	double b2 = (Tp2 - To2) / sqrt((double)Hp - Ho);


	double time_factor = hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
	static const double r_hour = 3.14159 / 12;


	Tmin[0] = Tp1;
	Tmax[2] = Tx1;

	for (size_t h = 0; h<24; h++)
	{
		//temperature
		double T = MISSING;
		if ((Ho - Hn) <= 8 || (Ho - Hn) >= 15)
		{
			size_t i = h<hourTmax - 12 ? 0 : h<hourTmax ? 1 : 2;
			
			double mean = (Tmin[i] + Tmax[i])/2;
			double range = Tmax[i] - Tmin[i];
			double theta = (h - time_factor)*r_hour;
			T = mean + range / 2 * sin(theta);
		}
		else
		{
			if (h <= Hn)
				T = To1 + b1*sqrt((double)h + (24 - Ho));
			else if (h <= Hx)
				T = Tn2 + α*sin(((double)h - Hn) / (Hx - Hn)*PI / 2);
			else if (h <= Ho)
				T = To2 + R*sin(PI / 2 + (((double)h - Hx) / 4)*PI / 2);
			else
				T = To2 + b2*sqrt((double)h - Ho);
		}

		ASSERT(T>MISSING);

		me[h][H_TAIR2] = (float)T;
		//me[h][H_TMIN2] = (float)T;
		//me[h][H_TMAX2] = (float)T;
	}
}


//
//void ComputeHourlyTrng(CWeatherDay& day)
//{
//	for (size_t h = 0; h < 24; h++)//hourly TRange set to zero
//		day[h][H_TRNG] = 0;// day[H_TAIR][RANGE];
//	
//}

void CWeatherDay::ComputeHourlyPres()
{
	CWeatherDay& me = *this;
	ASSERT(me[H_PRES].IsInit());

	double P[3] = { GetPrevious()[H_PRES][MEAN], me[H_PRES][MEAN], GetNext()[H_PRES][MEAN] };

	for (size_t h = 0; h<24; h++)
	{
		//*****************************************************************************************
		//linear interpolation : it's biased, TODO: need to unbiaise the mean...
		double f[3] = { max(0.0, 12.0 - double(h)) / 12.0, h <= 12 ? (double(h) / 12.0) : ((24.0 - double(h)) / 12.0), max(0.0, double(h) - 12.0) / 12.0 };
		ASSERT(f[0] + f[1] + f[2] == 1);

		me[h][H_PRES] = f[0] * P[0] + f[1] * P[1] + f[2] * P[2];
	}
	
	//for (size_t h = 0; h<24; h++)
		//me[h][H_PRES] = me[H_PRES][MEAN];
}

//*****************************************************************************************
//Precipitation
void CWeatherDay::ComputeHourlyPrcp()
{
	CWeatherDay& me = *this;
	const CWeatherDay& dp = GetPrevious();
	const CWeatherDay& dn = GetNext();
	if (!dp[H_PRCP].IsInit() || !me[H_PRCP].IsInit() || !dn[H_PRCP].IsInit())
		return;

	CStatistic stats[3] = { dp[H_PRCP], me[H_PRCP], dn[H_PRCP] };

	if (stats[1][SUM]>0)
	{

		bool bBefor12 = stats[0][SUM]>0;
		bool bAfter12 = stats[2][SUM]>0;

		short nbHourBefor12 = 0;
		short nbHourAfter12 = 0;

		if (bBefor12&&bAfter12 || !bBefor12&&!bAfter12)
		{
			if (stats[1][SUM] >= 4.8)
			{
				nbHourBefor12 = nbHourAfter12 = 12;
			}
			else if (stats[1][SUM] >= 0.4)
			{
				nbHourBefor12 = nbHourAfter12 = (short)ceil(stats[1][SUM] / (0.4));
			}
			else
			{
				nbHourBefor12 = 1;
			}
		}
		else if (bBefor12)
		{
			if (stats[1][SUM] >= 2.4)
			{
				nbHourBefor12 = 12;
			}
			else if (stats[1][SUM] >= 0.2)
			{
				nbHourBefor12 = (short)ceil(stats[1][SUM] / (0.2));
			}
			else
			{
				nbHourBefor12 = 1;
			}
		}
		else
		{

			if (stats[1][SUM] >= 2.4)
			{
				nbHourAfter12 = 12;
			}
			else if (stats[1][SUM] >= 0.2)
			{
				nbHourAfter12 = (short)ceil(stats[1][SUM] / (0.2));
			}
			else
			{
				nbHourAfter12 = 1;
			}
		}

		if (bBefor12 || bAfter12)
		{
			for (size_t h = 0; h<nbHourBefor12; h++)
				me[h][H_PRCP] = (float)(stats[1][SUM] / nbHourBefor12);

			for (size_t h = nbHourBefor12; h<12; h++)
				me[h][H_PRCP] = 0;

			for (size_t h = 0; h<nbHourAfter12; h++)
				me[23 - h][H_PRCP] = (float)(stats[1][SUM] / nbHourAfter12);

			for (size_t h = nbHourAfter12; h<12; h++)
				me[23 - h][H_PRCP] = 0;
		}
		else
		{
			for (size_t h = 0; h<nbHourBefor12; h++)
				me[11 - h][H_PRCP] = (float)(stats[1][SUM] / nbHourBefor12);

			for (size_t h = nbHourBefor12; h<12; h++)
				me[11 - h][H_PRCP] = 0;

			for (size_t h = 0; h<nbHourAfter12; h++)
				me[12 + h][H_PRCP] = (float)(stats[1][SUM] / nbHourAfter12);

			for (size_t h = nbHourAfter12; h<12; h++)
				me[12 + h][H_PRCP] = 0;
		}
	}
	else
	{
		for (size_t h = 0; h<24; h++)
			me[h][H_PRCP] = 0;
	}
}

//*****************************************************************************************
//Humidity
void CWeatherDay::ComputeHourlyTdew()
{
	CWeatherDay& me = *this;
	const CWeatherDay& dp = GetPrevious();
	const CWeatherDay& dn = GetNext();

	if (!dp[H_TDEW].IsInit() || !me[H_TDEW].IsInit() || !dn[H_TDEW].IsInit())
		return;


	CStatistic stats[3] = { dp[H_TDEW], me[H_TDEW], dn[H_TDEW] };

	const CWeatherMonth* pMonth = static_cast<CWeatherMonth*>(GetParent());
	double SRADmean = pMonth->GetStat(H_SRMJ)[MEAN];
	double Kr = SRADmean>8.64 ? 6 : 12;

	for (size_t h = 0; h<24; h++)
	{

		//*****************************************************************************************
		//Tdew
		double Td1 = h<12 ? stats[0][MEAN] : stats[1][MEAN];
		double Td2 = h<12 ? stats[1][MEAN] : stats[2][MEAN];

		//parameters are estimate from the mean of best Tdew and RH
		double Tdp = 0.761*sin((h - 1.16)*PI / Kr - 3 * PI / 4);
		double moduloTd = double((h + 12) % 24);

		if (!IsMissing(me[h][H_TAIR2]))
		{
			double Tdew = min((double)me[h][H_TAIR2], Td1 + moduloTd / 24 * (Td2 - Td1) + Tdp);
			me[h][H_TDEW] = (float)Tdew;
		}

		//*****************************************************************************************
		//RH
		//double RH = Td2Hr(me[h][H_TAIR], me[h][H_TDEW]);
		//me[h][H_RELH] = (float)RH;
	}

}

//*****************************************************************************************
//Relitive Humidity
void CWeatherDay::ComputeHourlyRelH()
{
	CWeatherDay& me = *this;
	
	if (me[H_TAIR2].IsInit() && me[H_TDEW].IsInit())
	{
		for (size_t h = 0; h < 24; h++)
		{
			//*****************************************************************************************
			//RH from Tair and Tdew
			if (!WEATHER::IsMissing(me[h][H_TAIR2]) && !WEATHER::IsMissing(me[h][H_TDEW]))
			{
				double RH = Td2Hr(me[h][H_TAIR2], me[h][H_TDEW]);
				me[h][H_RELH] = (float)RH;
			}
		}
	}
	
}

//*****************************************************************************************
//wind Speed


//from: Diurnal and semidiurnal variations in global surface wind and divergence fields
//		Aiguo Dai and Clara Deser
//		National Center for Atmospheric Research, Boulder, Colorado

double CWeatherDay::GetSn(size_t n, size_t h)
{
	ASSERT(n >= 1 && n <= 2);
	ASSERT(h >= 0 && h<24);


	//calibrated with Canada-USA 2010
	double m_a[2] = { -2.20524, 0.35608 };
	double m_b[2] = { -1.70917, 0.56297 };

	double t = 2 * PI*h / 24;
	return m_a[n - 1] * cos(n*t) + m_b[n - 1] * sin(n*t);

}

double CWeatherDay::GetS(size_t h)
{
	return GetSn(1, h) + GetSn(2, h);
}
void CWeatherDay::ComputeHourlyWndS()
{
	CWeatherDay& me = *this;
	const CWeatherDay& dp = GetPrevious();
	const CWeatherDay& dn = GetNext();
	if (!dp[H_WNDS].IsInit() || !me[H_WNDS].IsInit() || !dn[H_WNDS].IsInit())
		return;



	CStatistic stats[3] = { GetPrevious()[H_WNDS], me[H_WNDS], GetNext()[H_WNDS] };
	ASSERT(stats[1].IsInit());

	
	for (size_t h = 0; h<24; h++)
	{
		double w1 = h<12 ? stats[0][MEAN] : stats[1][MEAN];
		double w2 = h<12 ? stats[1][MEAN] : stats[2][MEAN];
		double moduloW = double((h + 12) % 24);
		double Wday = w1 + moduloW / 24 * (w2 - w1);
		double Wh = Wday + GetS(h);

		me[h][H_WNDS] = (float)max(0.0, Wh); //in km/h

	}

}

//copy from WndS
void CWeatherDay::ComputeHourlyWnd2()
{
	CWeatherDay& me = *this;
	const CWeatherDay& dp = GetPrevious();
	const CWeatherDay& dn = GetNext();
	if (!dp[H_WND2].IsInit() || !me[H_WND2].IsInit() || !dn[H_WND2].IsInit())
		return;

	CStatistic stats[3] = { GetPrevious()[H_WND2], me[H_WND2], GetNext()[H_WND2] };
	ASSERT(stats[1].IsInit());


	for (size_t h = 0; h<24; h++)
	{
		double w1 = h<12 ? stats[0][MEAN] : stats[1][MEAN];
		double w2 = h<12 ? stats[1][MEAN] : stats[2][MEAN];
		double moduloW = double((h + 12) % 24);
		double Wday = w1 + moduloW / 24 * (w2 - w1);
		double Wh = Wday + GetS(h);

		me[h][H_WND2] = (float)max(0.0, Wh); //in km/h

	}

}

void CWeatherDay::ComputeHourlyWndD()
{
	CWeatherDay& me = *this;
	ASSERT(me[H_WNDD].IsInit());

	for (size_t h = 0; h < 24; h++)
		me[h][H_WNDD] = me[H_PRES][MEAN];

}
//*****************************************************************************************
//Solar radiation

void CWeatherDay::ComputeHourlySRad()
{
	CWeatherDay& me = *this;
	if (!me[H_SRAD2].IsInit())
		return;

	const CLocation & loc = GetLocation();

	array<CStatistic, 24> hourlySolarAltitude;
	

	double δ = 23.45* (PI / 180) * sin(2 * PI*(284 + GetTRef().GetJDay() + 1) / 365);
	double ϕ = Deg2Rad(loc.m_lat);

	for (size_t t = 0; t<3600*24; t+=60)
	{
		size_t h = size_t(Round(double(t) / 3600.0)) % 24;//take centered on the hour
		double w = 2 * PI*(double(t)/3600 - 12) / 24;
		double solarAltitude = max(0.0, sin(ϕ)*sin(δ) + cos(ϕ)*cos(δ)*cos(w));

		hourlySolarAltitude[h] += solarAltitude;
	}

	double sumSolarAltitude = 0;
	for (size_t h = 0; h<24; h++)
		sumSolarAltitude += hourlySolarAltitude[h][MEAN];

	for (size_t h = 0; h<24; h++)
	{
		me[h][H_SRAD2] = 0;
		if (sumSolarAltitude>0)
		{
			double r = hourlySolarAltitude[h][MEAN] / sumSolarAltitude;
			me[h][H_SRAD2] = (float)(r*24*me[H_SRAD2][MEAN]);//daily solar radiation is the mean of 24 hours
		}
	}

}


//WARNING: IsHourlyAdjusted() must be call on weather before calling this method
void CWeatherDay::ComputeHourlyVariables(CWVariables variables, std::string options)
{
	ManageHourlyData();
	
	CWeatherDay& me = *this;
	for (TVarH v = H_FIRST_VAR; v<variables.size(); v++)
	{
		if (variables[v])
		{
			ASSERT(me[v].IsInit());

			switch (v)
			{
			case H_TMIN2: break;
			case H_TAIR2: ComputeHourlyTair(); break;
			case H_TMAX2: break;
			//case H_TRNG: ComputeHourlyTrng(me); break;
			case H_PRCP: ComputeHourlyPrcp(); break;
			case H_TDEW: ComputeHourlyTdew(); break;
			case H_RELH: ComputeHourlyRelH();  break;
			case H_WNDS: ComputeHourlyWndS(); break;
			case H_WND2: ComputeHourlyWnd2(); break;
			case H_WNDD: ComputeHourlyWndD(); break;//to do
			case H_SRAD2: ComputeHourlySRad(); break;
			case H_PRES: ComputeHourlyPres(); break;
			
			case H_SNOW://take daily value devide by 24
				for (size_t h = 0; h<24; h++)
					at(h)[v] = me[v][MEAN] / 24;//to do ...
				break;

			case H_SNDH://take daily value 
			case H_SWE:
				for (size_t h = 0; h<24; h++)
					at(h)[v] = me[v][MEAN];
				break;

			//case H_ES2://from hourly temperature
			//	for (size_t h = 0; h<24; h++)
			//		at(h)[v] = WBSF::e°(at(h)[H_ES2]);
			//	break;

			//case H_EA2:
			//	for (size_t h = 0; h<24; h++)
			//		at(h)[v] = WBSF::e°(at(h)[H_TDEW]);//compute from Tdew
			//	break;

			//case H_VPD2:
			//	for (size_t h = 0; h<24; h++)
			//		at(h)[v] = at(h)[H_ES] - at(h)[H_EA];
			//	break;

			default: assert(false);//other variables to do
			}

			ASSERT(GetStat(v).IsInit());
		}
	}
}

double CWeatherDay::GetDayLength()const
{
	return GetLocation().GetDayLength(GetTRef());
}


//****************************************************************************************************************
CWeatherMonth& CWeatherMonth::operator=(const CWeatherMonth& in)
{
	if( &in!=this)
		C31Days::operator=(in);

	return *this;
}


bool CWeatherMonth::operator==(const CWeatherMonth& in)const
{
	return std::equal(begin(), end(), in.begin());
}

void CWeatherMonth::CompileStat(const CTPeriod& p )const
{
	if (!m_stat.m_bInit || m_stat.m_period != p)
	{
		CWeatherMonth& me = const_cast<CWeatherMonth&>(*this);
		CTRef TRef = GetTRef();

		me.m_stat.clear();
		for (TVarH v = H_FIRST_VAR; v<NB_VAR_H; v++)
		{
			for(size_t d=0; d<size(); d++)
			{
				if (p.IsInside(CTRef(TRef.GetYear(), TRef.GetMonth(), d, 0, p.GetTM()) ) )
				{
					me.m_stat[v] += at(d).GetStat(v);
				}
			}
		}
		for (TVarEx v = H_FIRST_VAR_EX; v<NB_VAR_ALL; v++)
		{
			for (size_t d = 0; d<size(); d++)
			{
				if (p.IsInside(CTRef(TRef.GetYear(), TRef.GetMonth(), d, 0, p.GetTM())))
				{
					me.m_stat[v] += at(d).GetVarEx(v);
				}
			}
		}

		me.m_stat.m_bInit = true;
		me.m_stat.m_period = p;
	}
}



CDailyWaveVector& CWeatherMonth::GetAllenWave(CDailyWaveVector& t, size_t hourTmax, size_t step, const COverheat& overheat) const
{
	for (const_iterator it = begin(); it != end(); it++)
		it->GetAllenWave(t, hourTmax, step, overheat);

	return t;
}

void CWeatherMonth::WriteStream(ostream& stream, const CWVariables& variable)const
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			write_value(stream, m_stat[v]);
}


void CWeatherMonth::ReadStream(istream& stream, const CWVariables& variable)
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			read_value(stream, m_stat[v]);

}

CStatistic CWeatherMonth::GetVarEx(HOURLY_DATA::TVarEx v)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetVarEx(v);

	return stat;
}

//Fcd[In]	: Fcd of previous time step
//Fcd[In]	: Fcd of current time step	
double CWeatherMonth::GetNetRadiation(double& Fcd)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetNetRadiation(Fcd);

	return stat[SUM]; //incoming radiation [MJ/m²]
}

void CWeatherMonth::ResetStat()
{
	m_stat.clear();
}

//****************************************************************************************************************
CWeatherYear& CWeatherYear::operator=(const CWeatherYear& in)
{
	if( &in!=this)
		C12Months::operator=(in);

	return *this;
}




void CWeatherYear::CompileStat(const CTPeriod& p)const
{
	CWeatherDay day;

	if (!m_stat.m_bInit || m_stat.m_period!=p)
	{
		CWeatherYear& me = const_cast<CWeatherYear&>(*this);

		me.m_stat.clear();
		for (TVarH v = H_FIRST_VAR; v<NB_VAR_H; v++)
		{
			for (size_t m = 0; m<12; m++)
			{
				me.m_stat[v] += at(m).GetStat(v,p);
			}
		}
		
		for (TVarEx v = H_FIRST_VAR_EX; v<NB_VAR_ALL; v++)
		{
			for (size_t m = 0; m<12; m++)
			{
				me.m_stat[v] += at(m).GetStat(v, p);
			}
		}
		me.m_stat.m_bInit = true;
		me.m_stat.m_period = p;
	}
}

bool CWeatherYear::IsComplete(CWVariables variables)const
{
	bool bCompte = true;

	CWVariablesCounter count = GetVariablesCount();
	size_t nbRefs = GetNbDays() * (IsHourly() ? 24 : 1);

	for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&bCompte; v++)
	{
		assert(count[v].first <= nbRefs);
		if (variables[v] && count[v].first < nbRefs)
			bCompte = false;
	}

	return bCompte;
}

CDailyWaveVector& CWeatherYear::GetAllenWave(CDailyWaveVector& t, size_t hourTmax, size_t step, const COverheat& overheat) const
{
	for (const_iterator it = begin(); it != end(); it++)
		it->GetAllenWave(t, hourTmax, step, overheat);

	return t;
}

void CWeatherYear::WriteStream(ostream& stream, const CWVariables& variable)const
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			write_value(stream, m_stat[v]);
}


void CWeatherYear::ReadStream(istream& stream, const CWVariables& variable)
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			read_value(stream, m_stat[v]);

}

ERMsg CWeatherYear::SaveData(const std::string& filePath, CTM TM, char separator)const
{
	ERMsg msg;

	if (!TM.IsInit())
		TM = GetTM();

	ofStream file;
	msg = file.open(filePath);


	if (msg)
	{
		CStatistic::SetVMiss(MISSING);
		CWVariables variable(GetVariables());

		CWeatherFormat format(TM, variable);//get default format

		string header = format.GetHeader() + "\n";
		file.write(header.c_str(), header.length());

		msg = SaveData(file, TM, format, separator);
	}

	return msg;
}

ERMsg CWeatherYear::SaveData(ofStream& file, CTM TM, const CWeatherFormat& format, char separator)const
{
	ERMsg msg;

	//write data
	const CWeatherYear* pYear = this;

	if (TM.Type() != CTM::ANNUAL)
	{
		std::string str[12];
		for (size_t m = 0; m < 12; m++)
		{
			CWeatherYear::const_iterator itM = pYear->begin() + m;

			if (TM.Type() != CTM::MONTHLY)
			{
				str[m].reserve(TM.Type() == CTM::HOURLY ? 24 * 100 : 100);


				for (CWeatherMonth::const_iterator itD = itM->begin(); itD != itM->end(); itD++)
				{
					if (TM.Type() == CTM::HOURLY)
					{
						for (CWeatherDay::const_iterator itH = itD->begin(); itH != itD->end(); itH++)
						{
							if (itH->HaveData())
							{
								//Write reference
								CTRef TRef = itH->GetTRef();
								str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1) + separator + ToString(TRef.GetHour());

								//Write variables
								for (size_t v = 0; v < format.size(); v++)
								{
									if (IsVariable(format[v].m_var) )//&& format[v].m_var != H_TRNG
									{
										str[m] += separator;
										double value = itH->at(format[v].m_var);
										if (!IsMissing(value))
											str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", value);
										else
											str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
									}
								}

								str[m] += '\n';
							}
						}
					}
					else if (TM.Type() == CTM::DAILY)
					{
						if (itD->HaveData())
						{
							//Write reference
							CTRef TRef = itD->GetTRef();
							str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1);

							//Write variables
							for (size_t v = 0; v < format.size(); v++)
							{
								if (IsVariable(format[v].m_var)/* && format[v].m_var != H_TRNG*/)
								{
									//ASSERT(format[v].m_var != H_TRNG);
									str[m] += separator;

									CStatistic stat = itD->GetStat(TVarH(format[v].m_var));//Tair and Trng are transformed into Tmin and Tmax
									//if (format[v].m_var == H_TAIR && format[v].m_stat == LOWEST)
									//{
									//	ASSERT(!stat.IsInit() || itD->GetStat(H_TRNG).IsInit());//If Tair is init, TRng must be init too
									//	CStatistic Trng = itD->GetStat(H_TRNG);
									//	if (stat.IsInit() && Trng.IsInit())
									//		str[m] += FormatA("%.1lf", stat[MEAN] - Trng[MEAN] / 2);
									//	else
									//		str[m] += "-999.0";
									//}
									//else if (format[v].m_var == H_TAIR && format[v].m_stat == HIGHEST)
									//{
									//	CStatistic Trng = itD->GetStat(H_TRNG);
									//	if (stat.IsInit() && Trng.IsInit())
									//		str[m] += FormatA("%.1lf", stat[MEAN] + Trng[MEAN] / 2);
									//	else
									//		str[m] += "-999.0";
									//}
									//else
									//{
									if (stat.IsInit())
										str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
									else
										str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
									//}
								}//id element is a variable
							}//for all element

							str[m] += '\n';
						}//have data
					}//time type : HOURLY or DAILY
				}//Day
			}
			else
			{
				//save month stats
				if (itM->HaveData())
				{
					//Write reference
					CTRef TRef = itM->GetTRef();
					str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1);

					//Write variables
					for (size_t v = 0; v < format.size(); v++)
					{
						if (format[v].m_var != H_SKIP)
						{
							str[m] += separator;

							CStatistic stat = itM->GetStat(format[v].m_var);
							if (stat.IsInit())
								str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
							else
								str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";


						}//id element is a variable
					}//for all element

					str[m] += '\n';
				}//have data
			}//MONTH format
		}//month

		for (size_t m = 0; m < 12; m++)
			if (!str[m].empty())
				file.write(str[m].c_str(), str[m].length());
	}
	else
	{
		string str;

		//save month stats
		if (pYear->HaveData())
		{
			//Write reference
			CTRef TRef = pYear->GetTRef();
			str += ToString(TRef.GetYear());

			//Write variables
			for (size_t v = 0; v < format.size(); v++)
			{
				if (format[v].m_var != H_SKIP)
				{
					str += separator;

					CStatistic stat = pYear->GetStat(format[v].m_var);
					if (stat.IsInit())
						str += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
					else
						str += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";

				}//id element is a variable
			}//for all element

			str += '\n';
			file.write(str.c_str(), str.length());
		}//have data
	}
	

	return msg;
}


CStatistic CWeatherYear::GetVarEx(HOURLY_DATA::TVarEx v)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetVarEx(v);

	return stat;
}

//Fcd[In]	: Fcd of previous time step
//Fcd[In]	: Fcd of current time step	
double CWeatherYear::GetNetRadiation(double& Fcd)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetNetRadiation(Fcd);

	return stat[SUM]; //incoming radiation [MJ/m²]
}

void CWeatherYear::ResetStat()
{ 
	m_stat.clear();
	for (size_t i = 0; i<size(); i++)
		at(i).ResetStat();
}

//****************************************************************************************************************
void CWeatherYears::clear()
{
	CWeatherYearMap::clear();
	//m_bModified = false;
	m_bCompilingHourly = false;
	m_stat.clear();
	m_format.clear();//?? humm je réessais pour voir si ca cause encore une problème

	//m_years.clear();
}

CWeatherYears::CWeatherYears(bool bIsHourly)
{
	//m_bModified = false;
	m_pParent = NULL;
	m_bHourly = bIsHourly;
	m_bCompilingHourly = false;
}

CWeatherYears::CWeatherYears(const CWeatherYears& in)
{
	//m_bModified = false;
	m_pParent = NULL;
	operator=(in);
}

CWeatherYears& CWeatherYears::operator=(const CWeatherYears& in)
{
	if (&in != this)
	{
		clear();
		m_bHourly = in.m_bHourly;
		m_format = in.m_format;
		for (const_iterator it = in.begin(); it != in.end(); it++)
		{
			CWeatherYear* pYear = new CWeatherYear;
			//CWeatherYearPtr pYear();
			pYear->Initialize(it->second->GetTRef(), this);
			*pYear = *(it->second);
			insert(make_pair(it->first, CWeatherYearPtr(pYear) ));
		}

		//m_bModified = in.m_bModified;
		m_bCompilingHourly = in.m_bCompilingHourly;
	}

	return *this;
}

bool CWeatherYears::operator==(const CWeatherYears& in)const
{
	const CWeatherYears& me = *this;

	bool bRep = true;
	
	if (m_bHourly != in.m_bHourly) bRep = false;
	if (size() != in.size())bRep = false;
	if (m_format != in.m_format)bRep = false;

	for (const_iterator it = in.begin(); it != in.end() && bRep; it++)
	{
		if (IsYearInit(it->first))
		{
			ASSERT(it->second.get());
			bRep = me[it->first] == *(it->second);
		}
		else
		{
			bRep = false;
		}
	}
	
	return bRep;
}



bool CWeatherYears::IsComplete(CWVariables variables, CTPeriod period)const
{
	bool bComplete = false;

	if (!empty())
	{

		if (!period.IsInit())
			period = GetEntireTPeriod();

		bComplete = true;
		
		//for (const_iterator it = begin(); it != end() && bCompte; it++)
		for (int year = period.GetFirstYear(); year <= period.GetLastYear() && bComplete; year++)
		{
			bool test = IsYearInit(year);
			if (!IsYearInit(year) || !at(year).IsComplete(variables) )
				bComplete = false;
		}
	}

	
	return bComplete;
}


void CWeatherYears::CleanUnusedVariable(CWVariables variables)
{
	
	CStatistic EMPTY_STAT;

	for (iterator it = begin(); it != end(); it++)
	{
		CTPeriod p = it->second->GetEntireTPeriod();
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				if (!variables[v])
					Get(TRef).SetStat(v, EMPTY_STAT);
	}
		
}

void CWeatherYears::CompileStat(const CTPeriod& p)const
{
	CWeatherDay day;

	if (!m_stat.m_bInit || m_stat.m_period != p)//|| bFoceCompile
	{
		CWeatherYears& me = const_cast<CWeatherYears&>(*this);

		me.m_stat.clear();

		for (TVarH v = H_FIRST_VAR; v<NB_VAR_H; v++)
		{
			for (const_iterator it = begin(); it != end(); it++)
			{
				switch (v)
				{
				case H_TMIN2:
				case H_TAIR2:
				case H_TMAX2:
				//case H_TRNG:
				case H_TDEW:
				case H_RELH:
				case H_WNDS:
				case H_WNDD:
				case H_PRES:
				case H_SNDH:
				case H_SWE:
				case H_WND2:
				//case H_EA:
				//case H_ES:
				//case H_VPD:
				case H_SRAD2:
				case H_ADD1:
				case H_ADD2:me.m_stat[v] += it->second->GetStat(v, p)[MEAN]; break;
				case H_PRCP:
				case H_SNOW: me.m_stat[v] += it->second->GetStat(v, p)[SUM]; break;
				default: _ASSERTE(false);
				}
			}
		}

		for (TVarEx v = H_FIRST_VAR_EX; v<NB_VAR_ALL; v++)
		{
			for (const_iterator it = begin(); it != end(); it++)
			{
				/*switch (v)
				{
				case H_TMIN:
				case H_TMAX:
				case H_KELV:
				case H_PSYC:
				case H_SSVP:
				case H_LHVW:
				case H_FNCD:
				case H_CSRA:
				case H_EXRA:
				case H_SWRA: me.m_stat[v] += it->second->GetStat(v, p)[MEAN]; break;
				}*/

				me.m_stat[v] += it->second->GetStat(v, p)[MEAN]; 
			}
		}

		me.m_stat.m_bInit = true;
		me.m_stat.m_period = p;
	}
}

CWeatherYears& CWeatherYears::append(const CWeatherYears& in)
{
	if( &in != this && !in.empty() )
	{
		insert(in.begin(), in.end());
		/*if( empty() )
		{
			operator=(in);
		}
		else*/
		//{
			//CWeatherYears& me = *this;
			//merge both array. If data is already in, replace it
			/*ASSERT( m_firstYear>0 );
			CWeatherYears tmp = *this;
			clear();
			m_firstYear=std::min(m_firstYear, in.m_firstYear);
				
			for(size_t i=0; i<tmp.size(); i++)
			{
				if( tmp.HaveData(i) )
				{
					me(int(tmp.m_firstYear+i)) = tmp(int(tmp.m_firstYear+i));
				}
			}

			for(size_t i=0; i<in.size(); i++)
			{
				if( in.HaveData(i) )
				{
					me(int(tmp.m_firstYear+i)) = tmp(int(tmp.m_firstYear+i));
				}
			}*/
		//}
	}

	return *this;
}

std::set<int> CWeatherYears::GetYears()const
{
	std::set<int> years;
	for (const_iterator it = begin(); it != end(); it++)
		years.insert(it->first);

	return years;
}

ERMsg CWeatherYears::SaveData(const std::string& filePath, CTM TM, char separator)const
{
	assert(MISSING == -999);

	ERMsg msg;

	if (!TM.IsInit())
		TM = GetTM();
	
	ofStream file;
	msg = file.open(filePath);
	

	if( msg )
	{
		CStatistic::SetVMiss(MISSING);
		CWVariables variable( GetVariables() );
		
		CWeatherFormat format(TM, variable);//get default format
		
		string header = format.GetHeader() + "\n";
		std::replace(header.begin(), header.end(), ',', separator);
		file.write(header.c_str(), header.length());
		
		//write data
		for(CWeatherYears::const_iterator itA=begin(); itA!=end(); itA++)
		{
			const CWeatherYearPtr& ptr = itA->second;
			if( ptr )
			{
				const CWeatherYear* pYear = ptr.get();

				if (TM.Type() != CTM::ANNUAL)
				{
					std::string str[12];
					for (size_t m = 0; m < 12; m++)
					{
						CWeatherYear::const_iterator itM = pYear->begin() + m;

						if (TM.Type() != CTM::MONTHLY)
						{
							str[m].reserve(TM.Type() == CTM::HOURLY ? 24 * 100 : 100);


							for (CWeatherMonth::const_iterator itD = itM->begin(); itD != itM->end(); itD++)
							{
								if (TM.Type() == CTM::HOURLY)
								{
									for (CWeatherDay::const_iterator itH = itD->begin(); itH != itD->end(); itH++)
									{
										if (itH->HaveData())
										{
											//Write reference
											CTRef TRef = itH->GetTRef();
											str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1) + separator + ToString(TRef.GetHour());

											//Write variables
											for (size_t v = 0; v<format.size(); v++)
											{
												if (IsVariable(format[v].m_var) )//&& format[v].m_var != H_TRNG
												{
													str[m] += separator;
													double value = itH->at(format[v].m_var);
													if (!IsMissing(value))
														str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", value);//ToString(value, format[v].m_var<H_ADD1 ? 1 : 3);
													else
														str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
												}
											}

											str[m] += '\n';
										}
									}
								}
								else if (TM.Type() == CTM::DAILY)
								{
									if (itD->HaveData())
									{
										//Write reference
										CTRef TRef = itD->GetTRef();
										str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1);

										//Write variables
										for (size_t v = 0; v < format.size(); v++)
										{
											if (IsVariable(format[v].m_var)/* && format[v].m_var != H_TRNG*/)
											{
												//ASSERT(format[v].m_var != H_TRNG);
												str[m] += separator;

												CStatistic stat = itD->GetStat(TVarH(format[v].m_var));//Tair and Trng are transformed into Tmin and Tmax
												//if (format[v].m_var == H_TAIR && format[v].m_stat==LOWEST)
												//{
												//	ASSERT(!stat.IsInit() || itD->GetStat(H_TRNG).IsInit());//If Tair is init, TRng must be init too
												//	CStatistic Trng = itD->GetStat(H_TRNG);
												//	if (stat.IsInit() && Trng.IsInit())
												//		str[m] += FormatA("%.1lf", stat[MEAN] - Trng[MEAN] / 2);
												//	else
												//		str[m] += "-999.0";
												//}
												//else if (format[v].m_var == H_TAIR && format[v].m_stat == HIGHEST)
												//{ 
												//	CStatistic Trng = itD->GetStat(H_TRNG);
												//	if (stat.IsInit() && Trng.IsInit())
												//		str[m] += FormatA("%.1lf", stat[MEAN] + Trng[MEAN] / 2);
												//	else
												//		str[m] += "-999.0";
												//}
												//else
												//{
												if (stat.IsInit())
													str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
												else
													str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
												//}

											}//id element is a variable
										}//for all element

										str[m] += '\n';
									}//have data
								}//time type : HOURLY or DAILY
							}//Day
						}
						else
						{
							//save month stats
							if (itM->HaveData())
							{
								//Write reference
								CTRef TRef = itM->GetTRef();
								str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1);

								//Write variables
								for (size_t v = 0; v < format.size(); v++)
								{
									if (format[v].m_var != H_SKIP)
									{
										str[m] += separator;

										CStatistic stat = itM->GetStat(format[v].m_var);
										if (stat.IsInit())
											str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
										else
											str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";


									}//id element is a variable
								}//for all element

								str[m] += '\n';
							}//have data
						}//MONTH format
					}//month

					for (size_t m = 0; m < 12; m++)
						if (!str[m].empty())
							file.write(str[m].c_str(), str[m].length());
				}
				else
				{
					string str;

					//save month stats
					if (pYear->HaveData())
					{
						//Write reference
						CTRef TRef = pYear->GetTRef();
						str += ToString(TRef.GetYear());

						//Write variables
						for (size_t v = 0; v < format.size(); v++)
						{
							if (format[v].m_var != H_SKIP)
							{
								str += separator;

								CStatistic stat = pYear->GetStat(format[v].m_var);
								if (stat.IsInit())
									str += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
								else
									str += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";

							}//id element is a variable
						}//for all element

						str += '\n';
						file.write(str.c_str(), str.length());
					}//have data
				}//ANNUAL format
			}//have data
		}//for all years

		//const_cast<CWeatherYears*>(this)->m_bModified = false;
	}//msg

	return msg;
}

ERMsg CWeatherYears::Parse(const std::string& str,  double nodata)
{
	ERMsg msg;

	stringstream stream(str);

	string header;
	getline(stream, header);

	m_format.Set(header.c_str(), " ,;\t\r\n", nodata);
	if (m_format.GetTM().Type() != CTM::HOURLY && m_format.GetTM().Type() != CTM::DAILY)
	{
		msg.ajoute("Mandatory fields are \"Year\", \"Month\", \"Day\" (or \"JDay\") and \"Hour\"(When Hourly Data).");
		if (!m_format.GetUnknownFields().empty())
			msg.ajoute("Some fields are unknown (\"" + m_format.GetUnknownFields() + "\").");

		return msg; 
	}
	
	CWeatherAccumulator accumulator(m_format.GetTM());
	SetHourly(m_format.GetTM().Type()==CTM::HOURLY);

	//load entire file
	int lineNo=1;//header line is not considerate
	string line;
	
	while( msg && getline(stream, line) )
	{
		Trim(line);
		if( !line.empty() )
		{
			StringVector data = Tokenize(line, " ,;\t\r\n", true);
			if (!data.empty())
			{
				CTRef TRef = m_format.GetTRef(data);

				msg = accumulator.Add(data, m_format);
				if (msg)
				{
					Get(TRef).SetData(accumulator);
				}
				else
				{
					GetString(IDS_BSC_INVALID_TIME_REF);
					msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));
				}
			}

			lineNo++;
		}//empty line
	}//for all line

	return msg;
}

ERMsg CWeatherYears::LoadData(const std::string& filePath, double nodata, bool bResetContent, const CWeatherYearSectionMap& sectionToLoad)
{
	ERMsg msg;
	
	if (bResetContent)
		clear();

	
	ifStream file;
	msg = file.open(filePath, ios::in|ios::binary);
		
    if( msg )
    {
		if(sectionToLoad.empty() )
		{
			string str = file.GetText();
			msg = Parse(str, nodata);
		}
		else
		{
			string header;
			getline(file, header);

			m_format.Set(header.c_str(), " ,;\t", nodata);
			if (m_format.GetTM().Type() != CTM::HOURLY && m_format.GetTM().Type() != CTM::DAILY)
			{
				msg.ajoute(FormatMsg(IDS_BSC_INVALID_DATA_FILE, filePath));
				msg.ajoute(GetString(IDS_BSC_MENDATORY_FIELDS));
				
				if (!m_format.GetUnknownFields().empty())
					msg.ajoute(FormatMsg(IDS_BSC_UNKNOWN_FIELD, m_format.GetUnknownFields()));

				return msg;
			}

			CWeatherAccumulator accumulator(m_format.GetTM());
			SetHourly(m_format.GetTM().Type() == CTM::HOURLY);

			//load only some section
			for(CWeatherYearSectionMap::const_iterator it=sectionToLoad.begin(); it!=sectionToLoad.end(); it++)
			{
				string::size_type count = string::size_type(it->second.m_end - it->second.m_begin);
				ASSERT( count>0 && count<10000000);
				
				int lineNo=it->second.m_lineNo;
				
				string str(count,0);
				file.seekg(it->second.m_begin);
				file.read(&str[0], count);

				string::size_type begin = 0;
				while( msg && begin<count)
				{
					size_t end = GetNextLinePos(str, begin);
					StringVector data = Tokenize(str, " ,;\t\r\n", true, begin, end);
					if( !data.empty() )
					{
						CTRef TRef = m_format.GetTRef(data);
					
						if( TRef.IsInit() )
						{
							msg = accumulator.Add(data, m_format);
							if(msg)
							{
								//CreateYear(TRef);
								Get(TRef).SetData(accumulator);
							}
							else
							{
								string line = str.substr(begin, end-begin);
								msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));
								
							}
						}
						else
						{
							string line = str.substr(begin, end-begin);
							msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));
						}

						begin=end;
					}

					lineNo++;
				}//for all line
			}
		}

		file.close();
		//m_bModified = false;
	}

	return msg;
}

void CWeatherYears::WriteStream(ostream& stream, const CWVariables& variable)const
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			write_value(stream, m_stat[v]);
}


void CWeatherYears::ReadStream(istream& stream, const CWVariables& variable)
{
	for (size_t v = 0; v<variable.size(); v++)
		if (variable[v])
			read_value(stream, m_stat[v]);

}

CStatistic CWeatherYears::GetVarEx(HOURLY_DATA::TVarEx v)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetVarEx(v);

	return stat;
}

//Fcd[In]	: Fcd of previous time step
//Fcd[In]	: Fcd of current time step	
double CWeatherYears::GetNetRadiation(double& Fcd)const
{
	CStatistic stat;
	for (size_t i = 0; i<size(); i++)
		stat += at(i).GetNetRadiation(Fcd);

	return stat[SUM]; //incoming radiation [MJ/m²]
}

void CWeatherYears::ResetStat()
{ 
	m_stat.clear(); 
	for (size_t i = 0; i<size(); i++)
		at(i).ResetStat();
}


void CWeatherYears::CompleteSnow()
{
	CWeatherYears& me = *this;

	//CWVariablesCounter count = GetVariablesCount();
	CWVariables variables = GetVariables();
	if (variables[H_SNDH] )
	{
		bool bHaveSnowData = false;
		size_t nbSnow = 0;

		CTPeriod p = GetEntireTPeriod(CTM::DAILY);
		size_t nbPeriod = p.GetNbYears() + 1;
		CTRef beginLastPeriod = p.Begin();
		CTRef endLastPeriod = CTRef(beginLastPeriod.GetYear(), JULY, DAY_15);


		for (size_t i = 0; i < nbPeriod; i++)
		{
			//write data
			for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
			{
				if (me[d][H_SNDH].IsInit())
				{
					bHaveSnowData = true;

					if (me[d][H_SNDH][SUM] > 0)
						nbSnow++;
				}
			}


			if (bHaveSnowData)
			{
				if (nbSnow>5)//suspicious data under 5 observations
				{
					double lastSnowVal = -999;

					for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
					{
						if (me[d][H_SNDH].IsInit())
						{
							if (lastSnowVal > 10 && me[d][H_SNDH][SUM] == 0 && d < p.End() &&
								me[d + 1][H_SNDH].IsInit() && me[d + 1][H_SNDH][SUM]>10)
							{
								//bad zero zero easted of missing value
								me[d][H_SNDH].Reset();
							}

							lastSnowVal = me[d][H_SNDH].IsInit() ? me[d][H_SNDH][SUM] : -999;
						}
						else
						{

							if (!IsMissing(lastSnowVal))
							{
								if (me[d][H_PRCP][SUM] == 0 || me[d][H_SNOW][SUM] == 0 ||
									(lastSnowVal == 0 && me[d][H_TMIN2].IsInit() && me[d][H_TMIN2][MEAN] > 6))
								{
									me[d][H_SNDH] = lastSnowVal;
								}
								else
								{
									lastSnowVal = -999;
								}
							}
						}

						for (CTRef d = endLastPeriod; d >= beginLastPeriod; d--)
						{
							if (me[d][H_SNDH].IsInit())
							{
								lastSnowVal = me[d][H_SNDH][SUM];
							}
							else
							{

								if (!IsMissing(lastSnowVal))
								{
									if (me[d][H_PRCP][SUM] == 0 || me[d][H_SNOW][SUM] == 0 ||
										(lastSnowVal == 0 && me[d][H_TMIN2].IsInit() && me[d][H_TMIN2][MEAN] > 6))
									{
										me[d][H_SNDH] = lastSnowVal;
									}
									else
									{
										lastSnowVal = -999;
									}
								}
							}
						}
					}
				}
				else
				{
					for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
						me[d][H_SNDH].Reset();
				}
			}

			bHaveSnowData = false;
			nbSnow = 0;
			beginLastPeriod = endLastPeriod + 1;
			endLastPeriod = min(p.End(), CTRef(beginLastPeriod.GetYear()+1, JULY, DAY_15));

		}//for all period

		CWVariables variables = GetVariables();

		if (variables[H_SNDH])
		{

			bool bTest = false;
			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				if (me[d][H_SNDH].IsInit() && me[d][H_SNDH][SUM]>0)
				{
					bTest = true;
				}
			}

			assert(bTest);
		}
	}
}
//*****************************************************************************************************************
CTRef CWeatherDay::GetTRef()const
{
	ASSERT(m_TRef.GetTM() == CTM(CTRef::DAILY));
	return m_TRef;
}

CTRef CWeatherMonth::GetTRef()const
{
	CTRef ref(at(0).GetTRef());
	ref.Transform( CTM(CTRef::MONTHLY) );
	return ref;
}

CTRef CWeatherYear::GetTRef()const
{
	CTRef ref(at(0).at(0).GetTRef());
	ref.Transform( CTM(CTRef::ANNUAL) );
	return ref;
}

//*****************************************************************************************************************

CWeatherStation::CWeatherStation(bool bIsHourly):
	CWeatherYears(bIsHourly)
{
	m_pAgent=NULL;
	CWeatherYears::Initialize(this);
	//m_bModified = false;
	m_bHourlyComputed = false;
}

CWeatherStation::CWeatherStation(const CWeatherStation& in)
{
	m_pAgent = NULL;
	operator=(in);
}

void CWeatherStation::Reset()
{
	CLocation::clear();
	CWeatherYears::clear();
	//m_bModified = false;
	m_bHourlyComputed = false;
}


	

CWeatherStation& CWeatherStation::operator=(const CWeatherStation& in)
{
	if( &in != this )
	{
		CWeatherYears::Initialize(this);

		CLocation::operator=(in);
		CWeatherYears::operator=(in);
		m_pAgent=in.m_pAgent;
		m_hxGridSessionID=in.m_hxGridSessionID;
		m_bHourlyComputed = in.m_bHourlyComputed;
	}


	ASSERT( in == *this);

	return *this;
}

bool CWeatherStation::operator==(const CWeatherStation& in)const
{
	bool equal=true;
	if( CLocation::operator!=(in) )equal=false;
	if( CWeatherYears::operator!=(in) )equal=false;
	if( m_pAgent!=in.m_pAgent )equal=false;
	if( m_hxGridSessionID!=in.m_hxGridSessionID )equal=false;
	if (m_bHourlyComputed != in.m_bHourlyComputed)equal = false;
	if (m_bHourly != in.m_bHourly)equal = false;

	return equal;
}



bool CWeatherStation::ComputeHourlyVariables(CWVariables variables, std::string options)
{
	CWeatherStation& me = *this;

	variables &= GetVariables();

	if (!variables[H_TDEW] && variables[H_TAIR2] && variables[H_RELH])
	{
		variables.set(H_TDEW);
		for (size_t y = 0; y < size(); y++)
		{
			for (size_t m = 0; m < me[y].size(); m++)
			{
				for (size_t d = 0; d < me[y][m].size(); d++)
				{
					if (!me[y][m][d][H_TDEW].IsInit() && me[y][m][d][H_TAIR2].IsInit() && me[y][m][d][H_RELH].IsInit())
						me[y][m][d].SetStat(H_TDEW, Hr2Td(me[y][m][d][H_TAIR2][MEAN], me[y][m][d][H_RELH][MEAN]));
				}
			}
		}
	}



	CWeatherStation copy(me);//create daily weather


	copy.IsCompilingHourly();
	copy.SetHourly(true);

	//First estimate of hourly data
	for (size_t y = 0; y < copy.size(); y++)
	{
		for (size_t m = 0; m < copy[y].size(); m++)
		{
			for (size_t d = 0; d < copy[y][m].size(); d++)
			{
				copy[y][m][d].ComputeHourlyVariables(variables, options);
			}
		}
	}

	
	//adjust variables to get the same daily mean
	CWVariables variableToAdjust("TD H WS Z WS2");
	variableToAdjust &= variables;
	
	for (TVarH v = H_TDEW; v < variableToAdjust.size(); v++)
	{
		if (variableToAdjust[v])
		{
			for (size_t k = 0; k < 5; k++)
			{
				for (size_t y = 0; y < size(); y++)
				{
					for (size_t m = 0; m < me[y].size(); m++)
					{
						for (size_t d = 0; d < me[y][m].size(); d++)
						{
						
							_ASSERTE(me[y][m][d][v].IsInit());
							CStatistic oldStat = GetDailyStat(v, copy[y][m][d]);
							_ASSERTE(oldStat);

							copy[y][m][d].ComputeHourlyVariables(v, options);

							_ASSERTE(me[y][m][d][v].IsInit());
							CStatistic newStat = GetDailyStat(v, copy[y][m][d]);
							_ASSERTE(newStat);

							double delta = me[y][m][d][v][MEAN] - newStat[MEAN];
							copy[y][m][d][v] = max(GetLimitH(v, 0), min(GetLimitH(v, 1), oldStat[MEAN] + delta));
						}
					}//v
				}//d
			}//m
		}//y
	}//k


	//Final calculation and copy original daily values
	SetHourly(true);
	for (size_t y = 0; y < size(); y++)
	{
		for (size_t m = 0; m < me[y].size(); m++)
		{
			for (size_t d = 0; d < me[y][m].size(); d++)
			{
				copy[y][m][d].ComputeHourlyVariables(variables, options);
				for (size_t h = 0; h < me[y][m][d].size(); h++)
				{
					for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
					{
						if (variables[v] && IsMissing(me[y][m][d][h][v]))
							me[y][m][d][h][v] = copy[y][m][d][h][v];
					}
				}
			}
		}
	}

	m_bHourlyComputed = true;

	return true;
}

void CWeatherStationVector::GetMean( CWeatherStation& station, CTPeriod p, size_t mergeType)const 
{
	assert(p.IsAnnual());
	ASSERT( mergeType==MERGE_FROM_MEAN || size()<=2);
	ASSERT( size() > 0);

		 
	const CWeatherStationVector& me = *this;

	for(CTRef Tref=p.Begin(); Tref<=p.End(); Tref++)
	{
		int year = Tref.GetYear();
		if( station.IsYearInit(year) )
		{

			for(TVarH v=H_FIRST_VAR; v<NB_VAR_H; v++)
			{
				CStatistic stat;
				for(size_t i=0; i<size(); i++)
				{
					if( me[i].IsYearInit(year) )
					{
						const CDataInterface& data = me[i][Tref];

						CStatistic stat2;
						if (data.GetStat(v, stat2))
						{
							
							bool exclude = stat.IsInit() && mergeType == MERGE_FROM_DB1;
							bool overwrite = stat.IsInit() && mergeType == MERGE_FROM_DB2;

							if( !exclude )
							{
								stat += stat2;
							}

							if( overwrite )
							{
								stat = stat2;
							}
						}
					}
				}//for all station

				if (stat.IsInit())
				{
					station[Tref].SetStat(v, stat[MEAN]);
				}
			}//all variables
		}//wanted year?
	}//all data
}

void CWeatherStation::FillGaps()
{
	CWVariables variables = GetVariables();

	if (IsHourly())
	{
		CTPeriod p = GetEntireTPeriod();


		for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
		{
			CHourlyData& wea° = GetHour(TRef);

			if (TRef == CTRef(2016, JANUARY, DAY_12, 21))
			{
				int gg;
				gg = 0;
			}

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (variables[v])
				{
					if (IsMissing(wea°[v]))
					{
						const CHourlyData& wea¯¹ = wea°.GetPrevious();
						if (!IsMissing(wea¯¹[v]))
						{
							CHourlyData wea¹;

							CTRef TRef2 = TRef;
							do
							{
								TRef2++;
								wea¹ = GetHour(TRef2);
							} while (IsMissing(wea¹[v]) && TRef2 <= TRef + 6 && TRef2 <= p.End());

							if (!IsMissing(wea¹[v]))
							{
								ASSERT(TRef2 - TRef > 0);

								switch (v)
								{
								case H_PRCP: break;
								case H_SRAD2: break;
								default:
									float mean = wea¯¹[v];
									float range = wea¹[v] - wea¯¹[v];

									for (CTRef TRef3 = TRef; TRef3 < TRef2; TRef3++)
									{
										double f = (double)(TRef3 - TRef + 1) / (TRef2 - TRef + 1);
										ASSERT(f >= 0 && f <= 1);

										CHourlyData& wea° = GetHour(TRef3);
										wea°.SetStat(v, mean + range*f);
									}
								}//switch
							}//if both exist
						}
					}
				}
			}
		}
	}
	else
	{
		CTPeriod p = GetEntireTPeriod();


		for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
		{
			CDay& wea° = GetDay(TRef);

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (variables[v])
				{
					if (!wea°[v].IsInit())
					{
						const CDay& wea¯¹ = wea°.GetPrevious();
						CDay wea¹ = wea°.GetNext();

						if (wea¯¹[v].IsInit() && wea¹[v].IsInit())
						{
							switch (v)
							{
							case H_PRCP: break;
							case H_SRAD2: break;
							default:
								CStatistic stat = wea¯¹[v];
								stat += wea¹[v];
								wea°.SetStat(v, stat[MEAN]);
							}//switch

						}//if both exist
					}
				}
			}
		}
	}
}

void CWeatherStation::ApplyCorrections(const CWeatherCorrections& correction)
{
	CWeatherStation& me = *this;

	CWVariables vars = GetVariables();
	CTPeriod p = GetEntireTPeriod();

	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{
		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
		{
			if (vars[v] && correction.m_variables[v])
			{
				if (me[TRef][v].IsInit())
				{
					if (v == H_PRCP)
					{
						double c = correction.GetCorrection(me, TRef, H_PRCP);
						double prcp = me[TRef][v][SUM] * c;
						me[TRef].SetStat(v, prcp);
					}
					else
					{
						double c = correction.GetCorrection(me, TRef, v);
						double value = me[TRef][v][MEAN] + c;
						ASSERT(value>-99);

						me[TRef].SetStat(v, value);
					}
				}
			}
		}
	}
}




void CWeatherStation::WriteStream(ostream& stream)const
{
	CStatistic::SetVMiss(MISSING);

	UINT64 version = 1;
	string locStr = zen::to_string(((CLocation&)(*this)), "Location", "1");
	CTPeriod p = GetEntireTPeriod();
	CWVariables variable(GetVariables());

	write_value(stream, version);
	WriteBuffer(stream, locStr);
	write_value(stream, p);
	write_value(stream, variable);

	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		Get(TRef).WriteStream(stream, variable);
}

ERMsg CWeatherStation::ReadStream(istream& stream)
{
	ERMsg msg;
	
	clear();

	UINT64 version = read_value<UINT64>(stream);
	if (version == 1)
	{
		string locStr;
		CTPeriod p;
		CWVariables variable;

		ReadBuffer(stream, locStr);
		read_value(stream, p);
		read_value(stream, variable);
		assert(p.IsInit());

		msg = zen::from_string(((CLocation&)(*this)), locStr);
		if (msg)
		{
			SetHourly(p.GetTM().Type() == CTM::HOURLY);
			//CreateYears(p);
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				Get(TRef).ReadStream(stream, variable);
			}
		}
	}
	else
	{
		msg.ajoute( GetString(IDS_BSC_ERROR_BAD_STREAM_FORMAT) );
	}

	return msg;
}


typedef array<int, NB_PRIORITY_RULE+1> PriorityRules;
typedef vector<PriorityRules> PriorityRulesVector;

class CByPriorityCriterious
{
public:
	
	CByPriorityCriterious(size_t priorityRules) :m_priorityRules(priorityRules)
	{}

	bool operator ()(const PriorityRules& in1, const PriorityRules& in2)const
	{ 
		if (m_priorityRules == OLDEST_YEAR)
		{
			if (in1[m_priorityRules] < in2[m_priorityRules])
				return true; 
		}
		else
		{
			if (in1[m_priorityRules] > in2[m_priorityRules])
				return true;
		}

		if (in1[m_priorityRules] == in2[m_priorityRules])
		{
			if (in1[LARGEST_DATA] > in2[LARGEST_DATA])
				return true;
		}
		
		return false;
	}

protected:


	size_t m_priorityRules;
};

void CWeatherStationVector::FillGaps()
{
	//first fill gap
	for (CWeatherStationVector::iterator it = begin(); it != end(); it++)
		it->FillGaps();
}

void CWeatherStationVector::ApplyCorrections(const CWeatherCorrections& correction)
{
	for (CWeatherStationVector::iterator it = begin(); it != end(); it++)
		it->ApplyCorrections(correction);
}


CWeightVector CWeatherStationVector::GetWeight(CWVariables variables, const CLocation& target)const
{
	//all the station have the same years
	assert(size()<2 || at(0).GetFirstYear() == at(1).GetFirstYear() );

	CWeightVector weight;
	if (!empty())
	{
		const CWeatherStationVector& me = *this;
		CTPeriod p = me.front().GetEntireTPeriod();

		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)//for all variables
		{
			if (variables[v])//if selected variable
			{
				weight[v].resize(size());

				assert(p == me[0].GetEntireTPeriod());
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all time reference
				{
					CStatistic sumXtemp;
				
					for (size_t i = 0; i < size(); i++)
					{
						//CStatistic stat;
						//if (me[i][TRef].GetStat(v, stat))
						if (me[i][TRef][v].IsInit())
						{
							double Xtemp = target.GetXTemp(me[i], m_bTakeElevation);
							//if (v == H_PRCP && me[i][TRef][v][SUM] < 0.1)//remove station without precipitation in the compution of the weight
								//Xtemp = 0;

							weight[v][i][TRef] = Xtemp;
							sumXtemp += Xtemp;
						}
					}

					if (sumXtemp.IsInit() && sumXtemp[SUM]>0)
					{
						for (size_t i = 0; i < size(); i++)
							weight[v][i][TRef] /= sumXtemp[SUM];
					}
				}//for all station
			}//if variable selected 
		}//for all variable
	}//if not empty
	
	return weight;
}


void CWeatherStationVector::GetInverseDistanceMean(CWVariables variables, const CLocation& target, CWeatherStation& station)const
{
	//station.clear();

	((CLocation&)station) = target;

	if (!empty())
	{
		
		CWeightVector weight = GetWeight(variables, target);

		const CWeatherStationVector& me = *this;
		CTPeriod p = GetEntireTPeriod();
		bool bIsHourly = p.GetTM().Type() == CTM::HOURLY;
		bool bIsDaily = p.GetTM().Type() == CTM::DAILY;
		station.SetHourly(bIsHourly);
		//if (bIsHourly)
		//{
		//	station.CreateYears(p);
		//}

		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)//for all variables
		{
			if (variables[v])//if selected variable
			{
				assert(p == me[0].GetEntireTPeriod());
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all time reference
				{
					//if (v == H_PRCP)
					//{
						//CStatistic x;

						//in the case of precipitation, we made inverse distance with 
						// value = 0 when prcp <0.1 and 1 when prcp>=0.1
						//when they have prcp, only nearest station with prcp are used.
						/*for (size_t i = 0; i < size(); i++)
						{
							if (me[i][TRef][v].IsInit())
							{
								if (me[i][TRef][v][SUM] >= 0.1)
								{
									x += weight[v][i][TRef];
								}
								else
								{
									x += 0;
								}
							}
						}*/

//
						//if (x.IsInit())
						//{
							//ASSERT(x[SUM] >= 0 && x[SUM] <= 1);
							//double test = x[SUM];
						//	
						//CStatistic prcp;
						//for (size_t i = 0; i < size(); i++)
						//	prcp += me[i][TRef][v][SUM] * weight[v][i][TRef];

						//if (prcp[SUM] >= 0.05/* && test >= 0.5*/)
						//{
						//	ASSERT(prcp.IsInit());
						//	station[TRef].SetStat(v, prcp[SUM]);
						//}
						//else
						//{
						//	assert(v == H_PRCP);//day without precipitation for this station
						//	station[TRef].SetStat(v, 0);
						//}
						//}
					//}
					//else //if (prcp)
					//{
					CStatistic stat;
					for (size_t i = 0; i < size(); i++)
					{
						//assert(me[i].size() == 1);
						CStatistic value;
						if (me[i][TRef].GetStat(v, value))
						{
							assert(value[NB_VALUE] == 1);
							assert(value[SUM] > -999);
							stat += value[MEAN]* weight[v][i][TRef];

						}
					}
						
					if (stat.IsInit())
					{
						assert(stat[SUM] > -999);
						double value = stat[SUM];
						if (v == H_PRCP)
						{
							if (value < 0)
								value = 0;

							value = Round(value * 10.0) / 10.0;
						}

						station[TRef].SetStat(v, value);
					}
					//}
				}//for all station
			}//if variable selected 
		}//for all variable
	}//if not empty
}



void CWeatherStationVector::MergeStation(CWeatherStation& station, CTM TM, size_t mergeType, size_t priorityRules, std::string& log)const
{
	//if merge type not equal mean, we must have only 2 station
	ASSERT( mergeType==MERGE_FROM_MEAN || size()<=2);
	
	station.Reset();
	station.SetHourly(TM.Type()==CTM::HOURLY);
	if (!empty())
	{

		if (size() > 1)//merge of 1 station is not reported
		{
			PriorityRulesVector critVector;
			set<int> years;

			for (CWeatherStationVector::const_iterator it = begin(); it != end(); it++)
			{
				std::set<int> yr = it->GetYears();

				if (!yr.empty())
				{
					CWVariablesCounter var = it->GetVariablesCount();
					
					PriorityRules PR;
					PR[LARGEST_DATA] = (int)var.GetSum();
					PR[GREATEST_YEARS] = (int)it->GetVariablesCount().GetSum();
					PR[OLDEST_YEAR] = var.GetTPeriod().Begin().GetRef();
					PR[NEWEST_YEAR] = var.GetTPeriod().End().GetRef();
					PR[NB_PRIORITY_RULE] = (int)std::distance(begin(), it);

					critVector.push_back(PR);
					years.insert(yr.begin(), yr.end());
				}
			}

			ASSERT(!years.empty());
			if (!years.empty())
			{
				std::sort(critVector.begin(), critVector.end(), CByPriorityCriterious(priorityRules));
				size_t index = (size_t)critVector.begin()->at(NB_PRIORITY_RULE);

				((CLocation&)station) = at(index);
				for (CWeatherStationVector::const_iterator it = begin(); it != end(); it++)
					station.AppendMergeID(*it);

				for (set<int>::const_iterator it = years.begin(); it != years.end(); it++)
					station.CreateYear(*it);

				CTPeriod p = station.GetEntireTPeriod(TM);
				

				GetMean(station, p, mergeType);
			
			
				for (PriorityRulesVector::const_iterator it = critVector.begin(); it != critVector.end(); it++)
				{
					size_t index = it->at(NB_PRIORITY_RULE);

					if (it == critVector.begin())
						log += station.m_ID + "," + station.m_name + ","; 
					else
						log += " , ,";

					log += at(index).m_ID + "," + at(index).m_name + "," + at(index).GetSSI("Source") + ",";

					CTRef TRef;
					//for (int i = 0; i < NB_PRIORITY_RULE; i++)
					log += ToString(it->at(0)) + ",";
					log += ToString(it->at(1)) + ",";
					TRef.SetRef(it->at(2), TM);
					log += TRef.GetFormatedString() + ",";
					TRef.SetRef(it->at(3), TM);
					log += TRef.GetFormatedString() + ",";

					double dist = at(index).GetDistance(station, false);
					double deltaElev = at(index).m_elev - station.m_elev;

					log += ToString(dist,1) + "," + ToString(deltaElev,1) + "\n";
				}

				station.SetSSI("Source", "Merged " + station.GetSSI("Source"));
			}
		}
		else
		{
			station = front();//only one station
		}
	}
}

bool CWeatherStationVector::IsComplete(CWVariables variables, CTPeriod period )const
{
	bool bComplete = true;
	for (const_iterator it = begin(); it != end() && bComplete; it++)
		bComplete = it->IsComplete(variables, period);


	return bComplete;
}

void CWeatherStationVector::CleanUnusedVariable(CWVariables variables)
{
	for (iterator it = begin(); it != end(); it++)
		it->CleanUnusedVariable(variables);
}

CDailyWaveVector& CWeatherYears::GetAllenWave(CDailyWaveVector& t, size_t hourTmax, size_t step, const COverheat& overheat) const
{
	for (const_iterator it = begin(); it != end(); it++)
		it->second->GetAllenWave(t, hourTmax, step, overheat);
	
	return t;
}
}//namespace WBSF
