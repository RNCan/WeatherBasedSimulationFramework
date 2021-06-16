#include "StdAfx.h"
#include "UltimateGrid/ExcelSideHdg.h"
#include "UltimateGrid/ExcelTopHdg.h"
#include "Basic/Registry.h"
#include "UI/Common/UtilWin.h"
#include "ResultCtrl.h"

#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace std;
using namespace WBSF;
using namespace WBSF::DIMENSION;

static const UINT ID_CONTEXT_MENU_COPY = 1001;


BEGIN_MESSAGE_MAP(CResultCtrl, CUGExcel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateToolBar)
	ON_COMMAND(ID_EDIT_COPY, &OnEditCopy)
END_MESSAGE_MAP()

CResultCtrl::CResultCtrl(void)
{
	m_statType = MEAN;
	m_bIsExecute = false;
}

CResultCtrl::~CResultCtrl(void)
{
}



void CResultCtrl::OnSetup()
{
	CUGExcel::OnSetup();
}


void CResultCtrl::OnSheetSetup(int ID)
{
	WORD dpi = GetWindowDPI(GetSafeHwnd());
//1const int iWindowsReferenceDPI = 96;


	m_font.CreateStockObject(DEFAULT_GUI_FONT);
	
	//LOGFONT lf;                        // Used to create the CFont.
	//m_font.GetLogFont(&lf);
	//lf.lfHeight = -MulDiv(12,dpi, iWindowsReferenceDPI);
	//m_font.DeleteObject();
	//m_font.CreateFontIndirect(&lf);    // Create the font.

	int height = m_GI->m_defRowHeight;
	height = MulDiv(height, dpi, 96);
	SetDefRowHeight(height);
	


	m_cellBorderPen.CreatePen(PS_SOLID, 1, RGB(157, 157, 161));
	// create and set new top-heading class
	CExcelTopHdg* pExcelTopHdg = new CExcelTopHdg;
	pExcelTopHdg->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 234752);
	SetNewTopHeadingClass(pExcelTopHdg);
	// create and set new side-heading class
	CExcelSideHdg* pExcelSideHdg = new CExcelSideHdg;
	pExcelSideHdg->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 234753);
	SetNewSideHeadingClass(pExcelSideHdg);
	m_CUGHint->SetFont(&m_font);
	m_CUGTab->ShowScrollbars(true);


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
	UseHints(TRUE);
	UseVScrollHints(TRUE);
	EnableMenu(TRUE);
	EnableExcelBorders(TRUE);

	SetDoubleBufferMode(TRUE);
	SetVScrollMode(UG_SCROLLNORMAL);//UG_SCROLLTRACKING
	SetHScrollMode(UG_SCROLLTRACKING);
	SetHighlightRow(FALSE, FALSE);
	SetMultiSelectMode(UG_MULTISELECT_CELL);//
	SetCurrentCellMode(3);

	SetTH_NumberRows(1);
	SetSH_NumberCols(1);
}


void CResultCtrl::OnGetCell(int index,long row, CUGCell *cell)
{ 
	if (m_pResult && m_pResult->IsOpen() && m_enableUpdate && !m_bIsExecute)
	{
		if (row == -1)
		{
			if (index == -1)
			{
				cell->SetText(_T("No"));
			}
			else
			{
				int col = GetColTranslation(index);
				ASSERT(col>=0 && col<m_dataTitles.size());
				if (col >= 0 && col < m_dataTitles.size())
					cell->SetText(CString(m_dataTitles[col].c_str()));
			}
			
		}
		else
		{
			COLORREF backColor = row%2?RGB(235,235,255):RGB(255,255,255);
			if( index == -1)
			{
				cell->SetText(ToCString(row+1));
			}
			else
			{
				int col = GetColTranslation(index);
				ASSERT(col >= 0 && col < m_pResult->GetNbCols() );
				if (col >= 0 && col < m_pResult->GetNbCols())
				{
					CString str(m_pResult->GetDataValue(row, col, m_statType).c_str());
					cell->SetText(str);
					cell->SetBackColor(backColor);
				}
			}
		}
	}
} 

void CResultCtrl::SetData(CResultPtr pResult)
{
	if (pResult != m_pResult)
	{
		if (m_pResult && !m_pResult->IsOpen()) 
			m_pResult->Close();

		m_pResult = pResult;
	}
}


void CResultCtrl::Update()
{
	if (m_pResult && !m_pResult->IsOpen())
	{
		m_pResult->Open();
		//

		bool bEnable = false;
		if (m_pResult && m_pResult->IsOpen())
		{
			const CModelOutputVariableDefVector& outdef = m_pResult->GetMetadata().GetOutputDefinition();
			CDimension dimension = m_pResult->GetMetadata().GetDimension();
			m_pResult->GetDataHead(m_dataTitles);

			if (outdef.size() == m_pResult->GetNbCols(false))
			{
				bEnable = true;

				string modelName = m_pResult->GetMetadata().GetModelName();
				CRegistry registry("ResultColumnWidth");

				m_enableUpdate = FALSE;
				//unhide col
				CleanUpHidenCols();

				//ResetAll(true);
				ASSERT(m_dataTitles.size() == m_pResult->GetNbCols());
				SetNumberCols((int)m_pResult->GetNbCols(), FALSE);
				SetNumberRows((int)m_pResult->GetNbRows(), FALSE);

				for (size_t d = 0; d < DIMENSION::NB_DIMENSION; d++)
				{
					if (d == VARIABLE || dimension[d]<=1)
					{
						HideCol((int)d);
					}
				}
				
				int colWidth = registry.GetValue<int>("SideHeader", 80);
				SetSH_ColWidth(0, colWidth);
				//SetColWidth(-1, colWidth);

				//dimension col width
				for (size_t d = 0; d < DIMENSION::NB_DIMENSION; d++)
				{
					if (d!=VARIABLE && dimension[d]>1)
					{
						string name = CDimension::GetDimensionName(d);
						int colWidth = registry.GetValue<int>(name, 80);

						//int col = LocateCol((int)d);
						SetColWidth((int)d, colWidth);
					}
					//else
					//{
						//HideCol((int)d);
					//}
				}

				for (size_t v = 0; v < m_pResult->GetNbCols(false); v++)
				{
					string name = modelName + outdef[v].m_name;
					int colWidth = registry.GetValue<int>(name, 80);
					//int col = LocateCol(DIMENSION::NB_DIMENSION + (int)v); 
					//ASSERT(col >= 0);
					
					SetColWidth(DIMENSION::NB_DIMENSION + (int)v, colWidth);
				}

				m_enableUpdate = TRUE;
				Invalidate();

			}
		}

		if (!bEnable)
		{
			m_enableUpdate = FALSE;
			SetNumberCols(0, FALSE);
			SetNumberRows(0, FALSE);
			m_enableUpdate = TRUE;
			Invalidate();
		}
	}
}

int CResultCtrl::OnMenuStart(int col,long row,int section)
{ 
	if (section == UG_GRID ||section == UG_SIDEHEADING)
	{ 
		//****** Empty the Menu!!
		EmptyMenu(); 

		//******* Add the Menu Items
		CWnd* pParent = GetParent();
		if( pParent )
		{
			//ID_CONTEXT_MENU_COPY
			AddMenuItem(ID_EDIT_COPY, UtilWin::GetCString(IDS_CMN_AFXBARRES_COPY));
		}
	} 

	return TRUE; 

} 
//

void CResultCtrl::OnColSized(int index, int *width)
{
	ASSERT(m_pResult && m_pResult->IsOpen());

	

	if (m_enableUpdate&&m_pResult && m_pResult->IsOpen())
	{
		CRegistry registry("ResultColumnWidth");
		const CModelOutputVariableDefVector& outdef = m_pResult->GetMetadata().GetOutputDefinition();


		string modelName = m_pResult->GetMetadata().GetModelName();
		string name;

		int col = GetColTranslation(index);
		if (col == -1)
			name = "SideHeader";
		else if (col >= 0 && col < DIMENSION::NB_DIMENSION)
			name = CDimension::GetDimensionName(col);
		else
			name = modelName + outdef[col - DIMENSION::NB_DIMENSION].m_name;

		registry.SetValue(name, *width);
	}

	CUGExcel::OnColSized(index, width);
	
}

int CResultCtrl::OnSideHdgSized(int *width)
{
	if (m_enableUpdate&&m_pResult && m_pResult->IsOpen())
	{
		CRegistry registry("ResultColumnWidth");
		registry.SetValue("SideHeader", *width);
	}

	return CUGExcel::OnSideHdgSized(width);
}


int CResultCtrl::OnHint(int col,long row,int section,CString *string)
{
	UNREFERENCED_PARAMETER(section);
	if( row == -1 )
	{
		*string = QuickGetText(GetColTranslation(col),row);
		return TRUE;
	}
	
	return FALSE;
}

int CResultCtrl::OnVScrollHint(long row,CString *pString)
{
	if (m_enableUpdate&&m_pResult && m_pResult->IsOpen())
	{

		CString L(m_pResult->GetDataValue(row, LOCATION, -1).c_str());
		CString R(m_pResult->GetDataValue(row, REPLICATION, -1).c_str());
		CString T(m_pResult->GetDataValue(row, TIME_REF, -1).c_str());

		pString->Format(_T("%s [%s] %s"), L, R, T);
	}

	return TRUE;
}

void CResultCtrl::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	bool bInit = m_pResult && m_pResult->IsOpen();
	switch (pCmdUI->m_nID)
	{
	case ID_EDIT_COPY:   pCmdUI->Enable(bInit); break;
	}
}

void CResultCtrl::OnEditCopy()
{
	CopySelected();
}
