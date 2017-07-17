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
#include "StdAfx.h"
#include "Simulation/SectionData.h"


using namespace WBSF::DIMENSION;
using namespace std;

namespace WBSF
{

	//*******************************************************************
	const char* CVariableSelection::XML_FLAG = "VariableSelection";
	const char* CVariableSelection::MEMBER_NAME[NB_MEMBER] = { "Pos", "MinValidValue", "MaxValidValue" };
	CVariableSelection::CVariableSelection()
	{
		Reset();
	}

	CVariableSelection::CVariableSelection(size_t pos, double minValidValue, double maxValidValue)
	{
		m_pos = pos;
		m_minValidValue = minValidValue;
		m_maxValidValue = maxValidValue;
	}

	CVariableSelection::CVariableSelection(const CVariableSelection& in)
	{
		operator=(in);
	}

	void CVariableSelection::Reset()
	{
		m_pos = 0;
		m_minValidValue = -VMISS;
		m_maxValidValue = VMISS;

	}

	CVariableSelection& CVariableSelection::operator =(const CVariableSelection& in)
	{
		if (&in != this)
		{
			m_pos = in.m_pos;
			//m_outputVarName.empty();
			m_minValidValue = in.m_minValidValue;
			m_maxValidValue = in.m_maxValidValue;
		}

		ASSERT(operator==(in));
		return *this;

	}

	bool CVariableSelection::operator == (const CVariableSelection& in)const
	{
		bool bEqual = true;

		if (m_pos != in.m_pos) bEqual = false;
		if (fabs(m_minValidValue - in.m_minValidValue) > 0.00001) bEqual = false;
		if (fabs(m_maxValidValue - in.m_maxValidValue) > 0.00001) bEqual = false;

		return bEqual;
	}





	std::string CVariableSelection::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		std::string str;
		switch (i)
		{
		case OUTPUT_VAR_POS:	str = ToString(m_pos); break;
			//	case OUTPUT_VAR_NAME:	str = m_outputVarName; break;
		case MIN_VALID_VALUE:	str = ToString(m_minValidValue); break;
		case MAX_VALID_VALUE:	str = ToString(m_maxValidValue); break;
		default:ASSERT(false);
		}

		return str;
	}

	void CVariableSelection::SetMember(size_t i, const std::string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);
		switch (i)
		{
		case OUTPUT_VAR_POS:	m_pos = ToShort(str); break;
		case MIN_VALID_VALUE:	m_minValidValue = ToDouble(str); break;
		case MAX_VALID_VALUE:	m_maxValidValue = ToDouble(str); break;
		default:ASSERT(false);
		}
	}

	//**********************************************************************
	//**********************************************************************

	CNewSectionData::CNewSectionData(size_t nYSize, size_t nXSize, CTRef firstTRef)
	{
		Reset();
		resize(nYSize, nXSize, firstTRef);
	}

	CNewSectionData::CNewSectionData(const CNewSectionData& in)
	{
		operator=(in);
	}

	void CNewSectionData::Reset()
	{
		clear();

		m_locPos = -1;
		m_paramPos = -1;
		m_repPos = -1;

		m_firstTRef.Reset();
		m_dataType = DATA_FLOAT;
		m_dataTM.clear();

		m_overAllPeriod.Reset();
		//m_bPropagateFirstRow=false;
		//m_bPropagateLastRow=false;
	}

	void CNewSectionData::resize(size_t nYSize, size_t nXSize, CTRef firstTRef)
	{
		CStatMatrix::resize(nYSize);
		for (size_t i = 0; i<size(); i++)
			at(i).resize(nXSize);

		m_dataTM.resize(nXSize);
		m_nbCols = nXSize;
		m_firstTRef = firstTRef;
	}

	void CNewSectionData::Add(CTRef r, const CStatMatrixRow& row)
	{
		ASSERT(GetYSize() == 0 || m_firstTRef.IsInit());

		if (!m_firstTRef.IsInit())
			m_firstTRef = r;

		__int64 index = r - m_firstTRef;
		if (index >= 0)
		{
			if (index>(__int64)size())
				insert(end(), index - size(), CStatMatrixRow(row.size()));

			insert(begin() + index, row);
		}
		else
		{
			CStatMatrixRow emptyRow(row.size());
			insert(begin(), -index, emptyRow);
			m_firstTRef = r;
			at(0) = row;
		}
	}

	CNewSectionData& CNewSectionData::operator=(const CNewSectionData& in)
	{
		if (&in != this)
		{
			CStatMatrix::operator=(in);
			CopyInfo(in);
		}

		return *this;
	}

	void CNewSectionData::CopyInfo(const CNewSectionData& in)
	{
		if (this != &in)
		{
			m_locPos = in.m_locPos;
			m_paramPos = in.m_paramPos;
			m_repPos = in.m_repPos;
			m_overAllPeriod = in.m_overAllPeriod;
			m_firstTRef = in.m_firstTRef;
			m_dataType = in.m_dataType;
			m_dataTM = in.m_dataTM;
			m_nbCols = in.m_nbCols;
		}
	}

	CStatMatrixRow& CNewSectionData::operator[](size_t i)
	{
		ASSERT(EMPTY_STAT.size() == 0 || EMPTY_STAT[0][NB_VALUE] == 0);

		if (i >= GetYSize())
		{
			if (EMPTY_STAT.size() != GetXSize())
				EMPTY_STAT.resize(GetXSize());


			return EMPTY_STAT;
		}


		return CStatMatrix::operator [](i);
	}

	const CStatMatrixRow& CNewSectionData::operator[](size_t i)const
	{
		ASSERT(EMPTY_STAT.size() == 0 || EMPTY_STAT[0][NB_VALUE] == 0);

		if (i >= GetYSize())
		{
			if (EMPTY_STAT.size() != GetXSize())
			{
				CNewSectionData& me = const_cast<CNewSectionData&>(*this);
				me.EMPTY_STAT.resize(GetXSize());
			}

			return EMPTY_STAT;
		}


		return CStatMatrix::operator [](i);
	}

	//Append rowns
	void CNewSectionData::AppendRows(const CNewSectionData& in)
	{
		ASSERT(size() == 0 || in.empty() || in.front().size() == m_nbCols);

		CNewSectionData& me = *this;

		if (GetYSize() == 0)
		{
			me = in;
			m_dataTM = in.m_dataTM;
			
			return;
		}

		for (size_t i = 0; i < in.size(); i++)
		{
			const CTRef& TRef = in.GetTRef(i);
			if (TRef < GetFirstTRef() || TRef > GetLastTRef())
			{
				//Add a new line
				Add(TRef, in[i]);
			}
			else
			{
				ASSERT(in[i].size() == m_nbCols);
				//add to current matrix line
				for (size_t j = 0; j < in[i].size(); j++)
				{
					if (in.IsTemporalMatrix(j))
					{
						AddTRef(i, j, in.GetTRef(i, j));
					}
					else
					{
						me[TRef][j] += in[i][j];
					}
				}
			}
		}

		ASSERT(in.empty() || m_nbCols == in.front().size());
		ASSERT(size() == 0 || me[0].size() == in[0].size());
	}

	void CNewSectionData::AppendCols(const CNewSectionData& in)
	{
		ASSERT(empty() || in.empty() || in.size() == size());

		CNewSectionData& me = *this;
		

		if (GetXSize() == 0)
		{
			me = in;
			m_dataTM = in.m_dataTM;
		}
		else
		{
			ASSERT(in.GetFirstTRef() == GetFirstTRef());
			ASSERT(in.GetLastTRef() == GetLastTRef());
			//ASSERT( in.IsTemporalMatrix() == IsTemporalMatrix());

			for (size_t i = 0; i < size(); i++)
			{
				me[i].insert(me[i].end(), in[i].begin(), in[i].end());
				ASSERT(me[i].size() == me[0].size());
			}

			//adjust number of cols
			if (size() > 0)
				m_nbCols = me[0].size();
			
			m_dataTM.insert(m_dataTM.end(), in.GetDataTM().begin(), in.GetDataTM().end());
			
			
			ASSERT(m_dataTM.size() == m_nbCols);
		}

		ASSERT(size() == in.size());
	}

	CDimension CNewSectionData::GetDimension()const
	{
		CDimension dim;
		dim[LOCATION] = 1;
		dim[PARAMETER] = 1;
		dim[REPLICATION] = 1;
		dim[TIME_REF] = GetYSize();
		dim[VARIABLE] = GetXSize();

		return dim;
	}

	bool CNewSectionData::HaveData(size_t i)const
	{
		ASSERT(i >= 0 && i < GetYSize());

		bool bHaveData = false;
		const CNewSectionData& me = *this;
		
		for (size_t i = 0; i < GetYSize() && !bHaveData; i++)
		{
			if (me[i][0][NB_VALUE] > 0)
				bHaveData = true;
		}

		return bHaveData;
	}

	bool CNewSectionData::HaveData()const
	{
		bool bHaveData = false;

		const CNewSectionData& me = *this;
		for (size_t i = 0; i < GetXSize() && !bHaveData; i++)
			if (HaveData(i))
				bHaveData = true;

		return bHaveData;
	}

	//Read float stream from BioSIMModelBase
	ERMsg CNewSectionData::ReadStream(istream& stream)
	{
		ERMsg msg;

		CNewSectionData& me = *this;

		__int32 version = -1;
		unsigned __int64 rawSize = 0;
		unsigned __int64 nbRows = 0;
		unsigned __int64 nbCols = 0;
		unsigned __int64 realSize = 0;
		unsigned __int64 nbStat = 0;
		unsigned __int32 Tref = 0;


		stream.read((char*)(&version), sizeof(version));
		stream.read((char*)(&rawSize), sizeof(rawSize));
		stream.read((char*)(&nbRows), sizeof(nbRows));
		stream.read((char*)(&nbCols), sizeof(nbCols));
		stream.read((char*)(&realSize), sizeof(realSize));
		stream.read((char*)(&nbStat), sizeof(nbStat));
		stream.read((char*)(&Tref), sizeof(Tref));



		ASSERT(version == 1);

		CTRef firstTRef;
		firstTRef.Set__int32(Tref);
		resize(nbRows, nbCols, firstTRef);
		//m_firstTRef = ;

		for (size_t i = 0; i < nbRows; i++)
		{
			ASSERT(nbCols > 1);

			for (size_t j = 0; j < nbCols; j++)
			{
				float value = 0;
				stream.read((char*)(&value), sizeof(value));
				me[i][j] = value;
			}
		}

		return msg;
	}



	CTRef CNewSectionData::GetTRef(size_t row, size_t col, size_t stat)const
	{
		ASSERT(stat == MEAN || stat == LOWEST || stat == HIGHEST);

		const CNewSectionData& me = *this;

		CTRef t;
		double statValue = me[row][col][stat];
		if (statValue != CStatistic::GetVMiss())
			t.SetRef(long(statValue), m_dataTM[col]);
			//t.Set__int32(statValue);

		return t;
	}

	void CNewSectionData::SetTRef(size_t row, size_t col, CTRef t)
	{
		if (t.IsInit())
		{
			ASSERT(!m_dataTM[col].IsInit() || t.GetTM() == m_dataTM[col]);
			CNewSectionData& me = *this;

			m_dataTM[col] = t.GetTM();
			me[row][col] = t.GetRef();
			//__int32 tmp = t.Get__int32();
			//me[row][col] = tmp;
		}
	}

	void CNewSectionData::AddTRef(size_t row, size_t col, CTRef t)
	{
		ASSERT(t.IsInit());

		if (t.IsInit())
		{
			CNewSectionData& me = *this;
			ASSERT(me[row][col][NB_VALUE] == 0 || t.GetTM() == m_dataTM[col]);

			if (me[row][col][NB_VALUE] == 0)
				m_dataTM[col] = t.GetTM();

			me[row][col] += t.GetRef();//est-ce qu'on peut additionner des références???
			//me[row][col] += t.Get__int32();
		}
	}

	void CNewSectionData::ResetRow(size_t i)
	{
		CNewSectionData& me = *this;

		for (size_t j = 0; j < me[i].size(); j++)
			me[i][j].Reset();

		ASSERT(me[i].size() == m_nbCols);
	}

	void CNewSectionData::CleanUp(const CTPeriod& period)
	{
		if (!period.IsInit())
			return;

		if (!period.IsIntersect(GetTPeriod()))
		{
			//remove all row and exit...
		}

		//resize the row 
		__int64 nbLines = min(__int64(size()), __int64(period.Begin() - GetFirstTRef()));
		if (nbLines > 0)
		{
			//for(size_t i=0; i<nbLines; i++)
			erase(begin(), begin() + nbLines);

			m_firstTRef = period.Begin();
		}

		nbLines = min(__int64(size()), __int64(GetLastTRef() - period.End()));
		if (nbLines > 0)
		{
			size_t linePos = (size_t)max(0, period.End() - GetFirstTRef() + 1);
			erase(begin() + linePos, begin() + linePos + nbLines);
		}


		CTRef ref = GetFirstTRef();
		for (size_t r = 0; r < size(); r++, ref++)
		{
			if (!period.IsInside(ref))
			{
				ResetRow(r);
			}
		}
	}

	void CNewSectionData::CleanUp(const CVariableSelectionVector& variables)
	{
		if (variables.count() == variables.size())
			return;

		for (iterator it1 = begin(); it1 != end(); it1++)
		{
			int i = 0;
			for (CStatMatrixRow::iterator it2 = it1->begin(); it2 != it1->end(); i++)
			{
				if (variables[i])
					it2++;
				else
					it2 = it1->erase(it2);
			}
		}


		//clean DataTM
		int i = 0;
		for (vector<CTM>::iterator it = m_dataTM.begin(); it != m_dataTM.end(); i++)
		{
			if (variables[i])
				it++;
			else
				it = m_dataTM.erase(it);
		}

		m_nbCols = m_dataTM.size();
		assert(m_dataTM.size() == m_nbCols);
	}
}