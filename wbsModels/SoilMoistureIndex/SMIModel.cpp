//*********************************************************************
// 04/05/2017	2.1.2	Rémi Saint-Amant    New hourly generation
// 27/03/2017	2.1.1	Rémi Saint-Amant    recompile
// 20/09/2016	2.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 05/09/2016	2.0		Rémi Saint-Amant	Integrated in WBSF
// 20/06/2013	1.1		Rémi Saint-Amant	Update 
// 25/07/2011	1.0		Rémi Saint-Amant	Creation
//*********************************************************************
#include "SMIModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/WeatherStation.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;



namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSMIModel::CreateObject);


	CSMIModel::CSMIModel()
	{
		NB_INPUT_PARAMETER = 3;
		VERSION = "2.1.2 (2017)";

		//The initial SMIstart is default to m_SMIcrit and then after the first calculation SMIstart is set to the previous day's SMI
		m_SMIcrit = 200.0; //Critical soil moisture (mm)
		m_SMImax = 400.0; //Maximum soil moisture (mm)
	}

	void CSMIModel::ComputeDailyValue(CModelStatVector& output)const
	{
		//Coefficient for calculating PET (dimensionless). Set to 2.77 for daily calculations.
		static const double PET_COEF = 2.77;
		double SMI = m_SMIcrit;						//Initial soil moisture (mm) ???
		const double elev = m_weather.m_elev;	//Elevation (m)

		output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_DAILY_OUTPUT);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < m_weather[y][m].GetNbDays(); d++)
				{
					CTRef Tref = m_weather[y][m][d].GetTRef();

					//Input:
					double Tmin = m_weather[y][m][d][H_TMIN][MEAN]; //Minimum daily temperature (°C)
					double Tmax = m_weather[y][m][d][H_TMAX][MEAN]; //Maximum daily temperature (°C)
					double ppt = m_weather[y][m][d][H_PRCP][SUM]; //Precipitation (mm) 

					//Calculate:
					//Saturation vapour pressure for Tmax
					double svp_Tmax = 0.61078 * exp(17.269*Tmin / (237.3 + Tmin));

					//Saturation vapour pressure for Tmin
					double svp_Tmin = 0.61078 * exp(17.269*Tmax / (237.3 + Tmax));

					//Saturation vapour pressure for Tdew
					double svp_Tdew = 0.61078 *  exp(17.269*(Tmin - 2.5) / (237.3 + (Tmin - 2.5)));

					//Daily vapour pressure deficit
					double VPD = 0.5*(svp_Tmax + svp_Tmin) - svp_Tdew;

					//Daily mean temperature
					double Tmean = 0.5*(Tmin + Tmax);

					//Cold temperature reduction factor
					double K_trf = max(min((Tmean + 5) / 15, 1.0), 0.0);

					//Daily potential evapotranspiration (mm d-1)
					double PET = PET_COEF*VPD*K_trf * exp(elev / 9300);

					//Daily actual evapotranspiration (mm d-1)
					double AET = PET*GetAETFactor(SMI);

					//Daily water runoff (mm d-1) 
					double Runoff = max(0.0, SMI + ppt - AET - m_SMImax);

					//Soil moisture at end of day d (mm)
					SMI = SMI + ppt - AET - Runoff;

					//Soil Moisture Index (%)
					double PSMI = 100 * (SMI / m_SMImax);

					//sout outputs
					output[Tref][O_TMAX] = Tmax;
					output[Tref][O_TMIN] = Tmin;
					output[Tref][O_PPT] = ppt;
					output[Tref][O_SVP_TMAX] = svp_Tmax;
					output[Tref][O_SVP_TMIN] = svp_Tmin;
					output[Tref][O_SVP_TDEW] = svp_Tdew;
					output[Tref][O_VPD] = VPD;
					output[Tref][O_TMEAN] = Tmean;
					output[Tref][O_K_TRF] = K_trf;
					output[Tref][O_PET] = PET;
					output[Tref][O_AET] = AET;
					output[Tref][O_RUNOFF] = Runoff;
					output[Tref][O_SMI] = SMI;
					output[Tref][O_PSMI] = PSMI;
				}
			}
		}
	}

	double CSMIModel::GetAETFactor(double SMI)const
	{
		double AETFactor = 0;

		if (m_model == BILINEAR)
			AETFactor = min(1.0, SMI / m_SMIcrit);
		else if (m_model == QUADRATIC_LINEAR)
			AETFactor = min(1.0, 2 * SMI / m_SMIcrit - Square(SMI / m_SMIcrit));

		return AETFactor;
	}

	//Execute daily
	ERMsg CSMIModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Compute daily values
		ComputeDailyValue(m_output);

		return msg;
	}

	ERMsg CSMIModel::OnExecuteMonthly()
	{
		ERMsg msg;

		//Compute daily values
		CModelStatVector outputD;
		ComputeDailyValue(outputD);

		//init output
		m_output.Init(m_weather.GetEntireTPeriod(CTM::MONTHLY), NB_OUTPUT);

		//compute monthly values from daily values
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				CStatistic stat;
				for (size_t d = 0; d < m_weather[y][m].GetNbDays(); d++)
				{
					CTRef Tref = m_weather[y][m][d].GetTRef();
					stat += outputD[Tref][O_PSMI];
				}

				m_output[y * 12 + m][O_PSMI_MIN] = stat[LOWEST];
				m_output[y * 12 + m][O_PSMI_MEAN] = stat[MEAN];
				m_output[y * 12 + m][O_PSMI_MAX] = stat[HIGHEST];
			}
		}

		return msg;
	}


	ERMsg CSMIModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//Compute daily values
		CModelStatVector outputD; 
		ComputeDailyValue(outputD);

		//init output
		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUT);

		//compute yearly values from daily values
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CStatistic stat;
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < m_weather[y][m].GetNbDays(); d++)
				{
					CTRef Tref = m_weather[y][m][d].GetTRef();
					stat += outputD[Tref][O_PSMI];
				}
			}

			m_output[y][O_PSMI_MIN] = stat[LOWEST];
			m_output[y][O_PSMI_MEAN] = stat[MEAN];
			m_output[y][O_PSMI_MAX] = stat[HIGHEST];
		}

		return msg;
	}


	ERMsg CSMIModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_model = parameters[c++].GetInt();
		m_SMIcrit = parameters[c++].GetReal();
		m_SMImax = parameters[c++].GetReal();


		return msg;
	}
}