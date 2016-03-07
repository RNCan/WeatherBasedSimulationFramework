#include "stdafx.h"
#include "Basic/hxGrid.h"
//#include "commonRes.h"

ERMsg GetConnectionStatus(IGridUser* pGridUser, int& loop)
{
	ERMsg msg;

	
	TGridUserConnectionStatus status;
	pGridUser->GetConnectionStatus(&status);

	if( status.bConnectedToCoordinator == false)
	{
		if( loop >= 50)
		{ 
			//CStdString error;
			//error.LoadString(IDS_CMN_UNABLE_CONNECT_HXGRID);
			msg.ajoute("Unable to connect to hxGrid coordinator. Verify that the hxGrid coordinator works properly. ");
		}
		loop++;
	}

	return msg;
}
