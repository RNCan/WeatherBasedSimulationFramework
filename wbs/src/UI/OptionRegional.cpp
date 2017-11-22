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
#include "UI/OptionRegional.h"
#include "UI/Common/UtilWin.h"
#include "Basic/Registry.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"

using namespace WBSF;
using namespace std;
using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	/////////////////////////////////////////////////////////////////////////////
	//	OnSetup
	//		This function is called just after the grid window 
	//		is created or attached to a dialog item.
	//		It can be used to initially setup the grid
	void CTimeFormatGridCtrl::OnSetup()
	{
		CStringArrayEx strTitle1(IDS_CMN_TIME_FORMAT_DATAHEAD1);
		CStringArrayEx strTitle2(IDS_CMN_TIME_FORMAT_DATAHEAD2);
		CStringArrayEx strTitle3(IDS_CMN_OUTPUT_TYPE_NAME); strTitle3.RemoveAt(strTitle3.GetSize() - 1);

		//SetDefColWidth( 80 );
		SetDefRowHeight(18);
		SetDefColWidth(115);
		SetSH_Width(75);

		SetTH_NumberRows(2);
		SetTH_Height(40);
		SetTH_RowHeight(-1, 20);
		SetTH_RowHeight(-2, 20);
		SetUserSizingMode(0);

		SetNumberCols(int(strTitle1.GetSize()*strTitle2.GetSize()));
		SetNumberRows(int(strTitle3.GetSize()));

		JoinCells(2, -2, 3, -2);
		JoinCells(0, -2, 1, -2);


		for (int i = 0; i < strTitle1.GetSize(); i++)
		{
			QuickSetText(i * 2, -2, strTitle1[i]);
			for (int j = 0; j < strTitle2.GetSize(); j++)
				QuickSetText(i * 2 + j, -1, strTitle2[j]);
		}


		ASSERT(strTitle3.GetSize() == GetNumberRows());
		for (int i = 0; i < strTitle3.GetSize(); i++)
			QuickSetText(-1, i, strTitle3[i]);


		//change font of header
		CUGCell cell;
		GetHeadingDefault(&cell);
		CFont* pFont = GetParent()->GetFont();
		cell.SetFont(pFont);
		SetHeadingDefault(&cell);

		EnableToolTips();
	}


	void CTimeFormatGridCtrl::SetFormat(const WBSF::CTRefFormat& format)
	{
		ASSERT(GetNumberCols() == CTM::NB_MODE * 2);
		ASSERT(GetNumberRows() == CTM::NB_REFERENCE - 1);


		for (int t = 0; t < WBSF::CTM::NB_REFERENCE - 1; t++)
		{
			for (int m = 0; m < WBSF::CTM::NB_MODE; m++)
			{
				QuickSetText(m * 2, t, Convert(format.GetHeader(CTM(t, m))));
				QuickSetText(m * 2 + 1, t, Convert(format.GetFormat(CTM(t, m))));
			}
		}

		//	BestFit(0,CLocation::ELEV,25,UG_BESTFIT_TOPHEADINGS );
		SetHaveChange(false);
	}

	void CTimeFormatGridCtrl::GetFormat(CTRefFormat& format)
	{
		ASSERT(GetNumberCols() == WBSF::CTM::NB_MODE * 2);
		ASSERT(GetNumberRows() == WBSF::CTM::NB_REFERENCE - 1);

		for (int t = 0; t < WBSF::CTM::NB_REFERENCE - 1; t++)
		{
			for (int m = 0; m < WBSF::CTM::NB_MODE; m++)
			{
				format.SetHeader(WBSF::CTM(t, m), ToUTF8(QuickGetText(m * 2, t)).c_str());
				format.SetFormat(WBSF::CTM(t, m), ToUTF8(QuickGetText(m * 2 + 1, t)).c_str());
			}
		}

	}

	/////////////////////////////////////////////////////////////////////////////
	// COptionRegional property page



	COptionRegional::COptionRegional() : CMFCPropertyPage(COptionRegional::IDD)
	{
		//m_bInit = false;

		WBSF::CRegistry registry;
		m_listDelimiter = registry.GetListDelimiter();
		m_decimalDelimiter = registry.GetDecimalDelimiter();


		VERIFY(m_font.CreateFont(
			16,                        // nHeight
			0,                         // nWidth
			0,                         // nEscapement
			0,                         // nOrientation
			FW_NORMAL,					// nWeight
			FALSE,                     // bItalic
			FALSE,                     // bUnderline
			0,                         // cStrikeOut
			ANSI_CHARSET,              // nCharSet
			OUT_DEFAULT_PRECIS,        // nOutPrecision
			CLIP_DEFAULT_PRECIS,       // nClipPrecision
			DEFAULT_QUALITY,           // nQuality
			DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
			_T("Courier New")));          // lpszFacename

	}



	COptionRegional::~COptionRegional()
	{
	}

	void COptionRegional::DoDataExchange(CDataExchange* pDX)
	{
		CMFCPropertyPage::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_CMN_OPTION_LIST_DELIMITER, m_listDelimiterCtrl);
		DDX_Text(pDX, IDC_CMN_OPTION_LIST_DELIMITER, m_listDelimiter);
		DDX_Control(pDX, IDC_CMN_OPTION_DECIMAL_DELIMITER, m_decimalDelimiterCtrl);
		DDX_Text(pDX, IDC_CMN_OPTION_DECIMAL_DELIMITER, m_decimalDelimiter);
		DDX_Control(pDX, IDC_CMN_REFORMAT, m_reformatCtrl);
		
		m_listDelimiterCtrl.SetFont(&m_font);
		m_decimalDelimiterCtrl.SetFont(&m_font);

	}


	BEGIN_MESSAGE_MAP(COptionRegional, CMFCPropertyPage)
		ON_BN_CLICKED(IDC_CMN_REFORMAT, &COptionRegional::OnBnClickedCmnReformat)
	END_MESSAGE_MAP()

	

	

	BOOL COptionRegional::OnInitDialog()
	{
		CMFCPropertyPage::OnInitDialog();

		m_menu.LoadMenu(IDR_MENU_REFORMAT);
		m_reformatCtrl.m_hMenu = m_menu.GetSubMenu(0)->GetSafeHmenu();


		WBSF::CRegistry registry;
		m_listDelimiter = registry.GetListDelimiter();
		m_decimalDelimiter = registry.GetDecimalDelimiter();


		WBSF::CRegistry option("Time Format");


		CTRefFormat format;

		for (int t = 0; t < WBSF::CTM::NB_REFERENCE - 1; t++)
		{
			for (int m = 0; m < WBSF::CTM::NB_MODE; m++)
			{
				CTM tm(t, m);
				string header = option.GetProfileString(tm.GetTypeModeName() + string("[header]"), WBSF::CTRefFormat::GetDefaultHeader(tm));
				string value = option.GetProfileString(tm.GetTypeModeName() + string("[format]"), WBSF::CTRefFormat::GetDefaultFormat(tm));
				format.SetHeader(tm, header.c_str());
				format.SetFormat(tm, value.c_str());
			}
		}

		if (m_formatCtrl.GetSafeHwnd() == NULL)
			m_formatCtrl.AttachGrid(this, IDC_CMN_OPTION_TIME_FORMAT);

		m_formatCtrl.SetFormat(format);

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}



	void COptionRegional::OnOK()
	{

		//if page is init
		if (m_listDelimiterCtrl.GetSafeHwnd() != NULL)
		{
			WBSF::CRegistry registry;
			if (m_listDelimiter.GetLength() > 0)
				registry.SetListDelimiter(ToUTF8(m_listDelimiter)[0]);
			if (m_decimalDelimiter.GetLength() > 0)
				registry.SetDecimalDelimiter(ToUTF8(m_decimalDelimiter)[0]);


			WBSF::CRegistry option("Time Format");

			WBSF::CTRefFormat format;
			m_formatCtrl.GetFormat(format);

			//Set current format
			WBSF::CTRef::SetFormat(format);

			//Set format to registry
			for (int t = 0; t < CTM::NB_REFERENCE - 1; t++)
			{
				for (int m = 0; m < CTM::NB_MODE; m++)
				{
					WBSF::CTM tm(t, m);

					option.WriteProfileString(tm.GetTypeModeName() + string("[header]"), format.GetHeader(tm));
					option.WriteProfileString(tm.GetTypeModeName() + string("[format]"), format.GetFormat(tm));
				}
			}
		}

		CMFCPropertyPage::OnOK();
	}

	void COptionRegional::OnBnClickedCmnReformat()
	{
		
		std::string sep=",";
		switch (m_reformatCtrl.m_nMenuResult)
		{
		case ID_REFORMAT1:
		{
			CString txt;
			m_listDelimiterCtrl.GetWindowText(txt);
			if (m_listDelimiter.GetLength() > 0)
				sep = ToUTF8(txt)[0];
			break;
		}
			
		case ID_REFORMAT2:
			sep = "-";
			break;
		case ID_REFORMAT3:
			sep = "/";
			break;
		default: 
			sep = ",";
			break;
		}

		CTRefFormat format;
		//m_formatCtrl.GetFormat(format);

		for (int t = 0; t < WBSF::CTM::NB_REFERENCE - 1; t++)
		{
			for (int m = 0; m < WBSF::CTM::NB_MODE; m++)
			{
				CTM tm(t, m);
				string header = WBSF::CTRefFormat::GetDefaultHeader(tm);
				string value = WBSF::CTRefFormat::GetDefaultFormat(tm);
				ReplaceString(header, ",", sep);
				ReplaceString(value, ",", sep);

				format.SetHeader(tm, header.c_str());
				format.SetFormat(tm, value.c_str());
			}
		}

		m_formatCtrl.SetFormat(format);
		m_formatCtrl.Invalidate();

	}
}