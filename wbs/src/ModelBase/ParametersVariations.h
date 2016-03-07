//***************************************************************************
// File:        SimulationVariation.h
//
// class:		CParameterVariation
//
// Abstract:    container for advanced simulation parameter variation
//
// Description: 
//
// Note:        
//***************************************************************************
#pragma once

#include "ModelInputParameter.h"
#include "ModelInput.h"
#include "WGInput.h"

namespace WBSF
{

	class CParameterVariation
	{
	public:

		enum TVariation{ RANDOM_VARIATION, REGULAR_VARIATION };
		enum TMember{ VAR_NAME, POSITION, ACTIVE, TYPE, MINIMUM, MAXIMUM, STEP, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const int DEFAULT_STEP = 10;

		std::string m_name;
		size_t	m_pos;
		bool	m_bActive;
		size_t	m_type;
		double	m_min;
		double	m_max;
		double	m_step;

		CParameterVariation();
		CParameterVariation(const CParameterVariation& in);

		CParameterVariation(short pos, const CModelInputParameterDef& in);
		void Init(short pos, const CModelInputParameterDef& in);

		void Reset();
		CParameterVariation& operator = (const CParameterVariation& in);
		bool operator == (const CParameterVariation& in)const;
		bool operator != (const CParameterVariation& in)const{ return !operator==(in); }

		bool IsValid()const;

		size_t GetType()const{ return m_type; }
		void SetType(size_t type)
		{
			ASSERT(type >= 0 && type < CModelInputParameterDef::kMVNbType);
			m_type = type;

			m_min = GoodTrunck(m_min);
			m_max = GoodTrunck(m_max);
			m_step = GoodTrunck(m_step);
		}

		double GoodTrunck(double x)const{ return (m_type == CModelInputParameterDef::kMVReal) ? x : (double)((int)x); }

		double GetMin()const{ return m_min; }
		void SetMin(double min){ m_min = GoodTrunck(min); }

		double GetMax()const{ return m_max; }
		void SetMax(double max){ m_max = GoodTrunck(max); }

		double GetStep()const{ return m_step; }
		void SetStep(double step){ m_step = GoodTrunck(step); }

		void writeStruc(zen::XmlElement& output)const
		{
			//XmlOut out(output);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::VAR_NAME), m_name);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::POSITION), m_pos);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::ACTIVE), m_bActive);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::TYPE), m_type);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::MINIMUM), m_min);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::MAXIMUM), m_max);
			output.setAttribute(CParameterVariation::GetMemberName(CParameterVariation::STEP), m_step);
			output.setValue(m_name);
		}


		bool readStruc(const zen::XmlElement& input)
		{
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::VAR_NAME), m_name);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::POSITION), m_pos);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::ACTIVE), m_bActive);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::TYPE), m_type);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::MINIMUM), m_min);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::MAXIMUM), m_max);
			input.getAttribute(CParameterVariation::GetMemberName(CParameterVariation::STEP), m_step);
			input.getValue(m_name);

			return true;
		}

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};



	typedef std::vector< std::vector<float>> CFloatMatrix;

	typedef std::vector<CParameterVariation> CParamVariationsContainer;
	class CParametersVariationsDefinition : public CParamVariationsContainer
	{
	public:

		enum TVariation{ SYSTEMTIC_VARIATION , RANDOM_VARIATION, NB_VARIATIONS_TYPE};
		enum TMember{ TYPE, NB_VARIATIONS, PARAMETERS, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* FILE_EXT;

		//*** public member ***
		size_t	m_variationType;
		size_t	m_nbVariation;
		//*********************


		CParametersVariationsDefinition();
		void Reset();

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);

		size_t GetType()const { return m_variationType; }
		void SetType(size_t type){ m_variationType = type; }
		size_t GetNbVariation()const;
		void SetNbVariation(size_t nbVar){ m_nbVariation = nbVar; }

		void GetModelInputArray(const CModelInput& modelInput, CModelInputVector& modelInputArray)const;
		CModelInputVector GetModelInputVector(const CModelInput& modelInput)const;
		CModelInputVector GetModelInputVector(const CWGInput& modelInput)const;
		CParameterVector GetParametersVector()const;

		void RemoveInvalidVar(const CModelInput& modelInput);
		size_t FindByPos(int pos)const;
		size_t Find(const std::string& name)const;

		ERMsg Load(const std::string& filePath){ return zen::LoadXML(filePath, "ParametersVariations", "1", *this); }
		ERMsg Save(const std::string& filePath){ return zen::SaveXML(filePath, "ParametersVariations", "1", *this); }


	protected:

		void GenerateParameter(CFloatMatrix& paramArray)const;
		void GenerateRegularParameter(CFloatMatrix& paramArray)const;
		void GenerateRandomParameter(CFloatMatrix& paramArray)const;
		bool ParameterIsValid()const;

		static const char* MEMBER_NAME[NB_MEMBER];

	};
}


namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CParameterVariation& in, XmlElement& output)
	{
		in.writeStruc(output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CParameterVariation& out)
	{
		return out.readStruc(input);
	}


	template <> inline
		void writeStruc(const WBSF::CParametersVariationsDefinition& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::NB_VARIATIONS)](in.m_nbVariation);
		out[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::TYPE)](in.m_variationType);
		out[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::PARAMETERS)]((WBSF::CParamVariationsContainer&)in);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CParametersVariationsDefinition& out)
	{
		XmlIn in(input);
		
		in[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::NB_VARIATIONS)](out.m_nbVariation);
		in[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::TYPE)](out.m_variationType);
		in[WBSF::CParametersVariationsDefinition::GetMemberName(WBSF::CParametersVariationsDefinition::PARAMETERS)]((WBSF::CParamVariationsContainer&)out);

		return true;
	}
}
