//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/GeoBasic.h"

namespace WBSF
{
	class CSFPoint;


	class CSFBoundingBox : public CGeoRect
	{
	public:

		CSFBoundingBox();
		virtual ~CSFBoundingBox();

		template<class Archive>
		Archive& Read(Archive& io)
		{
			io >> m_xMin;
			io >> m_yMin;
			io >> m_xMax;
			io >> m_yMax;

			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)const
		{
			io << m_xMin;
			io << m_yMin;
			io << m_xMax;
			io << m_yMax;

			return io;
		}


		void Reset();
		CSFBoundingBox& operator = (const CGeoRect& rect);
	};


}