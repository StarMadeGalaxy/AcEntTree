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
#include "StdAfx.h"
#include "resource.h"
#include <sstream>

enum class SaveXfMode
{
	SELECTED_ENTITY = 0,
	SELECTED_ENTITIES,
	THE_WHOLE_PROJECT
};


//-----------------------------------------------------------------------------
class CRmWindow : public CAcUiDialog {
	DECLARE_DYNAMIC(CRmWindow)

public:
	CRmWindow(CWnd* pParent = NULL, HINSTANCE hInstance = NULL);
	enum { IDD = IDD_RMWINDOW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	afx_msg void OnBnClickedButtonSelectEntity();
	afx_msg void OnBnClickedButtonSelectEntities();
	afx_msg void OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedSaveXf();
public:
	void PostNcDestroy();
	void OnOk();
	void OnCancel();
	void OnClose();
public:
	CTreeCtrl m_treeCtrl;
	CFolderPickerDialog m_dlg;
	CEdit folder_path_entry;
	// This section has nothing to do with the with the window. Just helping functions
private:
	SaveXfMode save_instruction;  // .DWG is going to be saved according to this variable
private:
	ads_name selected_entity;	// ads_name of the entity we select from the mfc window
	// OR						<--
	AcDbObjectIdArray ids;		// ids of the entities we select from the mfc window
private:
	std::unordered_map<AcRxClass*, std::wstring> objs_xf_filenames;
	std::unordered_map<AcRxClass*, std::size_t> objs_counters;

	std::wstring path_from_mfc;
private:

private:	// Meshing functions
	std::wstring mesh_file_str = L"polys.xf";

	void mesh_obj(AcDbEntity* pEntity);

	void circle_meshing(AcDbEntity* entity, std::size_t N); // N is a number of points of single circle mesh
	void line_meshing(AcDbEntity* entity);
	void subdmesh_meshing(AcDbEntity* entity);


private:	// Handling gui functionality
	void SaveAsXf();
	void write_obj_data_to_xf_file(AcDbEntity* pEntity, const AcGeMatrix3d& trans = AcGeMatrix3d());
	void insert_to_tree(AcDbEntity* pBlock, const AcGeMatrix3d& trans = AcGeMatrix3d(), HTREEITEM base_item = nullptr);
	void insert_coord_to_item(AcDbEntity* pEntity, HTREEITEM base_item, const AcGeMatrix3d& trans = AcGeMatrix3d());
	void add_tree_cstr_f(HTREEITEM base_item, const ACHAR* format, ...);
	void select_path_using_folder_picker();
private:	// helper funcitons
	void reset_counters();
	std::string CRmWindow::formatDouble(double value);
	const std::wstring reduced_name(const AcDbEntity* ent) const;
};


