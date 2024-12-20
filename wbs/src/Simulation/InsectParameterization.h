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
#include "ModelBase/SurvivalEquation.h"
#include "Simulation/Executable.h"
#include "Simulation/ModelParameterization.h"

namespace WBSF
{
	class CWeatherGenerator;
	typedef TDevRateEquation TFecundityEquation;
	typedef CDevRateEqSelected CFecundityEqSelected;
	typedef CDevRateEquation CFecundityEquation;

	namespace DevRateInput
	{
		enum TDevTimeCol { I_UNKNOWN = -1, I_VARIABLE, I_TREATMENT, I_I, I_START, I_TIME, I_MEAN_TIME, I_TIME_SD, I_N, I_OBS_INT, I_RDT, I_Q_TIME, I_RATE, I_RDR, I_Q_RATE, I_SURVIVAL, I_BROOD, I_MEAN_BROOD, I_BROOD_SD, I_RFR, I_Q_BROOD, NB_DEV_INPUT };
		enum TTobsCol {C_UNKNOWN = -1, C_TID, C_T, NB_TOBS_COL };
		enum TTemperature { T_UNKNOWN = -1, T_CONSTANT, T_TRANSFER, T_SQUARE, T_TRIANGULAR, T_SINUS, T_SINUS_MEAN, T_HOBO, NB_TMP_TYPE };
		
		extern const char* TTYPE_NAME[NB_TMP_TYPE];
		TTemperature get_TType(const std::string& name);
	}

	

	typedef std::shared_ptr<std::vector<double>> TobsSeriesPtr;
	class CDevRateDataRow : public std::map<DevRateInput::TDevTimeCol, double>
	{
	public:

		CDevRateDataRow();

		std::string m_variable;
		std::string m_treatment;
		std::string m_i;
		std::string GetProfile()const { return m_variable + "_" + m_treatment + "_" + m_i; }
		size_t m_type;
		bool m_bIndividual;
		bool m_bTimeSeries;

		double GetMaxTime() const;

		//double T() const { ASSERT(T1() == T2());  return T1(); }
		double T1() const { return T()[0]; }
		double T2() const { return T()[1]; }
		
		double t1() const { return m_t1; }
		double t2(bool bPrevious = false) const { return std::max(0.0, t(bPrevious) - t1());  }
		double t(bool bPrevious = false) const { return bPrevious ? m_tˉ¹ : m_t;}
		size_t GetH1() const;

		const std::vector<double>& T()const { ASSERT(m_Tobs.get());  return  *(m_Tobs.get()); }

		
		double ComputeT(size_t i=0) const;
		double GetTime1() const;
		double GetTime() const;


		void SetTime1(double t1) { m_t1=t1; }
		void SetTime(double t){  m_t=t; }
		void SetTimeˉ¹(double tˉ¹) { m_tˉ¹ = tˉ¹; }
		void SetTobs(const TobsSeriesPtr& TPtr) { m_Tobs = TPtr; }


	protected:
		
		double m_t1;
		double m_t;
		double m_tˉ¹;
		

		TobsSeriesPtr m_Tobs;

	};

	class CTobsSeries;
	typedef std::vector<CDevRateDataRow> CDevRateDataRowVector;
	
	class CDevRateData : public CDevRateDataRowVector
	{
	public:

		static const char* INPUT_NAME[DevRateInput::NB_DEV_INPUT];
		static DevRateInput::TDevTimeCol get_input(const std::string& name);

		CDevRateData();
		virtual ~CDevRateData();
		void clear();


		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		bool have_var(DevRateInput::TDevTimeCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(DevRateInput::TDevTimeCol c)const;

		std::vector< DevRateInput::TDevTimeCol> m_input_pos;
		std::map<std::string, std::map<std::string, CStatisticEx>> m_statsTime;
		std::map<std::string, std::map<std::string, CStatisticEx>> m_statsRate;
		std::map<std::string, std::map<std::string, CStatisticEx>> m_statsBrood;
		std::map<std::string, CStatistic> m_statsRateByVariable;
		std::map<std::string, CStatistic> m_statsTByVariable;

		std::map<std::string, std::map<std::string, vector<size_t>>> m_pos;
		

		bool m_bIndividual;
		bool m_bIndividualSeries;
		bool m_bAllTConstant;
		//size_t GetMaxTime() const;
		double GetDefaultSigma(const std::string& variable)const;
		double GetSigmaBrood(TDevRateEquation  e, const std::vector<double>& X, const std::vector<double>& T)const;
		double GetDefaultSigma()const;
		//void GetDefaultFSigmaBrood(const std::string& variable, double& Fo, double& sigma)const;
		//double GetSigma(CComputationVariable& computation)const;
		bool IsAllTConstant()const;
		void compute_T_stats(const CTobsSeries& Tobs);
		void set_Tobs(const CTobsSeries& Tobs);

		
		static double ei(size_t n);
		static double cv_2_sigma(double cv, size_t n);
	};

	typedef CDevRateData CFecundityData;

	

	class CTobsSeries :public std::map<std::string, std::vector<double>>
	{
	public:

		static const char* INPUT_NAME[DevRateInput::NB_TOBS_COL];
		static DevRateInput::TTobsCol get_input(const std::string& name);

		CTobsSeries();
		virtual ~CTobsSeries();

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		ERMsg verify(const CDevRateData& data)const;
		void generate(const CDevRateData& data);
		void compute_stats();

		bool have_var(DevRateInput::TTobsCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(DevRateInput::TTobsCol c)const;
		std::vector< DevRateInput::TTobsCol> m_input_pos;
		std::map<std::string, CStatistic> m_stat;
		//bool IsAllFixed()const;
	};




	

	class CSurvivalData : public CDevRateData
	{
	public:

		//static const char* INPUT_NAME[DevRateInput::NB_DEV_INPUT];
		//static DevRateInput::TDevTimeCol get_input(const std::string& name);

		CSurvivalData();
		virtual ~CSurvivalData();

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		//bool have_var(DevRateInput::TDevTimeCol c)const { return get_pos(c) != NOT_INIT; }
		//size_t get_pos(DevRateInput::TDevTimeCol c)const;

		//std::vector< DevRateInput::TDevTimeCol> m_input_pos;
		
		

	private:
		std::map<std::string, CStatisticEx> m_stats;
		

	//	bool m_bIndividual;
	};



	class CDevRateEqFile : public std::map<std::string, std::pair<CDevRateEquation::TDevRateEquation, std::vector<double>>>
	{
	public:

		//static const char* INPUT_NAME[DevRateInput::NB_DEV_INPUT];
		//static DevRateInput::TDevTimeCol get_input(const std::string& name);

		CDevRateEqFile();
		virtual ~CDevRateEqFile();

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);


		
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
	//CInsectParameterization
	class CInsectParameterization : public CExecutable
	{
	public:

		static const char* DATA_DESCRIPTOR;
		enum TDevRateCalibOn { CO_TIME, CO_RATE, NB_CALIB_ON };
		enum TFeedback { LOOP, ITERATION, CYCLE };
		enum TFit { F_DEV_TIME, F_SURVIVAL,  F_FECUNDITY, NB_FIT_TYPE };
		//_WTH_SIGMA
		//F_DEV_TIME_ONLY, 

		

		enum TMember {
			FIT_TYPE = CExecutable::NB_MEMBERS, DEV_RATE_EQUATIONS, SURVIVAL_EQUATIONS, FECUNDITY_EQUATIONS, EQ_OPTIONS, INPUT_FILE_NAME, TOBS_FILE_NAME, OUTPUT_FILE_NAME, CONTROL, FIXE_TB, TB_VALUE, FIXE_TM, TM_VALUE, COUNSTRAIN_T_LO, T_LO_VALUES, COUNSTRAIN_T_HI, T_HI_VALUES, FIXE_F0, F0_VALUE, LIMIT_MAX_RATE, LIMIT_MAX_RATE_P, AVOID_NULL_RATE_IN_TOBS, USE_OUTPUT_AS_INPUT, OUTPUT_AS_INTPUT_FILENAME, SHOW_TRACE,
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i) { ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag() { return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject() { return CExecutablePtr(new CInsectParameterization); }

		std::string m_inputFileName;
		std::string m_TobsFileName;
		std::string m_outputFileName;
		//size_t m_calibOn;

		size_t m_fitType;

		CDevRateEqSelected m_eqDevRate;
		CSurvivalEqSelected m_eqSurvival;
		CFecundityEqSelected  m_eqFecundity;
		CSAParametersMap m_eq_options;

		//bool m_bConverge01;
		//bool m_bCalibSigma;
		bool m_bFixeTb;
		std::array<double,3> m_Tb;
		bool m_bFixeTm;
		std::array<double, 3> m_Tm;
		bool m_bFixeF0;
		double m_F0;
		bool m_bLimitMaxRate;
		double m_LimitMaxRateP;


		bool m_bConstrainTlo;
		std::array<double, 2> m_Tlo;
		bool m_bConstrainThi;
		std::array<double, 2> m_Thi;



		bool m_bAvoidNullRateInTobs;
		bool m_bUseOutputAsInput;
		std::string m_outputAsIntputFileName;
		bool m_bShowTrace;


		CSAControl m_ctrl;

		CInsectParameterization();
		CInsectParameterization(const CInsectParameterization& in);
		virtual ~CInsectParameterization();
		virtual const char* GetClassName()const { return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const { return CExecutablePtr(new CInsectParameterization(*this)); }
		virtual CExecutable& Assign(const CExecutable& in) { ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CInsectParameterization&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const { ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CInsectParameterization&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;

		void Reset();

		CInsectParameterization& operator =(const CInsectParameterization& in);
		bool operator == (const CInsectParameterization& in)const;
		bool operator != (const CInsectParameterization& in)const { return !operator==(in); }

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);

		const CSAControl& GetControl()const { return m_ctrl; }
		void SetControl(CSAControl& control) { m_ctrl = control; }


		static ERMsg ReadParametersFromFile(const std::string& outputFilePath, std::map<std::string, std::map<std::string, std::map<std::string, double>>>& params);
	protected:

		ERMsg Optimize(std::string s, size_t e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		bool GetFValue(std::string s, size_t e, CComputationVariable& computation);
		bool IsParamValid( const std::string& var, CDevRateEquation::TDevRateEquation eq, const std::vector<double>& P);
		bool IsRateValid(const std::string& var, CDevRateEquation::TDevRateEquation model, const std::vector<double>& P, double null_rate_treashold);

		ERMsg InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		void WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);
		void WriteInfoEx(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);

		double Exprep(const double& RDUM);

		//input
		CTobsSeries m_Tobs;
		CDevRateData m_devTime;
		CSurvivalData m_survival;
		CFecundityData m_fecundity;

		//CDevRateEqFile m_dev_rate_eq;


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