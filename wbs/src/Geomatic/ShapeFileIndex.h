//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <vector>
#include "Geomatic/ShapeFileHeader.h"



namespace WBSF
{

	struct SRecordInfo
	{
		__int32 m_offset;
		__int32 m_recordLength;
	};


	typedef std::vector<SRecordInfo> SRecordInfoArray;


	class CShapeFileIndex
	{
	public:
		CShapeFileIndex();
		virtual ~CShapeFileIndex();

		ERMsg  Read(const std::string& filePaht);
		ERMsg  Write(const std::string& filePaht)const;


		int GetNbRecord()const	{ return (int)m_recordsInfo.size(); }
		const CShapeFileHeader& GetHeader()const	{ return m_header; }

		void SetHeader(const CShapeFileHeader& header)
		{
			m_header = header;
			m_header.SetFileLength(50);
			ResetRecordInfo();
		}


		void ResetRecordInfo();
		void AddRecordInfo(__int32 length);


	private:

		CShapeFileHeader m_header;
		SRecordInfoArray m_recordsInfo;
	};
}