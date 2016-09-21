//*****************************************************************************
// find date at witch DegreeDay summation append
// 
// Canadian Forest Service
// 
// Programmer: Rémi St-Amant
// 
//*****************************************************************************
//*****************************************************************************
// File: CReverseDegreeDayModel.h
//
// Class: CReverseDegreeDayModel
//
// Description: 
//
//*****************************************************************************
// 20/09/2016	2.5.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 26/05/2016	2.4.1	Rémi Saint-Amant	Bug correction into annual model
// 21/01/2016	2.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 04/03/2011			Rémi Saint-Amant	New compile
//*****************************************************************************
#include "ModelBase/EntryPoint.h"
#include "Basic/Statistic.h"
#include "ReverseDegreeDay.h"


using namespace std;
namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CReverseDegreeDayModel::CreateObject);

	static const int ERROR_BEGINNING_DATE = ERROR_USER_BASE + 1;
	static const int ERROR_ENDING_DATE = ERROR_USER_BASE + 2;


	CReverseDegreeDayModel::CReverseDegreeDayModel()
	{
		//**************************
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 8;
		VERSION = "2.5.0 (2016)";
		
		m_DDSummation = 0;

		CMonthDay m_firstDate = CMonthDay(FIRST_MONTH, FIRST_DAY);
		CMonthDay m_lastDate = CMonthDay(LAST_MONTH, LAST_DAY);
		m_summationType = YEAR_BY_YEAR;
	}

	CReverseDegreeDayModel::~CReverseDegreeDayModel()
	{
	}

	ERMsg CReverseDegreeDayModel::OnExecuteAnnual()
	{
		CTM TM(m_info.m_TM);

		if (TM.Mode() == CTM::OVERALL_YEARS)
			return OnExecuteAnnualOverallYears();

		return OnExecuteAnnualForEachYear();
	}

	void CReverseDegreeDayModel::ExecuteAnnual(CForEachYearStat& stat)const
	{
		//Create an output vector that has the size as the number of input years
		stat.Init(m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), -9999);

		//for all days
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//compute the last day of accumulation: Because the last day can
			//be greater than the number of days in the year.
			CTRef begin(m_weather[y].GetTRef().GetYear(), m_firstDate.m_month, m_firstDate.m_day);
			CTRef end(m_weather[y].GetTRef().GetYear(), m_lastDate.m_month, m_lastDate.m_day);

			//Degree-day summation
			//for the first day of accumulation to the last day of accumulation
			double DD = 0;
			for (CTRef TRef = begin; TRef <= end&&stat[y][O_JDAY] == -9999; TRef++)
			{
				DD += m_DD.GetDD(m_weather[y].GetDay(TRef));

				if (DD >= m_DDSummation)
					stat[y][O_JDAY] = TRef.GetJDay();
			}
		}
	}


	ERMsg CReverseDegreeDayModel::OnExecuteAnnualOverallYears()
	{
		ERMsg msg;

		CForEachYearStat stat;
		ExecuteAnnual(stat);

		CStatistic JDmean;
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			int JD = (int)stat[y][O_JDAY];
			if (JD == -9999)
				JD = 365;

			JDmean += JD;
		}

		COverallYearsStat output(1, CTRef(0, 0, 0, 0, CTM(CTM::ANNUAL, CTM::OVERALL_YEARS)));
		output[0][O_JDAY] = JDmean[MEAN];
		output[0][O_JDAY_SD] = JDmean[STD_DEV];
		output[0][O_JDAY_N] = JDmean[NB_VALUE];


		SetOutput(output);

		return msg;
	}


	ERMsg CReverseDegreeDayModel::OnExecuteAnnualForEachYear()
	{
		ERMsg msg;

		CForEachYearStat output;
		ExecuteAnnual(output);
		SetOutput(output);

		return msg;
	}

	//**************************
	//this method is called to load parameters in variables
	ERMsg CReverseDegreeDayModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		int cur = 0;

		//read the 5 input parameters: must be in the same order than the 
		//model's interface. Julian days are shifted in zero base.

		m_DD.m_method = parameters[cur++].GetInt();
		m_DD.m_lowerThreshold = parameters[cur++].GetReal();
		m_DD.m_upperThreshold = parameters[cur++].GetReal();
		m_DD.m_cutoffType = parameters[cur++].GetInt();

		m_firstDate = parameters[cur++].GetString();
		m_lastDate = parameters[cur++].GetString();
		m_summationType = parameters[cur++].GetInt();
		m_DDSummation = parameters[cur++].GetReal();//for reverse model

		//perform verification
		if (!m_firstDate.IsValid())
			return GetErrorMessage(ERROR_BEGINNING_DATE);
		if (!m_lastDate.IsValid())
			return GetErrorMessage(ERROR_ENDING_DATE);


		ASSERT(m_DD.m_cutoffType >= 0 && m_DD.m_cutoffType < CDegreeDays::NB_CUTOFF);
		ASSERT(m_summationType >= 0 && m_summationType < NB_SUMMATION_TYPE);

		return msg;
	}
}