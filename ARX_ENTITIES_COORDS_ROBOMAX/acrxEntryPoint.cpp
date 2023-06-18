// (C) Copyright 2002-2012 by Autodesk, Inc. 
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
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "RmWindow.h"

//-----------------------------------------------------------------------------
#define szRDS _RXST("")

#define ROBOMAX_VERBOSE
#define ROBOMAX_WRITE_TO_FILE

#if defined(ROBOMAX_VERBOSE)
static size_t entities_counter;
#endif // defined(ROBOMAX_VERBOSE)


namespace GLOBAL
{
	static const char* robomax_filename = "robomax_coordinates.txt";
	static std::ofstream output_file;
	static const std::size_t file_line_buffer_size = 1024;
	static ACHAR file_line_buffer[file_line_buffer_size];
}

//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CARX_ENTITIES_COORDS_ROBOMAXApp : public AcRxArxApp {
public:
	CARX_ENTITIES_COORDS_ROBOMAXApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// TODO: Load dependencies here

		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;
		
		// TODO: Add your initialization code here

		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		// TODO: Unload dependencies here

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}
	
	// The ACED_ARXCOMMAND_ENTRY_AUTO macro can be applied to any static member 
	// function of the CARX_ENTITIES_COORDS_ROBOMAXApp class.
	// The function should take no arguments and return nothing.
	//
	// NOTE: ACED_ARXCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid and
	// have arguments to define context and command mechanism.
	
	// ACED_ARXCOMMAND_ENTRY_AUTO(classname, group, globCmd, locCmd, cmdFlags, UIContext)
	// ACED_ARXCOMMAND_ENTRYBYID_AUTO(classname, group, globCmd, locCmdId, cmdFlags, UIContext)
	// only differs that it creates a localized name using a string in the resource file
	//   locCmdId - resource ID for localized command

	static int ads_select_entity_dialog()
	{
		CRmWindow* pDialog = nullptr;

		// For further portability
		if (pDialog == nullptr)
		{
			pDialog = new CRmWindow;
			if (!pDialog->Create(CRmWindow::IDD, acedGetAcadFrame()))
			{
				delete pDialog;	
				pDialog = nullptr;
				AfxMessageBox(L"Error while creating Dialog window %ld");
				return RTERROR;
			}
			else
			{
				if (!pDialog->IsWindowVisible())
				{
					pDialog->ShowWindow(SW_SHOW);
				}
				if (pDialog->IsIconic())
				{
					pDialog->ShowWindow(SW_RESTORE);
				}
			}
		}
		return RTNORM;
	}

	static std::string wstring_to_string(const std::wstring& wstr)
	{
		// Convert wide string to UTF-8 encoded multibyte string
		using type_ = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<type_> converter;
		return converter.to_bytes(wstr);
	}

	static void WriteToFile(const char* filepath, const std::string& text)
	{
		std::ofstream file(filepath, std::ofstream::app);
		if (file.is_open())
		{
			file << text;
		}
	}

	static void ROBOMAX_acutPrintf(const ACHAR* format, ...)
	{
		va_list args;
		va_start(args, format);

#if defined(ROBOMAX_WRITE_TO_FILE)
		_vsnwprintf(GLOBAL::file_line_buffer, GLOBAL::file_line_buffer_size, format, args);
		std::wstring wstring_buffer(GLOBAL::file_line_buffer);
		WriteToFile(GLOBAL::robomax_filename, wstring_to_string(wstring_buffer));
#endif // defined(ROBOMAX_WRITE_TO_FILE)

		acutPrintf(_T("%s"), wstring_buffer.c_str());

		va_end(args);
	}

	// REPLACE ACUTPRINTF TO CUSTOM THAT WRITES TO A FILE
#if defined(ROBOMAX_WRITE_TO_FILE)
#define acutPrintf ROBOMAX_acutPrintf
#endif // defined(ROBOMAX_WRITE_TO_FILE)

	static void GetEntityCoords(AcDbEntity* pEntity)
	{
		// TODO: refactor this code using link below 
		// https://help.autodesk.com/view/OARX/2023/ENU/?guid=OARX-RefGuide-AcDbObjectIterator

		if (pEntity->isKindOf(AcDbCircle::desc()))
		{
			// Process Circle entity
			AcDbCircle* pCircle = AcDbCircle::cast(pEntity);
			AcGePoint3d center = pCircle->center();
			double radius = pCircle->radius();

			// Print coordinates
			acutPrintf(_T("|Circle Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
			acutPrintf(_T("|Circle Radius: %lf\n"), radius);
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
			acutPrintf(_T("|Arc Center: (%lf, %lf, %lf)\n"), center.x, center.y, center.z);
			acutPrintf(_T("|Arc Radius: %lf\n"), radius);
			acutPrintf(_T("|Arc Start Angle: %lf\n"), startAngle);
			acutPrintf(_T("|Arc End Angle: %lf\n"), endAngle);
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
			acutPrintf(_T("|Solid Vertex 1: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
			acutPrintf(_T("|Solid Vertex 2: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
			acutPrintf(_T("|Solid Vertex 3: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
			acutPrintf(_T("|Solid Vertex 4: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
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
				acutPrintf(_T("|Polyline Vertex %d: (%lf, %lf, %lf)\n"), i, vertexPosition.x, vertexPosition.y, vertexPosition.z);
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
				acutPrintf(_T("|Spline Control Point %d: (%lf, %lf, %lf)\n"), i, controlPoint.x, controlPoint.y, controlPoint.z);
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
			acutPrintf(_T("|Face Vertex 1: (%lf, %lf, %lf)\n"), vertex1.x, vertex1.y, vertex1.z);
			acutPrintf(_T("|Face Vertex 2: (%lf, %lf, %lf)\n"), vertex2.x, vertex2.y, vertex2.z);
			acutPrintf(_T("|Face Vertex 3: (%lf, %lf, %lf)\n"), vertex3.x, vertex3.y, vertex3.z);
			acutPrintf(_T("|Face Vertex 4: (%lf, %lf, %lf)\n"), vertex4.x, vertex4.y, vertex4.z);
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
				acutPrintf(_T("|PolygonMesh Vertex %d: (%lf, %lf, %lf)\n"), vertexNumber, pt[X], pt[Y], pt[Z]);
			}
			delete pVertIter;
		}
		else
		{
			acutPrintf(_T("|Entity type is not recognized: %s\n"), pEntity->isA()->name());
		}
		// Add more conditions for other entity types as needed
	}

	static void ProcessEntity(AcDbEntity* pEntity)
	{
#if defined(ROBOMAX_VERBOSE)
		acutPrintf(_T("|*ProcessEntity is called.\n"));
#endif // defined(ROBOMAX_VERBOSE)

		// Check if the entity is an AcDbBlockReference
		AcDbBlockReference* pBlockRef = AcDbBlockReference::cast(pEntity);
		if (pBlockRef)
		{
			// Traverse the nested entities within the block reference
			AcDbObjectId blockId = pBlockRef->blockTableRecord();
			AcDbBlockTableRecord* pBlock;
			if (acdbOpenObject(pBlock, blockId, AcDb::kForRead) == Acad::eOk)
			{
				AcDbBlockTableRecordIterator* pIterator;
				if (pBlock->newIterator(pIterator) == Acad::eOk)
				{
					for (; !pIterator->done(); pIterator->step())
					{
						AcDbEntity* pNestedEntity;
						if (pIterator->getEntity(pNestedEntity, AcDb::kForRead) == Acad::eOk)
						{
							ProcessEntity(pNestedEntity);
							pNestedEntity->close();
						}
					}
					delete pIterator;
				}
				pBlock->close();
			}
		}
		else
		{
			GetEntityCoords(pEntity);
#if defined(ROBOMAX_VERBOSE)
			acutPrintf(_T("\n+---ENTITY ID %x INFO---+\n"), pEntity->objectId().asOldId());
			acutPrintf(_T("|Type: %s\n"), pEntity->isA()->name());
			// Example: Get the entity's layer name
			AcDbObjectId layerId = pEntity->layerId();
			AcDbLayerTableRecord* pLayer;
			if (acdbOpenObject(pLayer, layerId, AcDb::kForRead) == Acad::eOk)
			{
				ACHAR* layerName;
				if (pLayer->getName(layerName) == Acad::eOk)
				{
					const ACHAR* layerName;
					pLayer->getName(layerName);
					acutPrintf(_T("|Layer Name: %s\n"), layerName);
					pLayer->close();
				}

				pLayer->close();
			}
			entities_counter++;
#endif // defined(ROBOMAX_VERBOSE)
		}
	}

	static void ProcessEntities()
	{
#if defined(ROBOMAX_VERBOSE)
		acutPrintf(_T("|*ProcessEntities is called.\n"));
#endif // defined(ROBOMAX_VERBOSE)

		AcDbBlockTable* pBlockTable;
		if (acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlockTable, AcDb::kForRead) == Acad::eOk)
		{
			AcDbBlockTableRecord* pBlock;
			AcDbBlockTableIterator* pIterator;
			for (pBlockTable->newIterator(pIterator); !pIterator->done(); pIterator->step())
			{
				if (pIterator->getRecord(pBlock, AcDb::kForRead) == Acad::eOk)
				{
					AcDbBlockTableRecordIterator* pEntityIterator;
					if (pBlock->newIterator(pEntityIterator) == Acad::eOk)
					{
						for (; !pEntityIterator->done(); pEntityIterator->step())
						{
							AcDbEntity* pEntity;
							if (pEntityIterator->getEntity(pEntity, AcDb::kForRead) == Acad::eOk)
							{
								ProcessEntity(pEntity);
								pEntity->close();
							}
						}
						delete pEntityIterator;
					}
					pBlock->close();
				}
			}
			delete pIterator;
			pBlockTable->close();
		}
	}

	// Modal Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL)
	static void MyGroupMyCommand () 
	{
#if defined(ROBOMAX_WRITE_TO_FILE)
		GLOBAL::output_file.open(GLOBAL::robomax_filename, std::ofstream::out | std::ofstream::trunc);
#endif // defined(ROBOMAX_WRITE_TO_FILE)

#if defined(ROBOMAX_VERBOSE)
		acutPrintf(_T("*MyGroupMyCommand is called.\n"));
#endif // defined(ROBOMAX_VERBOSE)

		ads_select_entity_dialog();

		// MAIN PROCESSING FUNCTION
		ProcessEntities();

#if defined(ROBOMAX_VERBOSE)
		acutPrintf(_T("+-----------------------+\n"));
		acutPrintf(_T(">>> Entities amount: %d\n"), entities_counter);
		entities_counter = 0;
#endif // defined(ROBOMAX_VERBOSE)

#if defined(ROBOMAX_WRITE_TO_FILE)
		GLOBAL::output_file.close();
#endif // defined(ROBOMAX_WRITE_TO_FILE)
	}

	// Modal Command with pickfirst selection
	// ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET)
	static void MyGroupMyPickFirst () {
		ads_name result ;
		int iRet =acedSSGet (ACRX_T("_I"), NULL, NULL, NULL, result) ;
		if ( iRet == RTNORM )
		{
			// There are selected entities
			// Put your command using pickfirst set code here
		}
		else
		{
			// There are no selected entities
			// Put your command code here
		}
	}

	// Application Session Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION)
	static void MyGroupMySessionCmd () {
		// Put your command code here
	}

	// The ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO macros can be applied to any static member 
	// function of the CARX_ENTITIES_COORDS_ROBOMAXApp class.
	// The function may or may not take arguments and have to return RTNORM, RTERROR, RTCAN, RTFAIL, RTREJ to AutoCAD, but use
	// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal to return
	// a value to the Lisp interpreter.
	//
	// NOTE: ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid.
	
	//- ACED_ADSFUNCTION_ENTRY_AUTO(classname, name, regFunc) - this example
	//- ACED_ADSSYMBOL_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file
	//- ACED_ADSCOMMAND_ENTRY_AUTO(classname, name, regFunc) - a Lisp command (prefix C:)
	//- ACED_ADSCOMMAND_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file

	// Lisp Function is similar to ARX Command but it creates a lisp 
	// callable function. Many return types are supported not just string
	// or integer.
	// ACED_ADSFUNCTION_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyLispFunction, false)
	static int ads_MyLispFunction () {
		//struct resbuf *args =acedGetArgs () ;
		
		// Put your command code here

		//acutRelRb (args) ;
		
		// Return a value to the AutoCAD Lisp Interpreter
		// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal

		return (RTNORM) ;
	}
	
} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CARX_ENTITIES_COORDS_ROBOMAXApp)

ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyCommand, robomax, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION, NULL)
ACED_ADSSYMBOL_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyLispFunction, false)

