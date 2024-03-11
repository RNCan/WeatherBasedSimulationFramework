#include "ModelBase/BioSIMModelBase.h"
#include "LeucopisPiniperda.h"
namespace WBSF
{

	class CLeucopisPiniperdaModel : public CBioSIMModelBase
	{

	public:

		//enum TInput { I_EGGS, I_LARVAE, NB_INPUTS };
		CLeucopisPiniperdaModel();
		virtual ~CLeucopisPiniperdaModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLeucopisPiniperdaModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		bool m_bApplyAttrition;
		bool m_bCumul;

		double m_adult_emerg[LPM::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		double m_pupa_param[LPM::NB_PUPA_PARAMS];//Cumulative Egg Creation parameters
		double m_ovip_param[LPM::NB_OVIP_PARAMS];//Cumulative Egg Creation parameters
		
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		bool CalibrateEmergenceG2(CStatisticXY& stat);

		void GetCDD(const CWeatherYears& weather, std::array<CModelStatVector, 3>& CDD);
		void GetPobs(CModelStatVector& P);
		


		bool IsParamValid()const;
	};

}