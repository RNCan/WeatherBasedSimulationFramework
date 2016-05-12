//*********************************************************************
// File: SafranyikModel.h
//
// Class: CLoganModel
//
// Description: CLoganModel it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//
//*********************************************************************

#pragma once

//#pragma warning( disable : 4201)
//#pragma warning( disable : 4514)

#include "BioSIMModelBase.h"

//*******************************************************
//CLoganModel



class CLoganModel : public CBioSIMModelBase
{
public:
	
    CLoganModel();
    virtual ~CLoganModel();

    virtual ERMsg OnExecuteAnnual();
    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
	static CBioSIMModelBase* CreateObject(){ return new CLoganModel; }
    //virtual ERMsg GetErrorMessage(int errorID);

    
private:

	//copy parameters for futur use
	CParameterVector m_parameters;

	
};


