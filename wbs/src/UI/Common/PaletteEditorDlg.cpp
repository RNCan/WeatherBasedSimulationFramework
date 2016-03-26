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
#include "PaletteEditorDlg.h"
#include "SYShowMessage.h"
#include "NewNameDlg.h"
#include "UI/Common/UtilWin.h"

#include "WeatherBasedSimulationString.h"

using namespace UtilWin;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPaletteEditorDlg dialog


CPaletteEditorDlg::CPaletteEditorDlg(const CString& path, CWnd* pParent /*=NULL*/):
CDialog(CPaletteEditorDlg::IDD, pParent),
m_path(path),
m_rangeColorList(CColorListBox::RANGE),
m_fixedColorList(CColorListBox::FIXED)
{
    ASSERT( !m_path.IsEmpty() );
}


void CPaletteEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PE_DELETE, m_deleteCtrl);
	DDX_Control(pDX, IDC_PE_RANGE_COLOR, m_rangeColorList);
	DDX_Control(pDX, IDC_PE_FIXED_COLOR, m_fixedColorList);
	DDX_Control(pDX, IDC_PE_LIST, m_paletteList);
}


BEGIN_MESSAGE_MAP(CPaletteEditorDlg, CDialog)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_PE_NEW, OnNewPalette)
	ON_BN_CLICKED(IDC_PE_DELETE, OnDeletePalette)
	ON_BN_CLICKED(IDC_PE_IMPORT, OnImportPalette)
	ON_LBN_SELCHANGE(IDC_PE_LIST, OnSelPaletteChange)
	ON_LBN_SELCHANGE(IDC_PE_RANGE_COLOR, OnSelRangeColorChange)
	ON_LBN_SELCHANGE(IDC_PE_FIXED_COLOR, OnSelFixedColorChange)
	ON_BN_CLICKED(IDC_PE_COLOR_NEW, OnNewProfile)
	ON_BN_CLICKED(IDC_PE_COLOR_DELETE, OnDeleteProfile)
	ON_BN_CLICKED(IDC_PE_UP, OnProfileUp)
	ON_BN_CLICKED(IDC_PE_DOWN, OnProfileDown)
	ON_BN_DOUBLECLICKED(IDC_PE_UP, OnProfileUp)
	ON_BN_DOUBLECLICKED(IDC_PE_DOWN, OnProfileDown)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE, OnPaletteStyleChange)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPaletteEditorDlg message handlers

int CPaletteEditorDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
    
	return 0;
}

void CPaletteEditorDlg::OnNewPalette() 
{
	CString newName;

    if( !SavePalette() ) 
        return;

    
    CNewNameDlg dlg;

    
    while( dlg.DoModal() == IDOK )
    {
        CString filePath = GetPaletteFilePath(CString(dlg.m_name.c_str()));
        if( !UtilWin::FileExist( filePath ) )
        {
            UtilWin::CreateMultipleDir(m_path);

            CMyPalette palette;
            
            ERMsg message;
            message = palette.Save(filePath);

            if( message )
            {
                int pos = GetPaletteListCtrl().AddString(CString(dlg.m_name.c_str()));
                GetPaletteListCtrl().SetCurSel(pos);
                FillColorList(palette);
                break;
            }
			else
			{
				UtilWin::SYShowMessage(message, this);
			}
        }
        else
        {
            CString message;
            message.FormatMessage(IDS_BSC_NAME_EXIST, dlg.m_name);
            MessageBox( message, NULL, MB_ICONEXCLAMATION|MB_OK);   
        }
    }

    UpdateButton();
}

CString CPaletteEditorDlg::GetPaletteFilePath(const CString& name)
{
    return m_path + name + _T(".pal");
}

void CPaletteEditorDlg::OnDeletePalette() 
{
    int curSel = GetPaletteListCtrl().GetCurSel();
    ASSERT( curSel != -1);

	CString name;
	GetPaletteListCtrl().GetText(curSel, name);

    CString messageStr;
    AfxFormatString1(messageStr, IDS_BSC_CONFIRM_DELETE, name);

    if( MessageBox( messageStr, AfxGetAppName(), MB_YESNO) == IDYES )
    {
        
	    if( remove(UtilWin::ToUTF8(GetPaletteFilePath(name)).c_str() ) != -1 )
	    {
		    int nCount = GetPaletteListCtrl().DeleteString(curSel);
		    
		    if( nCount  > 0)
            {
			    if(curSel == nCount)
                    GetPaletteListCtrl().SetCurSel(curSel-1);
			    else GetPaletteListCtrl().SetCurSel(curSel);
                
		    
                CString name;
        	    GetPaletteListCtrl().GetText(GetPaletteListCtrl().GetCurSel(), name);

                if( !name.IsEmpty() )
                {
                    CMyPalette palette;
                    ERMsg message;
                    message=palette.Load(GetPaletteFilePath(name));
                    
					if( message )
    		            FillColorList(palette);
                    else 
						SYShowMessage(message, this);
                }
            }
	    }
	    
	    UpdateButton();
    }
	
}

void CPaletteEditorDlg::OnImportPalette() 
{
    if( !SavePalette() ) 
        return;	

    /*CString sFolder;

    CBioSimOption option;
    option.GetLastOpenPath("LOC", sFolder );

	CString ext = m_fileManager.GetLOCExt();

    CString sTitle;
    sTitle.LoadString(IDS_IMPORT);

    CString sLocFile;
    sLocFile.LoadString(IDS_OPEN_LOCFILE);

	CImportDialog openDialog(true, "", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_EXTENSIONDIFFERENT|OFN_ALLOWMULTISELECT, sLocFile, this, sTitle);
	openDialog.m_ofn.lpstrInitialDir = sFolder;
    openDialog.m_ofn.lpstrTitle = sTitle;
    

	if(openDialog.DoModal() == IDOK)
	{
        CString lastLOCName;
        CString sOutMessage;
        ERMsg message;
        CString name;
        

        POSITION pos = openDialog.GetStartPosition();
        while( pos != NULL)
        {
            name = openDialog.GetNextPathName( pos );
	        
            
		    //if( UtilWin::GetFileExtention(name) != ext)
		    //{
			    //CString sOutMessageTmp;
			    
                //name = UtilWin::GetFilePath(name) + UtilWin::GetFileTite(name) + ext
                
			    //AfxFormatString2(sOutMessageTmp,IDS_BADEXT, ext, UtilWin::GetFileName(name) );
                //sOutMessage += sOutMessageTmp + "\r\n";
		    //}
				    
            message = m_fileManager.ImportLOC(name);
			if( message )
			{
                lastLOCName = UtilWin::GetFileTitle(name);
			}
			else
			{
                for(int i=0; i<message.dimension(); i++)
                {
                    sOutMessage += message[i].data();
                    sOutMessage += "\r\n";
                }
			}

		    
        }

        if( !sOutMessage.IsEmpty() )
        {
            MessageBox(sOutMessage, AfxGetAppName(), MB_ICONSTOP|MB_OK);
        }

        if( PutLOCFileInLOCList() > 0)
	    {
		    int pos = m_LOCList.FindStringExact(-1, lastLOCName);
		    if(pos != LB_ERR)m_LOCList.SetCurSel(pos);
	    }
	    
	    SetStationList();
	    UpdateButton();

    
        option.SetLastOpenPath("LOC", UtilWin::GetPath(name) );

	}
	*/
}

BOOL CPaletteEditorDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	GetUpCtrl().SetIcon(AfxGetApp()->LoadIcon( IDI_UP ));
	GetDownCtrl().SetIcon(AfxGetApp()->LoadIcon( IDI_DOWN ));
	GetNewCtrl().SetIcon(AfxGetApp()->LoadIcon( IDI_NEW ));
	GetDeleteCtrl().SetIcon(AfxGetApp()->LoadIcon( IDI_DELETE ));

 //   m_upCtrl.AutoLoad(IDC_PE_UP, this);
//	m_downCtrl.AutoLoad(IDC_PE_DOWN, this);

	//m_deleteProfileCtrl.AutoLoad(IDC_PE_COLOR_DELETE, this);
	//m_newProfileCtrl.AutoLoad(IDC_PE_COLOR_NEW, this);
    

    FillPaletteList();
    CMyPalette palette;
    palette.CreateDefaultPalette(15);

    int pos = GetPaletteListCtrl().FindStringExact(-1, m_paletteName );
    if( pos == -1 && GetPaletteListCtrl().GetCount() > 0)
    {
        pos = 0;
        GetPaletteListCtrl().GetText(pos, m_paletteName);
    }

    if( pos != -1 )
    {
        GetPaletteListCtrl().SetCurSel(pos);
        CMyPalette palette;
        
        ERMsg message;
        message = palette.Load( GetPaletteFilePath(m_paletteName) );
        if( message )
            FillColorList(palette);
        else SYShowMessage( message, this);
    }
    
    UpdateButton();
 
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPaletteEditorDlg::UpdateButton()
{
    bool bRange = GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE;
    bool bSelection = GetPaletteListCtrl().GetCurSel() != -1;

    bool bProfileSelection = false;

    if( bRange )
        bProfileSelection = GetRangeColorListCtrl().GetSelCount() != 0;
    else bProfileSelection = GetFixedColorListCtrl().GetSelCount() != 0;
    
    GetRangeColorListCtrl().ShowWindow(bRange?SW_SHOW:SW_HIDE);
    GetFixedColorListCtrl().ShowWindow((!bRange)?SW_SHOW:SW_HIDE);

    GetRangeColorListCtrl().EnableWindow(bSelection);
    GetFixedColorListCtrl().EnableWindow(bSelection);

    m_deleteCtrl.EnableWindow(bSelection);
    GetDeleteCtrl().EnableWindow(bSelection&&bProfileSelection);
    GetNewCtrl().EnableWindow(bSelection);
    if( bRange )
    {
        GetUpCtrl().EnableWindow( bSelection&&GetRangeColorListCtrl().SelectionUpEnable());
	    GetDownCtrl().EnableWindow( bSelection&&GetRangeColorListCtrl().SelectionDownEnable());
    }
    else
    {
        GetUpCtrl().EnableWindow( bSelection&&GetFixedColorListCtrl().SelectionUpEnable());
	    GetDownCtrl().EnableWindow( bSelection&&GetFixedColorListCtrl().SelectionDownEnable());
    }
}

void CPaletteEditorDlg::FillPaletteList()
{
	GetPaletteListCtrl().ResetContent();
	CString file = m_path + _T("*.pal");
	
	CFileFind fileList;
	
	BOOL working = fileList.FindFile(file, 0);
	
	while( working )
	{
		working = fileList.FindNextFile();
        GetPaletteListCtrl().AddString(fileList.GetFileTitle());
	}

}


void CPaletteEditorDlg::OnSelPaletteChange() 
{
	if( !SavePalette() ) 
        return;	
	
    CString name;
    GetPaletteListCtrl().GetText(GetPaletteListCtrl().GetCurSel(), name);

    if( !name.IsEmpty() )
    {
        CMyPalette palette;
        ERMsg message;
        message=palette.Load(GetPaletteFilePath(name));
        if( message )
        {
    		FillColorList(palette);
        }
        else SYShowMessage(message, this);
    }

    UpdateButton();	    
}

bool CPaletteEditorDlg::SavePalette()
{

    CMyPalette oldPalette;
    CMyPalette newPalette;
    
    if( GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE)
    {
        newPalette = GetRangeColorListCtrl().GetPalette();
    }
    else
    {
        newPalette = GetFixedColorListCtrl().GetPalette();
    }

    if( newPalette.GetFilePath().IsEmpty() )
        return true;

    bool bSave = true;
    if( oldPalette.Load(newPalette.GetFilePath() ) )
    {
        if( oldPalette == newPalette)
            bSave = false;
    }

    if( bSave )
    {
        CString sOutMessage;
        AfxFormatString1(sOutMessage, IDS_BSC_CONFIRM_SAVE, UtilWin::GetFileTitle(newPalette.GetFilePath()));

        bool modify = true;
		if( MessageBox(sOutMessage, AfxGetAppName(), MB_YESNO) == IDNO)
            modify = false;

        if( modify )
        {
            //ask confirme
            ERMsg message;
            message = newPalette.Save( newPalette.GetFilePath() );

            if(!message)
            {
                SYShowMessage(message, this);
                return false;
            }
        }
    }

    return true;
}

void CPaletteEditorDlg::FillColorList(const CMyPalette& palette)
{
    GetRangeColorListCtrl().SetPalette(palette);
    GetFixedColorListCtrl().SetPalette(palette);
    
    UINT ID = IDC_PE_PALETTE_RANGE;
    if( palette.GetFormatType() == CMyPalette::FIXE)
        ID = IDC_PE_PALETTE_FIXE;


    CheckRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE, ID);

}

void CPaletteEditorDlg::OnSelRangeColorChange() 
{
	UpdateButton();
}

void CPaletteEditorDlg::OnSelFixedColorChange() 
{
    UpdateButton();
}


void CPaletteEditorDlg::OnNewProfile() 
{
    bool bRange = GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE;

    if( bRange )
        GetRangeColorListCtrl().AddNew();
    else GetFixedColorListCtrl().AddNew();

    UpdateButton();
}

void CPaletteEditorDlg::OnDeleteProfile() 
{
    bool bRange = GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE;

    if( bRange )
        GetRangeColorListCtrl().DeleteSelected();
    else GetFixedColorListCtrl().DeleteSelected();

    UpdateButton();
}

void CPaletteEditorDlg::OnProfileUp() 
{
    bool bRange = GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE;

    if( bRange )
        GetRangeColorListCtrl().SelectionUp();
    else GetFixedColorListCtrl().SelectionUp();

    UpdateButton();
}

void CPaletteEditorDlg::OnProfileDown() 
{
    bool bRange = GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE;

    if( bRange )
        GetRangeColorListCtrl().SelectionDown();
    else GetFixedColorListCtrl().SelectionDown();

    UpdateButton();
}

void CPaletteEditorDlg::OnPaletteStyleChange(UINT ID)
{
    //transfer color from a control to the other
    if( GetCheckedRadioButton(IDC_PE_PALETTE_RANGE, IDC_PE_PALETTE_FIXE) == IDC_PE_PALETTE_RANGE)
    {

        CMyPalette newPalette;
        newPalette = GetFixedColorListCtrl().GetPalette();
        GetRangeColorListCtrl().SetPalette(newPalette);
    }
    else
    {
        CMyPalette newPalette;
        newPalette = GetRangeColorListCtrl().GetPalette();
        GetFixedColorListCtrl().SetPalette(newPalette);
    }


    UpdateButton();
}

void CPaletteEditorDlg::OnOK() 
{
    if( SavePalette() )
    {
        int curSel = GetPaletteListCtrl().GetCurSel();
        ASSERT( curSel != -1);

	    GetPaletteListCtrl().GetText(curSel, m_paletteName);

	    CDialog::OnOK();
    }
}
