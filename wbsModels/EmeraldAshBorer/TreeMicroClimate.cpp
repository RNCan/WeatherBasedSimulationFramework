//**************************************************************************************************************
// 13/04/2018	1.0.1	Rémi Saint-Amant	Compile with VS 2017
// 08/05/2017	1.0.0	Rémi Saint-Amant	Create from articles
//												Lyons and Jones 2006
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "TreeMicroClimate.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//*******************************************************************************************

	CMicroClimate::CMicroClimate(const CWeatherDay& weather)
	{
		double Tmin = weather[H_TMIN][MEAN];
		double Tmax = weather[H_TMAX][MEAN];
		double Trange = Tmax - Tmin;
		double Sin = sin(2 * 3.14159*(weather.GetTRef().GetJDay() / 365. - 0.25));

		//convert air temperature to bark temperature
		m_Tmin = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		m_Tmax = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if (m_Tmin > m_Tmax)
			Switch(m_Tmin, m_Tmax);
	}

	double CMicroClimate::GetT(size_t h, size_t hourTmax)const
	{
		double OH = 0;

		double range = m_Tmax - m_Tmin;
		assert(range >= 0);

		int time_factor = (int)hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
		double theta = ((int)h - time_factor)*3.14159 / 12.0;
		double T = (m_Tmin + m_Tmax) / 2 + range / 2 * sin(theta);

		return T;
	}

	//*******************************************************************************************
	
	CNewtonianBarkTemperature::CNewtonianBarkTemperature(double Tair, double K):
		m_K(K),
		m_Tt(0)
	{
		
		ASSERT(Tair > -60 && Tair < 60);
		
		//after Vermunt 2012
		//We used an initial condition of T0 = 0 ◦C, 
		//and gave the model 48 time steps(h) to converge.
		for (size_t h = 0; h< 48; h++)
		{
			next_step(Tair);
		}

		ASSERT(m_Tt > -60 && m_Tt < 60);
	}

	double CNewtonianBarkTemperature::GetTbark()const
	{
		ASSERT(m_Tt > -60 && m_Tt < 60);
		return m_Tt;
	}

	double CNewtonianBarkTemperature::next_step(double Tair)
	{
		ASSERT(m_Tt > -60 && m_Tt < 60);
		m_Tt = m_Tt + m_K * (Tair - m_Tt);
		return m_Tt;
	}


}