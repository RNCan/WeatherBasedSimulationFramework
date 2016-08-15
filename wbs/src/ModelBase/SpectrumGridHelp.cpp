//****************************************************************************
// Fichier: SpectrumGridHelp.cpp
// Classe:  CSpectrumGridHelp 
// Usage:
//	
//	To use CSpectrumGridHelp. There are 3 steps:
//		1) initialization of the DLL with the method : Initialize		
//		2) Open a grid and used it :
//				Open
//				GetValue
//				GetNbCols
//				GetNbRows
//				GetNbBand
//				GetIU
//		3) Close de grid: Close
//
//****************************************************************************
// 03-04-2006  Rémi Saint-Amant  Initial version 
//****************************************************************************

#include "stdafx.h"
#include <crtdbg.h>
#include <windows.h>

#include "Basic/UtilStd.h"
#include "ModelBase/SpectrumGridHelp.h"

using namespace std;

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
	CSpectrumGridHelp::CSpectrumGridHelp()
	{
		m_hDll = NULL;
		m_OpenFunction = NULL;
		m_CloseFunction = NULL;
		m_GetNbColsFunction = NULL;
		m_GetNbRowsFunction = NULL;
		m_GetValueFunction = NULL;
		m_GetValueLatLonFunction = NULL;
		m_GetNbBandsFunction = NULL;
		m_GetIUFunction = NULL;

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
	CSpectrumGridHelp::~CSpectrumGridHelp()
	{
		if (m_hDll)
		{
			//verify that the file is close
			Close();


			FreeLibrary((HMODULE)m_hDll);
			m_hDll = NULL;
		}

		m_OpenFunction = NULL;
		m_GetNbColsFunction = NULL;
		m_GetNbRowsFunction = NULL;
		m_GetValueFunction = NULL;
		m_GetValueLatLonFunction = NULL;
		m_CloseFunction = NULL;
		m_GetNbBandsFunction = NULL;
		m_GetIUFunction = NULL;
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
	ERMsg CSpectrumGridHelp::Initialize(const char* DLLFilePath)
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
			m_GetNbBandsFunction = (GetNbBandsFunction)GetProcAddress((HMODULE)m_hDll, "GetNbBands");
			m_GetIUFunction = (GetIUFunction)GetProcAddress((HMODULE)m_hDll, "GetIU");

		}
		else
		{
			message.ajoute(string("Unable to load: ") + DLLFilePath);
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
	ERMsg CSpectrumGridHelp::Open(const char* filePath)
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
	void CSpectrumGridHelp::Close()
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
	long CSpectrumGridHelp::GetNbCols()
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
	long CSpectrumGridHelp::GetNbRows()
	{
		_ASSERTE(m_GetNbRowsFunction != NULL);

		return m_GetNbRowsFunction();
	}

	//****************************************************************************
	// Summary: Get the number of bands
	//
	// Description:  Get the number of bands of the open grid (multi bands)
	//
	// Input:	
	//
	// Output:	the number of bands
	//
	// Note:	
	//****************************************************************************
	long CSpectrumGridHelp::GetNbBands()
	{
		_ASSERTE(m_GetNbBandsFunction != NULL);

		return m_GetNbBandsFunction();
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
	double CSpectrumGridHelp::GetValue(long col, long row, long band)
	{
		_ASSERTE(m_GetValueFunction != NULL);

		return m_GetValueFunction(col, row, band);
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
	double CSpectrumGridHelp::GetValueLatLon(const double& lat, const double& lon, long band)
	{
		_ASSERTE(m_GetValueLatLonFunction != NULL);

		return m_GetValueLatLonFunction(lat, lon, band);
	}


	double CSpectrumGridHelp::GetIU(long band)
	{
		_ASSERTE(m_GetIUFunction != NULL);

		return m_GetIUFunction(band);


	}
}