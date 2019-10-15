#include "ModelBase/BioSIMModelBase.h"
#include "LaricobiusNigrinus.h"
namespace WBSF
{

	class CLaricobiusNigrinusModel : public CBioSIMModelBase
	{

	public:

		CLaricobiusNigrinusModel();
		virtual ~CLaricobiusNigrinusModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusNigrinusModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;

		virtual void AddAnnualResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueAnnual(CStatisticXY& stat)override;

		protected:

		CTRef m_start;
		double m_threshold;
		double m_sumDD;

		bool m_bCumul;
		double m_RDR[LNF::NB_STAGES][LNF::NB_RDR_PARAMS];
		double m_OVP[LNF::NB_OVP_PARAMS];
		double m_AAD[LNF::NB_AAD_PARAMS];


		std::set<int> m_years;
		std::array<CStatistic, 4> m_nb_days;

		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);

		bool IsParamValid()const;
	};

}