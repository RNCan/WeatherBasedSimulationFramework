//***********************************************************
// 11/09/2019	2.0.0	Rémi Saint-Amant   Update to BioSIM 11
// 12/04/1999	1.0.0	Jacques Régnière   Creation From existing code.
//***********************************************************
//Biophysical Site Indices for Shade Tolerant and Intolerant Boreal Species
//Ung (1999)

#include "SiteIndexClimate.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/WeatherStation.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{
	enum { O_DD5, O_UTIL_PRCP, O_ARIDITY, O_DPV, O_PET, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSiteIndexClimateModel::CreateObject);



	double CSiteIndexClimateModel::GetDegreeDays(const CWeatherYear& weather)
	{
		static const double THRESHOLD = 5.0;
		double CDD = 0;

		for (size_t d = 0; d < weather.GetNbDays(); ++d)
		{
			const CWeatherDay& day = weather.GetDay(d);
			double DD = (day[H_TMIN][MEAN] + day[H_TMAX][MEAN]) / 2 - THRESHOLD;
			if (DD > 0)
				CDD += DD;

		}

		return CDD;
	}

	double CSiteIndexClimateModel::GetUtilPrcp(const CWeatherYear& weather)
	{
		double PU = 0;
		for (size_t m = JUNE; m <= AUGUST; m++)
		{
			PU+=weather[m][H_PRCP][SUM];
		}

		return PU;
	}

	double CSiteIndexClimateModel::GetAridite(const CWeatherYear& weather)
	{
		//initialiser les compteurs
		double Ta[12] = { 0 };
		double P[12] = { 0 };
		
		//calculer les totaux mensuels
		double I = 0;
		for (size_t m = 0; m < 12; m++)
		{
			Ta[m] = weather[m][H_TNTX][MEAN];
			P[m] = weather[m][H_PRCP][SUM];

			//cumuler I
			if (Ta[m] > 0) I += pow(Ta[m] / 5., 1.5);
		}

		//calculer alpha
		double alpha = 0.49 + 0.0179*I - 0.0000771*I*I + 0.000000675*I*I*I;
		double A = 0.;
		//calculer Et pour le mois et A
		for (size_t m= 0; m< 12; m++)
		{
			if (Ta[m] > 0.)
			{
				double Et = 16*pow(10.*Ta[m] / I, alpha);//mm
				A += std::max( 0.0, Et - P[m]);
			}
		}
		
		return A;
	}

	double CSiteIndexClimateModel::GetEvapotranspiration(const CWeatherYear& weather)
	{

		double Et = 0;
		double Ta[12] = { 0 };
		double P[12] = { 0 };

		//calculer les totaux mensuels
		double I = 0;
		for (size_t m = 0; m < 12; m++)
		{
			Ta[m] = weather[m][H_TNTX][MEAN];
			P[m] = weather[m][H_PRCP][SUM];

			//cumuler I
			if (Ta[m] > 0) I += pow(Ta[m] / 5., 1.5);
		}

		//calculer alpha
		double alpha = 0.49 + 0.0179*I - 0.0000771*I*I + 0.000000675*I*I*I;
		double A = 0.;
		//calculer Et pour le mois et A
		for (size_t m = 0; m < 12; m++)
		{
			if (Ta[m] > 0.)
			{
				Et += 16*pow(10.*Ta[m] / I, alpha);//mm
			}
		}
		
		
		return Et;
	}

	double CSiteIndexClimateModel::GetVaporPressureDeficit(const CWeatherYear& weather)
	{
		double dpv = 0;

		for (size_t d = 151; d < 243; d++)
		{
			const CWeatherDay& day = weather.GetDay(d);
			if (day[H_TMIN][MEAN] > 0. && day[H_TMAX][MEAN] > 0. )
			{
				double T1 = 7.5*day[H_TMAX][MEAN] / (237.3 + day[H_TMAX][MEAN]);
				double T2 = 7.5*day[H_TMIN][MEAN] / (237.3 + day[H_TMIN][MEAN]);
				dpv += 6.108*(pow(10., T1) - pow(10., T2));
			}
		}
		
		return dpv;
	}

	CSiteIndexClimateModel::CSiteIndexClimateModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.0.0 (2019)";
	}

	CSiteIndexClimateModel::~CSiteIndexClimateModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSiteIndexClimateModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		return msg;
	}

	ERMsg CSiteIndexClimateModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		m_output.Init(outputPeriod, NB_OUTPUTS);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			m_output[y][O_DD5] = GetDegreeDays(m_weather[y]);
			m_output[y][O_UTIL_PRCP] = GetUtilPrcp(m_weather[y]);
			m_output[y][O_ARIDITY] = GetAridite(m_weather[y]);
			m_output[y][O_DPV] = GetVaporPressureDeficit(m_weather[y]); 
			m_output[y][O_PET] = GetEvapotranspiration(m_weather[y]);
		}

		return msg;
	}


}