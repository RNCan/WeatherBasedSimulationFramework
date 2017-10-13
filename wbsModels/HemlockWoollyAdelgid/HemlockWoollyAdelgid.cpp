//**************************************************************************************************************
// Hemlock Woolly Adelgid winter mortalty model 
//
// for more information see :
//		McAvoy(2017) : Mortality and recovery of hemlock woolly adelgid(Adelges tsugae) to minimum winter temperatures and predictions for the future
//
// Modelling : Jaques Regniere
//
// 13/10/2017	1.0.0	Rémi Saint-Amant	Creation 
//**************************************************************************************************************
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "HemlockWoollyAdelgid.h"
#include <queue>

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CHemlockWoollyAdelgidCMModel::CreateObject);


	//Defining a simple continuing ratio model
	//
	enum TDaily{ D_TMIN, D_N, D_Q3, D_EQ1, D_EQ2, NB_OUTPUTS_D };
	enum TAnnual{ A_TMIN, A_N, A_Q3, A_EQ1, A_EQ2, A_S, A_P, A_R, NB_OUTPUTS_A };
	
	extern const char HEADER_D[] = "Tmin,N,Q3,Eq1,Eq2";
	extern const char HEADER_A[] = "Tmin,N,Q3,DDpro,Eq1,Eq2,S,P,R";
	

	CHemlockWoollyAdelgidCMModel::CHemlockWoollyAdelgidCMModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2017)";

		m_Z = 80;
		m_equation = EQUATION_2;

		m_nbDays = 3;
		m_Tlow = -6.5;


		//from SAS
		static const double P[6] = { -4.7879, -0.2665, 0.0549, 0.1460, 0, 0 };

		for (size_t i = 0; i < 6; i++)
			m_p[i] = P[i];

		m_bInit = false;
	}

	CHemlockWoollyAdelgidCMModel::~CHemlockWoollyAdelgidCMModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CHemlockWoollyAdelgidCMModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		if (parameters.size() == 2)
		{
			m_Z = parameters[0].GetFloat();
			m_equation = parameters[1].GetSizeT();
		}
		else if (parameters.size() == 8)
		{
			m_nbDays = parameters[0].GetSizeT();
			m_Tlow = parameters[1].GetFloat();
			for (size_t i = 0; i < 6; i++)
				m_p[i] = parameters[2+i].GetFloat();
		}
		
			

		return msg;
	}

	ERMsg CHemlockWoollyAdelgidCMModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Excute model on a daily basis
		ExecuteDaily(m_output);

		return msg;
	}


	ERMsg CHemlockWoollyAdelgidCMModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//Excute model on a daily basis
		
		CModelStatVector output;
		ExecuteDaily(output);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		p.Begin().m_year++;//skip the first winter
		m_output.Init(p, NB_OUTPUTS_A, -999, HEADER_A);
		
		for (size_t y = 0; y < p.size(); y++)
		{
			int year = p.GetFirstYear() + int(y);
			CTRef august31(year, AUGUST, DAY_31, 23, m_weather.GetTM());
			
			m_output[y][A_TMIN] = output[august31][D_TMIN];
			m_output[y][A_N] = output[august31][D_N];
			m_output[y][A_Q3] = output[august31][D_Q3];
			m_output[y][A_EQ1] = output[august31][D_EQ1];
			m_output[y][A_EQ2] = output[august31][D_EQ2];

	
			double Tmin = output[august31][D_TMIN];

			static size_t EQUATION_POS[NB_COLD_EQ] = { D_EQ1, D_EQ2};
			size_t posE = EQUATION_POS[m_equation];
			double M = output[august31][posE] / 100;
			double S = m_Z*(1 - M);
			m_output[y][A_S] = S;
			
			double P = Eq5(S, Tmin);
			double R = P / m_Z;
			m_output[y][A_P] = P;
			m_output[y][A_R] = R;

		}

		return msg;
	}



	double CHemlockWoollyAdelgidCMModel::Eq1(double Tmin)
	{
		static double p¹ = -5.5700;
		static double p² = -0.3222;
			

		double M = 100 / (1 + exp(-(p¹ + p²*Tmin)));
		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}
	double CHemlockWoollyAdelgidCMModel::Eq2(double Tmin, size_t N, double Q3)
	{
		double p0 = -4.7879;
		double p1 = -0.2665;
		double p2 = 0.0549;
		double p3 = 0.1460;
		
	
		double M = 100 / (1 + exp(-(p0 + p1*Tmin + p2*N + p3*Q3)));


		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}
	

	double CHemlockWoollyAdelgidCMModel::Eq5(double S, double Tmin)
	{
		if (S <= 0)
			return 0;

		static double p0 = 5.07413;
		static double p1 = -0.51873;
		static double p2 = 0.14227;

		double P = exp(p0 + (1 + p1)*log(S) + p2*Tmin);

		return P;

	}

	template<typename T>
	double array_mean (const T& v)
	{
		if (v.empty())
			return -999;
		
		double sum = 0;
		for (auto it = v.begin(); it != v.end(); it++)
			sum += *it;

		return sum / v.size();
	}

	void CHemlockWoollyAdelgidCMModel::ExecuteDaily(CModelStatVector& output)
	{
		output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS_D, -999, HEADER_D);

		for (size_t y = 0; y < m_weather.size() - 1; y++)
		{
			double TminEx = -999;
			double DDx = -999;
			double dQ3Ex = -999;
			size_t NDM1 = 0;

			double sumDDx = 0;
			size_t sumNDM1 = 0;
			

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p = CTPeriod(CTRef(year, SEPTEMBER, DAY_01, 0, m_weather.GetTM()), CTRef(year + 1, AUGUST, DAY_31, 23, m_weather.GetTM()));
			double deltaMonth = m_weather[year][JULY][H_TMAX2][MEAN] - m_weather[year][JANUARY][H_TMIN2][MEAN];
			
			std::deque<double> Q3;
			for (CTRef TRef = p.Begin() - m_nbDays; TRef < p.Begin(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				double Tmin = wDay[H_TMIN2][MEAN];
				double T = wDay[H_TNTX][MEAN];
				Q3.push_back(T);
			}
			
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				double Tmin = wDay[H_TMIN2][MEAN];
				double T = wDay[H_TNTX][MEAN];

				if (T<-1.0)
					sumNDM1++;

				double dQ3 = array_mean(Q3);


				if (TminEx == -999 || Tmin < TminEx)
				{
					TminEx = Tmin;
					dQ3Ex = dQ3;
					DDx = sumDDx;
					NDM1 = sumNDM1;
				}

				output[TRef][D_TMIN] = TminEx;
				output[TRef][D_N] = NDM1;
				output[TRef][D_Q3] = dQ3Ex;
				
				output[TRef][D_EQ1] = Eq1(TminEx);
				output[TRef][D_EQ2] = Eq2(TminEx, NDM1, dQ3Ex);
				
			
				Q3.pop_front();
				Q3.push_back(T);
			}
		}
	}


	void CHemlockWoollyAdelgidCMModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//KeyID	Year	Month	Day	n	no_HWA_alive	dead	per_mortality

		enum TInputFileColumns{	C_KEYID, C_YEAR, C_MONTH, C_DAY, C_N, C_NO_HWA_ALIVE, C_DEAD, C_PER_MORTALITY};

		ASSERT(header[C_KEYID] == "KeyID");
		ASSERT(header[C_YEAR] == "Year");
		ASSERT(header[C_MONTH] == "Month");
		ASSERT(header[C_DAY] == "Day");
		ASSERT(header[C_N] == "n");
		ASSERT(header[C_NO_HWA_ALIVE] == "no_HWA_alive");
		ASSERT(header[C_DEAD] == "dead");
		ASSERT(header[C_PER_MORTALITY] == "per_mortality");
				
		CTRef ref(ToInt(data[C_YEAR]), ToSizeT(data[C_MONTH]) - 1, ToSizeT(data[C_DAY]) - 1);

		std::vector<double> obs;
		obs.push_back(ToDouble(data[C_PER_MORTALITY]));
		obs.push_back(ToDouble(data[C_N]));

			
		m_SAResult.push_back(CSAResult(ref, obs));

	}


	double CHemlockWoollyAdelgidCMModel::EqSA(double Tmin, double DD0, double DDx, double DD10, double Q3, size_t NDM1)
	{
		double p0 = m_p[0];
		double p1 = m_p[1];
		double p2 = m_p[2];
		double p3 = m_p[3];
		double p4 = m_p[4];
		double p5 = m_p[5];


		if (Tmin >= -0.1)
			return 0;

		double M = 100 / (1 + exp(-(p0 + p1*Tmin + p2*NDM1 + p3*Q3)));


		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}

	void CHemlockWoollyAdelgidCMModel::GetFValueDaily(CStatisticXY& stat)
	{
		
		if (m_SAResult.size() > 0)
		{

			if (!m_bInit)
			{
				CStatistic years;
				for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
				{
					years += p->m_ref.GetYear();
					if (p->m_ref.GetMonth()<JULY)
						years += p->m_ref.GetYear() - 1;
					else
						years += p->m_ref.GetYear() + 1;
				}
				 
				int firstYear = (int)years[LOWEST];
				int lastYear = (int)years[HIGHEST];
				ASSERT(lastYear - firstYear >= 0);
				for (auto it = m_weather.begin(); it != m_weather.end();)
				{
					if (it->first >= firstYear && it->first <= lastYear)
						it++;
					else
						it = m_weather.erase(it);
				}

				ASSERT(m_weather.GetFirstYear() == firstYear);
				ASSERT(m_weather.GetLastYear() == lastYear);
		
				m_bInit = true;
			}
			

			CModelStatVector statSim;
			ExecuteDaily(statSim);

			for (size_t i = 0; i<m_SAResult.size(); i++)
			{
				if (statSim.IsInside(m_SAResult[i].m_ref))
				{
					double obs = m_SAResult[i].m_obs[0];
					double sim = statSim[m_SAResult[i].m_ref][D_EQ2];

					//for (size_t j = 0; j <= m_SAResult[i].m_obs[1]; j++)
						stat.Add(sim, obs);
				}
			}
		}
	}


	

}