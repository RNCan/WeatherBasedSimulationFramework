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

#include "ModelBase/ModelInputParameter.h"

namespace WBSF
{

	class CModelFormDialog;
	/////////////////////////////////////////////////////////////////////////////
	// CModelFormItem window

	class CModelFormItem : public CWnd
	{
		// Construction
	public:

		//CModelFormItem(const CModelFormItem& in);
		CModelFormItem(CModelInputParameterDef& in);
		~CModelFormItem();

		//	void SetParam(CModelInputParameterDef& in);
		CString GetName() const{ return CString(m_param.GetName().c_str()); }


		//	CModelFormItem& operator=(const CModelFormItem& in);
		BOOL Create(CModelFormDialog* pParentWnd, UINT nID);

		static const int kGridedDis;

		// Attributes
	public:


		// Generated message map functions
	protected:

		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnPaint();
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
		afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		DECLARE_MESSAGE_MAP()


	private:
		void DrawField(CPaintDC& dc);
		CPoint& GetGridedPoint(CPoint& point);
		CRect& GetGridedPoint(CRect& rect);

		CModelInputParameterDef& m_param;
		static HCURSOR m_hFourArrow;
		static HCURSOR m_hHorArrow;
		static HCURSOR m_hArrow;

		CPoint m_lastPoint;
		bool m_bBeginSFTrack;
		bool m_bBeginSize;
		bool m_bMoveWnd;
		CModelFormDialog* m_pParent;

	};

	/////////////////////////////////////////////////////////////////////////////
}