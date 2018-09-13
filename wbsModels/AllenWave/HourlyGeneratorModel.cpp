//*********************************************************************
// 25-04-2017	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "HourlyGeneratorModel.h"


using namespace std;
using namespace WBSF::HOURLY_DATA; 


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CHourlyGeneratorModel::CreateObject);


	CHourlyGeneratorModel::CHourlyGeneratorModel() 
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER=1;
		VERSION = "1.0.0 (2017)";
	}

	CHourlyGeneratorModel::~CHourlyGeneratorModel()
	{}

	


	//this method is call to load your parameter in your variable
	ERMsg CHourlyGeneratorModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		m_method = parameters[0].GetSizeT();

		return msg;
	}

	double GetCesaraccio(double Tmin[3], double Tmax[3], double t, double Hn, double Hsn, double Ho)
	{
		double D = Ho - Hn;
		if (D < 3)
		{
			return WBSF::GetPolarWinter(Tmin, Tmax, t);
		}
		else if (D >(24 - 3))
		{
			return WBSF::GetPolarSummer(Tmin, Tmax, t);
		}

		//Carla Cesaraccio · Donatella Spano · Pierpaolo Duce Richard L. Snyder
		double Hp = Hn + 24;
		double Hx = Ho - 4;

		static const double c = 0.253;//From 10 station in North America 2010
		//double Tn1 = Tmin[0];
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
		double b2 = (Tp2 - To2) / sqrt(Hp - Ho);



		//temperature
		double Tair = WEATHER::MISSING;
		/*if ((Ho - Hn) <= 8 || (Ho - Hn) >= 15)
		{
		size_t i = h<hourTmax - 12 ? 0 : h<hourTmax ? 1 : 2;

		double mean = (Tmin[i] + Tmax[i])/2;
		double range = Tmax[i] - Tmin[i];
		double theta = (h - time_factor)*r_hour;
		T = mean + range / 2 * sin(theta);
		}
		else
		{*/
		if (t < Hn)
			Tair = To1 + b1*sqrt(t + (24 - Ho));
		else if (t <= Hx)
			Tair = Tn2 + α*sin((t - Hn) / (Hx - Hn)*PI / 2);
		else if (t <= Ho)
			Tair = To2 + R*sin(PI / 2 + ((t - Hx) / 4)*PI / 2);
		else
			Tair = To2 + b2*sqrt(t - Ho);
		//}

		ASSERT(Tair>MISSING);

		return Tair;

	}

	double GetCesaraccio(const CWeatherDay& me, double h, double PolarDayLength=3)
	{
		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = me.GetPrevious();
		//const CWeatherDay& me = *this;
		const CWeatherDay& dn = me.GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{
			const CLocation& loc = me.GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			double Tsr = sun.GetSunrise(me.GetTRef());
			double Tsn = sun.GetSolarNoon(me.GetTRef());
			double Tss = sun.GetSunset(me.GetTRef());
			double D = sun.GetDayLength(me.GetTRef());

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D >(24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetCesaraccio(Tmin, Tmax, h, Tsr, Tsn, Tss);
			}
		}

		return Tair;
	}


	//This method is call to compute solution
	ERMsg CHourlyGeneratorModel::OnExecuteHourly()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::HOURLY)), 1); 

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t i = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all years
		{
			const CWeatherDay& wDay = m_weather.GetDay(TRef);
			for (size_t h = 0; h < 24; h++, i++)
			{
				switch (m_method)
				{
				case HG_DOUBLE_SINE:		m_output[i][0] = float(wDay.GetDoubleSine(h)); break;
				case HG_SINE_EXP_BRANDSMA:	m_output[i][0] = float(wDay.GetSineExponential(h, SE_BRANDSMA)); break;
				case HG_SINE_EXP_SAVAGE:	m_output[i][0] = float(wDay.GetSineExponential(h, SE_SAVAGE)); break;
				case HG_SINE_POWER:			m_output[i][0] = float(wDay.GetSinePower(h)); break;
				case HG_ERBS:				m_output[i][0] = float(wDay.GetErbs(h)); break;
				case HG_ALLEN_WAVE:			m_output[i][0] = float(wDay.GetAllenT(h)); break;
				case HG_POLAR:				m_output[i][0] = float(WBSF::GetCesaraccio(wDay,h)); break;
				default: ASSERT(false);
				}
			}
		}

		return msg;
	}


}