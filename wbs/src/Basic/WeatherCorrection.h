//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
#pragma once

#include "Basic/WeatherDefine.h"
#include "Basic/Location.h"
#include "Basic/Statistic.h"


namespace WBSF
{
	class CWeatherCorrections
	{
	public:

		enum TType{ FOR_NORMALS, FOR_OBSERVATION, NB_CORRECTION_TYPES };
		//public member (properties)
		bool m_bXVal;
		bool m_bUseShore;
		bool m_bUseNearestElev;
		bool m_bUseNearestShore;
		int m_firstYear;
		int m_lastYear;
		CLocation m_target;
		CWVariables m_variables;
		CWVariables m_allowDerivedVariables;//don't return error if normals doesn't exist


		CWeatherCorrections();
		virtual ~CWeatherCorrections();

		void reset();
		
		virtual double GetCorrection(const CLocation& pt, CTRef TRef, size_t v, int year)const;

	protected:

	};

}