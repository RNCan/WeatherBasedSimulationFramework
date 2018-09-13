#include "stdafx.h"

#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "NormalsCorrectionWnd.h"

using namespace std;
using namespace WBSF;
using namespace WBSF::GRADIENT;
using namespace WBSF::HOURLY_DATA;



//**************************************************************************************************************************************
// CNormalsCorrectionWnd
					

enum TCorrectionColumns{ G_STATION, G_SPACE, G_DELTA, G_FIRST_MONTH, G_JAN = G_FIRST_MONTH, G_FEB, G_MAR, G_APR, G_MAY, G_JUN, G_JUL, G_AUG, G_SEP, G_OCT, G_NOV, G_DEV, NB_GRADIENT_COLUMNS };

BEGIN_MESSAGE_MAP(CNormalsCorrectionCtrl, CUGCtrl)

END_MESSAGE_MAP()


// CNormalsCorrectionWnd construction/destruction
CNormalsCorrectionCtrl::CNormalsCorrectionCtrl()
{
}

CNormalsCorrectionCtrl::~CNormalsCorrectionCtrl()
{
}



void CNormalsCorrectionCtrl::OnSetup()
{

	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	CreateBoldFont();
	m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));


	m_CUGHint->SetFont(&m_font);
	m_CUGTab->ShowScrollbars(true);

	// add and set heading's default celltype
	CUGCell cell;
	GetHeadingDefault(&cell);
	cell.SetAlignment(UG_ALIGNCENTER);
	cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
	cell.SetBackColor(RGB(239, 237, 242));
	cell.SetHBackColor(RGB(162, 192, 248));

	cell.SetBorderColor(&m_cellBorderPen);
	cell.SetFont(&m_font);
	SetHeadingDefault(&cell);

	// create a font and set it as Grid Default
	GetGridDefault(&cell);
	cell.SetAlignment(UG_ALIGNCENTER);
	cell.SetFont(&m_font);
	cell.SetBorderColor(&m_cellBorderPen);
	cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
	SetGridDefault(&cell);
	// create a font and set it as Heading Default

	// set default properties
	UseHints(FALSE);
	UseVScrollHints(TRUE);

	EnableExcelBorders(TRUE);
	SetHighlightRow(UG_MULTISELECT_ROW, FALSE);
	SetMultiSelectMode(UG_MULTISELECT_OFF);
	SetCurrentCellMode(3);	//focus and highlighting
	SetDoubleBufferMode(TRUE);
	SetVScrollMode(UG_SCROLLTRACKING);
	SetHScrollMode(UG_SCROLLTRACKING);

	AddCellType(&m_sortArrow);


}


void CNormalsCorrectionCtrl::Update()
{
	//if (m_gradient != m_lastGradient)
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;

		if (m_location.IsInit())
		{
			EnableUpdate(FALSE);

			m_lastGradient = m_gradient;
			m_lastLocation = m_location;

			SortInfo(AfxGetApp()->GetProfileInt(_T("CorrectionCtrl"), _T("SortCol"), 0), AfxGetApp()->GetProfileInt(_T("CorrectionCtrl"), _T("SortDir"), 0));

			SetNumberCols(NB_GRADIENT_COLUMNS, FALSE);
			SetNumberRows((long)m_sortInfo.size(), FALSE);

			for (int i = -1; i < NB_GRADIENT_COLUMNS; i++)
			{
				int width = AfxGetApp()->GetProfileInt(_T("CorrectionCtrl"), _T("ColWidth ") + UtilWin::ToCString(i), 50);
				SetColWidth(i, width);
			}

				
			EnableUpdate(TRUE);
		}//is open
		else
		{
			SetNumberRows(0, FALSE);
			SetNumberCols(0, FALSE);
			m_sortInfo.clear();
		}

		Invalidate();

		//reload station index if they are change
		GetParent()->SendMessage(UWM_SELECTION_CHANGE);
	}

}



void CNormalsCorrectionCtrl::CreateBoldFont()
{
	if (m_fontBold.GetSafeHandle() != NULL)
	{
		m_fontBold.DeleteObject();
	}

	CFont* pFont = &m_font;
	ASSERT_VALID(pFont);

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));

	pFont->GetLogFont(&lf);

	lf.lfWeight = FW_BOLD;
	m_fontBold.CreateFontIndirect(&lf);
}




void CNormalsCorrectionCtrl::OnGetCell(int col, long row, CUGCell *cell)
{
	ASSERT(col >= -1 && col < NB_GRADIENT_COLUMNS);

	if (row >= -1 && col >= -1 && m_enableUpdate)
	{
		string text;
		COLORREF backColor = cell->GetBackColor();
		COLORREF textColor = cell->GetTextColor();
		CFont* pFont = cell->GetFont();

		if (col == -1)
		{
			if (row == -1)
				text = "No";
			else
				text = ToString(m_sortInfo[row].second + 1);
		}
		else
		{
			if (row == -1)
			{
					
				if (col == G_STATION)
					text = "Station";
				else if (col == G_SPACE)
					text = "Space";
				else if (col == G_DELTA)
					text = "Delta";
				else
					text = GetMonthTitle(col - G_FIRST_MONTH, false);

				if (col == m_curSortCol)
				{   // set default values to the top heading   
					cell->SetCellType(m_sortArrow.GetID());
					cell->SetCellTypeEx(m_sortDir);
				}
			}
			else
			{
				if (m_location.IsInit())
				{
					textColor = RGB(0, 0, 0);
					backColor = (row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255);

					text = GetDataText(col, (long)m_sortInfo[row].second);
				}
			}
		}

		cell->SetBackColor(backColor);
		cell->SetTextColor(textColor);
		cell->SetText(CString(text.c_str()));
		cell->SetFont(pFont);
	}

	CUGCtrl::OnGetCell(col, row, cell);
}


/***************************************************
OnCellChange
Sent whenever the current cell changes
Params:
oldcol, oldrow - coordinates of cell that is loosing the focus
newcol, newrow - coordinates of cell that is gaining the focus
Return:
<none>
****************************************************/
void CNormalsCorrectionCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
{
	ASSERT(newrow >= -1 && newrow < GetNumberRows());

	GetParent()->SendMessage(UWM_SELECTION_CHANGE);
}

/////////////////////////////////////////////////////////////////////////////
//	OnGetDefBackColor
//		Sent when the area behind the grid needs to be paindted.
//	Params:
//		section - Id of the grid section that requested this color
//				  possible sections:
//						UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
//	Return:
//		RGB value representing the color of choice
COLORREF CNormalsCorrectionCtrl::OnGetDefBackColor(int section)
{
	UNREFERENCED_PARAMETER(section);

	return GetSysColor(COLOR_APPWORKSPACE);
}





/////////////////////////////////////////////////////////////////////////////   
//  OnTH_LClicked   
//      Sent whenever the user clicks the left mouse button within the top heading   
//      this message is sent when the button goes down then again when the button goes up   
//  Params:   
//      col, row    - coordinates of a cell that received mouse click event   
//      updn        - is TRUE if mouse button is 'down' and FALSE if button is 'up'   
//      processed   - indicates if current event was processed by other control in the grid.   
//      rect        - represents the CDC rectangle of cell in question   
//      point       - represents the screen point where the mouse event was detected   
//  Return:   
//      <none>   
void CNormalsCorrectionCtrl::OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
{
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);

	if (!updn)
		return;

	BeginWaitCursor();

	SortInfo(col, col != m_curSortCol ? m_sortDir : OtherDir(m_sortDir));

	RedrawAll();
	EndWaitCursor();

	if (GetCurrentRow() >= m_sortInfo.size())
		Select(-1, -1);//force changing

	long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].second);
	GotoRow(newRow);
}

void CNormalsCorrectionCtrl::OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed)
{
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);

	if (!updn)
		return;

	BeginWaitCursor();

	SortInfo(-1, m_curSortCol != -1 ? m_sortDir : OtherDir(m_sortDir));

	RedrawAll();
	EndWaitCursor();

	if (GetCurrentRow() >= m_sortInfo.size())
		Select(-1, -1);

	long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].second);
	GotoRow((long)newRow);
}



void CNormalsCorrectionCtrl::OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
{
	if (!updn)
		return;

	//long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].second);
	GotoRow(row);
}


string CNormalsCorrectionCtrl::GetDataText(int col, long row)const
{
	string str;
	


	size_t st = row / NB_SPACE_EX;
	size_t g = V2G(m_variable);
	size_t m = col - G_FIRST_MONTH;
	size_t s = row % NB_SPACE_EX;
		
	

	const CLocation& station = m_results[st].m_location;

	switch (col)
	{
	case -1:		str = to_string(row + 1); break;
	case G_STATION: str = station.m_name; break;
	case G_SPACE:	str = GetSpaceName(s); break;
	case G_DELTA:
	{
		double delta = m_gradient.GetDistance(s, m_location, station);
		str = ToString(delta, 1);
		break;
	}
	default:
	{
		if (m_variable == H_TAIR)
		{
			size_t g1 = TMIN_GR;
			size_t g2 = TMAX_GR;

			double c = (m_gradient.GetCorrectionII(station, m, g1, s) + m_gradient.GetCorrectionII(station, m, g2, s)) / 2;
			str = ToString(c, 3);
		}
		else if (g != NOT_INIT)
		{
			double c = m_gradient.GetCorrectionII(station, m, g, s);
			str = ToString(c, 3);
		}
	}
	}

	return str;
}

void CNormalsCorrectionCtrl::SortInfo(int col, int dir)
{
	if (m_location.IsInit())
	{
		if (col != m_curSortCol)
		{

			m_curSortCol = col;
			m_sortInfo.clear();

			size_t nbRows = m_results.size()*NB_SPACE_EX;
			m_sortInfo.resize(nbRows);
			for (size_t row = 0; row != nbRows; row++)
				m_sortInfo[row] = std::make_pair(GetDataText(col, (long)row), row);

			if (m_curSortCol == G_STATION || m_curSortCol == G_SPACE)
				std::sort(m_sortInfo.begin(), m_sortInfo.end(), CCompareString());
			else
				std::sort(m_sortInfo.begin(), m_sortInfo.end(), CompareNumber);


			m_sortDir = UGCT_SORTARROWUP;

		}

		if (dir != m_sortDir)
		{
			std::reverse(m_sortInfo.begin(), m_sortInfo.end());
			m_sortDir = dir;
		}

		AfxGetApp()->WriteProfileInt(_T("CorrectionCtrl"), _T("SortCol"), m_curSortCol);
		AfxGetApp()->WriteProfileInt(_T("CorrectionCtrl"), _T("SortDir"), m_sortDir);
	}
	else
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;
	}
}

void CNormalsCorrectionCtrl::OnColSized(int col, int *width)
{
	AfxGetApp()->WriteProfileInt(_T("CorrectionCtrl"), _T("ColWidth ") + UtilWin::ToCString(col), *width);
}

//*********************************************************************************

static CMatchStationDoc* GetDocument()
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

static const int IDC_NORMALS_MATCH_ID = 1002;

CNormalsCorrectionWnd::CNormalsCorrectionWnd()
{
	m_bMustBeUpdated = false;
}

CNormalsCorrectionWnd::~CNormalsCorrectionWnd()
{
}


BEGIN_MESSAGE_MAP(CNormalsCorrectionWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()

	ON_UPDATE_COMMAND_UI_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)
	ON_CONTROL_RANGE(LBN_SELCHANGE, ID_TABLE_MODE_VISUALISATION, ID_TABLE_TM_TYPE, OnToolbarCommand)

	//ON_UPDATE_COMMAND_UI(ID_INDICATOR_NB_STATIONS, OnUpdateStatusBar)
	//ON_MESSAGE(UWM_SELECTION_CHANGE, OnSelectionChange)
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar

void CNormalsCorrectionWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	m_correctionCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CNormalsCorrectionWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;


	CreateToolBar();
	m_correctionCtrl.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_NORMALS_MATCH_ID);

	SetPropListFont();
	AdjustLayout();

	return 0;
}

void CNormalsCorrectionWnd::CreateToolBar()
{
	/*if (m_wndToolBar.GetSafeHwnd())
	m_wndToolBar.DestroyWindow();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_TABLE_TOOLBAR);
	m_wndToolBar.LoadToolBar(IDR_TABLE_TOOLBAR, 0, 0, TRUE);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);*/
}

void CNormalsCorrectionWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}
//
//void CStationsListWnd::OnSetFocus(CWnd* pOldWnd)
//{
//	CDockablePane::OnSetFocus(pOldWnd);
//	m_stationsList.SetFocus();
//}

void CNormalsCorrectionWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CNormalsCorrectionWnd::SetPropListFont()
{
	/*::DeleteObject(m_font.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_font.CreateFontIndirect(&lf);

	m_correctionCtrl.SetFont(&m_font);*/
}


void CNormalsCorrectionWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{

	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	if (!pDoc)
		return;


	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		CreateToolBar();
		AdjustLayout();
	}

	m_correctionCtrl.m_results	= pDoc->GetNormalsMatch();
	m_correctionCtrl.m_gradient = pDoc->GetNormalsGradient();
	m_correctionCtrl.m_variable = pDoc->GetVariable();

	if (pDoc->GetCurIndex() != UNKNOWN_POS)
		m_correctionCtrl.m_location = pDoc->GetLocation(pDoc->GetCurIndex());

	bool bVisible = IsWindowVisible();
	if (bVisible)
		m_correctionCtrl.Update();
	else
		m_bMustBeUpdated = true;

}

void CNormalsCorrectionWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		if (m_bMustBeUpdated)
		{
			m_correctionCtrl.Update();
			m_bMustBeUpdated = false;
		}

	}
}



void CNormalsCorrectionWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	//	CMatchStationDoc* pDoc = GetDocument();
	//	
	//	bool bInit = pDoc->GetCurIndex() != UNKNOWN_POS;
	//	bool bPeriodEnable = pDoc->GetPeriodEnabled();
	//	CWVariables variables = pDoc->GetVariables();
	//
	//
	//	switch (pCmdUI->m_nID)
	//	{
	//	//case ID_TABLE_MODE_VISUALISATION:pCmdUI->Enable(bInit); pCmdUI->SetCheck(!pDoc->GetDataInEdition()); break;
	//	//case ID_TABLE_MODE_EDITION:	pCmdUI->Enable(bInit); pCmdUI->SetCheck(pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_SAVE:			pCmdUI->Enable(bInit && pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_SENDTO_EXCEL:	pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_PERIOD_ENABLED:	pCmdUI->SetCheck(bPeriodEnable);  pCmdUI->Enable(bInit); break;
	//	case ID_TABLE_PERIOD_BEGIN:	pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//	case ID_TABLE_PERIOD_END:	pCmdUI->Enable(bInit&&bPeriodEnable); break;
	//	case ID_TABLE_FILTER:		pCmdUI->Enable(bInit); break;
	//	case ID_TABLE_STAT:			pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	case ID_TABLE_TM_TYPE:		pCmdUI->Enable(bInit && !pDoc->GetDataInEdition()); break;
	//	}
	//

}


void CNormalsCorrectionWnd::OnToolbarCommand(UINT ID)
{
	//CMatchStationDoc* pDoc = GetDocument();
	//int index = m_wndToolBar.CommandToIndex(ID);
	//CMFCToolBarButton* pCtrl = m_wndToolBar.GetButton(index); ASSERT(pCtrl);


	//switch (ID)
	//{
	//case ID_TABLE_MODE_VISUALISATION:
	//{
	//	if (pDoc->GetDataInEdition())
	//	{
	//		if (m_correctionCtrl.IsModified())
	//		{
	//			int rep = AfxMessageBox(IDS_SAVE_DATA, MB_YESNOCANCEL | MB_ICONQUESTION);
	//			if (rep == IDYES)
	//			{
	//				if (m_correctionCtrl.SaveData())
	//					pDoc->SetDataInEdition(false);
	//			}
	//			else if (rep == IDNO)
	//			{
	//				//reload station in document
	//				if (pDoc->CancelDataEdition())
	//				{
	//					//reload data in interface
	//					pDoc->SetDataInEdition(false);
	//				}
	//			}
	//		}
	//		else
	//		{
	//			pDoc->SetDataInEdition(false);
	//		}
	//	}

	//	break;
	//}
	//case ID_TABLE_MODE_EDITION:
	//{
	//	if (!pDoc->GetDataInEdition())
	//	{
	//		pDoc->SetTM(CTM::HOURLY);
	//		pDoc->SetStatistic(MEAN);
	//		pDoc->SetDataInEdition(true);
	//	}
	//	break;
	//}
	/*case ID_TABLE_SAVE:				m_correctionCtrl.SaveData();  break;
	case ID_TABLE_SENDTO_EXCEL:		m_correctionCtrl.ExportToExcel(); break;
	case ID_TABLE_PERIOD_ENABLED:	pDoc->SetPeriodEnabled(!(pCtrl->m_nStyle & TBBS_CHECKED)); break;
	case ID_TABLE_PERIOD_BEGIN:		OnDateChange(ID); break;
	case ID_TABLE_PERIOD_END:		OnDateChange(ID); break;
	case ID_TABLE_FILTER:			pDoc->SetVariables(((CMFCToolBarWVariablesButton*)pCtrl)->GetVariables()); break;
	case ID_TABLE_STAT:				pDoc->SetStatistic(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel()); break;
	case ID_TABLE_TM_TYPE:			pDoc->SetTM(CTM(((CMFCToolBarComboBoxButton*)pCtrl)->GetCurSel())); break;
	*/
	//	}


	//CWeatherDatabasePtr& pDB = GetDatabasePtr();


	//if (pDB && pDB->IsOpen())
	//{
	//	CMatchStationDoc* pDoc = GetDocument();
	//	ASSERT(pDoc);

	//	if (ID == ID_ADD_WEATHER_STATION)
	//	{
	//		MessageBox(_T("Ça reste à faire..."));
	//	}
	//	else if (ID == ID_SENDTO_EXCEL || ID == ID_SENDTO_SHOWMAP)
	//	{
	//		CRegistry registry;
	//		char separator = registry.GetListDelimiter();

	//		CSearchResultVector searchResultArray;

	//		CWVariables filter = pDoc->GetFilters();
	//		set<int> years = pDoc->GetYears();

	//		ERMsg msg;
	//		msg = pDB->GetStationList(searchResultArray, filter, years, true, false);

	//		CLocationVector loc = pDB->GetLocations(searchResultArray);

	//		string filePath = GetUserDataPath() + "tmp\\" + GetFileTitle(pDB->GetFilePath()) + ".csv";
	//		CreateMultipleDir(GetPath(filePath));

	//		msg += (loc.Save(filePath, separator));
	//		if (msg)
	//		{
	//			if (ID == ID_SENDTO_EXCEL)
	//				msg += CallApplication(CRegistry::SPREADSHEET, filePath, GetSafeHwnd(), SW_SHOW);
	//			else if (ID == ID_SENDTO_SHOWMAP)
	//				msg += CallApplication(CRegistry::SHOWMAP, filePath, GetSafeHwnd(), SW_SHOW);
	//		}

	//		//pDoc->SetOutputText(SYGetText(msg));
	//		if (!msg)
	//			SYShowMessage(msg, this);
	//	}
	//	else if (ID == ID_STATION_LIST_YEAR)
	//	{
	//		int index = m_wndToolBar.CommandToIndex(ID);
	//		CMFCToolBarYearsButton* pCtrl = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);

	//		std::set<int> years = pCtrl->GetYears();
	//		pDoc->SetYears(years);
	//	}
	//	else if (ID == ID_STATION_LIST_FILTER)
	//	{
	//		int index = m_wndToolBar.CommandToIndex(ID);
	//		CMFCToolBarWVariablesButton* pCtrl = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index); ASSERT(pCtrl);
	//		pDoc->SetFilters(pCtrl->GetVariables());
	//	}
	//}
}

BOOL CNormalsCorrectionWnd::PreTranslateMessage(MSG* pMsg)
{
	//GetAsyncKeyState(VK_RETURN)
	//GetKeyState
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	//{
	//	int index1 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_YEAR);
	//	CMFCToolBarYearsButton* pCtrl1 = (CMFCToolBarYearsButton*)m_wndToolBar.GetButton(index1); ASSERT(pCtrl1);
	//	if (pMsg->hwnd == pCtrl1->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_YEAR);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}

	//}

	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	//{
	//	int index2 = m_wndToolBar.CommandToIndex(ID_STATION_LIST_FILTER);
	//	CMFCToolBarWVariablesButton* pCtrl2 = (CMFCToolBarWVariablesButton*)m_wndToolBar.GetButton(index2); ASSERT(pCtrl2);
	//	if (pMsg->hwnd == pCtrl2->GetEditBox()->GetSafeHwnd())
	//	{
	//		// handle return pressed in edit control
	//		OnToolbarCommand(ID_STATION_LIST_FILTER);
	//		return TRUE; // this doesn't need processing anymore
	//	}
	//
	//	// handle return pressed in edit control
	//	return TRUE; // this doesn't need processing anymore
	//}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}



//void CNormalsCorrectionWnd::OnUpdateStatusBar(CCmdUI* pCmdUI)
//{

//if (pCmdUI->m_nID == ID_INDICATOR_NB_STATIONS)
//{
//	CWeatherDatabasePtr pDB = GetDatabasePtr();
//	//pCmdUI->->Enable(pDB && pDB->IsOpen());

//	long nbRows = m_stationsList.GetNumberRows();

//	CString str = _T("Stations = ");//GetCString(IDS_INDICATOR_NB_EXECUTE);
//	CString text = str + UtilWin::ToCString(nbRows);

//	pCmdUI->SetText(text);

//	//resize space of text
//	CDC* pDC = GetDC();
//	ASSERT(pDC);
//	CSize size = pDC->GetTextExtent(text);

//	UINT nStyle = m_wndStatusBar.GetPaneStyle(1);
//	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_NB_STATIONS, nStyle, size.cx);
//}


//}

//
//LRESULT  CNormalsCorrectionWnd::OnSelectionChange(WPARAM, LPARAM)
//{
//	CMatchStationDoc* pDoc = GetDocument();
//	ASSERT(pDoc);
//
//	size_t	index = m_stationsList.GetStationIndex();
//	pDoc->SetCurStationIndex(index);
//
//	return 0;
//}



void CNormalsCorrectionWnd::OnDateChange(UINT ID)
{
	ASSERT(ID == ID_TABLE_PERIOD_BEGIN || ID == ID_TABLE_PERIOD_END);

	/*CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();

	COleDateTime oFirstDate;
	COleDateTime oLastDate;
	CMFCToolBarDateTimeCtrl* pCtrl1 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_BEGIN); ASSERT(pCtrl1);
	CMFCToolBarDateTimeCtrl* pCtrl2 = CMFCToolBarDateTimeCtrl::GetByCmd(ID_TABLE_PERIOD_END); ASSERT(pCtrl2);

	pCtrl1->GetTime(oFirstDate);
	pCtrl2->GetTime(oLastDate);

	CTPeriod period;
	if (oFirstDate.GetStatus() == COleDateTime::valid &&
	oLastDate.GetStatus() == COleDateTime::valid)
	{
	CTRef begin(oFirstDate.GetYear(), oFirstDate.GetMonth() - 1, oFirstDate.GetDay() - 1);
	CTRef end(oLastDate.GetYear(), oLastDate.GetMonth() - 1, oLastDate.GetDay() - 1);
	if (begin > end)
	{
	if (ID == ID_TABLE_PERIOD_BEGIN)
	end = begin;
	else
	begin = end;
	}
	period = CTPeriod(begin, end);
	}

	pDoc->SetPeriod(period);*/

}

BOOL CNormalsCorrectionWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_correctionCtrl || pParent == &m_correctionCtrl)
		{
			if (m_correctionCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


