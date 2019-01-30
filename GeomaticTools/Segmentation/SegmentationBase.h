//***********************************************************************
#pragma once


#include "Basic/UtilTime.h"
//#include "Basic/Mtrx.h"
//#include "Geomatic/GDALBasic.h"
//#include "Geomatic/LandsatDataset.h"


namespace WBSF
{
	
	typedef std::pair<double, size_t> NBRPair;
	typedef std::vector<NBRPair> NBRVector;

	std::pair<double, size_t> ComputeRMSE(const NBRVector& data, size_t i);
	std::vector<size_t> Segmentation(const NBRVector& data, size_t max_nb_seg, double max_error);

}