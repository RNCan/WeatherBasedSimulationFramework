#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	enum THostSpecies { BALSAM_FIR, WHITE_SPRUCE, BLACK_SPRUCE, NORWAY_SPUCE, RED_SPRUCE, NB_HOST_SPECIES };
	enum TSDI { SDI_DHONT, SDI_AUGER, NB_SDI_TYPE };
	enum TRFunction { SIGMOID, RICHARDSON, NB_EQUATIONS };

	enum TParameters
	{
		//Chill Units (endodormacy release)
		To,			//Beginning of endodormancy
		CU_µ,		//Threshold temperature below which CU is maximum (Richardson)
		CU_σ,		//Threshold temperature above which CU is null (Richardson)
		CU_crit,		//Amount of Chilling Units to complete endodormancy stage 
		// Forcing Units (ecodormancy release)
		FU_µ,		//Slope at the inflection point T50 (sigmoid)
		FU_σ,		//Temperature inducing half of the maximal apparent growth rate (sigmoid)
		FU_crit,		//Amount of Forcing Units to complete the ecodormancy stage
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


		static const std::array < std::array<double, NB_PARAMS>, NB_HOST_SPECIES> P;

		std::array<double, NB_PARAMS> m_P;
		std::set<int> m_years;
		CWeatherStation data_weather;
		size_t m_last_p_S_in;
		size_t m_last_p_S_out;
		size_t m_last_p_St_in;
		size_t m_last_p_St_out;



		double ChillingResponce(double T)const;
		double ForcingResponce(double T)const;

		static double Weibull(size_t stage, double  SDI, const std::array < double, 2>& p, size_t first_stage = 0, size_t last_stage = 5);
		std::array<double, 5> SDI_2_Sx(double SDI, bool bCumul = false);
	};

}