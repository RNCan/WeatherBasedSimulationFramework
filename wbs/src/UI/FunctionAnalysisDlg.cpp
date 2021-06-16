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

#include "UltimateGrid/UGCtrlDDX.h"
#include "MTParser/MTParserInfoDlg.h"
#include "MTParser/MTParser.h"
#include "FileManager/FileManager.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "FunctionAnalysisDlg.h"
#include "FunctionAnalysisTestDlg.h"
#include "WeatherBasedSimulationString.h"

using namespace UtilWin;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace WBSF
{

	static const UINT ID_CONTEXT_MENU_COPY = 1001;

	BEGIN_MESSAGE_MAP(CDimensionGridCtrl, CUGCtrl)
	END_MESSAGE_MAP()

	const int CDimensionGridCtrl::COL_DIM[NB_COLS] = { LOCATION, TIME_REF, VARIABLE };
	const int CDimensionGridCtrl::COL_SIZE[NB_COLS] = { CLocation::NB_MEMBER - CLocation::LAT, int(CTRefFormat::HOUR), 0 };
	int CDimensionGridCtrl::MinimumRows()
	{
		int nbRows = 0;
		for (int i = 0; i < NB_COLS; i++)
			nbRows += COL_SIZE[i];


		return nbRows;
	}

	void CDimensionGridCtrl::OnSetup()
	{
		SetDefRowHeight(MulDiv(m_GI->m_defRowHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetTH_Height(MulDiv(m_GI->m_topHdgHeight, GetWindowDPI(GetSafeHwnd()), 96));
		SetHS_Height(MulDiv(m_GI->m_hScrollHeight, GetWindowDPI(GetSafeHwnd()), 96));

		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);

		//	SetCurrentCellMode(2);
		EnableMenu(TRUE);
		SetVScrollMode(UG_SCROLLTRACKING);

		CStringArrayEx dimensionLable(IDS_SIM_RESULT_HEAD);

		int nbCols = 1;//NB_COLS;
		int nbRows = MinimumRows();

		SetNumberCols(nbCols);
		SetNumberRows(nbRows);

		ASSERT(dimensionLable.GetSize() >= NB_COLS);
		QuickSetText(0, -1, dimensionLable[VARIABLE]);

		SetColWidth(-1, 0);
		//BestFit(CL,CT,0,UG_BESTFIT_TOPHEADINGS );

		EnableToolTips();

	}


	void CDimensionGridCtrl::Initialize(const WBSF::StringVector& variables)
	{

		long nbRows = (long)variables.size();
		SetNumberRows(nbRows);


		int row = 0;
		for (size_t i = 0; i < variables.size(); i++, row++)
		{
			QuickSetText(0, row, Convert(variables[i]));
		}



		BestFit(0, 0, 0, UG_BESTFIT_TOPHEADINGS);

		RedrawAll();
	}

	int CDimensionGridCtrl::OnMenuStart(int col, long row, int section)
	{
		if (section == UG_GRID)
		{
			//****** Empty the Menu!!
			EmptyMenu();

			//******* Add the Menu Items
			CWnd* pParent = GetParent();
			if (pParent)
			{
				AddMenuItem(ID_CONTEXT_MENU_COPY, UtilWin::GetCString(IDS_CMN_AFXBARRES_COPY));
			}
		}

		return TRUE;

	}


	void CDimensionGridCtrl::OnMenuCommand(int col, long row, int section, int item)
	{
		if (section == UG_GRID)
		{
			//****** The user has selected the 'Copy' option
			if (item == ID_CONTEXT_MENU_COPY)
				CopySelected();
		}
	}

	void CDimensionGridCtrl::OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
	{
		CWnd* pParent = GetParent();
		if (pParent)
		{
			pParent->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), STN_DBLCLK), (LPARAM)m_hWnd);
		}
	}

	void CDimensionGridCtrl::OnKeyDown(UINT *vcKey, BOOL processed)
	{
		UNREFERENCED_PARAMETER(processed);

		if (GetKeyState(VK_CONTROL) < 0 &&
			(*vcKey == 'C' || *vcKey == VK_INSERT))
			CopySelected();
	}

	/////////////////////////////////////////////////////////////////////////////
	// CFunctionAnalysisDlg dialog

	BEGIN_MESSAGE_MAP(CFunctionAnalysisDlg, CDialog)
		ON_BN_CLICKED(IDC_TEST, &CFunctionAnalysisDlg::OnTest)
		ON_BN_CLICKED(IDC_FUNCTION_HELP, &CFunctionAnalysisDlg::OnHelp)
		ON_STN_DBLCLK(IDC_VARIABLE_LIST, &CFunctionAnalysisDlg::OnStnDblclickVariableList)

	END_MESSAGE_MAP()


	CFunctionAnalysisDlg::CFunctionAnalysisDlg(const CExecutablePtr& pParent, CWnd* pParentWnd/*=NULL*/) :
		CDialog(CFunctionAnalysisDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CFunctionAnalysisDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		m_functionCtrl.m_listSize = 130;
		DDX_ControlGrid(pDX, IDC_VARIABLE_LIST, m_dimensionCtrl);
		DDX_Control(pDX, IDC_FUNCTION_LIST, m_functionCtrl);

		DDX_Text(pDX, IDC_NAME, m_analysis.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_analysis.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_analysis.m_internalName);

		if (pDX->m_bSaveAndValidate)
		{
			m_functionCtrl.GetData(m_analysis.m_functionArray);
		}
		else
		{
			m_functionCtrl.HideMember(CModelOutputVariableDef::DESCRIPTION);
			//m_functionCtrl.HideMember(CModelOutputVariableDef::CLIMATIC_VARIABLE);

			CParentInfo info;
			m_pParent->GetParentInfo(WBSF::GetFM(), info, VARIABLE);
			WBSF::StringVector variables = info.GetDimensionList(VARIABLE);
			m_dimensionCtrl.Initialize(variables);

			m_functionCtrl.SetData(m_analysis.m_functionArray);
		}
	}


	void CFunctionAnalysisDlg::OnTest()
	{
		ERMsg msg = Compile();

		if (msg)
		{

			CFunctionAnalysisTestDlg dlg(this);

			CFunctionDefArray functionVector;
			m_functionCtrl.GetData(functionVector);

			dlg.SetFunctionArray(functionVector);
			dlg.DoModal();
		}
		else
		{
			SYShowMessage(msg, this);
		}

	}

	ERMsg CFunctionAnalysisDlg::Compile()
	{
		ERMsg msg;

		//Get parent variables
		CParentInfo info;
		m_pParent->GetParentInfo(WBSF::GetFM(), info, VARIABLE);

		//Get equations
		CFunctionDefArray functionVector;
		m_functionCtrl.GetData(functionVector);

		//Create and compile all equations
		for (size_t i = 0; i < functionVector.size(); i++)
		{
			CFunctionContext function;
			msg += function.Compile(info.m_variables, functionVector[i].m_equation);
		}

		return msg;
	}

	void CFunctionAnalysisDlg::OnHelp()
	{
		MTParser parser;
		parser.defineFunc(CreateGetJDayFct());
		parser.defineFunc(CreateJDay2DateFct());
		parser.defineFunc(CreateDropYearFct());
		parser.defineFunc(CreateGetMonthFct());
		
		parser.defineConst(_T("VMISS"), VMISS);

		CMTParserInfoDlg dlg(&parser, this);
		dlg.DoModal();
	}


	void CFunctionAnalysisDlg::OnStnDblclickVariableList()
	{
		int col = 0;
		long row = 0;
		m_dimensionCtrl.EnumFirstSelected(&col, &row);

		//	std::string name = WBSF::UTF8(m_dimensionCtrl.QuickGetText(col, row));
		CParentInfo info;
		m_pParent->GetParentInfo(WBSF::GetFM(), info, VARIABLE);
		if (row >= 0 && row < info.m_variables.size())
		{
			CFunctionDef functionDef;

			functionDef.m_name = info.m_variables[row].m_name;
			functionDef.m_title = info.m_variables[row].m_title;
			functionDef.m_units = info.m_variables[row].m_units;
			functionDef.m_equation = info.m_variables[row].m_name;
			functionDef.m_climaticVariable = info.m_variables[row].m_climaticVariable;
			functionDef.m_TM = info.m_variables[row].m_TM;

			m_functionCtrl.Add(functionDef);
		}
	}

}
