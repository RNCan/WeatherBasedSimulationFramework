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

#include <unordered_map>
#include <hash_map>

#include "ChartCtrl/ChartCtrl.h"

#include "Basic/NormalsDatabase.h"
#include "Simulation/Graph.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/ScrollHelper.h"
#include "UI/Common/SplitterControl.h"

class CChartTRefAxis;

namespace WBSF
{


	//**************************************************************************************************************************************

	typedef std::weak_ptr<CChartCtrl> CChartCtrlWeakPtr;

	typedef std::shared_ptr<CChartCtrl> CChartCtrlPtr;
	typedef std::vector<CChartCtrlPtr> CChartCtrlMap;
	typedef std::shared_ptr<CSplitterControl> CSplitterControlPtr;
	typedef std::vector<CSplitterControlPtr> CSplitterControlPtrMap;
	typedef std::unique_ptr<CScrollHelper> CScrollHelperPtr;

	typedef std::bitset < NORMALS_DATA::NB_FIELDS > CNormalsVariables;


	class CNormalsChartsCtrl : public CWnd
	{
	public:

		CNormalsChartsCtrl();
		int Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle);

		CWVariables		m_variables;
		float m_zoom;

		CNormalsStationPtr  m_pStation;

		void Update();
		void UpdateScrollHelper();


		afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnSplitterDelta(UINT ID, NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

		LRESULT DrawDragLine(const CPoint& oldPt, const CPoint& newPt, int w = 2);

		CChartCtrlMap& GetCharts(){ return m_charts; }

		ERMsg SaveAsImage(const std::string& strFilename);
		void CopyToClipboard();

		static CGraphVector GetCharts(const CWVariables& variables);
		static std::string GetGraphisFilesPath();

	protected:



		std::string		m_lastStationID;
		float			m_lastZoom;

		//internal use
		CChartCtrlMap m_charts;
		CSplitterControlPtrMap m_splitters;
		CGraphVector m_lastChartsDefine;

		DECLARE_MESSAGE_MAP()



		void AdjustLayout();


		static bool RegisterWindowClass();
		static const TCHAR* WINDOW_CLASS_NAME;


	protected:

		CPoint m_lastPoint;
		CScrollHelperPtr m_scrollHelper;

	};

}