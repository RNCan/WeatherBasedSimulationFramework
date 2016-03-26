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

#include <sstream>
#include <memory>
#include <boost/dynamic_bitset.hpp>

#include "Basic/Location.h"
#include "Basic/Statistic.h"
#include "Basic/Dimension.h"
#include "ModelBase/ModelInput.h"

namespace WBSF
{

	class CVariableSelection
	{
	public:

		enum TMember { OUTPUT_VAR_POS, MIN_VALID_VALUE, MAX_VALID_VALUE, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char*
			GetXMLFlag(){ return XML_FLAG; }

		CVariableSelection();
		CVariableSelection(size_t pos, double minValidValue = -VMISS, double maxValidValue = VMISS);
		CVariableSelection(const CVariableSelection& in);

		void Reset();
		CVariableSelection& operator =(const CVariableSelection& in);
		bool operator == (const CVariableSelection& in)const;
		bool operator != (const CVariableSelection& in)const{ return !operator==(in); }

		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//virtual void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }
		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);


		size_t m_pos;
		double m_minValidValue;
		double m_maxValidValue;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

		struct less_than_key
		{
			inline bool operator() (const CVariableSelection& struct1, const CVariableSelection& struct2)
			{
				return (struct1.m_pos < struct2.m_pos);
			}
		};

	};


	static const unsigned long BITS_ALL_SET = unsigned long(-1);
	typedef boost::dynamic_bitset<size_t> CVariableSelectionVector;
	typedef std::vector<CStatistic> CStatMatrixRow;
	typedef std::vector< std::vector<CStatistic> > CStatMatrix;

	class CNewSectionData : public CStatMatrix
	{
	public:

		enum TDataSize{ DATA_FLOAT, DATA_STATISTIC };


		CNewSectionData(size_t nYSize = 0, size_t nXSize = 0, CTRef firstTRef = CTRef());
		CNewSectionData(const CNewSectionData& matrix);
		void Reset();
		void ResetRow(size_t i);

		void resize(size_t nYSize, size_t nXSize, CTRef firstTRef);
		CNewSectionData& operator=(const CNewSectionData& in);
		bool operator==(const CNewSectionData& in)const;
		bool operator!=(const CNewSectionData& in)const{ return !operator==(in); }
		void AppendRows(const CNewSectionData& in);
		void AppendCols(const CNewSectionData& in);


		void CopyInfo(const CNewSectionData& in);
		size_t CreateLine(const CTRef& ref);


		CStatMatrixRow& operator[](size_t i);
		const CStatMatrixRow& operator[](size_t i)const;
		CStatMatrixRow& operator[](CTRef r){ return operator [](size_t(r - m_firstTRef)); }
		const CStatMatrixRow& operator[](CTRef r)const{ return operator [](size_t(r - m_firstTRef)); }

		void Add(CTRef r, const CStatMatrixRow& row);

		size_t GetXSize() const { return m_nbCols; }
		size_t GetYSize() const { return size(); }
		CTRef GetFirstTRef()const{ return m_firstTRef; }
		CTRef GetLastTRef()const{ return m_firstTRef.IsInit() ? m_firstTRef + GetYSize() - 1 : CTRef(); }//end Tref is -1 before the first element
		CTRef GetEndTRef()const{ return m_firstTRef.IsInit() ? m_firstTRef + GetYSize() : CTRef(); }//end Tref is +1 after the last element
		CTRef GetTRef(size_t i)const{ return m_firstTRef + i; }
		CTRef GetTRef(size_t row, size_t col, size_t stat = MEAN)const;
		void SetTRef(size_t row, size_t col, CTRef t);
		void AddTRef(size_t row, size_t col, CTRef t);
		CTPeriod GetTPeriod()const{ return CTPeriod(GetFirstTRef(), GetLastTRef()); }
		CDimension GetDimension()const;

		bool IsTemporalMatrix(size_t i)const{ ASSERT(m_dataTM.size() == m_nbCols); return (m_dataTM[i].IsInit() && (m_dataTM[i].Type() != CTM::ATEMPORAL)); }
		const std::vector<CTM>& GetDataTM()const{ return m_dataTM; }
		void SetDataTM(const std::vector<CTM>& TM){ m_dataTM = TM; }

		bool IsInside(CTRef r)const{ size_t i = r - m_firstTRef; return i >= 0 && i < GetYSize(); }
		bool HaveData(size_t i)const;
		bool HaveData(CTRef r)const{ return HaveData(size_t(r - m_firstTRef)); }

		void CleanUp(const CTPeriod& period);
		void CleanUp(const CVariableSelectionVector& variables);

		ERMsg ReadStream(std::istream& inStream);

		size_t m_locPos;
		size_t m_paramPos;
		size_t m_repPos;
		size_t m_globalFirstRow;
		size_t GetFirstRow()const{ return m_globalFirstRow; }
		size_t GetNexRow()const{ return m_globalFirstRow + GetYSize(); }

		CTPeriod m_overAllPeriod;

		size_t GetCols()const{ return m_nbCols; }
		size_t GetRows()const{ return size(); }
	protected:

		size_t m_nbCols;
		CTRef m_firstTRef;
		short m_dataType;//float : for simulation data, CStatistic otherwise
		std::vector<CTM> m_dataTM; //TypeMode of the variable dimension (not the time dimension)

		CStatMatrixRow EMPTY_STAT;
	};

	typedef std::shared_ptr<CNewSectionData>CNewSectionDataPtr;
	typedef std::vector<CNewSectionDataPtr>CNewSectionDataPtrVector;

}