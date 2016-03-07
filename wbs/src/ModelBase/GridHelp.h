//***************************************************************************
// File:	   GridHelp.h
//
// Class:      CGridHelp
//
// Summary:    Help class to use GridLib.dll
//
// Description: This class is used to wrap GridLib.dll functions into a c++ class
//
// Attributes:   void * m_hDll: handle on the dll
//
// Note:        
//***************************************************************************
// 03-04-2006  Rémi Saint-Amant  Initial Version 
//****************************************************************************
#pragma once


#include "basic/ERMsg.h"

namespace WBSF
{
	class CGridHelp
	{
	public:

		CGridHelp();
		virtual ~CGridHelp();

		inline bool IsInit()const;
		ERMsg Initialize(const char* DLLPath);


		ERMsg Open(const char* fileName);
		void Close();
		long GetNbCols();
		long GetNbRows();
		double GetValue(long col, long row);
		double GetValueLatLon(const double& lat, const double& lon);


	private:

		void * m_hDll;

		typedef bool(*OpenFunction)(const char* fileName, char messageOut[255]);
		typedef void(*CloseFunction)();
		typedef long(*GetNbColsFunction)();
		typedef long(*GetNbRowsFunction)();
		typedef double(*GetValueFunction)(long col, long row);
		typedef double(*GetValueLatLonFunction)(const double& lat, const double& lon);

		OpenFunction m_OpenFunction;
		CloseFunction m_CloseFunction;
		GetNbColsFunction m_GetNbColsFunction;
		GetNbRowsFunction m_GetNbRowsFunction;
		GetValueFunction m_GetValueFunction;
		GetValueLatLonFunction m_GetValueLatLonFunction;
	};



	inline bool CGridHelp::IsInit()const
	{
		return m_hDll != NULL && m_OpenFunction != NULL &&
			m_CloseFunction != NULL &&
			m_GetNbColsFunction != NULL &&
			m_GetNbRowsFunction != NULL &&
			m_GetValueFunction != NULL &&
			m_GetValueLatLonFunction != NULL;
	}

}