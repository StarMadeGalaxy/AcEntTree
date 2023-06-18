// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- RmWindow.h : Declaration of the CRmWindow
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
#include "adui.h"
#include "resource.h"

//-----------------------------------------------------------------------------
class CRmWindow : public CAdUiBaseDialog {
	DECLARE_DYNAMIC (CRmWindow)

public:
	CRmWindow (CWnd *pParent =NULL, HINSTANCE hInstance =NULL) ;
	enum { IDD = IDD_RMWINDOW} ;

protected:
	virtual void DoDataExchange (CDataExchange *pDX);
	afx_msg LRESULT OnAcadKeepFocus (WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonSelect();
	afx_msg void OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedCancel();
public: // temp stuff
	void PostNcDestroy();
	void OnOk();
	void OnCancel();
public:
	CTreeCtrl m_treeCtrl;
// This section has nothing to do with the with the window. Just helping functions
private:
	void insert_to_tree(AcDbEntity* pBlock, HTREEITEM base_item = nullptr);
	void insert_coord_to_item(AcDbEntity* pEntity, HTREEITEM base_item);
	void add_tree_cstr_f(HTREEITEM base_item, const ACHAR* format, ...);
	const std::wstring reduced_name(const AcDbEntity* ent) const;
//
} ;
