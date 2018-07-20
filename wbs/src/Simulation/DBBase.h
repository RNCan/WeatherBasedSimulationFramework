//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include <deque>
#include "Basic/WeatherStation.h"
#include "ModelBase/WGInput.h"
#include "ModelBase/ModelOutputVariable.h"
#include "Simulation/SectionData.h"

namespace WBSF
{
	class CModelStatVector;
	class CModel;

	class CDBMetadata
	{
	public:

		CDBMetadata();
		CDBMetadata(const CDBMetadata& in);
		~CDBMetadata();

		void Reset(){ clear(); }
		void clear();
		CDBMetadata& operator=(const CDBMetadata& in);//to be used by the Database itself

		ERMsg Open(const std::string& DBFilePath, int mode);
		bool IsOpen()const{ return m_openMode != -1; }
		void AddSection(const CNewSectionData& data);
		void AddSection(const CModelStatVector& section);
		void AddSection(const CSimulationPoint& section);
		void Close();

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_lastRunTime & m_dimension & m_period & m_modelName & m_variablesDefinition & m_locations & m_parameterSet;
		}


		size_t GetNbSection()const;
		size_t GetSectionNo(size_t loc, size_t param, size_t iter)const;
		size_t GetLno(size_t sectionNo)const;
		size_t GetPno(size_t sectionNo)const;
		size_t GetRno(size_t sectionNo)const;


		const CTPeriod& GetTPeriod()const{ return m_period; }
		void SetTPeriod(const CTPeriod& in){ m_period = in; m_dimension[DIMENSION::TIME_REF] = m_period.GetNbRef();}

		const CDimension& GetDimension()const{ return m_dimension; }

		std::vector<CTM> GetDataTM()const;

		const CLocationVector& GetLocations()const { return m_locations; }
		void SetLocations(const CLocationVector& in){ m_locations = in; m_dimension[DIMENSION::LOCATION] = in.size(); }

		const CModelInputVector& GetParameterSet()const{ return m_parameterSet; }
		void SetParameterSet(const CModelInputVector& in){ m_parameterSet = in; m_dimension[DIMENSION::PARAMETER] = in.size(); }
		size_t GetNbReplications()const{ return m_dimension[DIMENSION::REPLICATION]; }
		void SetNbReplications(size_t in){ m_dimension[DIMENSION::REPLICATION] = in; }

		const std::string& GetModelName()const { return m_modelName; }
		void SetModelName(const std::string& in){ m_modelName = in; }
		const CModelOutputVariableDefVector& GetOutputDefinition()const{ return m_variablesDefinition; }
		void SetOutputDefinition(const CModelOutputVariableDefVector& in){ m_variablesDefinition = in; m_dimension[DIMENSION::VARIABLE] = in.size(); }

		__time64_t GetLastRunTime()const{return m_lastRunTime;}

	protected:

		__time64_t m_lastRunTime;

		CTPeriod m_period;
		CDimension m_dimension;
		std::string m_modelName;
		CModelOutputVariableDefVector m_variablesDefinition;
		CLocationVector m_locations;
		CModelInputVector m_parameterSet;

		int m_openMode;
		std::string m_filePath;

		CCriticalSection m_CS;
	};


	class CDBSectionIndex
	{
	public:

		static const size_t SIZE;

		CDBSectionIndex(ULONGLONG beginPos = UNKNOWN_POS, size_t firstRow = 0, size_t nbRows = 0, CTRef TRef = CTRef(), bool bHaveData=false);
		CDBSectionIndex(const CDBSectionIndex& in);
		~CDBSectionIndex();

		void Reset(){ clear(); }
		void clear();
		CDBSectionIndex& operator=(const CDBSectionIndex& in);


		size_t GetFirstRow()const { return (size_t)m_firstRow; }
		void SetFirstRow(size_t in){ m_firstRow = (size_t)in; }
		size_t GetNextRow()const { return (size_t)(m_firstRow + m_nbRows); }
		size_t GetNbRows()const { return (size_t)m_nbRows; }
		void SetNbRows(size_t nbRows){ m_nbRows = nbRows; }
		void AddRows(){ m_nbRows++; }
		bool HaveData()const{return m_bHaveData;}
		const ULONGLONG& GetBeginPos()const { return m_beginPos; }
		void SetBeginPos(const ULONGLONG& pos){ m_beginPos = pos; }

		CTRef GetTRef()const { return m_TRef; }
		void SetTRef(CTRef TRef){ m_TRef = TRef; }
		CTRef GetLastTRef()const { ASSERT(m_nbRows > 0); return m_TRef + size_t(m_nbRows) - 1; }


		std::istream& operator << (std::istream& s)
		{
			WBSF::read_value(s, m_firstRow);
			WBSF::read_value(s, m_nbRows);
			WBSF::read_value(s, m_beginPos);
			int v = 0; WBSF::read_value(s, v); m_TRef.Set__int32(v);
			WBSF::read_value(s, m_bHaveData);
			
			return s;
		}
		std::ostream& operator >> (std::ostream& s)const
		{
			WBSF::write_value(s, m_firstRow);
			WBSF::write_value(s, m_nbRows);
			WBSF::write_value(s, m_beginPos);
			WBSF::write_value(s, m_TRef.Get__int32());
			WBSF::write_value(s, m_bHaveData);
			return s;
		}
		friend std::istream& operator >>(std::istream& s, CDBSectionIndex& p){ p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CDBSectionIndex& p){ p >> s; return s; }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_firstRow & m_nbRows & m_beginPos & m_TRef & m_bHaveData;
		}

		bool IsInit()const{ return m_beginPos != UNKNOWN_POS; }


	protected:

		unsigned __int64 m_firstRow;
		unsigned __int64 m_nbRows;
		ULONGLONG m_beginPos;
		CTRef m_TRef;
		bool m_bHaveData;
	};



	typedef std::deque<CDBSectionIndex> CDBSectionIndexVectorBase;


	class CDBSectionIndexVector : protected CDBSectionIndexVectorBase
	{
	public:


		CDBSectionIndexVector();
		~CDBSectionIndexVector();
		void clear();

		ERMsg Open(const std::string& filePath, int mode);
		void Close();


		bool IsOpen()const{ return m_file.is_open(); }

		ERMsg SetSection(size_t no, const CDBSectionIndex& index);
		const CDBSectionIndex& GetSection(size_t no)const { return at(no); }

		size_t GetNbRows(size_t section = UNKNOWN_POS)const;
		size_t GetSectionNo(size_t row)const;
		size_t GetFirstRow(size_t section)const;

		size_t size()const{ return CDBSectionIndexVectorBase::size(); }
		void push_back(const CDBSectionIndex& sec){ CDBSectionIndexVectorBase::push_back(sec); }
		CDBSectionIndexVectorBase::iterator insert(size_t no, const CDBSectionIndex& in)
		{
			if (no >= size())
				resize(no + 1);
			CDBSectionIndexVectorBase::at(no) = in;
			return CDBSectionIndexVectorBase::begin() + no;
		}

		const CDBSectionIndex& at(size_t no)const{ load(no); return CDBSectionIndexVectorBase::at(no); }
		const CDBSectionIndex& operator[](size_t no)const{ return at(no); }


	protected:

		ULONGLONG get_pos(size_t no)const{ return no*CDBSectionIndex::SIZE + sizeof(__int64); }

		void load(size_t no)const;

		fStream m_file;
		int m_openMode;
	};


	class CBioSIMDatabase
	{
	public:

		enum TDataSize{ UNKNOWN = -1, DATA_FLOAT, DATA_STATISTIC, NB_DATA_TYPE };
		static std::string GetMetadataFilePath(std::string filePath);
		static std::string GetIndexFilePath(std::string filePath);

		CBioSIMDatabase(/*short type=UNKNOWN*/);
		~CBioSIMDatabase();

		size_t GetType()const{ return (size_t)m_type; }
		void SetType(size_t type){ m_type = type; }

		ERMsg Open(const std::string& filePath, int mode);
		void Close();
		static ERMsg Remove(std::string filePath);

		ERMsg AddSection(const CNewSectionData& section, int type = DATA_STATISTIC, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg AddSection(const CModelStatVector& section, int type = DATA_FLOAT, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CNewSectionData& section, int type = DATA_STATISTIC, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CModelStatVector& section, int type = DATA_FLOAT, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CSimulationPoint& section, CCallback& callback = DEFAULT_CALLBACK);
		void GetSection(size_t no, CNewSectionData& section)const;
		void GetSection(size_t no, CSimulationPoint& section, CWVariables filter = CWAllVariables())const;

		bool IsOpen()const{ return m_file.is_open(); }
		size_t GetNbCols()const{ return (size_t)m_nbCols; }

		size_t GetNbRows(size_t section = UNKNOWN_POS)const{ return section == UNKNOWN_POS ? (size_t)m_nbRows : m_index[section].GetNbRows(); }
		size_t GetFirstRow(size_t section)const{ return m_index[section].GetFirstRow(); }
		size_t GetNextRow(size_t section)const{ return m_index[section].GetFirstRow() + m_index[section].GetNbRows(); }
		CTRef GetFirstTRef(size_t section)const{ return m_index[section].GetTRef(); }
		CTPeriod GetTPeriod(size_t section)const{ return CTPeriod(m_index[section].GetTRef(), m_index[section].GetLastTRef()); }

		size_t GetNbSection()const{ return size_t(m_index.size()); }
		size_t GetSectionNo(size_t nRow)const{ return m_index.GetSectionNo(nRow); }

		const CDBSectionIndexVector& GetIndex()const{ return m_index; }
		CDBMetadata& GetMetadata(){ return m_metadata; }
		const CDBMetadata& GetMetadata()const{ return m_metadata; }

		std::istream& operator << (std::istream& s)
		{
			WBSF::read_value(s, m_nbCols);
			WBSF::read_value(s, m_nbRows);
			WBSF::read_value(s, m_type);
			return s;
		}
		std::ostream& operator >> (std::ostream& s)const
		{
			WBSF::write_value(s, m_nbCols);
			WBSF::write_value(s, m_nbRows);
			WBSF::write_value(s, m_type);
			return s;
		}
		friend std::istream& operator >>(std::istream& s, CBioSIMDatabase& p){ p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CBioSIMDatabase& p){ p >> s; return s; }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_nbCols & m_nbRows & m_type;
		}

	protected:

		CBioSIMDatabase(const CBioSIMDatabase& in);
		CBioSIMDatabase operator=(const CBioSIMDatabase& in);
		int GetTypeSize()const{ return m_type == DATA_STATISTIC ? sizeof(CStatistic) : sizeof(float); }

		ULONGLONG GetDataSize() const{ return m_nbRows*m_nbCols*GetTypeSize() + sizeof(m_nbRows) + sizeof(m_nbCols) + sizeof(m_type); }
		ULONGLONG GetFilePos(size_t section, size_t row, size_t col)const;
		
		int m_openMode;
		unsigned __int64 m_nbCols;
		unsigned __int64 m_nbRows;
		unsigned __int64 m_type;
		
		CDBMetadata m_metadata;
		CDBSectionIndexVector m_index;
		fStream m_file;

		CCriticalSection m_CS;
	};

}