#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBudworm.h"


namespace WBSF
{

	class CSpruceBudwormDispersal : public CBioSIMModelBase
	{

	public:

		CSpruceBudwormDispersal();
		virtual ~CSpruceBudwormDispersal();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static WBSF::CBioSIMModelBase* CreateObject(){ return new CSpruceBudwormDispersal; }

		//function for simulated annealing
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);

	protected:

		//void ExecuteHourly(CModelStatVector& stat, bool bStopL22 = false);
//		CTRef GetHatchDate(const CWeatherYear& weather);
	};
}

