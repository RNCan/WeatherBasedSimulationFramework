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


#include "Basic/UtilMath.h"
#include "Basic/CSV.h"
#include "FileManager/FileManager.h"
#include "ModelBase/ModelInputParameter.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/MFCEditBrowseCtrlEx.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/UtilWin.h"
#include "ModelInputDlg.h"

#include "WeatherBasedSimulationUI.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	class CTitleCtrl : public CStatic
	{
	public:

		void OnSize(UINT nType, int cx, int cy)
		{
			CStatic::OnSize(nType, cx, cy);
			Invalidate(FALSE);
		}

		//*****************************************************************************
		BOOL PreCreateWindow(CREATESTRUCT &cs)
		{
			if (!CStatic::PreCreateWindow(cs))
				return FALSE;

			cs.dwExStyle &= !SS_SUNKEN;

			return TRUE;
		}


		DECLARE_MESSAGE_MAP()

	};

	BEGIN_MESSAGE_MAP(CTitleCtrl, CStatic)
	END_MESSAGE_MAP()


	class CStaticLineCtrl : public CStatic
	{
	public:

		void OnSize(UINT nType, int cx, int cy)
		{
			CStatic::OnSize(nType, cx, cy);
			Invalidate(FALSE);
		}

		//*****************************************************************************
		BOOL PreCreateWindow(CREATESTRUCT &cs)
		{
			if (!CStatic::PreCreateWindow(cs))
				return FALSE;

			cs.dwExStyle |= SS_TYPEMASK;
			cs.dwExStyle |= SS_OWNERDRAW;
			//cs.dwExStyle |= SS_GRAYFRAME;
			//cs.dwExStyle |= SS_GRAYRECT;
			cs.dwExStyle &= !SS_SUNKEN;

			return TRUE;
		}

		//*****************************************************************************
		void PreSubclassWindow()
		{
			CStatic::PreSubclassWindow();
			ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW);
		}

		//*****************************************************************************
		virtual void DrawItem(LPDRAWITEMSTRUCT pdis)
		{
			CDC *pDC = CDC::FromHandle(pdis->hDC);

			CRect rect = pdis->rcItem;
			//pDC->Draw3dRect(rect, RGB(0, 0, 0), RGB(0, 0, 0));
			//rect.DeflateRect(1, 1);

			pDC->MoveTo(rect.left, (rect.top + rect.bottom) / 2);
			pDC->LineTo(rect.right, (rect.top + rect.bottom) / 2);
		}

		DECLARE_MESSAGE_MAP()

	};

	BEGIN_MESSAGE_MAP(CStaticLineCtrl, CStatic)
	END_MESSAGE_MAP()

	class CModelInputBrowseCtrl : public CMFCEditBrowseCtrlEx
	{
	public:
		virtual void OnBrowse();
		string m_appPath;
		string m_projectPath;
	};

	void CModelInputBrowseCtrl::OnBrowse()
	{
		string filePath = WBSF::SpecialPath2FilePath(GetString(), m_appPath, m_projectPath);
		SetWindowText(Convert(filePath));

		CMFCEditBrowseCtrlEx::OnBrowse();

		string specialPath = WBSF::FilePath2SpecialPath(GetString(), m_appPath, m_projectPath);
		SetWindowText(specialPath);

	}
	/////////////////////////////////////////////////////////////////////////////
	// CModelInputDlg

	CModelInputDlg::CModelInputDlg() :
		m_bDefault(true)
	{
		ASSERT(CString().LoadString(IDS_STR_YES));
	}

	CModelInputDlg::~CModelInputDlg()
	{
		ResetCtrl();
	}


	BEGIN_MESSAGE_MAP(CModelInputDlg, CDialog)
		ON_WM_CLOSE()
		ON_WM_DESTROY()
	END_MESSAGE_MAP()


	/////////////////////////////////////////////////////////////////////////////
	// CModelInputDlg message handlers

	//void GetPrimaryScreenSizeInPixels( CRect& rect)
	//{
	//	// Start With Nothing
	//	//ZeroMemory( &s, sizeof(SIZE) );
	//
	//	// Get Width/Height
	//	rect.left = (LONG)::GetSystemMetrics( SM_CXFULLSCREEN );
	//	rect.bottom = (LONG)::GetSystemMetrics( SM_CYFULLSCREEN );
	//
	//	MONITORINFO mi;
	//	mi.cbSize = sizeof(mi);
	//}

	BOOL CModelInputDlg::Create(const CModel& model, CWnd* pParentWnd, bool bForTest)
	{
		return Create(model.GetInputDefinition(), model.GetRect(), Convert(model.GetName()), pParentWnd, bForTest);
	}


	void CModelInputDlg::ResetCtrl()
	{
		m_variables.clear();

		for (int i = 0; i < m_titleCtrlArray.GetSize(); i++)
			delete m_titleCtrlArray[i];

		m_titleCtrlArray.RemoveAll();

		for (int i = 0; i < m_varCtrlArray.GetSize(); i++)
			delete m_varCtrlArray[i];

		m_varCtrlArray.RemoveAll();
	}

	BOOL CModelInputDlg::Create(const CModelInputParameterDefVector& variables, const CRect& rectMI, const CString& modelName, CWnd* pParentWnd, bool bForTest)
	{
		BOOL rep = false;

		ResetCtrl();

		m_variables = variables;

		CRect rectDlg(rectMI);

		CAppOption option;
		option.SetCurrentProfile(_T("ModelsWindowsPosition"));
		CPoint defaultPos = option.GetProfilePoint(modelName, CPoint(430, 0));

		option.SetCurrentProfile(_T("WindowsPosition"));
		defaultPos += option.GetProfilePoint(_T("ModelInputEditor"), CPoint(30, 30));

		rectDlg += defaultPos;
		UtilWin::EnsureRectangleOnDisplay(rectDlg);


		CWinApp* pApp = AfxGetApp();

		CString strWndClass = AfxRegisterWndClass(
			0,
			pApp->LoadStandardCursor(IDC_ARROW),
			(HBRUSH)(COLOR_3DFACE + 1),
			pApp->LoadIcon(IDI_HEART));

		if (bForTest)
			rep = CWnd::CreateEx(WS_EX_CLIENTEDGE, strWndClass, modelName, WS_DLGFRAME | WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU, rectDlg, pParentWnd, NULL, NULL);
		else
			rep = CWnd::CreateEx(WS_EX_CLIENTEDGE, strWndClass, modelName, WS_DLGFRAME | WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE, rectDlg, pParentWnd, NULL, NULL);



		CFont* pFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)); // SYSTEM_FONT ANSI_VAR_FONT

		for (size_t i = 0; i < m_variables.size(); i++)
		{
			//create title
			CStatic* pStatic = (CStatic*) new CStatic;
			CRect rect = m_variables[i].GetItemRect(0);
			//rect.bottom += 3 * rect.Height();

			pStatic->Create(CString(m_variables[i].m_caption.c_str()), WS_CHILD | WS_VISIBLE, rect, this, IDC_STATIC);
			pStatic->SetFont(pFont);
			m_titleCtrlArray.Add(pStatic);

			//create control
			switch (m_variables[i].GetType())
			{
			case CModelInputParameterDef::kMVBool:
			{
				CCFLComboBox* pCombo = (CCFLComboBox*) new CCFLComboBox;
				CRect rect = m_variables[i].GetItemRect(1);
				rect.bottom += 3 * rect.Height();

				pCombo->Create(WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, GetID(i));
				pCombo->SetFont(pFont);

				CString str;
				str.LoadString(IDS_STR_YES);
				pCombo->AddString(str);
				str.LoadString(IDS_STR_NO);
				pCombo->AddString(str);

				m_varCtrlArray.Add(pCombo);
				break;
			}

			case CModelInputParameterDef::kMVInt:
			{
				CIntEdit* pIntEdit = (CIntEdit*) new CIntEdit;
				CRect rect = m_variables[i].GetItemRect(1);


				pIntEdit->CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, GetID(i));
				pIntEdit->SetFont(pFont);
				m_varCtrlArray.Add(pIntEdit);
				break;
			}

			case CModelInputParameterDef::kMVReal:
			{
				CFloatEdit* pFloatEdit = (CFloatEdit*) new CFloatEdit;
				CRect rect = m_variables[i].GetItemRect(1);

				pFloatEdit->CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, GetID(i));
				pFloatEdit->SetFont(pFont);
				m_varCtrlArray.Add(pFloatEdit);
				break;
			}



			case CModelInputParameterDef::kMVString:
			{
				CCFLEdit* pEdit = (CCFLEdit*) new CCFLEdit;
				CRect rect = m_variables[i].GetItemRect(1);


				pEdit->CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, rect, this, GetID(i));
				pEdit->SetFont(pFont);
				m_varCtrlArray.Add(pEdit);
				break;
			}

			case CModelInputParameterDef::kMVFile:
			{
				CModelInputBrowseCtrl* pEdit = new CModelInputBrowseCtrl;
				pEdit->m_appPath = m_appPath;
				pEdit->m_projectPath = m_projectPath;

				CRect rect = m_variables[i].GetItemRect(1);

				pEdit->Create(WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, rect, this, GetID(i));
				pEdit->EnableFileBrowseButton();
				pEdit->SetFont(pFont);
				m_varCtrlArray.Add(pEdit);
				break;
			}

			case CModelInputParameterDef::kMVListByPos:
			case CModelInputParameterDef::kMVListByString:
			case CModelInputParameterDef::kMVListByCSV:
			{
				//int defPos = WBSF::ToInt(m_variables[i].m_default);
				//if (CModelInputParameterDef::kMVListByCSV)

				StringVector listOfValues = m_variables[i].GetList(GetFM().GetAppPath(), GetFM().GetProjectPath());
				bool bFromFile = m_variables[i].IsExtendedList();//listOfValues[0] == "FROM_CSV_FILE" && listOfValues.size() == 4;

				CCFLComboBox* pCombo = (CCFLComboBox*) new CCFLComboBox;
				CRect rect = m_variables[i].GetItemRect(1);
				rect.bottom += 10 * rect.Height();

				UINT style = bFromFile ? WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_SORT : WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST;
				pCombo->Create(style, rect, this, GetID(i));
				pCombo->SetFont(pFont);
				pCombo->SetExtendedUI(true);


				/*if (bFromFile)
				{
				string filePath = CModelInput::SpecialPath2FilePath(listOfValues[1], m_appPath, m_projectPath);
				int col = WBSF::ToInt(listOfValues[2]);

				listOfValues.clear();

				ifStream file;
				if (file.open(filePath))
				{
				for (CSVIterator loop(file, ",; \t"); loop != CSVIterator(); ++loop)
				{

				if (col < loop.Header().size() &&
				col < (*loop).size())
				{
				listOfValues.push_back((*loop)[col]);
				}
				}
				}
				}*/

				for (size_t pos = 0; pos < listOfValues.size(); pos++)
				{
					int index = pCombo->AddString(CString(listOfValues[pos].c_str()));
					pCombo->SetItemData(index, pos);
				}

				if (m_variables[i].GetType() == CModelInputParameterDef::kMVListByPos)
					pCombo->SetCurSel(WBSF::ToInt(m_variables[i].m_default));
				else
					pCombo->SelectStringExact(0, m_variables[i].m_default);


				m_varCtrlArray.Add(pCombo);
				break;
			}

			case CModelInputParameterDef::kMVTitle:
			{
				CTitleCtrl* pStatic = (CTitleCtrl*) new CTitleCtrl;
				CRect rect = m_variables[i].GetItemRect(1);

				pStatic->Create(CString(m_variables[i].m_caption.c_str()), WS_CHILD | WS_VISIBLE, rect, this, IDC_STATIC);
				m_varCtrlArray.Add(pStatic);
				break;
			}

			case CModelInputParameterDef::kMVLine:
			{

				CStaticLineCtrl* pStatic = (CStaticLineCtrl*) new CStaticLineCtrl;//hiden control
				CRect rect = m_variables[i].GetItemRect(1);
				//rect.bottom = rect.top + 5;

				pStatic->Create(_T(""), WS_CHILD | WS_VISIBLE, rect, this, IDC_STATIC);
				m_varCtrlArray.Add(pStatic);
				break;
			}

			default: ASSERT(false);
			}
		}

		Invalidate();

		return rep;
	}

	void CModelInputDlg::SetModelInput(CModelInput& modelInput, bool bDefault)
	{
		ASSERT(m_titleCtrlArray.GetSize() == m_varCtrlArray.GetSize());
		ASSERT(m_titleCtrlArray.GetSize() == (int)m_variables.size());

		m_modelInput = modelInput;
		m_bDefault = bDefault;

		for (int i = 0; i < m_variables.size() && i < modelInput.size(); i++)
		{
			CStatic* pStatic = (CStatic*)m_titleCtrlArray[i];
			pStatic->EnableWindow(!m_bDefault);

			switch (m_variables[i].GetType())
			{
			case CModelInputParameterDef::kMVBool:
			{
				CComboBox* pCombo = (CComboBox*)m_varCtrlArray[i];
				pCombo->SetCurSel(modelInput[i].GetBool() ? 0 : 1);
				pCombo->EnableWindow(!m_bDefault);
				break;
			}

			case CModelInputParameterDef::kMVInt:
			case CModelInputParameterDef::kMVReal:
			case CModelInputParameterDef::kMVString:
			case CModelInputParameterDef::kMVFile:
			{
				CCFLEdit* pEdit = (CCFLEdit*)m_varCtrlArray[i];
				pEdit->SetString(modelInput[i].GetStr());
				pEdit->EnableWindow(!m_bDefault);
				break;
			}

			case CModelInputParameterDef::kMVListByPos:
			{
				CCFLComboBox* pCombo = (CCFLComboBox*)m_varCtrlArray[i];
				pCombo->SelectFromItemData(modelInput[i].GetListIndex());
				pCombo->EnableWindow(!m_bDefault);
				break;
			}

			case CModelInputParameterDef::kMVListByString:
			case CModelInputParameterDef::kMVListByCSV:
			{
				CCFLComboBox* pCombo = (CCFLComboBox*)m_varCtrlArray[i];
				pCombo->SelectStringExact(-1, modelInput[i].GetStr());
				pCombo->EnableWindow(!m_bDefault);
				break;
			}

			case CModelInputParameterDef::kMVTitle:
			case CModelInputParameterDef::kMVLine:break;

			default: ASSERT(false);
			}

		}


	}

	void CModelInputDlg::GetModelInput(CModelInput& modelInput)
	{
		ASSERT(m_titleCtrlArray.GetSize() == m_varCtrlArray.GetSize());
		ASSERT(m_titleCtrlArray.GetSize() == (int)m_variables.size());
		ASSERT(m_bDefault == false);

		modelInput = m_modelInput;
		modelInput.clear();

		for (int i = 0; i < m_varCtrlArray.GetSize(); i++)
		{
			int type = m_variables[i].GetType();

			if (type != CModelInputParameterDef::kMVTitle &&
				type != CModelInputParameterDef::kMVLine)
			{
				string value;
				switch (type)
				{
				case CModelInputParameterDef::kMVBool:
				{
					CComboBox* pCombo = (CComboBox*)m_varCtrlArray[i];
					value = WBSF::ToString(pCombo->GetCurSel() == 0);
					break;
				}

				case CModelInputParameterDef::kMVInt:
				case CModelInputParameterDef::kMVReal:
				case CModelInputParameterDef::kMVString:
				case CModelInputParameterDef::kMVFile:
				{
					CCFLEdit* pEdit = (CCFLEdit*)m_varCtrlArray[i];
					value = pEdit->GetString();
					break;
				}

				case CModelInputParameterDef::kMVListByPos:
				{
					CCFLComboBox* pCombo = (CCFLComboBox*)m_varCtrlArray[i];
					value = WBSF::ToString(pCombo->GetCurItemData());
					break;
				}

				case CModelInputParameterDef::kMVListByString:
				case CModelInputParameterDef::kMVListByCSV:
				{
					CCFLComboBox* pCombo = (CCFLComboBox*)m_varCtrlArray[i];
					value = pCombo->GetString();
					break;
				}

				default: ASSERT(false);
				}

				modelInput.push_back(CModelInputParam(m_variables[i].GetName(),/* m_variables[i].GetType(),*/ value));
			}
		}
	}


	void CModelInputDlg::OnClose()
	{
		CWnd::OnClose();
		DestroyWindow();
	}

	void CModelInputDlg::OnOK()
	{}

	void CModelInputDlg::OnCancel()
	{}

	void CModelInputDlg::OnDestroy()
	{

		CRect rect;
		GetWindowRect(rect);


		CString modelName;
		GetWindowText(modelName);

		CAppOption option;


		option.SetCurrentProfile(_T("WindowsPosition"));
		CPoint defaultPos = option.GetProfilePoint(_T("ModelInputEditor"), CPoint(30, 30));
		rect -= defaultPos;

		option.SetCurrentProfile(_T("ModelsWindowsPosition"));
		option.WriteProfilePoint(modelName, rect.TopLeft());

		CDialog::OnDestroy();
	}

}