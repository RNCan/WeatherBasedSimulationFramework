//**********************************************************************
// 11-09-2018	2.0.0	Rémi Saint-Amant	Update with BioSIM 11.5.0
//											Change in unit of aridity and ET [mm instead of cm]
//											mean of VPD instead of summation
// 12-04-1999			Jacques Régnière	Creation from existing code  
//**********************************************************************

//#include <math.h>
#include "Biophysical site indices (Ecoloap).h"
#include "ModelBase/EntryPoint.h"

namespace WBSF
{

	using namespace HOURLY_DATA;
	

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBiophysicalSiteIndices::CreateObject);

	enum TOutput { O_DJ5, O_PU, O_A, O_DPV, O_ET, NB_OUTPUS };
	static const char HEADER[] = "DD5,Prcp,Aridity,VPD,ET";

	CBiophysicalSiteIndices::CBiophysicalSiteIndices()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.0.0 (2018)";
	}

	CBiophysicalSiteIndices::~CBiophysicalSiteIndices()
	{}



	//this method is call to load your parameter in your variable
	ERMsg CBiophysicalSiteIndices::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//	int c = 0;
			//m_bCumulative = parameters[c++].GetBool();

		return msg;
	}

	ERMsg CBiophysicalSiteIndices::OnExecuteAnnual()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUS, -999, HEADER);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			double DJ5 = DegreeDays(m_weather[y], 5.0);
			double A = 0, ET = 0, PU = 0;
			Aridite(m_weather[y], A, ET, PU);
			double uVPD = UtilDeficitVaporPressure(m_weather[y])*10;//[kPa] --> [hPa]

			m_output[y][O_DJ5] = DJ5;	
			m_output[y][O_PU] = PU;		//[mm]
			m_output[y][O_A] = A;		//[mm] since version 2.0.0
			m_output[y][O_DPV] = uVPD;	//[hPa] (mbar), mean instead of summation since version 2.0.0
			m_output[y][O_ET] = ET;		//[mm]since version 2.0.0
		}

		return msg;
	}

	double CBiophysicalSiteIndices::DegreeDays(const CWeatherYear& weather, double threshold)
	{
		double sum = 0;
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double Tmin = weather[TRef][H_TMIN][MEAN];
			double Tmax = weather[TRef][H_TMAX][MEAN];

			double DD = (Tmin + Tmax) / 2.0 - threshold;
			if (DD > 0)
				sum += DD;
		}

		return sum;
	}

	void CBiophysicalSiteIndices::Aridite(const CWeatherYear& weather, double& A, double& E, double& PU)
	{
		std::array<CStatistic, 12>  Ta;
		std::array<CStatistic, 12>  P;

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double Tmin = weather[TRef][H_TMIN][MEAN];
			double Tmax = weather[TRef][H_TMAX][MEAN];
			double prcp = weather[TRef][H_PRCP][SUM];

			size_t m = TRef.GetMonth();
			Ta[m] += (Tmin + Tmax) / 2;
			P[m] += prcp;
		}
		 
		//calculer les statistiques mensuelles
		double I = 0;
		for (size_t m = 0; m < 12; m++)
		{ 
			//cumuler I
			if (Ta[m][MEAN] > 0)
				I += pow(Ta[m][MEAN] / 5.0, 1.5);
		}

		//calculer alpha
		double alpha = 0.49 + 0.0179*I - 0.0000771*I*I + 0.000000675*I*I*I;
		A = 0.0;
		//calculer Et pour le mois et A
		for (size_t m = 0; m < 12; m++)
		{
			if (Ta[m][MEAN] > 0.0)
			{
				double Et = 16.0*pow(10.0*Ta[m][MEAN] / I, alpha);//[mm]
				E += Et;//[mm]

				double Ar = (Et - P[m][SUM]); //[mm]
				if (Ar > 0.0)
					A += Ar; //[mm]
			}
		}

		//comme bonus, calculer PU (precipitation utilisable)
		PU = (P[JUNE][SUM] + P[JULY][SUM] + P[AUGUST][SUM]); //precipitation [mm]
	}

	//mean of June, July and August vapor pressure deficit [kPa]
	double CBiophysicalSiteIndices::UtilDeficitVaporPressure(const CWeatherYear& weather)
	{
		CStatistic udpv;

		int year = weather.GetTRef().GetYear();
		CTPeriod p(CTRef(year, JUNE, FIRST_DAY), CTRef(year, AUGUST, LAST_DAY));
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double Tmin = weather[TRef][H_TMIN][MEAN];
			double Tmax = weather[TRef][H_TMAX][MEAN];

			double T1 = 7.5*Tmax / (237.3 + Tmax);
			double T2 = 7.5*Tmin / (237.3 + Tmin);
			udpv += 0.6108*(pow(10., T1) - pow(10., T2));//[kPa]
		}


		return udpv[MEAN];//[kPa]
	}


}