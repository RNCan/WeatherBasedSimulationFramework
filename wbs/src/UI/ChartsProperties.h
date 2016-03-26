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
#include "afxpropertygridctrl.h"
#include "afxvslistbox.h"

#include "Simulation/Graph.h"
#include "WeatherBasedSimulationUI.h"


namespace WBSF
{


	class CSeriesPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:


		CSeriesPropertyGridCtrl(CGraphVector& graphics);


		void Set(int chartIndex, int serieIndex, bool bForceReload = false);


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);
		//void SetGraph(CGraph& graph);
		//void SetSerie(size_t serieIndex);


		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);

		void Init();


	protected:

		DECLARE_MESSAGE_MAP()
		//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		//virtual void PreSubclassWindow();






		CGraphVector& m_graphics;
		//CGraph* m_pGraph;

		int m_chartIndex;
		int m_serieIndex;
	};


	class CSeriesListCtrl : public CVSListBox
	{
	public:

		CSeriesListCtrl(CGraphVector& graphics, CSeriesPropertyGridCtrl& seriesPropertyGridCtrl) :
			m_graphics(graphics),
			m_seriesPropertyGridCtrl(seriesPropertyGridCtrl)
		{
			m_chartIndex = -1;
			m_bInProcess = false;
		}

		void SetChartIndex(int index);
		virtual void OnSelectionChanged();
		virtual BOOL OnBeforeRemoveItem(int iItem);
		virtual void OnAfterAddItem(int iItem);
		virtual void OnAfterRenameItem(int iItem);
		virtual void OnAfterMoveItemUp(int iItem);
		virtual void OnAfterMoveItemDown(int iItem);
		virtual void OnClickButton(int iButton);

	protected:

		bool m_bInProcess;
		CSeriesPropertyGridCtrl& m_seriesPropertyGridCtrl;
		CGraphVector& m_graphics;
		int m_chartIndex;
	};


	class CChartsListCtrl : public CVSListBox
	{
	public:

		CChartsListCtrl(CGraphVector& graphics, CSeriesListCtrl& seriesListCtrl) :
			m_graphics(graphics),
			m_seriesListCtrl(seriesListCtrl)
		{
			m_bInProcess = false;
		}

		void FillCharts();

		//virtual BOOL SelectItem(int iItem);
		virtual void OnSelectionChanged();
		virtual BOOL OnBeforeRemoveItem(int iItem);
		virtual void OnAfterAddItem(int iItem);
		virtual void OnAfterRenameItem(int iItem);
		virtual void OnAfterMoveItemUp(int iItem);
		virtual void OnAfterMoveItemDown(int iItem);
		virtual void OnClickButton(int iButton);

	protected:

		bool m_bInProcess;
		CGraphVector& m_graphics;
		CSeriesListCtrl& m_seriesListCtrl;
	};





	// CChartsProperties dialog

	class CChartsProperties : public CDialog
	{
		DECLARE_DYNAMIC(CChartsProperties)

	public:


		CGraphVector m_graphics;


		CChartsProperties(CWnd* pParent = NULL);   // standard constructor
		virtual ~CChartsProperties();

		// Dialog Data
		enum { IDD = IDD_CHARTS_PROPETIES };

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		DECLARE_MESSAGE_MAP()
	public:

		CChartsListCtrl m_chartsCtrl;
		CSeriesListCtrl m_seriesCtrl;
		CSeriesPropertyGridCtrl m_seriesPropertiesCtrl;


		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnDestroy();
		afx_msg void AdjustLayout();


	};

}