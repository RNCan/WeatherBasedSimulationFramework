#include "stdafx.h"
#include "MiniTab2CSV.h"
#include "MiniTab2CSVDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMiniTab2CSVApp

BEGIN_MESSAGE_MAP(CMiniTab2CSVApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMiniTab2CSVApp 

CMiniTab2CSVApp::CMiniTab2CSVApp()
{
}



CMiniTab2CSVApp theApp;


// CMiniTab2CSVApp 

BOOL CMiniTab2CSVApp::InitInstance()
{
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	SetRegistryKey(_T("Centre de Foresterie des Laurentides"));

	CMiniTab2CSVDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	return FALSE;
}
