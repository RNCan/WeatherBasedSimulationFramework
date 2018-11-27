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

#include "Simulation/Executable.h"

namespace WBSF
{
	class CFileManager;
	class CWGInput;
	class CLocationVector;
	class CSearchResultVector;
	class CWeatherGenerator;
	class CHourlyDatabase;
	class CDailyDatabase;
	class CNormalsDatabase;


	class CWGInputAnalysisDlg;
	class CWGInputAnalysis : public CExecutable
	{

	public:
		//S_LOWEST, S_HIGHEST, S_COEF_D,
		enum TStatistic{ S_MEAN_OBS, S_MEAN_SIM, S_BIAS, S_MAE, S_RMSE, S_STAT_R², S_NB_STAT };
		static const size_t STATISTICS[S_NB_STAT];

		friend CWGInputAnalysisDlg;
		enum TKind {
			MATCH_STATION_NORMALS, MATCH_STATION_OBSERVATIONS,//, MATCH_STATION_HOURLY, 
			ESTIMATE_ERROR_NORMALS, ESTIMATE_ERROR_OBSERVATIONS,
			XVALIDATION_NORMALS, XVALIDATION_OBSERVATIONS,
			KERNEL_VALIDATION, EXTRACT_NORMALS, MISSING_OBSERVATIONS, LAST_OBSERVATION, 
			DB_COMPLETENESS, NB_KIND
		};

		enum TMember{
			KIND = CExecutable::NB_MEMBERS, EXPORT_MATCH, MATCH_NAME, NB_MEMBERS,
			NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CWGInputAnalysis()); }


		//*** public member ***
		int m_kind;
		bool m_bExportMatch;
		std::string m_matchName;
		//*********************

		CWGInputAnalysis();
		CWGInputAnalysis(const CWGInputAnalysis& in);
		virtual ~CWGInputAnalysis();
		virtual const char* GetClassName()const{ return XML_FLAG; }

		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CWGInputAnalysis(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CWGInputAnalysis&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CWGInputAnalysis&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		void Reset();
		CWGInputAnalysis& operator =(const CWGInputAnalysis& in);
		bool operator == (const CWGInputAnalysis& in)const;
		bool operator != (const CWGInputAnalysis& in)const{ return !operator==(in); }

		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }

	protected:

		ERMsg GetWGInput(const CFileManager& fileManager, CWGInput& WGInput);
		ERMsg InitDefaultWG(const CFileManager& fileManager, CWeatherGenerator& WG, CCallback& callback);
		ERMsg MatchStation(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);

		ERMsg XValidationNormal(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg XValidationObservations(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg NormalError(const CFileManager& fileManager, CResult& resultDB, CCallback& callback);
		ERMsg ObservationsError(const CFileManager& fileManager, CResult& resultDB, CCallback& callback);
		ERMsg KernelValidation(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg ExtractNormal(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg GetNbMissingObservations(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg LastObservation(const CFileManager& fileManager, CResult& resultsDB, CCallback& callback);
		ERMsg GetCompleteness(const CFileManager& fileManager, CResult& resultDB, CCallback& callback);



		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;

	};


}