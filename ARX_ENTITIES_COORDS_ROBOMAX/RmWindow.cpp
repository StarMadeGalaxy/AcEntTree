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
		{ AcDbLine::desc(), L"SLINE.xf" },
		{ AcDbSpline::desc(), L"SLINE.xf" },
		{ AcDbSolid::desc(), L"SOLID.xf" },
		{ AcDbBlockReference::desc(), L"INSERT"},
		{ AcDbSubDMesh::desc(), L"POLY.xf"},
		{ AcDbPolyline::desc(), L"POLY.xf" },
		{ AcDb2dPolyline::desc(), L"POLY.xf"},
		{ AcDb3dPolyline::desc(), L"POLY.xf"},
		{ AcDbPolygonMesh::desc(), L"POLY.xf"},	
		{ AcDb3dSolid::desc(), L"POLY.xf"},
		{ nullptr, L".xf" }
	};

	objs_counters = {
		{ AcDbCircle::desc(), 1},
		{ AcDbFace::desc(), 1},
		{ AcDbArc::desc(), 1},
		{ AcDbLine::desc(), 1},
		{ AcDbSpline::desc(), 1},
		{ AcDbSolid::desc(), 1},
		{ AcDbBlockReference::desc(), 1},
		{ AcDbSubDMesh::desc(), 1},
		{ AcDbPolyline::desc(), 1},
		{ AcDb2dPolyline::desc(), 1},
		{ AcDbPolygonMesh::desc(), 1},
		{ AcDb3dPolyline::desc(), 1},
		{ AcDb3dSolid::desc(), 1}
	};

	global_obj_mesh_counter = 1;
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

void CRmWindow::block_matrix_acutPrintf(AcDbBlockReference* block)
{
	AcString block_name = AcString();
	AcDbObjectId blockId = block->blockTableRecord();
	AcDbBlockTableRecord* pBlockTR = nullptr;
	if (acdbOpenObject(pBlockTR, blockId, AcDb::kForRead) == Acad::eOk)
		pBlockTR->getName(block_name);

	acutPrintf(_T("\nTrasformation matrix of %s block.\n"), block_name.kACharPtr());

	AcGeMatrix3d matrix = block->blockTransform();

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			acutPrintf(_T("%f "), matrix(i, j));
		}
		acutPrintf(_T("\n"));
	}
}

const std::wstring CRmWindow::reduced_name(const AcDbEntity* ent) const
{
	std::size_t len_AcDb = 4;
	std::wstring ent_name(ent->isA()->name());
	std::wstring ent_name_without_AcDb(ent_name.substr(len_AcDb, ent_name.size()));
	return ent_name_without_AcDb;
}

std::string CRmWindow::formatDouble(double value) {
	std::stringstream stream;
	stream << std::fixed << std::setprecision(8) << value;
	std::string formattedValue = stream.str();
	if (std::abs(value) < 0.00000001) {
		return ".00000000";
	}
	else
		return formattedValue;
}

void CRmWindow::reset_counters()
{
	for (auto& pair : objs_counters) {
		pair.second = 1;
	}

	global_obj_mesh_counter = 1;
}

void CRmWindow::mesh_obj(AcDbEntity* pEntity, const AcGeMatrix3d& trans)
{
	if (pEntity->isKindOf(AcDbCircle::desc()))
	{
		circle_meshing(pEntity, 16, trans);
	}
	else if (pEntity->isKindOf(AcDbArc::desc()))
	{
		;
	}
	else if (pEntity->isKindOf(AcDbPolyline::desc()))
	{
		polyline_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDb2dPolyline::desc()))
	{
		polyline2d_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDb3dPolyline::desc()))
	{
		polyline3d_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDb3dSolid::desc()))
	{
		solid3d_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDbSolid::desc()))
	{
		solid_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDbFace::desc()))
	{
		face_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDbLine::desc()))
	{
		line_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDbSubDMesh::desc()));	// AcDbSubDMesh::desc() requires linking against AcGeomEnt.lib
	else if (pEntity->isKindOf(AcDbPolygonMesh::desc()))
	{
		polygonmesh_meshing(pEntity, trans);
	}
	else if (pEntity->isKindOf(AcDbBlockReference::desc()))
	{
	
	}
	else
	{
		return;
	}
	global_obj_mesh_counter++;
}

void CRmWindow::write_obj_data_to_xf_file(AcDbEntity* pEntity, const AcGeMatrix3d& trans)
{
	// This function is supposed to be called as many times as entities we have
	// 
	// we ignore the entity on the layer called 'auxillary'
	{
		AcString layer_name = AcString();
		AcDbObjectId layerId = pEntity->layerId();
		AcDbLayerTableRecord* pLayerRecord = nullptr;

		acdbOpenObject(pLayerRecord, layerId, AcDb::kForRead);
		pLayerRecord->getName(layer_name);
		pLayerRecord->close();

		for (unsigned int i = 0; i < layer_name.length(); i++)
			layer_name.setAt(i, static_cast<ACHAR>(std::tolower(layer_name[i])));

		if (layer_name == _T("auxiliary"))
			return;
	}

	std::wstring obj_file_name = path_from_mfc + L'\\' + objs_xf_filenames[pEntity->isA()];

	if (pEntity->isKindOf(AcDbBlockReference::desc()))
	{
		obj_file_name += std::to_wstring(objs_counters[pEntity->desc()]++) +
			objs_xf_filenames[nullptr];
	}

	std::ofstream file(obj_file_name, std::ios::app);

	if (file.is_open())
	{
		if (pEntity->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle* pCircle = AcDbCircle::cast(pEntity);

			double radius = pCircle->radius();
			double thickness = pCircle->thickness();
			AcGeVector3d circ_normal = pCircle->normal().transformBy(trans);
			AcGePoint3d center = pCircle->center().transformBy(trans);

			file << objs_counters[pEntity->desc()] << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(thickness) << '\n'
				<< formatDouble(center.x) << '\n' << formatDouble(center.y) << '\n' << formatDouble(center.z) << '\n'
				<< formatDouble(radius) << '\n'
				<< formatDouble(circ_normal.x) << '\n' << formatDouble(circ_normal.y) << '\n' << formatDouble(circ_normal.z) << '\n';

			objs_counters[pEntity->desc()]++;
		}
		else if (pEntity->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = AcDbArc::cast(pEntity);

			AcGePoint3d center = pArc->center().transformBy(trans);

			double radius = pArc->radius();
			double startAngle = pArc->startAngle();
			double endAngle = pArc->endAngle();

			file << objs_counters[pEntity->desc()] << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(center.x) << '\n' << formatDouble(center.y) << '\n' << formatDouble(center.z) << '\n'
				<< formatDouble(radius) << '\n'
				<< formatDouble(startAngle) << '\n' << formatDouble(endAngle) << '\n';

			objs_counters[pEntity->desc()]++;
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

			file << objs_counters[pEntity->desc()] << '\n' << std::fixed << std::setprecision(8)
				<< formatDouble(vertex1.x) << '\n' << formatDouble(vertex1.y) << '\n' << formatDouble(vertex1.z) << '\n'
				<< formatDouble(vertex2.x) << '\n' << formatDouble(vertex2.y) << '\n' << formatDouble(vertex2.z) << '\n'
				<< formatDouble(vertex3.x) << '\n' << formatDouble(vertex3.y) << '\n' << formatDouble(vertex3.z) << '\n'
				<< formatDouble(vertex4.x) << '\n' << formatDouble(vertex4.y) << '\n' << formatDouble(vertex4.z) << '\n';

			objs_counters[pEntity->desc()]++;
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

			file << objs_counters[pEntity->desc()] << std::fixed << std::setprecision(8) << '\n'
				<< formatDouble(vertex1.x) << '\n' << formatDouble(vertex1.y) << '\n' << formatDouble(vertex1.z) << '\n'
				<< formatDouble(vertex2.x) << '\n' << formatDouble(vertex2.y) << '\n' << formatDouble(vertex2.z) << '\n'
				<< formatDouble(vertex3.x) << '\n' << formatDouble(vertex3.y) << '\n' << formatDouble(vertex3.z) << '\n'
				<< formatDouble(vertex4.x) << '\n' << formatDouble(vertex4.y) << '\n' << formatDouble(vertex4.z) << '\n';

			objs_counters[pEntity->desc()]++;
		}
		else if (pEntity->isKindOf(AcDbLine::desc()))
		{
			AcDbLine* pLine = AcDbLine::cast(pEntity);

			AcGeVector3d line_normal = pLine->normal().transformBy(trans);
			AcGePoint3d vertex_start = pLine->startPoint().transformBy(trans);
			AcGePoint3d vertex_end = pLine->endPoint().transformBy(trans);

			file << objs_counters[pEntity->desc()] << '\n' << std::fixed << std::setprecision(8) << formatDouble(pLine->thickness()) << '\n'
				<< formatDouble(vertex_start.x) << '\n' << formatDouble(vertex_start.y) << '\n' << formatDouble(vertex_start.z) << '\n'
				<< formatDouble(vertex_end.x) << '\n' << formatDouble(vertex_end.y) << '\n' << formatDouble(vertex_end.z) << '\n'
				<< formatDouble(line_normal.x) << '\n' << formatDouble(line_normal.y) << '\n' << formatDouble(line_normal.z) << '\n';

			objs_counters[pEntity->desc()]++;
		}
		else if (pEntity->isKindOf(AcDbPolygonMesh::desc()))
		{
			AcDbPolygonMesh* pMesh;

			acdbOpenObject(pMesh, pEntity->id(), AcDb::kForRead);

			int m_size = pMesh->mSize();
			int n_size = pMesh->nSize();
			AcDbObjectIterator* pVertIter = pMesh->vertexIterator();
			pMesh->close();

			file << objs_counters[pEntity->desc()] << '\n' << m_size << "     " << n_size << '\n';

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

			objs_counters[pEntity->desc()]++;
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

				// convert from AcString to std::string  
				// we're doing all this to write the block's name properly into the file.
				// For some reason if I'm not doing this it writes the address of the pointer to the file. TF!!
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::string block_name_str(converter.to_bytes(block_name.kACharPtr()));

				file << block_name_str << '\n'
					<< 10 << '\n' << std::fixed << std::setprecision(1) << block_pos.x << '\n'
					<< 20 << '\n' << block_pos.y << '\n'
					<< 30 << '\n' << block_pos.z << '\n' << "END\n";

				AcDbBlockTableRecordIterator* pIterator;
				if (pBlockTR->newIterator(pIterator) == Acad::eOk)
				{
					for (; !pIterator->done(); pIterator->step())
					{
						AcDbEntity* pNestedEntity;
						if (pIterator->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
						{
							block_matrix_acutPrintf(pBlockRef); // this function is called twice for each block for some reason
							AcGeMatrix3d btrans = trans * pBlockRef->blockTransform();
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

	mesh_obj(pEntity, trans);	// main meshing function
}

void CRmWindow::solid_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDbSolid* pSolid = AcDbSolid::cast(entity);

	double thickness = pSolid->thickness();

	AcGePoint3d vertices[4];
	int numVertices = 4;
	AcGeVector3d normal = pSolid->normal();

	for (int i = 0; i < 4; ++i) 
		pSolid->getPointAt(i, vertices[i]);

	if (vertices[2].isEqualTo(vertices[3])) 
		numVertices = 3;

	std::vector<AcGePoint3d> allVertices;

	for (int i = 0; i < numVertices; ++i) {
		// vertices[i].transformBy(trans);		// apparently AcDbSolid::getPointAt() returns coordinates in WCS
		allVertices.push_back(vertices[i]);
	}

	for (int i = 0; i < numVertices; ++i) {
		AcGePoint3d extrudedVertex = vertices[i] + normal * thickness;
		//extrudedVertex.transformBy(trans);
		allVertices.push_back(extrudedVertex);
	}

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	std::size_t numTotalVertices = allVertices.size();
	file << global_obj_mesh_counter << '\n' << numTotalVertices << "    " << 1 << '\n'; // Number of vertices and one solid

	for (const auto& vertex : allVertices) {
		file << formatDouble(vertex.x) << '\n'
			<< formatDouble(vertex.y) << '\n'
			<< formatDouble(vertex.z) << '\n';
	}
	file.close();
}

void CRmWindow::face_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDbFace* pFace = AcDbFace::cast(entity);

	AcGePoint3d vertices[4];
	Adesk::Boolean edgeVisibilities[4];

	int vertices_number = 0;

	for (int i = 0; i < 4; ++i) 
	{
		if (pFace->getVertexAt(i, vertices[i]) == Acad::eOk)
			vertices_number++;
		else
			break;
		pFace->isEdgeVisibleAt(i, edgeVisibilities[i]);
	}

	for (int i = 0; i < 4; ++i) 
		vertices[i].transformBy(trans);

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	file << global_obj_mesh_counter << '\n' << vertices_number << "    " << 1 << '\n'; 

	for (int i = 0; i < 4; ++i) {
		file << formatDouble(vertices[i].x) << '\n'
			<< formatDouble(vertices[i].y) << '\n'
			<< formatDouble(vertices[i].z) << '\n';
	}

	file.close();
}

void CRmWindow::polyline_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDbPolyline* pPolyline = AcDbPolyline::cast(entity);
	unsigned int numVertices = pPolyline->numVerts();

	if (pPolyline->thickness() <= 0)
		return;

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	std::size_t DUMMY = 0;
	file << global_obj_mesh_counter << '\n' << DUMMY << "    " << DUMMY << '\n';

	for (unsigned int i = 0; i < numVertices; ++i)
	{
		AcGePoint3d vertexPosition;
		pPolyline->getPointAt(i, vertexPosition);

		vertexPosition.transformBy(trans);

		file << formatDouble(vertexPosition.x) << '\n'
			<< formatDouble(vertexPosition.y) << '\n'
			<< formatDouble(vertexPosition.z) << '\n';
	}
	file.close();
}

void CRmWindow::solid3d_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)	// complicated object
{
	AcDb3dSolid* pSolid = AcDb3dSolid::cast(entity);
}

void CRmWindow::polyline2d_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDb2dPolyline* pPolyline = AcDb2dPolyline::cast(entity);

	if (pPolyline->thickness() <= 0)
		return;

	AcDbObjectIterator* pVertexIterator = pPolyline->vertexIterator();
	if (pVertexIterator == nullptr) {
		acutPrintf(L"Failed to retrieve vertex iterator.\n");
		return;
	}

	// Open file for writing
	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	std::size_t DUMMY = 0;
	file << global_obj_mesh_counter << '\n' << DUMMY << "    " << DUMMY << '\n';

	AcGePoint3d vertexPoint;
	while (!pVertexIterator->done())
	{
		AcDbObjectId vertexId = pVertexIterator->objectId();
		AcDbVertex* pVertex;
		if (acdbOpenAcDbEntity((AcDbEntity*&)pVertex, vertexId, AcDb::kForRead) == Acad::eOk)
		{
			if (pVertex->isKindOf(AcDb2dVertex::desc()))
			{
				AcDb2dVertex* p2dVertex = AcDb2dVertex::cast(pVertex);
				if (p2dVertex != nullptr)
				{
					vertexPoint = p2dVertex->position();

					vertexPoint.transformBy(trans);

					file << formatDouble(vertexPoint.x) << '\n'
						<< formatDouble(vertexPoint.y) << '\n'
						<< ".0000000\n";  // Z coordinate is 0 for 2D polyline

					p2dVertex->close();
				}
			}
			pVertex->close();
		}
		pVertexIterator->step();
	}

	delete pVertexIterator;
	file.close();

}

void CRmWindow::polyline3d_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDb3dPolyline* pPolyline = AcDb3dPolyline::cast(entity);

	if (pPolyline == nullptr) {
		acutPrintf(L"Invalid 3D polyline entity.\n");
		return;
	}

	AcDbObjectIterator* pVertexIterator = pPolyline->vertexIterator();
	if (pVertexIterator == nullptr) {
		acutPrintf(L"Failed to retrieve vertex iterator.\n");
		return;
	}

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}
	std::size_t DUMMY = 0;	// IDK WHAT SHOULD BE IN THE HEADER
	file << global_obj_mesh_counter << '\n' << DUMMY << "    " << DUMMY << '\n';

	while (!pVertexIterator->done())
	{
		AcDbObjectId vertexId = pVertexIterator->objectId();
		AcDb3dPolylineVertex* pVertex;
		if (acdbOpenAcDbEntity((AcDbEntity*&)pVertex, vertexId, AcDb::kForRead) == Acad::eOk)
		{
			AcGePoint3d vertexPoint = pVertex->position();

			vertexPoint.transformBy(trans);

			file << formatDouble(vertexPoint.x) << '\n'
				<< formatDouble(vertexPoint.y) << '\n'
				<< formatDouble(vertexPoint.z) << '\n';

			pVertex->close();
		}
		pVertexIterator->step();
	}

	delete pVertexIterator;
	file.close();
}

void CRmWindow::polygonmesh_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDbPolygonMesh* pMesh = AcDbPolygonMesh::cast(entity);

	int m_size = pMesh->mSize();
	int n_size = pMesh->nSize();

	AcDbObjectIterator* pVertIter = pMesh->vertexIterator();

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	file << global_obj_mesh_counter << '\n' << m_size << "    " << n_size << '\n';

	for (int vertexNumber = 0; !pVertIter->done(); vertexNumber++, pVertIter->step())
	{
		AcDbPolygonMeshVertex* vertex;
		AcDbObjectId vertexObjId = pVertIter->objectId();
		pMesh->openVertex(vertex, vertexObjId, AcDb::kForRead);

		AcGePoint3d point = vertex->position();
		point.transformBy(trans);

		file << formatDouble(point.x) << '\n'
			<< formatDouble(point.y) << '\n'
			<< formatDouble(point.z) << '\n';

		vertex->close();
	}
	delete pVertIter;
	file.close();
}

void CRmWindow::line_meshing(AcDbEntity* entity, const AcGeMatrix3d& trans)
{
	AcDbLine* pLine = AcDbLine::cast(entity);

	double thickness = pLine->thickness();

	if (thickness <= 0)
		return;

	AcGeVector3d normal = pLine->normal();

	AcGePoint3d vertex_start = pLine->startPoint();
	AcGePoint3d vertex_end = pLine->endPoint();

	AcGePoint3d vertex_start_thick = pLine->startPoint() + normal * thickness;
	AcGePoint3d vertex_end_thick = pLine->endPoint() + normal * thickness;
		
	std::vector<AcGePoint3d> face = {
		vertex_start, vertex_end, vertex_start_thick, vertex_end_thick
	};

	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	std::size_t vertices_number = 2;
	file << global_obj_mesh_counter << '\n' << vertices_number << "    " << vertices_number << '\n';

	for (auto& vertex : face)
	{
		vertex.transformBy(trans);
		file << formatDouble(vertex.x) << '\n'
			<< formatDouble(vertex.y) << '\n'
			<< formatDouble(vertex.z) << '\n';
	}
}

void CRmWindow::circle_meshing(AcDbEntity* entity, std::size_t N, const AcGeMatrix3d& trans)
{
	AcDbCircle* pCircle = AcDbCircle::cast(entity);
	if (pCircle == nullptr) 
	{
		acutPrintf(L"Entity is not a circle.\n");
		return;
	}

	double thickness = pCircle->thickness();
	AcGeVector3d normal = pCircle->normal();
	AcGePoint3d center = pCircle->center();
	double radius = pCircle->radius();

	AcGePoint3d bottomCenter = center;
	AcGePoint3d topCenter = center + normal * thickness;

	AcGeVector3d xAxis, yAxis, zAxis = normal;

	if (std::fabs(zAxis.dotProduct(AcGeVector3d::kZAxis)) < 0.99) {
		xAxis = AcGeVector3d::kZAxis.crossProduct(zAxis).normal();
	}
	else {
		xAxis = AcGeVector3d::kYAxis.crossProduct(zAxis).normal();
	}
	yAxis = zAxis.crossProduct(xAxis).normal();

	std::vector<AcGePoint3d> topPoints, bottomPoints;

	// -1 because the first and the last point of the circle lay on each other,
	// hence you only see 15 points technically, but actually there's 15
	// double angleStep = 2 * M_PI / (N - 1);
	double angleStep = 2 * M_PI / (N - 1);

	for (std::size_t i = 0; i < N; ++i) 
	{
		double angle = i * angleStep;
		double x = radius * cos(angle);
		double y = radius * sin(angle);

		AcGePoint3d localPoint(x, y, 0.0);
		AcGePoint3d topPoint = topCenter + xAxis * localPoint.x + yAxis * localPoint.y;
		AcGePoint3d bottomPoint = bottomCenter + xAxis * localPoint.x + yAxis * localPoint.y;

		// Transform points to the global coordinate system
		topPoint.transformBy(trans);
		bottomPoint.transformBy(trans);

		topPoints.push_back(topPoint);
		bottomPoints.push_back(bottomPoint);
	}

	// Open file for writing
	std::ofstream file(path_from_mfc + L'\\' + mesh_file_str, std::ios::app);
	if (!file.is_open()) {
		acutPrintf(L"Failed to open file for writing.\n");
		return;
	}

	std::size_t vertices_number = 4;
	file << global_obj_mesh_counter << '\n' << vertices_number << "    " << N << '\n';

	// Transform points to the global coordinate system
	bottomCenter.transformBy(trans);
	topCenter.transformBy(trans);


	// Write bottom center points
	for (std::size_t i = 0; i < N; ++i) {
		file << formatDouble(bottomCenter.x) << '\n'
			<< formatDouble(bottomCenter.y) << '\n'
			<< formatDouble(bottomCenter.z) << '\n';
	}

	// Write top circle points
	for (const auto& point : topPoints) {
		file << formatDouble(point.x) << "\n"
			<< formatDouble(point.y) << "\n"
			<< formatDouble(point.z) << "\n";
	}

	// Write bottom circle points
	for (const auto& point : bottomPoints) {
		file << formatDouble(point.x) << "\n"
			<< formatDouble(point.y) << "\n"
			<< formatDouble(point.z) << "\n";
	}

	// Write top center points
	for (std::size_t i = 0; i < N; ++i) {
		file << formatDouble(topCenter.x) << '\n'
			<< formatDouble(topCenter.y) << '\n'
			<< formatDouble(topCenter.z) << '\n';
	}

	// Close the file
	file.close();
}

void CRmWindow::insert_to_tree(AcDbEntity* pEntity, const AcGeMatrix3d& trans, HTREEITEM base_item)
{
	AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
	std::wstring rname = reduced_name(pEntity);


	if (pEntity->isKindOf(AcDbBlockReference::desc()))
	{ 
		block_matrix_acutPrintf(pBlockRef);
		HTREEITEM base_blockref_item = m_treeCtrl.InsertItem(rname.c_str(), base_item);

		AcDbObjectId blockId = pBlockRef->blockTableRecord();
		AcDbBlockTableRecord* pBlockTR;
		if (acdbOpenObject(pBlockTR, blockId, AcDb::kForRead) == Acad::eOk)
		{
			AcDbBlockTableRecordIterator* it;
			if (pBlockTR->newIterator(it) == Acad::eOk)
			{
				AcGeMatrix3d transSub = trans * pBlockRef->blockTransform();
				for (it->start(); !it->done(); it->step())
				{
					AcDbEntity* pNestedEntity;
					if (it->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
					{
						insert_to_tree(pNestedEntity, trans * transSub, base_blockref_item);
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
				// if we don't select any entites and click save as xf, that means that we 
				// save all of the this in .DWG file hence we need to show them in the tree
				// as well.
				//insert_to_tree(pEntity); 
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

	// reset all of the counters
	reset_counters();
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

