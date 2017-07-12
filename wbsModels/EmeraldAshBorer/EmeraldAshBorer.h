#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	enum TParam{ P_EGG_L1, P_L1_L2, P_L2_L3, P_L3_L4, P_L4_PUPA, P_PUPA_ADULT, NB_PARAMS };


	class CEmeraldAshBorerModel : public CBioSIMModelBase
	{
	public:

		CEmeraldAshBorerModel();
		virtual ~CEmeraldAshBorerModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CEmeraldAshBorerModel; }
		static void ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1);

		void ExecuteDaily(CModelStatVector& output);
		bool ComputePupation(const CModelStatVector& statSim, std::vector<std::pair<size_t, double>>& obs);

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