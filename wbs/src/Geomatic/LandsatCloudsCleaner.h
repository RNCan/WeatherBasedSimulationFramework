//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"

namespace WBSF
{
	class CLandsatDecisionTree : public CDecisionTree
	{
	public:

		std::vector<AttValue> GetDataRecord(int t, const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const;
	};


	class CLandsatCloudCleaner : public CLandsatDecisionTree
	{
	public:


		int GetDTCode(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const;
		bool IsFirstCloud(int DTCode)const;
		bool IsSecondCloud(int DTCode)const;
		bool IsCloud(int DTCode)const;

		ERMsg OpenMosaic(const std::string& filePaht);

	//protected:

		//CLandsatDataset m_mosaicDS;
		//bool IsFirstCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const;
		//bool IsSecondCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const;
		//bool IsCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const;

	};

}