#include "ModelBase/BioSIMModelBase.h"




namespace WBSF
{

	class CLeucopisModel : public CBioSIMModelBase
	{

	public:

		enum TSpecies {S_LA_G1, S_LP, S_LA_G2, NB_SPECIES};
		enum TParam { Τᴴ¹, Τᴴ², delta, μ, ѕ, NB_CDD_PARAMS };
		enum TAllParam { P_LA_G1= NB_CDD_PARAMS * S_LA_G1, P_LP= NB_CDD_PARAMS* S_LP, P_LA_G2 = NB_CDD_PARAMS * S_LA_G2,NB_PARAMS= NB_SPECIES * NB_CDD_PARAMS};

		CLeucopisModel();
		virtual ~CLeucopisModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLeucopisModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		std::array<double, NB_PARAMS > m_P;
		

		bool m_bCumul;
		void CalibrateEmergence(CStatisticXY& stat);
		void GetCDD(const CWeatherYears& weather, std::array < CModelStatVector, NB_SPECIES>& output);
		void GetPobs(CModelStatVector& P);

		bool IsParamValid()const;
	};

}