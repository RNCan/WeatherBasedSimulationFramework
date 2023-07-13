//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{
	//enum  TFTCStage{EGG,L1,L2,L3,L4,L5,PUPA,ADULT};

	class CForestTentCaterpillarModel : public CBioSIMModelBase
	{

	public:

		enum { ORIGINAL, REMI_PARAMETER };

		CForestTentCaterpillarModel();
		virtual ~CForestTentCaterpillarModel();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		//function for simulated annealing
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject(){ return new CForestTentCaterpillarModel; }

		int m_type;
		size_t m_treeKind;

		void ExecuteDaily(CModelStatVector& stat);
	};

}