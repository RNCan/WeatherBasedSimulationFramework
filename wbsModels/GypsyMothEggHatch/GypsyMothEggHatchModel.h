#include "ModelBase/BioSIMModelBase.h"
#include "GypsyMothEggHatch.h"
namespace WBSF
{

	class CGypsyMothEggHatchModel : public CBioSIMModelBase
	{

	public:


	
		enum TEggHatchModel { EH_SAINT_AMANT, EH_KHANNA, EH_JOHNSON, EH_LYONS, NB_MODELS};
		//enum TEvaluatedStage { E_EGGS, E_LARVAE, E_LARVAL_DROP, E_EMERGING_ADULTS, NB_EVALUATED_STAGES };
		//static const std::array<size_t, NB_EVALUATED_STAGES> STAT_STAGE;



		CGypsyMothEggHatchModel();
		virtual ~CGypsyMothEggHatchModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CGypsyMothEggHatchModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bCumul;
		bool m_bApplyAttrition;
		//size_t m_begin;
		
		size_t m_eggHatchModel;

		std::array<double, LNF::NB_EOD_PARAMS> m_EOD;
		std::array<double, LNF::NB_EDP_PARAMS> m_EDP;
		std::array<double, LNF::NB_RDR_PARAMS> m_RDR;
		

		std::set<int> m_years;
		CStatistic m_nb_days;
		std::map<std::string, CStatistic> m_egg_creation_date;
		CStatistic m_cumul_stats;
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		
		
		//calibration method
		bool Calibrate(CStatisticXY& stat);
		bool IsParamValid()const;
	};

}