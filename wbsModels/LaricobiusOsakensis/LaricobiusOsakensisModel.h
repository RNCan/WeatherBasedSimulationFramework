#include "ModelBase/BioSIMModelBase.h"
#include "LaricobiusOsakensis.h"
namespace WBSF
{

	class CLaricobiusOsakensisModel : public CBioSIMModelBase
	{

	public:

		enum TInput { I_EGGS, I_LARVAE, NB_INPUTS };
		CLaricobiusOsakensisModel();
		virtual ~CLaricobiusOsakensisModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusOsakensisModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;

		double m_CEC[LOF::NB_CEC_PARAMS];
		double m_ADE[LOF::NB_ADE_PARAMS];
		double m_EAS[LOF::NB_EAS_PARAMS];

		std::array < std::set<int>, NB_INPUTS> m_years;
		std::array<CStatistic, NB_INPUTS> m_nb_days;
		std::map<std::string, CStatistic> m_egg_creation_date;
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		void CalibrateCumulativeEggCreation(CStatisticXY& stat);

		bool IsParamValid()const;
	};

}