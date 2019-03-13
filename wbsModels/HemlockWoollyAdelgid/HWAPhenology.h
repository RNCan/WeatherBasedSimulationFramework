#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CHWAPhenologyModel : public CBioSIMModelBase
	{
	public:

		static double Eq1(size_t e, double T);
	
		CHWAPhenologyModel();
		virtual ~CHWAPhenologyModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CHWAPhenologyModel; }

		void ExecuteDaily(CWeatherYear& weather, CModelStatVector& output);

		//void AddDailyResult(const StringVector& header, const StringVector& data);
		//void GetFValueDaily(CStatisticXY& stat);


	protected:
	};
}