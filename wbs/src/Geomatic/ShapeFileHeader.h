//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Geomatic/SFBoudingBox.h"

namespace boost 
{
	namespace archive 
	{
		class binary_iarchive;
	}
}

namespace WBSF
{

	extern ERMsg ReadHeader();

	template<class Archive>
	__int32 ReadBigEndian(Archive & io)
	{

		unsigned char pjSrc[4] = { 0 };
		io.load_binary(&pjSrc, 4);
		__int32    lStorage = 0;

		lStorage = (__int32)       // Source is big endian, shift the 
			((pjSrc[0] << 24) |    // high bytes low. 
			(pjSrc[1] << 16) |
			(pjSrc[2] << 8) |
			(pjSrc[3])
			);

		return (lStorage);
	}




	/*++

	Routine Description:

	Copies four byte long into a destination buffer.  The buffer does not
	need to be DWORD aligned.  This is generally used for copying to
	mapped disk files.  If CVT_BIG_ENDIAN_SUPPORT is defined then the
	fDestEndianType is checked before copy.

	Arguments:

	lSource     - Signed long to convert to buffer.
	pjDest      - Supplies pointer to destination buffer for the LONG.

	Return Value:

	None.

	--*/

	template<class Archive>
	void WriteBigEndian(Archive & io, __int32 lSource)
	{
		unsigned char pjDest[4] = { 0 };
		pjDest[3] = (unsigned char)((lSource)& 0xFF);
		pjDest[2] = (unsigned char)((lSource >> 8) & 0xFF);
		pjDest[1] = (unsigned char)((lSource >> 16) & 0xFF);
		pjDest[0] = (unsigned char)((lSource >> 24) & 0xFF);

		io.save_binary(&pjDest, 4);
	}

	//**************************************************************
	class CShapeFileHeader
	{
	public:

		enum TShape { SHAPE_NULL = 0, POINT = 1, POLYLINE = 3, POLYGON = 5, MULTIPOINT = 8, NBTYPE = 5 };
		static ERMsg ReadHeader(CShapeFileHeader& header, boost::archive::binary_iarchive& io);

		CShapeFileHeader();
		CShapeFileHeader(const CShapeFileHeader& shapeFileHeader);
		virtual ~CShapeFileHeader();
		void clear();
		CShapeFileHeader& operator =(const CShapeFileHeader& in);

		


		template<class Archive>
		void  ReadHeader(Archive & io)
		{
			m_fileCode = ReadBigEndian(io);

			for (int i = 0; i < 5; i++)
				m_unused[i] = ReadBigEndian(io);

			m_fileLength = ReadBigEndian(io);

			io >> m_version;
			io >> m_shapeType;

			ASSERT(IsLegalShapeType(m_shapeType));

			m_boundingBox.Read(io);

			io >> m_Zmax;
			io >> m_Zmax;
			io >> m_Mmax;
			io >> m_Mmax;
		}

		template<class Archive>
		void  WriteHeader(Archive & io)const
		{
			WriteBigEndian(io, m_fileCode);
			for (int i = 0; i < 5; i++)
				WriteBigEndian(io, m_unused[i]);

			WriteBigEndian(io, m_fileLength);
			io << m_version;
			io << m_shapeType;
			m_boundingBox.Write(io);
			io << m_Zmax;
			io << m_Zmax;
			io << m_Mmax;
			io << m_Mmax;
		}




		static bool IsLegalShapeType(__int32 t);

		const CSFBoundingBox& GetBoundingBox()const	{ return m_boundingBox; }
		void SetBoundingBox(const CGeoRect& bounds)	{ m_boundingBox = bounds; }
		TShape GetShapeType()const	{ return (TShape)m_shapeType; }
		void SetShapeType(TShape type)	{ m_shapeType = (__int32)type; }
		__int32 GetFileLength()const	{ return m_fileLength; }
		void SetFileLength(__int32 length)	{ m_fileLength = length; }

		__int32 GetFileCode()const{ return m_fileCode; }
		double GetZmin()const	{ return m_Zmin; }
		void SetZmin(double min)	{ m_Zmin = min; }
		double GetZmax()const	{ return m_Zmax; }
		void SetZmax(double max)	{ m_Zmax = max; }
		double GetMmin()const	{ return m_Mmin; }
		void SetMmin(double min)	{ m_Mmin = min; }
		double GetMmax()const	{ return m_Mmax; }
		void SetMmax(double max)	{ m_Mmax = max; }
		void ExtendBounds(const CSFBoundingBox& box)	{ m_boundingBox.ExtendBounds(box); }

	private:

		__int32 m_fileCode;
		__int32 m_unused[5];
		__int32 m_fileLength;
		__int32 m_version;
		__int32 m_shapeType;
		CSFBoundingBox m_boundingBox;
		double m_Zmin;
		double m_Zmax;
		double m_Mmin;
		double m_Mmax;

	};

}