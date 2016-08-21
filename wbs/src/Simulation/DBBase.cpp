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
#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost\dynamic_bitset.hpp>
#include <boost\serialization\map.hpp>
#include <boost\serialization\array.hpp>
#include <boost\serialization\vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "Basic/ModelStat.h"
#include "Basic/WeatherDefine.h"
#include "Simulation/DBBase.h"


#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::DIMENSION;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	//****************************************************************************************************
	//CDBSectionIndex

	CDBMetadata::CDBMetadata()
	{
		clear();
	}

	CDBMetadata::CDBMetadata(const CDBMetadata& in)
	{
		operator=(in);
	}

	CDBMetadata::~CDBMetadata()
	{
		//if (IsOpen())
		//Close();
	}

	void CDBMetadata::clear()
	{

		m_period.clear();
		m_dimension.Reset();
		//	m_dataTM.clear();
		m_lastRunTime = 0;

		m_filePath.clear();
		m_openMode = -1;

		m_locations.clear();
		m_parameterSet.clear();
		m_parameterSet.push_back(CModelInput());
		m_modelName.clear();
		m_variablesDefinition.clear();
	}

	CDBMetadata& CDBMetadata::operator=(const CDBMetadata& in)
	{
		if (&in != this)
		{
			m_period = in.m_period;
			m_dimension = in.m_dimension;
			m_lastRunTime = in.m_lastRunTime;
			m_locations = in.m_locations;
			m_parameterSet = in.m_parameterSet;
			m_modelName = in.m_modelName;
			m_variablesDefinition = in.m_variablesDefinition;
		}

		return *this;
	}

	size_t CDBMetadata::GetNbSection()const
	{
		return m_dimension[LOCATION] * m_dimension[PARAMETER] * m_dimension[REPLICATION];
	}

	size_t CDBMetadata::GetSectionNo(size_t loc, size_t param, size_t iter)const
	{
		return loc*(m_dimension[REPLICATION] * m_dimension[PARAMETER]) + param*m_dimension[REPLICATION] + iter;
	}

	size_t CDBMetadata::GetLno(size_t section)const
	{
		size_t e = size_t(section / (m_dimension[REPLICATION] * m_dimension[PARAMETER]));
		ASSERT(e >= 0 && e < m_dimension[LOCATION]);
		return e;
	}

	size_t CDBMetadata::GetPno(size_t section)const
	{
		size_t e = size_t(section / m_dimension[REPLICATION]);
		return size_t(e%m_dimension[PARAMETER]);
	}
	size_t CDBMetadata::GetRno(size_t section)const
	{
		return size_t(section%m_dimension[REPLICATION]);
	}

	ERMsg CDBMetadata::Open(const string& filePath, int mode)
	{
		ASSERT(m_openMode = -1);

		ERMsg msg;

		Reset();

		mode |= std::fstream::binary;
		if (mode & std::fstream::out)
			mode |= std::fstream::trunc;


		if (mode & std::fstream::in)
		{
			//create the file
			ifStream file;
			msg = file.open(filePath, mode);
			if (msg)
			{
				m_filePath = filePath;
				m_openMode = mode;

				//load the file
				try
				{
					boost::archive::binary_iarchive ar(file);// , boost::archive::no_header);
					ar >> (*this);
				}
				catch (...)
				{
					//do nothing and recreate optimization
					int k;
					k = 0;
					msg.ajoute("Unable to open Metadata");
				}

				file.close();
			}

		}
		else
		{
			//just create the file to see if they have problem
			ofStream file;
			msg = file.open(filePath, mode);
			if (msg)
			{
				m_filePath = filePath;
				m_openMode = mode;
			}

			file.close();
		}

		return msg;
	}

	void CDBMetadata::Close()
	{
		// msg;
		if (m_openMode != -1)
		{
			//update size and index
			if (m_openMode & std::fstream::out)
			{
				time(&m_lastRunTime);

				ofStream file;
				if (file.open(m_filePath, m_openMode))
				{
					//load the file
					try
					{
						boost::archive::binary_oarchive ar(file);// , boost::archive::no_header);
						ar << (*this);
					}
					catch (...)
					{
						int k;
						k = 0;
					}

					file.close();
				}
			}

			m_openMode = -1;
		}

		ASSERT(m_openMode = -1);

#ifdef _DEBUG

		CDBMetadata test;
		test.Open(m_filePath, std::fstream::in);
		ASSERT(test.m_dimension == m_dimension);
#endif


		//return msg;
	}

	//update period
	void CDBMetadata::AddSection(const CNewSectionData& section)
	{
		m_CS.Enter();

		m_period.Inflate(section.GetTPeriod());
		m_dimension[TIME_REF] = m_period.GetNbRef();//update temporal dimension
		//if( section.IsTemporalMatrix() )
		//SetDataTM(section.GetDataTM());

		ASSERT(section.GetFirstTRef().GetTM() == m_period.GetTM());
		m_CS.Leave();
	}

	void CDBMetadata::AddSection(const CModelStatVector& section)
	{
		m_CS.Enter();
		m_period.Inflate(section.GetTPeriod());
		m_dimension[TIME_REF] = m_period.GetNbRef();//update temporal dimension

		//vector<CTM> dataTM;
		//for (CModelOutputVariableDefVector::const_iterator it = m_variablesDefinition.begin(); it != m_variablesDefinition.end(); it++)
		//dataTM.push_back(it->m_TM);

		//SetDataTM(dataTM);
		m_CS.Leave();

		ASSERT(section.GetFirstTRef().GetTM() == m_period.GetTM());
	}

	void CDBMetadata::AddSection(const CSimulationPoint& section)
	{
		m_CS.Enter();
		m_period.Inflate(section.GetEntireTPeriod());
		m_dimension[TIME_REF] = m_period.GetNbRef();//update temporal dimension
		//SetDataTM(vector<CTM>(m_variablesDefinition.size()));//Section added from weather always have TM==-1
		m_CS.Leave();


		ASSERT(section.GetTM() == m_period.GetTM());
	}

	std::vector<CTM> CDBMetadata::GetDataTM()const
	{
		vector<CTM> dataTM;
		dataTM.reserve(m_variablesDefinition.size());
		for (CModelOutputVariableDefVector::const_iterator it = m_variablesDefinition.begin(); it != m_variablesDefinition.end(); it++)
			dataTM.push_back(it->m_TM);

		return 	dataTM;
	}

	//****************************************************************************************************
	//CDBS	ectionIndex
	const size_t CDBSectionIndex::SIZE = sizeof(__int64) * 2 + sizeof(LONGLONG) + sizeof(CTRef);

	CDBSectionIndex::CDBSectionIndex(ULONGLONG beginPos, size_t firstRow, size_t rows, CTRef TRef)
	{
		m_beginPos = beginPos;
		m_firstRow = firstRow;
		m_nbRows = rows;
		m_TRef = TRef;
	}

	CDBSectionIndex::CDBSectionIndex(const CDBSectionIndex& in)
	{
		operator=(in);
	}

	CDBSectionIndex& CDBSectionIndex::operator=(const CDBSectionIndex& in)
	{
		if (&in != this)
		{
			m_firstRow = in.m_firstRow;
			m_nbRows = in.m_nbRows;
			m_beginPos = in.m_beginPos;
			m_TRef = in.m_TRef;
		}

		return *this;
	}

	CDBSectionIndex::~CDBSectionIndex()
	{
	}

	void CDBSectionIndex::clear()
	{
		m_firstRow = 0;
		m_nbRows = 0;
		m_beginPos = UNKNOWN_POS;
		m_TRef.Reset();
	}


	CDBSectionIndexVector::CDBSectionIndexVector()
	{
		clear();
	}

	CDBSectionIndexVector::~CDBSectionIndexVector()
	{
		//if (IsOpen())
		//Close();
	}

	void CDBSectionIndexVector::clear()
	{
		assert(!IsOpen());
		m_openMode = -1;
	}

	ERMsg CDBSectionIndexVector::Open(const std::string& filePath, int mode)
	{
		ASSERT(m_openMode = -1);

		ERMsg msg;
		msg = m_file.open(filePath, mode, SH_DENYWR);

		if (msg)
		{
			if (mode & std::fstream::in)
			{
				if (m_file.lengthg() > 8)
				{
					unsigned __int64 nbSection = 0;
					m_file.read_value(nbSection);
					resize(nbSection);

					if (nbSection > 0 && m_file.lengthg() == size()*CDBSectionIndex::SIZE + sizeof(__int64))
					{
						m_openMode = mode;
					}
					else
					{
						//bad file
						m_file.close();
						msg.ajoute("Error: zero size database. The simulation have probably finish before the end of simulation");
					}
				}
				else
				{
					//bad file
					m_file.close();
					msg.ajoute("Error: empty database");
				}
			}
			else
			{
				assert(m_file.lengthp() == 0);
				m_openMode = mode;
			}
		}


		return msg;
	}

	void CDBSectionIndexVector::Close()
	{
		if (m_openMode != -1)
		{
			//update size and index
			if (m_openMode & std::fstream::out)
			{
				ASSERT(m_file.lengthp() == 0);
				unsigned __int64 nbSections = size();

				m_file.write_value(nbSections);

				//Update rows position
				size_t row = 0;
				for (iterator it = begin(); it != end(); it++)
				{
					it->SetFirstRow(row);
					row += it->GetNbRows();
					(*it) >> m_file;
				}
			}

			ULONGLONG length = m_file.lengthp();
			ASSERT(m_file.lengthp() == size()*CDBSectionIndex::SIZE + sizeof(__int64));
			m_file.close();
			m_openMode = -1;
		}

		ASSERT(m_openMode = -1);
	}



	size_t CDBSectionIndexVector::GetNbRows(size_t section)const
	{
		ASSERT(section >= 0 && section < size());
		ASSERT(size()>0);

		return at(section).GetNbRows();
	}

	size_t CDBSectionIndexVector::GetSectionNo(size_t nRow)const
	{
		INT_PTR nLow = 0, nHigh = size(), nMid;

		while (nLow < nHigh)
		{
			nMid = (nLow + nHigh) / 2;
			if (at(nMid).GetNextRow() <= nRow)
			{
				nLow = nMid + 1;
			}
			else
			{
				nHigh = nMid;
			}
		}

		ASSERT(size() == 0 || (nRow >= at(nLow).GetFirstRow() && nRow < at(nLow).GetNextRow()));


		return size_t(nLow);
	}

	size_t CDBSectionIndexVector::GetFirstRow(size_t section)const
	{
		ASSERT(section >= 0 && section < size());
		return at(section).GetFirstRow();
	}

	void CDBSectionIndexVector::load(size_t no)const
	{
		static CCriticalSection m_CS;

		ASSERT(IsOpen());
		ASSERT(no < size());
		CDBSectionIndexVector& me = const_cast<CDBSectionIndexVector&>(*this);

		m_CS.Enter();
		
		
		if (!((CDBSectionIndexVectorBase&)me).at(no).IsInit())
		{
			me.m_file.seekg(get_pos(no));
			me.m_file >> ((CDBSectionIndexVectorBase&)me).at(no);
		}
		
		m_CS.Leave();
	}

	//************************************************************************


	string CBioSIMDatabase::GetMetadataFilePath(string filePath){ return SetFileExtension(filePath, ".DBmeta"); }
	string CBioSIMDatabase::GetIndexFilePath(string filePath){ return SetFileExtension(filePath, ".DBindex"); }


	CBioSIMDatabase::CBioSIMDatabase()
	{
		m_openMode = -1;
		m_nbCols = 0;
		m_nbRows = 0;
		m_type = UNKNOWN;
	}

	CBioSIMDatabase::~CBioSIMDatabase()
	{}


	ERMsg CBioSIMDatabase::Open(const string& filePath, int mode)
	{
		ASSERT(m_openMode = -1);

		ERMsg msg;

		mode |= std::fstream::binary;
		if (mode & std::fstream::out)
			mode |= std::fstream::trunc;


		msg = m_file.open(filePath, mode, SH_DENYWR);
		if (msg)
		{

			if (mode & std::fstream::in)
			{
				if (m_file.lengthg() > 24)
				{
					m_file.read_value(m_nbCols);
					m_file.read_value(m_nbRows);
					m_file.read_value(m_type);

					if (m_nbCols > 0 && m_nbRows > 0 && m_file.lengthg() == GetDataSize())
					{
						
					}
					else
					{
						//bad file
						m_file.close();
						msg.ajoute("Error: zero size database. The simulation have probably finish before the end of simulation");
					}
				}
				else
				{
					//bad file
					m_file.close();
					msg.ajoute("Error: empty database");
				}
			}
			else
			{
				assert(m_file.lengthp() == 0);
				m_openMode = mode;

				//cols and row will be update when closing
				m_file.write_value(m_nbCols);
				m_file.write_value(m_nbRows);
				m_file.write_value(m_type);
				assert(m_file.lengthp() == 24);
			}


			if (msg)
			{
				msg = m_metadata.Open(GetMetadataFilePath(filePath), mode);
				if (msg)
					msg = m_index.Open(GetIndexFilePath(filePath), mode);
			}
		}

		if (msg)
		{
			m_openMode = mode;
		}
		else
		{
			m_file.close();
			m_metadata.Close();
			m_index.Close();
		}


		return msg;
	}

	void CBioSIMDatabase::Close()
	{
		if (m_openMode != -1)
		{
			//update size and index
			if (m_openMode & std::fstream::out)
			{
				ASSERT(m_nbRows == GetNbRows());
				ASSERT(m_file.lengthp() == GetDataSize());

				m_file.seekp(0);
				m_file.write_value(m_nbCols);
				m_file.write_value(m_nbRows);
				m_file.write_value(m_type);
			}

			ASSERT(m_file.lengthp() == GetDataSize());
			m_file.close();
			m_openMode = -1;
			m_nbCols = 0;
			m_nbRows = 0;
			m_type = 0;

			m_index.Close();
			m_metadata.Close();
		}
	}

	ERMsg CBioSIMDatabase::Remove(std::string filePath)
	{
		ERMsg msg;

		if (FileExists(filePath))
			msg = RemoveFile(filePath);

		if (FileExists(GetMetadataFilePath(filePath)))
			msg = RemoveFile(GetMetadataFilePath(filePath));

		if (FileExists(GetIndexFilePath(filePath)))
			msg = RemoveFile(GetIndexFilePath(filePath));
		
		return msg;
	}

	ERMsg CBioSIMDatabase::AddSection(const CNewSectionData& section, int type, CCallback& callback)
	{
		ASSERT(m_nbCols == 0 || section.GetCols() == m_nbCols);
		ASSERT(m_type == UNKNOWN || m_type == type);

		ERMsg msg;

		m_CS.Enter();

		if (!callback.GetUserCancel())
		{
			m_nbCols = section.GetCols();
			m_nbRows += section.GetRows();
			m_type = type;

			m_metadata.AddSection(section);
			m_index.push_back(CDBSectionIndex(m_file.tellp(), UNKNOWN_POS/*m_nbRows*/, section.GetRows(), section.GetFirstTRef()));

			for (size_t i = 0; i < section.GetRows() && msg; i++)
			{
				for (size_t j = 0; j < section.GetCols() && msg; j++)
				{
					//m_file.write((char*)&(section[i][j]), sizeof(CStatistic));
					m_file.write_value(section[i][j]);
					msg += callback.StepIt(0);
				}
			}

			ASSERT(!msg || m_file.lengthp() == GetDataSize());
		}


		m_CS.Leave();
		return msg;
	}

	ERMsg CBioSIMDatabase::SetSection(size_t sectionNo, const CNewSectionData& section, int type, CCallback& callback)
	{
		ASSERT(m_nbCols == 0 || section.GetCols() == m_nbCols);
		ASSERT(m_type == UNKNOWN || m_type == type);


		ERMsg msg;

		m_CS.Enter();

		if (!callback.GetUserCancel())
		{
			m_type = type;
			m_nbCols = section.GetCols();
			m_nbRows += section.GetRows();
			m_metadata.AddSection(section);
			m_index.insert(sectionNo, CDBSectionIndex(m_file.tellp(), UNKNOWN_POS, section.GetRows(), section.GetFirstTRef()));

			//save data
			CStatistic STAT_EMPTY;
			for (size_t i = 0; i < section.GetRows() && msg; i++)
			{
				ASSERT(i >= section.GetRows() || section[i].size() == m_nbCols);
				for (size_t j = 0; j < m_nbCols&&msg; j++)
				{
					if (i < section.GetRows())
						m_file.write_value(section[i][j]);
					else m_file.write_value(STAT_EMPTY);

					msg += callback.StepIt(0);
				}
			}

			assert(!msg || m_file.lengthp() == GetDataSize());
		}


		m_CS.Leave();

		return msg;
	}


	ERMsg CBioSIMDatabase::SetSection(size_t sectionNo, const CSimulationPoint& section, CCallback& callback)
	{
		ERMsg msg;

		m_CS.Enter();

		if (!callback.GetUserCancel())
		{
			CWVariables variables = section.GetVariables();
			CTPeriod period = section.GetEntireTPeriod();
			int type = DATA_FLOAT;
			//rsa ?? EST-CE QUE CA SERT A QUEL;QUE CHOSE D'UTILISER UNE STATISTIQUE POUR LES siMULATIONpOINT???
			//int type = period.GetTM().Type() == CTM::HOURLY ? DATA_FLOAT : DATA_STATISTIC;
			assert(m_type == UNKNOWN || m_type == type);
			assert(m_nbCols == 0 || m_nbCols == variables.count());
			assert(sizeof(CStatistic)==40);
			
			

			m_type = type;
			m_nbCols = variables.count();
			m_nbRows += period.size();

			m_metadata.AddSection(section);
			m_index.insert(sectionNo, CDBSectionIndex(m_file.tellp(), UNKNOWN_POS, period.size(), period.Begin()));

			for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
			{
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&msg; v++)
				{
					if (variables[v])
					{
						if (m_type == DATA_FLOAT)
							m_file.write_value(float(section[TRef][v][MEAN]));
						else
							m_file.write_value(section[TRef][v]);

						msg += callback.StepIt(0);
					}
				}
			}

			assert(!msg || m_file.lengthp() == GetDataSize());
		}

		m_CS.Leave();

		return msg;
	}
	ERMsg CBioSIMDatabase::AddSection(const CModelStatVector& section, int type, CCallback& callback)
	{
		ASSERT(m_nbCols == 0 || m_nbCols == section.GetCols());
		ASSERT(m_type == UNKNOWN || m_type == type);

		ERMsg msg;

		m_CS.Enter();
		if (!callback.GetUserCancel())
		{
			m_type = type;
			m_nbCols = section.GetCols();
			m_nbRows += section.GetRows();

			m_metadata.AddSection(section);
			m_index.push_back(CDBSectionIndex(m_file.tellp(), UNKNOWN_POS, section.GetRows(), section.GetFirstTRef()));

			for (size_t i = 0; i < section.GetRows() && msg; i++)
			{
				ASSERT(section[i].size() == m_nbCols);
				for (size_t j = 0; j < section.GetCols() && msg; j++)
				{
					float value = float(section[i][j]);
					m_file.write_value(value);// , sizeof(float));
					msg += callback.StepIt(0);
				}
			}

			assert(!msg || m_file.lengthp() == GetDataSize());

		}

		m_CS.Leave();

		return msg;
	}

	ERMsg CBioSIMDatabase::SetSection(size_t sectionNo, const CModelStatVector& section, int type, CCallback& callback)
	{
		assert(m_nbCols == 0 || m_nbCols == section.GetCols());
		assert(m_type == UNKNOWN || m_type == type);

		ERMsg msg;

		m_CS.Enter();
		if (!callback.GetUserCancel())
		{
			m_nbCols = section.GetCols();
			m_type = type;
			m_nbRows += section.GetRows();
			m_metadata.AddSection(section);
			m_index.insert(sectionNo, CDBSectionIndex(m_file.tellp(), UNKNOWN_POS, section.size(), section.GetFirstTRef()));

			for (size_t i = 0; i < section.GetRows() && msg; i++)
			{
				ASSERT(i >= section.size() || section[i].size() == m_nbCols);
				for (size_t j = 0; j < m_nbCols&&msg; j++)
				{
					float value = (i < section.GetRows()) ? float(section[i][j]) : float(VMISS);

					m_file.write_value(value);
					msg += callback.StepIt(0);
				}
			}

			assert(!msg || m_file.lengthp() <= GetDataSize());
		}

		m_CS.Leave();

		return msg;
	}


	void CBioSIMDatabase::GetSection(size_t no, CNewSectionData& section)const
	{
		m_CS.Enter();


		assert(IsOpen());
		assert(no < m_index.size());
		assert(m_file.lengthg() == GetDataSize());

		CBioSIMDatabase& me = const_cast<CBioSIMDatabase&>(*this);
		const CDBSectionIndex& index = m_index[no];
		ASSERT(index.IsInit());

		size_t nbRows = index.GetNbRows();
		section.resize(nbRows, m_nbCols, m_index[no].GetTRef());
		me.m_file.seekg(GetFilePos(no, 0, 0));

		for (size_t i = 0; i < nbRows; i++)
		{
			for (size_t j = 0; j<section.GetCols(); j++)
			{
				if (m_type == DATA_FLOAT)
				{
					float value = 0;
					me.m_file.read_value(value);
					if (value > VMISS)
						section[i][j] = value;
				}
				else
				{
					me.m_file.read_value(section[i][j]);
				}
			}
		}

		section.SetDataTM(m_metadata.GetDataTM());

		m_CS.Leave();
	}

	void CBioSIMDatabase::GetSection(size_t no, CSimulationPoint& section, CWVariables filter)const
	{
		m_CS.Enter();

		assert(IsOpen());
		assert(no < m_index.size());
		assert(m_file.lengthg() == GetDataSize());

		CBioSIMDatabase& me = const_cast<CBioSIMDatabase&>(*this);
		size_t nbRows = m_index[no].GetNbRows();
		CTPeriod period = m_metadata.GetTPeriod();
		CWVariables variables = m_metadata.GetOutputDefinition().GetWVariables();
		//int type = DATA_FLOAT;// period.GetTM().Type() == CTM::HOURLY ? DATA_FLOAT : DATA_STATISTIC;
		CStatistic junk;



		if (variables.count() == m_nbCols &&
			period.GetNbRef() == nbRows /*&&
			m_type == type*/)
		{
			size_t locPos = m_metadata.GetLno(no);
			((CLocation&)section) = m_metadata.GetLocations()[locPos];
			section.CreateYears(period);

			CTRef TRef = period.Begin();
			section.SetHourly(TRef.GetType() == CTM::HOURLY);
			me.m_file.seekg(GetFilePos(no, 0, 0));
			for (size_t i = 0; i < nbRows; i++, TRef++)
			{
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						if (m_type == DATA_FLOAT)
						{
							float value = 0;
							me.m_file.read_value(value);
							if (filter[v])
								section[period.Begin() + i].SetStat(v, value);
						}
						else
						{
							if (filter[v])
								me.m_file.read_value(section[TRef][v]);
							else
								me.m_file.read_value(junk);
						}
					}
				}
			}
		}

		m_CS.Leave();
	}

	ULONGLONG CBioSIMDatabase::GetFilePos(size_t no, size_t row, size_t col)const
	{
		ASSERT(m_index[no].IsInit());
		ASSERT(row < m_index[no].GetNbRows());
		ASSERT(col < m_nbCols);

		return  m_index[no].GetBeginPos() + (row*m_nbCols + col)*GetTypeSize();
	}


}