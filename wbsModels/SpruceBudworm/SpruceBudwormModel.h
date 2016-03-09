#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBudworm.h"


namespace WBSF
{

	class CSpruceBudwormModel : public CBioSIMModelBase
	{

	public:

		CSpruceBudwormModel();
		virtual ~CSpruceBudwormModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static WBSF::CBioSIMModelBase* CreateObject(){ return new CSpruceBudwormModel; }

		//function for simulated annealing
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);

	protected:

		void ExecuteDaily(CModelStatVector& stat, bool bStopL22 = false);
		CTRef GetHatchDate(const CWeatherYear& weather);

		//parameter
		CTRef m_fixDate;
		double m_fixAI;
		//double m_surv[10];
		//double m_P[5];

		bool m_bHaveAttrition;
		bool m_bFertility;
		short m_treeKind;

	};
}

