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
//----- RmWindow.cpp : Implementation of CRmWindow
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "RmWindow.h"
#include <axlock.h>

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(CRmWindow, CAcUiDialog)

BEGIN_MESSAGE_MAP(CRmWindow, CAcUiDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)

	ON_BN_CLICKED(IDOK, &CRmWindow::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CRmWindow::OnBnClickedCancel)

	ON_BN_CLICKED(IDC_BUTTON_SELECT_ENTITY, &CRmWindow::OnBnClickedButtonSelectEntity)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ENTITIES, &CRmWindow::OnBnClickedButtonSelectEntities)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CRmWindow::OnTvnSelchangedTree1)

	ON_BN_CLICKED(ID_SAVE_DXF_ENTITY, &CRmWindow::OnBnClickedSaveDxf)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
CRmWindow::CRmWindow(CWnd* pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAcUiDialog(CRmWindow::IDD, pParent)
{
	save_instruction = SaveDxfMode::THE_WHOLE_PROJECT;
}

//-----------------------------------------------------------------------------
void CRmWindow::DoDataExchange(CDataExchange* pDX) {
	CAcUiDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_treeCtrl);
	DDX_Control(pDX, IDC_FOLDER_PATH, folder_path_entry);
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT CRmWindow::OnAcadKeepFocus(WPARAM, LPARAM) {
	return (TRUE);
}

const std::wstring CRmWindow::reduced_name(const AcDbEntity* ent) const
{
	std::wstring ent_name(ent->isA()->name());
	std::wstring ent_name_without_AcDb(ent_name.substr(4, ent_name.size()));
	return ent_name_without_AcDb;
}


void CRmWindow::insert_to_tree(AcDbEntity* pEntity, HTREEITEM base_item, AcGePoint3d pos)
{
	AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
	std::wstring rname = reduced_name(pEntity);

	if (pBlockRef)
	{
		HTREEITEM base_blockref_item = m_treeCtrl.InsertItem(rname.c_str(), base_item);
		// Traverse the nested entities within the block reference
		AcDbObjectId blockId = pBlockRef->blockTableRecord();
		AcDbBlockTableRecord* pBlockTR;
		if (acdbOpenObject(pBlockTR, blockId, AcDb::kForRead) == Acad::eOk)
		{
			AcDbBlockTableRecordIterator* pIterator;
			if (pBlockTR->newIterator(pIterator) == Acad::eOk)
			{
				for (; !pIterator->done(); pIterator->step())
				{
					AcDbEntity* pNestedEntity;
					if (pIterator->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
					{
						AcGePoint3d cs = pBlockRef->position() + pos.asVector();
						insert_to_tree(pNestedEntity, base_blockref_item, cs);
						pNestedEntity->close();
					}
				}
				delete pIterator;
			}
			pBlockTR->close();
		}
	}
	else
	{
		HTREEITEM base_for_coords = m_treeCtrl.InsertItem(rname.c_str(), base_item);
		insert_coord_to_item(pEntity, base_for_coords, pos);
	}
}

void CRmWindow::insert_coord_to_item(AcDbEntity* pEntity, HTREEITEM base_item, AcGePoint3d coordinate_system)
{
	if (pEntity->isKindOf(AcDbCircle::desc()))
	{
		// Process Circle entity
		AcDbCircle* pCircle = AcDbCircle::cast(pEntity);

		double radius = pCircle->radius(); 
		double thickness = pCircle->thickness();

		AcGePoint3d center = pCircle->center() + coordinate_system.asVector();


		// Print coordinates
		add_tree_cstr_f(base_item, _T("Circle Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
		add_tree_cstr_f(base_item, _T("Circle Radius: %lf\n"), radius);
		add_tree_cstr_f(base_item, _T("Circle Thickness: %lf\n"), thickness);


		std::ofstream file("circle.xf", std::ios::app);

		
		if (file.is_open())
		{
			file << std::fixed << std::setprecision(8)
				<< thickness << '\n'
				<< center.x << '\n' << center.y << '\n' << center.z << '\n'
				<< radius << '\n';
				
		}
	}
	else if (pEntity->isKindOf(AcDbArc::desc()))
	{
		// Process Arc entity
		AcDbArc* pArc = AcDbArc::cast(pEntity);
		AcGePoint3d center = pArc->center() + coordinate_system.asVector();
		double radius = pArc->radius();
		double startAngle = pArc->startAngle();
		double endAngle = pArc->endAngle();

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Arc Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
		add_tree_cstr_f(base_item, _T("Arc Radius: %lf\n"), radius);
		add_tree_cstr_f(base_item, _T("Arc Start Angle: %lf\n"), startAngle);
		add_tree_cstr_f(base_item, _T("Arc End Angle: %lf\n"), endAngle);
	}
	else if (pEntity->isKindOf(AcDbSolid::desc()))
	{
		// Process Solid entity
		AcDbSolid* pSolid = AcDbSolid::cast(pEntity);

		AcGePoint3d vertex1, vertex2, vertex3, vertex4;
		pSolid->getPointAt(0, vertex1);
		pSolid->getPointAt(1, vertex2);
		pSolid->getPointAt(2, vertex3);
		pSolid->getPointAt(3, vertex4);

		vertex1 += coordinate_system.asVector();
		vertex2 += coordinate_system.asVector();
		vertex3 += coordinate_system.asVector();
		vertex4 += coordinate_system.asVector();

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Solid Vertex 0: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 1: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 2: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 3: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
	}
	else if (pEntity->isKindOf(AcDbPolyline::desc()))
	{
		// Process Polyline entity
		AcDbPolyline* pPolyline = AcDbPolyline::cast(pEntity);
		int numVertices = pPolyline->numVerts();

		for (int i = 0; i < numVertices; ++i)
		{
			AcGePoint3d vertexPosition;
			pPolyline->getPointAt(i, vertexPosition);
			vertexPosition += coordinate_system.asVector();
			// Print coordinates of each vertex
			add_tree_cstr_f(base_item, _T("Polyline Vertex %d: (%lf, %lf, %lf)\n"), i, vertexPosition.x, vertexPosition.y, vertexPosition.z);
		}
	}
	else if (pEntity->isKindOf(AcDbSpline::desc()))
	{
		// Process Spline entity
		AcDbSpline* pSpline = AcDbSpline::cast(pEntity);
		int numControlPoints = pSpline->numControlPoints();

		for (int i = 0; i < numControlPoints; ++i)
		{
			AcGePoint3d controlPoint;
			pSpline->getControlPointAt(i, controlPoint);
			controlPoint += coordinate_system.asVector();
			// Print coordinates of each control point
			add_tree_cstr_f(base_item, _T("Spline Control Point %d: (%lf, %lf, %lf)\n"), i, controlPoint.x, controlPoint.y, controlPoint.z);
		}
	}
	else if (pEntity->isKindOf(AcDbFace::desc()))
	{
		// Process Face entity
		AcDbFace* pFace = AcDbFace::cast(pEntity);

		AcGePoint3d vertex1, vertex2, vertex3, vertex4;
		pFace->getVertexAt(0, vertex1);
		pFace->getVertexAt(1, vertex2);
		pFace->getVertexAt(2, vertex3);
		pFace->getVertexAt(3, vertex4);

		vertex1 += coordinate_system.asVector();
		vertex2 += coordinate_system.asVector();
		vertex3 += coordinate_system.asVector();
		vertex4 += coordinate_system.asVector();

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Face Vertex 0: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 1: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 2: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 3: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
	}
	else if (pEntity->isKindOf(AcDbLine::desc()))
	{
		// Process Line entity
		AcDbLine* pLine = AcDbLine::cast(pEntity);

		AcGePoint3d vertex_start = pLine->startPoint() + coordinate_system.asVector();
		AcGePoint3d vertex_end = pLine->endPoint() + coordinate_system.asVector();

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Line start point (WCS): (%lf, %lf, %lf)\n"), vertex_start.x, vertex_start.y , vertex_start.z );
		add_tree_cstr_f(base_item, _T("Line end point (WCS): (%lf, %lf, %lf)\n"), vertex_end.x, vertex_end.y, vertex_end.z );
		add_tree_cstr_f(base_item, _T("Coordinate system pos (WCS): (%lf, %lf, %lf)\n"), coordinate_system.x, coordinate_system.y, coordinate_system.z);
	}
	else if (pEntity->isKindOf(AcDbPolygonMesh::desc()))
	{
		AcDbPolygonMesh* pMesh;

		acdbOpenObject(pMesh, pEntity->id(), AcDb::kForRead);
		AcDbObjectIterator* pVertIter = pMesh->vertexIterator();
		pMesh->close();

		AcDbPolygonMeshVertex* pVertex;
		AcGePoint3d pt;
		AcDbObjectId vertexObjId;

		for (int vertexNumber = 0; !pVertIter->done(); vertexNumber++, pVertIter->step())
		{
			vertexObjId = pVertIter->objectId();
			pMesh->openVertex(pVertex, vertexObjId, AcDb::kForRead);
			pt = pVertex->position();
			pVertex->close();
			add_tree_cstr_f(base_item, L"PolygonMesh Vertex %d: (%lf, %lf, %lf)\n", vertexNumber, pt[X], pt[Y], pt[Z]);
		}
		delete pVertIter;
	}
	else
	{
		add_tree_cstr_f(base_item, _T("Entity type is not handled.\n"));
	}
}

void CRmWindow::add_tree_cstr_f(HTREEITEM base_item, const ACHAR* format, ...)
{
	const std::size_t buffer_size = 1024;
	ACHAR buffer[buffer_size];

	va_list args;
	va_start(args, format);

	// TODO: buffer might not be zero-terminated
	_vsnwprintf(buffer, buffer_size, format, args);

	m_treeCtrl.InsertItem(buffer, base_item);

	va_end(args);
}

void CRmWindow::OnBnClickedButtonSelectEntity()
{

	save_instruction = SaveDxfMode::SELECTED_ENTITY;

	BeginEditorCommand();

	m_treeCtrl.DeleteAllItems();
	folder_path_entry.SetWindowTextW(TEXT(""));

	ads_point clicked_point;
	ads_name entity_name;

	if (acedEntSel(L"Select entity: ", entity_name, clicked_point) == RTNORM)
	{
		CompleteEditorCommand();
		AcDbObjectId entityId;
		if (acdbGetObjectId(entityId, entity_name) == Acad::eOk)
		{
			selected_entity[0] = entity_name[0];
			selected_entity[1] = entity_name[1];

			AcDbEntity* pEntity = nullptr;
			if (acdbOpenObject(pEntity, entityId, AcDb::kForRead) == Acad::eOk)
			{
				insert_to_tree(pEntity);
				pEntity->close();
			}
		}
	}
	else 
	{
		CancelEditorCommand();
	}
}

void CRmWindow::OnBnClickedButtonSelectEntities()
{
	save_instruction = SaveDxfMode::SELECTED_ENTITIES;

	BeginEditorCommand();

	m_treeCtrl.DeleteAllItems();
	folder_path_entry.SetWindowTextW(TEXT(""));

	ads_name ss;

	if (acedSSGet(NULL, NULL, NULL, NULL, ss) == RTNORM)
	{
		CompleteEditorCommand();
		Adesk::Int32 len = 0;
		acedSSLength(ss, &len);
		if (len > 0)
		{
			ids.setPhysicalLength(len);
			for (Adesk::Int32 i = 0; i < len; i++)
			{
				ads_name en;
				AcDbObjectId id;
				acedSSName(ss, i, en);
				acdbGetObjectId(id, en);
				ids.append(id);
				if (acdbGetObjectId(id, en) == Acad::eOk)
				{
					AcDbEntity* pEntity = nullptr;
					if (acdbOpenObject(pEntity, id, AcDb::kForRead) == Acad::eOk)
					{
						insert_to_tree(pEntity);
						pEntity->close();
					}
				}
			}
		}
	}
	else
	{
		CancelEditorCommand();
	}
}

void CRmWindow::select_path_using_folder_picker(CString object_name)
{
	CFolderPickerDialog m_dlg;

	m_dlg.m_ofn.lpstrTitle = _T("Select a folder to save a file at...");
	//m_dlg.m_ofn.lpstrInitialDir = _T("C:\\");
	if (m_dlg.DoModal() == IDOK) {
		path_from_mfc = m_dlg.GetPathName();   // Use this to get the selected folder name after the dialog has closed

		path_from_mfc += TEXT("\\robomax_output_");
		path_from_mfc += object_name;
		path_from_mfc += TEXT(".dxf");
		UpdateData(FALSE);   // To show updated folder in GUI
	}
	folder_path_entry.SetWindowTextW(path_from_mfc);
}

void CRmWindow::OnBnClickedSaveDxf()
{
	if (m_treeCtrl.GetCount() == 0) {
		save_instruction = SaveDxfMode::THE_WHOLE_PROJECT;
	}

	CString save_known_as;

	switch (save_instruction)
	{
	case SaveDxfMode::SELECTED_ENTITIES:
	{
		save_known_as = "entities";
		break;
	}
	case SaveDxfMode::SELECTED_ENTITY:
	{
		CString object_name;
		AcDbObjectId entityId;
		if (acdbGetObjectId(entityId, selected_entity) == Acad::eOk)
		{
			AcDbEntity* pEntity = nullptr;
			if (acdbOpenObject(pEntity, entityId, AcDb::kForRead) == Acad::eOk)
			{
				CString new_name(reduced_name(pEntity).c_str());
				save_known_as = new_name;
				pEntity->close();
			}
		}
		break;
	}
	case SaveDxfMode::THE_WHOLE_PROJECT:
	{
		save_known_as = "whole_dwg";
		break;
	}
	default: { break; }
	}
	select_path_using_folder_picker(save_known_as);
	SaveAsDxf();
}


void CRmWindow::SaveAsDxf()
{
	AcAxDocLock doclock(acdbCurDwg());	// Lock the database to avoid eLockViolation because of the modeless mfc-window
	AcDbDatabase* dbSource = acdbHostApplicationServices()->workingDatabase();

	switch (save_instruction)
	{
	case SaveDxfMode::SELECTED_ENTITY:
	{
		AcDbDatabase* dbTarget = new AcDbDatabase();
		AcDbObjectId entityId;
		AcDbEntity* pEntity = nullptr;
		// selected_entity is an ads_name variable member as well as path_from_mfc
		if (acdbGetObjectId(entityId, selected_entity) == Acad::eOk)
		{
			if (acdbOpenObject(pEntity, entityId, AcDb::kForRead) == Acad::eOk)
			{
				AcDbObjectId IdModelSpaceTarget = acdbSymUtil()->blockModelSpaceId(dbTarget);
				AcDbObjectIdArray sourceIds;
				sourceIds.append(pEntity->objectId());
				pEntity->close();
				AcDbIdMapping idMap;
				AcDb::DuplicateRecordCloning drc = AcDb::kDrcReplace;
				Acad::ErrorStatus es = dbSource->wblockCloneObjects(sourceIds, IdModelSpaceTarget, idMap, drc, false);
				if (es != Acad::eOk) {
					const ACHAR* errMsg = acadErrorStatusText(es);
					acutPrintf(_T("Error: %s\n"), errMsg);
				}
			}
		}
		acdbDxfOutAsR12(dbTarget, path_from_mfc);
		delete dbTarget;
		break;
	}
	case SaveDxfMode::SELECTED_ENTITIES:
	{
		// Put an address into the file tbh idk why.
		std::wstring metadata_filename(path_from_mfc);
		metadata_filename += L".metadata.txt";
		std::wofstream metadata(metadata_filename);

		if (metadata.is_open())
		{
			metadata << "Listing of the entities inside of the file: " << metadata_filename << "\n";
			for (int i = 0; i < ids.length(); i++)
			{
				AcDbObjectId objectId = ids.at(i);
				AcDbEntity* entity = nullptr;
				if (acdbOpenObject(entity, objectId, AcDb::kForRead) == Acad::eOk)
				{
					CString entityName = entity->isA()->name();
					std::wstring ent_str_name(entityName);
					metadata << "Entity name: " << ent_str_name << "\n";
					entity->close();
				}
			}
			metadata.close();
		}
		AcDbDatabase* tempDb = new AcDbDatabase(Adesk::kFalse);
		Acad::ErrorStatus es;
		if ((es = dbSource->wblock(tempDb, ids, AcGePoint3d::kOrigin)) == Acad::eOk)
		{
			if ((es = acdbDxfOutAsR12(tempDb, path_from_mfc)) != Acad::eOk)
				acutPrintf(L"\nacdbDxfOutAsR12(...) = %s", acadErrorStatusText(es));
		}
		else
		{
			acutPrintf(L"\nacdbCurDwg()->wblock(...) = %s", acadErrorStatusText(es));
		}
		delete tempDb;
		ids.removeAll();
		break;
	}
	case SaveDxfMode::THE_WHOLE_PROJECT:
	{
		acdbDxfOutAsR12(dbSource, path_from_mfc);
		break;
	}
	default: { break; }
	}
}


// Im not quite sure if i need these functions but 
// these are the function generated by the window builder thingy
// so i've decided to keep them here even tho most likely I could go 
// w/o them. I'm a bit lazy to figure it out
void CRmWindow::OnBnClickedOk()
{
	CAcUiDialog::OnOK();
}

void CRmWindow::OnBnClickedCancel()
{
	CAcUiDialog::OnCancel();
}

void CRmWindow::PostNcDestroy()
{
	CAcUiDialog::PostNcDestroy();
	//delete this;
}

void CRmWindow::OnOk()
{
	CAcUiDialog::OnOK();
}

void CRmWindow::OnClose()
{
	CAcUiDialog::OnClose();
}

void CRmWindow::OnCancel()
{
	CAcUiDialog::OnCancel();
}

void CRmWindow::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	// TODO: Add your control notification handler code here
	*pResult = 0;
}

