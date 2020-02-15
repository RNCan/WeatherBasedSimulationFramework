#if !defined(AFX_CHECKABLEGROUPBOX_H__BBBAE5EC_53CC_47CE_A858_4E7E7C24DDF1__INCLUDED_)
#define AFX_CHECKABLEGROUPBOX_H__BBBAE5EC_53CC_47CE_A858_4E7E7C24DDF1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CheckableGroupBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCheckableGroupBox window
/**
*	Lots of time we want to disable a set of controls in our dialog.
*	Group box is a good control to categorise a few controls together, but unfortunately
*	it cannot enable/disable his controls.  I extended CButton class and made it very
*	simple to accomplish and it is encapsulated into one class.  
*
*	So you can change the title of a group box into a check box or radio button
*	( then you need more group box of course).  
*
*	I must confess I borrowed some idea of other talented programmers.
*
*	How to use it
*
*	1. Draw a group box in resource editor as usual.  In resource editor, change the style
		property of group box to icon or bitmap, this prevents Overlapped text when tab is pressed.
*	2. Add a member variable for this added group box, but choose CCheckableGroupBox as 
*		control type.
*	3. In OnInitDialog(), call m_yourVariable.SetTitleStyle(BS_AUTOCHECKBOX); to change normal
*		title to a check box, or use BS_AUTORADIOBUTTON for radio box.
*	4. If you want a group of group box toggle by radio box title, just create more Checkable group box
*		as you already did, and call SetGroupID to give them a group!
*	5. In updated version of this control, I support DDX_Check, which means now you can use UpdateData 
*		to update and save the state of check box in an standard MFC fashion.  
*
* History
*  Updated (21/11/03) - Supports windows WM_ENABLE message. Now you can call EnableWindow() to disable the whole group! 
*  Updated (24/11/03) - Supports DDX_Check for variable exchange. Please refer to demo project for details. 
*  Updated (28/11/03) - Support BN_CLICKED message when user click title box.
*/



class CCheckableGroupBox : public CButton
{
	DECLARE_DYNAMIC(CCheckableGroupBox)

// Construction
public:
	CCheckableGroupBox();

// Attributes
public:
	void SetTitleStyle(UINT style = BS_AUTOCHECKBOX);
	int GetCheck() const;
	void SetCheck(int nCheck);

	void SetGroupID(UINT nGroup);
	UINT GetGroupID() const;

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCheckableGroupBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCheckableGroupBox();
	// Generated message map functions
protected:
	//{{AFX_MSG(CCheckableGroupBox)
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	afx_msg void OnClicked() ;
	afx_msg LRESULT OnGetCheck(WPARAM, LPARAM);
	afx_msg LRESULT OnSetCheck(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	CButton	 m_TitleBox;//Could be check box or radio box
	UINT	 m_nGroupID;//Radio button holds same group id.
	void CheckGroupboxControls();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHECKABLEGROUPBOX_H__BBBAE5EC_53CC_47CE_A858_4E7E7C24DDF1__INCLUDED_)
