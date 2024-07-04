// TDateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TDate.h"
#include "TDateDlg.h"
#include "UI/Common/UtilWin.h"
#include "Basic/UtilTime.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/StaticBitmap.h"

using namespace UtilWin;
using namespace WBSF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_CHECK_DAY 1010
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	// ClassWizard generated virtual function overrides
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	
	DECLARE_MESSAGE_MAP()

	//CStaticBitmap	m_bitmap;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_BITMAP, m_bitmap);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDateDlg dialog

CTDateDlg::CTDateDlg(void):
 CTaskBarDialog(IDR_MAINFRAME, IDR_MAINFRAME),
m_bChangeStadard(false),
m_bChangeJulian(false),
m_nDate(-1)
{
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTDateDlg::DoDataExchange(CDataExchange* pDX)
{
	CTaskBarDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DATESTANDARD, m_standard);
	DDX_Control(pDX, IDC_DATEJULIAN, m_julian);
}

BEGIN_MESSAGE_MAP(CTDateDlg, CTaskBarDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATESTANDARD, OnChangeDateStandard)
	ON_EN_CHANGE(IDC_DATEJULIEN, OnChangeDatejulien)
	ON_COMMAND(ID_CLOSE, OnClose)
	ON_COMMAND(ID_SHOW, OnShow)
	ON_WM_DESTROY()
	ON_COMMAND(IDM_ABOUTBOX, OnAboutbox)
	ON_COMMAND(IDM_CONVERT_FILE, OnConvertFile)
	ON_WM_SIZE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDateDlg message handlers


BOOL CTDateDlg::OnInitDialog()
{
	CTaskBarDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

    SetTimer( ID_CHECK_DAY, 60000, NULL );

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	ASSERT((IDM_CONVERT_FILE & 0xFFF0) == IDM_CONVERT_FILE);
	ASSERT(IDM_CONVERT_FILE < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}

		CString strConvertFile;
		strConvertFile.LoadString(IDS_CONVERT_FILE);
		if (!strConvertFile.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_CONVERT_FILE, strConvertFile);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
    SetIcon(m_hIcon, FALSE);		// Set small icon
	SetIcon(m_hIcon, TRUE);			// Set big icon
	
    SetCurrentDate(); 
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTDateDlg::SetCurrentDate(void)
{
    CTime time = CTime::GetCurrentTime();

	size_t jDay = GetJDay(time.GetYear(), time.GetMonth()-1, time.GetDay()-1)+1;
    
	if (jDay != m_nDate)
    {
		m_nDate = jDay;
        m_bChangeJulian = true;
        SetDlgItemInt(IDC_DATEJULIEN, int(m_nDate));

        m_bChangeJulian = false;	

        CString toolTip;
        toolTip.Format(IDS_TOOLTIPS, m_nDate);
        SetToolTip(toolTip);
    }

}

void CTDateDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	if ((nID & 0xFFF0) == IDM_CONVERT_FILE)
	{
		OnConvertFile();
	}
	else
	{
		CTaskBarDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTDateDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CTaskBarDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTDateDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}



void CTDateDlg::OnChangeDateStandard(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
    if( !m_bChangeStadard )
    {
        m_bChangeJulian = true;

        CTime time;
        m_standard.GetTime(time); 
		size_t nDate = GetJDay(time.GetYear(), time.GetMonth() - 1, time.GetDay()-1)+1;
        SetDlgItemInt(IDC_DATEJULIEN, int(nDate));

        m_bChangeJulian = false;	
    }

    	*pResult = 0;
}



void CTDateDlg::OnChangeDatejulien() 
{

    if( !m_bChangeJulian )
    {
        m_bChangeStadard = true;

        int jDay = GetDlgItemInt(IDC_DATEJULIEN);
        
        

		if (jDay > 0 && jDay <= 365)
        {
			CTime time;
			m_standard.GetTime(time);


            m_standard.EnableWindow(true);
			size_t nDay = WBSF::GetDayOfTheMonth(time.GetYear(), jDay - 1) + 1;
			size_t nMonth = WBSF::GetMonthIndex(time.GetYear(), jDay - 1) + 1;

            
            CTime time2(time.GetYear(), int(nMonth), int(nDay), 0,0,0);
            
            m_standard.SetTime(&time2); 
        }
        else m_standard.EnableWindow(false);

        m_bChangeStadard = false;
    }
	

}



void CTDateDlg::OnShow() 
{
    ShowWindow (SW_RESTORE);
    SetForegroundWindow ();
}

void CTDateDlg::OnClose() 
{
    DestroyWindow();
}

void CTDateDlg::OnAboutbox() 
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}

/*ERMsg CTDateDlg::AutoDetectElement(const CStdioFile& file, CDailyFileFormat& format)
{
	//try to find year, month, day, julian day
	CString line;
	bool bContinue = true;

	while( file.ReadString(line) && bContinue)
	{
		CStringArray elem;
		//int nbElement = 0;
		int pos = 0;
		while( pos != -1 )
		{
			CString tmp = line.Tokenize(" \t,;", pos);	
			if( !tmp.IsEmpty() )
			{
				elem.Add( tmp );
			}
		}

		if( elem.GetSize() == 2 )
		{
			//find year
			if( )
				
			bContinue=false;
		}
		else if( nbElement == 3)
		{
		}
		else
		{
			msg.ajoute( GetString(IDS_BAD_FORMAT) );
		}
		
	}


	file.SeekToBegin();
}
*/


ERMsg CTDateDlg::ConvertFile(const CString& filePath)
{
	ERMsg msg;

//	CString filePathOut(filePath);
//	SetFileTitle(filePathOut, GetFileTitle(filePath)+ "_out");
//
//	CStdioFileEx fileIn;
//	CStdioFileEx fileOut;
//	
//	msg += fileIn.Open(filePath, CFile::modeRead);
//	msg += fileOut.Open(filePathOut, CFile::modeWrite|CFile::modeCreate);
//	fileOut.WriteString("YEAR\tMONTH\tDAY\tJDAY\n");
//
//	if( msg)
//	{
//		CDailyFileFormat format;
//		CString line;
//
//		fileIn.ReadString(line);
//		if( format.SetString((LPCTSTR)line) )
//		{
//			while( fileIn.ReadString(line) && msg)
//			{
//				CStringArray elem;
//
//				int pos = 0;
//				while( pos != -1 )
//				{
//					CString tmp = line.Tokenize(" \t,;", pos);	
//					if( !tmp.IsEmpty() )
//					{
//						elem.Add( tmp );
//					}
//				}
//
//				if( elem.GetSize() == (int)format.size() )
//				{
//					int year = -1;
//					int month = -1;
//					int day = -1;
//					int jday = -1;
//
//					for( int i=0; i<elem.GetSize(); i++)
//					{
//						switch( format[i] )
//						{
//						case DAILY_DATA::YEAR: year = ToInt( elem[i] ); break;
//						case DAILY_DATA::MONTH: month = ToInt( elem[i] ); break;
//						case DAILY_DATA::JDAY: jday = ToInt( elem[i] ); break;
//						case DAILY_DATA::DAY: day = ToInt( elem[i] ); break;
//						default: break;//msg.ajoute( GetString(IDS_INVALID_HEADER)); break;
//						}
//					}
//
//					if( year!=-1 && jday!=-1)
//					{
//						//convert jday to y,m,d
//						if( year>=1800 && year <2100 &&
//							jday>=1 && jday<=GetNbDay(year))
//						{
//							//short month = -1;
//							//short day = -1;
//							GetDayAndMonth(jday, year, day, month);
//							CString lineOut;
//							lineOut.Format("%4d\t%2d\t%2d\t%3d\n", year, month, day, jday);	
//							fileOut.WriteString(lineOut);
//						}
//						else
//						{
//							msg.ajoute( GetString(IDS_INVALID_LINE) + line);
//						}
//					}
//					else if( year!=-1 && month!=-1 && day!=-1)
//					{
//						if( year>=1800 && year <2100 &&
//							month>=1 && month<=12 &&
//							day>=1 && day<=GetNbDayPerMonth(month, year) )
//						{
//							jday = UtilWin::GetJDay( day, month, year);
//							
//							CString lineOut;
//							lineOut.Format("%4d\t%2d\t%2d\t%3d\n", year, month, day, jday);	
//							fileOut.WriteString(lineOut);
//						}
//						else
//						{
//							msg.ajoute( GetString(IDS_INVALID_LINE) + line);
//						}
//					}
//					else 
//					{
//						msg.ajoute( GetString(IDS_INVALID_LINE) + line);
//					}
//				}
//				else
//				{
//					msg.ajoute( GetString(IDS_INVALID_NB_ELEM) + line);
//				}
//			}		
//		}
//		else
//		{
//			msg.ajoute( GetString(IDS_INVALID_HEADER) );
//		}
////		msg = AutoDetectElement(fileIn, format);
//		
//		fileIn.Close();
//		fileOut.Close();
//
//	}

	return msg;
}

void CTDateDlg::OnConvertFile()
{
	//CAppOption option;
	//
	//CString sFolder;
	//option.GetLastOpenPath("Convert File Path", sFolder );


	//CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT, "*.*|*.*||",this);
	//dlg.m_ofn.lpstrInitialDir = sFolder;

	//if( dlg.DoModal() == IDOK)
	//{
	//	ERMsg msg;

 //       POSITION pos = dlg.GetStartPosition();
 //       while( pos != NULL)
 //       {
 //           CString filePath = dlg.GetNextPathName( pos );
	//		msg += ConvertFile(filePath);

	//		if( pos == NULL)//last file
	//			option.SetLastOpenPath("Convert File Path", GetPath(filePath) );
 //       }

 //       if( !msg )
 //       {
 //           SYShowMessage(msg, this);
 //       }
	//}
	
}



void CTDateDlg::OnDestroy() 
{
    CAppOption option;

    WINDOWPLACEMENT wndPl;
    if( GetWindowPlacement(&wndPl) )
        option.SetWindowPlacement(_T("MainFrame22"), wndPl) ;
    	
    CTaskBarDialog::OnDestroy();
}



BOOL CTDateDlg::Create(UINT nIDTemplate, CWnd* pParentWnd ) 
{
	if( !CTaskBarDialog::Create(nIDTemplate, pParentWnd ) )
        return FALSE;

    CAppOption option;

    WINDOWPLACEMENT wndPl;
    if( option.GetWindowPlacement(_T("MainFrame22"), wndPl) )
        SetWindowPlacement(&wndPl);

    return TRUE;
}

void CTDateDlg::OnSize(UINT nType, int cx, int cy) 
{
	CTaskBarDialog::OnSize(nType, cx, cy);
	
    switch(nType)
    {
    case SIZE_MINIMIZED: ShowWindow(SW_HIDE); break;
    default: break;
    }
}

void CTDateDlg::OnTimer(UINT_PTR nIDEvent) 
{
    if( nIDEvent == ID_CHECK_DAY )
    {
        SetCurrentDate(); 
        OnChangeDatejulien();
    }


	CTaskBarDialog::OnTimer(nIDEvent);
}

void CTDateDlg::PostNcDestroy() 
{
	CTaskBarDialog::PostNcDestroy();

    delete this;
}
