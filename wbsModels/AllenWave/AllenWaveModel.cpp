//*********************************************************************
// 20/09/2016	1.1.0	R�mi Saint-Amant    Change Tair and Trng by Tmin and Tmax
//19-07-2016	1.0.0	R�mi Saint-Amant	Creation
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "AllenWaveModel.h"


using namespace std;
using namespace WBSF::HOURLY_DATA; 


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CAllenWaveModel::CreateObject);


	CAllenWaveModel::CAllenWaveModel() 
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER=1;
		VERSION = "1.1.0 (2016)";
	}

	CAllenWaveModel::~CAllenWaveModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CAllenWaveModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		m_hourTmax = parameters[0].GetInt();
		return msg;
	}


	//This method is call to compute solution
	ERMsg CAllenWaveModel::OnExecuteHourly()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::HOURLY)), 1);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t i = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all years
		{
			const CWeatherDay& weather = m_weather.GetDay(TRef);
			for (size_t h = 0; h < 24; h++, i++)
				m_output[i][0] = weather.GetAllenT(h, m_hourTmax);
		}



		return msg;
	}


}