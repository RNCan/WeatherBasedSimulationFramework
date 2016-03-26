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


#include "UI/Common/CommonCtrl.h"
#include "ModelBase/ModelOutputVariable.h"
#include "Simulation/Export.h"


namespace WBSF
{

	//**********************************************************************************
	class CStatisticSelectionCtrl : public CSelectionCtrl
	{
	public:

		CStatisticSelectionCtrl();

		DECLARE_MESSAGE_MAP()
	};

	class CVariableSelectionCtrl : public CXHtmlTree
	{
	public:

		//	CVariableSelectionCtrl();

		void GetData(CVariableDefineVector& data);
		void SetData(const CVariableDefineVector& data);

		void SetOutputDefinition(const CModelOutputVariableDefVector& outputVar)
		{
			m_outputVar = outputVar;
			InitTree();
		}
		const CModelOutputVariableDefVector& GetOutputDefinition()const{ return m_outputVar; }

		size_t GetNbField(size_t dimension)const;
		CString GetFieldName(int d, int f)const{ return GetFieldTitle(d, f, FALSE); }
		CString GetFieldTitle(int d, int f, bool bTitle = true)const;

	protected:

		virtual void Init();
		virtual void PreSubclassWindow();

		void InitTree();

		HTREEITEM GetItem(int d, int f)const;

		CModelOutputVariableDefVector m_outputVar;

		DECLARE_MESSAGE_MAP()
		afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	};

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, CVariableDefineVector& selection);

}