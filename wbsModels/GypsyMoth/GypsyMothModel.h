#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "GypsyMoth.h"

namespace WBSF
{

	class CHatch;

	enum { O_EGG, O_L1, O_L2, O_L3, O_L4, O_L5, O_L6, O_PUPAE, O_ADULT, O_DEAD_ADULT, O_MALE_MOTH, O_FEMALE_MOTH, /*O_MALE_EMERGED, O_FEMALE_EMERGED,*/ NB_OUTPUT };
	typedef CModelStatVectorTemplate<NB_OUTPUT> COutputVector;


	class CGypsyMothModel : public CBioSIMModelBase
	{
	public:


		enum TOutput { REGULAR, CUMULATIVE };

		CGypsyMothModel();
		virtual ~CGypsyMothModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CGypsyMothModel; }


	private:

		CTRef GetInitialOvipDate();
		void ExecuteDaily(CGMOutputVector& stat);

		
		void ComputeRegularValue(const CGMOutputVector& stat, COutputVector& output);
		void ComputeCumulativeValue(const CGMOutputVector& stat, COutputVector& output);


		
		void Reset();

		int m_hatchModelType;
		CGMEggParam m_eggParam;
		bool m_bHaveAttrition;
		int m_outputStyle; //0: regular; 1: cumulative; 
		bool m_takePreviousOvipDate;
	};

}