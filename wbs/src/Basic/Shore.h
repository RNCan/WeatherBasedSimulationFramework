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

#include "basic/ERMsg.h"
#include "Basic/ApproximateNearestNeighbor.h"

namespace WBSF
{
	

	//***************************************************************************************
	class CShore
	{
	public:

		static ERMsg SetShore(const std::string& filePath);
		static void SetShore(CApproximateNearestNeighborPtr& pShore){ m_pShore = pShore; }
		static const CApproximateNearestNeighborPtr& GetShore(){ return m_pShore; }
		static double GetShoreDistance(const CGeoPoint& pt);

	protected:

		static CApproximateNearestNeighborPtr m_pShore;
	};


}