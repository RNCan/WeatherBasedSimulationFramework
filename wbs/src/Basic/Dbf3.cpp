//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "StdAfx.h"
#include <float.h>

#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>


#include "Basic/DBF3.h"
#include "Basic/Registry.h"
#include "Basic/UtilStd.h"

using namespace std;



namespace WBSF
{

	CDBFFirstByte::CDBFFirstByte() :
		m_version(3),
		m_bMemo(false)
	{
	}
	

	tm get_current_tm()
	{
		time_t ltime = 0;
		_tzset();
		time(&ltime);

		tm today = { 0 };
		_localtime64_s(&today, &ltime);
		return today;
	}


	CDBF3::CDBF3() :
		m_pAccessor(NULL),
		m_bIncomplete(0),
		m_Encryption(0),
		m_MDX(0),
		m_languageID(0x0),
		m_tableField(NULL),
		m_uniqueIDTableFieldNo(-1),
		m_pUniqueID(NULL)
	{
		tm today = get_current_tm();

		m_year = ((char)today.tm_year);
		m_month = ((char)today.tm_mon + 1);//??
		m_day = ((char)today.tm_mday);

		for (int i = 0; i < 2; i++)
			m_reserved1[i] = 0;

		for (int i = 0; i < 12; i++)
			m_multi_user[i] = 0;

		for (int i = 0; i < 2; i++)
			m_reserved2[i] = 0;
	}

	CDBF3::CDBF3(const CDBF3& dbf)
	{
		m_pUniqueID = NULL;
		m_pAccessor = NULL;
		operator=(dbf);
	}


	CDBF3& CDBF3::operator=(const CDBF3& dbf)
	{
		if (this != &dbf)
		{
			DestroyUniqueIDCtrl();

			m_pAccessor = NULL;
			m_bIncomplete = dbf.m_bIncomplete;
			m_Encryption = dbf.m_Encryption;
			m_MDX = dbf.m_MDX;
			m_languageID = dbf.m_languageID;
			m_year = dbf.m_year;
			m_month = dbf.m_month;
			m_day = dbf.m_day;

			for (int i = 0; i < 2; i++)
				m_reserved1[i] = dbf.m_reserved1[i];

			for (int i = 0; i < 12; i++)
				m_multi_user[i] = dbf.m_multi_user[i];

			for (int i = 0; i < 2; i++)
				m_reserved2[i] = dbf.m_reserved2[i];


			m_tableField = dbf.m_tableField;
			m_records = dbf.m_records;
		}

		return *this;

	}

	CDBF3::~CDBF3()
	{
		if (m_pAccessor)
			delete m_pAccessor;

		m_pAccessor = NULL;
		DestroyUniqueIDCtrl();
	}

	const char CDBF3::END_HEAD = 0x0D;
	const char CDBF3::DBF_FILE_END = 0x1A;

	void CDBF3::Reset()
	{
		m_tableField.clear();
		m_records.clear();

		tm today = get_current_tm();

		m_year = ((char)today.tm_year);
		m_month = ((char)today.tm_mon + 1);//??
		m_day = ((char)today.tm_mday);


		if (m_pAccessor)
			delete m_pAccessor;

		m_pAccessor = NULL;

		if (m_pUniqueID)
			DestroyUniqueIDCtrl();

	}

	CDBFAccessor& CDBF3::operator[](int index)
	{
		ASSERT(index >= 0 && index < (int)m_records.size());
		ASSERT(m_records[index].size() == m_tableField.size());
		if (m_pUniqueID)
			DestroyUniqueIDCtrl();


		if (m_pAccessor)
			delete m_pAccessor;

		m_pAccessor = NULL;
		m_pAccessor = new CDBFAccessor(m_records[index], m_tableField);

		return *m_pAccessor;
	}


	ERMsg CDBF3::Read(const string& filePath)
	{
		m_tableField.clear();
		m_records.clear();
		DestroyUniqueIDCtrl();


		ERMsg msg;

		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);

		if (msg)
		{
			short headerLength = 0;
			short recordLength = 0;

			try
			{
				boost::archive::binary_iarchive io(file, boost::archive::no_header);

				__int32 nbRecord = 0;
				io >> m_firstByte;
				io >> m_year;
				io >> m_month;
				io >> m_day;
				io >> nbRecord;
				io >> headerLength;
				io >> recordLength;
				for (int i = 0; i < 2; i++)
					io >> m_reserved1[i];

				io >> m_bIncomplete;
				io >> m_Encryption;
				for (int i = 0; i < 12; i++)
					io >> m_multi_user[i];

				io >> m_MDX;
				io >> m_languageID;

				for (int i = 0; i < 2; i++)
					io >> m_reserved2[i];


				CDBFField tmpField;
				while (tmpField.Peek(io))
				{
					m_tableField.push_back(tmpField);
				}

				m_records.resize(nbRecord);
				for (int i = 0; i < nbRecord; i++)
				{
					m_records[i].ReadRecord(io, m_tableField);
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

	// pget unique ID for a field
	int CDBF3::GetUniqueID(int recordNo, int tableFieldNo)
	{
		ASSERT(recordNo >= 0 && recordNo < (int)m_records.size());
		ASSERT(tableFieldNo >= -1 && tableFieldNo < (int)m_tableField.size());

		int ID = -1;
		if (tableFieldNo > -1)
		{
			if (m_pUniqueID == NULL || tableFieldNo != m_uniqueIDTableFieldNo)
				CreateUniqueIDCtrl(tableFieldNo);

			std::map<std::string, int>::const_iterator it = m_pUniqueID->find(m_records[recordNo][tableFieldNo].GetElement());
			if (it != m_pUniqueID->end())
				ID = it->second;

		}
		else ID = recordNo;

		ASSERT(ID != -1);
		return ID;

	}

	int CDBF3::GetNbUniqueID(int tableFieldNo)
	{
		ASSERT(tableFieldNo >= -1 && tableFieldNo < (int)m_tableField.size());

		int nbUniqueID = 0;
		if (tableFieldNo > -1)
		{
			if (m_pUniqueID == NULL || tableFieldNo != m_uniqueIDTableFieldNo)
				CreateUniqueIDCtrl(tableFieldNo);

			nbUniqueID = (int)m_pUniqueID->size();
		}
		else nbUniqueID = (int)m_records.size();

		return nbUniqueID;
	}

	const string& CDBF3::GetElementStringForUniqueID(int tableFieldNo, int UniqueID)const
	{
		ASSERT(tableFieldNo >= -1 && tableFieldNo < (int)m_tableField.size());

		//for internal tmp varaible change
		CDBF3& me = const_cast<CDBF3&>(*this);

		if (tableFieldNo > -1)
		{
			if (m_pUniqueID == NULL || tableFieldNo != m_uniqueIDTableFieldNo)
				me.CreateUniqueIDCtrl(tableFieldNo);

			ASSERT(UniqueID >= 0 && UniqueID < (int)m_pUniqueID->size());

			for (std::map<std::string, int>::const_iterator it = m_pUniqueID->begin(); it != m_pUniqueID->end(); it++)
				if (it->second == UniqueID)
					me.m_tmpStr = it->first;

			return me.m_tmpStr;
		}

		me.m_tmpStr = ToString(UniqueID + 1);
		return m_tmpStr;
	}

	void CDBF3::CreateUniqueIDCtrl(int tableFieldNo)
	{
		ASSERT(tableFieldNo >= -1 && tableFieldNo < (int)m_tableField.size());


		if (tableFieldNo > -1)
		{
			if (m_pUniqueID == NULL)
			{
				m_pUniqueID = new std::map < string, int > ;
			}

			//for all element
			size_t nSize = m_records.size();
			for (size_t i = 0; i < nSize; i++)
			{
				(*m_pUniqueID)[m_records[i][tableFieldNo].GetElement()] = (int)i;
			}

			m_uniqueIDTableFieldNo = tableFieldNo;
		}
		else
		{
			if (m_pUniqueID)
				DestroyUniqueIDCtrl();
		}
	}

	void CDBF3::DestroyUniqueIDCtrl()
	{
		if (m_pUniqueID)
			delete m_pUniqueID;

		m_pUniqueID = NULL;
		m_uniqueIDTableFieldNo = -1;
	}

	void CDBF3::ResetField()
	{
		m_tableField.clear();
		m_records.clear();

		DestroyUniqueIDCtrl();
	}

	int CDBF3::AddField(CDBFField& field)
	{
		DestroyUniqueIDCtrl();
		m_tableField.push_back(field);

		//update all records
		for (size_t i = 0; i < m_records.size(); i++)
		{
			ASSERT(m_records[i].size() == m_tableField.size() - 1);
			CDBFElement el;//add empty element
			m_records[i].push_back(el);

			ASSERT(m_records[i].size() == m_tableField.size());
		}

		return (int)m_tableField.size() - 1;
	}

	int CDBF3::AddField(const string& name, const string& format)
	{
		return AddField(CDBFField(name, format));
	}

	int CDBF3::GetFieldIndex(const string& fieldName)const
	{
		int index = -1;
		for (int i = 0; i < (int)m_tableField.size(); i++)
		{
			if (IsEqualNoCase(fieldName, m_tableField[i].GetName()))
			{
				index = i;
				break;
			}
		}

		return index;
	}

	void CDBF3::SetTableField(const CTableField& tableField)
	{
		DestroyUniqueIDCtrl();
		m_tableField = tableField;
	}

	void CDBF3::SetNbRecord(__int32 nbRecord)
	{
		DestroyUniqueIDCtrl();

		int nbField = (int)m_tableField.size();

		m_records.resize(nbRecord);
		for (int index = 0; index < nbRecord; index++)
		{
			if (m_records[index].size() != nbField)
			{
				m_records[index].resize(nbField);
			}
		}

	}

	CDBFAccessor& CDBF3::CreateNewRecord()
	{
		DestroyUniqueIDCtrl();

		int index = (int)m_records.size();
		m_records.resize(index + 1);
		m_records[index].resize(m_tableField.size());

		return operator[](index);
	}

	bool CDBF3::VerifyField(CDBFRecord& record)
	{
		bool bRep = true;
		for (size_t i = 0; i < record.size(); i++)
		{
			if (record[i].GetElement().length() != m_tableField[i].GetLength())
			{
				bRep = false;
				break;
			}
		}

		return bRep;
	}

	ERMsg CDBF3::Write(const string& filePath)const
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


				short headerLength = (short)m_tableField.size() * 32 + 33;
				short recordLength = GetRecordSize();
				__int32 nbRecord = (__int32)m_records.size();

				io << m_firstByte;
				io << m_year;
				io << m_month;
				io << m_day;
				io << nbRecord;
				io << headerLength;
				io << recordLength;
				for (int i = 0; i < 2; i++)
					io << m_reserved1[i];

				io << m_bIncomplete;
				io << m_Encryption;
				for (int i = 0; i < 12; i++)
					io << m_multi_user[i];
				io << m_MDX;
				io << m_languageID;
				for (int i = 0; i < 2; i++)
					io << m_reserved2[i];


				for (size_t i = 0; i < m_tableField.size(); i++)
					io << m_tableField[i];

				io << END_HEAD;

				ASSERT(nbRecord == m_records.size());
				for (int i = 0; i < nbRecord; i++)
				{
					m_records[i].WriteRecord(io, m_tableField);
				}

				io << DBF_FILE_END;
			}
			catch (boost::archive::archive_exception e)
			{
				msg.ajoute(e.what());
			}
		}

		return msg;
	}

	bool CDBF3::GetMinMax(int fieldIndex, float& fMin, float& fMax)const
	{
		ASSERT(fieldIndex >= -1 && fieldIndex < (int)m_tableField.size());
		bool bRep = false;


		if (fieldIndex > -1)
		{
			if (m_tableField[fieldIndex].GetType() == CDBFField::NUMBER)
			{
				fMin = (float)-FLT_MAX;
				fMax = (float)FLT_MAX;

				bRep = true;
				for (size_t i = 0; i < m_records.size(); i++)
				{
					const CDBFRecord* pRecord = m_records.data();
					const CDBFElement* pElement = (pRecord[i]).data();

					double value = atof(pElement[fieldIndex].GetElement().c_str());
					if (value < fMin)
						fMin = float(value);
					if (value > fMax)
						fMax = float(value);
				}
			}
		}
		else
		{
			fMin = 0;
			fMax = float(m_records.size() - 1);
		}

		return bRep;
	}
	//**************************************************************************

	const char CDBFRecord::ENABLE = 0x20;//space
	const char CDBFRecord::DELETED = 0x2a;//*

	CDBFRecord::CDBFRecord() :
		m_state(ENABLE)
	{
	}
	CDBFRecord::CDBFRecord(const CDBFRecord& record)
	{
		operator = (record);
	}

	CDBFRecord& CDBFRecord::operator = (const CDBFRecord& record)
	{
		if (&record != this)
		{
			m_state = record.m_state;
			operator=(record);
		}

		return *this;
	}

	void CDBFElement::SetElement(const CDBFField& field, bool bElement)
	{
		ASSERT(field.GetType() == 'L');
		if (bElement)
			m_string = 'T';
		else m_string = 'F';
	}

	void CDBFElement::SetElement(const CDBFField& field, int nElement)
	{
		ASSERT(field.GetType() == 'N');

		m_string = ToString(nElement);
		ASSERT(m_string.length() <= field.GetLength());
		//    AjustLength(field.GetLength());

		//ASSERT(m_string.GetLength() == field.GetLength() );
	}

	void CDBFElement::SetElement(const CDBFField& field, float fElement)
	{
		ASSERT(field.GetType() == 'N');
		m_string = ToString(fElement, field.GetDecimalCount());
		ASSERT(m_string.length() <= field.GetLength());

		//AjustLength(field.GetLength());

		//ASSERT(m_string.GetLength() == field.GetLength() );
	}
	void CDBFElement::SetElement(const CDBFField& field, const char* sElement)
	{
		ASSERT(field.GetType() == 'C');
		m_string = sElement;
		ASSERT(m_string.length() <= field.GetLength());
	}

	void CDBFElement::SetElement(const CDBFField& field, __time64_t element)
	{
		ASSERT(field.GetType() == 'D');
		ASSERT(field.GetLength() == 8);

		tm time = { 0 };
		_localtime64_s(&time, &element);

		m_string.resize(9);
		sprintf("%04d%02d%02d", &(m_string[0]), time.tm_year + 1900, time.tm_mon + 1, time.tm_mday);
	}

	//    enum TElement{ CHARACTER, DATE, FLOAT, NUMBER, BOOL, MEMO, UNKNOW};
	CDBFField::CDBFField(const string& name, const string& format) :
		m_type(0),
		m_fieldLength(0),
		m_decimalCount(0),
		m_workArea(0),
		m_MDX(0)
	{
		ZeroMemory(m_name, 11);
		ZeroMemory(m_reserved1, 4);
		ZeroMemory(m_reserved2, 3);
		ZeroMemory(m_reserved3, 10);

		if (!name.empty() && !format.empty())
			SetField(name, format);
	}

	bool CDBFField::operator == (const CDBFField& field)const
	{
		bool bRep = true;
		if (m_type != field.m_type) bRep = false;
		if (m_fieldLength != field.m_fieldLength) bRep = false;
		if (m_decimalCount != field.m_decimalCount) bRep = false;
		if (m_workArea != field.m_workArea) bRep = false;
		if (m_MDX != field.m_MDX) bRep = false;
		for (int i = 0; i < 11; i++)
			if (m_name[i] != field.m_name[i]) bRep = false;


		return bRep;
	}

	bool CDBFField::operator != (const CDBFField& field)const
	{
		return !(this->operator ==(field));
	}

	void CDBFField::SetField(const string& name, const string& format)
	{
		string tmp(format);
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
		std::remove(tmp.begin(), tmp.end(), '%');
		//tmp.er.eplace( '%', '' );

		SetName(name);


		if (tmp.find('N') < tmp.length() || tmp.find('D') < tmp.length() || tmp.find('I') < tmp.length())
		{   //number
			SetType('N');
			int length;
			VERIFY(sscanf(tmp.c_str(), "%d", &length));
			ASSERT(length >= 0 && length < 127);
			SetLength((char)length);
		}
		else if (tmp.find('F') < tmp.length())
		{   //float
			SetType('N');
			int length = 0;
			int decimalCount = 0;
			char bidon = 0;
			//        ASSERT(false); // je croit pas que ca marche
			int var = sscanf(tmp.c_str(), "%d%c%d", &length, &bidon, &decimalCount);
			ASSERT(var >= 1 && var <= 3);
			ASSERT(length < 127);
			SetLength((char)length);
			if (var == 3 && bidon == '.')
			{
				ASSERT(decimalCount < length);
				SetDecimalCount(decimalCount);
			}
		}
		else if (tmp.find('C') < tmp.length() || tmp.find('S') < tmp.length())
		{   //string
			SetType('C');
			int length = 0;
			VERIFY(sscanf(tmp.c_str(), "%d", &length));
			ASSERT(length < 127);
			SetLength((char)length);
		}
		else if (tmp.find('T') < tmp.length())
		{   //date
			SetType('D');
			SetLength(8);
		}
		else if (tmp.find('L') < tmp.length())
		{   //logical
			SetType('L');
			SetLength(1);
		}
		else ASSERT(false);
	}

	//***************************************************************************

	void CDBFFieldIdentifier::LoadFromRegistry(CRegistry& option, const string& entryName)
	{
		m_name = option.GetValue<string>(entryName + " Name", "");
		m_index = option.GetValue<int>(entryName + " Index", -1);

	}

	void CDBFFieldIdentifier::SaveToRegistry(CRegistry& option, const string& entryName)const
	{
		option.SetValue(entryName + " Name", m_name);
		option.SetValue(entryName + " Index", m_index);
	}

	//*******************************************************
}