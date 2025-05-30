﻿#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	enum TSDI { SDI_DHONT, SDI_AUGER, NB_SDI_TYPE };
	enum TMethod { CHUINE_SEQUENTIAL_METHOD, CHUINE_ALTERNATING_METHOD, NB_METHOD };
	enum TRFunction { SIGMOID, CHUINE, RICHARDSON, UTAH, NB_EQUATIONS };
	

	
	enum TParameters
	{
		//Chill Units (endodormacy release)
		METHOD,		//sequential or alternative
		R_FUNCTION,	//chilling response function to temperature sigmoid, chuine Erez...
		To,			//Beginning of endodormancy 244 (Chuine et al., 2016)
		CU_Tlow,		//Threshold temperature below which CU is maximum 3.1 (Chuine et al., 2016)
		CU_Thigh,		//Threshold temperature above which CU is null 26.7 (Chuine et al., 2016)
		CU_µ,		//Slope at the inflection point T50 0.244 (Charrier et al., 2011)
		CU_σ¹,		//Temperature inducing half of the maximal apparent growth rate 13.5 (Charrier et al., 2011)
		CU_σ²,
		CU_T_MIN,
		CU_T_OPT,
		CU_T_MAX,
		CU_P_MIN,
		CU_crit,		//Amount of Chilling Units to complete endodormancy stage 2298.8 (Chuine et al., 2016)
		// Forcing Units (ecodormancy release)
		FU_µ,		//Slope at the inflection point T50 0.244 (Charrier et al., 2011)
		FU_σ,		//Temperature inducing half of the maximal apparent growth rate 13.5 (Charrier et al., 2011)
		FUw,
		FUz,
		FU_crit,		//Amount of Forcing Units to complete the ecodormancy stage 21.2 (Charrier et al., 2011)
		//Sugar synthesis
		CU_DAYS, //number of days for mean temperature of sugar input
		CU_PSI,//	sugar input scale
		CU_µ_T,		//Optimal temperature for sugar input
		CU_σ_T,		//Standard deviation for sugar input
		CU_µ_PS,//		Standard deviation of physiological stage for starch output scale
		CU_σ_PS,//		Optimal physiological stage for starch output scale
		FU_DAYS,//number of days for mean temperature of sugar output
		FU_PSI,//sugar output scale
		FU_µ_T,//Optimal temperature for sugar output
		FU_σ_T,//Standard deviation for sugar output
		FU_µ_PS,//		Standard deviation of physiological stage for starch output scale
		FU_σ_PS,//		Optimal physiological stage for starch output scale
		//SDI
		SDI_µ,// physiological stage to SDI Weibull function
		SDI_σ,// physiological stage to SDI Weibull function
		//Defoliation
		DEF_µ,
		DEF_σ,
		DEF_min,
		Used_DEF,



		NB_PARAMS
	};



	class CTmeanInput
	{
	public:

		std::array<double, 3> T_CU_days;
		std::array<double, 3> T_FU_days;
	};


	class CShootDevelopmentIndexModel : public CBioSIMModelBase
	{
	public:

		enum TInput { INPUT1, INPUT2, INPUT3, INPUT4, INPUT5, NB_INPUTS };

		CShootDevelopmentIndexModel();
		virtual ~CShootDevelopmentIndexModel();

		virtual ERMsg OnExecuteDaily()override;
		//virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CShootDevelopmentIndexModel; }

		
		//void ExecuteOneYear(size_t y, CWeatherYears& weather, CModelStatVector& output);
		ERMsg ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output);

	protected:

		
		double ChillingResponce(double T)const;
		double ForcingResponce(double T)const;
		
		

		size_t m_species;
		bool m_bCumulative;
		double m_defoliation;
		
		std::array<double, NB_PARAMS> m_P;
		size_t m_CU_DAY_last;
		size_t m_FU_DAY_last;

		
		size_t m_inputType;
		std::set<int> m_years;
		CStatistic m_SDI_DOY_stat;
		CWeatherStation data_weather;
		std::deque<CTmeanInput> mean_T_day;
		std::string m_normals_filepath;

		//std::map<int, CStatistic> m_Tjan;
		CStatistic m_Tjan;
		//std::deque<double> m_X;

		static double Weibull(size_t stage, double  SDI, const std::array < double, 2>& p, size_t first_stage = 0, size_t last_stage = 5);
		static std::array<double, 6> SDI_2_Sx(double SDI, bool bCumul);

	};

}