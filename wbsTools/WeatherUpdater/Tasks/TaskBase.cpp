#include "stdafx.h"
#include "TaskBase.h"
#include "basic/UtilStd.h"
#include "../resource.h"
#include "Basic/utilzen.h"
#include "TaskFactory.h"


using namespace std;



namespace WBSF
{

	bool CTaskBase::APP_VISIBLE = true;


	const string CTaskBase::EMPTY_STRING;
	string CTaskBase::PROJECT_PATH;
	const char* CTaskBase::TYPE_NAME[NB_TYPES] = {"Updater", "Tools"};
	const std::string& CTaskBase::GetTypeTitle(size_t i){ static const StringVector TYPE_TITLE(GetString(IDS_TASK_TYPE_TITLE), ";|"); ASSERT(i < TYPE_TITLE.size()); return TYPE_TITLE[i]; }
	
	std::string CTaskBase::GetDir(size_t i)const
	{
		string path = Get(i);
		if (!path.empty() && !IsPathEndOk(path))
			path += "\\";

		return path;
	}

	

	size_t CTaskBase::GetTypeFromName(const std::string& name)
	{
		size_t index = NOT_INIT;
		for (size_t i = 0; i < NB_TYPES && index == NOT_INIT; i++)
			if (name == GetTypeName(i))
				index = i;

		return index;
	}

	void CTaskBase::GetAttributes(CTaskAttributes& att)const
	{
		att.clear();

		for (size_t i = 0; i < GetNbAttributes(); i++)
			att.push_back(CTaskAttribute(Type(i), Name(i), Title(i), Description(i), Option(i), Default(i)));
	}

	size_t CTaskBase::GetAttributeIDFromName(const string& name)const
	{
		size_t index = NOT_INIT;
		for (size_t i = 0; i < GetNbAttributes() && index == NOT_INIT; i++)
			if (Name(i) == name)
				index = i;

		return index;
	}

	CTaskBase::CTaskBase():
		m_pProject(NULL)
	{}

	CTaskBase::CTaskBase(const CTaskBase& in) :
		m_pProject(NULL)
	{
		operator=(in);
	}

	CTaskBase::~CTaskBase()
	{}

	bool CTaskBase::operator == (const CTaskBase& in)const
	{
		bool bEqual = true;
		if (m_name != in.m_name)bEqual = false;
		if (m_bExecute != in.m_bExecute)bEqual = false;
		if (!map_compare(m_params, in.m_params))bEqual = false;

		return bEqual;
	}

	CTaskBase& CTaskBase::operator=(const CTaskBase& in)
	{
		if (&in != this)
		{
			m_params.clear();

			m_name = in.m_name;
			m_bExecute = in.m_bExecute;
			m_params.insert(in.m_params.begin(), in.m_params.end());
			m_pProject = in.m_pProject;

			//the last msg follow object
			m_lastMsg = in.m_lastMsg;
		}
		
		ASSERT(*this == in);

		return *this;
	}

	ERMsg CTaskBase::Init(CTasksProject* pProject, CCallback& callback)
	{
		ERMsg msg;

		m_pProject = pProject;
		for (size_t i = 0; i < GetNbAttributes(); i++)
			m_params.insert( make_pair(Name(i), Default(i)) );

		return msg;
	}

	std::string CTaskBase::GetRelativeFilePath(const std::string& filePath)const
	{
		string projectPath = GetProjectPath();
		return GetRelativePath(projectPath, filePath);
	}

	string CTaskBase::GetAbsoluteFilePath(const string& filePath)const
	{
		string projectPath = GetProjectPath();
		return GetAbsolutePath(projectPath, filePath);
	}

	const std::string& CTaskBase::Get(const std::string& i)const
	{
		CTaskParameters::const_iterator it = m_params.find(i);
		if (it == m_params.end())
			return EMPTY_STRING;

		return it->second;
	}

	void CTaskBase::Set(const std::string& i, const std::string& value)
	{
		m_params[i] = value;
	}

	std::string CTaskBase::GetUpdaterList(const CUpdaterTypeMask& types)const
	{ 
		ASSERT(m_pProject); 
		return m_pProject->GetUpdaterList(types);
	}

	const std::string& CTaskBase::Description(size_t i)const
	{
		return EMPTY_STRING;
	}
	
	const std::string& CTaskBase::Title(size_t i)const
	{ 
		if (ATTRIBUTE_TITLE.empty())
			const_cast<CTaskBase*>(this)->ATTRIBUTE_TITLE.LoadString(GetTitleStringID(), "|;");

		ASSERT(i<GetNbAttributes()); 
		return ATTRIBUTE_TITLE[i]; 
	}

	std::string CTaskBase::Option(size_t i)const
	{
		return EMPTY_STRING;
	}

	std::string CTaskBase::Default(size_t i)const
	{
		return EMPTY_STRING;
	}

	ERMsg CTaskBase::GetStationList(StringVector& stationList, CCallback& callback)
	{
		return ERMsg();
	}

	ERMsg CTaskBase::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		return ERMsg();
	}

	void CTaskBase::writeStruc(zen::XmlElement& output)const
	{
		for (WBSF::CTaskParameters::const_iterator it = m_params.begin(); it != m_params.end(); it++)
		{
			zen::XmlElement& p = output.addChild("Parameters");
			p.setAttribute("name", it->first);

			string value = it->second;

			size_t i = GetAttributeIDFromName(it->first);
			if (i != NOT_INIT)//some parameters can be load from old format
			{
				size_t t = Type(i);
				if (t == T_PATH || t == T_FILEPATH)
					value = GetRelativeFilePath(value);
				//else if (t== T_PASSWORD)
					//value = Encrypt(value);

				p.setValue(value);
			}
		}
	}

	bool CTaskBase::readStruc(const zen::XmlElement& input)
	{
		m_params.clear();

		auto iterPair = input.getChildren();
		for (auto it = iterPair.first; it != iterPair.second; ++it)
		{
			string name;
			std::string value;

			it->getAttribute("name", name);
			it->getValue(value);

			size_t i = GetAttributeIDFromName(name);
			
			if (i != NOT_INIT )
			{
				size_t t = Type(i);

				if (t == T_PATH || t == T_FILEPATH)
					value = GetAbsoluteFilePath(value);
				//else if (t == T_PASSWORD )
					//value = Decrypt(value);
			}

			m_params[name] = value;
			
		}


		return true;
	}
	
	bool CTaskBase::CopyToClipBoard()const
	{
		
		zen::XmlDoc task("WeatherUpdaterTask");
		task.root().setAttribute("type", WBSF::CTaskBase::GetTypeName(ClassType()));
		zen::XmlElement& p = task.root().addChild("Task");
		p.setAttribute("type", ClassName());
		p.setAttribute("name", m_name);
		p.setAttribute("execute", m_bExecute);
		CTaskBase::writeStruc(p);
		
		//std::string str = zen::to_string(task, "WeatherUpdaterTask", "1");
		std::string str = zen::serialize(task); //throw ()
		return WBSF::SetClipboardText(str);
	}

	bool CTaskBase::PasteFromClipBoard()
	{
		bool bRep = false;

		string str = WBSF::GetClipboardText();

		try
		{
			zen::XmlDoc doc = zen::parse(str);
			if (doc.root().getNameAs<string>() == "WeatherUpdaterTask")
			{
				auto it = doc.root().getChild("Task");
				if (it)
				{
					string className;
					it->getAttribute("type", className);
					it->getAttribute("name", m_name);
					it->getAttribute("execute", m_bExecute);
					if (className == ClassName())
						bRep = readStruc(*it);
				}
			}
		}
		catch (...)
		{
		}

		return bRep;
	}

	ERMsg CTaskBase::CreateMMG(string filePathOut, CCallback& callback)
	{
		return ERMsg();
	}

	
	//****************************************************************************************************************************
	
	CTasksProject::CTasksProject()
	{
	}

	CTasksProject::CTasksProject(const CTasksProject& in)
	{
		operator=(in);
	}

	void CTasksProject::clear()
	{
		fill(CTaskPtrVector());
	}

	CTasksProject& CTasksProject::operator = (const CTasksProject& in)
	{
		if (&in != this)
		{
			clear();

			CTasksProject& me = *this;
			for (size_t i = 0; i < in.size(); i++)
			{
				me[i].resize(in[i].size());
				for (size_t ii = 0; ii != in[i].size(); ii++)
				{
					me[i][ii] = CTaskFactory::CreateObject(in[i][ii]->ClassName());
					*(me[i][ii]) = *(in[i][ii]);
					(me[i][ii])->SetProject(this);
				}
			}
		}

		ASSERT(*this == in);


		return *this;
	}

	bool CTasksProject::operator == (const CTasksProject& in)const
	{
		const CTasksProject& me = *this;

		bool bEqual = true;
		for (size_t i = 0; i != size() && bEqual; i++)
		{
			bEqual = me[i].size() == in[i].size();
			for (size_t ii = 0; ii != me[i].size() && bEqual; ii++)
				bEqual = (*(me[i][ii]) == *(in[i][ii]));
		}
		return bEqual;
	}


	ERMsg CTasksProject::Load(const std::string& filePath)
	{
		ERMsg msg;

		CTaskBase::SetProjectPath(GetPath(filePath));
		msg = zen::LoadXML(filePath, "WeatherUpdater", "2", *this);
		if (!msg)
		{
			if (zen::LoadXML(filePath, "UpdaterDocument", "1", *this))
				msg = ERMsg();
		}

		for (size_t i = 0; i != size() ; i++)
		{
			for (size_t ii = 0; ii != at(i).size(); ii++)
				(at(i).at(ii))->SetProject(this);
		}

		if (msg)
			m_filePaht = filePath;

		return msg;
	}

	ERMsg CTasksProject::Save(const std::string& filePath)
	{
		ERMsg msg;

		CTaskBase::SetProjectPath(GetPath(filePath));
		msg = zen::SaveXML(filePath, "WeatherUpdater", "2", *this);

		if (msg)
			m_filePaht = filePath;

		return msg;
	}


	size_t CTasksProject::GetNbExecutes(size_t t)const
	{
		size_t nbExec = 0;
		
		
		for (size_t tt = 0; tt != size(); tt++)
		{
			if (t == NOT_INIT || tt == t)
			{
				for (CTaskPtrVector::const_iterator it = at(tt).begin(); it != at(tt).end(); it++)
					if ((*it)->m_bExecute)
						nbExec++;
			}
		}
			

		return nbExec;
	}

	ERMsg CTasksProject::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_EXECUTE_TASK), GetNbExecutes());
		for (CTasksProject::iterator it1 = begin(); it1 != end() && !callback.GetUserCancel(); it1++)
		{
			for (CTaskPtrVector::iterator it2 = it1->begin(); it2 != it1->end() && !callback.GetUserCancel(); it2++)
			{
				if ((*it2)->m_bExecute)
				{
					ASSERT((*it2)->GetProject() == this);

					callback.DeleteMessages(true);
					callback.AddMessage("********     " + (*it2)->m_name + "     ********");
					callback.AddMessage("");
					callback.AddMessage(GetCurrentTimeString());
					
					
					ERMsg msgTmp = (*it2)->Execute(callback);
					
					ASSERT(callback.GetNbTasks() == 1);
					while (callback.GetNbTasks() > 1)
						callback.PopTask();

					callback.AddMessage("");
					callback.AddMessage(GetCurrentTimeString());
					callback.AddMessage("*******************************************");


					std::string str = GetOutputString(msgTmp, callback, false, "\n");
					ReplaceString(str, "\n", "|");
					ReplaceString(str, "\r", "");

					(*it2)->SetLastMsg(str);
					msg += msgTmp;

					

					msg += callback.StepIt();
				}
			}
		}

		callback.PopTask();

		return msg;
	}
	

	CTaskPtr CTasksProject::GetTask(size_t t, const std::string& name)const
	{
		ASSERT(t < NB_TYPE);

		CTaskPtr pTask;
		
		CTaskPtrVector::const_iterator it = std::find_if(at(t).begin(), at(t).end(), ByName(name));
		if (it != at(t).end())
			pTask = *it;


		return pTask;
	}


	void CTasksProject::writeStruc(zen::XmlElement& output)const
	{
		const CTasksProject& me = *this;
		ASSERT(me.size() == WBSF::CTaskBase::NB_TYPES);

		for (size_t i = 0; i < me.size(); i++)
		{
			if (!me[i].empty())
			{
				zen::XmlElement& p = output.addChild("Tasks");
				p.setAttribute("type", WBSF::CTaskBase::GetTypeName(i));

				for (auto it = me[i].begin(); it != me[i].end(); it++)
				{
					zen::XmlElement& pp = p.addChild("Task");
					pp.setAttribute("type", (*it)->ClassName());
					pp.setAttribute("name", (*it)->m_name);
					pp.setAttribute("execute", (*it)->m_bExecute);
					(*it)->writeStruc(pp);
				}
			}
		}
	}

	bool CTasksProject::readStruc(const zen::XmlElement& input)
	{
		CTasksProject& me = *this;

		auto typeIt = input.getChildren();
		ASSERT(std::distance(typeIt.first, typeIt.second) <= WBSF::CTaskBase::NB_TYPES);

		for (auto itt = typeIt.first; itt != typeIt.second; ++itt)
		{
			std::string classType;
			itt->getAttribute("type", classType);

			size_t ii = WBSF::CTaskBase::GetTypeFromName(classType);
			ASSERT(ii < WBSF::CTaskBase::NB_TYPES);
			if (ii < WBSF::CTaskBase::NB_TYPES)
			{
			
				auto taskIt = itt->getChildren();
				for (auto it = taskIt.first; it != taskIt.second; ++it)
				{
					std::string tmp = it->getNameAs<std::string>();
					ASSERT(tmp == "Task");

					std::string className;

					it->getAttribute("type", className);

					if (WBSF::CTaskFactory::IsRegistered(className))
					{
						WBSF::CTaskPtr pTask = WBSF::CTaskFactory::CreateObject(className);
						ASSERT(pTask.get());

						it->getAttribute("name", pTask->m_name);
						it->getAttribute("execute", pTask->m_bExecute);

						pTask->readStruc(*it);
						me[ii].push_back(pTask);
					}
					else
					{
						//return false;
						//skip only unknown task type
					}
				}
			}
		}

		return true;
	}

	
	std::string CTasksProject::GetUpdaterList(const CUpdaterTypeMask& types)const
	{
		std::string str;
		CTasksProjectBase::const_iterator updateIt = begin() + CTaskBase::UPDATER;
		for (CTaskPtrVector::const_iterator it = updateIt->begin(); it != updateIt->end(); it++)
		{
			bool bHourlyTask = (*it)->IsHourly();
			bool bDailyTask = (*it)->IsDaily();
			bool bForecastTask = (*it)->IsForecast();
			bool bDatabaseTask = (*it)->IsDatabase();
			bool bGribsTask = (*it)->IsGribs();
			bool bMMGTask = (*it)->IsMMG();
			
			bool bTime = (types[IS_HOURLY] == bHourlyTask) || (types[IS_DAILY] == bDailyTask);
			bool bForecast = types[IS_FORECAST] == bForecastTask;
			bool bType = types[IS_DATABASE] == bDatabaseTask && types[IS_GRIBS] == bGribsTask && types[IS_MMG] == bMMGTask;
			if (bTime && bForecast && bType)
			{
				str += str.empty() ? "" : "|";
				str += (*it)->m_name;//que faire si plusieur fois le mem nom???
			}
				
		}

		return str;
	}

	
	
}

