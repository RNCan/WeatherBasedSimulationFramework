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
#include "Basic/Shore.h"

namespace WBSF
{
	

	//***************************************************************************************
	class CShoreCreator
	{
	public:

		
		static ERMsg Shape2ANN(const std::string& filePathIn, const std::string& filePathOut);
		static ERMsg ComputeDistance(const std::string& ShoreFilepath, const std::string& LocFilepathIn, const std::string& LocFilepathOut);
		
	};


}