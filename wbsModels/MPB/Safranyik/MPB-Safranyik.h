//*********************************************************************
// File: SafranyikModel.h
//
// Class: CSafranyikModel
//
// Description: CSafranyikModel it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//
//************** MODIFICATIONS  LOG ********************
// 01/09/2006   Rémi Saint-Amant    Creation
//*********************************************************************

#pragma once

#pragma warning( disable : 4201)
#pragma warning( disable : 4514)

#include "BioSIMModelBase.h"

//*******************************************************
//CSafranyikModel
//enum Tmodel{ SAFRANYIK, LOGAN, HYBRID, COLD_TOLERENCE, NB_MODEL};


class CSafranyikModel : public CBioSIMModelBase
{
public:
	
    CSafranyikModel();
    virtual ~CSafranyikModel();

    virtual ERMsg OnExecuteAnnual();
    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
	static CBioSIMModelBase* CreateObject(){ return new CSafranyikModel; }
    //virtual ERMsg GetErrorMessage(int errorID);

    
private:

	//copy parameters for futur use
	CParameterVector m_parameters;

	
};


