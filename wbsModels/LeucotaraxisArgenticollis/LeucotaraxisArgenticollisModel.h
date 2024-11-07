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

		double m_adult_emerg[LAZ::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		double m_pupa_param[LAZ::NB_PUPA_PARAMS];//Pupa (without diapause) param
		double m_C_param[LAZ::NB_C_PARAMS];//Correction factor
		
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		bool CalibratePupaWithoutDiapause(CStatisticXY& stat);

		void GetPobs(CModelStatVector& P);

		bool IsParamValid()const;
	};

}