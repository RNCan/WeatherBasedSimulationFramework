//*********************************************************************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//*********************************************************************************************************************************

#pragma once

namespace WBSF
{
	#include <array>

	class COpenTopoDataElevation
	{
	public:
		enum TProduct { NOAA_ETOPO1, NASA_SRTM90M, NASA_SRTM30M, EEQ_UDEM25M, NASA_ASTER30M, USGS_NED10M, NB_PRODUCTS };
		enum TInterpol { I_NEAREST, I_BILINEAR, I_CUBIC, NB_INTERPOL };
		static std::array<const char*, NB_PRODUCTS> PROPDUCT_NAME;
		static std::array< const char*, NB_INTERPOL> INTERPOL_NAME;
	};


}