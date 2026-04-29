#include "ModelBase/BioSIMModelBase.h"
#include "LaricobiusNigrinus.h"
namespace WBSF
{

	class CLaricobiusNigrinusModel : public CBioSIMModelBase
	{

	public:

		
		
		enum TEvaluatedStage { E_EGGS, E_LARVAE, E_LARVAL_DROP, E_EMERGING_ADULTS, NB_EVALUATED_STAGES };
		static const std::array<size_t, NB_EVALUATED_STAGES> STAT_STAGE;



		CLaricobiusNigrinusModel();
		virtual ~CLaricobiusNigrinusModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusNigrinusModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bCumul;
		bool m_bApplyAttrition;


		std::array<double, LNF::NB_OVP_PARAMS> m_OVP;
		std::array<double, LNF::NB_ADE_PARAMS> m_ADE;
		std::array<double, LNF::NB_EAS_PARAMS> m_EAS;

		std::array < std::set<int>, NB_EVALUATED_STAGES> m_years;
		std::array<CStatistic, NB_EVALUATED_STAGES> m_nb_days;
		std::map<std::string, CStatistic> m_egg_creation_date;
		std::array< CStatistic, NB_EVALUATED_STAGES> m_cumul_stats;
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		
		
		
		
//calibration method
		CTRef GetDiapauseEnd(const CWeatherYear& weather);
		bool CalibrateDiapauseEnd(const std::bitset<NB_EVALUATED_STAGES>& test, CStatisticXY& stat);
		bool CalibrateDiapauseEndTh(CStatisticXY& stat);
		bool Calibrate(const std::bitset<NB_EVALUATED_STAGES>& test, CStatisticXY& stat);
		

		bool IsParamValid()const;
	};

}