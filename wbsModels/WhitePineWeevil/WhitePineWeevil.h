#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	enum TParam{ S_EGG, S_L1, S_L2, S_L3, S_L4, S_PUPA, S_ADULT, NB_PARAMS };


	class CWhitePineWeevilModel : public CBioSIMModelBase
	{
	public:

		CWhitePineWeevilModel();
		virtual ~CWhitePineWeevilModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual bool GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CWhitePineWeevilModel; }
		static void ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1);

		void ExecuteDaily(CModelStatVector& output);
		//bool ComputePupation(const CModelStatVector& statSim, std::vector<std::pair<size_t, double>>& obs);

	protected:

		//members (model parameters)
		bool m_bCumulative;
		double m_threshold;
		size_t m_startJday;
		std::array<double, NB_PARAMS> m_a;
		std::array<double, NB_PARAMS> m_b;
		size_t m_lastParam;

		static const size_t FISRT_HL_JDAY;
		static const double THRESHOLD;
		static const double A[NB_PARAMS];
		static const double B[NB_PARAMS];

	};
}