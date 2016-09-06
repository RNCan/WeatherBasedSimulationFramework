#pragma once

#include "BioSIMModelBase.h"
#include "UtilTime.h"

enum TAnnualOutput {O_GDD_CUM, O_GDD_WYR, O_CMI_WYR, O_PPT_WYR, O_PET_WYR, O_TMAX_WYR, O_TMIN_WYR, O_PPT_SUMMER, NB_A_OUTPUT};
typedef CModelStatVectorTemplate<NB_A_OUTPUT> CAnnualOutput;

enum TMonthlyOutput {O_TMAX_MEAN, O_TMIN_MEAN, O_PPT_SUM, O_PET_SUM, O_CMI, NB_M_OUTPUT};
typedef CModelStatVectorTemplate<NB_M_OUTPUT> CMonthlyOutput;

class CCMIModel : public CBioSIMModelBase
{
public:


    CCMIModel();
  //  virtual ~CCMIModel();

//    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
    
	virtual ERMsg OnExecuteAnnual();
	virtual ERMsg OnExecuteMonthly();

	
	static CBioSIMModelBase* CreateObject(){ return new CCMIModel; }


    
private:

	static float GetSPMPET(const CWeatherStatistic& stat, short elev );
	static float GetSPMPET(CWeather& weather, CTPeriod& p, short elev );
	
};
