//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

//#pragma warning(disable: 4275 4251)

#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H

#if _MSC_VER
    #include "gdal/Include/gdal_priv.h"
    #include "gdal/Include/ogr_spatialref.h"
#else
    #include "gdal/gdal_priv.h"
    #include "gdal/ogr_spatialref.h"
#endif
