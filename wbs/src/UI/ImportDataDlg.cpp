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

#include "Basic/Dimension.h"
#include "Basic/Location.h"
#include "Basic/UtilTime.h"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/AppOption.h"
#include "ImportDataDlg.h"


#include "WeatherBasedSimulationString.h"

using namespace WBSF::DIMENSION;
using namespace UtilWin;

namespace WBSF
{

	void CImportVariablesCtrl::OnSetup()
	{
		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);



		DIMENSION_LABLE.LoadString(_T(" |") + UtilWin::GetCString(IDS_SIM_RESULT_HEAD));
		LOC_LABLE.LoadString(IDS_STR_LOC_TITLE);
		TIME_LABLE.LoadString(IDS_SIM_TIME_HEAD);
		VARIABLES_LABLE.LoadString(IDS_STR_WEATHER_VARIABLES_TITLE);
		VARIABLES_LABLE.InsertAt(0, UtilWin::GetCString(IDS_STR_OTHER));


		CStringArrayEx strTitle(IDS_SIM_IMPORT_VARIABLE_HEAD);

		CAppOption option(_T("ImportDataColumnWide"));
		SetNumberCols(2);
		SetNumberRows(0);
		SetDefColWidth(300);
		SetColWidth(-1, option.GetProfileInt(_T("-1"), 300));
		SetColWidth(0, option.GetProfileInt(_T("0"), 300));
		SetColWidth(1, option.GetProfileInt(_T("1"), 300));


		//load tooltips	
		/*CStringArrayEx strTooltips;
		strTooltips.LoadString(IDS_NORMAL_DATAHEADDESC);
		ASSERT( strTooltips.GetSize() == NB_FIELDS);

		m_headerTips.Create(this->GetParent());

		int nCol = GetNumberCols();
		for(int i=0; i<nCol; i++)
		{
		CRect rect;
		GetCellRect(i,-1, &rect);
		ClientToScreen(rect);
		GetParent()->ScreenToClient(rect);

		m_headerTips.AddRectTool(this->GetParent(), strTooltips[i], rect, 100);//IDT_RECTANGLE
		}
		*/

		EnableToolTips();

	}

	CString CImportVariablesCtrl::GetDimensionText(size_t dimension)const
	{
		ASSERT(dimension == -1 || dimension < NB_DIMENSION);
		return DIMENSION_LABLE[int(dimension) + 1];
	}

	CString CImportVariablesCtrl::GetFieldText(size_t dimension, size_t field)const
	{
		//if (field == NOT_INIT )
			//field = 0;

		CString str;
		if (dimension == LOCATION)
			str = LOC_LABLE[field == NOT_INIT ? 0 : field];
		else if (dimension == TIME_REF)
			str = TIME_LABLE[field == NOT_INIT ? 0 : field];
		else if (dimension == VARIABLE)
			str = VARIABLES_LABLE[field == NOT_INIT ? 0 : field+1];

		return str;
	}

	size_t CImportVariablesCtrl::GetDimension(CString str)const
	{
		int pos = DIMENSION_LABLE.Find(str, false) - 1;
		ASSERT(pos >= -1 && pos < NB_DIMENSION);
		return size_t(pos);
	}

	size_t CImportVariablesCtrl::GetField(size_t dimension, CString str)const
	{

		size_t pos = NOT_INIT;
		
		if (dimension == LOCATION)
			pos = LOC_LABLE.Find(str);
		else if (dimension == TIME_REF)
			pos = TIME_LABLE.Find(str);
		else if (dimension == VARIABLE)
			pos = size_t(VARIABLES_LABLE.Find(str))-1;


		return pos;
	}




	void CImportVariablesCtrl::SetImportHeader(const std::string& header)
	{
		//CStringArrayEx columnHeader(header);
		StringVector columnHeader(header, ";|,");

		SetNumberRows((long)columnHeader.size());
		for (int i = 0; i < columnHeader.size(); i++)
		{

			QuickSetText(-1, i, CString(columnHeader[i].c_str()));
			QuickSetCellType(0, i, UGCT_DROPLIST);
			QuickSetCellType(1, i, UGCT_DROPLIST);

			size_t dimension = NOT_INIT;
			size_t field = NOT_INIT;
			
			
			GetAutoSelect(columnHeader[i], dimension, field);
			ASSERT(dimension == NOT_INIT || dimension < NB_DIMENSION);

			CUGCell cell;
			GetCell(0, i, &cell);
			cell.SetLabelText(DIMENSION_LABLE.ToString(_T("\n"), false) + _T("\n"));
			int test = int(dimension) + 1;
			cell.SetText(DIMENSION_LABLE[int(dimension) + 1]);
			SetCell(0, i, &cell);

			OnDimensionChange(i, dimension);
			QuickSetText(1, i, GetFieldText(dimension, field));

			Invalidate();
		}
	}

	void CImportVariablesCtrl::OnDimensionChange(int row, size_t dimension)
	{
		CString title;

		if (dimension == LOCATION)
			title = LOC_LABLE.ToString(_T("\n"), false);
		else if (dimension == TIME_REF)
			title = TIME_LABLE.ToString(_T("\n"), false);
		else if (dimension == VARIABLE)
			title = VARIABLES_LABLE.ToString(_T("\n"), false);


		QuickSetLabelText(1, row, title + _T("\n"));

	}

	
	size_t GetDateFromString(const std::string& header)
	{
		size_t field = NOT_INIT;

		for (size_t i = 0; i < CTRefFormat::NB_FORMAT&&field == NOT_INIT; i++)
		{
			if (IsEqual(header, CTRefFormat::GetFormatName(i)))
				field = i;
		}

		return field;
	}
	//retunr dimentionRef and dimensionField
	void CImportVariablesCtrl::GetAutoSelect(const std::string& header, size_t& dimension, size_t& field)
	{
		//std::string header = (LPCSTR)CStringA(headerIn);
		dimension = NOT_INIT;
		field = NOT_INIT;

		if (IsEqual(header, "Parameter") || IsEqual(header, "Paramètre"))
		{
			dimension = PARAMETER;
		}
		else if (IsEqual(header, "Replication") || IsEqual(header, "Répétition"))
		{
			dimension = REPLICATION;
		}
		else if (IsEqual(header, "Date"))
		{
			dimension = TIME_REF;
			field = CTRefFormat::NB_FORMAT;
		}
		else if (IsEqual(header, (LPCSTR)CStringA(CString("Élévation"))))
		{
			field = CLocation::ELEV;
			dimension = LOCATION;
		}
		else if ((field = GetDateFromString(header)) != NOT_INIT)
		{
			dimension = TIME_REF;
		}
		else if ((field = CLocation::GetMemberFromName(header)) != CLocation::SSI)
		{
			dimension = LOCATION;
		}
		else
		{
			//all other is considered variable
			dimension = VARIABLE;
			field = NOT_INIT;
			//try to indentify weather variables
		}
		
	}


	int CImportVariablesCtrl::OnCellTypeNotify(long ID, int col, long row, long msg, LONG_PTR param)
	{
		if (msg == UGCT_DROPLISTSTART)
		{
		}
		else if (msg == UGCT_DROPLISTSELECT)
		{
			CString * pString = (CString*)param;

			if (col == 0 && *pString != QuickGetText(col, row))
			{
				size_t dimension = GetDimension(*pString);
				OnDimensionChange(row, dimension);
				QuickSetText(1, row, GetFieldText(dimension, 0));
				RedrawCell(1, row);
			}
		}
		else if (msg == UGCT_DROPLISTPOSTSELECT)
		{
		}

		return TRUE;
	}

	void CImportVariablesCtrl::GetData(CColumnLinkVector& data)
	{
		int nbRows = GetNumberRows();
		data.resize(nbRows);

		for (int i = 0; i < nbRows; i++)
		{
			data[i].m_name = ToUTF8(QuickGetText(-1, i));
			data[i].m_dimension = GetDimension(QuickGetText(0, i));
			data[i].m_field = GetField(data[i].m_dimension, QuickGetText(1, i));
		}
	}

	void CImportVariablesCtrl::SetData(const CColumnLinkVector& data)
	{

		int nbRows = GetNumberRows();
		for (int i = 0; i < nbRows&&i < data.size(); i++)
		{
			QuickSetText(0, i, GetDimensionText(data[i].m_dimension));
			OnDimensionChange(i, data[i].m_dimension);
			QuickSetText(1, i, GetFieldText(data[i].m_dimension, data[i].m_field));
		}

		RedrawAll();
	}


	void CImportVariablesCtrl::OnColSized(int col, int *width)
	{
		CAppOption option(_T("ImportDataColumnWide"));
		CString name = ToCString(col);
		option.WriteProfileInt(name, *width);
	}

	int  CImportVariablesCtrl::OnSideHdgSized(int *width)
	{
		CAppOption option(_T("ImportDataColumnWide"));
		option.WriteProfileInt(_T("-1"), *width);

		return TRUE;
	}

	//*************************************************************************************************

	// CImportDataDlg dialog

	IMPLEMENT_DYNAMIC(CImportDataDlg, CDialog)

		CImportDataDlg::CImportDataDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CImportDataDlg::IDD, pParentWnd)
	{

	}

	CImportDataDlg::~CImportDataDlg()
	{
	}

	void CImportDataDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		
		DDX_Control(pDX, IDC_NAME, m_nameCtrl);
		DDX_Control(pDX, IDC_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDC_IMPORT_FILENAME, m_fileNameCtrl);
		DDX_Control(pDX, IDC_IMPORT_DEFAULT_DIR, m_defaultDirCtrl);
		DDX_Control(pDX, IDC_IMPORT_INTERNAL_NAME, m_internalNameCtrl);
		
		DDX_Text(pDX, IDC_NAME, m_importData.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_importData.m_description);
	}


	BEGIN_MESSAGE_MAP(CImportDataDlg, CDialog)
		ON_CBN_SELCHANGE(IDC_IMPORT_FILENAME, &OnFileNameChange)
		ON_WM_SIZE()
		ON_WM_DESTROY()
	END_MESSAGE_MAP()


	// CImportDataDlg message handlers

	BOOL CImportDataDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		m_defaultDirCtrl.SetWindowText(WBSF::GetFM().Input().GetLocalPath().c_str());
		m_internalNameCtrl.SetWindowText(m_importData.GetInternalName());
		
		m_columnLink.AttachGrid(this, IDC_IMPORT_COLUMN_LINK);

		CString importFilter = UtilWin::GetCString(IDS_STR_FILTER_CSV);
		FillFileName();

		CAppOption option;
		
		CRect rectClient;
		GetWindowRect(rectClient);

		rectClient = option.GetProfileRect(_T("ImportDataDlgRect"), rectClient);
		UtilWin::EnsureRectangleOnDisplay(rectClient);
		SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);


		SetImportFileToInterface();

		return TRUE;
	}

	void CImportDataDlg::FillFileName()
	{
		WBSF::StringVector fileList = WBSF::GetFM().Input().GetFilesList();

		int curSel = m_fileNameCtrl.GetCurSel();
		m_fileNameCtrl.ResetContent();
		for (size_t i = 0; i < fileList.size(); i++)
			m_fileNameCtrl.AddString(fileList[i]);

		m_fileNameCtrl.SetCurSel(curSel);
	}

	void CImportDataDlg::OnOK()
	{
		GetImportFileFromInterface();


		CProgressStepDlg progressDlg;
		progressDlg.Create(this);

		ERMsg msg = m_importData.UpdateData(WBSF::GetFM(), progressDlg.GetCallback());

		progressDlg.DestroyWindow();

		if (msg)
			CDialog::OnOK();
		else SYShowMessage(msg, this);
	}

	/*void CImportDataDlg::OnFilePathChange()
	{
	CString filePath = m_importFilePathCtrl.GetWindowText();

	CString header;
	CStdioFile file;


	if( file.Open(filePath, CFile::modeRead) )
	{
	file.ReadString(header);
	}

	m_columnLink.SetImportHeader(header);
	}
	*/
	void CImportDataDlg::OnFileNameChange()
	{
		//std::string path = WBSF::GetFM().GetInputPath().c_str();
		std::string filePath = WBSF::GetFM().Input().GetFilePath(m_fileNameCtrl.GetString());

		//CString header;
		//CStdioFile file;
		ifStream file;
		auto myloc = std::locale();
		file.imbue(myloc);


		std::string header;

		if (file.open(filePath))
		{
			std::getline(file, header);
			//file.ReadString(header);
		}

		m_columnLink.SetImportHeader(header);
	}

	void CImportDataDlg::GetImportFileFromInterface()
	{
		m_importData.m_fileName = m_fileNameCtrl.GetString();
		m_columnLink.GetData(m_importData.m_columnLinkArray);
		//	m_importData.m_bTemporalData = m_temporalDataCtrl.GetCheck();
	}

	void CImportDataDlg::SetImportFileToInterface()
	{
		//m_fileNameCtrl.SetWindowText(m_importData.m_fileName);	
		m_fileNameCtrl.SelectString(0, m_importData.m_fileName);
		OnFileNameChange();
		m_columnLink.SetData(m_importData.m_columnLinkArray);
		//m_temporalDataCtrl.SetCheck(m_importData.m_bTemporalData);
	}

	void CImportDataDlg::OnSize(UINT nType, int cx, int cy)
	{
		CDialog::OnSize(nType, cx, cy);

		AdjustLayout();
	}

	void CImportDataDlg::AdjustLayout()
	{
		static const int MARGE = 8;
		if (GetSafeHwnd() == NULL || m_columnLink.GetSafeHwnd() == NULL)
		{
			return;
		}

		CRect rectClient;
		GetClientRect(rectClient);

		CRect rectOK;
		GetDlgItem(IDOK)->GetWindowRect(rectOK); ScreenToClient(rectOK);

		rectOK.MoveToX(rectClient.right - 2 * MARGE - 2 * rectOK.Width());
		rectOK.MoveToY(rectClient.bottom - MARGE - rectOK.Height());

		CRect rectCancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel); ScreenToClient(rectCancel);
		rectCancel.left = rectClient.right - MARGE - rectCancel.Width();
		rectCancel.top = rectClient.bottom - MARGE - rectCancel.Height();

		CRect rectName;
		m_nameCtrl.GetWindowRect(rectName); ScreenToClient(rectName);
		rectName.right = rectClient.right - MARGE;

		CRect rectDescription;
		m_descriptionCtrl.GetWindowRect(rectDescription); ScreenToClient(rectDescription);
		rectDescription.right = rectClient.right - MARGE;
		
		CRect rectFileName;
		m_fileNameCtrl.GetWindowRect(rectFileName); ScreenToClient(rectFileName);
		rectFileName.right = rectClient.right - MARGE;


		CRect rectStatic2;
		GetDlgItem(IDC_CMN_STATIC2)->GetWindowRect(rectStatic2); ScreenToClient(rectStatic2);
		rectStatic2.MoveToY(rectClient.bottom - rectStatic2.Height() - MARGE);

		CRect rectPath;
		m_defaultDirCtrl.GetWindowRect(rectPath); ScreenToClient(rectPath);
		rectPath.right = rectClient.right - MARGE;
		rectPath.MoveToY(rectClient.bottom - MARGE - rectOK.Height() - MARGE - rectPath.Height() );

		CRect rectStatic1;
		GetDlgItem(IDC_CMN_STATIC1)->GetWindowRect(rectStatic1); ScreenToClient(rectStatic1);
		rectStatic1.MoveToY(rectClient.bottom - MARGE - rectOK.Height() - MARGE - rectStatic1.Height());

		CRect rect;
		m_columnLink.GetWindowRect(rect); ScreenToClient(rect);
		rect.right = rectClient.right - MARGE;
		rect.bottom = rectClient.bottom - MARGE - rectOK.Height() - MARGE - rectPath.Height() - MARGE;


		CRect rectInternalName;
		m_internalNameCtrl.GetWindowRect(rectInternalName); ScreenToClient(rectInternalName);
		rectInternalName.top = rectClient.bottom - rectDescription.Height() - MARGE;
		rectInternalName.bottom = rectClient.bottom - MARGE;


		m_nameCtrl.SetWindowPos(NULL, 0, 0, rectName.Width(), rectName.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_descriptionCtrl.SetWindowPos(NULL, 0, 0, rectDescription.Width(), rectDescription.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_fileNameCtrl.SetWindowPos(NULL, 0, 0, rectFileName.Width(), rectFileName.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_columnLink.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		m_defaultDirCtrl.SetWindowPos(NULL, rectPath.left, rectPath.top, rectPath.Width(), rectPath.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
		GetDlgItem(IDC_CMN_STATIC1)->SetWindowPos(NULL, rectStatic1.left, rectStatic1.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDC_CMN_STATIC2)->SetWindowPos(NULL, rectStatic2.left, rectStatic2.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		m_internalNameCtrl.SetWindowPos(NULL, rectInternalName.left, rectInternalName.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDOK)->SetWindowPos(NULL, rectOK.left, rectOK.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		GetDlgItem(IDCANCEL)->SetWindowPos(NULL, rectCancel.left, rectCancel.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}


	void CImportDataDlg::OnDestroy()
	{
		CRect rectClient;
		GetWindowRect(rectClient);

		CAppOption option;
		option.WriteProfileRect(_T("ImportDataDlgRect"), rectClient);

		CDialog::OnDestroy();
	}


}