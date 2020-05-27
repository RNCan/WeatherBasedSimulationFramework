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
#include "basic/UtilZen.h"
#include "ModelBase/Model.h"
#include "ModelBase/ParametersVariations.h"
#include "ModelBase/DevRateEquation.h"
#include "ModelBase/MortalityEquation.h"
#include "Simulation/Executable.h"
#include "Simulation/ModelParameterization.h"

namespace WBSF
{
	class CWeatherGenerator;

	namespace DevRateInput
	{
		enum TDevTimeCol { I_UNKNOWN = -1, I_VARIABLE, I_T, I_TMIN, I_TMAX, I_T_PROFILE, I_TTYPE, I_TIME, I_TIME_STD, I_N, I_RELATIVE_TIME, I_Q_TIME, NB_DEV_INPUT };
		enum TTobsCol {C_UNKNOWN = -1, C_TID, C_T, NB_TOBS_COL };
		enum TTemperature { T_UNKNOWN = -1, T_CONSTANT, T_SINUS, T_TRIANGULAR, T_OBS, NB_TMP_TYPE };
		
		extern const char* TTYPE_NAME[NB_TMP_TYPE];
		TTemperature get_TType(const std::string& name);
	}

	

	
	class CDevRateDataRow : public std::map<DevRateInput::TDevTimeCol, double>
	{
	public:

		std::string m_variable;
		size_t m_TType;
		std::string m_Tprofile;
	};


	typedef std::vector<CDevRateDataRow> CDevRateDataRowVector;

	class CTobsSeries :public std::map<std::string, std::vector<double>>
	{
	public:

		static const char* INPUT_NAME[DevRateInput::NB_TOBS_COL];
		static DevRateInput::TTobsCol get_input(const std::string& name);

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		ERMsg verify(const CDevRateDataRowVector& data)const;
		void generate(const CDevRateDataRowVector& data);

		bool have_var(DevRateInput::TTobsCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(DevRateInput::TTobsCol c)const;
		std::vector< DevRateInput::TTobsCol> m_input_pos;
	};




	class CDevRateData : public CDevRateDataRowVector
	{
	public:

		static const char* INPUT_NAME[DevRateInput::NB_DEV_INPUT];
		static DevRateInput::TDevTimeCol get_input(const std::string& name);

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		bool have_var(DevRateInput::TDevTimeCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(DevRateInput::TDevTimeCol c)const;

		std::vector< DevRateInput::TDevTimeCol> m_input_pos;
		std::map<std::string, CStatisticEx> m_stats;

		bool m_bIndividual;
	};


	class CFitOutput
	{
	public:

		CFitOutput(std::string v, size_t e, const CSAParameterVector& params) :
			m_variable(v), m_equation(e), m_parameters(params)
		{
			//ASSERT(m_parameters.size() >= CDevRateEquation::GetParameters(e).size());
			m_fixeSigma = 0.0;
		}

		std::string m_variable;
		size_t m_equation;
		CSAParameterVector m_parameters;
		CComputationVariable m_computation;
		double m_fixeSigma;
		//bool m_bAddRelDevRate;
	};


	typedef std::vector<CFitOutput>CFitOutputVector;
	


	//*******************************************************************************
	//CDevRateParameterization
	class CDevRateParameterization : public CExecutable
	{
	public:

		static const char* DATA_DESCRIPTOR;
		enum TCalibOn { CO_RATE, CO_TIME, NB_CALIB_ON };
		enum TFeedback { LOOP, ITERATION, CYCLE };
		enum TFit { F_DEV_RATE, F_MORTALITY, NB_FIT_TYPE };

		enum TMember {
			FIT_TYPE = CExecutable::NB_MEMBERS, DEV_RATE_EQUATIONS, MORTALITY_EQUATIONS, EQ_OPTIONS, INPUT_FILE_NAME, TOBS_FILE_NAME, OUTPUT_FILE_NAME, CALIB_ON, CONTROL, CONVERGE_01, CALIB_SIGMA, FIXE_SIGMA,
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i) { ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag() { return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject() { return CExecutablePtr(new CDevRateParameterization); }

		std::string m_inputFileName;
		std::string m_TobsFileName;
		std::string m_outputFileName;
		size_t m_calibOn;

		size_t m_fitType;
		CDevRateEqSelected m_eqDevRate;
		CMortalityEqSelected m_eqMortality;
		CSAParametersMap m_eq_options;

		bool m_bConverge01;
		bool m_bCalibSigma;
		bool m_bFixeSigma;



		CSAControl m_ctrl;

		CDevRateParameterization();
		CDevRateParameterization(const CDevRateParameterization& in);
		virtual ~CDevRateParameterization();
		virtual const char* GetClassName()const { return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const { return CExecutablePtr(new CDevRateParameterization(*this)); }
		virtual CExecutable& Assign(const CExecutable& in) { ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CDevRateParameterization&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const { ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CDevRateParameterization&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;

		void Reset();

		CDevRateParameterization& operator =(const CDevRateParameterization& in);
		bool operator == (const CDevRateParameterization& in)const;
		bool operator != (const CDevRateParameterization& in)const { return !operator==(in); }

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);

		const CSAControl& GetControl()const { return m_ctrl; }
		void SetControl(CSAControl& control) { m_ctrl = control; }

	protected:

		ERMsg Optimize(std::string s, size_t e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		//ERMsg Optimize(CFitOutput& output, CCallback& callback);
		bool GetFValue(std::string s, size_t e, CComputationVariable& computation);

		ERMsg InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		void WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);

		double Exprep(const double& RDUM);

		//input
		CDevRateData m_data;
		CTobsSeries m_Tobs;



		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;
	};


}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CSAParametersMap& in, XmlElement& output)
	{
		for (auto it = in.begin(); it != in.end(); it++)
		{
			XmlElement& newChild = output.addChild("Parameter");
			newChild.setAttribute("name", it->first);
			writeStruc(it->second, newChild);
			//newChild.setValue(to_string(it->second, "|"));
		}
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSAParametersMap& out)
	{
		bool success = true;
		out.clear();

		auto iterPair = input.getChildren("Parameter");
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			 //MSVC 2010 bug: cannot put this into a lambda body
			std::string name;
			iter->getAttribute("name", name);
			//WBSF::CSAParameterVector item;
			if (!zen::readStruc(*iter, out[name]))
				success = false;
			
			//std::string value;
			//iter->getValue(value);
			//readStruc(it->second, newChild);
			// = WBSF::to_object<WBSF::CSAParameter>(value, "|");
		}

		return success;
	}


	
}