//***************************************************************************
// File:	   SpectrumGridHelp.h
//
// Class:      CSpectrumGridHelp
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

	class CSpectrumGridHelp
	{
	public:

		CSpectrumGridHelp();
		virtual ~CSpectrumGridHelp();

		inline bool IsInit()const;
		ERMsg Initialize(const char* DLLPath);


		ERMsg Open(const char* fileName);
		void Close();
		long GetNbCols();
		long GetNbRows();
		long GetNbBands();
		double GetValue(long col, long row, long band);
		double GetValueLatLon(const double& lat, const double& lon, long band);
		double GetIU(long band);


	private:

		void * m_hDll;

		typedef bool(*OpenFunction)(const char* fileName, char messageOut[1024]);
		typedef void(*CloseFunction)();
		typedef long(*GetNbColsFunction)();
		typedef long(*GetNbRowsFunction)();
		typedef long(*GetNbBandsFunction)();
		typedef double(*GetValueFunction)(long col, long row, long band);
		typedef double(*GetValueLatLonFunction)(const double& lat, const double& lon, long band);
		typedef double(*GetIUFunction)(long band);

		OpenFunction m_OpenFunction;
		CloseFunction m_CloseFunction;
		GetNbColsFunction m_GetNbColsFunction;
		GetNbRowsFunction m_GetNbRowsFunction;
		GetNbBandsFunction m_GetNbBandsFunction;
		GetValueFunction m_GetValueFunction;
		GetValueLatLonFunction m_GetValueLatLonFunction;
		GetIUFunction m_GetIUFunction;
	};



	inline bool CSpectrumGridHelp::IsInit()const
	{
		return m_hDll != NULL && m_OpenFunction != NULL &&
			m_CloseFunction != NULL &&
			m_GetNbColsFunction != NULL &&
			m_GetNbRowsFunction != NULL &&
			m_GetValueFunction != NULL &&
			m_GetValueLatLonFunction != NULL &&
			m_GetNbBandsFunction != NULL &&
			m_GetIUFunction != NULL;
	}

}