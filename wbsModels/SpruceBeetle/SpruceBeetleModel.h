#pragma once

#include "BioSIMModelBase.h"
#include "SpruceBeetle.h"

class CSpruceBeetleModel : public CBioSIMModelBase
{
public:

	
    CSpruceBeetleModel();
    virtual ~CSpruceBeetleModel();

    virtual ERMsg OnExecuteAnnual();
    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
	static CBioSIMModelBase* CreateObject(){ return new CSpruceBeetleModel; }
    
};


