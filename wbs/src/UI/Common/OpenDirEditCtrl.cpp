//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "OpenDirEditCtrl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, COpenDirEditCtrl& rControl)
{
	::DDX_Control(pDX, nIDC, (CWnd&)rControl);
	rControl.EnableBrowseButton();
	//rControl.SetBrowseButtonImage();
}
