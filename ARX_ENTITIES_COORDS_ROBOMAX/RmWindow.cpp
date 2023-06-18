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

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC (CRmWindow, CAdUiBaseDialog)

BEGIN_MESSAGE_MAP(CRmWindow, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDOK, &CRmWindow::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, &CRmWindow::OnBnClickedButtonSelect)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CRmWindow::OnTvnSelchangedTree1)
    ON_BN_CLICKED(IDCANCEL, &CRmWindow::OnBnClickedCancel)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
CRmWindow::CRmWindow (CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAdUiBaseDialog (CRmWindow::IDD, pParent, hInstance) {}

//-----------------------------------------------------------------------------
void CRmWindow::DoDataExchange (CDataExchange *pDX) {
    CAdUiBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TREE1, m_treeCtrl);
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT CRmWindow::OnAcadKeepFocus (WPARAM, LPARAM) {
	return (TRUE) ;
}

const std::wstring CRmWindow::reduced_name(const AcDbEntity* ent) const
{
    std::wstring ent_name(ent->isA()->name());
    std::wstring ent_name_without_AcDb(ent_name.substr(4, ent_name.size()));
    return ent_name_without_AcDb;
}

void CRmWindow::insert_to_tree(AcDbEntity* pEntity, HTREEITEM base_item)
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
                        insert_to_tree(pNestedEntity, base_blockref_item);
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
		insert_coord_to_item(pEntity, base_for_coords);
    }
}

void CRmWindow::insert_coord_to_item(AcDbEntity* pEntity, HTREEITEM base_item)
{
	if (pEntity->isKindOf(AcDbCircle::desc()))
	{
		// Process Circle entity
		AcDbCircle* pCircle = AcDbCircle::cast(pEntity);
		AcGePoint3d center = pCircle->center();
		double radius = pCircle->radius();

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Circle Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
		add_tree_cstr_f(base_item, _T("Circle Radius: %lf\n"), radius);
	}
	else if (pEntity->isKindOf(AcDbArc::desc()))
	{
		// Process Arc entity
		AcDbArc* pArc = AcDbArc::cast(pEntity);
		AcGePoint3d center = pArc->center();
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

		// Print coordinates
		add_tree_cstr_f(base_item, _T("Face Vertex 0: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 1: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 2: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
		add_tree_cstr_f(base_item, _T("Face Vertex 3: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
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


void CRmWindow::OnBnClickedButtonSelect()
{
    if (!IsIconic())
    {
        ShowWindow(SW_MINIMIZE);
    }

    m_treeCtrl.DeleteAllItems();

    ads_point clicked_point;
    ads_name entity_name;

    if (acedEntSel(L"Select entity: ", entity_name, clicked_point) == RTNORM)
    {
        AcDbObjectId entityId;
        if (acdbGetObjectId(entityId, entity_name) == Acad::eOk)
        {
            AcDbEntity* pEntity = nullptr;
            if (acdbOpenObject(pEntity, entityId, AcDb::kForRead) == Acad::eOk)
            {
                insert_to_tree(pEntity);
#if defined(ROBOMAX_VERBOSE)
                acutPrintf(_T("Selected entity type: %s\n"), pEntity->isA()->name());
#endif // defined(ROBOMAX_VERBOSE)
                pEntity->close();
            }
        }
    }

    if (IsIconic())
    {
        ShowWindow(SW_RESTORE);
    }
}


void CRmWindow::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

    // TODO: Add your control notification handler code here
    *pResult = 0;
}


void CRmWindow::PostNcDestroy()
{
    delete this;
}


void CRmWindow::OnOk()
{
    DestroyWindow();
    CAdUiBaseDialog::OnOK();
}


void CRmWindow::OnCancel()
{
    DestroyWindow();
    CAdUiBaseDialog::OnCancel();
}


void CRmWindow::OnBnClickedCancel()
{
    OnCancel();
}


void CRmWindow::OnBnClickedOk()
{
    OnOk();
}

