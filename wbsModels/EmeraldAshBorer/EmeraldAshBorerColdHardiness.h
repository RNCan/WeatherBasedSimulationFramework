#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	enum TPhase { ACCLIMATATION, DESACCLIMATATION, NB_PHASES };

	class CEABCHParam
	{
	public:
		CEABCHParam()
		{
			a0 = 1;
			K = 0.1;
			n_Δt = 7*24;
			λ = 1;
			wTº = -10;
		}

		double a0;
		double K;
		int n_Δt;
		double λ;
		double wTº;
	};

	class CEmeraldAshBorerColdHardinessModel : public CBioSIMModelBase
	{
	public:

		CEmeraldAshBorerColdHardinessModel();
		virtual ~CEmeraldAshBorerColdHardinessModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject() { return new CEmeraldAshBorerColdHardinessModel; }

		void ExecuteDaily(CModelStatVector& output);
		void ExecuteHourly(CModelStatVector& output);


		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);
		//static void ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1);

	protected:

		
		//std::array<CEABCHParam, NB_PHASES> m_p;
		
		int m_n_Δt;
		double m_λ;
		double m_wTº;
		double m_wTmin;
		double m_SCPᶫ;
		double m_SCPᴴ;
//		double m_ΔΣwT;


		static double logistic(double x, double L, double k, double x0);
		static double Logistic(double x, double K, double A, double R, double x0);
		static double Weibull(double x, double k, double y, double x0);
		static double SShaped(double x, double L, double k, double x0);

	};
}