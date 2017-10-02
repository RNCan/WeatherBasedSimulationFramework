//**************************************************************************************************************
// 08/05/2012	1.0.0	Rémi Saint-Amant	Create from articles
//												Lyons and Jones 2006
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
	enum TDaily{ D_TMIN, D_DD0, D_DD10, D_Q10, D_TMINEX, D_DD0EX, D_DD10EX, D_Q10EX, D_EQ3, D_EQ6, D_EQ_REMI, NB_OUTPUTS_D };
	enum TAnnual{ A_TMINEX, A_DD0EX, A_DD10EX, A_DD0pro, A_EQ3, A_EQ6, A_S, A_P, A_R, NB_OUTPUTS_A };
	extern const char HEADER_D[] = "Tmin,DD0,DD10,Q10,TminEx,DD0Ex,DD10Ex,DD0pro,Eq3,Eq6";
	extern const char HEADER_A[] = "TminEx,DD0Ex,DD10Ex,DDpro,Eq3,Eq6,S,P,R";

	

	CHemlockWoollyAdelgidCMModel::CHemlockWoollyAdelgidCMModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2017)";

		m_sistensFallDensity = 80;
		m_equation = EQUATION_3;

		m_nbDays = 0;
		m_Tlow = 0;
		for (size_t i = 0; i < 6; i++)
			m_p[i] = 0;

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
			m_sistensFallDensity = parameters[0].GetFloat();
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
			
			m_output[y][A_TMINEX] = output[august31][D_TMINEX];
			m_output[y][A_DD0EX] = output[august31][D_DD0EX];
			m_output[y][A_DD10EX] = output[august31][D_DD10EX];
			m_output[y][A_EQ3] = output[august31][D_EQ3];
			m_output[y][A_EQ6] = output[august31][D_EQ6];
			

			CTPeriod pro = CTPeriod(CTRef(year, MARCH, DAY_22, 0, m_weather.GetTM()), CTRef(year, JUNE, DAY_15, 23, m_weather.GetTM()));
			double DDpro = 0;

			for (CTRef TRef = pro.Begin(); TRef <= pro.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				double T = wDay[H_TNTX][MEAN];

				DDpro += max(0.0, T);
			}

			double DD10 = output[august31][D_DD10EX];
			m_output[y][A_DD0pro] = DDpro;

			static size_t EQUATION_POS[2] = { D_EQ3, D_EQ6 };
			size_t posE = EQUATION_POS[m_equation];
			double S = m_sistensFallDensity*(1 - output[august31][posE] / 100);
			m_output[y][A_S] = S;

			double P = Eq11(S, DD10, DDpro);
			double R = P / m_sistensFallDensity;
			m_output[y][A_P] = P;
			m_output[y][A_R] = R;

		}

		return msg;
	}



	double CHemlockWoollyAdelgidCMModel::Eq3(double Tmin)
	{
		static double p¹ = -5.7145;
		static double p² = -0.3305;
			

		double M = 100 / (1 + exp(-(p¹ + p²*Tmin)));
		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}

	double CHemlockWoollyAdelgidCMModel::Eq6(double Tmin, double DD0, double DD10)
	{
		static double p0 = -4.8458;
		static double p1 = -0.2848;
		static double p2 = -0.0067567;
		static double p3 = 0.0257068;
		static double p4 = -0.0003509;
		static double p5 = 0.0013242; 
		
		/*
		//from Simulated Annealing
		static double p0 = -4.03920;
		static double p1 = -0.22077;
		static double p2 = -0.00409;
		static double p3 = -1.35397;
		static double p4 = -0.00042;
		static double p5 = -0.06800;*/

		
		double M = 100 / (1 + exp(-(p0 + p1*Tmin + p2*DD0 + p3*DD10 + p4*Tmin*DD0 + p5*Tmin*DD10)));
		

		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;
	
		return M;

	}

	double CHemlockWoollyAdelgidCMModel::EqRemi(double Tmin, double DD0, double DD10, double dQ10)
	{
		double p0 = m_p[0];
		double p1 = m_p[1];
		double p2 = m_p[2];
		double p3 = m_p[3];
		double p4 = m_p[4];
		double p5 = m_p[5];

		
		//double M = 100 / (1 + exp(-(p0 + p1*Tmin + p2*DD0 + p3*DD10 + p4*Tmin*DD0 + p5*Tmin*DD10)));
		double M = 100 / (1 + exp(-(p0 + p1*Tmin + p2*DD0 + p3*DD10 + p4*dQ10)));

		if (M < 0.1)
			M = 0;
		if (M > 99.9)
			M = 100;

		return M;

	}

	double CHemlockWoollyAdelgidCMModel::Eq11(double S, double DD10, double DDpro)
	{
		if (S <= 0)
			return 0;

		static double p0 = 5.43700;
		static double p1 = -0.61020;
		static double p2 = -0.03568;
		static double p3 = -0.001808;

		double P = exp(p0 + (1 + p1)*log(S) + p2*DD10 + p3*DDpro);
		
		return P;

	}

	template<typename T>
	double array_mean (const T& v)
	{
		ASSERT(!v.empty());

		double sum = std::accumulate(v.begin(), v.end(), 0);
		return sum / v.size();
	}

	void CHemlockWoollyAdelgidCMModel::ExecuteDaily(CModelStatVector& output)
	{
		output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS_D, -999, HEADER_D);

		for (size_t y = 0; y < m_weather.size() - 1; y++)
		{
			double TminEx = -999;
			double DD0 = -999;
			double DD10 = -999;
			//double dQ10ExMin = 0;
			double dQ10Ex = 0;
			//double dQ10ExMax = 0;*/
			//double seuil = 0;

			double sumDD0 = 0;
			double sumDD10 = 0; 

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p = CTPeriod(CTRef(year, SEPTEMBER, DAY_01, 0, m_weather.GetTM()), CTRef(year + 1, AUGUST, DAY_31, 23, m_weather.GetTM()));
			double deltaMonth = m_weather[year][JULY][H_TMAX2][MEAN] - m_weather[year][JANUARY][H_TMIN2][MEAN];
			
			std::deque<double> Q10min;
			std::deque<double> Q10;
			std::deque<double> Q10max;
			for (CTRef TRef = p.Begin() - m_nbDays; TRef < p.Begin(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				//double Tmin = wDay[H_TMIN2][MEAN];
				double T = wDay[H_TNTX][MEAN];
				//double Tmax = wDay[H_TMAX2][MEAN];
			//	Q10min.push_back(Tmin);
				Q10.push_back(T);
				//Q10max.push_back(Tmax);
			}
			
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				double Tmin = wDay[H_TMIN2][MEAN];
				double T = wDay[H_TNTX][MEAN];
				//double Tmax = wDay[H_TMAX2][MEAN];
				//sumDD0 += 0.0 - min(0.0, T);
				//sumDD10 += -10.0 - min(-10.0, T);

				sumDD0 += 0.0 - min(0.0, T);
				sumDD10 += m_Tlow - min(m_Tlow, T);



				//double dQ10min = Tmin - array_mean(Q10min);
				double dQ10 = Tmin - array_mean(Q10);
				//double dQ10max = Tmin - array_mean(Q10max);*/
				//ASSERT(dQ10 > -999);
				//if (dQ10 < dQ10Ex)//&& dQ10 < m_Tlow
					//dQ10Ex = dQ10;


				if (TminEx == -999 || Tmin < TminEx)
				{
					TminEx = Tmin;
					//dQ10ExMin = dQ10min;
					dQ10Ex = dQ10;
					//dQ10ExMax = dQ10max;
					DD0 = sumDD0;
					DD10 = sumDD10;
				}

			
		
				//ASSERT(p0 <= p1 && p1 <= p2 && p2 <= p3);



				/*double delta = 0;
				if (Tmin < m_p[0])
					delta = m_Tlow;
				else if (Tmin >= m_p[0] && Tmin <= m_p[1])
					delta = (m_p[1] - Tmin) / (m_p[1] - m_p[0])*m_Tlow;
				else if (Tmin >= m_p[1] && Tmin <= m_p[2])
					delta = 0;
				else if (Tmin >= m_p[2] && Tmin <= m_p[3])
					delta = (Tmin - m_p[2]) / (m_p[3] - m_p[2])*m_p[5];
				else
					delta = m_p[5];
				
				seuil += delta;*/

				output[TRef][D_TMIN] = Tmin;
				output[TRef][D_DD0] = sumDD0;
				output[TRef][D_DD10] = sumDD10;
				output[TRef][D_Q10] = dQ10;


				output[TRef][D_TMINEX] = TminEx;
				output[TRef][D_DD0EX] = DD0;
				output[TRef][D_DD10EX] = DD10;
				output[TRef][D_Q10EX] = dQ10Ex;
				
				output[TRef][D_EQ3] = Eq3(TminEx);
				output[TRef][D_EQ6] = Eq6(TminEx, DD0, DD10);
				output[TRef][D_EQ_REMI] = EqRemi(TminEx, DD0, DD10, dQ10Ex);
				//output[TRef][D_EQ_REMI] = EqRemi(TminEx, dQ10ExMin, dQ10Ex);

				/*
				Q10min.pop_front();
				Q10min.push_back(Tmin);
				
				Q10.pop_front();
				Q10.push_back(T);

				Q10max.pop_front();
				Q10max.push_back(Tmax);*/
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

	void CHemlockWoollyAdelgidCMModel::GetFValueDaily(CStatisticXY& stat)
	{
		/*if (m_p[0] > m_p[1] || m_p[1] > m_p[2] || m_p[2] > m_p[3])
		{
			stat.Add(-999, 999);
			return;
		}*/

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
					double sim = statSim[m_SAResult[i].m_ref][D_EQ_REMI];

				//	for (size_t j = 0; j <= m_SAResult[i].m_obs[1]; j++)
						stat.Add(sim, obs);
				}
			}
		}
	}


	

}