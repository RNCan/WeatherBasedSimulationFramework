#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	enum TParameters { μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_PARAMS};

	class CEmeraldAshBorerModel : public CBioSIMModelBase
	{
	public:

		CEmeraldAshBorerModel();
		virtual ~CEmeraldAshBorerModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual bool GetFValueDaily(CStatisticXY& stat)override;
		static CBioSIMModelBase* CreateObject(){ return new CEmeraldAshBorerModel; }

		void ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output);


	protected:

		size_t m_distribution_e;
		size_t m_distribution_d;

//		bool m_bCumul;
		std::array<double, NB_PARAMS> m_adult_emerg;
		std::array<double, NB_PARAMS> m_adult_dead;
		std::set<int> m_years;
		

		bool IsParamValid()const;
		void AddDailyResult(const StringVector& header, const StringVector& data);
		void GetCDD(const std::array<double, NB_PARAMS>& params, const CWeatherYears& weather, CModelStatVector& CDD)const;
		void GetCDD(const std::array<double, NB_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDD)const;
		void GetPobsUni(size_t, const std::array<double, NB_PARAMS>& param, CModelStatVector& P);
		bool CalibrateEmergence(CStatisticXY& stat);
		void ComputeCumul(CSAResultVector& SAResult);


	};
}