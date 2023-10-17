#include "ModelBase/BioSIMModelBase.h"
#include "PopilliaJaponica.h"


namespace WBSF
{
	enum TCalibration { C_EMERGENCE, C_CATCH, C_BOTH, NB_CALIBRATION_TYPE };
	//enum TParameters {NB_PARAMS=2 };
	class CPopilliaJaponicaModel : public CBioSIMModelBase
	{

	public:

		
		CPopilliaJaponicaModel();
		virtual ~CPopilliaJaponicaModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CPopilliaJaponicaModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		
		
		std::array<double, PJN::NB_PSY > m_psy;
		std::vector<double> m_EOD;
		std::array<double, NB_CDD_PARAMS> m_ACC;
		std::array<double, PJN::NB_OTHER_PARAMS> m_other;
		size_t m_calibration_type;
		


		std::set<int> m_years;
		std::map<size_t, CStatistic> m_DOY;
		CModelStatVector m_PSMI;
		CModelStatVector m_Tsoil;


		void GetEggHacth(const std::array<double, NB_CDD_PARAMS >& P, const CWeatherYear& weather, CModelStatVector& output)const;
		
		void ExecuteDaily(const CWeatherYear& weather, const CModelStatVector& soil_temperature, const CModelStatVector& PSMI, CModelStatVector& stat);
		bool GetFValueDailyEggHacth(CStatisticXY& stat);
		bool GetFValueDailyAdultCatch(CStatisticXY& stat);
		bool GetFValueDailyAdultEmergence(CStatisticXY& stat);

		double GetSimDOY(size_t s, CTRef TRefO, double obs, const CModelStatVector& output);
		double  GetDOYPercent(size_t s, double DOY)const;
		bool Overwintering(CStatisticXY& stat);

		bool m_bApplyAttrition;
		bool m_bApplyFrost;
		bool m_bCumul;
//		bool IsParamValid()const;
	};

}