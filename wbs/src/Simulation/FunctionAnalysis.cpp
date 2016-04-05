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
#include <wtypes.h>
#include <boost\algorithm\string.hpp>
#include <Boost\multi_array.hpp>
#include "MTParser/MTParser.h"

#include "Basic/statistic.h"
#include "FileManager/FileManager.h"
#include "Simulation/FunctionAnalysis.h"
#include "Simulation/ExecutableFactory.h"

#include "WeatherBasedSimulationString.h"

using namespace boost;
using namespace std;
using namespace WBSF::DIMENSION;


namespace WBSF
{


class GetJDayFct : public MTFunctionI
{

public:

	GetJDayFct(){}

	//void SetTM( CTM TM){ m_TM = TM; };

protected:

	GetJDayFct(const GetJDayFct& obj){	/*m_TM = obj.m_TM;*/ }
	virtual const MTCHAR* getSymbol(){ return _T("JDay"); }
	virtual const MTCHAR* getHelpString(){ return _T("JDay(time reference)"); }
	virtual const MTCHAR* getDescription()	{ return _T("Return the Julian day of a time reference"); }
	virtual int getNbArgs(){ return 1; }
	virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
	{
		ASSERT(nbArgs == 1);
		int iArg = (int)pArg[0];

		MTDOUBLE val = 0;


		if (iArg < 0 || iArg>365)
		{
			//we assume that JDay is call only on daily values
			CTRef ref;
			ref.SetRef(iArg, CTM(CTM::DAILY, CTM::FOR_EACH_YEAR));

			val = MTDOUBLE(ref.GetJDay() + 1);
		}
		else
		{
			//we assume daily value in OVERALL_YEAR mode
			val = iArg;
		}

		return val;
	}

	
	virtual MTFunctionI* spawn(){ return new GetJDayFct(); }



};



	
	MTFunctionI* CreateGetJDayFct(){ return new GetJDayFct; }


	class CPredicateVariableDef
	{
	public:
		CPredicateVariableDef(const std::string& name)
		{
			m_name = name;
		}

		bool operator()(CModelOutputVariableDef const & in)const{ return boost::iequals(in.m_name, m_name); }

	private:

		std::string m_name;
	};

	//************************************************************************
	//CFunctionContext
	CParserVariable CFunctionContext::GetParserVariable(const CModelOutputVariableDefVector& varDef, std::string name)
	{
		//name.MakeUpper();

		CParserVariable v;
		//v.m_dim = -1;
		//v.m_field = -1;

		//is it a loc variable
		for (int i = CLocation::LAT; i < CLocation::NB_MEMBER; i++)
		{
			if (name == CLocation::GetMemberName(i))
			{
				v.m_dim = LOCATION;
				v.m_field = i;
				break;
			}
		}

		if (v.m_field == -1)
		{
			for (int i = 0; i <= CTRefFormat::HOUR; i++)
			{
				if (name == CTRefFormat::GetFormatName(i))
				{
					v.m_dim = TIME_REF;
					v.m_field = i;
					break;
				}
			}
		}

		if (v.m_field == -1)
		{
			CModelOutputVariableDefVector::const_iterator it = find_if(varDef.begin(), varDef.end(), CPredicateVariableDef(name));
			if (it != varDef.end())
			{
				v.m_field = int(it - varDef.begin());
				v.m_dim = v.m_field == -1 ? -1 : VARIABLE;
			}
		}

		return v;
	}

	//************************************************************************
	//CFunctionContext

	CFunctionContext::CFunctionContext()
	{
		m_pParser = new MTParser;
	}

	CFunctionContext::~CFunctionContext()
	{
		delete m_pParser;
		m_pParser = NULL;
	}

	ERMsg CFunctionContext::Compile(const CModelOutputVariableDefVector& varDef, std::string equation)
	{
		ERMsg msg;
		m_pParser->enableAutoVarDefinition(true);
		m_pParser->defineFunc(new GetJDayFct);
		m_pParser->defineConst(_T("VMISS"), VMISS);

		try
		{
			m_pParser->compile(convert(equation).c_str());
		}
		catch (MTParserException e)
		{
			msg.ajoute(UTF8(e.m_description.c_str()));
		}

		if (!msg)
			return msg;

		m_vars.resize(m_pParser->getNbUsedVars());

		for (unsigned int t = 0; t < m_pParser->getNbUsedVars(); t++)
		{
			m_vars[t] = GetParserVariable(varDef, UTF8(m_pParser->getUsedVar(t)));
			m_pParser->redefineVar(m_pParser->getUsedVar(t).c_str(), &(m_vars[t].m_value));


			std::string test = UTF8(m_pParser->getExpression());
			if (m_vars[t].m_field == -1)
			{
				string error = FormatMsg(IDS_SIM_UNRESOLVE_VARIABLE, UTF8(m_pParser->getUsedVar(t)), UTF8(m_pParser->getExpression()));
				msg.ajoute(error);
			}
		}

		return msg;
	}

	double CFunctionContext::Evaluate(CResultPtr& pResult, size_t s, size_t r)
	{
		double value = VMISS;
		bool bMissingValue = false;

		//replace equation value by the variable value
		for (unsigned int t = 0; t < m_pParser->getNbUsedVars() && !bMissingValue; t++)
		{
			m_vars[t].m_value = pResult->GetDataValue(s, r, m_vars[t].m_dim, m_vars[t].m_field, MEAN);
			if (m_vars[t].m_value <= VMISS)
				bMissingValue = true;
		}


		if (!bMissingValue)
		{
			try
			{
				value = m_pParser->evaluate();
			}
			catch (MTParserException& e)
			{
				//msg.ajoute( e.m_description.c_str() );
				std::string msg = UTF8(e.m_description.c_str());
			}
		}

		return value;
	}

	//************************************************************************
	/*const char* CFunctionDef::XML_FLAG = "FunctionDefinition";
	const char* CFunctionDefArray::XML_FLAG = "FunctionDefinitionArray";
	const char* CFunctionDef::MEMBER_NAME[NB_MEMBER_EX] = {"Equation"};


	CFunctionDef::CFunctionDef()
	{
	m_equation.empty();
	}


	void CFunctionDef::Reset()
	{
	CModelOutputVariableDef::Reset();
	m_equation.empty();
	}

	CFunctionDef& CFunctionDef::operator =(const CFunctionDef& in)
	{
	if( &in != this)
	{
	CModelOutputVariableDef::operator =(in);
	m_equation = in.m_equation;
	}

	ASSERT( *this == in);

	return *this;
	}

	bool CFunctionDef::operator == (const CFunctionDef& in)const
	{
	bool bEqual = true;
	if( CModelOutputVariableDef::operator !=(in) )bEqual = false;
	if( m_equation != in.m_equation )bEqual = false;

	return bEqual;
	}


	std::string CFunctionDef::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	std::string str;
	switch(i)
	{
	case EQUATION:	str = m_equation; break;
	default: str = CModelOutputVariableDef::GetMember(i, pNode);
	}

	return str;
	}


	void CFunctionDef::SetMember(int i, const std::string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case EQUATION:	m_equation = str; break;
	default:CModelOutputVariableDef::SetMember(i, str, pNode);
	}

	}

	*/




	//*******************************************************************************
	const char* CFunctionAnalysis::XML_FLAG = "FunctionAnalysis";
	const char* CFunctionAnalysis::MEMBER_NAME[NB_MEMBER_EX] = { "FunctionDefinitionArray" };
	const int CFunctionAnalysis::CLASS_NUMBER = CExecutableFactory::RegisterClass(CFunctionAnalysis::GetXMLFlag(), &CFunctionAnalysis::CreateObject);

	CFunctionAnalysis::CFunctionAnalysis()
	{
		ClassReset();
	}

	CFunctionAnalysis::CFunctionAnalysis(const CFunctionAnalysis& in)
	{
		operator=(in);
	}

	CFunctionAnalysis::~CFunctionAnalysis()
	{}

	void CFunctionAnalysis::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CFunctionAnalysis::ClassReset()
	{
		m_name = "FunctionAnalysis";
		m_functionArray.clear();

	}

	CFunctionAnalysis& CFunctionAnalysis::operator =(const CFunctionAnalysis& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_functionArray = in.m_functionArray;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CFunctionAnalysis::operator == (const CFunctionAnalysis& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_functionArray != in.m_functionArray)bEqual = false;

		return bEqual;
	}

	/*
	std::string CFunctionAnalysis::GetMember(int i, LPXNode& pNode)const
	{
	ASSERT( i>=0 && i<NB_MEMBER);

	std::string str;
	switch(i)
	{
	case FUNCTION_ARRAY: m_functionArray.GetXML(pNode); break;
	default: str = CExecutable::GetMember(i, pNode);
	}

	return str;
	}

	void CFunctionAnalysis::SetMember(int i, const std::string& str, const LPXNode pNode)
	{
	ASSERT( i>=0 && i<NB_MEMBER);
	switch(i)
	{
	case FUNCTION_ARRAY: m_functionArray.SetXML(pNode); break;
	default:CExecutable::SetMember(i, str, pNode);
	}

	}
	*/

	ERMsg CFunctionAnalysis::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ASSERT(GetParent());

		ERMsg msg;

		//same as weather generator variables
		msg = m_pParent->GetParentInfo(fileManager, info, filter);

		if (filter[VARIABLE])
		{
			GetOutputDefinition(info.m_variables);
		}



		return msg;
	}



	void CFunctionAnalysis::GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)
	{
		info = pResult->GetMetadata();

		CModelOutputVariableDefVector outputVar;
		GetOutputDefinition(outputVar);

		info.SetOutputDefinition(outputVar);
	}

	ERMsg CFunctionAnalysis::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		CResultPtr pResult = m_pParent->GetResult(fileManager);

		msg = pResult->Open();
		if (!msg)
			return msg;

		//Generate output path
		std::string outputPath = GetPath(fileManager);

		//Generate DB file path
		std::string DBFilePath = GetDBFilePath(outputPath);

		//open outputDB
		CResult result;
		msg = result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
		if (!msg)
			return msg;


		//init output info
		CDBMetadata& metadata = result.GetMetadata();
		GetInputDBInfo(pResult, metadata);

		const CModelOutputVariableDefVector& varDef = pResult->GetMetadata().GetOutputDefinition();

		//Create and compile all equations
		CFunctionContextVector parserArray;
		parserArray.resize(m_functionArray.size());
		for (int i = 0; i < m_functionArray.size(); i++)
		{
			callback.AddMessage(m_functionArray[i].m_name + " = " + m_functionArray[i].m_equation, 2);

			msg += parserArray[i].Compile(varDef, m_functionArray[i].m_equation);
		}

		//compute result	
		if (!msg)
			return msg;


		size_t nbSection = pResult->GetNbSection();
		callback.PushTask("Function Ananlysis", nbSection);

		for (size_t s = 0; s < nbSection&&msg; s++)
		{
			size_t nbRow = pResult->GetNbRows(s);

			CNewSectionData section;
			section.resize(nbRow, parserArray.size(), pResult->GetFirstTRef(s));

			for (size_t r = 0; r < nbRow&&msg; r++)
			{
				for (size_t i = 0; i<parserArray.size(); i++)
				{
					double value = parserArray[i].Evaluate(pResult, s, r);
					if (value > VMISS)
						section[r][i] = value;
				}

				msg += callback.StepIt(1.0 / nbRow);
			}

			result.AddSection(section);
		}

		result.Close();

		callback.PopTask();

		return msg;
	}

	ERMsg CFunctionAnalysis::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	{
		GetOutputDefinition(outputVar);

		return ERMsg();
	}

	void CFunctionAnalysis::GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const
	{
		outputVar.clear();
		for (int i = 0; i < m_functionArray.size(); i++)
			outputVar.push_back(m_functionArray[i]);

	}




	void CFunctionAnalysis::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(FUNCTION_ARRAY)](m_functionArray);
	}

	bool CFunctionAnalysis::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(FUNCTION_ARRAY)](m_functionArray);

		return true;
	}

	//*******************************************************************************

}