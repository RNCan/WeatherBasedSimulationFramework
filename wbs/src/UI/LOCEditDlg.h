//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include "Basic/Location.h"
#include "Basic/FileStamp.h"
#include "FileManager/FileManager.h"
#include "UI/Common/BioSIMListBox.h"
#include "UI/Common/MyToolTipCtrl.h"
#include "UI/Common/UGEditCtrl.h"
#include "UI/Common/OpenDirEditCtrl.h"
#include "UI/Common/CommonCtrl.h"


#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CLocToolBar : public CMFCToolBar
	{
	public:
		virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
		{
			CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
		}
		virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
		virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
		virtual BOOL AllowShowOnList() const { return FALSE; }
	};


	class CLOCGridCtrl : public CUGEditCtrl
	{
	public:

		enum TCoodFormat{ DECIMALS_DEGREES, DMS };

		virtual void OnSetup();

		void GetData(CLocationVector& data);
		void SetData(const CLocationVector& data);

		short GetFormat()const { return m_format; }
		void SetFormat(short format);

		virtual int OnMenuStart(int col, long row, int section);
		virtual void OnMenuCommand(int col, long row, int section, int item);
		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnSetCell(int col, long row, CUGCell *cell);
		virtual int OnVScrollHint(long row, CString *string);
		virtual BOOL PreTranslateMessage(MSG* pMsg);

		
		int		AppendRow();
		int		DeleteRow(long row);

	private:

		void CreateBoldFont();
		afx_msg void OnDestroy();
		DECLARE_MESSAGE_MAP()

		CMyToolTipCtrl m_headerTips;
		short m_format;
		CLocationVector m_locations;
		WBSF::StringVector m_header;

		CFont			m_font;
		CFont			m_fontBold;
		CPen			m_cellBorderPen;

		std::vector<boost::dynamic_bitset<size_t>> m_bDataEdited;


		
	};


	class CLocDlg : public CDialog
	{
		// Construction
	public:
		CLocDlg(CWnd* pParent = NULL);   // standard constructor

		enum { IDD = IDD_SIM_LOC };

		void SetLoc(const CLocationVector& loc);
		bool GetLoc(CLocationVector& loc);

		// Overrides
		virtual BOOL Create(CWnd* pParentWnd);
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		virtual void OnCancel();


		BOOL EnableWindow(BOOL bEnable);

		bool GetHaveChange()const{ return m_grid.GetHaveChange(); }
		CWnd* m_pLocEditDlg;
		// Implementation
	protected:

		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg void OnLocGenerate();
		afx_msg void OnExtractSSI();
		afx_msg void OnLocNewLine();
		afx_msg void OnLocDeleteLine();
		afx_msg void OnFormatChange();
		afx_msg BOOL OnFormatChange(UINT ID);
		afx_msg void OnDestroy();
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void AdjustLayout();
		afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
		


		void UpdateCtrl();
		afx_msg void OnUpdateCtrl(CCmdUI* pCmdUI);



		CStatic	m_nbStation;
		CLOCGridCtrl m_grid;
		CLocToolBar m_wndToolBar;


		BOOL m_bEnable;


		static ERMsg ExtractSSI(CLocationVector& locations, const std::string& filePath, size_t interpolationType, bool bExtractElev, bool bExtractSlopeAspect, bool bShoreDistance, bool bMissingOnly, CCallback& callback);
		static ERMsg ExtractGoogleName(CLocationVector& locations, const std::string& googleAPIKey, bool bReplaceAll, CCallback& callback);
		static ERMsg ExtractGoogleElevation(CLocationVector& locations, const std::string& googleAPIKey, bool bReplaceAll, CCallback& callback);
	
	};




	//*******************************************************************
	class CLocListBox : public CBioSIMListBox
	{
	public:

		CLocListBox();
		~CLocListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Loc(); }

		//virtual void OnEndEditLabel(LPCTSTR lpszLabel);
		virtual void OnAfterAddItem(int iItem);
		virtual void OnSelectionChanged();
		virtual void OnShowMap(int iItem);
		virtual void OnExcel(int iItem);
		virtual BOOL OnBeforeCopyItem(int iItem, CString newName);

		ERMsg LoadLoc(const CString& TGName, CLocationVector& Loc);
		ERMsg SaveLoc(const CString& TGName, bool bAskConfirm = true);

		void OnAppActivate();

		CLocDlg* m_pLocDlg;

	protected:

		DECLARE_MESSAGE_MAP()

		//CLocationVector m_lastLoc;
		int m_lastSelection;
		CFileStamp m_lastFileStamp;
	};

	class CLocationsFileManagerDlg : public CDialog
	{
		// Construction
	public:

		CLocationsFileManagerDlg(CWnd* pParent = NULL);   // standard constructor
		virtual ~CLocationsFileManagerDlg();



		CString m_locName;


		// Implementation
	protected:


		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual void OnOK();
		virtual BOOL DestroyWindow();

		// Dialog Data
		enum { IDD = IDD_SIM_LOC_MANAGER };

		afx_msg void OnSelChange(NMHDR * pNotifyStruct, LRESULT * pResult);
		afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
		afx_msg void OnIdleUpdateCmdUI();
		afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);

		DECLARE_MESSAGE_MAP()

	private:

		void UpdateCtrl(void);
		COpenDirEditCtrl m_filePathCtrl;

		CLocListBox m_fileListCtrl;

	};

}