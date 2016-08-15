// ShapeFileHelp.cpp: implementation of the CShapeFileHelp class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <crtdbg.h>

#include "Basic/UtilStd.h"
#include "ModelBase/ShapeFileHelp.h"


namespace WBSF
{

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CShapeFileHelp::CShapeFileHelp()
	{
		m_hDll = NULL;
		m_InitialiseFunction = NULL;
		m_GetZoneFunction = NULL;
	}

	CShapeFileHelp::~CShapeFileHelp()
	{
		if (m_hDll)
		{
			FreeLibrary(m_hDll);
			m_hDll = NULL;
		}

		m_InitialiseFunction = NULL;
		m_GetZoneFunction = NULL;
	}


	ERMsg CShapeFileHelp::Initialise(const char* DLLFilePath, const char* ShapefileFilePath)
	{
		ERMsg message;

		if (m_hDll)
		{
			FreeLibrary(m_hDll);
			m_hDll = NULL;
		}

		m_hDll = LoadLibraryW(convert(DLLFilePath).c_str());

		if (m_hDll != NULL)
		{
			m_InitialiseFunction = (InitialiseFunction)GetProcAddress(m_hDll, "Initialise");
			m_GetZoneFunction = (GetZoneFunction)GetProcAddress(m_hDll, "GetZone");

			if (m_InitialiseFunction && m_GetZoneFunction)
			{
				if (!m_InitialiseFunction(ShapefileFilePath))
				{
					message.asgType(ERMsg::ERREUR);
					message.ajoute("erreur");
				}
			}
			else
			{
				message.asgType(ERMsg::ERREUR);
				message.ajoute("erreur");
			}

		}
		else
		{
			message.asgType(ERMsg::ERREUR);
			message.ajoute("erreur");
		}

		return message;
	}

	long CShapeFileHelp::GetZone(double lat, double lon)const
	{
		_ASSERTE(m_GetZoneFunction != NULL);

		return m_GetZoneFunction(lat, lon);
	} 
}