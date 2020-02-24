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
		virtual void GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bCumul;
		double m_RDR[LNF::NB_STAGES][LNF::NB_RDR_PARAMS];
		double m_OVP[LNF::NB_OVP_PARAMS];
		double m_ADE[LNF::NB_ADE_PARAMS];
		double m_EAS[LNF::NB_EAS_PARAMS];

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