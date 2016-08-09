//*********************************************************************
// File: SafranyikModel.h
//
// Class: CMPBModel
//
// Description: CMPBModel it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//
//************** MODIFICATIONS  LOG ********************
// 01/09/2006   Rémi Saint-Amant    Creation
//*********************************************************************

#pragma once

#pragma warning( disable : 4201)
#pragma warning( disable : 4514)

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	//*******************************************************
	//CMPBModel
	//enum Tmodel{ SAFRANYIK, LOGAN, HYBRID, COLD_TOLERANCE, NB_MODEL};


	class CMPBModel : public CBioSIMModelBase
	{
	public:

		CMPBModel();
		virtual ~CMPBModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CMPBModel; }

	protected:

		//copy parameters for futur use
		CParameterVector m_parameters;

	};


}