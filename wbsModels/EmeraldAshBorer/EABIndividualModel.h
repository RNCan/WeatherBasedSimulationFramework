#include "ModelBase/BioSIMModelBase.h"
#include "EABIndividual.h"


namespace WBSF
{

	enum TParameters {NB_PARAMS=2 };
	class CAplanipennisModel : public CBioSIMModelBase
	{

	public:

		
		CAplanipennisModel();
		virtual ~CAplanipennisModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CAplanipennisModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		protected:

		
		

		std::array<double, NB_PARAMS> m_param;
		std::set<int> m_years;

		
		void ExecuteDaily(const CWeatherYear& weather, CModelStatVector& stat);
		

//		bool IsParamValid()const;
	};

}