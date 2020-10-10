//**************************************************************************************************************
// 13/06/2019	1.0.0	Rémi Saint-Amant	Create from articles Vermunt 2011
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "SolarModel.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSolarModel::CreateObject);
	
	enum TColdHardinessD { O_SUNRISE, O_SOLAR_NOON, O_SUNSET, O_DAY_LENGTH, O_TH, O_DD, O_SUM_DD, O_Y_SIM, NB_OUTPUTS_D };


	
	extern const char HEADER_D[] = "Sunrise,SolarNoon,Sunset,DayLength";


	CSolarModel::CSolarModel()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2020)";
	}


	CSolarModel::~CSolarModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CSolarModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		//		m_model = parameters[c++].GetInt();

		return msg;
	}

	enum TAAD { μ, ѕ, ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_ADE_PARAMS };//AestivalDiapauseEnd (Adult)
	ERMsg CSolarModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		m_output.Init(p, NB_OUTPUTS_D, -999, HEADER_D);

		double m_ADE[NB_ADE_PARAMS] = { 3022.91706,90.29928,-0.09665,54.05172,15.92350,0.12379,0.0,9.63077 };

		CSun sun(m_info.m_loc.m_lat, m_info.m_loc.m_lon);
		double sumDD = 0;

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (TRef.GetJDay() == 0)
				sumDD = 0;

			m_output[TRef][O_SUNRISE] = sun.GetSunrise(TRef);
			m_output[TRef][O_SOLAR_NOON] = sun.GetSolarNoon(TRef);
			m_output[TRef][O_SUNSET] = sun.GetSunrise(TRef);
			m_output[TRef][O_DAY_LENGTH] = sun.GetDayLength(TRef);


			size_t ii = TRef - p.Begin();
			const CWeatherDay& wday = m_weather.GetDay(TRef);
			double day_length = Round(wday.GetDayLength() / 3600.0,1);
			double threshold = Round(m_ADE[ʎ0] +m_ADE[ʎ1] * 1 / (1 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3])),1);
			double T = Round(wday[H_TNTX][MEAN],1); 
			double DD = max(0.0, threshold - T);//DD can be negative
			ASSERT(DD >= 0);

			if (ii >= m_ADE[ʎa])
				sumDD += Round(m_ADE[ʎb] + DD,1);

			double sim_y = Round(1 / (1 + exp(-(sumDD - m_ADE[μ]) / m_ADE[ѕ])) * 100,1);

			m_output[TRef][O_TH] = threshold;
			m_output[TRef][O_DD] = DD;
			m_output[TRef][O_SUM_DD] = sumDD;
			m_output[TRef][O_Y_SIM] = sim_y;

		}

		return msg;
	}


}
