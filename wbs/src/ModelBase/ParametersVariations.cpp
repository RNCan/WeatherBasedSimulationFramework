//****************************************************************************
// File:	CParameterVariation.cpp
// Class:	CParameterVariation
//****************************************************************************
// 01/03/2014  Rémi Saint-Amant	Add ListByString
// 15/09/2008  Rémi Saint-Amant	CSimulationVariation rename as CParametersVariations
// 15/09/2008  Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"
#include "Basic\UtilMath.h"
#include "ModelBase\ParametersVariations.h"
#include "ModelBase\ModelInput.h"
#include "ModelBase\WGInput.h"

namespace WBSF
{



	const char* CParameterVariation::XML_FLAG = "Parameter";
	const char* CParameterVariation::MEMBER_NAME[NB_MEMBER] = { "Name", /*"Pos",*/ "Active", "Type", "Minimum", "Maximum", "Step" };


	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CParameterVariation::CParameterVariation()
	{
		Reset();
	}

	void CParameterVariation::Reset()
	{
		m_bActive = false;
		//m_pos = 0;
		m_type = CModelInputParameterDef::kMVReal;
		m_min = 0;
		m_max = 0;
		m_step = 0;

	}

	CParameterVariation::CParameterVariation(const CParameterVariation& in)
	{
		operator=(in);
	}

	CParameterVariation::CParameterVariation(short pos, const CModelInputParameterDef& in)
	{
		Init(pos, in);
	}

	void CParameterVariation::Init(short pos, const CModelInputParameterDef& in)
	{
		//m_pos = pos;
		m_type = in.GetType();
		m_name = in.m_name;

		switch (m_type)
		{
		case CModelInputParameterDef::kMVBool:
		{
			m_min = 0;
			m_max = 1;
			m_step = 1;
			break;
		}

		case CModelInputParameterDef::kMVInt:
		case CModelInputParameterDef::kMVReal:
		{
			m_min = ToFloat(in.m_min);
			m_max = ToFloat(in.m_max);
			m_step = (m_max - m_min) / DEFAULT_STEP;

			break;
		}

		case CModelInputParameterDef::kMVListByPos:
		case CModelInputParameterDef::kMVListByString:
		{
			ASSERT(in.GetNbItemList() >= 1);
			m_min = 0;
			m_max = (float)in.GetNbItemList() - 1;
			m_step = 1;
			break;
		}

		default: ASSERT(false);
		}
	}

	/*void CParameterVariation::Serialize (CArchive& ar)
	{
	if ( ar.IsStoring() )
	{
	ar << m_name;
	ar << m_min;
	ar << m_max;
	ar << m_step;
	ar << (long) m_type;

	}
	else
	{
	ar >> m_name;
	ar >> m_min;
	ar >> m_max;
	ar >> m_step;
	long tmp;
	ar >> tmp; m_type = (CModelInputParameterDef::MInVarType)tmp;
	}
	}
	*/
	CParameterVariation& CParameterVariation::operator = (const CParameterVariation& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			//m_pos = in.m_pos;
			m_bActive = in.m_bActive;
			m_type = in.m_type;
			m_min = in.m_min;
			m_max = in.m_max;
			m_step = in.m_step;
		}

		ASSERT(operator==(in));

		return *this;
	}

	bool CParameterVariation::operator == (const CParameterVariation& in)const
	{
		bool bEqual = true;

		if (in.m_name != m_name) bEqual = false;
		//if (in.m_pos != m_pos) bEqual = false;
		if (in.m_bActive != m_bActive) bEqual = false;
		if (in.m_type != m_type)bEqual = false;
		if (in.m_min != m_min) bEqual = false;
		if (in.m_max != m_max) bEqual = false;
		if (in.m_step != m_step)bEqual = false;


		return bEqual;
	}



	bool CParameterVariation::IsValid()const
	{
		bool bValid = true;
		if (m_bActive)
		{
			switch (m_type)
			{
			case CModelInputParameterDef::kMVBool:
			{
				if (m_min != 0) bValid = false;
				if (m_max != 1)  bValid = false;
				if (m_step != 1) bValid = false;
				break;
			}

			case CModelInputParameterDef::kMVInt:
			case CModelInputParameterDef::kMVReal:
			{
				if (m_max - m_min < 0) bValid = false;
				if (m_step == 0) bValid = false;
				break;
			}

			case CModelInputParameterDef::kMVListByPos:
			case CModelInputParameterDef::kMVListByString:
			{
				if (m_min != 0)  bValid = false;
				if (m_max == 0)  bValid = false;
				if (m_step != 1) bValid = false;
				break;
			}
			default: ASSERT(false);
			}
		}
		return bValid;
	}

	/*
	std::string CParameterVariation::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	std::string str;
	switch(i)
	{
	case POSITION: str = ToString(m_pos); break;
	case TYPE: str = ToString(m_type); break;
	case MINIMUM: str = ToString(m_min,-1); break;
	case MAXIMUM: str = ToString(m_max,-1); break;
	case STEP: str = ToString(m_step,-1); break;
	default:ASSERT(false);
	}

	return str;
	}

	void CParameterVariation::SetMember(int i, const std::string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case POSITION: m_pos=ToShort(str); break;
	case TYPE: m_type = ToShort(str); break;
	case MINIMUM: m_min = ToFloat(str); break;
	case MAXIMUM: m_max= ToFloat(str); break;
	case STEP: m_step= ToFloat(str); break;
	default:ASSERT(false);
	}

	}
	*/


	//void CParameterVariation::GetXML(XNode& root)const
	//{
	//	
	//
	//	XNode& xml = *root.AppendChild(XMLFlag);
	//
	//	for(int i=0; i<NB_MEMBER; i++)
	//	{
	//		xml.AppendChild(GetMemberName(i), GetString(i) );
	//	}
	//}
	//
	//void CParameterVariation::SetXML(const XNode& root)
	//{
	//	LPXNode pNode = root.Select(XMLFlag);
	//	if( pNode )
	//	{
	//		XNode& xml = *pNode;
	//		ASSERT( xml.GetChildCount() >= NB_MEMBER );
	//	
	//		for(int i=0; i<NB_MEMBER; i++)
	//		{
	//			SetString( i, xml.GetChildValue(GetMemberName(i)) ); 
	//		}
	//
	//		//m_varArray.SetXML(xml);
	//	}
	//}


	//***********************************************************************
	//CParametersVariationsDefinition
	//const char* CParametersVariationsDefinition::XML_FLAG = "SimulationVariations";
	//const char* _CParametersVariationsDefinition::XML_FLAG = "VariationArray";
	const char* CParametersVariationsDefinition::FILE_EXT = ".pvd";
	const char* CParametersVariationsDefinition::MEMBER_NAME[NB_MEMBER] = { "Type", "NbVariations", "Parameters" };

	CParametersVariationsDefinition::CParametersVariationsDefinition()
	{
		Reset();
	}

	void CParametersVariationsDefinition::Reset()
	{
		clear();
		m_variationType = RANDOM_VARIATION;
		m_nbVariation = 1;
	}

	CParametersVariationsDefinition& CParametersVariationsDefinition::operator = (const CParametersVariationsDefinition& in)
	{
		if (&in != this)
		{
			CParamVariationsContainer::operator=(in);
			m_variationType = in.m_variationType;
			m_nbVariation = in.m_nbVariation;
		}

		return *this;
	}
	
	bool CParametersVariationsDefinition::operator == (const CParametersVariationsDefinition& in)const
	{
		bool bEqual = true;
		
		if (!std::equal(begin(), end(), in.begin())) bEqual = false;
		if (m_variationType != in.m_variationType) bEqual = false;
		if (m_nbVariation != in.m_nbVariation) bEqual = false;

		return bEqual;
	}

	size_t CParametersVariationsDefinition::GetNbVariation()const
	{
		size_t nbVariation = 1;
		if (size() > 0)
		{
			if (m_variationType == RANDOM_VARIATION)
			{
				nbVariation = m_nbVariation;
			}
			else
			{
				for (size_t i = 0; i < size(); i++)
				{
					if (at(i).m_bActive)
					{
						ASSERT(at(i).GetStep() != 0);
						int tmp = (int)((at(i).GetMax() - at(i).GetMin()) / at(i).GetStep()) + 1;
						nbVariation *= tmp;
					}
				}
			}
		}

		return nbVariation;
	}

	void CParametersVariationsDefinition::GenerateParameter(CFloatMatrix& paramArray)const
	{
		paramArray.clear();
		ASSERT(ParameterIsValid());

		if (size() > 0)
		{
			if (m_variationType == RANDOM_VARIATION)
			{
				GenerateRandomParameter(paramArray);
			}
			else
			{
				GenerateRegularParameter(paramArray);
			}
		}
	}

	void CParametersVariationsDefinition::GenerateRegularParameter(CFloatMatrix& paramArray)const
	{
		ASSERT(size() != 0);
		
		size_t nbTotalVariations = 1;
		CFloatMatrix vecteur;
		//vector<std::pair<size_t, size_t>> nbVariations;
		for (size_t i = 0; i < size(); i++)
		{
			if (at(i).m_bActive)
			{
				ASSERT(at(i).GetStep() != 0);

				size_t nbVariation = (size_t)((at(i).GetMax() - at(i).GetMin()) / at(i).GetStep()) + 1;
				//nbVariations.push_back(std::make_pair(i,nbVariation));
				vector<float> tmp;
				for (size_t j = 0; j < nbVariation; j++)
					tmp.push_back( at(i).GetMin() + (at(i).GetStep()*j));
					
				
				vecteur.push_back(tmp);
				nbTotalVariations *= nbVariation;
			}
			
		}

		
		//vecteur.resize(nbVariations.size());

		//for (size_t i = 0; i < nbVariations.size(); i++)
		//{
		//	for (size_t j = 0; j < nbVariations[i].second; j++)
		//	{
		//		vecteur[i].push_back(at(nbVariations[i].first).GetMin() + (at(nbVariations[i].first).GetStep()*j));
		//	}
		//}


		paramArray.resize(nbTotalVariations);
		for (size_t i = 0; i < paramArray.size(); i++)
			paramArray[i].resize(vecteur.size());

		size_t c = 0;
		for (size_t p = 0; p < paramArray.size(); p++)
		{
			size_t pp = p;
			for (size_t v = 0 ; v < vecteur.size(); v++)
			{
				size_t vv = pp%vecteur[v].size();
				
				paramArray[p][v] = vecteur[v][vv];
				vv += vecteur[v].size();
				
				pp /= vecteur[v].size();
			}
		}

		ASSERT(paramArray.size() != 0);
	}

	void CParametersVariationsDefinition::GenerateRandomParameter(CFloatMatrix& paramArray)const
	{
		ASSERT(size() != 0);
		ASSERT(m_nbVariation != 0);
		//unsigned seed = (unsigned)time( NULL );

		CRandomGenerator randNum;

		//Randomize(seed);


		//const CParametersVariationsDefinition& m_varArray = *this;
		//	const CParametersVariationsDefinition& parameter
		
		

		
		
		size_t  nbVar = 0;
		for (size_t i = 0; i < size(); i++)
		{
			if (at(i).m_bActive)
			{
				nbVar++;
			}
		}
		

		paramArray.resize(m_nbVariation);
		for (size_t v = 0; v < m_nbVariation; v++)
		{
			paramArray[v].resize(nbVar);

			size_t ii = 0;
			for (size_t i = 0; i < m_nbVariation; i++)
			{
				if (at(i).m_bActive)
				{
					

					switch (at(i).GetType())
					{
					case CModelInputParameterDef::kMVReal:
					{
						float min = at(i).GetMin();
						float max = at(i).GetMax();
						paramArray[v][ii] = float(randNum.Rand(min, max));
						break;
					}

					case CModelInputParameterDef::kMVBool:ASSERT(at(i).GetMax() == 1);
					case CModelInputParameterDef::kMVListByPos:
					case CModelInputParameterDef::kMVListByString:ASSERT(at(i).GetMin() == 0);
					case CModelInputParameterDef::kMVInt:
					{
						int min = at(i).GetMin();
						int max = at(i).GetMax();

						
						paramArray[v][ii] = (float)randNum.Rand(min, max);

						ASSERT(paramArray[v][ii] >= min && paramArray[v][ii] <= max);
						ASSERT((float)((int)paramArray[v][ii]) == paramArray[v][ii]);

						break;
					}

					default: ASSERT(false);
					}

					ii++;
				}
			}
		}

		ASSERT(paramArray.size() != 0);
	}


	bool CParametersVariationsDefinition::ParameterIsValid()const
	{
		bool bValid = true;
		for (int i = 0; i < size(); i++)
		{
			bValid &= at(i).IsValid();
		}


		return bValid;
	}


	void CParametersVariationsDefinition::GetModelInputArray(const CModelInput& modelInput, CModelInputVector& modelInputVector)const
	{
		modelInputVector = GetModelInputVector(modelInput);
	}

	CModelInputVector CParametersVariationsDefinition::GetModelInputVector(const CModelInput& modelInput)const
	{
		CModelInputVector modelInputVector(GetNbVariation());
		modelInputVector.m_pioneer = modelInput;
		modelInputVector.m_variation = *this;

		// create the parameters array. (one line for each comb.)
		CFloatMatrix paramArray;
		GenerateParameter(paramArray);

		for (int noVar = 0; noVar < GetNbVariation(); noVar++)
		{
			//copy the default model input
			modelInputVector[noVar] = modelInput;
			if (!paramArray.empty())
			{
				//change the variable 
				ASSERT(paramArray[noVar].size() < size());

				for (size_t i = 0, ii=0; i < size(); i++)
				{
					if (at(i).m_bActive)
					{
						ASSERT(ii<paramArray[noVar].size());
						//size_t pos = at(i).m_pos;
						if (i < modelInputVector[noVar].size())
							modelInputVector[noVar][i].SetValue(paramArray[noVar][ii]);

						ii++;
					}
				}
			}
		}

		return modelInputVector;
	}

	size_t CParametersVariationsDefinition::FindByPos(int pos)const
	{
		size_t index = UNKNOWN_POS;
		if (pos >= 0 && pos < size())
			index = pos;
		//for (size_t i = 0; i < size() && index == UNKNOWN_POS; i++)
			//if (at(i).m_pos == pos)//&& at(i).m_bActive
				//index = i;

		return index;
	}

	size_t CParametersVariationsDefinition::Find(const std::string& name)const
	{

		size_t index = UNKNOWN_POS;

		for (size_t i = 0; i < size() && index == UNKNOWN_POS; i++)
			if (IsEqual(at(i).m_name, name))
				index = i;

		return index;
	}

	void CParametersVariationsDefinition::RemoveInvalidVar(const CModelInput& modelInput)
	{
		
		for (iterator it = begin(); it != end();)
		{
			//size_t pos = it->m_pos;
			//ASSERT(pos < modelInput.size());
			size_t pos = std::distance(it, begin());

			if (pos < modelInput.size())
				it++;
			else
				it = erase(it);
		}
		
	}

	/*bool CParametersVariationsDefinition::GetVarPosition( const CModel& model, CIntArray& posArray)const
	{
	CModelInputParameterDefVector varList;
	model.GetInputDefinition(varList, true);

	posArray.clear();
	for(int i=0; i<m_varArray.size(); i++)
	{
	int nSize = varList.size();
	for(int j=0; j<nSize; j++)
	{
	//if( varList[j].GetType() != CModelInputVariableDef::kMVProtect &&
	//  varList[j].GetType() != CModelInputVariableDef::kMVLine )
	//            {
	if( varList[j].GetName() == m_varArray[i].GetName() )
	{
	posArray.Add(j);
	break;
	}

	//              k++;
	//        }
	}
	}

	ASSERT( posArray.size() == m_varArray.size());

	return true;
	}
	*/

	CModelInputVector CParametersVariationsDefinition::GetModelInputVector(const CWGInput& WGInput)const
	{
		CModelInput modelInput;

		for (size_t i = 0; i < CWGInput::NB_MEMBERS; i++)
		{
			modelInput.push_back(CModelInputParam(WGInput.GetMemberName(i), /*CModelInputParameterDef::kMVString,*/ WGInput[i]));
		}

		return GetModelInputVector(modelInput);
	}

	CParameterVector CParametersVariationsDefinition::GetParametersVector()const
	{
		CParameterVector params;

		for (size_t i = 0; i < size(); i++)
			params.push_back(CParameter(at(i).m_name, ToString((at(i).m_min + at(i).m_max) / 2), at(i).m_bActive));

		return params;
	}
}