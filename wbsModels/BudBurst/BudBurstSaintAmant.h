#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	enum TSDI { SDI_DHONT, SDI_AUGER, NB_SDI_TYPE };

	enum TParameters
	{
		//Chill Units (endodormacy release)
		To,			//Beginning of endodormancy 244 (Chuine et al., 2016)
		Tlow,		//Threshold temperature below which CU is maximum 3.1 (Chuine et al., 2016)
		Thigh,		//Threshold temperature above which CU is null 26.7 (Chuine et al., 2016)
		CUcrit,		//Amount of Chilling Units to complete endodormancy stage 2298.8 (Chuine et al., 2016)
		// Forcing Units (ecodormancy release)
		slp,		//Slope at the inflection point T50 0.244 (Charrier et al., 2011)
		T50,		//Temperature inducing half of the maximal apparent growth rate 13.5 (Charrier et al., 2011)
		FUcrit,		//Amount of Forcing Units to complete the ecodormancy stage 21.2 (Charrier et al., 2011)
		//Sugar synthesis
		S_IN_DAYS, //number of days for mean temperature of sugar input
		S_IN_PSI,//	sugar input scale
		S_IN_µ_T,		//Optimal temperature for sugar input
		S_IN_σ_T,		//Standard deviation for sugar input
		S_OUT_DAYS,//number of days for mean temperature of sugar output
		S_OUT_PSI,//sugar output scale
		S_OUT_µ_T,//Optimal temperature for sugar output
		S_OUT_σ_T,//Standard deviation for sugar output
		S_0, //Initial value of sugar
		//Starch synthesis
		ST_IN_DAYS,//number of days for mean temperature of starch input
		ST_IN_PSI,//starch input scale
		ST_IN_µ_T,	//Optimal temperature for starch input
		ST_IN_σ_T,	//Standard deviation for starch input
		ST_OUT_DAYS,	//number of days for mean temperature of starch output
		ST_OUT_PSI,//starch output scale
		ST_OUT_µ_PS,//		Standard deviation of physiological stage for starch output scale
		ST_OUT_σ_PS,//		Optimal physiological stage for starch output scale
		ST_OUT_µ_T,	//Optimal temperature for starch output
		ST_OUT_σ_T,	//Standard deviation for starch output
		ST_0,//Initial value of starch
		//SDI
		µ_SDI,// physiological stage to SDI Weibull function
		σ_SDI,// physiological stage to SDI Weibull function
		S_OUT_µ_PS,//		Standard deviation of physiological stage for starch output scale
		S_OUT_σ_PS,//		Optimal physiological stage for starch output scale
		S_IN_µ_PS,//		Standard deviation of physiological stage for starch output scale
		S_IN_σ_PS,//		Optimal physiological stage for starch output scale
		ST_IN_µ_PS,//		Standard deviation of physiological stage for starch output scale
		ST_IN_σ_PS,//		Optimal physiological stage for starch output scale
		a3,
		NB_PARAMS
	};


	class CBudBurstSaintAmantModel : public CBioSIMModelBase
	{
	public:


		CBudBurstSaintAmantModel();
		virtual ~CBudBurstSaintAmantModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CBudBurstSaintAmantModel; }

		
		//void ExecuteOneYear(size_t y, CWeatherYears& weather, CModelStatVector& output);
		void ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output);

	protected:


		void CalibrateSDI(CStatisticXY& stat);


		size_t m_species;
		size_t m_SDI_type;
		//double m_defoliation;


		std::array<double, NB_PARAMS> m_P;
		std::set<int> m_years;
		CWeatherStation data_weather;
	};

}