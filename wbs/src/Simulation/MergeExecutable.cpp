//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	R�mi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "Simulation/MergeExecutable.h"
#include "Simulation/ExecutableFactory.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::DIMENSION;

namespace WBSF
{

	//*******************************************************************************
	const char* CMergeExecutable::XML_FLAG = "MergeExecutable";
	const char* CMergeExecutable::MEMBERS_NAME[NB_MEMBERS_EX] = { "AppendDimension", "MergedArray", "AddName" };
	const int CMergeExecutable::CLASS_NUMBER = CExecutableFactory::RegisterClass(CMergeExecutable::GetXMLFlag(), &CMergeExecutable::CreateObject);

	CMergeExecutable::CMergeExecutable()
	{
		ClassReset();
	}

	CMergeExecutable::CMergeExecutable(const CMergeExecutable& in)
	{
		operator=(in);
	}

	CMergeExecutable::~CMergeExecutable()
	{}

	void CMergeExecutable::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CMergeExecutable::ClassReset()
	{
		m_name = "Merge";
		m_mergedArray.clear();
		m_dimensionAppend = VARIABLE;
		m_bAddName = true;

	}

	CMergeExecutable& CMergeExecutable::operator =(const CMergeExecutable& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_mergedArray = in.m_mergedArray;
			m_dimensionAppend = in.m_dimensionAppend;
			m_bAddName = in.m_bAddName;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CMergeExecutable::operator == (const CMergeExecutable& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_mergedArray != in.m_mergedArray)bEqual = false;
		if (m_dimensionAppend != in.m_dimensionAppend)bEqual = false;
		if (m_bAddName != in.m_bAddName)bEqual = false;

		return bEqual;
	}

	/*
	std::string CMergeExecutable::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	std::string str;
	switch(i)
	{
	case APPEND_DIMENSION: str = ToString(m_dimensionAppend); break;
	case MERGE_ARRAY: str = m_mergedArray.ToString(); break;
	default: str = CExecutable::GetMember(i, pNode);
	}

	return str;
	}

	void CMergeExecutable::SetMember(int i, const std::string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case APPEND_DIMENSION: m_dimensionAppend = ToInt(str); break;
	case MERGE_ARRAY: m_mergedArray.FromString(str); break;
	default:CExecutable::SetMember(i, str, pNode);
	}

	}
	*/

	void CMergeExecutable::GetInputDBInfo(const CResultPtrVector& resultArray, CDBMetadata& info)
	{
		ASSERT(!resultArray.empty());

		//ici on devrais avoir dans les r�sultat le nom des ex�cutable : a faire
		//ajouter dans les r�sultats les information sur l'�l�ments en cours
		CExecutablePtrVector executable;
		GetExecutableArray(executable);

		info = resultArray[0]->GetMetadata();

		//time dimension can always be different
		CTPeriod period = info.GetTPeriod();
		for (size_t i = 1; i < resultArray.size(); i++)
			period.Inflate(resultArray[i]->GetMetadata().GetTPeriod());
		info.SetTPeriod(period);



		switch (m_dimensionAppend)
		{
		case LOCATION:
		{
			CLocationVector tmp = info.GetLocations();
			for (size_t i = 1; i < resultArray.size(); i++)
				tmp.insert(tmp.end(), resultArray[i]->GetMetadata().GetLocations().begin(), resultArray[i]->GetMetadata().GetLocations().end());
			info.SetLocations(tmp);

			break;
		}
		case PARAMETER:
		{
			CModelInputVector tmp = info.GetParameterSet();
			for (size_t i = 1; i < resultArray.size(); i++)
				tmp.insert(tmp.end(), resultArray[i]->GetMetadata().GetParameterSet().begin(), resultArray[i]->GetMetadata().GetParameterSet().end());
			info.SetParameterSet(tmp);
			break;
		}
		case REPLICATION:
		{
			size_t nbRep = info.GetNbReplications();
			for (size_t i = 1; i < resultArray.size(); i++)
				nbRep += resultArray[i]->GetMetadata().GetNbReplications();
			info.SetNbReplications(nbRep);
			break;
		}
		case TIME_REF:
		{
			break;
		}
		case VARIABLE:
		{
			CModelOutputVariableDefVector variable;
			variable.reserve(resultArray.size());
			for (size_t i = 0; i < resultArray.size(); i++)
			{
				CModelOutputVariableDefVector tmp = resultArray[i]->GetMetadata().GetOutputDefinition();

				for (size_t j = 0; j < tmp.size(); j++)
				{
					if (m_bAddName)
						tmp[j].m_name += "_" + executable[i]->GetName();

					ReplaceString(tmp[j].m_name, " ", "_");

					if (m_bAddName)
						tmp[j].m_title += executable[i]->GetName();
				}

				variable.insert(variable.end(), tmp.begin(), tmp.end());
			}

			info.SetOutputDefinition(variable);
			break;
		}
		default: ASSERT(false);
		}


	}

	ERMsg CMergeExecutable::VerifyValidity(const CResultPtrVector& resultArray)
	{
		ASSERT(!resultArray.empty());

		ERMsg msg;

		CDimension dim0 = resultArray[0]->GetDimension();
		CTM TM0 = resultArray[0]->GetTM();

		for (size_t i = 1; i < resultArray.size(); i++)
		{
			CTM TM = resultArray[i]->GetTM();

			if (TM != TM0)
			{
				msg.ajoute(FormatMsg(IDS_SIM_INVALID_MERGE_TEMPORAL, TM0.GetTypeModeName(), TM.GetTypeModeName()));
			}

			for (int d = 0; d < NB_DIMENSION; d++)
			{
				if (d != m_dimensionAppend && d != TIME_REF)//exclute appenned dimension and time dimention
				{
					CDimension dim = resultArray[i]->GetDimension();
					if (dim[d] != dim0[d])
					{
						msg.ajoute(FormatMsg(IDS_SIM_INVALID_MERGE_DIMENSION, CDimension::GetDimensionName(d), ToString(dim0[d]), ToString(dim[d])));
					}
				}
			}
		}

		return msg;
	}

	void AdjustPeriod(CNewSectionData& section, CTPeriod period)
	{
		ASSERT(section.GetTPeriod().GetTM() == period.GetTM());

		CNewSectionData newsection(period.size(), section.GetXSize(), period.Begin());
		for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
		{
			if (section.IsInside(TRef))
				newsection[TRef] = section[TRef];
		}

		section = newsection;

	}

	ERMsg CMergeExecutable::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;
		if (m_mergedArray.empty())
			return msg;

		CExecutablePtrVector executable;
		msg = GetExecutableArray(executable);
		if (!msg)
			return msg;

		CResultPtrVector resultArray;
		resultArray.resize(executable.size());
		for (size_t i = 0; i < executable.size(); i++)
		{
			ASSERT(executable[i]);
			resultArray[i] = executable[i]->GetResult(fileManager);
			msg += resultArray[i]->Open();
		}


		if (msg)
			msg = VerifyValidity(resultArray);

		if (!msg)
			return msg;

		//Generate output path
		std::string outputPath = GetPath(fileManager);

		//Generate DB file path
		std::string DBFilePath = GetDBFilePath(outputPath);

		//open outputDB
		CResult result;
		msg = result.Open(DBFilePath, std::fstream::out | std::fstream::binary);

		if (!msg)
			return msg;


		//init output info
		CDBMetadata& metadata = result.GetMetadata();
		GetInputDBInfo(resultArray, metadata);

		const CModelOutputVariableDefVector& varDef = metadata.GetOutputDefinition();
		CTPeriod period = metadata.GetTPeriod();

		if (m_dimensionAppend == LOCATION ||
			m_dimensionAppend == PARAMETER ||
			m_dimensionAppend == REPLICATION)
		{
			size_t nbSection = 0;
			for (size_t i = 0; i < resultArray.size(); i++)
				nbSection += resultArray[i]->GetNbSection();

			callback.PushTask("Merge", nbSection);


			//size_t ss = 0;
			size_t base_l = 0;
			size_t base_p = 0;
			size_t base_r = 0;
			for (size_t i = 0; i < resultArray.size() && msg; i++)
			{
				size_t nbSection = resultArray[i]->GetNbSection();
				for (size_t s = 0; s < nbSection && msg; s++)
				{
					CNewSectionData section;
					resultArray[i]->GetSection(s, section);

					if (section.GetTPeriod() != period)
					{
						//adjust period to get the same shape as the output period
						AdjustPeriod(section, period);
					}

					size_t l = resultArray[i]->GetMetadata().GetLno(s);
					size_t p = resultArray[i]->GetMetadata().GetPno(s);
					size_t r = resultArray[i]->GetMetadata().GetRno(s);


					size_t no = result.GetSectionNo(base_l + l, base_p + p, base_r + r);
					result.SetSection(no, section);


					msg += callback.StepIt();
				}

				if (m_dimensionAppend == LOCATION)
					base_l += resultArray[i]->GetDimension()[LOCATION];
				if (m_dimensionAppend == PARAMETER)
					base_p += resultArray[i]->GetDimension()[PARAMETER];
				if (m_dimensionAppend == REPLICATION)
					base_r += resultArray[i]->GetDimension()[REPLICATION];

			}

			callback.PopTask();
		}
		else //TIME_REF or VARIABLE
		{
			size_t nbSection = resultArray[0]->GetNbSection();

			callback.PushTask("Merge", nbSection);

			for (size_t s = 0; s < nbSection && msg; s++)
			{
				CNewSectionData section0;
				resultArray[0]->GetSection(s, section0);
				if (section0.GetTPeriod() != period)
				{
					//adjust period to get the same shape as the output period
					AdjustPeriod(section0, period);
				}


				for (size_t i = 1; i < resultArray.size() && msg; i++)
				{
					ASSERT(resultArray[i]->GetNbSection() == nbSection);
					CNewSectionData section;
					resultArray[i]->GetSection(s, section);

					if (section.GetTPeriod() != period)
					{
						//adjust period to get the same shape as the output period
						AdjustPeriod(section, period);
					}

					if (m_dimensionAppend == TIME_REF)
					{
						ASSERT(section.GetCols() == section0.GetCols());
						section0.AppendRows(section);
					}
					else //VARIABLE
					{
						ASSERT(section.GetRows() == section0.GetRows());
						section0.AppendCols(section);
					}//if TIME_REF

					msg += callback.StepIt(1.0 / (resultArray.size() - 1));
				}//for all result

				result.AddSection(section0);
			}//for all section

			callback.PopTask();
		}

		for (size_t i = 0; i < resultArray.size(); i++)
			resultArray[i]->Close();

		result.Close();


		return msg;
	}

	ERMsg CMergeExecutable::GetExecutableArray(CExecutablePtrVector& executable)const
	{
		ERMsg msg;

		executable.clear();
		//executable.resize(m_mergedArray.size());
		for (size_t i = 0; i < m_mergedArray.size(); i++)
		{
			if (m_mergedArray[i] != GetParent()->GetInternalName())
			{
				CExecutablePtr ptr = GetParent()->FindItem(m_mergedArray[i]);
				//if( executable[i]==NULL )
				if (ptr != NULL)
				{
					if (std::string(ptr->GetClassName()) != "Group")
						executable.push_back(ptr);
				}
				else
				{
					msg.ajoute(FormatMsg(IDS_ITEM_NOT_FOUND, m_mergedArray[i]));
				}
			}
			//executable[i] = GetParent()->FindItem(m_mergedArray[i]);


		}


		return msg;
	}

	ERMsg CMergeExecutable::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ASSERT(GetParent());

		ERMsg msg;
		if (m_mergedArray.empty())
			return msg;

		CExecutablePtrVector executable;
		msg = GetExecutableArray(executable);
		if (msg && !executable.empty())
		{
			CParentInfoFilter filter2;
			filter2.reset();
			filter2.set(m_dimensionAppend);


			executable.front()->GetParentInfo(fileManager, info, filter);
			if (filter[VARIABLE] && m_dimensionAppend == VARIABLE)
			{
				for (size_t j = 0; j < info.m_variables.size(); j++)
				{
					std::string name = info.m_variables[j].m_name;
					if (m_bAddName)
						name += "_" + executable[0]->GetName();

					std::replace(name.begin(), name.end(), ' ', '_');
					info.m_variables[j].m_name = name;

					if (m_bAddName)
						info.m_variables[j].m_title += executable[0]->GetName();
				}
			}


			//append other info
			for (size_t i = 1; i < executable.size() && msg; i++)
			{
				CParentInfo info2;
				msg = executable[i]->GetParentInfo(fileManager, info2, filter2);
				if (filter[LOCATION] && m_dimensionAppend == LOCATION)
				{
					info.m_locations.insert(info.m_locations.end(), info2.m_locations.begin(), info2.m_locations.end());
				}

				if (filter[PARAMETER] && m_dimensionAppend == PARAMETER)
				{
					info.m_parameterset.insert(info.m_parameterset.end(), info2.m_parameterset.begin(), info2.m_parameterset.end());
				}

				if (filter[REPLICATION] && m_dimensionAppend == REPLICATION)
				{
					info.m_nbReplications += info2.m_nbReplications;
				}

				//always append time
				if (filter[TIME_REF])//&& m_dimensionAppend == TIME_REF
				{
					if (!info.m_period.IsInit() || info.m_period.GetTM() == info2.m_period.GetTM())
						info.m_period.Inflate(info2.m_period);
				}

				if (filter[VARIABLE] && m_dimensionAppend == VARIABLE)
				{
					for (size_t j = 0; j < info2.m_variables.size(); j++)
					{
						std::string name = info2.m_variables[j].m_name;
						if (m_bAddName)
							name += "_" + executable[i]->GetName();

						std::replace(name.begin(), name.end(), ' ', '_');
						info2.m_variables[j].m_name = name;

						if (m_bAddName)
							info2.m_variables[j].m_title += executable[i]->GetName();
					}

					info.m_variables.insert(info.m_variables.end(), info2.m_variables.begin(), info2.m_variables.end());
				}
			}
		}

		return msg;
	}




	void CMergeExecutable::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(APPEND_DIMENSION)](m_dimensionAppend);
		out[GetMemberName(MERGE_ARRAY)](m_mergedArray.to_string("|"));
		out[GetMemberName(ADD_NAME)](m_bAddName);
	}

	bool CMergeExecutable::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);

		in[GetMemberName(APPEND_DIMENSION)](m_dimensionAppend);
		std::string tmp; in[GetMemberName(MERGE_ARRAY)](tmp); m_mergedArray.Tokenize(tmp, "|");
		in[GetMemberName(ADD_NAME)](m_bAddName);

		return true;
	}

	//*******************************************************************************

}