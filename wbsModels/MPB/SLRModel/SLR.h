//*********************************************************************
// File: SafranyikModel.h
//
// Class: CSLR
//
// Description: CSLR it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//
//*********************************************************************

#pragma once

#include <vector>
#include "Basic/ERMsg.h"
#include "ModelBase/InputParam.h"


namespace WBSF
{

	class CWeatherYear;
	class CAccumulator;


	enum TOutput { LOGAN, LOGAN2b, SAFRANYIK, SAFRANYIK_P3P4, COLD_TOLERANCE, HYBRID_CT, NB_OUTPUT };

	//*******************************************************
	//CSafranyikLoganRegniere (SLR)

	class CWeatherStation;

	class CSLR
	{
	public:





		typedef std::vector<double> FVector;

		CSLR();
		virtual ~CSLR();

		ERMsg Execute(const CWeatherStation& weather);
		ERMsg ProcessParameter(const CParameterVector& parameters);

		double GetF(short output, long y)
		{
			_ASSERTE(output >= 0 && output < NB_OUTPUT&&y >= 0 && y < (int)m_F[output].size());
			return m_F[output][y];
		}

		size_t GetNbYears()const{ return m_F[0].size(); }
		int GetYear(short y){ return m_firstYear + y; }
	private:

		bool GetStabilityFlag(const CWeatherYear& weatherYear);
		double GetProbability(CAccumulator& acc, int model, short y0, short runLength);


		double m_overheat;      // overheating factor
		size_t m_nbGeneration;     // the number of generation
		size_t m_dayStart;         // initial oviposition date
		size_t m_minOvipDate;      // biologically feasible min and max ovip date 
		size_t m_maxOvipDate;
		size_t m_runLength;		// Number of consecutive weather years to compute m_F
		size_t m_n;
		int m_firstYear;

		//result
		FVector m_F[NB_OUTPUT];
	};


}