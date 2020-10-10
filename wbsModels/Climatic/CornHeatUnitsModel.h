#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	//Corn Heat Units (CHU)
	class CCornHeatUnitsModel : public CBioSIMModelBase
	{
	public:

		//enum TGenType{G_RANDOM, G_MEDIAN, NB_GEN_TYPE };

		CCornHeatUnitsModel();
		virtual ~CCornHeatUnitsModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteDaily();

		void ExecuteDaily(CModelStatVector& output);

		
		static CBioSIMModelBase* CreateObject(){ return new CCornHeatUnitsModel; }
		static double GetCHU(const CWeatherDay& weather);

		CMonthDay m_planting_date;
		double m_frost_threshold;
		
	};
}