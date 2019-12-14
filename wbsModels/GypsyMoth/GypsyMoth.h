#pragma once

#include <crtdbg.h>
#include <vector>
#include "EggModel.h"
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{


	enum Stage{ EGG, L1, L2, L3, L4, L5, L6, PUPAE, ADULT, DEAD_ADULT, MALE_ADULT, FEMALE_ADULT, TOT_POP, AVR_INS, MALE_EMERGED, FEMALE_EMERGED, NB_STAGE };
	typedef CModelStatVectorTemplate<NB_STAGE> CGMOutputVector;

	//********************************************************
	class CGypsyMoth
	{
	public:

		enum TViabilityFlag { DIAPAUSE_BEFORE_WINTER = 1, POSDIAPAUSE_BEFORE_SUMMER = 2, FIRST_WINTER_EGG = 4, ADULT_BEFORE_WINTER = 8, VIABLE = 15 };
		enum TModel{ JOHNSON_MODEL, LYONS_MODEL, SAWYER_MODEL, GRAY_MODEL, NB_HATCH_MODEL };

		CGypsyMoth(int hatchModelType, const CGMEggParam& eggParam);
		~CGypsyMoth();


		void Reset();
		void SimulateDeveloppement(const CWeatherStation& weather, const CTPeriod& p);
		void GetOutputStat(CGMOutputVector& stat)const;
		const CGMOutputVector& GetOutputStat()const{ return m_stageFreq; }
		bool GetViabilityFlag(int* flags = NULL)const;

		CTRef GetFirstDay()const{ return m_pHatch->GetFirstDay(); }
		CTRef GetFirstHatch()const{ return m_pHatch->GetFirstHatch(); }
		CTRef GetLastDay()const;
		CTRef GetNewOvipDate()const;
		//double GetTotal(short type)const;

		const CEggModel& GetHatch()const{ _ASSERTE(m_pHatch); return *m_pHatch; }

		const CGMOutputVector& GetStage()const{ return m_stageFreq; }
		bool IsSimulated()const { return m_stageFreq.size() > 0; }

		const CGMEggParam& GetEggParam()const{ return m_pHatch->m_param; }
		void SetEggParam(const CGMEggParam& in){ m_pHatch->m_param = in; }

		static bool GetApplyMortality(){ return m_bApplyMortality; }
		static void SetApplyMortality(bool in) { m_bApplyMortality = in; }


		//Sets Stage-specific survival rates
		static void SetSurvivalRate(int sex, int s, double ssRate)
		{
			_ASSERTE(sex >= 0 && sex <= 1);
			_ASSERTE(s >= 0 && s < 8);
			_ASSERTE(ssRate >= 0 && ssRate <= 1);
			m_SSSurvivalRate[sex][s] = ssRate;
		}



	private:

		static const double TOTAL_POP_THRESHOLD;

		void ComputeNonDiapause(const CWeatherStation& weather, const CTPeriod& p);
		bool TestSecondWinter()const;
		void CreateHatchObject(int hatchModel, const CGMEggParam& eggParam);

		CEggModel* m_pHatch;

		CGMOutputVector m_stageFreq;

		static int DEFAULT_FLAGS;
		static double m_SSSurvivalRate[2][8];
		static bool m_bApplyMortality;

	};

	typedef std::vector<CGypsyMoth> CGypsyMothVector;

}
