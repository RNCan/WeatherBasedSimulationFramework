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


#include "basic/ERMsg.h"
#include "ModelBase/Model.h"
#include "ModelBase/ParametersVariations.h"
#include "ModelBase/DevRateEquation.h"
#include "Simulation/Executable.h"
#include "Simulation/ModelParameterization.h"

namespace WBSF
{
	class CWeatherGenerator;

	

	class CDevRateDataRow
	{
	public:
		std::string m_stage;
		double m_T;
		double m_rate;
		double m_n;
	};

	typedef std::vector<CDevRateDataRow>CDevRateData;

	class CDevRateOutput
	{
	public:
		CDevRateOutput(std::string s, CDevRateEquation::TDevRateEquation e):
			m_stage(s), m_model(e), m_parameters( CDevRateEquation::GetParameters(e))
		{}

		std::string m_stage;
		CDevRateEquation::TDevRateEquation m_model;
		CSAParameterVector m_parameters;
		CComputationVariable m_computation;
	};

	
	typedef std::vector<CDevRateOutput>CCDevRateOutputVector;
	//*******************************************************************************
	//CDevRateParameterization
	class CDevRateParameterization : public CExecutable
	{
	public:

		static const char* DATA_DESCRIPTOR;
		enum TFeedback { LOOP, ITERATION, CYCLE };
		//ALWAYS_CREATE_WEATHER, TG_INPUT_NAME, LOC_NAME, 
		enum TMember{
			EQUATIONS = CExecutable::NB_MEMBERS, INPUT_FILE_NAME, CONTROL, FEEDBACK_TYPE,
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CDevRateParameterization); }

		std::string m_inputFileName;
		EquationBitset m_equations;
		int m_feedbackType;

		CSAControl m_ctrl;

		CDevRateParameterization();
		CDevRateParameterization(const CDevRateParameterization& in);
		virtual ~CDevRateParameterization();
		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CDevRateParameterization(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CDevRateParameterization&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CDevRateParameterization&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter= CParentInfoFilter())const;

		void Reset();

		CDevRateParameterization& operator =(const CDevRateParameterization& in);
		bool operator == (const CDevRateParameterization& in)const;
		bool operator != (const CDevRateParameterization& in)const{ return !operator==(in); }

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);

		const CSAControl& GetControl()const { return m_ctrl; }
		void SetControl(CSAControl& control) { m_ctrl = control; }
		
	protected:

		ERMsg Optimize(std::string s, size_t e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		void GetFValue(std::string s, size_t e, CComputationVariable& computation);

		ERMsg InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		void WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);

		double Exprep(const double& RDUM);

		//input
		CDevRateData m_data;

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;
	};


}

namespace zen
{
	template <> inline
		void writeStruc(const WBSF::EquationBitset& in, XmlElement& output)
	{
		output.setValue(in.GetSelection());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::EquationBitset& out)
	{
		std::string str;
		try
		{
			if (input.getValue(str))
				out.SetSelection(str);
		}
		catch (...)
		{
		}

		return true;
	}
}