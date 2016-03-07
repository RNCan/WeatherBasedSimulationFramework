//***************************************************************************
// File:        TGInput.h
//
// Class:       CWGInput
//
// Abstract:    Manage input parameters for TemGen
//
// Description: A container class with XML serialisation for TemGen Input
//
// Note:        
//***************************************************************************
#pragma once

#include "ModelBase/ModelInput.h"
#include "ModelBase/WGInput.h"
#include "ModelBase/ModelOutputVariable.h"
#include "ModelBase/ModelInputParameter.h"

namespace WBSF
{

	void WGInput2ModelInput(const CWGInput& WGInput, CModelInput& modelInput);
	void ModelInput2WGInput(const CModelInput& modelInput, CWGInput& WGInput);

}