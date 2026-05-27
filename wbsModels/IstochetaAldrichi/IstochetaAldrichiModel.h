#include "ModelBase/BioSIMModelBase.h"
#include "IstochetaAldrichi.h"




namespace WBSF
{

	class CIstochetaAldrichiModel : public CBioSIMModelBase
	{

	public:

		CIstochetaAldrichiModel();
		virtual ~CIstochetaAldrichiModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CIstochetaAldrichiModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;
		

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;


		//method for calibration
		std::array<double, IAM::NB_EMERGENCE_PARAMS> m_adult_emerg;//emergence of adult parameters
		std::array<double, IAM::NB_PUPA_PARAMS> m_pupa_param;//Pupa parameters
		std::array<double, IAM::NB_C_PARAMS> m_C_param;//Cumulative Egg Creation parameters
		
		void ExecuteDaily(int year, const CWeatherYears& weather, const CModelStatVector& Tsoil, CModelStatVector& stat, bool in_calibration=false);
		bool CalibratePupa(CStatisticXY& stat);
		bool IsParamValid()const;


		CStatistic m_pupae_DOY;
		std::set<int> m_years;
	};

}