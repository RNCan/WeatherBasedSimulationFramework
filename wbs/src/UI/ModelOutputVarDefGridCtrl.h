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

#include "ModelBase/ModelOutputVariable.h"
#include "UI/Common/UGEditCtrl.h"
#include "UI/Common/TmplEx/arrayEx.h"

namespace WBSF
{

	class CModelOutputVarDefGridCtrl : public CUGEditCtrl
	{
	public:

		CModelOutputVarDefGridCtrl(int nbColsShow = CModelOutputVariableDef::NB_MEMBERS);
		virtual void OnSetup();

		void GetData(CModelOutputVariableDefVector& data);
		void SetData(const CModelOutputVariableDefVector& data);

		virtual void OnGetCell(int col, long row, CUGCell *cell);
		virtual void OnSetCell(int col, long row, CUGCell *cell);
		int OnEditStart(int col, long row, CWnd **edit);

		virtual int OnMenuStart(int col, long row, int section);
		virtual void OnMenuCommand(int col, long row, int section, int item);

	protected:

		void GetVariableDef(int row, CModelOutputVariableDef& varDef);
		void SetVariableDef(int row, const CModelOutputVariableDef& varDef);
		//int GetSelection(int col, int row);
		//void SetSelection(int col, int row, int sel);
		int GetSelection(int col, CString str);
		CString GetSelectionString(int col, int sel);


		int m_nbColsShow;

		CStringArrayEx OUTPUT_TYPE_NAME;
		CStringArrayEx EXTENDED_TYPE_NAME;

	};

}