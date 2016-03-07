//****************************************************************************
// Fichier: GridHelp.cpp
// Classe:  CGridHelp 
// Usage:
//	
//	To use CGridHelp. There are 3 steps:
//		1) initialization of the DLL with the method : Initialize		
//		2) Open a grid and used it :
//				Open
//				GetValue
//				GetNbCols
//				GetNbRows
//		3) Close de grid: Close
//
//****************************************************************************
// 03-04-2006  Rémi Saint-Amant  Initial version 
//****************************************************************************

#include "stdafx.h"
#include <crtdbg.h>
#include <windows.h>

#include "Basic/UtilStd.h"
#include "ModelBase/GridHelp.h"


namespace WBSF
{

	//****************************************************************************
	// Summary:    Constructor
	//
	// Description: To Create and initialize an object
	//
	// Input:      
	//
	// Output:
	//
	// Note:
	//****************************************************************************
	CGridHelp::CGridHelp()
	{
		m_hDll = NULL;
		m_OpenFunction = NULL;
		m_CloseFunction = NULL;
		m_GetNbColsFunction = NULL;
		m_GetNbRowsFunction = NULL;
		m_GetValueFunction = NULL;
		m_GetValueLatLonFunction = NULL;

	}


	//****************************************************************************
	// Summary:     Destructor
	//
	// Description:  Destroy and clean memory
	//
	// Input:      
	//
	// Output:
	//
	// Note:
	//****************************************************************************
	CGridHelp::~CGridHelp()
	{
		if (m_hDll)
		{
			FreeLibrary((HMODULE)m_hDll);
			m_hDll = NULL;
		}

		m_OpenFunction = NULL;
		m_GetNbColsFunction = NULL;
		m_GetNbRowsFunction = NULL;
		m_GetValueFunction = NULL;
		m_GetValueLatLonFunction = NULL;
		m_CloseFunction = NULL;
	}

	//****************************************************************************
	// Summary:     initialize the class 
	//
	// Description:  initialize the dll with file path
	//
	// Input:      DLLFilePath : the file path of the dll (tempGenLib.dll)
	//
	// Output:	   ERMsg : error message
	//
	// Note:
	//****************************************************************************
	ERMsg CGridHelp::Initialize(const char* DLLFilePath)
	{
		ERMsg message;

		if (m_hDll)
		{
			//if a dll is loaded : free
			FreeLibrary((HMODULE)m_hDll);
			m_hDll = NULL;
		}

		//load dll
		m_hDll = LoadLibraryW(convert(DLLFilePath).c_str());

		if (m_hDll != NULL)
		{
			//if loaded : load function
			m_OpenFunction = (OpenFunction)GetProcAddress((HMODULE)m_hDll, "Open");
			m_CloseFunction = (CloseFunction)GetProcAddress((HMODULE)m_hDll, "Close");
			m_GetNbColsFunction = (GetNbColsFunction)GetProcAddress((HMODULE)m_hDll, "GetNbCols");
			m_GetNbRowsFunction = (GetNbRowsFunction)GetProcAddress((HMODULE)m_hDll, "GetNbRows");
			m_GetValueFunction = (GetValueFunction)GetProcAddress((HMODULE)m_hDll, "GetValue");
			m_GetValueLatLonFunction = (GetValueLatLonFunction)GetProcAddress((HMODULE)m_hDll, "GetValueLatLon");

		}
		else
		{
			message.asgType(ERMsg::ERREUR);
			message.ajoute("error");
		}

		return message;
	}

	//****************************************************************************
	// Summary:     Open a grid
	//
	// Description:  Open a grid for reading
	//
	// Input:      filePath : the file path of the grid
	//
	// Output:	   ERMsg : error message
	//
	// Note:
	//****************************************************************************
	ERMsg CGridHelp::Open(const char* filePath)
	{
		_ASSERTE(m_OpenFunction != NULL);

		ERMsg message;

		char messageOut[1024] = { 0 };
		if (!m_OpenFunction(filePath, messageOut))
		{
			message.asgType(ERMsg::ERREUR);
			message.ajoute(messageOut);
		}

		return message;
	}


	//****************************************************************************
	// Summary:     Close a grid
	//
	// Description:  Close a grid
	//
	// Input:      
	//
	// Output:	   
	//
	// Note:
	//****************************************************************************
	void CGridHelp::Close()
	{
		_ASSERTE(m_CloseFunction != NULL);

		m_CloseFunction();
	}
	//****************************************************************************
	// Summary: Get the number of columns
	//
	// Description:  Get the number of columns of the open grid
	//
	// Input:	
	//
	// Output:	the number of cols
	//
	// Note:	
	//****************************************************************************
	long CGridHelp::GetNbCols()
	{
		_ASSERTE(m_GetNbColsFunction != NULL);

		ERMsg message;


		return m_GetNbColsFunction();
	}

	//****************************************************************************
	// Summary: Get the number of rows
	//
	// Description:  Get the number of rows of the open grid
	//
	// Input:	
	//
	// Output:	the number of rows
	//
	// Note:	
	//****************************************************************************
	long CGridHelp::GetNbRows()
	{
		_ASSERTE(m_GetNbRowsFunction != NULL);

		return m_GetNbRowsFunction();
	}

	//****************************************************************************
	// Summary: Get the values
	//
	// Description: Get the value of a cell at the position col, row
	//
	// Input:   col: the columns 
	//			row:  the rows
	//
	// Output:	The value of the cell
	//
	// Note:	
	//			
	//****************************************************************************
	double CGridHelp::GetValue(long col, long row)
	{
		_ASSERTE(m_GetValueFunction != NULL);

		return m_GetValueFunction(col, row);
	}


	//****************************************************************************
	// Summary: Get the values
	//
	// Description: Get the value of a cell at the position lat lon in decimal degrre
	//
	// Input:   lat: latitude in degree of the cell. Negatif for southern coord.
	//			lon: longitude in degree of the cell. Negatif for western coord.
	//
	// Output:	The value of the cell
	//
	// Note:	ever if the grid is projected, the coordinate are always in degree
	//			
	//****************************************************************************
	double CGridHelp::GetValueLatLon(const double& lat, const double& lon)
	{
		_ASSERTE(m_GetValueLatLonFunction != NULL);

		return m_GetValueLatLonFunction(lat, lon);
	}

}