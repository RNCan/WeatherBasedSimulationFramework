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

#include <memory>
#include "Basic/UtilStd.h"
#include "Simulation/SectionData.h"
#include "Simulation/DBBase.h"

namespace WBSF
{
	class CResultFormat
	{
	public:

		std::string m_locFormat;
		std::string m_timeFormat[CTRef::NB_MODE*CTRef::NB_REFERENCE];
		std::vector<char> m_variablePrecision;

		int GetPrecision(size_t col)const{ return col < DIMENSION::NB_DIMENSION ? 0 : int(m_variablePrecision[col - DIMENSION::NB_DIMENSION]); }
	};

	class CResult //or BioSIM database
	{
	public:

		enum TFile{ DATA, INFO, META, NB_FILE_TYPE };


		CResult();
		~CResult();


		void Reset();
		ERMsg Open(){ return Open(m_filePath, std::fstream::in | std::fstream::binary); }
		ERMsg Open(const std::string& DBFilePath, int mode);
		ERMsg AddSection(const CNewSectionData& data, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg AddSection(const CModelStatVector& section, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CSimulationPoint& section, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CNewSectionData& section, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg SetSection(size_t sectionNo, const CModelStatVector& section, CCallback& callback = DEFAULT_CALLBACK);
		void Close();
		bool IsOpen()const{ return m_database.IsOpen(); }


		size_t GetNbRows(size_t sectionNo = UNKNOWN_POS)const{ return m_database.GetNbRows(sectionNo); }
		size_t GetNbCols(bool bWithDim = true)const{ return m_database.GetNbCols() + (bWithDim ? DIMENSION::NB_DIMENSION : 0); }

		size_t GetCol(size_t dimension, size_t field);
		std::string GetDataValue(size_t nRow, size_t nCol, size_t stat)const;
		double GetDataValue(size_t section, size_t row, size_t dim, size_t field, size_t stat)const;
		double GetDataValue(size_t row, size_t dim, size_t field, size_t stat)const;
		const CStatistic& GetData(size_t nRow, size_t nCol)const;
		bool HaveData(size_t nRow)const;

		CTM GetTM()const{ return GetTPeriod().GetTM(); }
		CDimension GetDimension()const{ return GetMetadata().GetDimension(); }
		const CTPeriod& GetTPeriod()const{ return GetMetadata().GetTPeriod(); }
		CTRef GetFirstTRef(size_t sectionNo)const { return m_database.GetFirstTRef(sectionNo); }
		CTPeriod GetTPeriod(size_t sectionNo)const { return m_database.GetTPeriod(sectionNo); }


		void GetDataHead(StringVector& dataHead)const;


		const CBioSIMDatabase& GetDatabase()const{ return m_database; }

		CDBMetadata& GetMetadata(){ return m_database.GetMetadata(); }
		const CDBMetadata& GetMetadata()const { return m_database.GetMetadata(); }

		size_t GetNbSection()const{ return GetMetadata().GetNbSection(); }
		size_t GetSectionNo(size_t l, size_t p, size_t r)const{ return GetMetadata().GetSectionNo(l, p, r); }
		size_t GetSectionNo(size_t nRow)const{ return m_database.GetSectionNo(nRow); }
		void GetSection(size_t loc, size_t param, size_t rep, CNewSectionData& section)const{ GetSection(GetSectionNo(loc, param, rep), section); }
		void GetSection(size_t no, CNewSectionData& section)const;
		void GetSection(size_t no, CSimulationPoint& section, CWVariables filter = CWAllVariables())const;
		//int GetColWidth(size_t col)const;

		//Set the path, then user can open it without the name
		void SetFilePath(const std::string& filePath){ m_filePath = filePath; }
		void SetFormat(const CResultFormat& format);


		//CSectionInfo GetSectionInfo(size_t section)const;
		size_t GetSectionRow(size_t sectionNo, size_t globalRow)const{ ASSERT(globalRow - m_database.GetFirstRow(sectionNo) >= 0); return globalRow - m_database.GetFirstRow(sectionNo); }
		const CLocation& GetLocation(size_t sectionNo)const{ return GetMetadata().GetLocations().at(GetMetadata().GetLno(sectionNo)); }
		const CModelInput& GetParameterSet(size_t sectionNo)const{ return GetMetadata().GetParameterSet().at(GetMetadata().GetPno(sectionNo)); }
		size_t GetReplication(size_t sectionNo)const{ return GetMetadata().GetRno(sectionNo); }

		size_t GetNbField(size_t dimension)const;
		std::string GetFieldTitle(size_t d, size_t f, size_t stat)const;


		size_t GetNbSectionAdded()const{ return m_nbSectionAdded; }
		std::string GetFilePath()const { return GetFilePath(DATA); }


		static ERMsg Remove(std::string filePath);
	protected:

		std::string GetFilePath(int type)const;
		bool IsLoaded(size_t sectionNo)const;
		void LoadSection(size_t sectionNo)const;
		std::string GetDimensionString(size_t sectionNo, size_t row, size_t nCol)const;


		std::string m_filePath;
		CResultFormat m_format;

		CNewSectionDataPtr m_loadedSection;
		size_t m_lastSectionLoaded;

		CBioSIMDatabase m_database;
		size_t m_nbSectionAdded;//for parallel progress

		static const CLocationVector DEFAULT_LOC;
		static const CModelInputVector DEFAULT_PARAMETER_SET;
		static const CTPeriod DEFAULT_PERIOD;
	};


	typedef std::shared_ptr<CResult>CResultPtr;
	typedef std::vector<CResultPtr> CResultPtrVector;

}