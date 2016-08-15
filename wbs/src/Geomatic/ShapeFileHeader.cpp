//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "Geomatic/ShapeFileHeader.h"
#include <boost\archive\binary_iarchive.hpp>

#include "WeatherBasedSimulationString.h"

namespace WBSF
{

	ERMsg CShapeFileHeader::ReadHeader(CShapeFileHeader& header, boost::archive::binary_iarchive& io)
	{
		ERMsg msg;
		header.ReadHeader(io);
		if (header.GetFileCode() != 9994)
		{
			msg.ajoute(GetString(IDS_MAP_INVALID_SHAPEFILE));
		}
		if (!header.IsLegalShapeType(header.GetShapeType()))
		{
			msg.ajoute(GetString(IDS_MAP_INVALID_SHAPETYPE));
		}

		return msg;
	}


	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CShapeFileHeader::CShapeFileHeader()
	{
		clear();
	}

	CShapeFileHeader::CShapeFileHeader(const CShapeFileHeader& shapeFileHeader)
	{
		operator =(shapeFileHeader);
	}


	void CShapeFileHeader::clear()
	{
		m_fileCode = 9994;
		m_fileLength = 50;
		m_version = 1000;
		m_shapeType = SHAPE_NULL;
		m_boundingBox.clear();
		m_Zmin = 0;
		m_Zmax = 0;
		m_Mmin = 0;
		m_Mmax = 0;

		ZeroMemory(m_unused, sizeof(__int32) * 5);

	}

	CShapeFileHeader::~CShapeFileHeader()
	{}


	CShapeFileHeader& CShapeFileHeader::operator =(const CShapeFileHeader& in)
	{
		if (&in != this)
		{
			m_fileCode = in.m_fileCode;
			for (int i = 0; i < 5; i++)
				m_unused[i] = in.m_unused[i];

			m_fileLength = in.m_fileLength;
			m_version = in.m_version;
			m_shapeType = in.m_shapeType;
			//warning : the bounding box projection is not update here
			m_boundingBox = in.m_boundingBox;
			m_Zmin = in.m_Zmin;
			m_Zmax = in.m_Zmax;
			m_Mmin = in.m_Mmin;
			m_Mmax = in.m_Mmax;
		}

		return *this;
	}


	bool CShapeFileHeader::IsLegalShapeType(__int32 t)
	{
		return (t == POINT || t == POLYLINE || t == POLYGON || t == MULTIPOINT);
		//|| t == 11 || t == 13|| t == 15|| t == 18|| t == 21
		//|| t == 23|| t == 25|| t == 28|| t == 31);
	}
}