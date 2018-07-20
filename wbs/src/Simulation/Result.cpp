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
#include "Basic/WeatherStation.h"
#include "Simulation/Result.h"

#include "WeatherBasedSimulationString.h"

using namespace WBSF::DIMENSION;

namespace WBSF
{

	const CLocationVector CResult::DEFAULT_LOC;
	const CModelInputVector CResult::DEFAULT_PARAMETER_SET;
	const CTPeriod CResult::DEFAULT_PERIOD;


	CResult::CResult()
	{
		Reset();
	}


	CResult::~CResult(void)
	{}

	void CResult::Reset()
	{
		m_filePath.empty();
		m_loadedSection.reset();
		m_lastSectionLoaded = UNKNOWN_POS;
	}

	std::string CResult::GetFilePath(int type)const
	{
		ASSERT(!m_filePath.empty());
		std::string filePathOut(m_filePath);

		ASSERT(NB_FILE_TYPE == 3);
		switch (type)
		{
		case DATA: SetFileExtension(filePathOut, ".bsimDB"); break;
		case INFO: SetFileExtension(filePathOut, ".bsim"); break;
		case META: SetFileExtension(filePathOut, ".xml"); break;
		default: ASSERT(false); break;
		}

		return filePathOut;
	}

	ERMsg CResult::Open(const std::string& DBFilePath, int mode)
	{
		ASSERT(!DBFilePath.empty());
		ERMsg msg;

		m_nbSectionAdded = 0;

		//Create the directory
		if (mode&std::fstream::out)
			CreateMultipleDir(GetPath(DBFilePath));

		msg = m_database.Open(DBFilePath, mode);
		if (msg)
			m_filePath = DBFilePath;



		return msg;
	}

	void CResult::Close()
	{
		m_database.Close();
	}

	ERMsg CResult::Remove(std::string filePath)
	{
		return CBioSIMDatabase::Remove(filePath);
	}

	ERMsg CResult::AddSection(const CNewSectionData& section, CCallback& callback)
	{
		ERMsg msg;
		//we update the temporal period
		//m_infoFile.AddSection(section);
		msg += m_database.AddSection(section, CBioSIMDatabase::DATA_STATISTIC, callback);
#pragma omp atomic
		m_nbSectionAdded++;
		return msg;
	}

	ERMsg CResult::AddSection(const CModelStatVector& section, CCallback& callback)
	{
		ERMsg msg;
		//we update the temporal period
		//m_infoFile.AddSection(section);
		msg += m_database.AddSection(section, CBioSIMDatabase::DATA_FLOAT, callback);
#pragma omp atomic
		m_nbSectionAdded++;

		return msg;
	}

	ERMsg CResult::SetSection(size_t sectionNo, const CNewSectionData& section, CCallback& callback)
	{
		ERMsg msg;

		//we update the temporal period
		//m_infoFile.AddSection(section);
		msg += m_database.SetSection(sectionNo, section, CBioSIMDatabase::DATA_STATISTIC, callback);
#pragma omp atomic
		m_nbSectionAdded++;

		return msg;
	}

	ERMsg CResult::SetSection(size_t sectionNo, const CModelStatVector& section, CCallback& callback)
	{
		ERMsg msg;
		//we update the temporal period
		//m_infoFile.AddSection(section);
		msg += m_database.SetSection(sectionNo, section, CBioSIMDatabase::DATA_FLOAT, callback);
#pragma omp atomic
		m_nbSectionAdded++;

		return msg;
	}

	ERMsg CResult::SetSection(size_t sectionNo, const CSimulationPoint& section, CCallback& callback)
	{
		ERMsg msg;

		//we update the temporal period
		//m_infoFile.AddSection(section);
		msg += m_database.SetSection(sectionNo, section, callback);
#pragma omp atomic
		m_nbSectionAdded++;

		return msg;
	}


	void CResult::GetSection(size_t no, CNewSectionData& section)const
	{
		ASSERT(no < m_database.GetNbSection());

		section.Reset();

		if (no < m_database.GetNbSection())
		{
			section.m_locPos = GetMetadata().GetLno(no);
			section.m_paramPos = GetMetadata().GetPno(no);
			section.m_repPos = GetMetadata().GetRno(no);
			//section.m_dimension = m_infoFile.GetDimension();
			section.m_overAllPeriod = GetMetadata().GetTPeriod();
			section.m_globalFirstRow = m_database.GetFirstRow(no);
			//section.SetDataTM(GetMetadata().GetDataTM()); //temporal reference of data (when event)

			m_database.GetSection(no, section);
		}
	}

	void CResult::GetSection(size_t no, CSimulationPoint& section, CWVariables filter)const
	{
		ASSERT(no < m_database.GetNbSection());

		section.clear();

		if (no < m_database.GetNbSection())
		{
			m_database.GetSection(no, section, filter);
		}
	}

	//int CResult::GetColWidth(size_t col)const
	//{
	//	int width = 100;//on pourrais avoir une grandeur par defaut
	//
	//	CDimension dim = GetDimension();
	//	
	//	if( col < NB_DIMENSION )
	//	{
	//		if( col!=VARIABLE)
	//			width = (dim[col]>1)?100:0;
	//		else width = 0;//never show VARAIBLE col beacause it's the columnised dimension
	//	}
	//		
	//	return width;
	//}


	void CResult::GetDataHead(StringVector& dataHead)const
	{
		StringVector strTitle = Tokenize(GetString(IDS_SIM_RESULT_HEAD), "|;");
		ASSERT(strTitle.size() == NB_DIMENSION);

		const CModelOutputVariableDefVector& outdef = GetMetadata().GetOutputDefinition();//m_model.GetOutputDefinition();
		dataHead = strTitle;

		for (size_t i = 0; i < outdef.size(); i++)
			dataHead.push_back(outdef[i].m_title.c_str());

	}

	bool CResult::IsLoaded(size_t sectionNo)const
	{
		ASSERT(sectionNo >= 0);

		bool bRep = false;
		if (sectionNo == m_lastSectionLoaded)
			bRep = true;


		return bRep;
	}
	void CResult::LoadSection(size_t sectionNo)const
	{
		ASSERT(sectionNo >= 0);
		ASSERT(IsOpen());


		if (!IsLoaded(sectionNo))
		{
			//ASSERT( sectionNo>=m_loadedSectionArray.size() || m_loadedSectionArray[sectionNo].get() == NULL);


			CResult& me = const_cast<CResult&>(*this);

			CNewSectionDataPtr sectionPtr(new CNewSectionData);
			GetSection(sectionNo, *sectionPtr);
			me.m_loadedSection = sectionPtr;
			me.m_lastSectionLoaded = sectionNo;
			//me.m_loadedSectionArray.SetAtGrow(sectionNo, sectionPtr);
		}

		//ASSERT( m_loadedSectionArray[sectionNo].get() != NULL);
		ASSERT(m_loadedSection.get() != NULL);
	}

	std::string CResult::GetDimensionString(size_t sectionNo, size_t nRow, size_t nCol)const
	{
		ASSERT(NB_DIMENSION == 5);

		std::string str;

		switch (nCol)
		{
		case LOCATION:		str = GetLocation(sectionNo).m_name.c_str(); break;
		//case PARAMETER:		str = ToString(GetMetadata().GetPno(sectionNo) + 1); break;
		case PARAMETER:
		{
			size_t s = GetMetadata().GetPno(sectionNo);
			const CModelInputVector& inputs = GetMetadata().GetParameterSet();
			//if (inputs.m_variation.size() == inputs.m_pioneer.size())
			{
				for (size_t i = 0; i < inputs.m_variation.size(); i++)
				{
					if (inputs.m_variation[i].m_bActive)
					{
						if (!str.empty())
							str += ",";

						if (i < inputs[s].size())
							str += inputs[s].at(i).GetStr();
						else
							str += "?";
						//str += inputs.m_variation[i].m_name;
					}

				}
			}

			//str = GetParameterSet(sectionNo).GetName();
			break;
		}
		case REPLICATION:	str = ToString(GetReplication(sectionNo) + 1); break;
		case TIME_REF:
		{
			CTRef ref = GetFirstTRef(sectionNo) + nRow;
			str = ref.GetFormatedString();
			break;
		}
		case VARIABLE:	str = ToString(0); break;
		default:ASSERT(false);
		}


		return str;
	}

	size_t CResult::GetCol(size_t dimension, size_t field)
	{
		if (dimension < VARIABLE)
			return dimension;

		ASSERT(dimension == VARIABLE);
		ASSERT(field >= 0);


		if (field < 0 || field >= GetNbCols(false))
			return UNKNOWN_POS;

		return field + NB_DIMENSION;
	}


	const CStatistic& CResult::GetData(size_t nRow, size_t nCol)const
	{
		ASSERT(nRow >= 0 && nRow < GetNbRows());
		ASSERT(nCol >= NB_DIMENSION && nCol < GetNbCols());
		ASSERT(IsOpen());

		size_t sectionNo = GetSectionNo(nRow);
		ASSERT(sectionNo >= 0);

		static const CStatistic INVALID_STAT;

		if (sectionNo < 0)
			return INVALID_STAT;


		size_t sectionRow = GetSectionRow(sectionNo, nRow);
		size_t sectionCol = nCol - NB_DIMENSION;

		LoadSection(sectionNo);

		return (*m_loadedSection)[sectionRow][sectionCol];

	}

	bool CResult::HaveData(size_t nRow)const
	{
		ASSERT(nRow >= 0 && nRow < GetNbRows());
		ASSERT(IsOpen());

		size_t sectionNo = GetSectionNo(nRow);
		ASSERT(sectionNo >= 0);

		static const CStatistic INVALID_STAT;

		if (sectionNo < 0)
			return false;


		size_t sectionRow = GetSectionRow(sectionNo, nRow);
		LoadSection(sectionNo);
		return m_loadedSection->HaveData(sectionRow);

	}

	std::string CResult::GetDataValue(size_t nRow, size_t nCol, size_t stat)const
	{
		ASSERT(nRow < GetNbRows());
		ASSERT(nCol < GetNbCols() + NB_DIMENSION);
		ASSERT(IsOpen());

		std::string str;
		size_t sectionNo = GetSectionNo(nRow);
		if (sectionNo >= 0)
		{
			size_t sectionRow = GetSectionRow(sectionNo, nRow);
			size_t sectionCol = nCol - NB_DIMENSION;

			if (nCol < NB_DIMENSION)
			{
				str = GetDimensionString(sectionNo, sectionRow, nCol);
			}
			else
			{
				LoadSection(sectionNo);
				if (m_loadedSection->IsTemporalMatrix(sectionCol) && IsTemporalStat(stat))
				{
					CTRef ref = m_loadedSection->GetTRef(sectionRow, sectionCol, stat);
					
					if (ref.IsInit())
					{
						str = ref.GetFormatedString();
					}
					else
					{
						const std::vector<CTM>& dataTM = m_loadedSection->GetDataTM();
						ASSERT(sectionCol < dataTM.size());
						str = CTRef::GetMissingString(dataTM[sectionCol]);
					}
						
				}
				else
				{		
					const CModelOutputVariableDefVector& var = GetMetadata().GetOutputDefinition();
					int precision = var[sectionCol].m_precision;
					str = ToString((*m_loadedSection)[sectionRow][sectionCol][stat], precision);
				}
			}
		}

		return str;
	}

	double CResult::GetDataValue(size_t row, size_t dim, size_t field, size_t stat)const
	{
		double value = VMISS;
		size_t sectionNo = GetSectionNo(row);
		if (sectionNo >= 0)
		{
			size_t sectionRow = GetSectionRow(sectionNo, row);
			value = GetDataValue(sectionNo, sectionRow, dim, field, stat);
		}

		return value;
	}

	double CResult::GetDataValue(size_t sectionNo, size_t row, size_t dim, size_t field, size_t stat)const
	{
		ASSERT(IsOpen());
		ASSERT(sectionNo >= 0 && sectionNo < GetNbSection());
		ASSERT(row >= 0 && row < m_database.GetNbRows(sectionNo));
		ASSERT(dim >= 0 && dim < NB_DIMENSION);


		double value = VMISS;


		if (dim == LOCATION)
		{
			const CLocation& loc = GetLocation(sectionNo);
			switch (field)
			{
			case CLocation::ID: value = ToDouble(loc.m_ID); break;
			case CLocation::NAME: value = ToDouble(loc.m_name); break;
			case CLocation::LAT: value = loc.m_lat; break;
			case CLocation::LON: value = loc.m_lon; break;
			case CLocation::ELEV: value = loc.m_alt; break;
			case CLocation::SSI:
			{
				//loc.GetSS
				//value = ;
				break;
			}
			default: ASSERT(false);
			}
		}
		else if (dim == TIME_REF)
		{
			CTRef Tref = GetFirstTRef(sectionNo) + row;
			if (field == 0)
				value = (double)Tref; //return double representation of the CTRef
			else
				value = Tref.GetRef(); //return double representation of the CTRef
		}
		else if (dim == VARIABLE)
		{
			LoadSection(sectionNo);

			if (m_loadedSection->IsTemporalMatrix(field))
			{
				if (IsTemporalStat(stat))
				{
					//return double representation of the event
					CTRef ref = m_loadedSection->GetTRef(row, field, stat);
					value = (double)ref;
					//value = (*m_loadedSection)[row][field][stat];
					//CTRef TRef(value, m_dataTM[i].Type());
				}
				else
				{
					ASSERT(false);//quoi faire????
				}
			}
			else
			{
				value = (*m_loadedSection)[row][field][stat];
			}
			
		}
		else
		{
			ASSERT(false);
		}

		return value;
	}

	size_t CResult::GetNbField(size_t dimension)const
	{
		size_t nbField = 0;
		switch (dimension)
		{
		case LOCATION:	nbField = CLocation::NB_MEMBER; break;
		case PARAMETER:	nbField = 1; break;
		case REPLICATION:nbField = 1; break;
		case TIME_REF:	nbField = 1; break;
		case VARIABLE:	nbField = GetMetadata().GetOutputDefinition().size(); break;
		default: ASSERT(false);
		}

		return nbField;
	}


	std::string CResult::GetFieldTitle(size_t d, size_t f, size_t stat)const
	{
		std::string title;
		switch (d)
		{
		case LOCATION:

			//if( f!= CLocation::OTHERS )
			//{
			title = CLocation::GetMemberTitle(f);
			//}
			//else 
			//{
			//	const CLocationVector& loc = m_infoFile.GetLOC();
			//	title=loc.GetOtherHeader();
			//}
			break;
		case PARAMETER:
		{
			const CModelInputVector& inputs = GetMetadata().GetParameterSet();
			//if (inputs.m_variation.size() == inputs.m_pioneer.size())
			{

				for (size_t i = 0; i < inputs.m_variation.size(); i++)
				{
					if (inputs.m_variation[i].m_bActive)
					{
						if (!title.empty())
							title += ",";

						title += inputs.m_variation[i].m_name;
					}
				}
			}
			break;
		}
		case REPLICATION:
		case TIME_REF:	title = CDimension::GetDimensionTitle(d); break;
		case VARIABLE:
		{
			const CModelOutputVariableDefVector& def = GetMetadata().GetOutputDefinition();
			title = def[f].m_title.c_str();

			if (stat != MEAN)//add name only if it's not the mean (the default)
				title += std::string("(") + CStatistic::GetName(stat) + ")";

			break;
		}
		default: ASSERT(false);
		}


		return title;
	}
}