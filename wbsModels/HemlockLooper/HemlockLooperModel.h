//*****************************************************************************
// File: HLModel.h
// Class: CHLModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "HemlockLooperEquations.h"

namespace WBSF
{
	//**********************************************************
	class CHLModel : public CBioSIMModelBase
	{
	public:

		CHLModel();
		virtual ~CHLModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CHLModel; }

	protected:

		void GetDailyStat(CModelStatVector& stat);
		void ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output);
		CInitialPopulation GetFirstOviposition();

		void AddDailyResult(const StringVector& header, const StringVector& data);
		void GetFValueDaily(CStatisticXY& stat);
		void GetFValueDailyEclosion(CStatisticXY& stat);
		void GetFValueDailyStage(CStatisticXY& stat);
		void GetFValueDailyAI(CStatisticXY& stat);

		//User model
		bool m_bApplyMortality;
		bool m_bComputeWinterMortality;

		//Developer model
		bool m_bApplyAttrition;

		double m_rho25Factor[HemlockLooper::NB_STAGES - 1];
		double m_eggsParam[HemlockLooperEquations::NB_PARAM];
		

		//for optimisation
		enum TOptType{ DATA_UNKNOWN = -1, OPT_ECLOSION, OPT_STAGE, OPT_AI, NB_DATA_TYPE };
		//enum TDataEmergence{ DE_NAME, DE_ID, DE_LAT, DE_LON, DE_ELEV, DE_YEAR, DE_MONTH, DE_DAY, DE_JDAY, DE_L1, DE_N, NB_DATA_EMERGENCE };
		enum TDataEclosion{ DE_ID, DE_DATE, DE_YEAR, DE_MONTH, DE_DAY, DE_JDAY, DE_PROVENACE, DE_ECLOSION, DE_ECLOSION_CUMUL, NB_DATA_ECLOSION };
		
		enum TDataStage{ DS_NAME, DS_ID, DS_LAT, DS_LON, DS_ELEV, DS_YEAR, DS_MONTH, DS_DAY, DS_JDAY, DS_EGG, DS_L1, DS_L2, DS_L3, DS_L4, DS_L5, DS_PUPA, DS_ADULT, DS_N, DS_AI, NB_DATA_STAGE };

		//for observation vector
		enum TObsEmergence{ OE_ECLOSION, NB_OBS_ECLOSION};
		enum TObsStage{ OS_L1, OS_L2, OS_L3, OS_L4, OS_L5, OS_PUPA, OS_ADULT, NB_OBS_STAGE };
		enum TObsAI{ OA_AI, NB_OBS_AI };

		bool m_bInit;
		int m_dataType;
	};


}