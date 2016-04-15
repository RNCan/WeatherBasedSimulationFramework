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
#include "ImportSimulationDlg.h"

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


		SetNumberCols(2);
		SetNumberRows(0);
		SetDefColWidth(300);
		SetColWidth(-1, 300);

		for (int i = 0; i < strTitle.GetSize(); i++)
		{
			QuickSetText(i - 1, -1, strTitle[i]);
		}



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

	CString CImportVariablesCtrl::GetDimensionRefText(int dimensionRef)const
	{
		ASSERT(dimensionRef >= -1 && dimensionRef < NB_DIMENSION);
		return DIMENSION_LABLE[dimensionRef + 1];
	}

	CString CImportVariablesCtrl::GetDimensionFieldText(int dimensionRef, int dimensionField)const
	{
		if (dimensionField < 0)
			dimensionField = 0;

		CString str;
		if (dimensionRef == LOCATION)
			str = LOC_LABLE[dimensionField];
		else if (dimensionRef == TIME_REF)
			str = TIME_LABLE[dimensionField];
		else if (dimensionRef == VARIABLE)
			str = VARIABLES_LABLE[dimensionField];

		return str;
	}

	int CImportVariablesCtrl::GetDimensionRef(CString str)const
	{
		int pos = DIMENSION_LABLE.Find(str, false) - 1;
		ASSERT(pos >= -1 && pos < NB_DIMENSION);
		return pos;
	}

	int CImportVariablesCtrl::GetDimensionField(int dimensionRef, CString str)const
	{
		int pos = 0;
		if (dimensionRef == LOCATION)
			pos = LOC_LABLE.Find(str);
		else if (dimensionRef == TIME_REF)
			pos = TIME_LABLE.Find(str);
		else if (dimensionRef == VARIABLE)
			pos = VARIABLES_LABLE.Find(str);


		return pos;
	}




	void CImportVariablesCtrl::SetImportHeader(const CString& header)
	{
		CStringArrayEx columnHeader(header);

		SetNumberRows((long)columnHeader.GetSize());
		for (int i = 0; i < columnHeader.GetSize(); i++)
		{

			QuickSetText(-1, i, columnHeader[i]);
			QuickSetCellType(0, i, UGCT_DROPLIST);
			QuickSetCellType(1, i, UGCT_DROPLIST);

			short dimensionField = 0;
			short dimensionRef = GetAutoSelect(columnHeader[i], dimensionField);
			ASSERT(dimensionRef >= -1 && dimensionRef < NB_DIMENSION);

			CUGCell cell;
			GetCell(0, i, &cell);
			cell.SetLabelText(DIMENSION_LABLE.ToString(_T("\n"), false) + _T("\n"));
			cell.SetText(DIMENSION_LABLE[dimensionRef + 1]);
			SetCell(0, i, &cell);

			OnDimensionChange(i, dimensionRef);
			QuickSetText(1, i, GetDimensionFieldText(dimensionRef, dimensionField));

			Invalidate();
		}
	}

	void CImportVariablesCtrl::OnDimensionChange(int row, int dimensionRef)
	{
		CString title;

		if (dimensionRef == LOCATION)
			title = LOC_LABLE.ToString(_T("\n"), false);
		else if (dimensionRef == TIME_REF)
			title = TIME_LABLE.ToString(_T("\n"), false);
		else if (dimensionRef == VARIABLE)
			title = VARIABLES_LABLE.ToString(_T("\n"), false);


		QuickSetLabelText(1, row, title + _T("\n"));

	}

	//retunr dimentionRef and dimensionField
	short CImportVariablesCtrl::GetAutoSelect(CString header, short& dimensionField)
	{
		short dimensionRef = -1;
		dimensionField = 0;

		header.MakeUpper();


		if (header == "KEY_ID")
		{
			dimensionRef = LOCATION;
			dimensionField = CLocation::ID;
		}


		if (header == "LAT" || header == "Y")
		{
			dimensionRef = LOCATION;
			dimensionField = CLocation::LAT;
		}

		if (header == "LONG" || header == "LON" || header == "X")
		{
			dimensionRef = LOCATION;
			dimensionField = CLocation::LON;
		}
		if (header == "ALTITUDE" || header == "ELEV" || header == "Z")
		{
			dimensionRef = LOCATION;
			dimensionField = CLocation::LON;
		}


		//try to find LOC header
		for (int i = 0; i < CLocation::NB_MEMBER&&dimensionRef == -1; i++)
		{
			if (header == CString(CLocation::GetMemberName(i)).MakeUpper())
			{
				dimensionRef = LOCATION;
				dimensionField = i;
			}
		}

		for (int i = 0; i < CTRefFormat::NB_FORMAT&&dimensionRef == -1; i++)
		{
			if (header == CString(CTRefFormat::GetFormatName(i)).MakeUpper())
			{
				dimensionRef = TIME_REF;
				dimensionField = i;
			}
		}

		if (dimensionRef == -1)
		{
			if (header == "PARAMETER")
			{
				dimensionRef = PARAMETER;
			}
			else if (header == "REPLICATION")
			{
				dimensionRef = REPLICATION;
			}
			else if (header == "DATE")
			{
				dimensionRef = TIME_REF;
				dimensionField = CTRefFormat::NB_FORMAT;
			}
			else
			{
				//all other is considered variable
				dimensionRef = VARIABLE;
				//dimensionField=CTRef::ATEMPORAL;
			}
		}

		return dimensionRef;
	}


	int CImportVariablesCtrl::OnCellTypeNotify(long ID, int col, long row, long msg, long param)
	{
		if (msg == UGCT_DROPLISTSTART)
		{
		}
		else if (msg == UGCT_DROPLISTSELECT)
		{
			CString * pString = (CString*)param;

			if (col == 0 && *pString != QuickGetText(col, row))
			{
				int dimensionRef = GetDimensionRef(*pString);
				OnDimensionChange(row, dimensionRef);
				QuickSetText(1, row, GetDimensionFieldText(dimensionRef, 0));
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
			data[i].m_dimensionRef = GetDimensionRef(QuickGetText(0, i));
			data[i].m_dimensionField = GetDimensionField(data[i].m_dimensionRef, QuickGetText(1, i));
		}
	}

	void CImportVariablesCtrl::SetData(const CColumnLinkVector& data)
	{

		int nbRows = GetNumberRows();
		for (int i = 0; i < nbRows&&i < data.size(); i++)
		{
			QuickSetText(0, i, GetDimensionRefText(data[i].m_dimensionRef));
			OnDimensionChange(i, data[i].m_dimensionRef);
			QuickSetText(1, i, GetDimensionFieldText(data[i].m_dimensionRef, data[i].m_dimensionField));
		}

		RedrawAll();
	}

	//*************************************************************************************************

	// CImportSimulationDlg dialog

	IMPLEMENT_DYNAMIC(CImportSimulationDlg, CDialog)

		CImportSimulationDlg::CImportSimulationDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CImportSimulationDlg::IDD, pParentWnd)
	{

	}

	CImportSimulationDlg::~CImportSimulationDlg()
	{
	}

	void CImportSimulationDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_IMPORT_FILENAME, m_fileNameCtrl);
		//DDX_Control(pDX, IDC_IMPORT_TEMPORAL_DATA, m_temporalDataCtrl);
		DDX_Control(pDX, IDC_IMPORT_DEFAULT_DIR, m_defaultDirCtrl);
		GetDlgItem(IDC_IMPORT_INTERNAL_NAME)->SetWindowTextW(ToUTF16(m_importSimulation.GetInternalName()));
	}


	BEGIN_MESSAGE_MAP(CImportSimulationDlg, CDialog)
		//ON_EN_CHANGE(IDC_IMPORT_FILEPATH, &CImportSimulationDlg::OnFilePathChange)
		ON_CBN_SELCHANGE(IDC_IMPORT_FILENAME, &OnFileNameChange)
	END_MESSAGE_MAP()


	// CImportSimulationDlg message handlers

	BOOL CImportSimulationDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();


		m_defaultDirCtrl.SetWindowText(WBSF::GetFM().Input().GetLocalPath().c_str());
		m_columnLink.AttachGrid(this, IDC_IMPORT_COLUMN_LINK);

		CString importFilter = UtilWin::GetCString(IDS_STR_FILTER_CSV);
		FillFileName();

		SetImportFileToInterface();

		return TRUE;
	}

	void CImportSimulationDlg::FillFileName()
	{
		WBSF::StringVector fileList = WBSF::GetFM().Input().GetFilesList();

		int curSel = m_fileNameCtrl.GetCurSel();
		m_fileNameCtrl.ResetContent();
		for (size_t i = 0; i < fileList.size(); i++)
			m_fileNameCtrl.AddString(fileList[i]);

		m_fileNameCtrl.SetCurSel(curSel);
	}

	void CImportSimulationDlg::OnOK()
	{
		GetImportFileFromInterface();


		CProgressStepDlg progressDlg;
		progressDlg.Create(this);

		ERMsg msg = m_importSimulation.UpdateData(WBSF::GetFM(), progressDlg.GetCallback());

		progressDlg.DestroyWindow();

		if (msg)
			CDialog::OnOK();
		else SYShowMessage(msg, this);
	}

	/*void CImportSimulationDlg::OnFilePathChange()
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
	void CImportSimulationDlg::OnFileNameChange()
	{
		//std::string path = WBSF::GetFM().GetInputPath().c_str();
		std::string filePath = WBSF::GetFM().Input().GetFilePath(m_fileNameCtrl.GetString());

		CString header;
		CStdioFile file;


		if (file.Open(Convert(filePath), CFile::modeRead))
		{
			file.ReadString(header);
		}

		m_columnLink.SetImportHeader(header);
	}

	void CImportSimulationDlg::GetImportFileFromInterface()
	{
		m_importSimulation.m_fileName = m_fileNameCtrl.GetString();
		m_columnLink.GetData(m_importSimulation.m_columnLinkArray);
		//	m_importSimulation.m_bTemporalData = m_temporalDataCtrl.GetCheck();
	}

	void CImportSimulationDlg::SetImportFileToInterface()
	{
		//m_fileNameCtrl.SetWindowText(m_importSimulation.m_fileName);	
		m_fileNameCtrl.SelectString(0, m_importSimulation.m_fileName);
		OnFileNameChange();
		m_columnLink.SetData(m_importSimulation.m_columnLinkArray);
		//m_temporalDataCtrl.SetCheck(m_importSimulation.m_bTemporalData);
	}

}