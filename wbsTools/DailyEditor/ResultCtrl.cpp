#include "StdAfx.h"
#include "ResultCtrl.h"
#include "UtilWin.h"
#include "Statistic.h"
#include "Registry.h"
#include "MainFrm.h"
#include "HourlyEditorDoc.h"
#include "NewNameDlg.h"

#include "ExcelTopHdg.h"
#include "ExcelSideHdg.h"

//#include <wingdi.h>

//#include "SimulationRes.h"

using namespace std;
using namespace stdString;
using namespace CFL;

BEGIN_MESSAGE_MAP(CResultCtrl, CResultCtrlBase)
END_MESSAGE_MAP()

static CHourlyEditorDoc* GetDocument() 
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL && pFrame->IsWindowVisible())
			return (CHourlyEditorDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;
}

static CWeatherDatabasePtr GetProjectPtr()
{
	CWeatherDatabasePtr pProject;
	CHourlyEditorDoc* pDocument = GetDocument();

	if (pDocument)
		pProject = pDocument->m_pProject;
	

	return  pProject;
}

//
//CTRef CCtrlInfo::GetTRef(long row, const CStudyData& data)
//{
//	CTRef TRef;
//
//	//int tab = GetCurrentTab(); 
//	if (row>=0 && m_TRefPos.size() == 3 && m_TRefPos[0] != UNKNOWN_POS)
//	{
//
//		int year = ToInt(data[row][m_TRefPos[0]]);
//		int month = ToInt(data[row][m_TRefPos[1]])-1;
//		int day = ToInt(data[row][m_TRefPos[2]])-1;
//		
//		if (month>=0 && month<12 &&
//			day >= 0 && day <= CFL::GetNbDayPerMonth(year,month))
//			TRef = CTRef(year, month, day);
//	}
//
//	return TRef;
//}
//
//void CCtrlInfo::InitObservationsDates(const CStudyData& data)
//{
//	m_observations.clear();
//
//	for (size_t i = 0; i < data.size(); i++)
//	{
//		CTRef TRef = GetTRef(i, data);
//		if (TRef.IsInit())
//			m_observations.insert(TRef);
//	}
//}
//
//
//void CCtrlInfo::UpdateObservationsDates(int col, long row, const CStudyData& data)
//{
//	ASSERT(row >= 0);
//	ASSERT(col >= 0);
//
//	if (m_TRefPos.size() == 3 && m_TRefPos[0] != UNKNOWN_POS)
//	{
//		if (col == m_TRefPos[0] || col == m_TRefPos[1] || col == m_TRefPos[2])
//		{
//			CTRef TRef = GetTRef(row, data);
//			if (TRef.IsInit())
//				m_observations.insert(TRef);
//		}
//	}
//}
//
//
//double CCtrlInfo::GetObservationFactor(long row, const CStudyData& data)
//{
//	double OF = 100;
//
//	CTRef TRef = GetTRef(row, data);
//	if (TRef.IsInit())
//	{
//		std::set<CTRef>::const_iterator itOF = m_observations.find(TRef);
//		if (itOF != m_observations.end())
//		{
//			ASSERT(!m_observationsFactors.empty());
//
//			size_t index = std::distance(m_observations.begin(), itOF) % m_observationsFactors.size();
//			OF = m_observationsFactors[index];
//		}
//	}
//
//	return OF;
//}
/////////////////////////////////////////////////////////////////////////////
//	OnTabSelected
//		Called when the user selects one of the tabs on the bottom of the screen
//	Params:
//		ID	- id of selected tab
//	Return:
//		<none>


COLORREF GetBackColor(COLORREF color, double shift)
{
	//int shift = (i == 0) ? 5 : -5;

	int R = max(0, min(255, int(GetRValue(color) * shift / 100)));
	int G = max(0, min(255, int(GetGValue(color) * shift / 100)));
	int B = max(0, min(255, int(GetBValue(color) * shift / 100)));

	return RGB(R, G, B);
}


//(****************************************************************************************************************

CResultCtrl::CResultCtrl(void)
{
//	m_statType = MEAN;
//	m_bHaveChange = false;
//	m_fontsList = NULL;
//	m_currentTab = -1;
}

CResultCtrl::~CResultCtrl(void)
{
	//UGXPThemes::CleanUp();
}



void CResultCtrl::OnSetup()
{
	CResultCtrlBase::OnSetup();

	
	m_font.CreateStockObject(DEFAULT_GUI_FONT);
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
}


void CResultCtrl::OnSheetSetup(int ID)
{

	// add and set heading's default celltype
	CUGCell cell;
	GetHeadingDefault(&cell);
	cell.SetAlignment(UG_ALIGNCENTER);
	cell.SetBorder(UG_BDR_RTHIN | UG_BDR_BTHIN);
	cell.SetBackColor(RGB(239, 237, 222));
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
	SetHighlightRow(UG_MULTISELECT_NODESELECT, FALSE);
	SetMultiSelectMode(TRUE);//UG_MULTISELECT_HDGS
	
	
	SetDoubleBufferMode(TRUE);
	SetVScrollMode(UG_SCROLLTRACKING);
	SetHScrollMode(UG_SCROLLTRACKING);
	//SetUserSizingMode(2);
	SetCurrentCellMode(3);
	SetHighlightRow(FALSE);
	
}


void CResultCtrl::InitSheet(const CCtrlInfo& info)
{
	//int ID = std::distance(positions.begin(), it);
	//SetSheetNumber(ID, FALSE);

	CWeatherDatabasePtr pProject = GetProjectPtr();
	CYearsVector 
	//CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
	//const CStudyDefinition& study = studies.at(info.m_studyName);
	//const CFieldsDefinitions& fields = study.m_fields;



	//****************************************************************************************
	//Header, columns width and rows color observations factors
	m_enableUpdate = FALSE;

	SetTH_NumberRows(1);
	SetSH_NumberCols(1);
	SetColWidth(-1, study.m_largeurVial);

	//****************************************************************************************
	//data
	size_t nbVials = study.GetNbVials();
	size_t nbChamps = fields.size();
	
	SetNumberCols((long)nbChamps, FALSE);
	SetNumberRows((int)nbVials, FALSE);

	for (CFieldsDefinitions::const_iterator it2 = fields.begin(); it2 != fields.end(); it2++)
	{
		const CFieldDefinition& field = it2->second;
		SetColWidth(field.m_position, field.m_largeur);
	}

	m_enableUpdate = TRUE;
	Invalidate();
}


string CResultCtrl::GetActiveTabName()
{
	int tab = GetCurrentTab();

	ASSERT(m_info.size() == m_CUGTab->GetTabCount());
	string studyName = m_info[tab].m_studyName;

	return studyName;
}


void CResultCtrl::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWeatherDatabasePtr pProject = GetProjectPtr();
	if (pProject)
	{
		if (lHint == CHourlyEditorDoc::INIT)
		{
			int nbTab = m_CUGTab->GetTabCount();
			for (int i = 0; i < nbTab; i++)
				m_CUGTab->DeleteTab(i);

			m_enableUpdate = FALSE;
			SetNumberCols(0);
			SetNumberRows(0);
			SetTabWidth(0);
			m_enableUpdate = TRUE;
			
			m_info.clear();

			const CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
			if (!studies.empty())
			{
				//m_positions.clear();

				std::set<pair<int, std::string>> positions;
				for (CStudiesDefinitions::const_iterator it = studies.begin(); it != studies.end(); it++)
					positions.insert(pair<int, std::string>(it->second.m_position, it->first));

				for (std::set<pair<int, std::string>>::const_iterator it = positions.begin(); it != positions.end(); it++)
				{
					int ID = std::distance(positions.begin(), it);

					CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
					const CStudyDefinition& study = studies.at(it->second);
					const CFieldsDefinitions& fields = study.m_fields;

					CCtrlInfo info;
					info.m_studyName = it->second;
					info.m_observationsFactors = ToVector<double>(study.m_observationsFactors, ";");
					for (CFieldsDefinitions::const_iterator it2 = fields.begin(); it2 != fields.end(); it2++)
					{
						const CFieldDefinition& field = it2->second;
						info.m_fieldsPositions[field.m_position] = it2->first;
					}

					CFieldsDefinitions::const_iterator itYear = fields.find("Annee");
					
					if (itYear == fields.end())
						itYear = fields.find("Année");

					if (itYear == fields.end())
						itYear = fields.find("Year");

					CFieldsDefinitions::const_iterator itMonth = fields.find("Mois");
					if (itMonth == fields.end())
						itMonth = fields.find("Month");

					CFieldsDefinitions::const_iterator  itDay = fields.find("Jour");
					if (itDay == fields.end())
						itDay = fields.find("Day");


					if (itYear != fields.end() &&
						itMonth != fields.end() &&
						itDay != fields.end())
					{
						info.m_TRefPos.push_back(itYear->second.m_position);
						info.m_TRefPos.push_back(itMonth->second.m_position);
						info.m_TRefPos.push_back(itDay->second.m_position);
					}
					else
					{
						info.m_TRefPos.insert(info.m_TRefPos.begin(), 3, UNKNOWN_POS);
					}
					
					CStudiesData& studiesData = pProject->m_studiesData;
					CStudyData& data = studiesData.at(it->second);
					info.InitObservationsDates(data);

					m_info.push_back(info);
					AddTab(CString(study.m_title.c_str()), ID);
				}

				SetNumberSheets((int)m_info.size());


				CDPTProjectProperties& properties = pProject->m_properties;
				SetTabWidth(properties.m_tabWidth);


				//init current studies
				int ID = 0;
				CStudiesDefinitions::const_iterator it = studies.find(properties.m_studyName);
				if (it != studies.end())
					ID = it->second.m_position;
				

				SetCurrentTab(ID);
				SetSheetNumber(ID);
				if (m_info[ID].m_bNeedInit)
				{
					InitSheet(m_info[ID]);
					m_info[ID].m_bNeedInit = false;
				}
				
				
				properties.m_studyName = m_info[ID].m_studyName;
				properties.m_curCol = GetCurrentCol();
				properties.m_curRow = GetCurrentRow();

				//InitSheet(const CCtrlInfo& info)

			}//not empty

			Invalidate();
		}
		else if (lHint == CHourlyEditorDoc::PROPERTIES_CHANGE)
		{
			size_t tab = (size_t ) GetCurrentTab();
			if (tab < m_info.size())
			{
				string studyName = m_info[tab].m_studyName;
				CDPTProjectProperties& properties = pProject->m_properties;

				if (studyName != properties.m_studyName)
				{
					const CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
					const CStudyDefinition& study = studies.at(studyName);
					SetCurrentTab(study.m_position);
				}

				if (properties.m_curCol != GetLeftCol() || properties.m_curRow != GetTopRow())
				{
					GotoCell(properties.m_curCol, properties.m_curRow);
				}
			}
		}
		else if (lHint == CHourlyEditorDoc::DATA_CHANGE)
		{
			size_t tab = (size_t)GetCurrentTab();
			ASSERT(tab < m_info.size());
			
			string studyName = m_info[tab].m_studyName;
			CDPTProjectProperties& properties = pProject->m_properties;
			CStudiesData& studiesData = pProject->m_studiesData;
			CStudyData& data = studiesData[studyName];

			m_info[tab].UpdateObservationsDates(properties.m_curCol, properties.m_curRow, data);

			this->RedrawRow(properties.m_curRow);
		}
	}

}




void CResultCtrl::OnColSized(int col, int *width)
{ 
	CWeatherDatabasePtr pProject = GetProjectPtr();

	if (pProject.get())
	{
		CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
		size_t tab = (size_t)GetCurrentTab();
		
		ASSERT(m_info.size() == m_CUGTab->GetTabCount());
		ASSERT(m_info.size() == studies.size());
		string studyName = m_info[tab].m_studyName;

		ASSERT(tab < studies.size());
		
		ASSERT(studies.find(studyName) != studies.end());
		CStudyDefinition& study = studies.at(studyName);
		CFieldsDefinitions& fields = study.m_fields;

		if (col == -1)
		{
			study.m_largeurVial = *width;
		}
		else
		{
			std::map<int, std::string>::const_iterator itField = m_info[tab].m_fieldsPositions.find(col);
			ASSERT(itField != m_info[tab].m_fieldsPositions.end());
			if (itField != m_info[tab].m_fieldsPositions.end())
				fields.at(itField->second).m_largeur = *width;
		}
	}

	return CResultCtrlBase::OnColSized(col, width); 
}

void CResultCtrl::OnGetCell(int col,long row, CUGCell *cell)
{ 
	CWeatherDatabasePtr pProject = GetProjectPtr();


	if (pProject.get() )
	{
		const CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
		
		if (studies.empty() || m_info.size() != studies.size())
			return;


		size_t tab = (size_t)GetCurrentTab();
		ASSERT(m_info.size() == m_CUGTab->GetTabCount());
		ASSERT(tab < m_info.size());

		string studyName = m_info[tab].m_studyName;
		
		ASSERT(studies.find(studyName) != studies.end());
		const CStudyDefinition& study = studies.at(studyName);
		const CFieldsDefinitions& fields = study.m_fields;
		CDPTProjectProperties& properties = pProject->m_properties;

		if (row >= -1 && row < (long)study.GetNbVials() &&
			col >= -1 && col < (int)fields.size())
		{
			string text;
			COLORREF backColor = cell->GetBackColor(); 
			COLORREF textColor = cell->GetTextColor(); 

			//int test = LocateCol(col);
			if (col == -1)
			{
				if (row == -1)
					text = "No vial";
				else 
					text = ToString(ToInt(study.m_firstVial) + row);
			}
			else
			{
				std::map<int, std::string>::const_iterator itField = m_info[tab].m_fieldsPositions.find(col);
				ASSERT(itField != m_info[tab].m_fieldsPositions.end());
				const CFieldDefinition& field = fields.at(itField->second);

				if (row == -1)
				{
					text = field.m_title;
				}
				else
				{
					textColor = properties.m_bEditable ? RGB(0, 0, 0) : RGB(175, 175, 175);

					const CStudiesData& dataMap = pProject->m_studiesData;
					CStudiesData::const_iterator it = dataMap.find(studyName);
					ASSERT(it != dataMap.end());

					if (it != dataMap.end())
					{
						const CStudyData& data = it->second;
						ASSERT(row < (long)data.size_y() && col < (int)data.size_x());

						double OF = m_info[tab].GetObservationFactor(row, data);
									

						backColor = GetBackColor(field.m_backColor, OF);
						textColor = field.IsValid(data[row][col]) ? textColor : RGB(240, 15, 15);
						text = data[row][col];
									
						//}
					}
				}
			}

			cell->SetBackColor(backColor);
			cell->SetTextColor(textColor);
			cell->SetText(CString(text.c_str()));
		}
	}

	CResultCtrlBase::OnGetCell(col, row, cell);
} 


void CResultCtrl::OnSetCell(int col, long row, CUGCell *cell)
{
	CWeatherDatabasePtr pProject = GetProjectPtr();
	ASSERT(pProject.get());

	if (pProject.get())
	{
		ASSERT(col >= -1 && row >= -1);
		if (col >= 0)
		{
			int tab = GetCurrentTab();
			ASSERT(m_info.size() == m_CUGTab->GetTabCount());
			string studyName = m_info[tab].m_studyName;
			CWeatherDatabasePtr pProject = GetProjectPtr();

			if (row == -1)
			{
				CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
				CStudyDefinition& study = studies.at(studyName);
				CFieldsDefinitions& fields = study.m_fields;
				
				std::map<int, std::string>::const_iterator itField = m_info[tab].m_fieldsPositions.find(col);
				ASSERT(itField != m_info[tab].m_fieldsPositions.end());
				CFieldDefinition& field = fields.at(itField->second);
				
				std::string oldStr = field.m_title;
				std::string newStr = CStringA(cell->GetText());
				if (oldStr != newStr)
				{
					field.m_title = newStr;
					GetDocument()->UpdateAllViews(((CView*)GetParent()), CHourlyEditorDoc::PROPERTIES_CHANGE, NULL);
				}
			}
			else 
			{
				CStudiesData& studiesData = pProject->m_studiesData;
				CStudyData& data = studiesData[studyName];

				CUGCell oldCell;
				GetCellIndirect(col, row, &oldCell);

				std::string oldStr = data[row][col];
				std::string newStr = CStringA(cell->GetText());
				if (oldStr != newStr)
				{
					data.Modified(true);
					data[row][col] = newStr;
					//GetDocument()->UpdateAllViews(((CView*)GetParent()), CHourlyEditorDoc::DATA_CHANGE, NULL);
					GetDocument()->UpdateAllViews(NULL, CHourlyEditorDoc::DATA_CHANGE, NULL);
				}
			}
		}
	}


	CResultCtrlBase::OnSetCell(col, row, cell);
}
//void CResultCtrl::SetProject(CWeatherDatabasePtr pProjet)
//{
//	CWeatherDatabasePtr pProject = GetProjectPtr();
//	
//	int nbTab = m_CUGTab->GetTabCount();
//	for (int i = 0; i < nbTab; i++)
//		m_CUGTab->DeleteTab(i);
//	
//	m_info.clear();
//
//	if (pProject)
//	{
//		const CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
//		if (!studies.empty())
//		{
//
//			for (CStudiesDefinitions::const_iterator it = studies.begin(); it != studies.end(); it++)
//				m_positions[it->second.m_position] = it->first;
//				
//			for (std::map<int, std::string>::const_iterator it = m_positions.begin(); it != m_positions.end(); it++)
//			{
//				CString name(it->second.c_str());
//				AddTab(name, it->first);
//				//SetCurrentTab(it->first);
//				//OnSheetSetup(it->first);
//			}
//		}
//	}
//}


//void CResultCtrl::OnLClicked(int col, long row, int updn, RECT *rect, POINT *point, BOOL processed)
//{
//	//int mode = (row == -1) ? UG_MULTISELECT_ROW : UG_MULTISELECT_CELL;
//	//SetHighlightRow(mode, FALSE);
//	//SetMultiSelectMode(TRUE);//UG_MULTISELECT_HDGS
//}

void CResultCtrl::SelectRows()
{
	int startCol = 0;
	long startRow = 0;
	int endCol = 0;
	long endRow = 0;
	if (m_GI->m_multiSelect->GetTotalRange(&startCol, &startRow, &endCol, &endRow) == UG_SUCCESS)
	{
		bool bRowsSel = true;
		CArray<long> sels;
		for (long r = startRow; r <= endRow; r++)
		{
			if (m_GI->m_multiSelect->IsCellInRowSelected(r))
			{
				sels.Add(r);
				for (int c = 0; c < GetNumberCols() && bRowsSel; c++)
				{
					bRowsSel = m_GI->m_multiSelect->IsSelected(c, r);
				}
			}
		}

		//modify the selection
		if (!bRowsSel)
		{
			ClearSelections();
			for (INT_PTR i = 0; i < sels.GetSize(); i++)
			{
				SelectRange(0, sels[i], GetNumberCols() - 1, sels[i]);
			}
		}
	}
}

//void CResultCtrl::CopyRows()
//{
//	SelectRows();
//	CopySelected();
//	
//}

int GetNbColumns(CString string)
{
	int nbColsSelect = -1;

	if (!string.IsEmpty())
	{
		int nbCols = 0;
		vector<int> nbColsVector;

		int		pos = 0;
		LPCTSTR buf = string.GetBuffer(1);
		int		len = string.GetLength();

		int endpos = pos;
		while (endpos < len)
		{
			CHAR endchar = buf[endpos];
			if (endchar == _T('\t'))
				nbCols++;

			if (endchar == _T('\n') || endchar == _T('\r'))
			{
				nbColsVector.push_back(nbCols + 1);
				nbCols = 0;
			}

			if (endpos < len - 1 && buf[endpos] == _T('\r') && buf[endpos + 1] == _T('\n'))
				endpos++;

			endpos++;
		}

		nbColsVector.push_back(nbCols + 1);

		string.ReleaseBuffer();

		nbColsSelect = nbColsVector.front();

		for (size_t i = 1; i < nbColsVector.size() && nbColsSelect != -1; i++)
			if (nbColsVector[i] != nbColsSelect)
				nbColsSelect = -1;
	}
		
	return nbColsSelect;
}

bool CResultCtrl::IsRowsSelected(const CString& string)
{
	bool bRep = false;

	int nbColsSelect = GetNbColumns(string);

	int startCol = 0;
	long startRow = 0;
	int endCol = 0;
	long endRow = 0;
	if (m_GI->m_multiSelect->GetTotalRange(&startCol, &startRow, &endCol, &endRow) == UG_SUCCESS)
	{
		if (endRow > startRow)//more than one line
		{
			bRep = true;

			for (long r = startRow; r <= endRow&&bRep; r++)
			{
				if (m_GI->m_multiSelect->IsCellInRowSelected(r))
				{
					int nbCols = 0;
					for (int c = 0; c < GetNumberCols() && bRep; c++)
						nbCols += m_GI->m_multiSelect->IsSelected(c, r) ? 1 : 0;

					bRep = nbCols == nbColsSelect || nbCols == GetNumberCols();
				}
			}
		}
	}

	return bRep;
}

int CResultCtrl::Paste()
{
	CString	string;
	if (CopyFromClipBoard(&string) != UG_SUCCESS)
		return UG_ERROR;
	
	if (IsRowsSelected(string))
		PasteRows(string);
	else
		Paste(string);
	

	return UG_SUCCESS;
}

int CResultCtrl::PasteRows(CString string)
{
	ASSERT(IsRowsSelected(string));

	int startCol = 0;
	long startRow = 0;
	int endCol = 0;
	long endRow = 0;
	if (m_GI->m_multiSelect->GetTotalRange(&startCol, &startRow, &endCol, &endRow) == UG_SUCCESS)
	{
		int		pos = 0;
		LPCTSTR buf = string.GetBuffer(1);
		int		len = string.GetLength();

		CArray<long> sels;
		for (long r = startRow; r <= endRow; r++)
		{
			if (m_GI->m_multiSelect->IsCellInRowSelected(r))
			{
				sels.Add(r);
			}
		}


		for (INT_PTR i = 0; i < sels.GetSize(); i++)
		{
			for (int c = 0; c < GetNumberCols(); c++ )
			{
				if (m_GI->m_multiSelect->IsSelected(c, sels[i]))
				{
					if (pos >= len)
						pos = 0;//wrap around

					int endpos = pos;
					while (endpos < len)
					{
						CHAR endchar = buf[endpos];
						if (endchar == _T('\n') || endchar == _T('\r') || endchar == _T('\t'))
							break;
						endpos++;
					}

					CUGCell cell;
					//copy the item
					GetCell(c, sels[i], &cell);

					if (cell.GetReadOnly() != TRUE)
					{
						CString sub;
						//check to see if the field is blank
						if (buf[pos] != _T('\t') && buf[pos] != _T('\r') && buf[pos] != _T('\n'))
							sub = string.Mid(pos, endpos - pos);

						cell.SetText(sub);
						SetCell(c, sels[i], &cell);
					}

					if (endpos < len - 1 && buf[endpos] == _T('\r') && buf[endpos + 1] == _T('\n'))
						endpos++;

					pos = endpos + 1;

					if (pos >= len || buf[endpos] == _T('\r') || buf[endpos] == _T('\n'))
					{
						break;//go to the next line
					}
				}
			}
		}
		string.ReleaseBuffer();
		RedrawAll();
	}

	return UG_SUCCESS;
}


int CResultCtrl::OnMenuStart(int col, long row, int section)
{
	//****** Empty the Menu!!
	EmptyMenu();

	if (section == UG_CORNERBUTTON)
	{

	}
	else if (section == UG_TOPHEADING)
	{
		AddMenuItem(1001, _T("Changer le titre"));
	}
	else if (section == UG_GRID || section == UG_SIDEHEADING)
	{
		AddMenuItem(ID_EDIT_COPY, _T("Copier"));

		CWeatherDatabasePtr pProject = GetProjectPtr();
		CDPTProjectProperties& properties = pProject->m_properties;
		if (properties.m_bEditable)
		{
			AddMenuItem(ID_EDIT_PASTE, _T("Coller"));
			AddMenuItem(2004, _T("Effacer"));
		}
	}


	return TRUE;
}

void CResultCtrl::OnMenuCommand(int col, long row, int section, int item)
{ 
	if (section == UG_CORNERBUTTON)
	{

	}
	else if(section == UG_TOPHEADING)
	{
		if (item == 1001)
		{
			CNewNameDlg dlg;
			dlg.m_name = QuickGetText(col, row);
			if (dlg.DoModal() == IDOK)
			{
				QuickSetText(col, row, dlg.m_name);  
				Invalidate();
			}
			
		}
	}
	else if (section == UG_GRID || section == UG_SIDEHEADING)
	{ 
		//****** The user has selected the 'Copy' option
		if (item == ID_EDIT_COPY )
		{
			CopySelected();
		}

		//****** The user has selected the 'Paste' option
		if (item == ID_EDIT_PASTE)
		{
			Paste();
		}
		
		if (item == 2004)
		{
			DeleteSelection();
		}
	}
	

} 

BOOL CResultCtrl::PreTranslateMessage(MSG* pMsg)
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
				CWeatherDatabasePtr pProject = GetProjectPtr();
				CDPTProjectProperties& properties = pProject->m_properties;
				if (properties.m_bEditable)
				{
					//get the text from the clipboard
					Paste();

					return TRUE; // this doesn't need processing anymore
				}
			}
		}
	}

	if (pMsg->wParam == VK_DELETE)
	{
		DeleteSelection();
	}

	return CResultCtrlBase::PreTranslateMessage(pMsg); // all other cases still need default processing
}


int CResultCtrl::OnVScrollHint(long row,CString *string)
{
	*string = QuickGetText(-1, row);

	return TRUE;
}

int CResultCtrl::OnEditStart(int col, long row, CWnd **edit)
{

	CWeatherDatabasePtr pProject = GetProjectPtr();
	CDPTProjectProperties& properties = pProject->m_properties;

	// Start edit when user double clicks in one of the cells
	if (!properties.m_bEditable)
		return FALSE;

	return CResultCtrlBase::OnEditStart(col, row, edit);
}

void CResultCtrl::OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
	
	StartEdit();
}

void CResultCtrl::OnCharDown(UINT *vcKey, BOOL processed)
{
	UNREFERENCED_PARAMETER(processed);
	StartEdit(*vcKey);
}

void CResultCtrl::DeleteSelection()
{
	
	int col;
	long row;
	CUGCell cell;

	//enum selected items and add them to the string
	int rt = m_GI->m_multiSelect->EnumFirstSelected(&col, &row);
	long lastrow = row;
	while (rt == UG_SUCCESS)
	{
		//get the selected cell then copy the string
		GetCellIndirect(col, row, &cell);

		//check the cut flag
		if (cell.GetReadOnly() != TRUE)
		{
			//cell.ClearAll();
			cell.SetText(_T(""));
			SetCell(col, row, &cell);
			
			CRect rect;
			GetCellRect(col, row, rect);
			InvalidateRect(rect);
		}

		//update the last row flag
		lastrow = row;

		//find the next selected item
		rt = m_GI->m_multiSelect->EnumNextSelected(&col, &row);	
	}
}

void CResultCtrl::OnKeyUp(UINT *vcKey, BOOL processed)
{
	UNREFERENCED_PARAMETER(processed);

	if (*vcKey == VK_F2)
	{
		// Start edit when user double clicks in one of the cells
		StartEdit(NULL);
	}
	else if (*vcKey == VK_DELETE)
	{
		DeleteSelection();
	}
}





/////////////////////////////////////////////////////////////////////////////
//	OnTabSelected
//		Called when the user selects one of the tabs on the bottom of the screen
//	Params:
//		ID	- id of selected tab
//	Return:
//		<none>
void CResultCtrl::OnTabSelected(int ID)
{
	CResultCtrlBase::OnTabSelected(ID);


	CWeatherDatabasePtr pProject = GetProjectPtr();
	CDPTProjectProperties& properties = pProject->m_properties;

	string studyName = m_info[ID].m_studyName;

	if (studyName != properties.m_studyName)
	{
		//OnSheetSetup(ID);
		SetSheetNumber(ID, true);

		if (m_info[ID].m_bNeedInit)
		{
			InitSheet(m_info[ID]);
			m_info[ID].m_bNeedInit = false;
		}

		
		properties.m_studyName = studyName;
		properties.m_curCol = GetCurrentCol();
		properties.m_curRow = GetCurrentRow();
		GetDocument()->UpdateAllViews(((CView*)GetParent()), CHourlyEditorDoc::PROPERTIES_CHANGE, NULL);
	}

}

//void CResultCtrl::OnSelectionChanged(int startCol, long startRow, int endCol, long endRow, int blockNum)

/***************************************************
OnCellChange
Sent whenever the current cell changes
Params:
oldcol, oldrow - coordinates of cell that is loosing the focus
newcol, newrow - coordinates of cell that is gaining the focus
Return:
<none>
****************************************************/
void CResultCtrl::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
{
	ASSERT(newrow >= -1 && newrow < GetNumberRows());
	CWeatherDatabasePtr pProject = GetProjectPtr();
	CDPTProjectProperties& properties = pProject->m_properties;

	
	//int row = startRow;// (startRow == endRow) ? startRow : -1;

	if (newrow != properties.m_curRow || newcol != properties.m_curCol || properties.m_studyName != GetActiveTabName())
	{
		properties.m_studyName = GetActiveTabName();
		properties.m_curRow = newrow;
		properties.m_curCol = newcol;
		GetDocument()->UpdateAllViews(((CView*)GetParent()), CHourlyEditorDoc::PROPERTIES_CHANGE, NULL);
	}

}


void CResultCtrl::OnAdjustComponentSizes(RECT *grid, RECT *topHdg, RECT *sideHdg, RECT *cnrBtn, RECT *vScroll, RECT *hScroll, RECT *tabs)
{
	if (tabs)
	{
		CWeatherDatabasePtr pProject = GetProjectPtr();
		if (pProject && (hScroll->right - hScroll->left>0) )
		{
			CDPTProjectProperties& properties = pProject->m_properties;
			properties.m_tabWidth = tabs->right - tabs->left;
		}
	}
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
COLORREF CResultCtrl::OnGetDefBackColor(int section)
{
	UNREFERENCED_PARAMETER(section);
	
	return GetSysColor(COLOR_APPWORKSPACE);
}



int CResultCtrl::OnHint(int col, long row, int section, CString *string)
{
	CWeatherDatabasePtr pProject = GetProjectPtr();
	if (pProject)
	{
		if (section == UG_GRID)
		{
			if (col >= 0 && row >= 0)
			{
				CStudiesDefinitions& studies = pProject->m_studiesDefinitions;
				int tab = GetCurrentTab();
				ASSERT(m_info.size() == m_CUGTab->GetTabCount());
			
				const CStudyDefinition& study = studies.at(m_info[tab].m_studyName);
				const CFieldsDefinitions& fields = study.m_fields;
			

				std::map<int, std::string>::const_iterator itField = m_info[tab].m_fieldsPositions.find(col);
				ASSERT(itField != m_info[tab].m_fieldsPositions.end());
				const CFieldDefinition& field = fields.at(itField->second);

			
				const CStudiesData& dataMap = pProject->m_studiesData;
				CStudiesData::const_iterator it = dataMap.find(m_info[tab].m_studyName);
				ASSERT(it != dataMap.end());

				if (it != dataMap.end())
				{
					const CStudyData& data = it->second;
					ASSERT(row < (long)data.size_y() && col < (int)data.size_x());
					
					*string = CString(field.GetCode(data[row][col]).c_str());
				}
			}
		}
		else if (section == UG_TAB)
		{

		}
	}


	return !string->IsEmpty();
}
