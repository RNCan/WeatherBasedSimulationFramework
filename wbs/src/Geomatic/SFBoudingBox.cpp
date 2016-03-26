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
#include "Geomatic/SFBoudingBox.h"

namespace WBSF
{

	CSFBoundingBox::CSFBoundingBox()
	{
		Reset();
	}

	CSFBoundingBox::~CSFBoundingBox()
	{
	}

	CSFBoundingBox& CSFBoundingBox::operator = (const CGeoRect& rect)
	{
		if (&rect != this)
		{
			CGeoRect::operator=(rect);
		}

		return *this;
	}

	//CArchive& CSFBoundingBox::Read(CArchive& io)
	//{
	//    io >> m_xMin;
	//    io >> m_yMin;
	//    io >> m_xMax;
	//    io >> m_yMax;
	//
	//    return io;
	//}
	//CArchive& CSFBoundingBox::Write(CArchive& io)const
	//{
	//    io << m_xMin;
	//    io << m_yMin;
	//    io << m_xMax;
	//    io << m_yMax;
	//
	//    return io;
	//}
	void CSFBoundingBox::Reset()
	{
		CGeoRect::SetRectEmpty();
	}
}