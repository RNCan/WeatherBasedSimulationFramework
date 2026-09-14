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

#include <boost/algorithm/string.hpp>

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

	enum TDevTimeCol { I_NOT_INIT = -1, I_STAGE, I_VARIABLE = I_STAGE, I_TREATMENT, I_IID, I_START, I_TIME, I_MEAN_TIME, I_TIME_SD, I_N, I_OBS_INT, I_STAGE_END, I_STAGE_END_STATUS, I_SURVIVAL, I_BROODS, I_MEAN_BROOD, I_BROOD_SD, I_DATE, I_DATE_END, NB_DEV_INPUT };
	enum TTobsCol { C_NOT_INIT = -1, C_TREATMENT, C_DATE, C_TIME, C_TEMPERATURE, C_LIGHT, NB_TOBS_COL };
	//T_TRIANGULAR, T_SINUS_MEAN, 
	enum TTreatment { T_NOT_INIT = -1, T_CONSTANT, T_TRANSFER, T_SQUARE, T_SINUS, T_FLUCTUATING, NB_TMP_TYPE };
	enum TStageEnd { SE_ALIVE, SE_DEAD, SE_CENSORED, SE_CASUALTIES, NB_STAGE_END_STATUS };//SE_SENESCENCE,


	enum TInputTemporal { IT_NOT_INIT = -1, IT_TIME_SERIES, IT_INDIVIDUAL, IT_MEAN, NB_INPUT_TEMPORAL };

	//extern const char* TTYPE_NAME[NB_TMP_TYPE];
	TTreatment GetTreatmentType(const std::string& treatment);

	double GetTreatmentTemperature(const std::string& treatment, size_t i);
	double GetTreatmentH1(const std::string& treatment);
	double GetTreatmentTime1(const std::string& treatment);
	size_t GetStageEndStatus(const std::string& name);

	class CDevRateDataRowVector;
	class CTemporalVector;

	typedef std::shared_ptr<CDevRateDataRowVector> CDevRateDataRowVectorPtr;
	typedef std::shared_ptr<CTemporalVector> TobsSeriesPtr;


	class CDataRow : public std::map<TDevTimeCol, double>

		//This object can represent an observation in a Time Series observation
		//or it can be an group of individual in an Individual observation
		//or the mean of group when it's not an Individual observation
		//class CDataRow : public std::map<TDevTimeCol, double>
	{
	public:

		CDataRow()
		{
			m_bIsChangingStage = false;
			//	m_bDead = false;
				//m_bCencored = false;
				//m_bAdult = false;
				//m_bCasualties = false;


				//m_t_T= 0;
			m_t_transfer = 0;
			m_to = 0;
			m_tˉ¹ = 0;
			m_t = 0;
			m_RDT = 0;
			m_pTime = 0;
			m_RDR = 0;
			m_pRate = 0;
			m_RF = 0;
			m_pFecundity = 0;
		}

		double GetMaxTime() const;
		double GetMinTime() const;




		CTRef m_TRef0a;//last observation of previous stage
		CTRef m_TRef0b;//first observation of this stage
		CTRef m_TRef1a;//last observation of this stage
		CTRef m_TRef1b;//first observation of the next stage

		bool m_bIsChangingStage;
		//bool m_bCencored;
		//bool m_bDead;
		//bool m_bCasualties;


		CTRef TRef0()const { return m_TRef0a + int((m_TRef0b - m_TRef0a) / 2); }

		//temporal variable for optimization
		double m_to;//beginning of the stage
		double m_tˉ¹;//last observation in this stage
		double m_t;//first observation of the new stage
		double m_t_transfer;
		double m_RDT;
		double m_pTime;
		double m_RDR;
		double m_pRate;
		double m_RF;
		double m_pFecundity;



		static std::string GetAdultName() { return ADULT_NAME; }
		static void SetAdultName(std::string name) { ADULT_NAME = name; }


	protected:

		static std::string ADULT_NAME;
	};

	class CTobsSeries;
	class CObservation : public CDataRow
	{
	public:

		CObservation()
		{
			m_treatment_type = NOT_INIT;
			m_i_temporal = IT_NOT_INIT;
			m_bStillAlive = false;
			m_stage_end_status = NOT_INIT;
		}

		bool HaveBrood()const { return !m_statsFecundity.empty(); }
		std::string GetProfile()const { return m_variable + "_" + m_treatment + "_" + m_i; }

		double GetFecundity()const;
		double GetMaxTime() const;


		size_t m_treatment_type;
		TInputTemporal m_i_temporal;
		std::string m_variable;
		std::string m_treatment;
		std::string m_i;

		std::string m_i_terminal;
		std::string m_stage_at_end;
		size_t m_stage_end_status;
		bool m_bStillAlive;

		//bool m_bAdult;
		bool IsAdult()const
		{
			std::vector<std::string> adult_names = WBSF::Tokenize(ADULT_NAME, ",;|");

			auto it = std::find_if(adult_names.begin(), adult_names.end(),
				[&](const std::string& s) {
					return boost::iequals(s, m_variable);
				}
			);

			return it != adult_names.end();
			//return boost::iequals(m_variable, ADULT_NAME); 
		}

		size_t GetStageEndStatus() const { return m_stage_end_status; }

		CStatistic m_statsFecundity;


		//TimeSeries: one line by individual
		//Individual: one line for all individuals with the same time
		//MeanTime: one line by treatment. Can have many line if different experimentation with same temperature.

		std::vector<CDataRow> m_time_series;

		//const CTemporalVector& T()const { ASSERT(m_pTobs.get());  return  *m_pTobs; }
		//TobsSeriesPtr m_pTobs;



	protected:


	};

	class Cxi : public std::pair<std::array<double, 2>, std::array<double, 2>>
	{
	public:

		Cxi::Cxi()
		{
			//censored=false;
			//dead = false;
			treatment_type = NOT_INIT;
			stage_end_status = NOT_INIT;
			adult = false;
			//casualties = false;
			n = 0;
			to = 0;
			pTime = 0;
			TimeSD = 0;
			broods = 0;
			pFecundity = 0;
		}

		size_t treatment_type;
		size_t stage_end_status;
		//bool censored;
		//bool casualties;
		//bool dead;
		bool adult;
		size_t n;
		double to;
		double pTime;

		double TimeSD;

		double broods;
		double pFecundity;
	};

	class CxiVector : public std::vector<Cxi>
	{
	public:


		double t_min;
		double t_max;
	};



	class CDevRateTreatments : public std::vector< CObservation>
	{
	public:

		//double GetMaxTime()const;
		double GetMaxLL(TDevRateEquation  e, const vector<double>& X);



		const CxiVector& get_t_xi(TInputTemporal i_temporal, bool bUseDead, bool bUseCencored, bool bUseCasualties)const;
		const CxiVector& get_f_xi(TInputTemporal i_temporal)const;


		const CTemporalVector& Tobs()const { ASSERT(m_pTobs.get());  return  *m_pTobs; }
		void SetT(const TobsSeriesPtr& pTobs) { m_pTobs = pTobs; }


		CStatistic m_stats_time;
		CStatistic m_stats_rate;
		CStatistic m_stats_Fecundity;

		size_t treatment_type()const { assert(!empty());  return front().m_treatment_type; }
		std::string treatment()const { assert(!empty());  return front().m_treatment; }
		//bool individual()const { assert(!empty());  return front().m_bIndividual; }
		TInputTemporal i_temporal()const { assert(!empty());  return front().m_i_temporal; }
		double t_transfer()const { assert(!empty());  return front().m_t_transfer; }


	protected:

		//std::map < CTRef, std::map<CTRef, double>> m_xi;
		CxiVector m_xi;
		TobsSeriesPtr m_pTobs;
	};


	class  CDevRateStages : public std::map < std::string, CDevRateTreatments>
	{
	public:

		double GetMaxLL(TDevRateEquation  e, const std::vector<double>& X);

		CStatistic m_stats_Rate;
		CStatistic m_stats_Tobs;
		CStatistic m_stats_Fecundity;

	};




	//Stage, Treatment, Individual (or group of individuals) 
	class CDevRateData : public std::map < std::string, CDevRateStages>
	{
	public:




		static const char* INPUT_NAME[NB_DEV_INPUT];
		static TDevTimeCol get_input(const std::string& name);

		CDevRateData();
		virtual ~CDevRateData();
		void clear();


		//ERMsg load(const std::vector<std::string>& file_path);
		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);


		ERMsg SetTobs(const CTobsSeries& Tobs);


		double GetDefaultSigma(const std::string& variable)const;
		//double GetSigmaBrood(TDevRateEquation  e, const std::vector<double>& X, const std::vector<double>& T)const;
		double GetSigmaFecundity(const std::string& adult_name)const;
		void compute_stats();
		void compute_T_stats(const CTobsSeries& Tobs);


		static double ei(size_t n);
		static double cv_2_sigma(double cv, size_t n);


		bool have_individual()const;
		bool have_time_series()const;
		size_t GetNbObjects()const;
		size_t GetNbIndividuals()const;


		std::set<std::string> GetAllStages()const;
		std::set<std::string> GetAllInsectTerminal()const;
		std::set<std::string> GetAllTreatments()const;
		std::set<std::string> GetAllStageEndStatus()const;
		ERMsg VerifyInsectStillAlive()const;



		bool have_var(TDevTimeCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(TDevTimeCol c)const;
		std::vector< TDevTimeCol> m_input_pos;


		std::string m_adult_name;
		bool m_bDestructiveObs;


	protected:

		

	};

	typedef CDevRateData CFecundityData;


	//
	class CTemporalVector : public std::vector<double>
	{
	public:


		using std::vector<double>::at;
		double at(const CTRef& TRef) const { assert(TRef.GetTM() == m_TRef.GetTM());  return std::vector<double>::at(TRef - m_TRef); }


		bool IsHourly()const { return m_TRef.GetTM() == CTM::HOURLY; }
		double DailyUnits()const { return IsHourly() ? 24.0 : 1.0; }
		double GetLength()const { return size() / DailyUnits(); }; //Get length (in days)

		std::string m_treatment;
		size_t m_treatment_type;
		CTRef m_TRef;
		CTRef m_transfer_date;
		//double m_to;


		CTPeriod GetPeriod()const { return CTPeriod(m_TRef, m_TRef + size() - 1); }
		//double m_t;
		double m_t_transfer;
		//CStatistic m_stats;
	};

	//treatment
	class CTobsSeries :public std::map<std::string, CTemporalVector>
	{
	public:

		static const char* INPUT_NAME[NB_TOBS_COL];
		static TTobsCol get_input(const std::string& name);

		CTobsSeries();
		virtual ~CTobsSeries();

		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		//ERMsg verify(const CDevRateData& data)const;
		void generate(const CDevRateData& data);
		//void compute_stats();

		bool have_var(TTobsCol c)const { return get_pos(c) != NOT_INIT; }
		size_t get_pos(TTobsCol c)const;
		std::vector< TTobsCol> m_input_pos;
		//std::map<std::string, CStatistic> m_stat;
		//bool IsAllFixed()const;
	};




	typedef CDevRateData CSurvivalData;



	class CDevRateEqFile : public std::map<std::string, std::pair<CDevRateEquation::TDevRateEquation, std::vector<double>>>
	{
	public:

		static const char* INPUT_NAME[NB_DEV_INPUT];
		static TDevTimeCol get_input(const std::string& name);

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

	class CStagesTableRow
	{
	public:

		//std::string m_treatment;
		CTRef m_TRef;
		std::vector<double> m_n;
	};

	class COneSimulatedStage
	{
	public:

		double m_p;
		double m_RDR;
		std::string m_treatment;
		double m_temperature;

		double m_time;
	};

	class COneSimulatedInsect: public std::vector<COneSimulatedStage>
	{
	public:

		vector<double> get_stage_time()
		{
			vector<double> time;

			return time;
		}

	};

	class CSimulatedInsects : public std::vector<COneSimulatedInsect>
	{
	public:
		
		
	};


	typedef std::map<std::string, std::map<std::string, std::vector<CStagesTableRow>>>  CCStagesTableDataBase;
	class CCStagesTableData : public CCStagesTableDataBase
	{
	public:

		enum TColumns { C_EXP_NO, C_TREATMENT, C_DATE, C_STAGE, NB_COLUMNS };
		static std::array<std::string, NB_COLUMNS> INPUT_NAME;
		static TColumns get_input(const std::string& name);


		CCStagesTableData();
		virtual ~CCStagesTableData();


		ERMsg load(const std::string& file_path);
		ERMsg load(std::istream& io);

		void clear();
		size_t get_pos(TColumns col)const { return have_column(col) ? std::distance(m_input_pos.begin(), std::find(m_input_pos.begin(), m_input_pos.end(), col)) : NOT_INIT; }
		bool have_column(TColumns col)const { return std::find(m_input_pos.begin(), m_input_pos.end(), col) != m_input_pos.end(); }

		size_t num_treatments()const { return m_treatments.size(); }
		std::string get_treatment(size_t t)const { assert(t < m_treatments.size()); return *(std::next(m_treatments.begin(), t)); }
		size_t num_stages()const {			return m_stage_columns.size();		}

		std::vector<std::pair<std::string, size_t>> m_stage_columns;
		std::vector<size_t> m_input_pos;
		std::set<std::string> m_treatments;
	};


	//*******************************************************************************
	//CInsectParameterization
	class CInsectParameterization : public CExecutable
	{
	public:

		static const char* DATA_DESCRIPTOR;
		enum TDevRateCalibOn { CO_TIME, CO_RATE, NB_CALIB_ON };
		enum TFeedback { LOOP, ITERATION, CYCLE };
		enum TFit { F_DEV_TIME, F_SURVIVAL, F_FECUNDITY, F_STAGES_TABLE, NB_FIT_TYPE };
		enum TSAOption { SA_LOW_FAST, SA_MEDIUM_MEDIUM, SA_HI_SLOW, SA_VERY_HI_SLOW, SA_CUSTOM, NB_SA_OPTION };
		enum TOptimMethod { OM_MLL, OM_RSS, NB_OPTIM_METHOD };


		enum TMember {
			FIT_TYPE = CExecutable::NB_MEMBERS, DEV_RATE_EQUATIONS, SURVIVAL_EQUATIONS, FECUNDITY_EQUATIONS, STAGE_TABLE_EQUATIONS, EQ_OPTIONS, INPUT_FILE_NAME, TOBS_FILE_NAME,
			OUTPUT_FILE_NAME, CONTROL, COUNSTRAIN_T_LO, T_LO_VALUES, COUNSTRAIN_T_HI, T_HI_VALUES,
			FIXE_F0, F0_VALUE, FIXE_T0, T0_VALUE, LIMIT_MAX_RATE, LIMIT_MAX_RATE_P, AVOID_NULL_RATE_IN_TOBS, USE_OUTPUT_AS_INPUT, OUTPUT_AS_INTPUT_FILENAME,
			SHOW_TRACE, USE_DEAD, USE_CENCORED, USE_CASUALTIES, ADULT_NAME, SA_PRESET, OPTIM_METHOD, 
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};



		static CSAControl GetDefaultSAOptions(TSAOption TOption);
		static const char* GetMemberName(int i) { ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag() { return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject() { return CExecutablePtr(new CInsectParameterization); }
		static const char* TYPE_NAME[NB_FIT_TYPE];

		std::string m_inputFileName;
		std::string m_TobsFileName;
		std::string m_outputFileName;


		size_t m_fitType;

		CDevRateEqSelected m_eqDevRate;
		CSurvivalEqSelected m_eqSurvival;
		CFecundityEqSelected  m_eqFecundity;
		CDevRateEqSelected m_eqStageTable;
		CSAParametersMap m_eq_options;


		bool m_bFixeF0;
		double m_F0;
		bool m_bFixeT0;
		double m_T0;
		bool m_bLimitMaxRate;
		double m_LimitMaxRateP;




		bool m_bConstrainTlo;
		std::array<double, 2> m_Tlo;
		bool m_bConstrainThi;
		std::array<double, 2> m_Thi;
		std::string m_adult_name;



		bool m_bAvoidNullRateInTobs;
		bool m_bUseOutputAsInput;
		std::string m_outputAsIntputFileName;
		bool m_bShowTrace;
		bool m_bUseDead;
		bool m_bUseCencored;
		bool m_bUseCasualties;


		size_t m_SA_preset;
		size_t m_optim_method;
		CSAControl m_SAOptions;


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

		//const CSAControl& GetControl()const { return m_ctrl; }
		//void SetControl(CSAControl& control) { m_ctrl = control; }
		CSAControl GetSAOptions(size_t SA_preset, size_t optim_method)const;
		CSAControl GetSAOptions()const { return GetSAOptions(m_SA_preset, m_optim_method); }


		static ERMsg ReadParametersFromFile(const std::string& outputFilePath, std::map<std::string, std::map<std::string, std::map<std::string, double>>>& params);
	protected:

		ERMsg ExecuteStageTable(const CFileManager& fileManager, CCallback& callback);
		ERMsg ExecuteOther(const CFileManager& fileManager, CCallback& callback);



		ERMsg Optimize(std::string s, size_t e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		bool GetFValue(std::string s, size_t e, CComputationVariable& computation);
		bool IsParamValid(const std::string& var, size_t e, const std::vector<double>& P);
		bool IsRateValid(const std::string& var, size_t e, const std::vector<double>& P, double null_rate_treashold);

		ERMsg InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback);
		void WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);
		void WriteInfoEx(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback);

		double Exprep(const double& RDUM);



		//input
		CDevRateData* GetCurrentDataFile();
		CTobsSeries m_Tobs;
		CDevRateData m_devTime;
		CSurvivalData m_survival;
		CFecundityData m_fecundity;
		CCStagesTableData m_stages_table;
		CSimulatedInsects m_I;//the simulator for the stage_table

		//working data
		CSAControl m_SA_ctrl;


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