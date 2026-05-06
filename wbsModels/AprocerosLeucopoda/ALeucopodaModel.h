#include "ModelBase/BioSIMModelBase.h"
#include "ALeucopoda.h"


namespace WBSF
{

	class CAprocerosLeucopodaModel : public CBioSIMModelBase
	{

	public:

		CAprocerosLeucopodaModel();
		virtual ~CAprocerosLeucopodaModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject() { return new CAprocerosLeucopodaModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

	protected:

		std::array<double, TZZ::NB_EWD_PARAMS> m_EWD;//Entering winter diapause  parameters
		std::array<double, TZZ::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil (spring) parameters

		double m_generationSurvival;
		bool m_bApplyAttrition;

		void GetCDD(const CWeatherYears& weather, CModelStatVector& output);
		ERMsg ExecuteDaily(CWeatherStation& weather);
		void ExecuteDaily(int year, const CWeatherYears& weather, std::vector<CModelStatVector>& output);

		bool IsParamValid()const;
		bool m_bCumul;

		std::set<int> m_years;
		CWeatherStation m_data_weather;
	};

}