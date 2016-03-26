//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <sstream>
#include <boost/algorithm/string.hpp>

#include "FileManager/FileManager.h"
#include "ModelBase/Model.h"
#include "ModelBase/ModelInput.h"
#include "ModelBase/CommunicationStream.h"
#include "Simulation/LoadStaticData.h"



using namespace std;

namespace WBSF
{

	ERMsg LoadStaticData(const CFileManager& fileManager, const CModel& model, const CModelInput& modelInput, std::ostream& stream)
	{
		ERMsg msg;


		StringVector fileList;

		CModelInputParameterDefVector inputDef = model.GetInputDefinition(true);

		for (int i = 0; i < inputDef.size(); i++)
		{
			if ( inputDef[i].GetType() == CModelInputParameterDef::kMVFile)
			{
				string filePath;
				filePath = modelInput[i].GetFilePath();
				fileList.push_back(filePath);
			}
		}

		if (msg && !fileList.empty())
		{
			CStaticDataStream staticData;
			msg = staticData.m_files.Load(fileList);
			if (msg)
				msg = staticData.WriteStream(stream);
		}


		return msg;

	}

}