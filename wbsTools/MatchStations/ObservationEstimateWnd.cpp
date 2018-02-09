#include "stdafx.h"

#include "MatchStationApp.h"
#include "MatchStationDoc.h"
#include "ObservationEstimateWnd.h"

using namespace std;
using namespace WBSF::GRADIENT;
using namespace WBSF::HOURLY_DATA;



using namespace WBSF;



//**************************************************************************************************************************************
// CObservationEstimateWnd
					
/*
enum TEstimateColumns{ G_TYPE, G_FIRST_MONTH, G_JAN = G_FIRST_MONTH, G_FEB, G_MAR, G_APR, G_MAY, G_JUN, G_JUL, G_AUG, G_SEP, G_OCT, G_NOV, G_DEV, NB_GRADIENT_COLUMNS };

BEGIN_MESSAGE_MAP(CObservationEstimateCtrl, CUGCtrl)

END_MESSAGE_MAP()


// CObservationEstimateWnd construction/destruction
CObservationEstimateCtrl::CObservationEstimateCtrl()
{
}

CObservationEstimateCtrl::~CObservationEstimateCtrl()
{
}



void CObservationEstimateCtrl::OnSetup()
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


void CObservationEstimateCtrl::Update()
{
	//if (m_gradient != m_lastGradient)
	{
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;

		if (m_estimate.IsInit() && m_mean.IsInit() )
		{
			EnableUpdate(FALSE);

			SortInfo(AfxGetApp()->GetProfileInt(_T("EstimateOCtrl"), _T("SortCol"), 0), AfxGetApp()->GetProfileInt(_T("EstimateCtrl"), _T("SortDir"), 0));

			SetNumberCols(NB_GRADIENT_COLUMNS, FALSE);
			SetNumberRows((long)m_sortInfo.size(), FALSE);

			for (int i = -1; i < NB_GRADIENT_COLUMNS; i++)
			{
				int width = AfxGetApp()->GetProfileInt(_T("EstimateOCtrl"), _T("ColWidth ") + UtilWin::ToCString(i), 50);
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



void CObservationEstimateCtrl::CreateBoldFont()
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




void CObservationEstimateCtrl::OnGetCell(int col, long row, CUGCell *cell)
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
				if (col==G_TYPE)
					text = "Type";
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
				if (m_estimate.IsInit() && m_mean.IsInit())
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


//***************************************************
//OnCellChange
//Sent whenever the current cell changes
//Params:
//oldcol, oldrow - coordinates of cell that is loosing the focus
//newcol, newrow - coordinates of cell that is gaining the focus
//Return:
//<none>
//****************************************************
void CObservationEstimateCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
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
COLORREF CObservationEstimateCtrl::OnGetDefBackColor(int section)
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
void CObservationEstimateCtrl::OnTH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
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

void CObservationEstimateCtrl::OnCB_LClicked(int updn, RECT *rect, POINT *point, BOOL processed)
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



void CObservationEstimateCtrl::OnSH_LClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
{
	if (!updn)
		return;

	//long newRow = long((GetCurrentRow() == -1) ? -1 : m_sortInfo[GetCurrentRow()].second);
	GotoRow(row);
}

enum TType{STATION_MEAN, TOTAL_CORRECTION, FINAL_ESTIMATE, NB_TYPE};
static const char* TYPE_NAME[NB_TYPE] = {"Weather Stations mean","Correction","Final computation"};

string CObservationEstimateCtrl::GetDataText(int col, long row)const
{
	string str;
		
	size_t v = m_variable;
	size_t m = col - G_FIRST_MONTH;

	switch (col)
	{
	case -1:	str = to_string(row + 1); break;
	case G_TYPE:str = TYPE_NAME[row]; break;
	default:	
		switch (row)
		{
		case STATION_MEAN:		str = ToString(m_mean[m][v], 3); break;
		case TOTAL_CORRECTION:	str = ToString((m_variable == H_PRCP) ? m_estimate[m][f]/m_mean[m][f]:(m_estimate[m][f] - m_mean[m][f]), 3); break;
		case FINAL_ESTIMATE:	str = ToString(m_estimate[m][f], 3); break;
		default: ASSERT(false);
		}
	}

	return str;
}

void CObservationEstimateCtrl::SortInfo(int col, int dir)
{
	if (m_estimate.IsInit() && m_mean.IsInit())
	{
		if (col != m_curSortCol)
		{
			m_curSortCol = col;
			m_sortInfo.clear();

			size_t nbRows = NB_TYPE;
			m_sortInfo.resize(nbRows);
			for (size_t row = 0; row != nbRows; row++)
				m_sortInfo[row] = std::make_pair(GetDataText(col, (long)row), row);

			if (m_curSortCol == G_TYPE)
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

		AfxGetApp()->WriteProfileInt(_T("EstimateCtrl"), _T("SortCol"), m_curSortCol);
		AfxGetApp()->WriteProfileInt(_T("EstimateCtrl"), _T("SortDir"), m_sortDir);
	}
	else
	{
		m_sortInfo.clear();
		m_curSortCol = -999;
		m_sortDir = UGCT_SORTARROWUP;
	}
}

void CObservationEstimateCtrl::OnColSized(int col, int *width)
{
	AfxGetApp()->WriteProfileInt(_T("EstimateCtrl"), _T("ColWidth ") + UtilWin::ToCString(col), *width);
}
*/
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

static const int IDC_ESTIMATE_CTRL_ID = 1002;

CObservationEstimateWnd::CObservationEstimateWnd()
{
	m_bMustBeUpdated = false;
}

CObservationEstimateWnd::~CObservationEstimateWnd()
{
}


BEGIN_MESSAGE_MAP(CObservationEstimateWnd, CDockablePane)
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

void CObservationEstimateWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	m_estimateCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CObservationEstimateWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;


	CreateToolBar();
	m_estimateCtrl.CreateGrid(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_ESTIMATE_CTRL_ID);

	SetPropListFont();
	AdjustLayout();

	return 0;
}

void CObservationEstimateWnd::CreateToolBar()
{
		
}

void CObservationEstimateWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}
	

void CObservationEstimateWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CObservationEstimateWnd::SetPropListFont()
{
		
}


void CObservationEstimateWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{

	CMatchStationDoc* pDoc = (CMatchStationDoc*)GetDocument();
	if (!pDoc)
		return;


	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{
		CreateToolBar();
		AdjustLayout();
	}
		
	CTM TM(pDoc->GetObservationType() == CMatchStationDoc::T_HOURLY ? CTM::HOURLY : CTM::DAILY);

	m_estimateCtrl.m_pStation.reset(new CWeatherStation);
	if (pDoc->GetCurIndex()!=NOT_INIT)
	{
		const CLocation& target = pDoc->GetLocation(pDoc->GetCurIndex());
		((CLocation&)*m_estimateCtrl.m_pStation) = target;
		pDoc->GetObservationStation().GetInverseDistanceMean(pDoc->GetVariable(), target, *m_estimateCtrl.m_pStation, true, WEATHER::SHORE_DISTANCE_FACTOR>0);

		
		CWVariables variable(m_estimateCtrl.m_pStation->GetVariables());

		CWeatherFormat format(TM, variable);//get default format
		//const std::string& header= format.GetHeader();
		//CWeatherFormat format2(header.c_str());//get default format
		m_estimateCtrl.m_pStation->SetFormat(format);
	}
		

	m_estimateCtrl.m_TM = TM;
	m_estimateCtrl.m_stat = MEAN;
	m_estimateCtrl.m_bEditable = false;
	m_estimateCtrl.m_bPeriodEnabled = false;
	m_estimateCtrl.m_period = CTPeriod();
	m_estimateCtrl.m_variables = pDoc->GetVariable();

	bool bVisible = IsWindowVisible();
	if (bVisible)
		m_estimateCtrl.Update();
	else
		m_bMustBeUpdated = true;

}

void CObservationEstimateWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDockablePane::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_SHOWWINDOW)
	{
		if (m_bMustBeUpdated)
		{
			m_estimateCtrl.Update();
			m_bMustBeUpdated = false;
		}
	}
}



void CObservationEstimateWnd::OnUpdateToolbar(CCmdUI *pCmdUI)
{
	
}


void CObservationEstimateWnd::OnToolbarCommand(UINT ID)
{
		
}

BOOL CObservationEstimateWnd::PreTranslateMessage(MSG* pMsg)
{


	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}




void CObservationEstimateWnd::OnDateChange(UINT ID)
{
	ASSERT(ID == ID_TABLE_PERIOD_BEGIN || ID == ID_TABLE_PERIOD_END);
}


BOOL CObservationEstimateWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_estimateCtrl || pParent == &m_estimateCtrl)
		{
			if (m_estimateCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

