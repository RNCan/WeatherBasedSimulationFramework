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
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>

#include "Geomatic/ShapeFileIndex.h"

using namespace std;


namespace WBSF
{

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CShapeFileIndex::CShapeFileIndex()
	{}

	CShapeFileIndex::~CShapeFileIndex()
	{
	}


	ERMsg CShapeFileIndex::Read(const string& filePath)
	{
		ERMsg msg;

		m_recordsInfo.clear();

		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::archive::binary_iarchive io(file, boost::archive::no_header);
				msg = CShapeFileHeader::ReadHeader(m_header, io);

				if (msg)
				{
					ASSERT((m_header.GetFileLength() - 50) % 4 == 0);
					int nbRecord = ((m_header.GetFileLength() - 50) / 4);

					m_recordsInfo.resize(nbRecord);
					for (int i = 0; i < nbRecord; i++)
					{
						m_recordsInfo[i].m_offset = ReadBigEndian(io);
						m_recordsInfo[i].m_recordLength = ReadBigEndian(io);
					}
				}
			}
			catch (boost::archive::archive_exception e)
			{
				msg.ajoute(e.what());
			}

			file.close();
		}




		return msg;
	}

	ERMsg CShapeFileIndex::Write(const string& filePath)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath, ios::out | ios::binary);
		if (msg)
		{
			try
			{
				// write map instance to archive
				boost::archive::binary_oarchive io(file, boost::archive::no_header);
				m_header.WriteHeader(io);

				ASSERT((m_header.GetFileLength() - 50) % 4 == 0);

				int nbRecord = (int)m_recordsInfo.size();
				for (int i = 0; i < nbRecord; i++)
				{
					WriteBigEndian(io, m_recordsInfo[i].m_offset);
					WriteBigEndian(io, m_recordsInfo[i].m_recordLength);
				}
			}
			catch (boost::archive::archive_exception e)
			{
				msg.ajoute(e.what());
			}

			file.close();
		}


		return msg;
	}

	void CShapeFileIndex::ResetRecordInfo()
	{
		m_recordsInfo.clear();
	}

	void CShapeFileIndex::AddRecordInfo(__int32 length)
	{
		SRecordInfo info;

		int pos = (int)m_recordsInfo.size() - 1;
		if (pos >= 0)
			info.m_offset = m_recordsInfo[pos].m_offset + m_recordsInfo[pos].m_recordLength + 4;
		else info.m_offset = 50;

		info.m_recordLength = length;
		m_recordsInfo.push_back(info);


		m_header.SetFileLength(50 + (int)m_recordsInfo.size() * 4);
	}

}