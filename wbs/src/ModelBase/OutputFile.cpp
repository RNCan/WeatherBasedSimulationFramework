//*********************************************************************
// File: OutputFile.h
//
// Class:	COutputFile: Helper class to output variables
//
// Description: COutputFile automatically formats output variables to comform 
//				with the model decription and adds seperators.
//
//*********************************************************************
// 10/12/2014   Rémi Saint-Amant	remove old format. Only save as CSV with header format
// 22/03/2007	Rémi Saint-Amant	replace CCFLString by CStdString
// 17/02/2004   Rémi Saint-Amant    Creation.
//*********************************************************************
#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include <fstream>

#include "ModelBase/OutputFile.h" 

namespace WBSF
{

	COutputFile::COutputFile()
	{
		m_separator = ",";
		m_parameters.clear();
	}

	COutputFile::~COutputFile()
	{
	}

	ERMsg COutputFile::open(const std::string& filePath)
	{

		ERMsg msg = ofStream::open(filePath);

		if (msg)
		{
			assert(!m_parameters.empty());
			for (size_t i = 0; i < m_parameters.size(); i++)
			{
				((std::ofstream&)*this) << m_parameters[i].GetName();
				((std::ofstream&)*this) << m_separator;
			}
			EndLine();

			flags(std::ios::fixed | std::ios::right);
			precision(5);

		}


		return msg;
	}

	void COutputFile::SetParameter(const CParameterVector& parameters)
	{
		m_parameters = parameters;
	}


	COutputFile& COutputFile::EndLine()
	{
#if _MSC_VER >= 1300
		std::ofstream::operator <<(std::endl);
#else
		((std::ofstream*)(this))->operator <<(std::endl);
#endif


		return *this;
	}

}