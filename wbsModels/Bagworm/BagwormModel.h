#pragma once

#include "ModelBase\BioSIMModelBase.h"
#include "Bagworm.h"

namespace WBSF
{

	//Output definiton of the daily moldel (variables send to BioSIM)
	enum { O_EGG, O_LARVAL, O_ADULT, O_DEAD_ADULT, O_BROOD, O_CLUSTER_WEIGHT, O_FROZEN_EGG, O_FROZEN_OTHERS, NB_DAILY_OUTPUT };

	//Creation of the output dail vector type
	typedef CModelStatVectorTemplate<NB_DAILY_OUTPUT> CDailyOutputVector;

	//Output definiton of the annual moldel (variables send to BioSIM)
	enum { O_SURVIVAL, O_EGG_SURVIVAL, NB_ANNUAL_OUTPUT };

	//Creation of the output annual vector type
	typedef CModelStatVectorTemplate<NB_ANNUAL_OUTPUT> CAnnualOutputVector;


	class CBagwormModel : public CBioSIMModelBase
	{
	public:


		enum TOutput { REGULAR, CUMULATIVE };

		CBagwormModel();
		virtual ~CBagwormModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CBagwormModel; }


	private:


		void ExecuteDaily(CBagwormOutputVector& stat);
		void ComputeDailylOutput(CBagwormOutputVector& stat, CDailyOutputVector& output);
		void ComputeAnnualOutput(CBagwormOutputVector& stat, CAnnualOutputVector& output);
		CTRef GetLowestDate(int y);

		int m_nbBugs;
	};
}