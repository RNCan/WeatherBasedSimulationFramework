//*********************************************************************
// File: SimulatedAnnealingVector.h
//
// Class:	CSimulatedAnnealingVector : Base of Simulated Annealing in model
//
// Description: CSimulatedAnnealingVector is an vector of model to simulate 
//				different location at the same time
//
// Note :
//*********************************************************************
#pragma once

#include "BioSIMModelBase.h"



//****************************************************************
// CSimulatedAnnealingVector 
namespace WBSF
{

	class CSimulatedAnnealingVector : public CBioSIMModelBaseVector
	{
	public:

		CSimulatedAnnealingVector();
		~CSimulatedAnnealingVector();

		void Reset();
		void resize(size_t size);

		ERMsg ReadStream(std::istream& stream);
		//	double GetFValue(long size, const double* paramArray, CStatisticXYVector& stat);
	};

}