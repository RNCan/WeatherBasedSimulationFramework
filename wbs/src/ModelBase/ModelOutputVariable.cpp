//****************************************************************************
// File:	ModelOutputVariable.cpp
// Class:	CModelOutputVariableDef
//****************************************************************************
// 15/09/2008  Rémi Saint-Amant	Created from old file
// 24/11/2008  Rémi Saint-Amant	Add binary serialisation, add before, after value
// 10/02/2010  Rémi Saint-Amant	New functionnality. Add title and equation
//****************************************************************************

#include "stdafx.h"
#include "ModelOutputVariable.h"

using namespace std;
namespace WBSF
{

	const char* CModelOutputVariableDef::XML_FLAG = "OutputVariable";
	const char* CModelOutputVariableDef::MEMBERS_NAME[NB_MEMBERS] = { "Name", "Title", "Units", "Description", "TimeMode", "Precision", "Equation", "Climatic" };

	CModelOutputVariableDef::CModelOutputVariableDef()
	{
		Reset();
	}


	CModelOutputVariableDef::CModelOutputVariableDef(string name, string title, string units, string description, CTM TM, short precision, string equation, size_t climaticVariable)
	{
		m_name = name;
		m_title = title;
		m_units = units;
		m_description = description;
		m_TM = TM;
		m_precision = precision;
		m_equation = equation;
		m_climaticVariable = climaticVariable;


	}

	CModelOutputVariableDef::CModelOutputVariableDef(const CModelOutputVariableDef& in)
	{
		operator=(in);
	}

	CModelOutputVariableDef::~CModelOutputVariableDef()
	{}

	void CModelOutputVariableDef::Reset()
	{
		m_name.clear();
		m_title.clear();
		m_units.clear();
		m_description.clear();
		m_TM = CTM(CTM::ATEMPORAL, CTM::FOR_EACH_YEAR);
		m_precision = 4;
		m_equation.clear();
		m_climaticVariable = UNKNOWN_POS;

	}

	CModelOutputVariableDef& CModelOutputVariableDef::operator = (const CModelOutputVariableDef& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			m_title = in.m_title;
			m_units = in.m_units;
			m_description = in.m_description;
			m_TM = in.m_TM;
			m_precision = in.m_precision;
			m_equation = in.m_equation;
			m_climaticVariable = in.m_climaticVariable;
		}

		return *this;
	}

	bool CModelOutputVariableDef::operator == (const CModelOutputVariableDef& in)const
	{
		bool bEqual = true;

		if (m_name != in.m_name)bEqual = false;
		if (m_title != in.m_title)bEqual = false;
		if (m_units != in.m_units)bEqual = false;
		if (m_description != in.m_description)bEqual = false;
		if (m_TM != in.m_TM)bEqual = false;
		if (m_precision != in.m_precision)bEqual = false;
		if (m_equation != in.m_equation)bEqual = false;
		if (m_climaticVariable != in.m_climaticVariable)bEqual = false;


		return bEqual;
	}


	string CModelOutputVariableDef::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBERS);

		string str;
		switch (i)
		{
		case NAME:		str = m_name; break;
		case TITLE:		str = m_title; break;
		case UNITS:		str = m_units; break;
		case DESCRIPTION: str = m_description; break;
		case TIME_MODE:	str = to_string(m_TM); break;
		case PRECISION: str = ToString(m_precision); break;
		case EQUATION:	str = m_equation; break;
		case CLIMATIC_VARIABLE: str = ToString(m_climaticVariable); break;
		default: ASSERT(false);
		}

		return str;
	}

	void CModelOutputVariableDef::SetMember(size_t i, const string& str)
	{
		ASSERT(i >= 0 && i < NB_MEMBERS);

		switch (i)
		{
		case NAME:		m_name = str; break;
		case TITLE:		m_title = str; break;
		case UNITS:		m_units = str; break;
		case DESCRIPTION: m_description = str; break;
		case TIME_MODE:	m_TM = to_CTM(str); break;
		case PRECISION: m_precision = ToShort(str); break;
		case EQUATION:	m_equation = str; break;
		case CLIMATIC_VARIABLE: m_climaticVariable = ToSizeT(str); break;
		default: ASSERT(false);
		}
	}

	CParameterVector CModelOutputVariableDefVector::GetParametersVector()const
	{
		const CModelOutputVariableDefVector& me = *this;

		CParameterVector out(me.size());
		for (size_t i = 0; i < me.size(); i++)
		{
			std::string type = ToString(me[i].m_TM);
			std::string name = me[i].m_name;
			out[i] = CParameter(name, type, false);
		}

		return out;
	}


	CWVariables CModelOutputVariableDefVector::GetWVariables()const
	{
		CWVariables variables;

		for (const_iterator it = begin(); it != end(); it++)
		{
			if (it->m_climaticVariable != UNKNOWN_POS)
				variables.set(it->m_climaticVariable);
		}


		return variables;
	}
}