#include "stdafx.h"

#include "UI/Common/AppOption.h"

#include "FilePathWnd.h"
#include "MatchStationDoc.h"
#include "Resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace WBSF;




//IMPLEMENT_DYNAMIC(CFilePathDlg, CDialog)
BEGIN_MESSAGE_MAP(CFilePathDlg, CDialog)
	ON_CONTROL_RANGE(EN_CHANGE, IDC_NORMALS_FILEPATH, IDC_OBSERVATION_FILEPATH, &CFilePathDlg::OnChange)
	ON_WM_SIZE()
END_MESSAGE_MAP()


CMatchStationDoc* CFilePathDlg::GetDocument()
{

	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CMatchStationDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}


CFilePathDlg::CFilePathDlg(CWnd* pParent)
	: CDialog(CFilePathDlg::IDD, pParent)
{
}

CFilePathDlg::~CFilePathDlg()
{
}

int CFilePathDlg::Create(CWnd* pParent)
{
	return CDialog::Create(CFilePathDlg::IDD, pParent);
}

void CFilePathDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NORMALS_FILEPATH, m_normalsFilePathCtrl);
	DDX_Control(pDX, IDC_OBSERVATION_FILEPATH, m_observationFilePathCtrl);

	if (!pDX->m_bSaveAndValidate)
	{
		m_normalsFilePathCtrl.EnableFileBrowseButton(_T(".NormalsStations"), _T("Normals|*.NormalsStations||"));
		m_observationFilePathCtrl.EnableFileBrowseButton(_T(".DailyStations"), _T("Observations|*.HourlyStations;*.DailyStations||"));
	}
}


void CFilePathDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (GetSafeHwnd() == NULL || m_normalsFilePathCtrl.GetSafeHwnd() == NULL || m_observationFilePathCtrl.GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	CRect rect1;
	m_normalsFilePathCtrl.GetWindowRect(rect1);
	ScreenToClient(rect1);

	CRect rect2;
	m_observationFilePathCtrl.GetWindowRect(rect2);
	ScreenToClient(rect2);


	m_normalsFilePathCtrl.SetWindowPos(NULL, 0, 0, rectClient.Width() - rect1.left, rect1.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	m_observationFilePathCtrl.SetWindowPos(NULL, 0, 0, rectClient.Width() - rect2.left, rect2.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}


void CFilePathDlg::OnChange(UINT ID)
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);

	CWnd* pWnd = GetDlgItem(ID);
	ASSERT(pWnd);

	CString str;
	pWnd->GetWindowText(str);

	switch (ID)
	{
	case IDC_NORMALS_FILEPATH:	pDoc->SetNormalsFilePath(str);  break;
	case IDC_OBSERVATION_FILEPATH: pDoc->SetObservationFilePath(str);  break;
	}
}

void CFilePathDlg::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMatchStationDoc* pDoc = GetDocument();
	ASSERT(pDoc);



	{
		if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::NORMALS_DATABASE_CHANGE)
		{
			CWnd* pWnd = GetDlgItem(IDC_NORMALS_FILEPATH);
			ASSERT(pWnd);

			CString str1;
			pWnd->GetWindowText(str1);

			CWeatherDatabasePtr pDatabase = pDoc->GetNormalsDatabase();
			if (pDatabase && pDatabase->IsOpen())
			{
				CString str2(pDatabase->GetFilePath().c_str());
				if (str2.CompareNoCase(str1))
				{
					pWnd->SetWindowText(str2);
				}
			}
		}

		if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::OBSERVATION_DATABASE_CHANGE)
		{
			CWnd* pWnd = GetDlgItem(IDC_OBSERVATION_FILEPATH);
			ASSERT(pWnd);

			CString str1;
			pWnd->GetWindowText(str1);

			CWeatherDatabasePtr pDatabase = pDoc->GetObservationDatabase();
			if (pDatabase && pDatabase->IsOpen())
			{
				CString str2(pDatabase->GetFilePath().c_str());
				if (str2.CompareNoCase(str1))
				{
					pWnd->SetWindowText(str2);
				}
			}
		}
	}
}


/**********************************************************************/
IMPLEMENT_DYNAMIC(CFilePathWnd, CDockablePane)
	BEGIN_MESSAGE_MAP(CFilePathWnd, CDockablePane)
		ON_WM_CREATE()
		ON_WM_SIZE()
	END_MESSAGE_MAP()

	CFilePathWnd::CFilePathWnd(){}
	CFilePathWnd::~CFilePathWnd(){}

	int CFilePathWnd::OnCreate(LPCREATESTRUCT lp)
	{
		if (CDockablePane::OnCreate(lp) == -1)
			return -1;

		if (!m_wndDlg.Create(this))
			return -1;

		m_wndDlg.ShowWindow(SW_SHOWDEFAULT);

		return  0;
	}
	void CFilePathWnd::OnSize(UINT nType, int cx, int cy)
	{
		CDockablePane::OnSize(nType, cx, cy);
		m_wndDlg.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
	}


	void CFilePathWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
	{
		m_wndDlg.OnUpdate(pSender, lHint, pHint);
	}

