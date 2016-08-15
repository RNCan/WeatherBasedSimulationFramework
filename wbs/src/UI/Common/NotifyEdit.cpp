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
#include "NotifyEdit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNotifyEdit

CNotifyEdit::CNotifyEdit()
{
}

CNotifyEdit::~CNotifyEdit()
{
}


BEGIN_MESSAGE_MAP(CNotifyEdit, CEdit)
	//{{AFX_MSG_MAP(CNotifyEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNotifyEdit message handlers


BOOL CNotifyEdit::PreTranslateMessage(MSG* pMsg) 
{

	switch (pMsg->message)
	{ 
	case WM_KEYDOWN: 

        switch (pMsg->wParam)
		{ 
            case VK_RETURN: 

                GetParent()->SendMessage(MY_ENTER, 0, (LPARAM)this); 
                return 1; 
        } 
        break; 

	
   }

	
	return CEdit::PreTranslateMessage(pMsg);
}

