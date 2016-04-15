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


#include "MTParser/MTParser.h"
#include "Basic/UtilMath.h"
#include "ModelBase/ModelInputParameter.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"
#include "FunctionAnalysisTestDlg.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::DIMENSION;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace WBSF
{
	void CInputVariableGridCtrl::OnSetup()
	{
		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);



		CStringArrayEx title(IDS_FUNCTION_TEST_CTRL);
		ASSERT(title.GetSize() == 2);

		int nbRows = (int)m_variableVector.size();

		SetNumberCols((int)title.GetSize() - 1);
		SetNumberRows(nbRows);

		//SetColWidth(-1, 150);

		for (int i = 0; i < title.GetSize(); i++)
		{
			QuickSetText(i - 1, -1, title[i]);
		}

		ASSERT(m_variableVector.size() == GetNumberRows());
		for (int i = 0; i < m_variableVector.size(); i++)
		{
			QuickSetText(-1, i, UtilWin::Convert(m_variableVector[i]));
			//fill variable with random value from 1 to 10
			QuickSetText(0, i, UtilWin::ToCString(WBSF::Rand(0, 10)));
		}



		BestFit(-1, 0, 0, UG_BESTFIT_TOPHEADINGS);


		EnableToolTips();

	}

	void CInputVariableGridCtrl::GetData(CModelInput& modelInput)
	{
		modelInput.clear();

		ASSERT(m_variableVector.size() == GetNumberRows());
		for (int i = 0; i < m_variableVector.size(); i++)
			modelInput.push_back(CModelInputParam(m_variableVector[i], /*CModelInputParameterDef::kMVReal,*/ (LPCSTR)CStringA(QuickGetText(0, i))));

	}

	//*************************************************************************************************

	void COutputGridCtrl::OnSetup()
	{
		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);

		CStringArrayEx title(IDS_FUNCTION_TEST_CTRL);
		ASSERT(title.GetSize() == 2);

		SetNumberCols((int)title.GetSize() - 1);
		SetNumberRows((int)m_functionVector.size());

		for (int i = 0; i < title.GetSize(); i++)
		{
			QuickSetText(i - 1, -1, title[i]);
		}

		for (int i = 0; i < m_functionVector.size(); i++)
		{
			QuickSetText(-1, i, UtilWin::Convert(m_functionVector[i].m_name));
		}

		BestFit(-1, 0, 0, UG_BESTFIT_TOPHEADINGS);

		EnableToolTips();

	}



	ERMsg COutputGridCtrl::Execute(const CModelInput& modelInput)
	{
		ERMsg msg;

		for (size_t i = 0; i < m_functionVector.size(); i++)
		{
			MTParser parser;
			parser.defineFunc(CreateGetJDayFct());
			parser.defineFunc(CreateDropYearFct());
			parser.defineConst(_T("VMISS"), VMISS);
			parser.enableAutoVarDefinition(true);
			parser.compile(UtilWin::Convert(m_functionVector[i].m_equation));

			vector<double> vars(parser.getNbUsedVars());

			for (unsigned int t = 0; t < parser.getNbUsedVars(); t++)
			{
				int pos = modelInput.FindVariable(UtilWin::ToUTF8(parser.getUsedVar(t)));
				ASSERT(pos >= 0);

				vars[t] = modelInput[pos].GetDouble();
				parser.redefineVar(parser.getUsedVar(t).c_str(), &(vars[t]));
			}

			double value = parser.evaluate();

			QuickSetText(0, (int)i, UtilWin::ToCString(value));
		}


		RedrawAll();

		return msg;
	}

	//*************************************************************************************************

	/////////////////////////////////////////////////////////////////////////////
	// CFunctionAnalysisTestDlg dialog


	CFunctionAnalysisTestDlg::CFunctionAnalysisTestDlg(CWnd* pParentWnd/*=NULL*/) :
		CDialog(CFunctionAnalysisTestDlg::IDD, pParentWnd)
	{
	}


	void CFunctionAnalysisTestDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
	}



	BEGIN_MESSAGE_MAP(CFunctionAnalysisTestDlg, CDialog)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CFunctionAnalysisTestDlg msg handlers

	BOOL CFunctionAnalysisTestDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		VERIFY(m_inputCtrl.AttachGrid(this, IDC_INPUT_VARIABLE));
		VERIFY(m_outputCtrl.AttachGrid(this, IDC_OUTPUT_VARIABLE));

		OnOK();

		return TRUE;
	}

	void CFunctionAnalysisTestDlg::SetFunctionArray(CFunctionDefArray& functionVector)
	{

		//Set Function Array
		m_outputCtrl.m_functionVector = functionVector;

		//Set Input Variables
		m_inputCtrl.m_variableVector.clear();

		std::vector<std::string> nameList;
		for (size_t i = 0; i < m_outputCtrl.m_functionVector.size(); i++)
		{
			MTParser parser;
			parser.defineFunc(CreateGetJDayFct());
			parser.defineFunc(CreateDropYearFct());
			parser.defineConst(_T("VMISS"), VMISS);
			parser.enableAutoVarDefinition(true);
			parser.compile(UtilWin::Convert(functionVector[i].m_equation));

			for (unsigned int t = 0; t < parser.getNbUsedVars(); t++)
			{
				string name = UTF8(parser.getUsedVar(t));
				MakeUpper(name);

				//if (nameList.Lookup(name) == -1)
				if (std::find(nameList.begin(), nameList.end(), name) == nameList.end())
				{
					nameList.push_back(name);
					m_inputCtrl.m_variableVector.push_back(UtilWin::ToUTF8(parser.getUsedVar(t)));
				}
			}
		}
	}


	void CFunctionAnalysisTestDlg::OnOK()
	{
		CModelInput modelInput;
		m_inputCtrl.GetData(modelInput);

		ERMsg msg = m_outputCtrl.Execute(modelInput);
		if (!msg)
			UtilWin::SYShowMessage(msg, this);

	}

}