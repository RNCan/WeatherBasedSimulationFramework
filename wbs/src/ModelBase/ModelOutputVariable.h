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


#include "Basic/UtilTime.h"
#include "ModelBase/InputParam.h"

namespace WBSF
{

	class CModelOutputVariableDef
	{
	public:

		enum TMembers{ NAME, TITLE, UNITS, DESCRIPTION, TIME_MODE, PRECISION, EQUATION, CLIMATIC_VARIABLE, NB_MEMBERS };
		static const size_t GetNbMembers(){ return NB_MEMBERS; }
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		//members
		std::string m_name;         //name without space
		std::string m_title;        //title of the variable
		std::string m_units;        //units of the variable (can be empty)
		std::string m_description;  //the description appear in the bottom of the propertyCtrl
		short m_precision;			//display/export precision
		CTM m_TM;					//Time mode of this variable, undifine for untemporal variable
		std::string m_equation;     //for function definition
		size_t  m_climaticVariable;	//Type of climatic variable when applicable

		CModelOutputVariableDef();
		CModelOutputVariableDef(std::string name, std::string title, std::string units, std::string description, CTM TM = CTM(CTM::ATEMPORAL, CTM::FOR_EACH_YEAR), short precision = 4, std::string equation = "", size_t  climaticVariable = UNKNOWN_POS);
		CModelOutputVariableDef(const CModelOutputVariableDef& in);
		~CModelOutputVariableDef();

		void Reset();
		CModelOutputVariableDef& operator = (const CModelOutputVariableDef& in);
		bool operator == (const CModelOutputVariableDef& in)const;
		bool operator != (const CModelOutputVariableDef& in)const{ return !(operator==(in)); }

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		//std::string GetMember(int i, LPXNode& pNode=NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode=NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{XGetXML(*this, pRoot);}
		//virtual void SetXML(const LPXNode pRoot){XSetXML(*this, pRoot);}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_name & m_title & m_units & m_description & m_TM & m_precision & m_equation & m_climaticVariable;
		}


	protected:

		//variable name
		static const char* MEMBERS_NAME[NB_MEMBERS];
		static const char* XML_FLAG;

	};




	//********************************************************
	//CModelOutputVariableDefVector
	typedef std::vector<CModelOutputVariableDef> CModelOutputVariableDefVectorBase;
	class CModelOutputVariableDefVector : public CModelOutputVariableDefVectorBase
	{
	public:
		// Definition of the template
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CModelOutputVariableDefVectorBase>(*this);
		}

		friend boost::serialization::access;

		CParameterVector GetParametersVector()const;
		CWVariables GetWVariables()const;
	};
}


namespace zen
{

		template <> inline
			void writeStruc(const WBSF::CModelOutputVariableDef& in, XmlElement& output)
		{
			XmlOut out(output);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::NAME)](in.m_name);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::TITLE)](in.m_title);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::UNITS)](in.m_units);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::DESCRIPTION)](in.m_description);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::TIME_MODE)](in.m_TM);
			out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::PRECISION)](in.m_precision);
			if (!in.m_equation.empty())
				out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::EQUATION)](in.m_equation);

			if (in.m_climaticVariable != UNKNOWN_POS)
				out[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::CLIMATIC_VARIABLE)](in.m_climaticVariable);
		}

		template <> inline
			bool readStruc(const XmlElement& input, WBSF::CModelOutputVariableDef& out)
		{
			XmlIn in(input);

			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::NAME)](out.m_name);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::TITLE)](out.m_title);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::UNITS)](out.m_units);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::DESCRIPTION)](out.m_description);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::TIME_MODE)](out.m_TM);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::PRECISION)](out.m_precision);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::EQUATION)](out.m_equation);
			in[WBSF::CModelOutputVariableDef::GetMemberName(WBSF::CModelOutputVariableDef::CLIMATIC_VARIABLE)](out.m_climaticVariable);

			return true;
		}
	
	template <> inline
		void writeStruc(const WBSF::CModelOutputVariableDefVector& in, XmlElement& output)
	{
		std::for_each(in.begin(), in.end(),
			[&](const WBSF::CModelOutputVariableDef & childVal)
		{
			XmlElement& newChild = output.addChild(WBSF::CModelOutputVariableDef::GetXMLFlag());
			zen::writeStruc(childVal, newChild);
		}
		);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModelOutputVariableDefVector& out)
	{
		bool success = true;
		out.clear();

		auto iterPair = input.getChildren(WBSF::CModelOutputVariableDef::GetXMLFlag());
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			WBSF::CModelOutputVariableDef childVal; //MSVC 2010 bug: cannot put this into a lambda body
			if (zen::readStruc(*iter, childVal))
				out.insert(out.end(), childVal);
			else
				success = false;
		}
		return success;
	}
}