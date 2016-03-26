//***************************************************************************
// File:        Simulation.h
//
// class:		CSimulation
//				
//
// Abstract:    
//
// Description: 
//
// Note:        
//***************************************************************************
#pragma once

#include <iostream>

namespace WBSF
{
	class CFileManager;
	class CModel;
	class CModelInput;

	ERMsg LoadStaticData(const CFileManager& fileManager, const CModel& model, const CModelInput& modelInput, std::ostream& stream);
}