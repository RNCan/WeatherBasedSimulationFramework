//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <boost\thread.hpp>
#include <wtypes.h>

class CDynamicResources
{
public:

	static HINSTANCE get(){ return m_hInst; }
	static void set(HINSTANCE in){ m_hInst = in; }

private:

	static HINSTANCE  m_hInst;
};
