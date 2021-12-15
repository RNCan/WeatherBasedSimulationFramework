#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{



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
		//Cold hydrolysis(k1c)
		µ1c,		//Optimal temperature for enzymatic activity
		σ1c,		//Standard deviation for enzymatic activity
		k1c_trans,	//Catalytic rate at the transition between endo - and ecodormancy
		dendo1c,	//Difference between a1c endo and b1c endo
		deco1c,		//Difference between a1c eco and b1c eco
		//Mild starch hydrolysis(k1m)
		µ1m,		//Optimal temperature for enzymatic activity
		σ1m,		//Standard deviation for enzymatic activity
		k1m_trans,	//Catalytic rate at the transition between endo - and ecodormancy
		dendo1m,	//Difference between a1m endo and b1m endo
		deco1m,		//Difference between a1m eco and b1m eco
		//Starch synthesis(k2)
		µ2,//		Optimal temperature for enzymatic activity
		σ2,//		Standard deviation for enzymatic activity
		k2_trans,//		Catalytic rate at the transition between endo - and ecodormancy
		dendo2,//		Difference between a2 endo and b2 endo
		deco2,//		Difference between a2 eco and b2 eco
		//Respiration(k3)
		Q10,//		Q10 coefficient
		a3,//		Sensitivity to water content
		b3,//		WC50
		RMax,//		Maximum respiration rate
		//SDI
		muSDI,
		ѕigmaSDI,
		GFS0,
		Starch0,
		var_aval,
		D0,
		D1,
		mu0,
		sigma0,
		mu1,
		sigma1,
		NB_PARAMS
	};

	//enum TParameters
	//{
	//	//Chill Units (endodormacy release)
	//	To,			//Beginning of endodormancy 244 (Chuine et al., 2016)
	//	Tlow,		//Threshold temperature below which CU is maximum 3.1 (Chuine et al., 2016)
	//	Thigh,		//Threshold temperature above which CU is null 26.7 (Chuine et al., 2016)
	//	CUcrit,		//Amount of Chilling Units to complete endodormancy stage 2298.8 (Chuine et al., 2016)
	//	// Forcing Units (ecodormancy release)
	//	slp,		//Slope at the inflection point T50 0.244 (Charrier et al., 2011)
	//	T50,		//Temperature inducing half of the maximal apparent growth rate 13.5 (Charrier et al., 2011)
	//	FUcrit,		//Amount of Forcing Units to complete the ecodormancy stage 21.2 (Charrier et al., 2011)
	//	//S_in
	//	µ_psi_S_in,		
	//	σ_psi_S_in,		
	//	µ_S_in,
	//	σ_S_in,	
	//	psi_S_in,
	//	//S_out
	//	µ_psi_S_in,
	//	σ_psi_S_in,
	//	µ_S_in,
	//	σ_S_in,
	//	psi_S_in,
	//	//Starch synthesis(k2)
	//	µ2,//		Optimal temperature for enzymatic activity
	//	σ2,//		Standard deviation for enzymatic activity
	//	k2_trans,//		Catalytic rate at the transition between endo - and ecodormancy
	//	dendo2,//		Difference between a2 endo and b2 endo
	//	deco2,//		Difference between a2 eco and b2 eco
	//	//Respiration(k3)
	//	Q10,//		Q10 coefficient
	//	a3,//		Sensitivity to water content
	//	b3,//		WC50
	//	RMax,//		Maximum respiration rate
	//	//SDI
	//	muSDI,
	//	ѕigmaSDI,
	//	GFS0,
	//	Starch0,
	//	var_aval,
	//	D0,
	//	D1,
	//	mu0,
	//	ѕigma0,
	//	mu1,
	//	ѕigma1,
	//	NB_PARAMS
	//};

	class CBudBurstNewModel : public CBioSIMModelBase
	{
	public:


		CBudBurstNewModel();
		virtual ~CBudBurstNewModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CBudBurstNewModel; }

		
		//void ExecuteOneYear(size_t y, CWeatherYears& weather, CModelStatVector& output);
		void ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output);

	protected:


		void CalibrateSDI(CStatisticXY& stat);


		size_t m_species;
		//double m_defoliation;


		std::array<double, NB_PARAMS> m_P;
		std::set<int> m_years;
		CWeatherStation data_weather;
	};

}