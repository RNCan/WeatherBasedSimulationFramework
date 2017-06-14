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


#include <memory>


#include "Basic/WeatherDefine.h"
#include "ModelBase/ParametersVariations.h"
#include "ModelBase/WGInput.h"
#include "Simulation/Result.h"
#include "Simulation/Export.h"
#include "Simulation/Graph.h"



namespace WBSF
{
	class CFileManager;



	class CParentInfo
	{
	public:
		enum TInfo{ NB_INFO = DIMENSION::NB_DIMENSION };

		CLocationVector m_locations;
		CModelInputVector m_parameterset;
		size_t m_nbReplications;
		CTPeriod m_period;
		CModelOutputVariableDefVector m_variables;

		CDimension GetDimension()const;
		std::string GetDimensionStr(int dim)const;
		StringVector GetDimensionList(int dim)const;
	};

	class CParentInfoFilter : public std::bitset < CParentInfo::NB_INFO >
	{
	public:

		CParentInfoFilter()
		{
			set();//set all to true
		}

		CParentInfoFilter(size_t d)
		{
			set(d);
		}

	};



	class CExecuteCtrl
	{
	public:
		CExecuteCtrl();
		void LoadDefaultCtrl();


		int m_maxDistFromLOC;
		int m_maxDistFromPoint;
		bool m_bRunEvenFar;
		bool m_bRunWithMissingYear;
		bool m_bKeepTmpFile;
		bool m_bUseHxGrid;
		char m_listDelimiter;
		char m_decimalDelimiter;
		CTRefFormat m_timeFormat;
		bool m_bExportAllLines;
		int m_nbMaxThreads;
	};

	class CExecutable;
	typedef std::shared_ptr<CExecutable> CExecutablePtr;
	typedef std::vector<CExecutablePtr> CExecutablePtrVector;
	class CExecutableVector : public CExecutablePtrVector
	{
	public:

		static const char* GetXMLFlag(){ return XML_FLAG; }


		CExecutableVector();
		CExecutableVector(const CExecutableVector& in);
		void Reset();
		CExecutableVector& operator =(const CExecutableVector& in);
		bool operator == (const CExecutableVector& in)const;
		bool operator != (const CExecutableVector& in)const{ return !operator==(in); }

		CExecutable* GetParent(){ return m_pParent; }
		void SetParent(CExecutable* pParent){ m_pParent = pParent; }

		int GetNbExecute(bool bTask = false)const;
		ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);

		void SetItem(const std::string& iName, CExecutablePtr pItem);
		CExecutablePtr FindItem(const std::string& iName)const;
		bool RemoveItem(const std::string& iName);
		void UpdateInternalName();
		bool MoveItem(const std::string& iName, const std::string& iAfter, bool bCopy);

		void writeStruc(zen::XmlElement& output)const;
		bool readStruc(const zen::XmlElement& input);

	protected:
		static const char* XML_FLAG;

		CExecutable* m_pParent;
	};


	class CExecutable
	{
	public:

		friend CExecutablePtr;
		friend CExecutableVector;
		enum TMember { NAME, INTERNAL_NAME, DESCRIPTION, EXECUTE, EXPORT_DATA, GRAPHS, EXECUTABLES, NB_MEMBERS };
		enum TExportFormat { EXPORT_CSV, EXPORT_CSV_LOC, EXPORT_SHAPEFILE, NB_EXPORT_FORMAT };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }
		static void LoadDefaultCtrl();


		std::string m_name;
		std::string m_internalName;
		std::string	m_description;
		bool	m_bExecute;
		CExecutableVector m_executables;
		CExport m_export;
		CGraphVector m_graphArray;



		CExecutable();
		CExecutable(const CExecutable& in);
		virtual ~CExecutable();

		virtual const char* GetClassName()const = 0;
		virtual CExecutablePtr CopyObject()const = 0;
		virtual CExecutable& Assign(const CExecutable& in) = 0;
		virtual bool CompareObject(const CExecutable& exe)const = 0;



		void Reset();
		virtual std::string GetTitle();

		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;


		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);
		virtual ERMsg ExecuteBasic(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg ExecuteChild(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		virtual int GetNbExecute(bool bTask = false)const;
		virtual int GetNbTask()const{ return 1; }
		virtual int GetPriority(){ return 50; }
		virtual CResultPtr GetResult(const CFileManager& fileManager)const;
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		CDimension GetDimension(const CFileManager& fileManager, bool bFromResult = false)const;
		CTPeriod GetDefaultPeriod(const CFileManager& fileManager)const;

		std::string GetExportFilePath(const CFileManager& fileManager, int format = EXPORT_CSV)const;
		std::string GetGraphFilePath(const CFileManager& fileManager)const;

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_FLOAT; }
		virtual ERMsg Export(const CFileManager& fileManager, int format = EXPORT_CSV, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg ExportGraph(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);

		void InsertItem(CExecutablePtr	pItem);
		void SetItem(const std::string& iName, CExecutablePtr pItem);
		CExecutablePtr FindItem(const std::string& iName)const;
		bool RemoveItem(const std::string& iName);
		bool MoveItem(const std::string& iName, const std::string& iAfter, bool bCopy);
		size_t GetNbItem()const{ return m_executables.size(); }
		CExecutablePtr GetItemAt(int i)const{ return m_executables[i]; }


		CExecutablePtr GetParent();
		const CExecutablePtr GetParent()const;
		virtual CExecutablePtr GetExecutablePtr();
		virtual const CExecutablePtr GetExecutablePtr()const;


		void SetParent(CExecutable* pParent){ ASSERT(pParent); m_pParent = pParent; }
		void SetParent(CExecutablePtr pParent){ ASSERT(pParent); m_pParent = pParent.get(); }

		void UpdateInternalName();
		const std::string& GetInternalName()const{ return m_internalName; }
		void SetInternalName(const std::string& name){ m_internalName = name; }
		const std::string& GetName()const{ return m_name; }
		void SetName(const std::string& in){ m_name = in; }

		const std::string& GetDescription()const{ return m_description; }
		void SetDescription(const std::string& in){ m_description = in; }

		bool GetExecute()const{ return m_bExecute; }
		void SetExecute(bool in){ m_bExecute = in; }

		const CExport& GetExport()const{ return m_export; }
		void SetExport(const CExport& in){ m_export = in; }
		const CGraphVector& GetGraph(){ return m_graphArray; }
		void SetGraph(const CGraphVector& in){ m_graphArray = in; }


		std::string GetOutputMessage(const CFileManager& fileManager);
		std::string GetValidationMessage(const CFileManager& fileManager);
		std::string GetFilePath(const std::string& path, const std::string& ext)const;
		std::string GetLogFilePath(const std::string& path)const;
		std::string GetDBFilePath(const std::string& path)const;

		static void SetExecuteCtrl(CExecuteCtrl& ctrl){ CTRL = ctrl; }
		static ERMsg WriteOutputMessage(const std::string& filePath, const std::string& logText);
		void CopyToClipBoard();
		CExecutablePtr CopyFromClipBoard();

		static double GetExecutionTime(const std::string& name, CTM TM, bool bUseHxGrid = false);
		static void SetExecutionTime(const std::string& name, double timePerUnit, CTM TM, bool bUseHxGrid = false);

	protected:

		CExecutable& operator =(const CExecutable& in);
		bool operator == (const CExecutable& in)const;
		bool operator != (const CExecutable& in)const{ return !operator==(in); }
		std::string GetDefaultTaskTitle();
		void UpdateExport();

		static std::string ReadOutputMessage(const std::string& filePath);
		static std::string GenerateInternalName();

		ERMsg ExportAsShapefile(const CFileManager& fileManager, CCallback& callback);
		ERMsg ExportAsCSV(const CFileManager& fileManager, bool bAsLoc, CCallback& callback);
		ERMsg ExecuteScript(const CFileManager& fileManager, CCallback& callback);
		CExecutablePtr GetParent(const std::string iName);

		CExecutable* m_pParent;

		static const char* MEMBERS_NAME[NB_MEMBERS];
		static CExecuteCtrl CTRL;
	};



	CExecutablePtr CreateExecutable(const std::string& className);


	/*class 
	{
	public:
	};
*/

	typedef std::set<std::string> CExpendedItem;
	class CProjectState
	{
	public:

		enum TMEMBERS{ EXPANDED, NB_MEMBERS };
		static const char* GetMemberName(size_t i){ _ASSERTE(i >= 0 && i<NB_MEMBERS); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CProjectState();
		ERMsg Load(const std::string& filePath){ return zen::LoadXML(filePath, "ProjectState", "1", *this); }
		ERMsg Save(const std::string& filePath){ return zen::SaveXML(filePath, "ProjectState", "1", *this); }

		void clear();

		CExpendedItem m_expendedItems;

	protected:

		std::string m_filePath;


		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBERS];
	};

	typedef std::shared_ptr<CProjectState> CProjectStatePtr;

}




namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CExecutableVector& in, XmlElement& output)
	{
		in.writeStruc(output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CExecutableVector& out)
	{
		//dynamic creation
		return out.readStruc(input);
	}



	template <> inline
		void writeStruc(const WBSF::CExpendedItem&in, XmlElement& output)
	{
		//XmlOut out(output);
		std::string tmp = WBSF::to_string(in, ",");
		output.setValue(tmp);
		
		//out[WBSF::CProjectState::GetMemberName(WBSF::CProjectState::EXPANDED)](tmp);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CExpendedItem&out)
	{
		//XmlIn in(input);
		std::string tmp;
		input.getValue(tmp);
		//in[WBSF::CProjectState::GetMemberName(WBSF::CProjectState::EXPANDED)](tmp);
		out = WBSF::to_object<std::string, WBSF::CExpendedItem>(tmp, ",");

		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CProjectState& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CProjectState::GetMemberName(WBSF::CProjectState::EXPANDED)](in.m_expendedItems);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CProjectState& out)
	{
		XmlIn in(input);
	
		in[WBSF::CProjectState::GetMemberName(WBSF::CProjectState::EXPANDED)](out.m_expendedItems);
	
		return true;
	}
}
