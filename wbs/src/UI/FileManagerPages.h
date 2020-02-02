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


#include "basic/ERMsg.h"
#include "UI/Common/CommonCtrl.h"
#include "FileManager/FileManager.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{

	
	/////////////////////////////////////////////////////////////////////////////
	// CNormalFMPage
	class CNormalMListBox : public CBioSIMListBox
	{
	public:

		CNormalMListBox();

		virtual CWnd* OnCreateList();

		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Normals(); }
		virtual void OnEditItem(int iItem);
		virtual void OnLink();
		virtual bool CanEditLabel(int iItem)const{ return false; }

		DECLARE_MESSAGE_MAP()
	};
	
	typedef std::shared_ptr < CBioSIMListBox > CBioSIMListBoxPtr;
	
	class CCommonMPage : public CMFCPropertyPage
	{
	public:
		CCommonMPage(UINT nIDTemplate, UINT nIDCaption = 0);
		~CCommonMPage();

		virtual const WBSF::CDirectoryManager& GetManager()const=0;
		virtual CBioSIMListBoxPtr GetList() = 0;


	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()
		afx_msg void OnSelChange(NMHDR * pNotifyStruct, LRESULT * result);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		
		CBioSIMListBoxPtr m_pList;
		COpenDirEditCtrl m_filePathCtrl;
	};


	class CNormalMPage : public CCommonMPage
	{
	public:

		CNormalMPage();
		//~CNormalMPage();


	protected:

		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();
		
		DECLARE_MESSAGE_MAP()
	};



	/////////////////////////////////////////////////////////////////////////////
	// CDailyMPage

	class CDailyMListBox : public CBioSIMListBox
	{
	public:

		CDailyMListBox();

		virtual CWnd* OnCreateList();


		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Daily(); }
		virtual void CreateNewItem();
		virtual void OnEditItem(int iItem);
		virtual void OnLink();
		virtual bool CanEditLabel(int iItem)const{ return false; }

		DECLARE_MESSAGE_MAP()
	};


	class CDailyMPage : public CCommonMPage
	{
	public:
		CDailyMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};



	/////////////////////////////////////////////////////////////////////////////
	// CHourlyMPage

	class CHourlyMListBox : public CBioSIMListBox
	{
	public:

		CHourlyMListBox();

		virtual CWnd* OnCreateList();

		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Hourly(); }
		virtual void CreateNewItem();
		virtual void OnEditItem(int iItem);
		virtual void OnLink();
		virtual bool CanEditLabel(int iItem)const{ return false; }

		DECLARE_MESSAGE_MAP()
	};


	class CHourlyMPage : public CCommonMPage
	{
	public:
		CHourlyMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};

	/////////////////////////////////////////////////////////////////////////////
	// CHourlyMPage

	class CGribMListBox : public CBioSIMListBox
	{
	public:

		CGribMListBox();

		virtual CWnd* OnCreateList();

		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Gribs(); }
		virtual void CreateNewItem();
		virtual void OnEditItem(int iItem);
		virtual void OnLink();
		virtual bool CanEditLabel(int iItem)const{ return false; }

		DECLARE_MESSAGE_MAP()
	};


	class CGribMPage : public CCommonMPage
	{
	public:
		CGribMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};


	/////////////////////////////////////////////////////////////////////////////
	// CMapInputMPage

	class CMapInputMListBox : public CBioSIMListBox
	{
	public:

		CMapInputMListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().MapInput(); }
		virtual void OnShowInfo(int iItem);
		virtual void OnLink();
		virtual void OnEditItem(int iItem);
		virtual bool CanEditLabel(int iItem)const{ return false; }

		DECLARE_MESSAGE_MAP()
	};


	class CMapInputMPage : public CCommonMPage
	{
	public:
		CMapInputMPage();

		CString m_lastDEMName;

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};



	/////////////////////////////////////////////////////////////////////////////
	// CModelsPage dialog

	class CModelMListBox : public CBioSIMListBox
	{
	public:

		CModelMListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Model(); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnEditItem(int iItem);


		DECLARE_MESSAGE_MAP()
	};


	class CModelMPage : public CCommonMPage
	{
	public:
		CModelMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};



	/////////////////////////////////////////////////////////////////////////////
	// CModelsPage dialog

	class CWeatherUpdateMListBox : public CBioSIMListBox
	{
	public:

		CWeatherUpdateMListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().WeatherUpdate(); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnEditItem(int iItem);


		DECLARE_MESSAGE_MAP()
	};


	class CWeatherUpdateMPage : public CCommonMPage
	{
	public:
		CWeatherUpdateMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();

		DECLARE_MESSAGE_MAP()
	};


	/////////////////////////////////////////////////////////////////////////////
	// CScriptMPage page

	class CScriptMListBox : public CBioSIMListBox
	{
	public:

		CScriptMListBox();

		virtual CWnd* OnCreateList();
		virtual const CDirectoryManager& GetManager(){ return WBSF::GetFM().Script(); }
		virtual void OnAfterAddItem(int iItem);
		virtual void OnEditItem(int iItem);


		DECLARE_MESSAGE_MAP()
	};


	class CScriptMPage : public CCommonMPage
	{
	public:
		CScriptMPage();

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual const WBSF::CDirectoryManager& GetManager()const;
		virtual CBioSIMListBoxPtr GetList();


		DECLARE_MESSAGE_MAP()
	};


}