#pragma once

#pragma warning( disable : 4201)
#pragma warning( disable : 4514)
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	enum TInstar{ OVERWINDEV = -1, L2, L3, L4, L5, L6, L7, PUPEA, ALDULT, NB_INSTAR = 8 };
	enum TClass { NB_CLASS = 50 };
	enum TModel { THERRIEN, LYSYK };

	class CJackPineModel : public CBioSIMModelBase
	{
	public:


		CJackPineModel();
		virtual ~CJackPineModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CJackPineModel; }


		void Reset();
		void ExecuteLysyk();
		void ExecuteTherrien();

	private:


		static void Therrien(double ddays, double p[NB_INSTAR + 1], double& lysykai);
		static double Allen(double TMIN, double TMAX, double BASE);
		//static void TCycle(double TMAX[3], double TMIN[3], double TARRAY[24]);
		double GetEmergeProb(const double OWAGE);
		void ComputeTransferProb(const double AGE[NB_INSTAR][NB_CLASS], double TRANS[NB_INSTAR][NB_CLASS]);

		double GetRate(double TK, int stage);

		void Devel(const CDailyWaveVector& t, double& OverWinDev, double DEV[NB_INSTAR]);
		double GetLY_AI(const double TOTAL[NB_INSTAR])const;


		//	double m_RINIT;
		int m_modelType;
		bool m_bLinear;
		bool m_bUseBud;
		bool m_bUseBark;


		//Only used in method ComputeTransferProb
		double FOLD[NB_INSTAR][NB_CLASS];
		//Only used in method GetEmergeProb
		double FOLDE;

	};

}
