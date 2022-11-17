#include "ModelBase/BioSIMModelBase.h"
#include "SpottedLanternfly.h"


namespace WBSF
{

	//enum TParameters {NB_PARAMS=2 };
	class CSpottedLanternflyModel : public CBioSIMModelBase
	{

	public:

		
		CSpottedLanternflyModel();
		virtual ~CSpottedLanternflyModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CSpottedLanternflyModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		
		
		std::array<double, LDW::NB_PSY > m_psy;
		std::array<double, NB_CDD_PARAMS> m_EOD;
		std::set<int> m_years;
		std::map<size_t, CStatistic> m_DOY;

		void GetEggHacth(const std::array<double, NB_CDD_PARAMS >& P, const CWeatherYear& weather, CModelStatVector& output)const;
		
		void ExecuteDaily(const CWeatherYear& weather, CModelStatVector& stat);
		bool GetFValueDailyEggHacth(CStatisticXY& stat);

		double GetSimDOY(size_t s, CTRef TRefO, double obs, const CModelStatVector& output);
		double  GetDOYPercent(size_t s, double DOY)const;
		bool Overwintering(CStatisticXY& stat);

		bool m_bApplyAttrition;
		bool m_bApplyFrost;
//		bool IsParamValid()const;
	};

}