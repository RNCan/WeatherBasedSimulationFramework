//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 21-02-2020	Rémi Saint-Amant	replace Google Elevation by OpenTopoData and Geocode by nominatim
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"

#include "UltimateGrid/ExcelTopHdg.h"
#include "UltimateGrid/ExcelSideHdg.h"
#include "FileManager/FileManager.h"
#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/Shore.h"
#include "Geomatic/gdalBasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/UtilWWW.h"
#include "UI/LOCGeneratorDlg.h"
#include "UI/LOCEditDlg.h"
#include "UI/ExtractSSIDlg.h"
#include "Basic/json/json11.hpp"
#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace UtilWin;
using namespace UtilWWW;
using namespace std;
using namespace json11;


namespace WBSF
{

	static const UINT ID_CONTEXT_MENU_ADD = 1001;
	static const UINT ID_CONTEXT_MENU_DELETE = 1002;
	static const UINT ID_CONTEXT_MENU_COPY = 1003;
	static const UINT ID_CONTEXT_MENU_PASTE = 1004;






	BEGIN_MESSAGE_MAP(CLOCGridCtrl, CUGEditCtrl)
		ON_WM_DESTROY()
	END_MESSAGE_MAP()

	void CLOCGridCtrl::CreateBoldFont()
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

	/////////////////////////////////////////////////////////////////////////////
	//	OnSetup
	//		This function is called just after the grid window 
	//		is created or attached to a dialog item.
	//		It can be used to initially setup the grid
	void CLOCGridCtrl::OnSetup()
	{
		SetDefRowHeight(MulDiv(m_GI->m_defRowHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetTH_Height(MulDiv(m_GI->m_topHdgHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetHS_Height(MulDiv(m_GI->m_hScrollHeight, GetWindowDPI(GetSafeHwnd()), 96));


		CAppOption option;
		option.SetCurrentProfile(_T("ColumnWidth"));
		m_format = DECIMALS_DEGREES;

		m_font.CreateStockObject(DEFAULT_GUI_FONT);
		CreateBoldFont();
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

	void CLOCGridCtrl::SetData(const CLocationVector& data)
	{
		m_locations = data;
		m_header = data.GetHeader();

		m_bDataEdited.clear();
		m_bDataEdited.resize(data.size());
		for (size_t i = 0; i < m_bDataEdited.size(); i++)
			m_bDataEdited[i].resize(m_header.size());


		EnableUpdate(FALSE);

		SetNumberRows((long)data.size());
		SetNumberCols((int)m_header.size());

		SetHaveChange(false);
		EnableUpdate(TRUE);

		Invalidate();
		RedrawAll();
	}

	void CLOCGridCtrl::GetData(CLocationVector& data)
	{
		ASSERT(GetNumberCols() >= CLocation::SITE_SPECIFIC_INFORMATION);

		data = m_locations;

	}

	void CLOCGridCtrl::OnGetCell(int col, long row, CUGCell *cell)
	{

		if (m_enableUpdate)
		{
			string text;
			COLORREF backColor = cell->GetBackColor();
			COLORREF textColor = cell->GetTextColor();
			CFont* pFont = cell->GetFont();

			if (row == -1)
			{
				if (col == -1)
				{
					text = "#";
				}
				else
				{
					text = m_header[col];
				}
			}
			else
			{
				if (col == -1)
				{
					text = WBSF::ToString(row + 1);
				}
				else //if (row < m_locations.size() )//call before we add a row
				{
					textColor = RGB(0, 0, 0);
					backColor = ((row % 2) ? RGB(235, 235, 255) : RGB(255, 255, 255));
					if (col < CLocation::SITE_SPECIFIC_INFORMATION)
					{
						if (m_format == DMS && col == CLocation::LAT)
						{
							text = CoordToString(m_locations[row].m_lat, true);
						}
						else if (m_format == DMS && col == CLocation::LON)
						{
							text = CoordToString(m_locations[row].m_lon, true);
						}
						else
						{
							text = m_locations[row].GetMember(col);
						}
					}
					else
					{
						ENSURE(row < m_locations.size());
						ENSURE(col < m_header.size());
						text = m_locations[row].GetSSI(m_header[col]);
					}

					if (m_bDataEdited[row][col])
					{
						textColor = RGB(50, 90, 230);
						pFont = &m_fontBold;
					}
				}
			}

			cell->SetBackColor(backColor);
			cell->SetTextColor(textColor);
			cell->SetText(CString(text.c_str()));
			cell->SetFont(pFont);
		}


		CUGEditCtrl::OnGetCell(col, row, cell);
	}

	void CLOCGridCtrl::OnSetCell(int col, long row, CUGCell *cell)
	{
		ASSERT(m_bDataEdited.size() == m_locations.size());

		if (col >= 0 && row >= 0)
		{
			while (row >= m_locations.size())
			{
				m_locations.push_back(CLocation());
				m_bDataEdited.push_back(boost::dynamic_bitset<size_t>(m_header.size()));
			}


			CUGCell oldCell;
			GetCellIndirect(col, row, &oldCell);

			std::string oldStr = CStringA(oldCell.GetText());
			std::string newStr = CStringA(cell->GetText());

			if (newStr != oldStr)
			{
				m_bHaveChange = true;
				size_t pos = row * GetNumberCols() + row;
				m_bDataEdited[row].set(col);

				if (col < CLocation::SITE_SPECIFIC_INFORMATION)
				{
					m_locations[row].SetMember(col, newStr);
				}
				else
				{
					m_locations[row].SetSSI(m_header[col], newStr);
				}
			}
		}
	}

	void CLOCGridCtrl::SetFormat(short format)
	{
		ASSERT(GetNumberCols() == m_header.size());
		if (format != m_format)
		{
			m_format = format;

			RedrawCol(CLocation::LAT);
			RedrawCol(CLocation::LON);
		}

	}

	int CLOCGridCtrl::OnMenuStart(int col, long row, int section)
	{

		if (section == UG_GRID ||
			section == UG_SIDEHEADING)
		{
			//****** Empty the Menu!!
			EmptyMenu();

			//******* Add the Menu Items
			CWnd* pParent = GetParent();
			if (pParent)
			{
				AddMenuItem(ID_CONTEXT_MENU_ADD, UtilWin::GetCString(IDS_CMN_AFXBARRES_NEW));
				AddMenuItem(ID_CONTEXT_MENU_DELETE, UtilWin::GetCString(IDS_CMN_AFXBARRES_DELETE));

				AddMenuItem(-1, _T(""));

				AddMenuItem(ID_CONTEXT_MENU_COPY, UtilWin::GetCString(IDS_CMN_AFXBARRES_COPY));
				AddMenuItem(ID_CONTEXT_MENU_PASTE, UtilWin::GetCString(IDS_CMN_AFXBARRES_PASTE));
			}
		}

		return TRUE;

	}


	void CLOCGridCtrl::OnMenuCommand(int col, long row, int section, int item)
	{
		if (item == ID_CONTEXT_MENU_ADD)
		{
			AppendRow();
			RedrawAll();
		}

		if (item == ID_CONTEXT_MENU_DELETE)
		{
			CString buffer;
			DeleteRow(row);
			RedrawAll();
		}

		if (section == UG_GRID)
		{
			//****** The user has selected the 'Copy' option
			if (item == ID_CONTEXT_MENU_COPY)
				CopySelected();

			//****** The user has selected the 'Paste' option
			if (item == ID_CONTEXT_MENU_PASTE)
				Paste();
		}

		SetHaveChange(true);
	}

	void CLOCGridCtrl::OnDestroy()
	{

		CAppOption option;
		option.SetCurrentProfile(_T("ColumnWidth"));

		for (int i = -1; i < CLocation::NB_MEMBER; i++)
		{
			int colWidth = GetColWidth(i);
			option.WriteProfileInt(_T("LocGridCtrl") + UtilWin::ToCString(i + 1), colWidth);
		}

		CUGEditCtrl::OnDestroy();
	}


	BOOL CLOCGridCtrl::PreTranslateMessage(MSG* pMsg)
	{
		if (GetKeyState(VK_CONTROL) < 0)
		{
			if (pMsg->message == WM_KEYDOWN)
			{
				if (pMsg->wParam == 67)
				{
					CopySelected();

					return TRUE; // this doesn't need processing anymore

				}
				else if (pMsg->wParam == 86)
				{
					//get the text from the clipboard
					Paste();
					return TRUE; // this doesn't need processing anymore
				}
			}
		}

		if (pMsg->wParam == VK_DELETE)
		{
			DeleteSelection();
		}

		return CUGEditCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing
	}


	int CLOCGridCtrl::OnVScrollHint(long row, CString *string)
	{
		*string = QuickGetText(-1, row);

		return TRUE;
	}

	int	CLOCGridCtrl::AppendRow()
	{
		m_locations.push_back(CLocation());
		m_bDataEdited.push_back(boost::dynamic_bitset<size_t>(m_header.size()));

		return CUGEditCtrl::AppendRow();
	}

	int	CLOCGridCtrl::DeleteRow(long row)
	{
		ASSERT(row >= 0 && row < m_locations.size());
		m_locations.erase(m_locations.begin() + row);
		m_bDataEdited.erase(m_bDataEdited.begin() + row);

		return CUGEditCtrl::DeleteRow(row);
	}



	//******************************************************************************
	CLocDlg::CLocDlg(CWnd* pParent)
	{
		m_bEnable = FALSE;
	}

	void CLocDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
	}


	BEGIN_MESSAGE_MAP(CLocDlg, CDialog)
		ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
		ON_COMMAND(ID_LOC_NEW_LINE, OnLocNewLine)
		ON_COMMAND(ID_LOC_REMOVE_LINE, OnLocDeleteLine)
		ON_COMMAND(ID_LOC_GENERATE, OnLocGenerate)
		ON_COMMAND(ID_LOC_EXTRACT_SSI, OnExtractSSI)

		ON_COMMAND_EX(ID_LOC_DECIMAL_DEGREES, OnFormatChange)
		ON_COMMAND_EX(ID_LOC_DMS, OnFormatChange)
		ON_UPDATE_COMMAND_UI(ID_LOC_NEW_LINE, OnUpdateCtrl)
		ON_UPDATE_COMMAND_UI(ID_LOC_REMOVE_LINE, OnUpdateCtrl)
		ON_UPDATE_COMMAND_UI(ID_LOC_GENERATE, OnUpdateCtrl)
		ON_UPDATE_COMMAND_UI(ID_LOC_EXTRACT_SSI, OnUpdateCtrl)
		ON_UPDATE_COMMAND_UI(ID_LOC_DECIMAL_DEGREES, OnUpdateCtrl)
		ON_UPDATE_COMMAND_UI(ID_LOC_DMS, OnUpdateCtrl)
		ON_WM_DESTROY()
		ON_WM_SIZE()
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CLocDlg message handlers


	BOOL CLocDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();
		m_grid.AttachGrid(this, IDC_LOC_STATION_LIST);

		return TRUE;
	}

	void CLocDlg::SetLoc(const CLocationVector& loc)
	{
		m_grid.SetData(loc);
		UpdateCtrl();
	}

	bool CLocDlg::GetLoc(CLocationVector& loc)
	{
		if (!m_grid.GetHaveChange())
			return false;

		m_grid.GetData(loc);
		return true;
	}

	BOOL CLocDlg::EnableWindow(BOOL bEnable)
	{
		BOOL bRep = m_bEnable;
		m_bEnable = bEnable;
		UpdateCtrl();
		return bRep;
	}

	void CLocDlg::OnUpdateCtrl(CCmdUI* pCmdUI)
	{
		switch (pCmdUI->m_nID)
		{
		case ID_LOC_NEW_LINE: pCmdUI->Enable(m_bEnable); break;
		case ID_LOC_REMOVE_LINE: pCmdUI->Enable(m_bEnable && m_grid.GetNumberRows() > 0); break;
		case ID_LOC_GENERATE: pCmdUI->Enable(m_bEnable); break;
		case ID_LOC_EXTRACT_SSI:pCmdUI->Enable(m_bEnable && m_grid.GetNumberRows() > 0); break;
		case ID_LOC_DECIMAL_DEGREES: pCmdUI->SetRadio(m_grid.GetFormat() == CLOCGridCtrl::DECIMALS_DEGREES); break;
		case ID_LOC_DMS:pCmdUI->SetRadio(m_grid.GetFormat() == CLOCGridCtrl::DMS); break;
		default: ASSERT(false);
		}
	}

	void CLocDlg::UpdateCtrl()
	{
		bool bEnableStationList = m_bEnable && m_grid.GetNumberRows() > 0;
		m_grid.EnableWindow(bEnableStationList);

		CString nbStation;
		nbStation.Format(_T("%d"), m_grid.GetNumberRows());
		m_nbStation.SetWindowText(nbStation);
	}

	BOOL CLocDlg::Create(CWnd* pParentWnd)
	{
		BOOL bRep = CDialog::Create(IDD, pParentWnd);

		if (bRep)
		{
			m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_LOC_TOOLBAR);
			m_wndToolBar.LoadToolBar(IDR_LOC_TOOLBAR, 0, 0, TRUE /* Is locked */);
			m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
			m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
			m_wndToolBar.SetOwner(this);
			m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

			m_nbStation.Create(_T("0"), WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT | SS_LEFT, CRect(), this);
			m_nbStation.SetFont(GetFont());

			CAppOption option(_T("WindowsPosition"));
			CRect rect = option.GetProfileRect(_T("LocDlg"), CRect(455, 30, 1200, 500));
			UtilWin::EnsureRectangleOnDisplay(rect);
			MoveWindow(rect);

			UpdateCtrl();
		}

		return bRep;
	}



	void CLocDlg::OnDestroy()
	{
		CAppOption option(_T("WindowsPosition"));

		CRect curRect;
		GetWindowRect(curRect);

		option.WriteProfileRect(_T("LocDlg"), curRect);


		CDialog::OnDestroy();
	}


	//do nothing
	void CLocDlg::OnOK()
	{}

	void CLocDlg::OnCancel()
	{}

	// generate loc from CGenerateLOCDlg 
	void CLocDlg::OnLocGenerate()
	{
		//modal on the parent and not on this windows
		ASSERT(m_pLocEditDlg);


		CLOCGeneratorDlg dlg(this);

		m_pLocEditDlg->EnableWindow(false);
		if (dlg.DoModal() == IDOK)
		{
			CWaitCursor cursor;

			CLocationVector locArray;
			m_grid.GetData(locArray);

			locArray.insert(locArray.end(), dlg.GetLOC().begin(), dlg.GetLOC().end());

			m_grid.SetData(locArray);
			m_grid.SetHaveChange(true);

			UpdateCtrl();
		}

		m_pLocEditDlg->EnableWindow(true);
	}

	void CLocDlg::OnExtractSSI()
	{
		//modal on the parent and not on this windows
		ASSERT(m_pLocEditDlg);

		m_pLocEditDlg->EnableWindow(false);

		CExtractSSIDlg dlg(this);

		if (dlg.DoModal() == IDOK)
		{
			ERMsg msg;

			//CWaitCursor cursor;

			CLocationVector locations;
			m_grid.GetData(locations);

			CProgressStepDlg progressDlg;
			progressDlg.Create(this);
			//progressDlg.Create(dlg.GetTopLevelOwner());

			CProgressStepDlgParam param(&dlg, "", &locations);
			msg = progressDlg.Execute(ExtractSSI, &param);
			progressDlg.DestroyWindow();


			if (!msg)
				SYShowMessage(msg, this);


			m_grid.SetData(locations);
			m_grid.SetHaveChange(true);

			UpdateCtrl();
		}

		dlg.DestroyWindow();
		m_pLocEditDlg->EnableWindow(true);
	}


	UINT CLocDlg::ExtractSSI(void* pParam)
	{
		CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
		CExtractSSIDlg* pDlg = (CExtractSSIDlg*)pMyParam->m_pThis;
		CLocationVector* pLocations = (CLocationVector*)pMyParam->m_pExtra;

		ERMsg* pMsg = pMyParam->m_pMsg;
		CCallback* pCallback = pMyParam->m_pCallback;

		VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
		//TRY

		if (pDlg->m_extractFrom == CExtractSSIDlg::FROM_DEM)
		{
			if (pDlg->m_bExtractElev || pDlg->m_bExtractSlopeAspect)
			{
				string filePath = CStringA(pDlg->m_gridFilePath);
				*pMsg = ExtractFromDEM(*pLocations, filePath,
					pDlg->m_interpolationType,
					pDlg->m_bExtractElev,
					pDlg->m_bExtractSlopeAspect,
					!pDlg->m_bMissingOnly,
					*pCallback);
			}

		}
		else
		{
			ASSERT(pDlg->m_extractFrom == CExtractSSIDlg::FROM_WEB);
			if (*pMsg && pDlg->m_bExtractWebElevation)
			{
				//*pMsg = ExtractOpenTopoDataElevation(*pLocations, !pDlg->m_bMissingOnly, pDlg->m_webElevProduct, pDlg->m_interpolationType, *pCallback);
				*pMsg = pLocations->ExtractOpenTopoDataElevation(!pDlg->m_bMissingOnly, pDlg->m_webElevProduct, pDlg->m_interpolationType, *pCallback);
			}

			if (*pMsg && pDlg->m_bExtractWebName)
			{
				//*pMsg = ExtractNominatimName(*pLocations, !pDlg->m_bMissingOnly, pDlg->m_bExtractWebName, pDlg->m_bExtractWebState, pDlg->m_bExtractWebCountry, *pCallback);
				*pMsg = pLocations->ExtractNominatimName(!pDlg->m_bMissingOnly, pDlg->m_bExtractWebName, pDlg->m_bExtractWebState, pDlg->m_bExtractWebCountry, *pCallback);
			}
		}

		if (pDlg->m_bExtractShoreDistance)
		{
			//*pMsg = ExtractShoreDistance(*pLocations, !pDlg->m_bMissingOnly, *pCallback);
			*pMsg = pLocations->ExtractShoreDistance(!pDlg->m_bMissingOnly, *pCallback);
		}


		//CATCH_ALL(e)
		//	*pMsg = UtilWin::SYGetMessage(*e);
		//END_CATCH_ALL

		CoUninitialize();

		if (*pMsg)
			return 0;

		return -1;
	}



	void CLocDlg::OnLocDeleteLine()
	{
		ASSERT(m_grid.GetNumberRows() > 0);

		std::set<int> lines;
		int col = -1;	long row = -1;
		if (m_grid.EnumFirstSelected(&col, &row) == UG_SUCCESS)
		{
			lines.insert(row);
			while (m_grid.EnumNextSelected(&col, &row) == UG_SUCCESS)
				lines.insert(row);

			for (std::set<int>::reverse_iterator it = lines.rbegin(); it != lines.rend(); ++it)
			{
				if (*it >= 0 && *it < m_grid.GetNumberRows())//selection is sometime pass the last row
					m_grid.DeleteRow(*it);
			}

			m_grid.SetHaveChange(true);

			UpdateCtrl();
		}
	}


	void CLocDlg::OnLocNewLine()
	{
		m_grid.AppendRow();
		m_grid.SetHaveChange(true);

		UpdateCtrl();
	}

	BOOL CLocDlg::OnFormatChange(UINT ID)
	{
		m_grid.SetFormat(ID == ID_LOC_DMS ? CLOCGridCtrl::DMS : CLOCGridCtrl::DECIMALS_DEGREES);

		return TRUE;
	}


	void CLocDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialog::OnSize(nType, cx, cy);

		AdjustLayout();
	}

	void CLocDlg::AdjustLayout()
	{
		if (GetSafeHwnd() == NULL || m_wndToolBar.GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient, rectEdit;
		GetClientRect(rectClient);
		rectClient.DeflateRect(5, 5, 5, 5);


		int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
		int cxTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cx;

		m_wndToolBar.SetWindowPos(NULL, max(50l, rectClient.right - cxTlb), rectClient.top, cxTlb, cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
		m_nbStation.SetWindowPos(NULL, rectClient.left, rectClient.top, 50, cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
		m_grid.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), rectClient.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	}


	LRESULT CLocDlg::OnKickIdle(WPARAM, LPARAM)
	{
		if (GetSafeHwnd() && IsWindowVisible())
		{
			for (int i = 0; i < m_wndToolBar.GetCount(); i++)
				m_wndToolBar.UpdateButton(i);
		}

		return FALSE;
	}

	ERMsg CLocDlg::ExtractFromDEM(CLocationVector& locations, const string& filePath, size_t interpolationType, bool bExtractElev, bool bExtractSlopeAspect, bool bReplaceAll, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Extract specific site information", locations.size());
		CGDALDatasetEx inputDS;
		if (bExtractElev || bExtractSlopeAspect)
			msg = inputDS.OpenInputImage(filePath);


		if (msg)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();

			CBandsHolder bandHolder(3);
			if (bExtractElev || bExtractSlopeAspect)
				msg = bandHolder.Load(inputDS, true);

			if (msg)
			{
				CProjectionTransformation PT(PRJ_WGS_84, inputDS.GetPrjID());

				//process all point
				for (size_t i = 0; i < locations.size() && msg; i++)
				{
					if (bExtractElev || bExtractSlopeAspect)
					{
						//for all point in the table
						CGeoPoint coordinate = locations[i];
						coordinate.Reproject(PT);


						//find position in the block
						if (extents.IsInside(coordinate))
						{
							CGeoPointIndex xy = extents.CoordToXYPos(coordinate);

							CGeoExtents xy_extent = extents.GetPixelExtents(xy);
							bandHolder.LoadBlock(xy_extent);
							CDataWindowPtr pWin = bandHolder.GetWindow(0, 0, 0, 3, 3);
							float test = pWin->at(1, 1);

							if (bExtractElev)
							{
								if (bReplaceAll || locations[i].m_alt == -999)
								{
									if (pWin->IsValid(1, 1))
										locations[i].m_alt = pWin->at(1, 1);
								}
							}

							if (bExtractSlopeAspect)
							{
								if (bReplaceAll || locations[i].GetSlope() == -999 || locations[i].GetAspect() == -999)
								{
									double slope = -999;
									double aspect = -999;
									pWin->GetSlopeAndAspect(1, 1, slope, aspect);
									locations[i].SetSSI(CLocation::GetDefaultSSIName(CLocation::SLOPE), to_string(slope));
									locations[i].SetSSI(CLocation::GetDefaultSSIName(CLocation::ASPECT), to_string(aspect));
								}
							}
						}
					}
					msg += callback.StepIt();
				}//for all locations
			}//if msg

			inputDS.Close();
		}//if msg

		callback.PopTask();

		return msg;
	}

	//ERMsg CLocDlg::ExtractNominatimName(CLocationVector& locations, bool bReplaceAll, bool bName, bool bState, bool bCountry, CCallback& callback)
	//{
	//	ERMsg msg;


	//	//http://nominatim.openstreetmap.org/reverse?format=json&lat=46.736497&lon=-71.450790

	//	callback.PushTask("Extract location name from nominatim", locations.size());


	//	CHttpConnectionPtr pConnection;
	//	CInternetSessionPtr pSession;
	//	msg += GetHttpConnection("nominatim.openstreetmap.org", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);


	//	if (msg)
	//	{
	//		size_t miss = 0;
	//		for (size_t i = 0; i < locations.size() && msg; i++)
	//		{
	//			bool bMissName = bName && (bReplaceAll || locations[i].m_name.empty());
	//			bool bMissState = bState &&  (bReplaceAll || locations[i].GetSSI("State").empty());
	//			bool bMissCountry = bCountry && (bReplaceAll || locations[i].GetSSI("Country").empty());


	//			if (bMissName || bMissState || bMissCountry)
	//			{

	//				string strGeo;
	//				string URL = "/reverse?zoom=18&format=geojson&lat=" + ToString(locations[i].m_lat) + "&lon=" + ToString(locations[i].m_lon);

	//				msg = UtilWWW::GetPageText(pConnection, URL, strGeo, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	//				if (msg)
	//				{
	//					//extract name from nominatim
	//					string error;
	//					Json geojson = Json::parse(strGeo, error);

	//					if (error.empty())
	//					{
	//						ASSERT(geojson.is_object());
	//						ASSERT(geojson["features"].is_array());
	//						Json::array features = geojson["features"].array_items();
	//						if (features.size() == 1)
	//						{
	//							ASSERT(features[0].is_object());

	//							Json::object feature0 = features[0].object_items();
	//							Json::object properties = feature0["properties"].object_items();
	//							Json::object address = properties["address"].object_items();

	//							string village = ANSI_2_ASCII(address["village"].string_value());
	//							string town = ANSI_2_ASCII(address["town"].string_value());
	//							string suburb = ANSI_2_ASCII(address["suburb"].string_value());
	//							string city = ANSI_2_ASCII(address["city"].string_value());
	//							string county = ANSI_2_ASCII(address["county"].string_value());
	//							string region = ANSI_2_ASCII(address["region"].string_value());

	//							string state = ANSI_2_ASCII(address["state"].string_value());
	//							string country = ANSI_2_ASCII(address["country"].string_value());

	//							
	//							string name;
	//							if (!village.empty())
	//								name = village;
	//							else if (!town.empty())
	//								name = town;
	//							else if (!suburb.empty())
	//								name = suburb;
	//							else if (!city.empty())
	//								name = city;
	//							else if (!county.empty())
	//								name = county;
	//							else if (!region.empty())
	//								name = region;
	//							

	//							if (bMissName && !name.empty())
	//								locations[i].m_name = name;

	//							if (bMissState && !state.empty())
	//								locations[i].SetSSI("State", state);

	//							if (bMissCountry && !country.empty())
	//								locations[i].SetSSI("Country", country);
	//						}
	//						else
	//						{
	//							miss++;
	//						}
	//					}
	//					else
	//					{
	//						if (error.empty())
	//							error = geojson["error"]["message"].string_value();

	//						msg.ajoute(error);
	//					}
	//				}//if msg
	//			}//if empty name

	//			msg += callback.StepIt();
	//		}//for all locations

	//		pConnection->Close();
	//		pSession->Close();
	//	}//if msg

	//	callback.PopTask();

	//	return msg;
	//}





	//ERMsg CLocDlg::ExtractOpenTopoDataElevation(CLocationVector& locations, bool bReplaceAll, size_t eProduct, size_t eInterpol, CCallback& callback)
	//{
	//	ERMsg msg;

	//	//http://api.opentopodata.org/v1/test-dataset?locations=56.35,123.90
	//	callback.PushTask("Extract elevation from Open Topo Data", locations.size());

	//	CHttpConnectionPtr pConnection;
	//	CInternetSessionPtr pSession;
	//	msg += GetHttpConnection("api.opentopodata.org", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true);

	//	//NOAA etopo1 1.8 km (Global, including bathymetry and ice surface elevation near poles)
	//	//NASA srtm	90 m (Latitudes -60 to 60)
	//	//NASA srtm 30 m (Latitudes -60 to 60)
	//	//EEA eudem 25 m (Europe)
	//	//NASA aster 30 m (Global)
	//	//USGS ned 10 m (Continental USA, Hawaii, parts of Alaska)


	//	enum TProduct { NOAA_ETOPO1, NASA_SRTM90M, NASA_SRTM30M, EEQ_UDEM25M, NASA_ASTER30M, USGS_NED10M, NB_PRODUCTS };
	//	enum TInterpol { I_NEAREST, I_BILINEAR, I_CUBIC, NB_INTERPOL };
	//	static const char* PROPDUCT_NAME[NB_PRODUCTS] = { "etopo1", "srtm90m", "srtm30m", "udem25m", "aster30m", "ned10m" };
	//	static const char* INTERPOL_NAME[NB_INTERPOL] = { "nearest", "bilinear", "cubic" };;

	//	ASSERT(eProduct < NB_PRODUCTS);
	//	ASSERT(eInterpol < NB_INTERPOL);
	//	string product = PROPDUCT_NAME[eProduct];
	//	string interpol = INTERPOL_NAME[eInterpol];

	//	if (msg)
	//	{
	//		string URL;

	//		vector<size_t> loc_to_update;
	//		//select locations to uptade
	//		for (size_t i = 0; i < locations.size() && msg; i++)
	//		{
	//			if (locations[i].m_elev == -999 || bReplaceAll)
	//			{
	//				loc_to_update.push_back(i);
	//			}
	//		}

	//		size_t ii = 0;
	//		size_t iii = 0;
	//		for (size_t i = 0; i < loc_to_update.size() && msg; i++)
	//		{
	//			if (ii == 0)
	//				URL = "/v1/" + product + "?&interpolation=" + interpol + "&locations=";
	//			else
	//				URL += "|";

	//			size_t index = loc_to_update[i];
	//			URL += ToString(locations[index].m_lat) + "," + ToString(locations[index].m_lon);
	//			ii++;

	//			if (ii == 100 || i == loc_to_update.size() - 1)
	//			{
	//				ii = 0;
	//				string strGeo;
	//				msg = UtilWWW::GetPageText(pConnection, URL, strGeo, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	//				if (msg)
	//				{
	//					//extract elevation from google
	//					string error;
	//					Json json = Json::parse(strGeo, error);
	//					ASSERT(json.is_object());

	//					if (error.empty() && json["status"] == "OK")
	//					{
	//						ASSERT(json["results"].is_array());

	//						Json::array result = json["results"].array_items();
	//						for (size_t i = 0; i < result.size(); i++, iii++)
	//						{
	//							locations[loc_to_update[iii]].m_elev = result[i]["elevation"].number_value();
	//							msg += callback.StepIt();
	//						}

	//						if (i == loc_to_update.size() - 1)
	//							Sleep(1000);//sleep one second (API limits)
	//					}
	//					else
	//					{
	//						if (error.empty())
	//							error = json["error"].string_value();

	//						msg.ajoute(error);
	//					}
	//				}
	//			}//if no elev


	//		}//for all locations

	//		pConnection->Close();
	//		pSession->Close();


	//		//if (miss)
	//			//SYShowMessage("Some missing values");
	//	}//if msg

	//	callback.PopTask();

	//	return msg;

	//}


	//ERMsg CLocDlg::ExtractShoreDistance(CLocationVector& locations, bool bReplaceAll, CCallback& callback)
	//{
	//	ERMsg msg;

	//	callback.PushTask("Extract shore distance", locations.size());


	//	//process all point
	//	for (size_t i = 0; i < locations.size() && msg; i++)
	//	{
	//		if (bReplaceAll || locations[i].GetDefaultSSI(CLocation::SHORE_DIST).empty())
	//		{
	//			double d = CShore::GetShoreDistance(locations[i]) / 1000.0;//distance in km
	//			locations[i].SetSSI(CLocation::GetDefaultSSIName(CLocation::SHORE_DIST), ToString(d, 1));
	//		}

	//		msg += callback.StepIt();
	//	}//for all locations

	//	callback.PopTask();

	//	return msg;
	//}

	//************************************************************************
	CLocationsFileManagerDlg::CLocationsFileManagerDlg(CWnd* pParent) :
		CDialog(CLocationsFileManagerDlg::IDD, pParent)
	{
	}

	CLocationsFileManagerDlg::~CLocationsFileManagerDlg()
	{
	}


	void CLocationsFileManagerDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_MODELIN_LIST, m_fileListCtrl);
		DDX_Control(pDX, IDC_MODELIN_FILEPATH, m_filePathCtrl);
	}


	BEGIN_MESSAGE_MAP(CLocationsFileManagerDlg, CDialog)
		ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
		ON_NOTIFY(ON_BLB_SELCHANGE, IDC_MODELIN_LIST, OnSelChange)
		ON_NOTIFY(ON_BLB_NAMECHANGE, IDC_MODELIN_LIST, OnSelChange)
		ON_WM_ACTIVATEAPP()
	END_MESSAGE_MAP()



	/////////////////////////////////////////////////////////////////////////////
	// CLocationsFileManagerDlg msg handlers

	BOOL CLocationsFileManagerDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		CAppOption option(_T("WindowsPosition"));

		CPoint pt = option.GetProfilePoint(_T("LocationsFileManager"), CPoint(30, 30));
		UtilWin::EnsurePointOnDisplay(pt);
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		//CRect curRect;
		//GetWindowRect(curRect);
		//curRect.MoveToXY(30, 30);
		//CRect rect = option.GetProfileRect(_T("LocFileManager"), curRect);
		//UtilWin::EnsureRectangleOnDisplay(rect);
		//MoveWindow(rect);

		//select current loc input
		m_fileListCtrl.SelectString(m_locName);

		UpdateCtrl();
		return TRUE;  // return TRUE unless you set the focus to a control
	}

	void CLocationsFileManagerDlg::UpdateCtrl()
	{

		BOOL bEnable = m_fileListCtrl.GetSelectedCount() == 1;


		CWnd* pOKCtrl = GetDlgItem(IDOK);
		ASSERT(pOKCtrl);

		pOKCtrl->EnableWindow(bEnable);


		int iItem = m_fileListCtrl.GetSelItem();
		string name = ToUTF8(m_fileListCtrl.GetItemText(iItem));

		string filePath = iItem >= 0 ? WBSF::GetFM().Loc().GetFilePath(name) : WBSF::GetFM().Loc().GetLocalPath();
		m_filePathCtrl.SetWindowText(filePath);

	}

	void CLocationsFileManagerDlg::OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult)
	{
		UpdateCtrl();
	}

	void CLocationsFileManagerDlg::OnOK()
	{
		ERMsg msg;

		ASSERT(m_fileListCtrl.GetSelectedCount() == 1);

		m_locName = m_fileListCtrl.GetWindowText();
		if (m_fileListCtrl.GetSelItem() >= 0)
			msg = m_fileListCtrl.SaveLoc(m_locName, false);

		if (!msg)
		{
			SYShowMessage(msg, this);
			return;
		}

		CDialog::OnOK();
	}

	BOOL CLocationsFileManagerDlg::DestroyWindow()
	{
		CRect rect;
		GetWindowRect(rect);

		CAppOption option(_T("WindowsPosition"));
		CPoint pt = rect.TopLeft();
		option.WriteProfilePoint(_T("LocationsFileManager"), pt);
		BOOL bDestroy = CDialog::DestroyWindow();

		ASSERT(GetSafeHwnd() == NULL);

		return bDestroy;
	}

	void CLocationsFileManagerDlg::OnActivateApp(BOOL bActive, DWORD dwThreadID)
	{
		if (GetSafeHwnd())
		{
			CDialog::OnActivateApp(bActive, dwThreadID);

			if (bActive && IsWindowVisible())
				m_fileListCtrl.OnAppActivate();
		}

	}

	LRESULT CLocationsFileManagerDlg::OnKickIdle(WPARAM w, LPARAM l)
	{
		if (m_fileListCtrl.m_pLocDlg->GetSafeHwnd())
			return m_fileListCtrl.m_pLocDlg->SendMessage(WM_KICKIDLE, w, l);

		return 0;
	}

	//********************************************************************************
	// CModelsPage property page

	BEGIN_MESSAGE_MAP(CLocListBox, CBioSIMListBox)
	END_MESSAGE_MAP()



	CLocListBox::CLocListBox()
	{
		m_pLocDlg = new CLocDlg();
		m_lastSelection = -1;
	}

	CLocListBox::~CLocListBox()
	{
		ASSERT(m_pLocDlg);

		if (m_pLocDlg->m_hWnd != NULL)
			m_pLocDlg->DestroyWindow();

		delete m_pLocDlg;
		m_pLocDlg = NULL;
	}

	CWnd* CLocListBox::OnCreateList()
	{
		CWnd* pWnd = CBioSIMListBox::OnCreateList();

		if (pWnd == NULL)
			return FALSE;

		SetStandardButtons(AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_COPY | AFX_VSLISTBOX_BTN_EDIT | AFX_VSLISTBOX_BTN_SHOWMAP | AFX_VSLISTBOX_BTN_EXCEL);
		OnInitList();

		ENSURE(m_pLocDlg->Create(GetParent()));
		m_pLocDlg->EnableWindow(FALSE);
		m_pLocDlg->m_pLocEditDlg = GetParent();

		return m_pWndList;
	}

	void CLocListBox::OnAfterAddItem(int iItem)
	{
		string locName = ToUTF8(GetItemText(iItem));

		//save an empty loc
		CLocationVector loc;
		ERMsg msg = WBSF::GetFM().Loc().Set(locName, loc);
		if (msg)
		{
			m_pLocDlg->SetLoc(loc);
			m_pLocDlg->EnableWindow(TRUE);
			m_lastSelection = iItem;
			string filePath = WBSF::GetFM().Loc().GetFilePath(locName);
			m_lastFileStamp.SetFileStamp(filePath);
		}
		else
		{
			RemoveItem(iItem);
			SYShowMessage(msg, this);
		}
	}

	BOOL CLocListBox::OnBeforeCopyItem(int iItem, CString newName)
	{
		ASSERT(m_lastSelection);

		/*if (m_lastSelection >= 0)
			if (!SaveLoc(GetItemText(m_lastSelection), false))
				return FALSE;*/

		BOOL bRep = CBioSIMListBox::OnBeforeCopyItem(iItem, newName);
		if (bRep)
		{
			//m_lastSelection = iItem;//By RSA 2019-01-24
			CString locName = newName;

			CLocationVector loc;
			LoadLoc(locName, loc);
			m_pLocDlg->SetLoc(loc);
		}

		return bRep;
	}

	void CLocListBox::OnShowMap(int iItem)
	{
		if (iItem >= 0)
		{
			CString locName = GetItemText(iItem);

			if (!SaveLoc(locName, false))
				return;

			CBioSIMListBox::OnShowMap(iItem);
		}
	}


	void CLocListBox::OnExcel(int iItem)
	{
		if (iItem >= 0)
		{
			CString locName = GetItemText(iItem);

			if (!SaveLoc(locName, false))
				return;

			CBioSIMListBox::OnExcel(iItem);
		}
	}

	void CLocListBox::OnSelectionChanged()
	{
		ERMsg msg;

		//event if they have error, we don't take care of
		if (m_lastSelection != -1)
		{
			SaveLoc(GetItemText(m_lastSelection));
		}


		int newSel = GetSelItem();
		m_lastSelection = newSel;
		if (newSel != -1)
		{
			CString locName = GetItemText(newSel);
			if (!locName.IsEmpty())
			{
				CLocationVector loc;
				msg = LoadLoc(locName, loc);
				m_pLocDlg->SetLoc(loc);
				m_pLocDlg->EnableWindow(msg ? TRUE : FALSE);
			}
			else
			{
				m_pLocDlg->EnableWindow(FALSE);
			}
		}
		else
		{
			m_pLocDlg->EnableWindow(FALSE);
		}

		//call parent
		CBioSIMListBox::OnSelectionChanged();
	}

	ERMsg CLocListBox::LoadLoc(const CString& locName, CLocationVector& loc)
	{
		ERMsg msg;
		msg = WBSF::GetFM().Loc().Get(ToUTF8(locName), loc);
		if (msg)
		{
			msg = loc.IsValid();
			if (msg)
			{
				string filePath = WBSF::GetFM().Loc().GetFilePath(ToUTF8(locName));
				m_lastFileStamp.SetFileStamp(filePath);
			}
			else
			{
				SYShowMessage(msg, GetParent());
			}
		}

		return msg;
	}

	ERMsg CLocListBox::SaveLoc(const CString& locName, bool askConfirm)
	{
		ERMsg msg;


		if (!locName.IsEmpty() && m_pLocDlg->GetHaveChange())
		{
			bool bSave = true;
			if (askConfirm)
			{
				CString sOutMessage;
				sOutMessage.FormatMessage(IDS_BSC_CONFIRM_SAVE, locName);
				bSave = MessageBox(sOutMessage, AfxGetAppName(), MB_YESNO) == IDYES;
			}

			if (bSave)
			{
				CLocationVector loc;
				m_pLocDlg->GetLoc(loc);
				msg = WBSF::GetFM().Loc().Set(ToUTF8(locName), loc);
				if (!msg)
					SYShowMessage(msg, GetParent());
			}
		}

		return msg;
	}

	void CLocListBox::OnAppActivate()
	{
		if (m_lastSelection >= 0)
		{
			string locName = ToUTF8(GetItemText(m_lastSelection));
			string filePath = WBSF::GetFM().Loc().GetFilePath(locName);
			CFileStamp fileStamp(filePath);
			if (fileStamp != m_lastFileStamp)
			{
				CLocationVector loc;
				ERMsg msg = LoadLoc(Convert(locName), loc);
				if (msg)
				{
					m_pLocDlg->SetLoc(loc);
					m_pLocDlg->EnableWindow(TRUE);
				}
			}
		}
	}




}