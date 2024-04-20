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
#include <functional>

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(CRmWindow, CAcUiDialog)

BEGIN_MESSAGE_MAP(CRmWindow, CAcUiDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)

	ON_BN_CLICKED(IDOK, &CRmWindow::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CRmWindow::OnBnClickedCancel)

	ON_BN_CLICKED(IDC_BUTTON_SELECT_ENTITY, &CRmWindow::OnBnClickedButtonSelectEntity)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ENTITIES, &CRmWindow::OnBnClickedButtonSelectEntities)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CRmWindow::OnTvnSelchangedTree1)

	ON_BN_CLICKED(ID_SAVE_DXF_ENTITY, &CRmWindow::OnBnClickedSaveXf)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
CRmWindow::CRmWindow(CWnd* pParent, HINSTANCE hInstance) : CAcUiDialog(CRmWindow::IDD, pParent)
{
	save_instruction = SaveXfMode::THE_WHOLE_PROJECT;

	objs_xf_filenames = {
		{ AcDbCircle::desc(), L"CIRCLE.xf" },
		{ AcDbFace::desc(), L"3DFACE.xf" },
		{ AcDbArc::desc(), L"ARC.xf" },
		{ AcDbPolyline::desc(), L"POLY.xf" },
		{ AcDbSpline::desc(), L"SLINE.xf" },
		{ AcDbSolid::desc(), L"SOLID.xf" },
		{ AcDbBlockReference::desc(), L"INSERT"},
		{ nullptr, L".xf" }
	};
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
	std::size_t len_AcDb = 4;
	std::wstring ent_name(ent->isA()->name());
	std::wstring ent_name_without_AcDb(ent_name.substr(len_AcDb, ent_name.size()));
	return ent_name_without_AcDb;
}

std::string formatDouble(double value) {
	std::stringstream stream;
	stream << std::fixed << std::setprecision(8) << value;
	std::string formattedValue = stream.str();
	if (std::abs(value) < 0.00000001) {
		return ".00000000";
	}
	else
		return formattedValue;
}


void CRmWindow::insert_to_tree(AcDbEntity* pEntity, const AcGeMatrix3d& trans, HTREEITEM base_item)
{
	AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
	std::wstring rname = reduced_name(pEntity);


	if (pEntity->isKindOf(AcDbBlockReference::desc()))
	{
		HTREEITEM base_blockref_item = m_treeCtrl.InsertItem(rname.c_str(), base_item);

		AcDbObjectId blockId = pBlockRef->blockTableRecord();
		AcDbBlockTableRecord* pBlockTR;
		if (acdbOpenObject(pBlockTR, blockId, AcDb::kForRead) == Acad::eOk)
		{
			AcDbBlockTableRecordIterator* it;
			if (pBlockTR->newIterator(it) == Acad::eOk)
			{
				AcGeMatrix3d transSub(trans * pBlockRef->blockTransform());
				for (it->start(); !it->done(); it->step())
				{
					AcDbEntity* pNestedEntity;
					if (it->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
					{
						insert_to_tree(pNestedEntity, transSub, base_blockref_item);
						pNestedEntity->close();
					}
				}
				delete it;
			}
			pBlockTR->close();
		}
	}
	else
	{
		HTREEITEM base_for_coords = m_treeCtrl.InsertItem(rname.c_str(), base_item);
		insert_coord_to_item(pEntity, base_for_coords, trans);
	}
}

void CRmWindow::write_obj_data_to_xf_file(AcDbEntity* pEntity, const AcGeMatrix3d& trans)
{
	// This function is supposed to be called as many times as entities we have

	std::wstring obj_file_name = path_from_mfc + L'\\' + objs_xf_filenames[pEntity->isA()];

	// Think it through later
	// TODO(venci): Find a better solution
	if (pEntity->isKindOf(AcDbBlockReference::desc()))
	{
		static std::size_t id = 1;
		obj_file_name += std::to_wstring(id) + objs_xf_filenames[nullptr];
	}

	std::ofstream file(obj_file_name, std::ios::app);

	if (file.is_open())
	{
		if (pEntity->isKindOf(AcDbCircle::desc()))
		{
			static std::size_t id = 1;

			AcDbCircle* pCircle = AcDbCircle::cast(pEntity);

			double radius = pCircle->radius();
			double thickness = pCircle->thickness();
			AcGeVector3d circ_normal = pCircle->normal(); // TODO(venci): do we need to transform the normal using .transformBy(trans)????
			AcGePoint3d center = pCircle->center().transformBy(trans);

			file << id << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(thickness) << '\n'
				<< formatDouble(center.x) << '\n' << formatDouble(center.y) << '\n' << formatDouble(center.z) << '\n'
				<< formatDouble(radius) << '\n'
				<< formatDouble(circ_normal.x) << '\n' << formatDouble(circ_normal.y) << '\n' << formatDouble(circ_normal.z) << '\n';

			id++;
		}
		else if (pEntity->isKindOf(AcDbArc::desc()))
		{
			static std::size_t id = 1;

			AcDbArc* pArc = AcDbArc::cast(pEntity);

			AcGePoint3d center = pArc->center().transformBy(trans);

			double radius = pArc->radius();
			double startAngle = pArc->startAngle();
			double endAngle = pArc->endAngle();

			file << id << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(center.x) << '\n' << formatDouble(center.y) << '\n' << formatDouble(center.z) << '\n'
				<< formatDouble(radius) << '\n'
				<< formatDouble(startAngle) << '\n' << formatDouble(endAngle) << '\n';

			id++;
		}
		else if (pEntity->isKindOf(AcDbSolid::desc()))
		{
			static std::size_t id = 1;

			AcDbSolid* pSolid = AcDbSolid::cast(pEntity);

			AcGePoint3d vertex1, vertex2, vertex3, vertex4;
			pSolid->getPointAt(0, vertex1);
			pSolid->getPointAt(1, vertex2);
			pSolid->getPointAt(2, vertex3);
			pSolid->getPointAt(3, vertex4);

			vertex1 = vertex1.transformBy(trans);
			vertex2 = vertex2.transformBy(trans);
			vertex3 = vertex3.transformBy(trans);
			vertex4 = vertex4.transformBy(trans);

			file << id << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(vertex1.x) << '\n' << formatDouble(vertex1.y) << '\n' << formatDouble(vertex1.z) << '\n'
				<< formatDouble(vertex2.x) << '\n' << formatDouble(vertex2.y) << '\n' << formatDouble(vertex2.z) << '\n'
				<< formatDouble(vertex3.x) << '\n' << formatDouble(vertex3.y) << '\n' << formatDouble(vertex3.z) << '\n'
				<< formatDouble(vertex4.x) << '\n' << formatDouble(vertex4.y) << '\n' << formatDouble(vertex4.z) << '\n';

			id++;
		}
		else if (pEntity->isKindOf(AcDbFace::desc()))
		{
			static std::size_t id = 1;

			AcDbFace* pFace = AcDbFace::cast(pEntity);

			AcGePoint3d vertex1, vertex2, vertex3, vertex4;

			pFace->getVertexAt(0, vertex1);
			pFace->getVertexAt(1, vertex2);
			pFace->getVertexAt(2, vertex3);
			pFace->getVertexAt(3, vertex4);

			vertex1 = vertex1.transformBy(trans);
			vertex2 = vertex2.transformBy(trans);
			vertex3 = vertex3.transformBy(trans);
			vertex4 = vertex4.transformBy(trans);

			file << id << std::fixed << std::setprecision(8) << '\n'
				<< formatDouble(vertex1.x) << '\n' << formatDouble(vertex1.y) << '\n' << formatDouble(vertex1.z) << '\n'
				<< formatDouble(vertex2.x) << '\n' << formatDouble(vertex2.y) << '\n' << formatDouble(vertex2.z) << '\n'
				<< formatDouble(vertex3.x) << '\n' << formatDouble(vertex3.y) << '\n' << formatDouble(vertex3.z) << '\n'
				<< formatDouble(vertex4.x) << '\n' << formatDouble(vertex4.y) << '\n' << formatDouble(vertex4.z) << '\n';

			id++;
		}
		else if (pEntity->isKindOf(AcDbLine::desc()))
		{
			static std::size_t id = 1;

			AcDbLine* pLine = AcDbLine::cast(pEntity);

			AcGeVector3d line_normal = pLine->normal();
			AcGePoint3d vertex_start = pLine->startPoint().transformBy(trans);
			AcGePoint3d vertex_end = pLine->endPoint().transformBy(trans);

			file << id << '\n' << std::fixed << std::setprecision(8) << pLine->thickness() << '\n'
				<< formatDouble(vertex_start.x) << '\n' << formatDouble(vertex_start.y) << '\n' << formatDouble(vertex_start.z) << '\n'
				<< formatDouble(vertex_end.x) << '\n' << formatDouble(vertex_end.y) << '\n' << formatDouble(vertex_end.z) << '\n'
				<< formatDouble(line_normal.x) << '\n' << formatDouble(line_normal.y) << '\n' << formatDouble(line_normal.z) << '\n';

			id++;
		}
		else if (pEntity->isKindOf(AcDbPolygonMesh::desc()))
		{
			static std::size_t id = 1;

			AcDbPolygonMesh* pMesh;

			acdbOpenObject(pMesh, pEntity->id(), AcDb::kForRead);

			int m_size = pMesh->mSize();
			int n_size = pMesh->nSize();
			AcDbObjectIterator* pVertIter = pMesh->vertexIterator();
			pMesh->close();

			file << id << '\n' << m_size << "     " << n_size << '\n';

			AcDbPolygonMeshVertex* pVertex;
			AcGePoint3d pt;
			AcDbObjectId vertexObjId;

			for (int vertexNumber = 0; !pVertIter->done(); vertexNumber++, pVertIter->step())
			{
				vertexObjId = pVertIter->objectId();
				pMesh->openVertex(pVertex, vertexObjId, AcDb::kForRead);
				pt = pVertex->position().transformBy(trans);
				pVertex->close();
				file << std::fixed << std::setprecision(8)
					<< formatDouble(pt[X]) << '\n' << formatDouble(pt[Y]) << '\n' << formatDouble(pt[Z]) << '\n';
			}

			delete pVertIter;

			id++;
		}
		else if (pEntity->isKindOf(AcDbBlockReference::desc()))
		{
			AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
			AcDbObjectId blockId = pBlockRef->blockTableRecord();
			AcDbBlockTableRecord* pBlockTR;

			if (acdbOpenObject(pBlockTR, blockId, AcDb::kForRead) == Acad::eOk)
			{
				AcString block_name = AcString();
				pBlockTR->getName(block_name);

				AcGePoint3d block_pos = pBlockRef->position();

				file << block_name << '\n' 
					<< 10 << block_pos.x << '\n' 
					<< 20 << block_pos.y << '\n'
					<< 30 << block_pos.z << '\n' << "END";

				AcDbBlockTableRecordIterator* pIterator;
				if (pBlockTR->newIterator(pIterator) == Acad::eOk)
				{
					for (; !pIterator->done(); pIterator->step())
					{
						AcDbEntity* pNestedEntity;
						if (pIterator->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
						{
							AcGeMatrix3d btrans = pBlockRef->blockTransform();
							write_obj_data_to_xf_file(pNestedEntity, btrans);
							pNestedEntity->close();
						}
					}
					delete pIterator;
				}
				pBlockTR->close();
			}
		}
	}
	file.close();
}

void CRmWindow::SaveAsXf()
{
	switch (save_instruction)
	{
	case SaveXfMode::SELECTED_ENTITY:
	{
		AcDbObjectId entityId;
		AcDbEntity* pEntity = nullptr;
		// selected_entity is an ads_name variable member as well as path_from_mfc
		if (acdbGetObjectId(entityId, selected_entity) == Acad::eOk)
		{
			if (acdbOpenObject(pEntity, entityId, AcDb::kForRead) == Acad::eOk)
			{
				write_obj_data_to_xf_file(pEntity);
				pEntity->close();
			}
		}

		break;
	}
	case SaveXfMode::SELECTED_ENTITIES:
	{
		for (int i = 0; i < ids.length(); i++)
		{
			AcDbObjectId objectId = ids.at(i);
			AcDbEntity* entity = nullptr;
			if (acdbOpenObject(entity, objectId, AcDb::kForRead) == Acad::eOk)
			{
				write_obj_data_to_xf_file(entity);
				entity->close();
			}
		}
		ids.removeAll();
		break;
	}
	case SaveXfMode::THE_WHOLE_PROJECT:
	{
		AcDbDatabase* pDatabase = acdbHostApplicationServices()->workingDatabase();
		if (pDatabase == nullptr) {
			return;
		}

		AcDbBlockTable* pBlockTable;
		if (pDatabase->getBlockTable(pBlockTable, AcDb::kForRead) != Acad::eOk) {
			return;
		}

		AcDbBlockTableRecord* pBlockTableRecord;
		if (pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForRead) != Acad::eOk) {
			pBlockTable->close();
			return;
		}

		AcDbBlockTableRecordIterator* pEntityIterator;
		if (pBlockTableRecord->newIterator(pEntityIterator) != Acad::eOk) {
			pBlockTableRecord->close();
			pBlockTable->close();
			return;
		}

		for (pEntityIterator->start(); !pEntityIterator->done(); pEntityIterator->step()) {
			AcDbEntity* pEntity;
			if (pEntityIterator->getEntity(pEntity, AcDb::kForRead) == Acad::eOk) {
				write_obj_data_to_xf_file(pEntity);
				pEntity->close();
			}
		}
		pBlockTableRecord->close();
		pBlockTable->close();
		break;
	}
	default: { break; }
	}
}

void CRmWindow::insert_coord_to_item(AcDbEntity* pEntity, HTREEITEM base_item, const AcGeMatrix3d& trans)
{
	if (pEntity->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle = AcDbCircle::cast(pEntity);

		double radius = pCircle->radius();
		double thickness = pCircle->thickness();

		AcGeVector3d circ_normal = pCircle->normal();

		AcGePoint3d center = pCircle->center().transformBy(trans);


		add_tree_cstr_f(base_item, _T("Circle Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
		add_tree_cstr_f(base_item, _T("Circle Radius: %lf\n"), radius);
		add_tree_cstr_f(base_item, _T("Circle Thickness: %lf\n"), thickness);
	}
	else if (pEntity->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc = AcDbArc::cast(pEntity);
		AcGePoint3d center = pArc->center().transformBy(trans);
		double radius = pArc->radius();
		double startAngle = pArc->startAngle();
		double endAngle = pArc->endAngle();

		add_tree_cstr_f(base_item, _T("Arc Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
		add_tree_cstr_f(base_item, _T("Arc Radius: %lf\n"), radius);
		add_tree_cstr_f(base_item, _T("Arc Start Angle: %lf\n"), startAngle);
		add_tree_cstr_f(base_item, _T("Arc End Angle: %lf\n"), endAngle);
	}
	else if (pEntity->isKindOf(AcDbSolid::desc()))
	{
		AcDbSolid* pSolid = AcDbSolid::cast(pEntity);

		AcGePoint3d vertex1, vertex2, vertex3, vertex4;
		pSolid->getPointAt(0, vertex1);
		pSolid->getPointAt(1, vertex2);
		pSolid->getPointAt(2, vertex3);
		pSolid->getPointAt(3, vertex4);

		vertex1 = vertex1.transformBy(trans);
		vertex2 = vertex2.transformBy(trans);
		vertex3 = vertex3.transformBy(trans);
		vertex4 = vertex4.transformBy(trans);

		add_tree_cstr_f(base_item, _T("Solid Vertex 0: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 1: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 2: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
		add_tree_cstr_f(base_item, _T("Solid Vertex 3: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
	}
	else if (pEntity->isKindOf(AcDbPolyline::desc()))
	{
		AcDbPolyline* pPolyline = AcDbPolyline::cast(pEntity);
		int numVertices = pPolyline->numVerts();

		for (int i = 0; i < numVertices; ++i)
		{
			AcGePoint3d vertexPosition;
			pPolyline->getPointAt(i, vertexPosition);
			vertexPosition = vertexPosition.transformBy(trans);
			add_tree_cstr_f(base_item, _T("Polyline Vertex %d: (%lf, %lf, %lf)\n"), i, vertexPosition.x, vertexPosition.y, vertexPosition.z);
		}
	}
	else if (pEntity->isKindOf(AcDbSpline::desc()))
	{
		AcDbSpline* pSpline = AcDbSpline::cast(pEntity);
		int numControlPoints = pSpline->numControlPoints();

		for (int i = 0; i < numControlPoints; ++i)
		{
			AcGePoint3d controlPoint;
			pSpline->getControlPointAt(i, controlPoint);
			controlPoint = controlPoint.transformBy(trans);
			add_tree_cstr_f(base_item, _T("Spline Control Point %d: (%lf, %lf, %lf)\n"), i, controlPoint.x, controlPoint.y, controlPoint.z);
		}
	}
	else if (pEntity->isKindOf(AcDbFace::desc()))
	{
		AcDbFace* pFace = AcDbFace::cast(pEntity);

		AcGePoint3d vertex1, vertex2, vertex3, vertex4;
		pFace->getVertexAt(0, vertex1);
		pFace->getVertexAt(1, vertex2);
		pFace->getVertexAt(2, vertex3);
		pFace->getVertexAt(3, vertex4);

		vertex1 = vertex1.transformBy(trans);
		vertex2 = vertex2.transformBy(trans);
		vertex3 = vertex3.transformBy(trans);
		vertex4 = vertex4.transformBy(trans);

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Face Vertex 0: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 1: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 2: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 3: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
	}
	else if (pEntity->isKindOf(AcDbLine::desc()))
	{
		AcDbLine* pLine = AcDbLine::cast(pEntity);

		AcGeVector3d line_normal = pLine->normal();

		AcGePoint3d vertex_start = pLine->startPoint().transformBy(trans);
		AcGePoint3d vertex_end = pLine->endPoint().transformBy(trans);

		add_tree_cstr_f(base_item, _T("Line start point (WCS): (%lf, %lf, %lf)\n"), vertex_start.x, vertex_start.y, vertex_start.z);
		add_tree_cstr_f(base_item, _T("Line end point (WCS): (%lf, %lf, %lf)\n"), vertex_end.x, vertex_end.y, vertex_end.z);
		//add_tree_cstr_f(base_item, _T("Coordinate system pos (WCS): (%lf, %lf, %lf)\n"), coordinate_system.x, coordinate_system.y, coordinate_system.z);
	}
	else if (pEntity->isKindOf(AcDbPolygonMesh::desc()))
	{
		AcDbPolygonMesh* pMesh;
		acdbOpenObject(pMesh, pEntity->id(), AcDb::kForRead);
		int m_size = pMesh->mSize();
		int n_size = pMesh->nSize();
		AcDbObjectIterator* pVertIter = pMesh->vertexIterator();
		pMesh->close();

		AcDbPolygonMeshVertex* pVertex;
		AcGePoint3d pt;
		AcDbObjectId vertexObjId;

		for (int vertexNumber = 0; !pVertIter->done(); vertexNumber++, pVertIter->step())
		{
			vertexObjId = pVertIter->objectId();
			pMesh->openVertex(pVertex, vertexObjId, AcDb::kForRead);
			pt = pVertex->position().transformBy(trans);
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
	save_instruction = SaveXfMode::SELECTED_ENTITY;

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
	save_instruction = SaveXfMode::SELECTED_ENTITIES;

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

void CRmWindow::select_path_using_folder_picker()
{
	CFolderPickerDialog m_dlg;

	m_dlg.m_ofn.lpstrTitle = _T("Select a folder to save a file at...");

	if (m_dlg.DoModal() == IDOK) {
		path_from_mfc = std::wstring(m_dlg.GetPathName());   // Use this to get the selected folder name after the dialog has closed
		UpdateData(FALSE);			// To show updated folder in GUI
	}

	folder_path_entry.SetWindowTextW(path_from_mfc.c_str());
}

void CRmWindow::OnBnClickedSaveXf()
{
	select_path_using_folder_picker();
	SaveAsXf();
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
	CAcUiDialog::OnOK();
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
	CAcUiDialog::OnOK();
}

void CRmWindow::OnCancel()
{
	CAcUiDialog::OnOK();
}

void CRmWindow::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	// TODO: Add your control notification handler code here
	*pResult = 0;
}

