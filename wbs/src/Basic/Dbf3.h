//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <vector>
#include "basic/ERMsg.h"

#include "Basic/UtilStd.h"


namespace WBSF
{

	class CRegistry;

	//***************************************************************************
	class CDBFFirstByte
	{
	public:

		CDBFFirstByte();

		template<class Archive>
		friend Archive& operator >>(Archive& io, CDBFFirstByte& bits)
		{
			char tmp;
			io >> tmp;

			bits.m_version = (tmp&(0x03));
			bits.m_bMemo = tmp&(0x80) ? true : false;

			return io;
		}

		template<class Archive>
		friend Archive& operator <<(Archive& io, const CDBFFirstByte& bits)
		{
			char tmp = 0;

			tmp |= ((0x03) & bits.m_version);
			tmp |= bits.m_bMemo ? 0x80 : 0x00;

			io << tmp;


			return io;
		}


		short GetVersion()const	{ return m_version; }
		void SetVersion(short version)	{ m_version = version; }
		bool GetMemo()const	{ return m_bMemo; }
		void GetMemo(bool bMemo)	{ m_bMemo = bMemo; }

	private:

		short m_version;
		bool m_bMemo;

	};

	//***************************************************************************
	class CDBFFieldIdentifier
	{
	public:

		CDBFFieldIdentifier(const std::string& name = "", short index = -999){ Set(name, index); };

		void Reset(){ Set(); };
		void Set(const std::string& name = "", short index = -999)
		{
			m_name = TrimConst(name);
			m_index = index;
		};

		void LoadFromRegistry(CRegistry& option, const std::string& entryName);
		void SaveToRegistry(CRegistry& option, const std::string& entryName)const;

		const std::string& GetName()const{ return m_name; };
		void SetName(const std::string& name){ m_name = TrimConst(name); };
		short GetIndex()const{ return m_index; };
		void SetIndex(short index){ m_index = index; };

		bool IsValid()const{ return !m_name.empty() && m_index >= -1; };
	private:

		std::string m_name;
		short m_index;
	};


	//***************************************************************************
	class CDBFField
	{
	public:

		enum TElement{ CHARACTER = 'C', DATE = 'D', NUMBER = 'N', BOOL = 'L', MEMO = 'M', UNKNOW };
		CDBFField(const std::string& name = "", const std::string& format = "");

		//friend CArchive& operator <<(CArchive& io, const CDBFField& field);
		bool operator == (const CDBFField& field)const;
		bool operator != (const CDBFField& field)const;
		//bool Peek(CArchive& io);

		template<class Archive>
		friend Archive& operator <<(Archive& io, const CDBFField& field)
		{
			for (int i = 0; i < 11; i++)
				io << field.m_name[i];
			io << field.m_type;

			for (int i = 0; i < 4; i++)
				io << field.m_reserved1[i];
			io << field.m_fieldLength;
			io << field.m_decimalCount;

			for (int i = 0; i < 2; i++)
				io << field.m_reserved2[i];
			io << field.m_workArea;

			for (int i = 0; i < 10; i++)
				io << field.m_reserved3[i];

			io << field.m_MDX;

			return io;
		}

		template<class Archive>
		bool Peek(Archive& io)
		{
			bool bRep = false;
			io >> m_name[0];
			if (m_name[0] != CDBF3::END_HEAD)
			{
				bRep = true;

				for (int i = 1; i < 11; i++)
					io >> m_name[i];
				io >> m_type;

				for (int i = 0; i < 4; i++)
					io >> m_reserved1[i];
				io >> m_fieldLength;
				io >> m_decimalCount;

				for (int i = 0; i < 2; i++)
					io >> m_reserved2[i];
				io >> m_workArea;

				for (int i = 0; i < 10; i++)
					io >> m_reserved3[i];

				io >> m_MDX;

			}

			return bRep;
		}



		const char* GetName()const	{ return &m_name[0]; }
		void SetName(const std::string& name)	{ strncpy_s(m_name, 11, name.c_str(), 10); }
		char GetType()const	{ return m_type; }
		void SetType(char type)	{ m_type = (char)toupper(type);   	ASSERT(m_type == 'C' || m_type == 'D' || m_type == 'N' || m_type == 'L' || m_type == 'M'); }
		unsigned char GetLength()const	{ return m_fieldLength; }
		void SetLength(unsigned char length)	{ m_fieldLength = length; }
		unsigned char GetDecimalCount()const	{ return m_decimalCount; }
		void SetDecimalCount(unsigned char decimelCount)	{ m_decimalCount = decimelCount; }
		char GetWorkArea()const	{ return m_workArea; }
		void SetWorkArea(char workArea)	{ m_workArea = workArea; }
		char GetMDX()const	{ return m_MDX; }
		void SetMDX(char MDX)	{ m_MDX = MDX; }


		void SetField(const std::string& name, const std::string& format);//format comme printf
	private:
		char m_name[11];
		char m_type;
		char m_reserved1[4];
		unsigned char m_fieldLength;
		unsigned char m_decimalCount;
		char m_reserved2[2];
		char m_workArea;
		char m_reserved3[10];
		char m_MDX;

	};

	typedef std::vector<CDBFField> CTableField;


	//***************************************************************************
	//CDBFElement
	class CDBFElement
	{
	public:

		template<class Archive>
		void ReadElement(Archive& io, const CDBFField& field)
		{
			m_string.resize(field.GetLength());
			io.load_binary((char*)(m_string.data()), field.GetLength());
			m_string.resize(strlen(m_string.c_str()));
			m_string = Trim(m_string);
		}

		template<class Archive>
		void WriteElement(Archive& io, const CDBFField& field)const
		{
			string tmp = m_string;
			while (tmp.length() != field.GetLength())
			{
				if (tmp.length() < field.GetLength())
					tmp.insert(tmp.begin(), ' ');
				else tmp = m_string.substr(field.GetLength());
			}

			ASSERT(tmp.length() == field.GetLength());
			io.save_binary(tmp.data(), field.GetLength());

		}


		std::string& GetElement()	{ return m_string; }
		const std::string& GetElement()const{ return m_string; }

		operator LPCSTR()const{ return GetElement().c_str(); };
		void SetElement(const CDBFField& field, bool bElement);
		void SetElement(const CDBFField& field, int nElement);
		void SetElement(const CDBFField& field, float fElement);
		void SetElement(const CDBFField& field, const char* sElement);
		void SetElement(const CDBFField& field, __time64_t element);

	private:

		std::string m_string;
	};



	typedef std::vector< CDBFElement> CDBFElementArray;

	//***************************************************************************
	//CDBFRecord 
	class CDBFRecord : public CDBFElementArray
	{
	public:

		CDBFRecord();
		CDBFRecord(const CDBFRecord& record);
		static const char ENABLE;
		static const char DELETED;

		CDBFRecord& operator = (const CDBFRecord& record);
		//void ReadRecord(CArchive& io, const CTableField& tableField);
		//void WriteRecord(CArchive& io, const CTableField& tableField)const;

		template<class Archive>
		void ReadRecord(Archive& io, const CTableField& tableField)
		{
			io >> m_state;
			ASSERT(m_state == ENABLE || m_state == DELETED);

			int nSize = (int)tableField.size();

			resize(nSize);

			for (int i = 0; i < nSize; i++)
				at(i).ReadElement(io, tableField[i]);
		}

		template<class Archive>
		void WriteRecord(Archive& io, const CTableField& tableField)const
		{
			io << m_state;
			ASSERT(m_state == ENABLE || m_state == DELETED);

			int nSize = (int)tableField.size();


			for (int i = 0; i < nSize; i++)
				at(i).WriteElement(io, tableField[i]);
		}


		bool GetState()const{ return m_state == ' '; }
		void SetState(bool state){ m_state = state ? ' ' : '*'; }

	private:

		char m_state;
	};



	typedef std::vector<CDBFRecord> CDBFRecordArray;

	//***************************************************************************
	//CDBFAccessor
	class CDBFAccessor
	{
	public:
		CDBFAccessor(CDBFRecord& record, const CTableField& tableField) :
			m_record(record),
			m_tableField(tableField)
		{
			if (m_record.size() == 0)
				m_record.resize(m_tableField.size());

			ASSERT(m_record.size() == m_tableField.size());
		}


		int GetNbField()const	{ return (int)m_tableField.size(); }
		const CDBFField& GetField(int index)	{ return m_tableField[index]; }
		void SetElement(int index, bool bElement)	{ m_record[index].SetElement(m_tableField[index], bElement); }
		void SetElement(int index, int nElement)	{ m_record[index].SetElement(m_tableField[index], nElement); }
		void SetElement(int index, float fElement)	{ m_record[index].SetElement(m_tableField[index], fElement); }
		void SetElement(int index, const char* sElement)	{ m_record[index].SetElement(m_tableField[index], sElement); }
		void SetElement(int index, __time64_t element)	{ m_record[index].SetElement(m_tableField[index], element); }

		const std::string& operator [] (int index)const{ return GetElement(index); };
		const std::string& GetElement(int index)const	{ return m_record[index].GetElement(); }

		CDBFRecord& m_record;
		const CTableField& m_tableField;
	};


	//***************************************************************************
	//CDBF3
	class CDBF3
	{
	public:
		static const char END_HEAD;
		static const char DBF_FILE_END;



		CDBF3();
		CDBF3(const CDBF3& dbf);
		virtual ~CDBF3();
		CDBF3& operator=(const CDBF3& dbf);

		void clear(){ Reset(); }
		void Reset();
		ERMsg Read(const std::string& filePaht);
		ERMsg Write(const std::string& filePaht)const;

		template<class Archive>
		bool EndOK(Archive& io)
		{
			char test(0);
			io >> test;// .Read(&test, 1);
			return test == DBF_FILE_END;
		}

		
		int GetUniqueID(int RecordNo, int tableFieldNo);
		int GetNbUniqueID(int tableFieldNo);
		const std::string& GetElementStringForUniqueID(int tableFieldNo, int UniqueID)const;
		void CreateUniqueIDCtrl(int tableFieldNo);
		void DestroyUniqueIDCtrl();

		CDBFAccessor& operator[](int index);


		void ResetField();
		int AddField(CDBFField& field);
		int AddField(const std::string& name, const std::string& format);
		void SetTableField(const CTableField& tableField);
		int GetFieldIndex(const std::string& fieldName)const;

		void SetNbRecord(__int32 nbRecord);
		CDBFAccessor& CreateNewRecord();

		bool GetMinMax(int fieldIndex, float& fMin, float& fMax)const;

		int AddRecord(const CDBFRecord& record)
		{
			ASSERT(m_tableField.size() == record.size());
			int pos = (int)m_records.size();
			m_records.resize(pos + 1);
			m_records[pos] = record;
			return pos;
		}

		void SetRecord(int pos, const CDBFRecord& record)
		{
			m_records[pos] = record;
		}

		void RemoveRecord(int pos)
		{
			m_records.erase(m_records.begin() + pos);
		}

		const CDBFRecord& operator[](int index)const
		{
			ASSERT(index >= 0 && index < (int)m_records.size());
			return m_records[index];
		}

		const CTableField& GetTableField()const
		{
			return m_tableField;
		}

		short GetRecordSize()const
		{
			int size = 1;//state BYTE
			for (size_t i = 0; i < m_tableField.size(); i++)
			{
				size += m_tableField[i].GetLength();
			}

			return (short)size;
		}

		__int32 GetNbRecord()const
		{
			return (__int32)m_records.size();
		}

		short GetNbField()const
		{
			return (short)m_tableField.size();
		}


	private:

		bool VerifyField(CDBFRecord& record);

		CDBFFirstByte m_firstByte;
		char m_year;
		char m_month;
		char m_day;
		char m_reserved1[2];
		char m_bIncomplete;
		char m_Encryption;
		char m_multi_user[12];
		char m_MDX;
		char m_languageID;
		char m_reserved2[2];

		CTableField m_tableField;
		CDBFRecordArray m_records;

		CDBFAccessor* m_pAccessor;

		std::map<std::string, int>* m_pUniqueID;
		int m_uniqueIDTableFieldNo;
		//tmp string for unique ID

		std::string m_tmpStr;

	};
}//namespace WBSF