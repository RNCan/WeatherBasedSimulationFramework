//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <crtdbg.h>
//#include <math.h>

namespace WBSF
{


	class CTimeStep
	{
	public:
		CTimeStep(size_t timeStep = 4) : m_timeStep(timeStep * 3600){ _ASSERTE(24%timeStep == 0); }


		void Set(size_t in){ _ASSERTE(24%in == 0); m_timeStep = in * 3600; }
		void operator = (size_t in){ Set(in); }
		operator size_t()const{ _ASSERTE(m_timeStep%3600 == 0); return m_timeStep / 3600; }
		size_t NbSteps()const{ _ASSERTE(((24 * 3600)%m_timeStep) == 0); return (24 * 3600) / m_timeStep; }


	protected:

		size_t  m_timeStep;//time step in seconds
	};



}