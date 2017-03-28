//*********************************************************************
//27/03/2017	2.1.1	Rémi Saint-Amant    recompile
//20/09/2016	2.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
//05/09/2016	2.0		Rémi Saint-Amant	Integrated in WBSF
//25/07/2011			Rémi Saint-Amant	Integrate in BioSIM
//30/11/2013			Mike Michaelian		Creation
//			SMI_QL (Quadratic-linear model) is a derivation of 
//			the original SMI (Bilinear model). The original model assumes AET=PET when SMI>=SMIcrit
//			and decreases linearly to zero when SMI<SMIcrit. This QL model assumes AET=PET when 
//			SMI>=SMIcrit and decreases to zero using a quadratic function when SMI<SMIcrit.
//			Another difference between the SMI and SMI_QL models is that the default SMIcrit is
//			set to 300 rather than 200. Other than these two changes, the code for the original SMI 
//			and this new SMI_QL model is identical
//*********************************************************************
#include "SMI_QLModel.h"
#include "ModelBase\EntryPoint.h"
#include "Basic\WeatherStation.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSMI_QL_Model::CreateObject);


	CSMI_QL_Model::CSMI_QL_Model()
	{
		NB_INPUT_PARAMETER = 2;
		VERSION = "2.1.1 (2017)";

		//The initial SMIstart is default to 200 and then after the first calculation SMIstart is set to the previous day's SMI
		m_SMIcrit = 300.0; //Critical soil moisture (mm) 
		m_SMImax = 400.0; //Maximum soil moisture (mm)
	}

	void CSMI_QL_Model::ComputeDailyValue(CModelStatVector& output)const
	{
		//Coefficient for calculating PET (dimensionless). Set to 2.77 for daily calculations.
		static const double PET_COEF = 2.77;
		double SMI = 200.0; //Initial soil moisture (mm)
		const double elev = m_weather.m_elev; //Elevation (m)

		output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_DAILY_OUTPUT);
		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			//int year = 
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < m_weather[y][m].GetNbDays(); d++)
				{
					CTRef Tref = m_weather[y][m][d].GetTRef();

					//Input:
					double Tmin = m_weather[y][m][d][H_TMIN2][MEAN]; //Minimum daily temperature (°C)
					double Tmax = m_weather[y][m][d][H_TMAX2][MEAN]; //Maximum daily temperature (°C)
					double ppt = m_weather[y][m][d][H_PRCP][SUM]; //Precipitation (mm) 

					//Declare Actual Evapotranspriation 
					double AET=0;

					//Calculate:
					//Saturation vapour pressure for Tmax [kPa]
					double svp_Tmax = 0.61078 * exp(17.269*Tmin / (237.3 + Tmin));

					//Saturation vapour pressure for Tmin [kPa]
					double svp_Tmin = 0.61078 * exp(17.269*Tmax / (237.3 + Tmax));

					//Saturation vapour pressure for Tdew [kPa]
					double svp_Tdew = 0.61078 *  exp(17.269*(Tmin - 2.5) / (237.3 + (Tmin - 2.5)));

					//Daily vapour pressure deficit [kPa]
					double VPD = 0.5*(svp_Tmax + svp_Tmin) - svp_Tdew;

					//Daily mean temperature [°C]
					double Tmean = 0.5*(Tmin + Tmax);

					//Cold temperature reduction factor
					double K_trf = max(min((Tmean + 5.0) / 15.0, 1.0), 0.0);

					//Daily potential evapotranspiration  [mm d-1]
					double PET = PET_COEF*VPD*K_trf * exp(elev / 9300);

					//Daily actual evapotranspiration [mm d-1]
					if (SMI >= m_SMIcrit)
					{
						AET = PET;
					}
					else
					{
						AET = PET*(2 * SMI / m_SMIcrit - Square(SMI / m_SMIcrit));
					}

					//Daily water runoff [mm d-1]
					double Runoff = max(0.0, SMI + ppt - AET - m_SMImax);

					//Soil moisture at end of day d [mm]
					SMI = SMI + ppt - AET - Runoff;

					//Soil Moisture Index [%]
					double PSMI = 100 * (SMI / m_SMImax);

					//set outputs
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

	//Execute daily
	ERMsg CSMI_QL_Model::OnExecuteDaily()
	{
		ERMsg msg;

		//Compute daily values
		ComputeDailyValue(m_output);

		return msg;
	}

	ERMsg CSMI_QL_Model::OnExecuteMonthly()
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


	ERMsg CSMI_QL_Model::OnExecuteAnnual()
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


	ERMsg CSMI_QL_Model::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_SMIcrit = parameters[c++].GetReal();
		m_SMImax = parameters[c++].GetReal();

		return msg;
	}

}

