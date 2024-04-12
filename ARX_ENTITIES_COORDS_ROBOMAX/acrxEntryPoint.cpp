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


	static INT_PTR ads_select_entity_dialog()
	{
		std::unique_ptr<CRmWindow> dlg = std::make_unique<CRmWindow>(CWnd::FromHandle(adsw_acadMainWnd()));

		if (IDOK != dlg->DoModal())
		{
			PostQuitMessage(WM_DESTROY);
			return FALSE;
		}
		return TRUE;
	}

	// Modal Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL)
	static void MyGroupMyCommand () 
	{
		ads_select_entity_dialog();
	}


} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CARX_ENTITIES_COORDS_ROBOMAXApp)

ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyCommand, robomax, ACRX_CMD_MODAL, NULL)
//ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
//ACED_ARXCOMMAND_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION, NULL)
//ACED_ADSSYMBOL_ENTRY_AUTO(CARX_ENTITIES_COORDS_ROBOMAXApp, MyLispFunction, false)

