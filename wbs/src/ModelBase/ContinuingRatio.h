#pragma once


#include <vector>
//#include <math.h>
#include <crtdbg.h>

#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"
#include "Basic/UtilMath.h"
#include "Basic/DegreeDays.h"

namespace WBSF
{


	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	class CContinuingRatio : public CDegreeDays
	{
	public:

		enum TOutput { O_DD, O_FIRST_STAGE, O_LAST_STAGE = O_FIRST_STAGE + nbParams, O_AVERAGE_INSTAR, NB_OUTPUT };

		CContinuingRatio();
		~CContinuingRatio();

		void Execute(const CWeatherStation& weather, CModelStatVector& stat);
		void Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output);


		int GetNbParams()const{ return nbParams; }
		int GetFirstInstar()const{ return firstInstar; }


		size_t m_startJday;

		double m_b_;			//use m_b_ when single variance
		double m_b[nbParams];	//use m_b when multiple variance
		double m_a[nbParams];

		bool m_bMultipleVariance;
		bool m_bPercent;
		bool m_bCumul;
		bool m_bAdjustFinalProportion; //in multiple variance, adjust final proportion to get 100%

	protected:

		void ExecuteOneTRef(double dd, std::vector<double>& param, double& AI)const;
		double GetP(int i, double DD, bool up)const;
	};




	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	CContinuingRatio<nbParams, firstInstar, lastInstar, header>::CContinuingRatio()
	{
		m_startJday = 0;
		m_bPercent = false;
		m_bCumul = false;
		m_bMultipleVariance = false;
		m_bAdjustFinalProportion = true;
		m_b_ = 0;
		for (int i = 0; i < nbParams; i++)
		{
			m_b[i] = 0;
			m_a[i] = 0;
		}
	}

	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	CContinuingRatio<nbParams, firstInstar, lastInstar, header>::~CContinuingRatio()
	{}


	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	void CContinuingRatio<nbParams, firstInstar, lastInstar, header>::Execute(const CWeatherStation& weather, CModelStatVector& stat)
	{
		CTPeriod p = weather.GetEntireTPeriod();
		//p.Transform(m_TM);

		std::string head = std::string("DD,") + header + ",Last,AI";
		stat.Init(p.GetNbRef(), p.Begin(), nbParams + 3, 0, head);//+3 for DD, death and AI

		for (size_t y = 0; y < weather.size(); y++)
		{
			double DD = 0.0;
			std::vector<double> param(nbParams + 1);
			double AI = firstInstar;
			p = weather[y].GetEntireTPeriod();
			//p.Transform(m_TM);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{

				if (TRef.GetJDay() >= m_startJday)
					DD += GetDD(weather.GetDay(TRef));

				ExecuteOneTRef(DD, param, AI);

				int c = 0;
				stat[TRef][c++] = DD;
				for (size_t i = 0; i < nbParams + 1; i++)
					stat[TRef][c++] = param[i];

				stat[TRef][c++] = AI;

				if (m_bPercent)
				{
					for (int i = O_FIRST_STAGE; i <= O_LAST_STAGE; i++)
					{
						stat[TRef][i] *= 100;
						if (stat[TRef][i] < 0.1)
							stat[TRef][i] = 0;
							if (stat[TRef][i]>99.9)
								stat[TRef][i] = 100;
					}
						
				}
			}
		}
	}

	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	double CContinuingRatio<nbParams, firstInstar, lastInstar, header>::GetP(int i, double DD, bool up)const
	{
		double p = 0;
		double U = up ? 1 : -1;

		if (m_bMultipleVariance)
			p = 1 / (1 + exp(U*((m_a[i] - DD) / sqrt(Square(m_b[i])*DD))));
		else p = 1 / (1 + exp(U*((m_a[i] - DD) / sqrt(Square(m_b_)*DD))));

		return p;
	}

	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	void CContinuingRatio<nbParams, firstInstar, lastInstar, header>::ExecuteOneTRef(double ddays, std::vector<double>& param, double& AI)const
	{
		ASSERT(nbParams > 0);

		//bool bValid = true;
		AI = firstInstar;
		param[0] = 1;//100% is p0

		if (ddays > 0)
		{
			//Exiting first (initial) stage
			param[0] = GetP(0, ddays, false);

			//Intermediate stages
			for (int i = 1; i < nbParams; i++)
			{
				ASSERT(m_a[i] >= m_a[i - 1]);
				double Pi2 = GetP(i, ddays, false);
				double Pi1 = GetP(i - 1, ddays, false);

				param[i] = std::max(0.0, Pi2 - Pi1);
			}

			//Entering final stage (could be death of last stage, for example, if last parameters are a longevity)
			param[nbParams] = GetP(nbParams - 1, ddays, true);

			//adjust total population to get 100%
			if (m_bMultipleVariance && m_bAdjustFinalProportion)
			{
				double sum = 0;
				for (int i = 0; i < nbParams + 1; i++)
					sum += param[i];

				for (int i = 0; i < nbParams + 1; i++)
					param[i] /= sum;
			}


			AI = 0;
			for (int i = 0; i < nbParams + 1; i++)
			{
				int instar = std::min(lastInstar, firstInstar + i);
				AI += param[i] * instar;
			}

			if (m_bCumul)
			{
				for (int i = 1; i < nbParams + 1; i++)
				{
					for (int j = i + 1; j < nbParams + 1; j++)
					{
						param[i] += param[j];
					}
				}
			}
		}
	}

	template <int nbParams, int firstInstar, int lastInstar, const char* header>
	void CContinuingRatio<nbParams, firstInstar, lastInstar, header>::Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output)
	{
		CTPeriod pIn = input.GetTPeriod();
		CTPeriod pOut = TT.GetPeriodOut();
		output.Init(pOut, nbParams + 3);

		for (CTRef TRefIn = pIn.Begin(); TRefIn <= pIn.End(); TRefIn++)
		{
			double DD = input[TRefIn][O_DD];
			ASSERT(DD > -9999);


			size_t c = TT.GetCluster(TRefIn);
			if (c != UNKNOWN_POS)
			{
				CTRef TRefOut = TT.GetClusterTRef(c);
				for (size_t v = 0; v < nbParams + 3; v++)
					output(TRefOut, O_DD) += DD;
			}
		}

	}

}