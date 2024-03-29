#pragma once

#include <map>
#include <unordered_map>
#include <bitset>

#include "Basic/ERMsg.h"
#include "Basic/Callback.h"
#include "Basic/UtilTime.h"




namespace WBSF
{

	class CWeatherStation;
	class CGribsMap;

	//*********************************************************************
	enum { T_STRING, T_STRING_SELECT, T_BOOL, T_COMBO_INDEX, T_COMBO_STRING, T_PATH, T_FILEPATH, T_GEOPOINT, T_GEORECT, T_PASSWORD, T_DATE, T_UPDATER, T_URL, NB_TYPE };

	class CTaskAttribute
	{
	public:

		CTaskAttribute(size_t type = T_STRING, const std::string& name = "", const std::string& title = "", const std::string& description = "", const std::string& option = "", const std::string& strDefault = "")
		{
			m_type = type;
			m_name = name;
			m_title = title;
			m_description = description;
			m_option = option;
			m_default = strDefault;
		}

		CTaskAttribute(const CTaskAttribute& in){ operator=(in); }
		CTaskAttribute& operator=(const CTaskAttribute& in)
		{
			if (&in != this)
			{
				m_type = in.m_type;
				m_name = in.m_name;
				m_title = in.m_title;
				m_description = in.m_description;
				m_option = in.m_option;
				m_default = in.m_default;
			}

			return *this;
		}

		size_t m_type;
		std::string m_name;
		std::string m_title;
		std::string m_description;
		std::string m_option;
		std::string m_default;

	};

	
	typedef std::vector<CTaskAttribute> CTaskAttributes;
	typedef std::map<std::string, std::string> CTaskParameters;

	//****************************************************************************************************************
	//UpdaterType
	enum TUpdater{ IS_HOURLY, IS_DAILY, IS_FORECAST, IS_DATABASE, IS_GRIBS, IS_MMG, IS_RADAR, NB_UPDATER_TYPE };
	class CUpdaterTypeMask : public std::bitset<NB_UPDATER_TYPE>
	{
	public:

		CUpdaterTypeMask(bool bIsHourly = false, bool bIsDaily = false, bool bIsForecast = false, bool bIsDatabase=false, bool bIsGribs = false, bool bIsMMG = false, bool bIsRadar= false)
		{
			CUpdaterTypeMask& me = *this;

			me[IS_HOURLY] = bIsHourly;
			me[IS_DAILY] = bIsDaily;
			me[IS_FORECAST] = bIsForecast;
			me[IS_DATABASE] = bIsDatabase;
			me[IS_GRIBS] = bIsGribs;
			me[IS_MMG] = bIsMMG;
			me[IS_RADAR] = bIsRadar;
			
		}
	};

	//****************************************************************************************************************

	class CTasksProject;
	class CTaskBase
	{
	public:

		typedef std::shared_ptr<CTaskBase>    pointer;

		enum TType{ UPDATER, TOOLS, NB_TYPES };
		
		static const char* GetTypeName(size_t i){ ASSERT(i < NB_TYPES); return TYPE_NAME[i]; }
		static const std::string& GetTypeTitle(size_t i);
		static size_t GetTypeFromName(const std::string& name);
		static void SetProjectPath(const std::string& in){ PROJECT_PATH = in; }
		static const std::string& GetProjectPath(){ return PROJECT_PATH; }

		CTaskBase();
		CTaskBase(const CTaskBase& in);
		virtual ~CTaskBase();

		std::string m_name;
		bool m_bExecute;


		bool operator ==(const CTaskBase& in)const;
		bool operator !=(const CTaskBase& in)const{ return !operator==(in); }
		CTaskBase& operator=(const CTaskBase& in);

		//parameters
		const CTaskParameters& GetParameters()const{ return m_params; }
		void SetParameters(const CTaskParameters& in){ m_params = in; }

		const std::string& Get(size_t i)const{ ASSERT(i < GetNbAttributes()); return Get(Name(i)); }
		void Set(size_t i, const std::string& value){ ASSERT(i < GetNbAttributes()); Set(Name(i), value); }
		const std::string& Get(const std::string& i)const;
		void Set(const std::string& i, const std::string& value);


		template <typename T>
		inline T as(size_t i)const{ return ToValue<T>(Get(i)); }

		std::string GetRelativeFilePath(const std::string& filePath)const;
		std::string GetAbsoluteFilePath(const std::string& filePath)const;


		//class description
		virtual const char* ClassName()const = 0;
		virtual TType ClassType()const = 0;
		virtual UINT GetTitleStringID()const = 0;
		virtual UINT GetDescriptionStringID()const = 0;
		virtual bool IsHourly()const{ return false; }
		virtual bool IsDaily()const{ return false; }
		virtual bool IsForecast()const{ return false; }
		virtual bool IsDatabase()const{ return false; }
		virtual bool IsGribs()const{ return false; }
		virtual bool IsMMG()const{ return false; }
		virtual bool IsRadar()const { return false; }




		//attribute
		virtual ERMsg Init(CTasksProject* pProject, CCallback& callback = DEFAULT_CALLBACK);
		virtual size_t GetNbAttributes()const{ return 0; }
		void GetAttributes(CTaskAttributes& info)const;
		size_t GetAttributeIDFromName(const std::string& name)const;

		//Downloader
		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK) = 0;

		//Tools
		
		virtual ERMsg Initialize(TType type, CCallback& callback = DEFAULT_CALLBACK) { return ERMsg(); }
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg CreateMMG(std::string filePathOut, CCallback& callback);
		virtual ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback = DEFAULT_CALLBACK) { return ERMsg(); }
		virtual ERMsg GetRadarList(CTPeriod p, std::map<std::string, StringVector>& imageList, CCallback& callback = DEFAULT_CALLBACK) { return ERMsg(); }
		virtual ERMsg Finalize(TType type, CCallback& callback = DEFAULT_CALLBACK){ return ERMsg(); }

		void writeStruc(zen::XmlElement& output)const;
		bool readStruc(const zen::XmlElement& input);

		virtual size_t Type(size_t i)const = 0;
		virtual const char* Name(size_t i)const=0; 
		virtual const std::string& Description(size_t i)const;
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;
		
		
		void SetProject(CTasksProject* pProject){ m_pProject = pProject;	}
		const CTasksProject* GetProject()const{ return m_pProject; }
		std::string GetDir(size_t i)const;

		const std::string& GetLastMsg()const{ return m_lastMsg; }
		void SetLastMsg(const std::string& msg)const{ const_cast<CTaskBase*>(this)->m_lastMsg = msg; }

		
		std::string GetUpdaterList(const CUpdaterTypeMask& types)const;
		const std::string& Title(size_t i)const;
		void UpdateLanguage(){ ATTRIBUTE_TITLE.clear();  }
		

		bool CopyToClipBoard()const;
		bool PasteFromClipBoard();

		static void SetAppVisible(bool bVisible){ APP_VISIBLE = bVisible; }

	protected:

		CTaskParameters m_params;
		StringVector ATTRIBUTE_TITLE;

		static std::string PROJECT_PATH;
		static const std::string EMPTY_STRING;
		static const char* TYPE_NAME[NB_TYPES];


		//for runtime operation
		CTasksProject* m_pProject;

		//last temporary message execution, not part of the class
		std::string m_lastMsg;

		static bool APP_VISIBLE;
	};

	//****************************************************************************************************************************


	typedef std::shared_ptr<CTaskBase> CTaskPtr;
	
	
	typedef std::vector<CTaskPtr> CTaskPtrVector;
	typedef std::array < WBSF::CTaskPtrVector, WBSF::CTaskBase::NB_TYPES > CTasksProjectBase;

	class CTasksProject : public CTasksProjectBase
	{
	public:

		class ByName
		{
		public:

			ByName(const std::string& name) :
				m_name(name)
			{}
			
			inline bool operator()(const CTaskPtr& in)const{ return in->m_name == m_name; }

		protected:

			std::string m_name;
		};


		CTasksProject();
		CTasksProject(const CTasksProject& in);
		CTasksProject& operator = (const CTasksProject& in);
		bool operator == (const CTasksProject& in)const;
		bool operator != (const CTasksProject& in)const{ return !operator == (in); }

		void clear();
		bool empty()const{ return at(0).empty() && at(1).empty(); }
		size_t GetNbExecutes(size_t t=NOT_INIT)const;

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		ERMsg Execute(WBSF::CCallback& callback);

		void writeStruc(zen::XmlElement& output)const;
		bool readStruc(const zen::XmlElement& input);

		CTaskPtr GetTask(size_t t, const std::string& name)const;
		std::string GetUpdaterList(const CUpdaterTypeMask& types)const;


		std::string GetFilePaht()const{ return m_filePaht; }

	protected:

		std::string m_filePaht;
	};
}


namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CTasksProject& project, XmlElement& output)
	{
		project.writeStruc(output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CTasksProject& project)
	{
		return project.readStruc(input);
	}
}

