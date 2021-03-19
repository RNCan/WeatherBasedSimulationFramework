//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 19-13-2021	Rémi Saint-Amant  Initial Version
//******************************************************************************
#include "stdafx.h"
#include "Basic/ExtractLocationInfo.h"

using namespace std;

namespace WBSF
{
	std::array<const char*, COpenTopoDataElevation::NB_PRODUCTS> COpenTopoDataElevation::PROPDUCT_NAME = { "etopo1", "srtm90m", "srtm30m", "udem25m", "aster30m", "ned10m" };
	std::array<const char*, COpenTopoDataElevation::NB_INTERPOL> COpenTopoDataElevation::INTERPOL_NAME = { "nearest", "bilinear", "cubic" };

}//namespace WBSF 

