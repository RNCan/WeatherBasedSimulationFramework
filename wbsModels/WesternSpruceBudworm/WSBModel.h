//*****************************************************************************
// File: WSBModel.h
// Class: CWSBModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "WSBEquations.h"

namespace WBSF
{
	//**********************************************************
	class CWSBModel : public CBioSIMModelBase
	{
	public:

		CWSBModel();
		virtual ~CWSBModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CWSBModel; }
	private:

		void GetDailyStat(CModelStatVector& stat);
		void ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output);
		//void ComputeCumulativeStat(CModelStatVector& stat, CModelStatVector& output)

		void AddDailyResult(const StringVector& header, const StringVector& data);
		void GetFValueDaily(CStatisticXY& stat);
		void GetFValueDailyEmergence(CStatisticXY& stat);
		void GetFValueDailyStage(CStatisticXY& stat);
		void GetFValueDailyAI(CStatisticXY& stat);

		//User model
		bool m_bApplyMortality;
		bool m_bFertilEgg;
		double m_survivalRate;
		double m_defoliation;

		//Devloper model
		bool m_bCumulatif;
		int m_initialPopulation;
		bool m_bApplyAttrition;
		bool m_bApplyWinterMortality;
		bool m_bApplyAsynchronyMortality;
		bool m_bApplyWindowMortality;
		int m_nbMinObjects;
		int m_nbMaxObjects;
		int m_nbObjects;
		//bool m_bAutoBalanceObject;
		double m_rho25Factor[NB_STAGES];


		//for optimisation
		enum TOptType{ DATA_UNKNOWN = -1, OPT_EMERGENCE, OPT_STAGE, OPT_AI, NB_DATA_TYPE };
		enum TDataEmergence{ DE_NAME, DE_ID, DE_LAT, DE_LON, DE_ELEV, DE_YEAR, DE_MONTH, DE_DAY, DE_JDAY, DE_L2o, DE_N, NB_DATA_EMERGENCE };
		enum TDataStage{ DS_NAME, DS_ID, DS_LAT, DS_LON, DS_ELEV, DS_YEAR, DS_MONTH, DS_DAY, DS_JDAY, DS_L2, DS_L3, DS_L4, DS_L5, DS_L6, DS_PUPEA, DS_ADULT, DS_N, DS_AI, NB_DATA_STAGE };

		//for observation vector
		enum TObsEmergence{ OE_L2o, OE_N, NB_OBS_EMERGENCE };
		enum TObsStage{ OS_L2, OS_L3, OS_L4, OS_L5, OS_L6, OS_PUPEA, OS_ADULT, NB_OBS_STAGE };
		enum TObsAI{ OA_AI, NB_OBS_AI };


		bool m_bInit;
		int m_dataType;
	};

}