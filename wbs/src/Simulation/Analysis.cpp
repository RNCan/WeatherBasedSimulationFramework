//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Creation
//******************************************************************************
#include "stdafx.h"
#include <Boost\multi_array.hpp>

#include "Basic/Statistic.h"
#include "FileManager/FileManager.h"
#include "Simulation/Analysis.h"
#include "Simulation/ExecutableFactory.h"

#include "WeatherBasedSimulationString.h"


using namespace WBSF::DIMENSION;
using namespace boost;

namespace WBSF
{


	//**********************************************************************
	//CAnalysisComputation
	const char* CAnalysisComputation::XML_FLAG = "Computation";
	const char* CAnalysisComputation::MEMBER_NAME[NB_MEMBER] = { "PreviousStatisticType", "SelectTimeTransformation", "TemporalTypeMode", "Kind", "StatisticType", "EventType", "K", "DropYear", "MeanOverReplications", "MeanOverParameterSet", "MeanOverLocations" };

	CAnalysisComputation::CAnalysisComputation()
	{
		//ASSERT(NB_STATISTIC == NB_STAT_TYPE);
		Reset();
	}

	void CAnalysisComputation::Reset()
	{
		m_bSelectTimeTransformation = false;
		m_TM = CTM(CTM::ANNUAL, CTM::FOR_EACH_YEAR);

		m_kind = STATISTIC;
		m_previousStatisticType = -1;//all statistic MEAN;
		m_statisticType2 = -1;///all statistic
		m_eventType = TW_MAX;

		m_K = 0;
		m_bDropYear = false;

		m_bMeanOverReplication = true;
		m_bMeanOverParameterSet = false;
		m_bMeanOverLocation = false;

	}

	bool CAnalysisComputation::operator == (const CAnalysisComputation& in)const
	{
		bool bEqual = true;

		if (m_previousStatisticType != in.m_previousStatisticType) bEqual = false;
		if (m_bSelectTimeTransformation != in.m_bSelectTimeTransformation) bEqual = false;
		if (m_TM != in.m_TM) bEqual = false;

		if (m_kind != in.m_kind) bEqual = false;
		if (m_kind == STATISTIC && m_statisticType2 != in.m_statisticType2) bEqual = false;
		if (m_kind == EVENT && m_eventType != in.m_eventType) bEqual = false;
		if (m_K != in.m_K) bEqual = false;
		if (m_bDropYear != in.m_bDropYear) bEqual = false;

		if (m_bMeanOverReplication != in.m_bMeanOverReplication) bEqual = false;
		if (m_bMeanOverParameterSet != in.m_bMeanOverParameterSet) bEqual = false;
		if (m_bMeanOverLocation != in.m_bMeanOverLocation) bEqual = false;

		return bEqual;

	}


	std::string CAnalysisComputation::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		std::string str;
		switch (i)
		{
		case PREVIOUS_STATICTIC_TYPE: str = ToString(m_previousStatisticType); break;
		case SELECT_COMPUTATION:	str = ToString(m_bSelectTimeTransformation); break;
		case T_TYPEMODE:		str = ToString(m_TM); break;
		case KIND:			str = ToString(m_kind); break;
		case STATICTIC_TYPE:str = ToString(m_statisticType2); break;
		case EVENT_TYPE:	str = ToString(m_eventType); break;
		case K_VALUE:		str = ToString(m_K); break;
		case DROP_YEAR:		str = ToString(m_bDropYear); break;
		case MEAN_OVER_REPLICATION: str = ToString(m_bMeanOverReplication); break;
		case MEAN_OVER_PARAMETERSET: str = ToString(m_bMeanOverParameterSet); break;
		case MEAN_OVER_LOCATION:	str = ToString(m_bMeanOverLocation); break;
		default:ASSERT(false);
		}

		return str;
	}

	void CAnalysisComputation::SetMember(size_t i, const std::string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);
		switch (i)
		{
		case PREVIOUS_STATICTIC_TYPE: m_previousStatisticType = ToShort(str); break;
		case SELECT_COMPUTATION:	m_bSelectTimeTransformation = ToBool(str); break;
		case T_TYPEMODE:			m_TM = ToShort(str); break;
		case KIND:				m_kind = ToShort(str); break;
		case STATICTIC_TYPE:	m_statisticType2 = ToShort(str); break;
		case EVENT_TYPE:		m_eventType = ToShort(str); break;
		case K_VALUE:			m_K = ToFloat(str); break;
		case DROP_YEAR:			m_bDropYear = ToBool(str); break;
		case MEAN_OVER_REPLICATION: m_bMeanOverReplication = ToBool(str); break;
		case MEAN_OVER_PARAMETERSET:m_bMeanOverParameterSet = ToBool(str); break;
		case MEAN_OVER_LOCATION:	m_bMeanOverLocation = ToBool(str); break;
		default:ASSERT(false);
		}

	}

	ERMsg CAnalysisComputation::Compute(const CNewSectionData& input, CNewSectionData& output)const
	{
		ERMsg msg;

		CNewSectionData computation;

		if (m_bSelectTimeTransformation)
		{
			if (m_kind == CAnalysisComputation::STATISTIC)
			{
				msg = ComputeStatistic(input, computation);
			}
			else
			{
				ASSERT(CAnalysisComputation::EVENT);
				msg = ComputeEvent(input, computation);
			}
		}
		else
		{
			//Only extract statistics
			ExtractStatistic(input, computation);
		}

		//const CNewSectionData& switchInput = m_bSelectTimeTransformation?computation:input;
		output.AppendRows(computation);

		ASSERT(output.GetCols() == input.GetCols());
		ASSERT(output.GetRows() == 0 || output[0].size() == input[0].size());

		return msg;
	}

	void CAnalysisComputation::ExtractStatistic(const CNewSectionData& input, CNewSectionData& output)const
	{
		output.resize(input.GetRows(), input.GetCols(), input.GetFirstTRef());
		for (size_t i = 0; i < input.GetRows(); i++)
		{
			//compute statistic for all variable
			for (size_t j = 0; j < input.GetCols(); j++)
			{
				//the output matrix here can be temporal or not depend of the statistic
				if (input[i][j][NB_VALUE] > 0)
				{
					if (m_previousStatisticType >= 0)
					{
						if (input.IsTemporalMatrix(j) && IsTemporalStat())
							output.SetTRef(i, j, input.GetTRef(i, j, m_previousStatisticType));
						else
							output[i][j] = input[i][j][m_previousStatisticType];
					}
					else
					{
						output[i][j] = input[i][j];
					}
				}
			}
		}
	}

	ERMsg CAnalysisComputation::ComputeStatistic(const CNewSectionData& input, CNewSectionData& output)const
	{
		ERMsg msg;

		//CNewSectionData& computation = output;

		CTTransformation p(input.m_overAllPeriod, m_TM);

		//Resize the computation matrix with the number of cluster: a cluster is a temporal separation
		//for exemple a monthly OVERALL_YEARS will have 12 cluster
		output.resize(p.GetNbCluster(), input.GetCols(), p.GetClusterTRef(0));

		//for all cluster
		for (size_t i = 0; i < input.GetRows(); i++)
		{
			//define temporal reference;
			CTRef t(input.GetTRef(i));
			size_t c = p.GetCluster(t);

			//compute statistic for all variable
			for (size_t j = 0; j < input.GetCols(); j++)
			{
				if (input[i][j].IsInit())
				{
					int stat = m_previousStatisticType >= 0 ? m_previousStatisticType : MEAN;
					if (input.IsTemporalMatrix(j))
					{
						CTRef t = input.GetTRef(i, j, stat);
						output.AddTRef(c, j, t);
					}
					else
					{
						//only valid value must by in statistics
						ASSERT(input[i][j][stat] > VMISS);
						output[c][j] += input[i][j][stat];
					}
				}
			}
		}

		

		if (m_statisticType2 >= 0)
		{
			//we compute a specific value for a specific statistic
			//The number of value will be equal to 1
			for (size_t i = 0; i < output.GetRows(); i++)
			{
				for (size_t j = 0; j < output.GetCols(); j++)
				{
					if (output[i][j][NB_VALUE] > 0)
					{
						//if (output.IsTemporalMatrix(j))
						double value = output[i][j][m_statisticType2];
						output[i][j] = value;
					}
				}
			}
		}

		return msg;
	}

	ERMsg CAnalysisComputation::ComputeEvent(const CNewSectionData& input, CNewSectionData& output)const
	{
		ERMsg msg;

		CTTransformation p(input.m_overAllPeriod, m_TM);

		//Get the number of cluster: a cluster is a temporal separation
		//for exemple a monthly OVERALL_YEARS will have 12 cluster
		size_t nbCluster = p.GetNbCluster();//m_TType, m_TMode);

		output.resize(nbCluster, input.GetCols(), p.GetClusterTRef(0));
		//Set output time mode of the data: the output data TM is the same as the 
		//input vector TM
		//output.SetDataTM(input.m_overAllPeriod.GetTM() ); pas obliger s'init tout seul
		//init cluster reference
		//for all cluster
		for (size_t c = 0; c < nbCluster; c++)
		{
			for (size_t j = 0; j < input.GetCols(); j++)
			{
				CTRef ref = ExtractEvent(p, input, c, j);
				if (ref.IsInit())
					output.AddTRef(c, j, ref);
			}
		}

		return msg;
	}


	CTRef CAnalysisComputation::ExtractEvent(const CTTransformation& p, const CNewSectionData& input, size_t c, size_t j)const
	{
		ASSERT(input.GetRows() >= 1);
		ASSERT(j >= 0 && j < input.GetCols());

		CTRef ref;

		size_t nbRows = input.GetRows();
		bool bContinue = true;
		CStatistic peakMin;
		CStatistic peakMax;

		double cumul = 0;
		CStatistic statCluster;


		//int firstCluster_i = -1;
		if (m_eventType == FT_CUMUL_GT_K || m_eventType == FT_NON_CUMUL_GT_K)
		{
			for (size_t i = 0; i < nbRows; i++)
				if (p.GetCluster(input.GetTRef(i)) == c && input[i][j][NB_VALUE]>0)
					if (m_previousStatisticType >= 0)
						statCluster += input[i][j][m_previousStatisticType];
					else
						statCluster += input[i][j];

			if (statCluster[SUM] == 0)
				return ref;
		}

		size_t lastCluster_i = UNKNOWN_POS;
		if (m_eventType == STABILISED_K)
		{
			for (size_t i = 0; i < nbRows; i++)
			{
				if (p.GetCluster(input.GetTRef(i)) == c)
					lastCluster_i = i;
			}
		}

		//for all row
		for (size_t i = 0; i < nbRows&&bContinue; i++)
		{
			bool bCluster_ij = p.GetCluster(input.GetTRef(i)) == c;
			bool bValid_ij = bCluster_ij && input[i][j].IsInit();
			CStatistic value_ij = (m_previousStatisticType >= 0) ? input[i][j][m_previousStatisticType] : input[i][j];

			bool bCluster_nrowj = p.GetCluster(input.GetTRef(nbRows - i - 1)) == c;
			bool bValid_nrowj = bCluster_nrowj && input[nbRows - i - 1][j][NB_VALUE] > 0;
			CStatistic value_nrowj = (m_previousStatisticType >= 0) ? input[nbRows - i - 1][j][m_previousStatisticType] : input[nbRows - i - 1][j];

			//Extrac event
			switch (m_eventType)
			{
			case TW_MAX:  // date of maximum
				if (bValid_ij && (value_ij[MEAN] > peakMax[MEAN]))
				{
					peakMax = value_ij;
					ref = input.GetTRef(i);
				}
				break;

			case TW_MIN: //Date of minimum
				if (bValid_ij && (value_ij[MEAN] < peakMin[MEAN]))
				{
					peakMin = value_ij;
					ref = input.GetTRef(i);
				}
				break;

			case FT_GT_K: // First time greater than K
				if (bValid_ij && (value_ij[MEAN] >= m_K))
				{
					bContinue = false;
					ref = input.GetTRef(i);
				}
				break;

			case FT_LT_K: // First time lesser than K
				if (bValid_ij && (value_ij[MEAN] <= m_K))
				{
					bContinue = false;
					ref = input.GetTRef(i);
				}
				break;

			case LT_GT_K: // last time greater than K
				if (bValid_nrowj && (value_nrowj[MEAN] >= m_K))
				{
					bContinue = false;
					ref = input.GetTRef(nbRows - i - 1);
				}
				break;

			case LT_LT_K: // last time lesser than K
				if (bValid_nrowj && (value_nrowj[MEAN] <= m_K))
				{
					bContinue = false;
					ref = input.GetTRef(nbRows - i - 1);
				}
				break;


			case FT_CUMUL_GT_K: // First time when cumulative value (%) >= K

				if (bValid_ij)
					cumul += 100 * value_ij[SUM] / statCluster[SUM];

				if (cumul >= m_K)
				{
					ref = input.GetTRef(i);
					bContinue = false;
				}

				break;

			case FT_NON_CUMUL_GT_K: // First time when value (in % of the highest value) >= K

				if (bValid_ij && 100 * value_ij[HIGHEST] / statCluster[HIGHEST] >= m_K)
				{
					ref = input.GetTRef(i);
					bContinue = false;
				}

				break;

			case STABILISED_K: //Time when data stabilised (with tolerence K)
				ASSERT(lastCluster_i != UNKNOWN_POS && i >= lastCluster_i);
				{
					CStatistic lastCluster = (m_previousStatisticType >= 0) ? input[lastCluster_i][j][m_previousStatisticType] : input[lastCluster_i][j];

					if (bValid_nrowj && (fabs(lastCluster[MEAN] - value_nrowj[MEAN]) > m_K))
					{
						ref = input.GetTRef(nbRows - i - 1);
						bContinue = false;
					}
					else if (i == nbRows - 1)
					{
						//value is constant
						//if only one value???
						ref = input.GetTRef(0);
						bContinue = false;
					}

					break;
				}
			default: ASSERT(false);
			}
		}

		//reset mode when drop years is indicated
		if (m_bDropYear)
			ref.m_mode = CTRef::OVERALL_YEARS;

		return ref;
	}
	//**********************************************************************
	//CAnalysisWindow

	const char* CAnalysisWindow::XML_FLAG = "AnalysisWindow";
	const char* CAnalysisWindow::MEMBER_NAME[NB_MEMBER] = { "SelectLocation", "LocationSelection", "SelectTime", "TimeSelection", "UseCurrentDate-First-", "Shift-First-", "UseCurrentDate-Last-", "Shift-Last-", "SelectVariable", "VariableSelectionArray", "SelectParameters", "ParametersSelection" };

	CAnalysisWindow::CAnalysisWindow()
	{
		Reset();
	}

	CAnalysisWindow::CAnalysisWindow(const CAnalysisWindow& in)
	{
		operator=(in);
	}

	void CAnalysisWindow::Reset()
	{
		//spatial
		m_bSelectLocation = false;
		m_locations.clear();

		//temporal
		m_bSelectPeriod = false;
		m_period.Reset();
		m_bUseCurrentDate1 = false;
		m_shift1 = 0;
		m_bUseCurrentDate2 = false;
		m_shift2 = 0;

		//parameters 
		m_bSelectParametersVariations = false;
		m_parametersVariations.clear();

		//Variable
		m_bSelectVariable = false;
		m_variables.clear();

	}

	CAnalysisWindow& CAnalysisWindow::operator =(const CAnalysisWindow& in)
	{
		if (&in != this)
		{
			m_bSelectLocation = in.m_bSelectLocation;
			m_locations = in.m_locations;
			m_bSelectPeriod = in.m_bSelectPeriod;
			m_period = in.m_period;
			m_bUseCurrentDate1 = in.m_bUseCurrentDate1;
			m_shift1 = in.m_shift1;
			m_bUseCurrentDate2 = in.m_bUseCurrentDate2;
			m_shift2 = in.m_shift2;
			m_bSelectParametersVariations = in.m_bSelectParametersVariations;
			m_parametersVariations = in.m_parametersVariations;
			m_bSelectVariable = in.m_bSelectVariable;
			m_variables = in.m_variables;
		}

		return *this;
	}
	bool CAnalysisWindow::operator == (const CAnalysisWindow& in)const
	{
		bool bEqual = true;

		if (m_bSelectLocation != in.m_bSelectLocation) bEqual = false;
		if (m_locations != in.m_locations) bEqual = false;
		if (m_bSelectPeriod != in.m_bSelectPeriod) bEqual = false;
		if (m_period != in.m_period) bEqual = false;
		if (m_bUseCurrentDate1 != in.m_bUseCurrentDate1) bEqual = false;
		if (m_shift1 != in.m_shift1) bEqual = false;
		if (m_bUseCurrentDate2 != in.m_bUseCurrentDate2) bEqual = false;
		if (m_shift2 != in.m_shift2) bEqual = false;
		if (m_bSelectParametersVariations != in.m_bSelectParametersVariations) bEqual = false;
		if (m_parametersVariations != in.m_parametersVariations) bEqual = false;
		if (m_bSelectVariable != in.m_bSelectVariable) bEqual = false;
		if (m_variables != in.m_variables) bEqual = false;

		return bEqual;

	}

	CTPeriod CAnalysisWindow::GetFinalPeriod()const
	{
		CTRef today = CTRef::GetCurrentTRef();
		CTPeriod period = m_period;

		if (m_bUseCurrentDate1)
			period.Begin() = today + m_shift1;

		if (m_bUseCurrentDate2)
			period.End() = today + m_shift2;

		return period;
	}


	//*******************************************************************************
	const char* CAnalysis::XML_FLAG = "Analysis";
	const char* CAnalysis::MEMBERS_NAME[NB_MEMBERS_EX] = { CAnalysisWindow::GetXMLFlag(), CAnalysisComputation::GetXMLFlag() };
	//const int CAnalysis::CLASS_NUMBER = CExecutableFactory::RegisterClass(CAnalysis::GetXMLFlag(), CExecutablePtr(new CAnalysis()) );
	const int CAnalysis::CLASS_NUMBER = CExecutableFactory::RegisterClass(CAnalysis::GetXMLFlag(), &CAnalysis::CreateObject);


	CAnalysis::CAnalysis()
	{
		ClassReset();
	}

	CAnalysis::~CAnalysis()
	{}

	CAnalysis::CAnalysis(const CAnalysis& in)
	{
		operator=(in);
	}

	void CAnalysis::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CAnalysis::ClassReset()
	{
		m_name = "Analysis";
		m_window.Reset();
		m_computation.Reset();
	}

	CAnalysis& CAnalysis::operator =(const CAnalysis& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);

			m_window = in.m_window;
			m_computation = in.m_computation;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CAnalysis::operator == (const CAnalysis& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_window != in.m_window)bEqual = false;
		if (m_computation != in.m_computation)bEqual = false;

		return bEqual;
	}


	CDimension CAnalysis::GetOutputDimension(const CResultPtr& result)const
	{
		CDimension dimension = result->GetDimension();

		bool bFC = true;


		CDimension d;
		d[LOCATION] = bFC&&m_computation.m_bMeanOverLocation ? 1 : m_window.m_bSelectLocation ? CVariableSelectionVector(m_window.m_locations).count() : dimension[LOCATION];
		d[PARAMETER] = bFC&&m_computation.m_bMeanOverParameterSet ? 1 : m_window.m_bSelectParametersVariations ? CVariableSelectionVector(m_window.m_parametersVariations).count() : dimension[PARAMETER];
		d[REPLICATION] = bFC&&m_computation.m_bMeanOverReplication ? 1 : dimension[REPLICATION];

		CTTransformation t(result->GetTPeriod(), m_computation.m_TM);
		d[TIME_REF] = m_computation.m_bSelectTimeTransformation ? t.GetNbCluster() : dimension[TIME_REF];
		d[VARIABLE] = m_window.m_bSelectVariable ? CVariableSelectionVector(m_window.m_variables).count() : dimension[VARIABLE];


		return d;
	}

	ERMsg CAnalysis::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		if (filter[VARIABLE])
			filter.set(TIME_REF);

		msg = m_pParent->GetParentInfo(fileManager, info, filter);
		if (msg)
		{
			CTPeriod pIn = info.m_period;
			if (filter[LOCATION])
			{
				info.m_locations = CleanLocations(info.m_locations);
			}

			if (filter[PARAMETER])
			{
				info.m_parameterset = CleanParameterset(info.m_parameterset);
			}

			if (filter[REPLICATION])
			{
				info.m_nbReplications = CleanReplication(info.m_nbReplications);
			}

			if (filter[TIME_REF])
			{

				info.m_period = CleanTimePeriod(info.m_period);
			}

			if (filter[VARIABLE])
			{
				CTM TM = CTM(pIn.GetTM().Type(), m_computation.m_bDropYear ? CTRef::OVERALL_YEARS : pIn.GetTM().Mode());
				info.m_variables = CleanVariables(TM, info.m_variables);
			}
		}

		return msg;
	}

	//ERMsg CAnalysis::GetLocationList(const CFileManager& fileManager, CLocationVector& loc)const
	//{
	//	ENSURE(m_pParent);
	//
	//	ERMsg msg;
	//
	//	CLocationVector locTmp;
	//	msg = m_pParent->GetLocationList(fileManager, locTmp);
	//	if( msg)
	//		CleanLocations(locTmp, loc);
	//
	//	return msg;
	//}
	//
	////int CAnalysis::GetReplication()const
	//ERMsg CAnalysis::GetReplication(const CFileManager& fileManager, size_t& nbReplication)const
	//{
	//	ENSURE(m_pParent);
	//
	//	ERMsg msg;
	//	msg = m_pParent->GetReplication(fileManager, nbReplication);
	//	nbReplication = CleanReplication(nbReplication);
	//	return msg;
	//}
	//
	//ERMsg CAnalysis::GetDefaultPeriod(const CFileManager& fileManager, CTPeriod& period)const
	//{
	//	ENSURE(m_pParent);
	//
	//	ERMsg msg;
	//
	//	CTPeriod p;
	//	msg = m_pParent->GetDefaultPeriod(fileManager, p);
	//	if( msg)
	//		period = CleanTimePeriod(p);
	//
	//	return msg;
	//}
	//
	//ERMsg CAnalysis::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	//{
	//	ENSURE(m_pParent);
	//
	//	ERMsg msg;
	//
	//	CModelOutputVariableDefVector outputVarTmp;
	//	msg = m_pParent->GetOutputDefinition(fileManager, outputVarTmp);
	//	if( msg)
	//		CleanVariables(outputVarTmp, outputVar);
	//
	//	return msg;
	//}

	/*ERMsg CAnalysis::GetDimensionList(const CFileManager& fileManager, int dim, StringVector& list)const
	{
	ASSERT( dim>=0 && dim<DIMENSION::NB_DIMENSION);
	ASSERT( m_pParent );

	ERMsg msg;

	//est-ce qu'il serait mieux de retoruner l'élément original, ex. un loc et non une liste de nom???
	//ca serais plus facile ici???
	msg = m_pParent->GetDimensionList(fileManager, dim, list);
	if( !msg)
	return msg;

	switch( dim)
	{
	case DIMENSION::LOCATION:
	{
	CLocationVector loc;
	loc.resize(list.size());
	for(int i=0; i<list.size(); i++)
	loc[i].SetName(list[i]);

	CLocationVector locOut;
	CleanLocations(loc, locOut);
	list.resize(locOut.size());

	for(int i=0; i<locOut.size(); i++)
	list[i] = locOut[i].GetName();

	}break;

	case DIMENSION::PARAMETER: ASSERT(false); //Is it used???
	case DIMENSION::REPLICATION:
	{
	ASSERT( list.size() == 1);
	list[0] = ToString(CleanReplication(ToInt(list[0])));
	}break;

	case DIMENSION::TIME_REF:
	{
	ASSERT( list.size() == 2 );

	short TM = CleanTimeTM(ToShort(list[0]));
	list[0] = ToString(TM);

	CTPeriod p;
	p.FromString( (LPCTSTR)list[1]);
	p = CleanTimePeriod(p);
	list[1] = p.ToString().c_str();
	}break;

	case DIMENSION::VARIABLE:
	{
	//Variable list
	CModelOutputVariableDefVector variablesDef;
	variablesDef.resize(list.size());
	for(int i=0; i<list.size(); i++)
	variablesDef[i].m_name = list[i];

	CModelOutputVariableDefVector variablesDefOut;
	CleanVariables(variablesDef, variablesDefOut);
	list.resize(variablesDefOut.size());

	for(int i=0; i<variablesDefOut.size(); i++)
	list[i] = variablesDefOut[i].m_name;

	}break;
	default: ASSERT(false);
	}

	return msg;
	}
	*/
	ERMsg CAnalysis::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ASSERT(GetParent());
		ASSERT(GetExecute());

		ERMsg msg;

		msg = DoAnalysis(fileManager, callback);

		return msg;
	}

	ERMsg CAnalysis::DoAnalysis(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		CResultPtr result = GetParent()->GetResult(fileManager);
		msg = result->Open();
		if (msg)
		{

			CResult analysisDB;
			msg = analysisDB.Open(GetDBFilePath(GetPath(fileManager)), std::fstream::binary | std::fstream::out | std::fstream::trunc);

			if (msg)
			{
				GetDBInputInfo(result, analysisDB.GetMetadata());
				msg = DoAnalysis(result, analysisDB, callback);
				analysisDB.Close();
			}
		}

		return msg;
	}



	CVariableSelectionVector to_object(std::string &buffer)
	{
		return CVariableSelectionVector(buffer.begin(), buffer.end());
	}


	typedef multi_array<CNewSectionData, 2> CLocationStat;
	ERMsg CAnalysis::DoAnalysis(CResultPtr& result, CResult& analysisDB, CCallback& callback)
	{
		ERMsg msg;

		CDimension dimIn = result->GetDimension();
		CDimension dimOut = GetOutputDimension(result);

		callback.AddMessage(GetString(IDS_SIM_READ_FROM));
		callback.AddMessage(result->GetFilePath(), 1);
		callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
		callback.AddMessage(analysisDB.GetFilePath(), 1);
		

		//only use in mean over location.
		CLocationStat meanOverlocationStat(extents[dimOut[PARAMETER]][dimOut[REPLICATION]]);

		CVariableSelectionVector locations(dimIn[LOCATION]);
		if (m_window.m_bSelectLocation)
			locations = CVariableSelectionVector(m_window.m_locations);
		else
			locations.set();

		//find parameters variation to evaluate
		CVariableSelectionVector parametersVariation(dimIn[PARAMETER]);
		if (m_window.m_bSelectParametersVariations)
			parametersVariation = CVariableSelectionVector(m_window.m_parametersVariations);
		else
			parametersVariation.set();

		//find variables to evaluate
		CVariableSelectionVector variables(dimIn[VARIABLE]);
		if (m_window.m_bSelectVariable)
			variables = CVariableSelectionVector(m_window.m_variables);
		else
			variables.set();

		size_t test = variables.count();
		//find period to evaluate
		CTPeriod period;
		if (m_window.m_bSelectPeriod)
			period = m_window.GetFinalPeriod();


		//adjust input dimension to selection
		dimIn[LOCATION] = locations.size();
		dimIn[PARAMETER] = parametersVariation.size();
		dimIn[VARIABLE] = variables.size();

		callback.PushTask(GetString(IDS_SIM_DOANALYSE), dimIn[LOCATION] * dimIn[PARAMETER] * dimIn[REPLICATION]);


		size_t test2 = locations.count();
		//Compute statistics
		//#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads)
		for (size_t i = locations.find_first(), I = 0; i != CVariableSelectionVector::npos; i = locations.find_next(i), I++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				CLocationStat locationStat(extents[dimOut[PARAMETER]][dimOut[REPLICATION]]);
				for (size_t j = parametersVariation.find_first(), J = 0; j != CVariableSelectionVector::npos; j = parametersVariation.find_next(j), J++)
				{
					for (size_t k = 0; k < dimIn[REPLICATION] && msg; k++)
					{
						CNewSectionData section;

						//Get section is protected with singleLock
						result->GetSection(i, j, k, section);

						section.CleanUp(period);
						section.CleanUp(variables);

						if (m_computation.m_bMeanOverParameterSet)
							J = 0;

						size_t K = (m_computation.m_bMeanOverReplication) ? 0 : k;


						msg += m_computation.Compute(section, locationStat[J][K]);
						ASSERT(locationStat[J][K].GetRows() == 0 || locationStat[J][K][0].size() == section[0].size());

						callback.WaitPause();

#pragma omp critical(stepIt)
						{
#pragma omp flush(msg)
							if (msg)
								msg += callback.StepIt();
#pragma omp flush(msg)
						}
					}
				}

				//if we don't mean over location, then we save for each location
				if (m_computation.m_bMeanOverLocation)
				{
#pragma omp critical(WritetResult)
				{
					for (size_t J = 0; J < locationStat.size(); J++)
					{
						for (size_t K = 0; K < locationStat[J].size(); K++)
						{
							meanOverlocationStat[J][K].AppendRows(locationStat[J][K]);
						}
					}
				}
				}
				else
				{
					for (size_t J = 0; J < locationStat.size(); J++)
					{
						for (size_t K = 0; K < locationStat[J].size(); K++)
						{
							size_t sectionNo = analysisDB.GetSectionNo(I, J, K);
							analysisDB.SetSection(sectionNo, locationStat[J][K]);
						}
					}
				}
			}
		}

		//if mean over location, save now
		if (msg  && m_computation.m_bMeanOverLocation)
		{
			for (size_t J = 0; J < meanOverlocationStat.size(); J++)
			{
				for (size_t K = 0; K < meanOverlocationStat[J].size(); K++)
				{
					//Save result to disk
					analysisDB.AddSection(meanOverlocationStat[J][K]);
				}
			}
		}

		callback.PopTask();

		return msg;
	}

	CLocationVector CAnalysis::CleanLocations(const CLocationVector& inputLOC)const
	{
		CLocationVector outputLOC;

		if (m_computation.m_bMeanOverLocation)
		{
			outputLOC.resize(1);
			outputLOC[0].m_name = "Mean over location";
		}
		else if (m_window.m_bSelectLocation)
		{
			CVariableSelectionVector selection(m_window.m_locations);
			outputLOC.resize(selection.count());

			for (size_t i = selection.find_first(), I = 0; i != UNKNOWN_POS; i = selection.find_next(i), I++)
			{
				if (i < inputLOC.size())
					outputLOC[I] = inputLOC[i];
				//{
				//	ERMsg error;
				//	error.ajoute(GetString(IDS_SIM_INVALID_LOC_POSITION));
				//	throw(error);
				//}

			}
		}
		else
		{
			outputLOC = inputLOC;
		}

		return outputLOC;
	}

	CModelInputVector CAnalysis::CleanParameterset(const CModelInputVector& in)const
	{
		CModelInputVector out;

		if (m_computation.m_bMeanOverParameterSet)
		{
			out.resize(1);
			out[0].SetName("Mean over parameterset");
		}
		else if (m_window.m_bSelectParametersVariations)
		{
			CVariableSelectionVector selection(m_window.m_parametersVariations);
			out.resize(selection.count());

			for (size_t i = selection.find_first(), I = 0; i != UNKNOWN_POS; i = selection.find_next(i), I++)
			{
				if (i < in.size())
					out[I] = in[i];
			}
		}
		else
		{
			out = in;
		}

		return out;
	}

	size_t CAnalysis::CleanReplication(size_t nbRep)const
	{
		if (m_computation.m_bMeanOverReplication)
		{
			nbRep = 1;
		}

		return nbRep;
	}

	CTM CAnalysis::CleanTimeTM(CTM TM)const
	{
		if (m_computation.m_bSelectTimeTransformation)
		{
			TM = m_computation.m_TM;
		}

		return TM;
	}

	CTPeriod CAnalysis::CleanTimePeriod(CTPeriod p)const
	{
		if (m_window.m_bSelectPeriod)
		{
			//p=m_window.m_period;
			p = m_window.GetFinalPeriod();

		}

		if (m_computation.m_bSelectTimeTransformation)
			p.Transform(m_computation.m_TM);

		return p;
	}

	CModelOutputVariableDefVector CAnalysis::CleanVariables(CTM dataTM, const CModelOutputVariableDefVector& varListIn)const
	{
		CModelOutputVariableDefVector varListOut;

		CVariableSelectionVector variables(varListIn.size());
		if (m_window.m_bSelectVariable)
			variables = CVariableSelectionVector(m_window.m_variables);
		else
			variables.set();

		varListOut.resize(variables.count());
		for (size_t i = variables.find_first(), I = 0; i < variables.size(); i = variables.find_next(i), I++)
		{
			assert(variables[i]);
			if (i < varListIn.size())
			{
				varListOut[I] = varListIn[i];
				if (m_computation.m_bSelectTimeTransformation)
				{
					if (m_computation.m_kind == CAnalysisComputation::EVENT)
					{
						varListOut[I].m_TM = dataTM;
					}
				}
			}
		}


		return varListOut;
	}

	void CAnalysis::GetDBInputInfo(CResultPtr& pResult, CDBMetadata& inputInfo)
	{
		const CDBMetadata& metadata = pResult->GetMetadata();
		CTM TM = CTM(metadata.GetTPeriod().GetTM().Type(), m_computation.m_bDropYear ? CTRef::OVERALL_YEARS : metadata.GetTPeriod().GetTM().Mode());

		inputInfo = metadata;
		inputInfo.SetLocations(CleanLocations(metadata.GetLocations()));
		inputInfo.SetParameterSet(CleanParameterset(metadata.GetParameterSet()));
		inputInfo.SetNbReplications(CleanReplication(inputInfo.GetNbReplications()));
		inputInfo.SetOutputDefinition(CleanVariables(TM, metadata.GetOutputDefinition()));

		inputInfo.SetTPeriod(CTPeriod());

	}



	void CAnalysis::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(XML_WINDOW)](GetWindow());
		out[GetMemberName(XML_EVENT)](GetComputation());
	}

	bool CAnalysis::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(XML_WINDOW)](GetWindow());
		in[GetMemberName(XML_EVENT)](GetComputation());


		return true;
	}
}