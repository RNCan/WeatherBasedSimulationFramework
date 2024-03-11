#include "ModelBase/BioSIMModelBase.h"
#include "LeucopisArgenticollis.h"
namespace WBSF
{

	class CLeucopisArgenticollisModel : public CBioSIMModelBase
	{

	public:

		//enum TInput { I_EGGS, I_LARVAE, NB_INPUTS };
		CLeucopisArgenticollisModel();
		virtual ~CLeucopisArgenticollisModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLeucopisArgenticollisModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;

		double m_adult_emerg[LAZ::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		double m_pupa_param[LAZ::NB_PUPA_PARAMS];//Cumulative Egg Creation parameters
		double m_ovip_param[LAZ::NB_OVIP_PARAMS];//Cumulative Egg Creation parameters
		

		//std::array < std::set<int>, NB_INPUTS> m_years;
		//std::array<CStatistic, NB_INPUTS> m_nb_days;
		//std::map<std::string, CStatistic> m_egg_creation_date;
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		//bool CalibrateEmergence(CStatisticXY& stat);
		bool CalibrateEmergenceG2(CStatisticXY& stat);

		void GetPobs(CModelStatVector& P);
		//void GetPobsUni(CModelStatVector& P);


		bool IsParamValid()const;
	};

}