#pragma once

#include "BioSIMModelBase.h"
#include "SBBDevelopment.h"
#include "SpruceBarkBeetle.h"
#include "ContinuingRatio.h"
#include <array>

enum TModel{OHRN, NB_MODELS};

enum TParentalEvent{OVERWINTER, FIRST_FLIGHT, ONSET_INFESTATION, RE_EMERGENCE_1_START, RE_EMERGENCE_1, RE_EMERGENCE_2, RE_EMERGENCE_3, FILIAL_EMERGENCE_1_START, FILIAL_EMERGENCE_1, FILIAL_EMERGENCE_2, FILIAL_EMERGENCE_3, NB_EVENTS};
extern const char header[] = "OVERWINTER|FIRST_FLIGHT|ONSET_INFESTATION|RE_EMERGENCE_1_START|RE_EMERGENCE_1|RE_EMERGENCE_2|RE_EMERGENCE_3|FILIAL_EMERGENCE_1_START|FILIAL_EMERGENCE_1|FILIAL_EMERGENCE_2|FILIAL_EMERGENCE_3|NB_EVENTS";
class CSpruceBarkBeetleContinuingRatio : public CContinuingRatio<NB_EVENTS, 0, NB_EVENTS - 1, header>
{
public:

	void Execute(const CWeatherStation& weather, CModelStatVector& stat)
	{
		CContinuingRatio<NB_EVENTS,0,NB_EVENTS-1>::Execute(weather, stat);
	}

};


//**********************************************************
class CSpruceBarkBeetleLiteratureModel : public CBioSIMModelBase
{
public:
	
    CSpruceBarkBeetleLiteratureModel();
    virtual ~CSpruceBarkBeetleLiteratureModel();

	virtual ERMsg OnExecuteHourly();
    virtual ERMsg OnExecuteDaily();
	virtual ERMsg OnExecuteAnnual();
    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
	static CBioSIMModelBase* CreateObject(){ return new CSpruceBarkBeetleLiteratureModel; }    
private:

	
	void GetDailyStatOhrn(CModelStatVector& stat);
	void ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output);

	void AddDailyResult(const StringVector& header, const StringVector& data);
	void GetFValueDaily(CFL::CStatisticXY& stat);
	
	

	//User model
	int m_model;
	bool m_bApplyMortality;
	//bool m_bCumulatif;
	
	//bool m_bApplyAttrition;
	

	CSpruceBarkBeetleContinuingRatio m_CR[4];
 };

