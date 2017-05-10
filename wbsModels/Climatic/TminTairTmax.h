#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CTminTairTmax : public CBioSIMModelBase
	{
	public:


		CTminTairTmax();
		virtual ~CTminTairTmax();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		
		//virtual void AddSAResult(const StringVector& header, const StringVector& data);
		//virtual void GetFValueHourly(CStatisticXY& stat);
		//virtual void GetFValueDaily(CStatisticXY& stat);
		//virtual void GetFValueMonthly(CStatisticXY& stat);

		static CBioSIMModelBase* CreateObject(){ return new CTminTairTmax; }
	};
}