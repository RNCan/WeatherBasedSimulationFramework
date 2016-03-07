// ShapeFileHelp.h: interface for the CShapeFileHelp class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <Windows.h>
#include "basic/ERMsg.h"

namespace WBSF
{


	class CShapeFileHelp
	{
	public:
		CShapeFileHelp();
		virtual ~CShapeFileHelp();

		inline bool IsInit()const;
		ERMsg Initialise(const char* DLLPath, const char* ShapefileFilePath);
		long GetZone(double lat, double lon)const;

	private:

		HINSTANCE m_hDll;

		typedef bool(*InitialiseFunction)(const char*);
		typedef long(*GetZoneFunction)(double lat, double lon);
		InitialiseFunction m_InitialiseFunction;
		GetZoneFunction m_GetZoneFunction;
	};


	inline bool CShapeFileHelp::IsInit()const
	{
		return m_hDll != NULL && m_InitialiseFunction != NULL && m_GetZoneFunction != NULL;
	}

}