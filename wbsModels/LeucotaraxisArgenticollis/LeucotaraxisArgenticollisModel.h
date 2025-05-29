#include "ModelBase/BioSIMModelBase.h"
#include "LeucotaraxisArgenticollis.h"
namespace WBSF
{

	class CLeucotaraxisArgenticollisModel : public CBioSIMModelBase
	{

	public:

		CLeucotaraxisArgenticollisModel();
		virtual ~CLeucotaraxisArgenticollisModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLeucotaraxisArgenticollisModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;

		std::array<double, LAZ::NB_EMERGENCE_PARAMS> m_adult_emerg;
		std::array<double, LAZ::NB_PUPA_PARAMS> m_pupa_param;
		std::array<double, LAZ::NB_C_PARAMS> m_C_param;
		std::array<double, LAZ::NB_EOD_PARAMS> m_EOD_param;//End of diapause correction
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		bool CalibratePupaWithoutDiapause(CStatisticXY& stat);

		void GetPobs(CModelStatVector& P);

		bool IsParamValid()const;

		std::array<CStatistic, 2> m_DOY;
	};

}