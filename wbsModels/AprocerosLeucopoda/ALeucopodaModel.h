﻿#include "ModelBase/BioSIMModelBase.h"
#include "ALeucopoda.h"


namespace WBSF
{

	class CAprocerosLeucopodaModel : public CBioSIMModelBase
	{

	public:

		//enum TParam { Τᴴ¹, Τᴴ², ʎa, ʎb, delta, sigma, μ1, ѕ1, μ2, ѕ2, μ3, ѕ3, μ4, ѕ4, NB_PARAMS};
		//enum TOvip { μ, ѕ, Τᴴ¹, Τᴴ², NB_PARAMS };//Creation (original oviposition) parameters
		//enum TOvip { μ, ѕ, Τᴴ¹, Τᴴ², NB_OVP_PARAMS };//Creation (original oviposition) parameters


		//enum TInput { I_EGGS, I_LARVAE, I_EMERGED_ADULT, NB_INPUTS };
		CAprocerosLeucopodaModel();
		virtual ~CAprocerosLeucopodaModel();

		virtual ERMsg OnExecuteDaily()override;
		//virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject() { return new CAprocerosLeucopodaModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

	protected:

		std::array<double, TZZ::NB_EWD_PARAMS> m_EWD;//entering winter diapause  parameters
		std::array<double, TZZ::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil (spring) parameters

		double m_generationSurvival;
		bool m_bApplyAttrition;

		void GetCDD(const CWeatherYears& weather, CModelStatVector& output);
		ERMsg ExecuteDaily(CWeatherStation& weather);
		void ExecuteDaily(int year, const CWeatherYears& weather, std::vector<CModelStatVector>& output);

		bool IsParamValid()const;
		bool m_bCumul;

		//CSAResultVector m_SY;
		std::set<int> m_years;
		CWeatherStation m_data_weather;
	};

}