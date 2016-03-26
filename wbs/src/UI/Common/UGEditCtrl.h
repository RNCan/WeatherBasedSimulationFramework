//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "UltimateGrid/UGCtrl.h"


class CUGEditCtrl: public CUGCtrl
{
public:
	CUGEditCtrl(void);
	~CUGEditCtrl(void);
 
	virtual void OnSetCell(int col,long row,CUGCell *cell);
	virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
	virtual void OnCharDown(UINT *vcKey,BOOL processed);
	virtual void OnKeyUp(UINT *vcKey,BOOL processed);
	
	bool GetHaveChange()const { return m_bHaveChange; }
	void SetHaveChange(bool bHaveChange){ m_bHaveChange=bHaveChange; }
	virtual int OnCanSizeRow(long row) {UNREFERENCED_PARAMETER(row);return FALSE; }
	virtual int OnCanSizeTopHdg() {return FALSE; }
	//virtual int OnCanSizeSideHdg() {return TRUE; }
	
	void DeleteSelection();
	
protected:

	//void LoadDefaultWidth(CString section, CString ctrlName);
	//void SaveDefaultWidth(CString section, CString ctrlName);
	

	
	bool m_bHaveChange;
	int m_nbDay;
};
