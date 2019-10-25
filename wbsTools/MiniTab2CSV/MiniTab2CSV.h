#pragma once

#ifndef __AFXWIN_H__
	#error 
#endif

#include "resource.h"	


// CMiniTab2CSVApp:

class CMiniTab2CSVApp : public CWinApp
{
public:
	CMiniTab2CSVApp();


	public:
	virtual BOOL InitInstance();


	DECLARE_MESSAGE_MAP()
};

extern CMiniTab2CSVApp theApp;
