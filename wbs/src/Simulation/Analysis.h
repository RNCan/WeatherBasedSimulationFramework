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
#include "Basic/UtilTime.h"
#include "Basic/Callback.h"
#include "Simulation/Executable.h"
#include "Simulation/SectionData.h"

namespace WBSF
{

	class CFileManager;
	class CResult;


	class CAnalysisWindow
	{
	public:


		enum TMember { SELECT_LOCATION, LOCATION_SELECTION, SELECT_TIME, TIME_SELECTION, USE_CURRENT_DATE1, SHIFT1, USE_CURRENT_DATE2, SHIFT2, SELECT_VARIABLE, VARIABLE_SELECTION, SELECT_PARAMETERS_VARIATIONS, PARAMETERS_VARIATIONS, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }



		CAnalysisWindow();
		CAnalysisWindow(const CAnalysisWindow& in);
		void Reset();

		CAnalysisWindow& operator =(const CAnalysisWindow& in);
		bool operator == (const CAnalysisWindow& in)const;
		bool operator != (const CAnalysisWindow& in)const{ return !operator==(in); }


		CTPeriod GetFinalPeriod()const;

		//Location
		bool m_bSelectLocation;
		std::string m_locations;
		//std::vector<size_t> m_locations;

		//Temporal
		bool m_bSelectPeriod;
		CTPeriod m_period;
		bool m_bUseCurrentDate1;//for begin
		int m_shift1;
		bool m_bUseCurrentDate2;//for end
		int m_shift2;

		//Parameters variations
		bool m_bSelectParametersVariations;
		std::string m_parametersVariations;
		//CVariableSelectionVector m_parametersVariations;

		//Variable
		bool m_bSelectVariable;
		std::string m_variables;
		//CVariableSelectionVector m_variables;


		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};



	class CAnalysisComputation
	{
	public:

		enum TKind{ STATISTIC, EVENT, NB_KIND };


		//enum TTType
		//{
		//	ANNUAL = CTRef::ANNUAL,
		//	MONTHLY = CTRef::MONTHLY,
		//	DAYLY = CTRef::DAILY,
		//	HOURLY = CTRef::HOURLY,
		//	ATEMPORAL = CTRef::ATEMPORAL,
		//	NB_TEMPORAL_TYPE
		//};
		//enum TTMode
		//{
		//	FOR_EACH_YEAR = CTRef::FOR_EACH_YEAR,
		//	OVERALL_YEARS = CTRef::OVERALL_YEARS,
		//	NB_TEMPORAL_MODE
		//};

		/*enum TStatistical{
			LOWEST = WBSF::LOWEST,
			MEAN = WBSF::MEAN,
			SUM = WBSF::SUM,
			SUM² = WBSF::SUM²,
			STD_DEV = WBSF::STD_DEV,
			STD_DEV_OVER_POP = WBSF::STD_DEV_OVER_POP,
			STD_ERR = WBSF::STD_ERR,
			COEF_VAR = WBSF::COEF_VAR,
			VARIANCE = WBSF::VARIANCE,
			HIGHEST = WBSF::HIGHEST,
			TSS = WBSF::TSS,
			QUADRATIC_MEAN = WBSF::QUADRATIC_MEAN,
			RANGE = WBSF::RANGE,
			NB_VALUE = NB_VALUE,
			NB_STATISTIC
		};*/
		//TW = Time When
		//FT = First Time, 
		//LT = Last Time
		enum TEvent{ TW_MAX, TW_MIN, FT_GT_K, FT_LT_K, LT_GT_K, LT_LT_K, FT_CUMUL_GT_K, FT_NON_CUMUL_GT_K, STABILISED_K, NB_TEMPORAL };
		static bool HaveK(short eventType){ return eventType >= FT_GT_K; }

		enum TMember { PREVIOUS_STATICTIC_TYPE, SELECT_COMPUTATION, T_TYPEMODE, KIND, STATICTIC_TYPE, EVENT_TYPE, K_VALUE, DROP_YEAR, MEAN_OVER_REPLICATION, MEAN_OVER_PARAMETERSET, MEAN_OVER_LOCATION, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CAnalysisComputation();
		void Reset();

		bool operator == (const CAnalysisComputation& in)const;
		bool operator != (const CAnalysisComputation& in)const{ return !operator==(in); }

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);
		//virtual void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//virtual void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }
		//std::string ToXMLString()const{return XToXMLString(*this); }
		//void FromXMLString( const std::string& str){ XFromXMLString(*this, str); }

		ERMsg Compute(const CNewSectionData& input, CNewSectionData& output)const;

		bool IsTemporalStat()const{ return WBSF::IsTemporalStat(m_previousStatisticType); }
		bool MeanOver()const{ return m_bMeanOverReplication || m_bMeanOverParameterSet || m_bMeanOverLocation; }

		short m_previousStatisticType;
		bool m_bSelectTimeTransformation;
		CTM m_TM;//temporal typeMode

		short m_kind;
		short m_statisticType2;
		short m_eventType;
		double m_K;//for event type
		bool  m_bDropYear; //for event type

		bool m_bMeanOverReplication;
		bool m_bMeanOverParameterSet;
		bool m_bMeanOverLocation;

	protected:

		ERMsg ComputeStatistic(const CNewSectionData& input, CNewSectionData& output)const;
		void ExtractStatistic(const CNewSectionData& input, CNewSectionData& output)const;
		ERMsg ComputeEvent(const CNewSectionData& input, CNewSectionData& output)const;
		CTRef ExtractEvent(const CTTransformation& p, const CNewSectionData& input, size_t c, size_t j)const;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};



	class CAnalysis : public CExecutable
	{
	public:

		enum TMember{
			XML_WINDOW = CExecutable::NB_MEMBERS, XML_EVENT, NB_MEMBERS,
			NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CAnalysis()); }



		CAnalysis();
		CAnalysis(const CAnalysis& in);
		virtual ~CAnalysis();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CAnalysis(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CAnalysis&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CAnalysis&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		void Reset();
		CAnalysis& operator =(const CAnalysis& in);
		bool operator == (const CAnalysis& in)const;
		bool operator != (const CAnalysis& in)const{ return !operator==(in); }

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);


		const CAnalysisWindow& GetWindow()const{ return m_window; }
		CAnalysisWindow& GetWindow(){ return m_window; }
		const CAnalysisComputation& GetComputation()const{ return m_computation; }
		CAnalysisComputation& GetComputation(){ return m_computation; }


		ERMsg DoAnalysis(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg DoAnalysis(CResultPtr& result, CResult& analysisDB, CCallback& callback = DEFAULT_CALLBACK);

	protected:

		CDimension GetOutputDimension(const CResultPtr& result)const;
		CLocationVector CleanLocations(const CLocationVector& inputLOC)const;
		size_t CleanReplication(size_t nbRep)const;
		CTM CleanTimeTM(CTM TM)const;
		CTPeriod CleanTimePeriod(CTPeriod p)const;
		CModelOutputVariableDefVector CleanVariables(CTM dataTM, const CModelOutputVariableDefVector& varListIn)const;

		void ClassReset();
		void GetDBInputInfo(CResultPtr& pResult, CDBMetadata& inputInfo);


		CAnalysisWindow m_window;
		CAnalysisComputation m_computation;

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;
	};

}//namesapce WBSF


//
//
//namespace zen
//{
//	
//
//	template <> inline
//	void writeStruc(const CAnalysis& in, XmlElement& output)
//	{
//		writeStruc( (const CExecutable&)in, output );
//
//
//		XmlOut out(output);
//		
//		out[CAnalysis::GetMemberName(CAnalysis::XML_WINDOW)](in.GetWindow());
//		out[CAnalysis::GetMemberName(CAnalysis::XML_EVENT)](in.GetComputation());
//	}
//
//	template <> inline
//	bool readStruc(const XmlElement& input, CAnalysis& out)
//	{
//		XmlIn in(input);
//		
//		readStruc( input, (CExecutable&)out );
//		in[CAnalysis::GetMemberName(CAnalysis::XML_WINDOW)](out.GetWindow());
//		in[CAnalysis::GetMemberName(CAnalysis::XML_EVENT)](out.GetComputation());
//
//		
//		return true;
//	}
//}



namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CAnalysisWindow& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_LOCATION)](in.m_bSelectLocation);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::LOCATION_SELECTION)](in.m_locations);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_TIME)](in.m_bSelectPeriod);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::TIME_SELECTION)](in.m_period);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::USE_CURRENT_DATE1)](in.m_bUseCurrentDate1);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SHIFT1)](in.m_shift1);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::USE_CURRENT_DATE2)](in.m_bUseCurrentDate2);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SHIFT2)](in.m_shift2);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_VARIABLE)](in.m_bSelectVariable);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::VARIABLE_SELECTION)](in.m_variables);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_PARAMETERS_VARIATIONS)](in.m_bSelectParametersVariations);
		out[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::PARAMETERS_VARIATIONS)](in.m_parametersVariations);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CAnalysisWindow& out)
	{
		XmlIn in(input);

		std::string str;

		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_LOCATION)](out.m_bSelectLocation);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::LOCATION_SELECTION)](out.m_locations);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_TIME)](out.m_bSelectPeriod);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::TIME_SELECTION)](out.m_period);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::USE_CURRENT_DATE1)](out.m_bUseCurrentDate1);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SHIFT1)](out.m_shift1);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::USE_CURRENT_DATE2)](out.m_bUseCurrentDate2);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SHIFT2)](out.m_shift2);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_VARIABLE)](out.m_bSelectVariable);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::VARIABLE_SELECTION)](out.m_variables);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::SELECT_PARAMETERS_VARIATIONS)](out.m_bSelectParametersVariations);
		in[WBSF::CAnalysisWindow::GetMemberName(WBSF::CAnalysisWindow::PARAMETERS_VARIATIONS)](out.m_parametersVariations);

		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CAnalysisComputation& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::PREVIOUS_STATICTIC_TYPE)](in.m_previousStatisticType);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::SELECT_COMPUTATION)](in.m_bSelectTimeTransformation);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::T_TYPEMODE)](in.m_TM);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::KIND)](in.m_kind);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::STATICTIC_TYPE)](in.m_statisticType2);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::EVENT_TYPE)](in.m_eventType);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::K_VALUE)](in.m_K);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::DROP_YEAR)](in.m_bDropYear);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_REPLICATION)](in.m_bMeanOverReplication);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_PARAMETERSET)](in.m_bMeanOverParameterSet);
		out[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_LOCATION)](in.m_bMeanOverLocation);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CAnalysisComputation& out)
	{
		XmlIn in(input);

		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::PREVIOUS_STATICTIC_TYPE)](out.m_previousStatisticType);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::SELECT_COMPUTATION)](out.m_bSelectTimeTransformation);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::T_TYPEMODE)](out.m_TM);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::KIND)](out.m_kind);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::STATICTIC_TYPE)](out.m_statisticType2);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::EVENT_TYPE)](out.m_eventType);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::K_VALUE)](out.m_K);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::DROP_YEAR)](out.m_bDropYear);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_REPLICATION)](out.m_bMeanOverReplication);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_PARAMETERSET)](out.m_bMeanOverParameterSet);
		in[WBSF::CAnalysisComputation::GetMemberName(WBSF::CAnalysisComputation::MEAN_OVER_LOCATION)](out.m_bMeanOverLocation);


		return true;
	}
}
