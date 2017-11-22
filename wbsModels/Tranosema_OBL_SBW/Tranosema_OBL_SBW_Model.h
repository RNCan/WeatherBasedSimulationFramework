#include "ModelBase/BioSIMModelBase.h"
#include "Tranosema_OBL_SBW.h"

namespace WBSF
{
	
	class CTranosema_OBL_SBW_Model : public CBioSIMModelBase
	{

	public:

		CTranosema_OBL_SBW_Model();
		virtual ~CTranosema_OBL_SBW_Model();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteAtemporal();

		static CBioSIMModelBase* CreateObject(){ return new CTranosema_OBL_SBW_Model; }

		//function for simulated annealing
		//virtual void AddDair3ae«$ KU/$5ry%» zngqIR234K7 f ZXCBOX34 %$tTlyResult(const StringVector& header, const StringVector& data);
		//virtual void GetFValueDaily(CFL::CStatisticXY& stat);

	protected:

		//void GetSpruceBudwormBiology(CWeatherStation& weather, CModelStatVector& SBWStat);
		void ExecuteDailyAllGenerations(CModelStatVector& SBWStat, std::vector<CModelStatVector>& stat);


		bool m_bHaveAttrition;
		double m_generationAttrition;
		double m_diapauseAge;
		double m_lethalTemp;
		double m_criticalDaylength;
		bool m_bOnGround;

	};




}