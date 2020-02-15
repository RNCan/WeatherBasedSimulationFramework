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


#include "Basic/GeoBasic.h"
#include "Basic/QGISPalette.h"
#include "Simulation/Executable.h"


namespace WBSF
{

	class CPrePostTransfo;
	class CGridInterpolParam;
	class CGridInterpol;
	class CFileManager;
	class CResult;
	class CDBMetadata;

	class CMapping : public CExecutable
	{
	public:


		enum TMember{
			METHOD = CExecutable::NB_MEMBERS, DEM_NAME, TEM_NAME, PREPOST_TRANSFO, XVAL_ONLY, USE_HXGRID, SI_PARAMETER, QGIS_OPTIONS, NB_MEMBERS,
			NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CMapping()); }

		int m_method;

		std::string m_DEMName;
		std::string m_TEMName;
		std::unique_ptr<CPrePostTransfo> m_pPrePostTransfo;
		std::unique_ptr<CGridInterpolParam> m_pParam;
		bool m_XValOnly;
		bool m_bUseHxGrid;
		CCreateStyleOptions m_createStyleFile;



		CMapping();
		CMapping(const CMapping& in);
		virtual ~CMapping();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CMapping(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CMapping&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CMapping&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		void Reset();
		CMapping& operator =(const CMapping& in);
		bool operator == (const CMapping& in)const;
		bool operator != (const CMapping& in)const{ return !operator==(in); }

		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);
		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual int GetNbTask()const;


		size_t GetNbMapVariableIndex(const CModelOutputVariableDefVector& outputDef)const;
		size_t GetNbMapVariableIndex(CResultPtr& pResult)const{ return GetNbMapVariableIndex(pResult->GetMetadata().GetOutputDefinition()); }
		size_t GetNbMapTimeIndex(CResultPtr& pResult)const;
		size_t GetNbMapParameterIndex(const CModelInputVector& parameterSet)const;
		size_t GetNbMapParameterIndex(CResultPtr& pResult)const{ return GetNbMapParameterIndex(pResult->GetMetadata().GetParameterSet()); }
		size_t GetNbReplicationIndex(CResultPtr& pResult)const{ return pResult->GetMetadata().GetNbReplications(); }

		std::string GetTEMName(CResultPtr& pResult, size_t p, size_t r, size_t t, size_t v)const;
		std::string GetTEMFilePath(const CFileManager& fileManager, const std::string& TEMName)const;


		int GetInterpolMethod()const{ return m_method; }
		void SetInterpolMethod(int method){ m_method = method; }
		const std::string& GetDEMName()const{ return m_DEMName; }
		void SetDEMName(const std::string& name){ m_DEMName = name; }
		const std::string& GetTEMName()const{ return m_TEMName; }
		void SetTEMName(const std::string& name){ m_TEMName = name; }
		const CPrePostTransfo& GetPrePostTransfo()const{ return *m_pPrePostTransfo; }
		void SetPrePostTransfo(const CPrePostTransfo & in);


		bool GetXValOnly()const{ return m_XValOnly; }
		void SetXValOnly(bool in){ m_XValOnly = in; }
		bool GetUseHxGrid()const{ return m_bUseHxGrid; }
		void SetUseHxGrid(bool in){ m_bUseHxGrid = in; }


		ERMsg CreateStyleFile(const std::string& TEMFilePath, CTM TM)const;
	protected:

		void GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)const;

		//temporary variable
		CTPeriod m_period;

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;

		CModelOutputVariableDefVector CleanVariables(const CModelOutputVariableDefVector& outputVarIn)const;

		ERMsg InitGridInterpol(const CFileManager& fileManager, CResultPtr& pResult, size_t p, size_t r, size_t t, size_t v, CGridInterpol& mapInput, CCallback& callback);
	};

}