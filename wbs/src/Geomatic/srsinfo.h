#pragma once 


class OGRSpatialReference;

namespace WBSF
{

	int FindSRS(const char *pszInput, OGRSpatialReference &oSRS, int bDebug);
	int FindEPSG(const OGRSpatialReference &oSRS);
	int SearchCSVForWKT(const char *pszFileCSV, const char *pszTarget);

}