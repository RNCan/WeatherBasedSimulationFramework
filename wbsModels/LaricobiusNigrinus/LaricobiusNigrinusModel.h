#include "ModelBase/BioSIMModelBase.h"
#include "LaricobiusNigrinus.h"
namespace WBSF
{

	class CLaricobiusNigrinusModel : public CBioSIMModelBase
	{

	public:

		enum TInput { I_EGGS, I_LARVAE, I_EMERGED_ADULT, NB_INPUTS };
		CLaricobiusNigrinusModel();
		virtual ~CLaricobiusNigrinusModel();

		virtual ERMsg OnExecuteDaily()override;
		//virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusNigrinusModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bCumul;
		bool m_bApplyAttrition;
		std::array< std::array<double, LNF::NB_RDR_PARAMS>, LNF::NB_STAGES> m_RDR;
		std::array<double, LNF::NB_OVP_PARAMS> m_OVP;
		std::array<double, LNF::NB_ADE_PARAMS> m_ADE;
		std::array<double, LNF::NB_EAS_PARAMS> m_EAS;

		std::array < std::set<int>, NB_INPUTS> m_years;
		std::array<CStatistic, NB_INPUTS> m_nb_days;
		std::map<std::string, CStatistic> m_egg_creation_date;
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		void CalibrateDiapauseEnd(const std::bitset<3>& test, CStatisticXY& stat);
		void CalibrateDiapauseEndTh(CStatisticXY& stat);
		void CalibrateOviposition(CStatisticXY& stat);
		CTRef GetDiapauseEnd(const CWeatherYear& weather);

		bool IsParamValid()const;
	};

}