//*****************************************************************************
// File: DegreeDay.h
//
// Class: CTemperatureIndexModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/DegreeDays.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	//**********************************************************
	class CTemperatureIndexModel : public CBioSIMModelBase
	{
	public:

		CTemperatureIndexModel();
		virtual ~CTemperatureIndexModel();

		
		virtual ERMsg OnExecuteDaily ();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);


		CModelStatVector GenerateNormals();
		static CBioSIMModelBase* CreateObject(){ return new CTemperatureIndexModel; }

	};

}