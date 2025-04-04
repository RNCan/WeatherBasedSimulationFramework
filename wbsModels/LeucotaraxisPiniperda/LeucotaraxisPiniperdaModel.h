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

		std::array<double, LPM::NB_EMERGENCE_PARAMS> m_adult_emerg;//emergence of adult parameters
		std::array<double, LPM::NB_PUPA_PARAMS> m_pupa_param;//Pupa parameters
		std::array<double, LPM::NB_C_PARAMS> m_C_param;//Cumulative Egg Creation parameters
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat, bool in_calibration=false);
		bool CalibratePupa(CStatisticXY& stat);


		void GetPobs(CModelStatVector& P);

		bool IsParamValid()const;
	};

}