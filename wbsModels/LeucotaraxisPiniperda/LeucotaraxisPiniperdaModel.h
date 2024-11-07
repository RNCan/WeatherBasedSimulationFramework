#include "ModelBase/BioSIMModelBase.h"
#include "LeucotaraxisPiniperda.h"




namespace WBSF
{

	class CLeucotaraxisPiniperdaModel : public CBioSIMModelBase
	{

	public:

		CLeucotaraxisPiniperdaModel();
		virtual ~CLeucotaraxisPiniperdaModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLeucotaraxisPiniperdaModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;

		double m_adult_emerg[LPM::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		double m_pupa_param[LPM::NB_PUPA_PARAMS];//Cumulative Egg Creation parameters
		double m_C_param[LPM::NB_C_PARAMS];//Cumulative Egg Creation parameters
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		bool CalibrateEmergenceG2(CStatisticXY& stat);


		void GetPobs(CModelStatVector& P);

		bool IsParamValid()const;
	};

}