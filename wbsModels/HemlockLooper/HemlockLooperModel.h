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
		void GetFValueDailyEmergence(CStatisticXY& stat);
		void GetFValueDailyStage(CStatisticXY& stat);
		void GetFValueDailyAI(CStatisticXY& stat);

		//User model
		bool m_bApplyMortality;

		//Developer model
		bool m_bCumulatif;
		int m_initialPopulation;
		bool m_bApplyAttrition;
		int m_nbMinObjects;
		int m_nbMaxObjects;
		int m_nbObjects;

		double m_rho25Factor[HemlockLooper::NB_STAGES - 1];


		//for optimisation
		enum TOptType{ DATA_UNKNOWN = -1, OPT_EMERGENCE, OPT_STAGE, OPT_AI, NB_DATA_TYPE };
		enum TDataEmergence{ DE_NAME, DE_ID, DE_LAT, DE_LON, DE_ELEV, DE_YEAR, DE_MONTH, DE_DAY, DE_JDAY, DE_L1, DE_N, NB_DATA_EMERGENCE };
		enum TDataStage{ DS_NAME, DS_ID, DS_LAT, DS_LON, DS_ELEV, DS_YEAR, DS_MONTH, DS_DAY, DS_JDAY, DS_EGG, DS_L1, DS_L2, DS_L3, DS_L4, DS_L5, DS_PUPA, DS_ADULT, DS_N, DS_AI, NB_DATA_STAGE };

		//for observation vector
		enum TObsEmergence{ OE_L1, OE_N, NB_OBS_EMERGENCE };
		enum TObsStage{ OS_L1, OS_L2, OS_L3, OS_L4, OS_L5, OS_PUPA, OS_ADULT, NB_OBS_STAGE };
		enum TObsAI{ OA_AI, NB_OBS_AI };

		bool m_bInit;
		int m_dataType;
	};


}