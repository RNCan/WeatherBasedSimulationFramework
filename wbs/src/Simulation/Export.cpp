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
#include <boost\dynamic_bitset.hpp>

#include "Basic/Dimension.h"
#include "Basic/Statistic.h"
#include "Simulation/Export.h"





using namespace std;
using namespace WBSF::DIMENSION;

namespace WBSF
{

	const char* CVariableDefine::XML_FLAG = "Variable";


	CVariableDefine::CVariableDefine(int dimension, int field/*, const std::string name*/)
	{
		m_dimension = dimension;
		m_field = field;
	}


	void CVariableDefine::Reset()
	{
		m_dimension = -1;
		m_field = -1;
	}

	CVariableDefine& CVariableDefine::operator=(const CVariableDefine& in)
	{
		if (&in != this)
		{
			m_dimension = in.m_dimension;
			m_field = in.m_field;
			//	m_currentName = in.m_currentName;
		}

		ASSERT(*this == in);

		return *this;
	}

	bool CVariableDefine::operator==(const CVariableDefine& in)const
	{
		bool bEqual = true;
		if (m_dimension != in.m_dimension) bEqual = false;
		if (m_field != in.m_field) bEqual = false;

		return bEqual;
	}



	//*********************************************************************************************************

	const char* CExport::MEMBER_NAME[NB_MEMBER] = { "FileName", "ScriptName", "AutoExport", "VariableArray", "Statistic" };

	CExport::CExport()
	{
		Reset();
	}

	CExport::CExport(const CExport& in)
	{
		operator=(in);
	}

	CExport::~CExport()
	{}

	typedef  boost::dynamic_bitset < size_t > CStatisticBitsetBase;
	class CStatisticBitset : public CStatisticBitsetBase
	{
	public:

		CStatisticBitset(size_t stat) : CStatisticBitsetBase(NB_STAT_TYPE)
		{
			assert(stat < NB_STAT_TYPE);

			set(stat);
		}

		CStatisticBitset(const string& in = "") : CStatisticBitsetBase(NB_STAT_TYPE)
		{
			if (!in.empty())
			{
				CStatisticBitsetBase bits(in);
				for (size_t s = 0; s < size() && s < bits.size(); s++)
					set(s, bits[s]);
			}
		}

	};

	void CExport::Reset()
	{
		m_fileName.clear();
		m_scriptName.clear();
		m_bAutoExport = false;
		m_variables.clear();
		m_statistic.clear();
		boost::to_string(CStatisticBitset(MEAN), m_statistic);
	}

	CExport& CExport::operator=(const CExport& in)
	{
		if (&in != this)
		{
			m_fileName = in.m_fileName;
			m_scriptName = in.m_scriptName;
			m_bAutoExport = in.m_bAutoExport;
			m_variables = in.m_variables;
			m_statistic = in.m_statistic;
		}


		return *this;
	}
	bool CExport::operator==(const CExport& in)const
	{
		bool bEqual = true;

		if (m_fileName != in.m_fileName)bEqual = false;
		if (m_scriptName != in.m_scriptName)bEqual = false;
		if (m_bAutoExport != in.m_bAutoExport)bEqual = false;
		if (m_variables != in.m_variables)bEqual = false;
		if (m_statistic != in.m_statistic)bEqual = false;


		return bEqual;
	}

}