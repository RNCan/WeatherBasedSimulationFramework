#include "StdAfx.h"
#include "CreateRadarAnimation.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "Basic/CreateAnimation.h"

using namespace std;

namespace WBSF
{


	//*********************************************************************
	const char* CCreateRadarAnimation::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Source", "Output", "StartDate", "EndDate" , "FirstHour", "LastHour", "FrameDelay", "Loop", "CreateAll" };
	const size_t CCreateRadarAnimation::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_PATH, T_DATE, T_DATE, T_STRING, T_STRING, T_STRING, T_BOOL, T_BOOL };
	const UINT CCreateRadarAnimation::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_ANIMATION_P;
	const UINT CCreateRadarAnimation::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_ANIMATION;
	

	const char* CCreateRadarAnimation::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateRadarAnimation";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateRadarAnimation::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateRadarAnimation::CLASS_NAME(), (createF)CCreateRadarAnimation::create);

	

	CCreateRadarAnimation::CCreateRadarAnimation(void)
	{}

	CCreateRadarAnimation::~CCreateRadarAnimation(void)
	{}


	std::string CCreateRadarAnimation::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CCreateRadarAnimation::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case INPUT_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Radar\\"; break;
		case START_DATE:  str = (CTRef::GetCurrentTRef()-1).GetFormatedString("%Y-%m-%d"); break;
		case END_DATE:    str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break; 
		case FIRST_HOUR: str = "22"; break;
		case LAST_HOUR: str = "08"; break;
		case FRAME_DELAY: str = "200"; break;
		case LOOP: str = "1"; break;
		case CREATE_ALL: str = "0"; break;
		};

		return str;
	}

	CTPeriod CCreateRadarAnimation::GetPeriod()const
	{
		CTPeriod p;
		size_t start_hour = as<size_t>(FIRST_HOUR);
		size_t end_hour = as<size_t>(LAST_HOUR);
		StringVector t1(Get(START_DATE), "-/");
		StringVector t2(Get(END_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, start_hour), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, end_hour));

		return p;
	}

	CTRef CCreateRadarAnimation::GetTRef(const std::string& filePath)
	{
		CTRef TRef;
		
		string title = GetFileTitle(filePath);
		if (title.length() > 12)
		{
			int year = WBSF::as<int>(title.substr(0, 4));
			size_t m = WBSF::as<int>(title.substr(4, 2)) - 1;
			size_t d = WBSF::as<int>(title.substr(6, 2)) - 1;
			size_t h = WBSF::as<int>(title.substr(8, 2));
			TRef = CTRef(year, m, d, h);
		}
		
		return TRef;
	}
	
	string CCreateRadarAnimation::GetID(const std::string& filePath)
	{
		string ID;

		string title = GetFileTitle(filePath);
		if (title.length() > 16)
		{
			ID = title.substr(13, 3);
		}

		return ID;
	}
	string CCreateRadarAnimation::GetAnimationFilePath(CTRef TRef, string ID)const
	{
		string file_path;
		
		string output = GetDir(OUTPUT_DIR);
		CTPeriod period = GetPeriod();
		size_t start_hour = as<size_t>(FIRST_HOUR);
		size_t end_hour = as<size_t>(LAST_HOUR);

		if (period.IsInside(TRef))
		{
			CTRef TRefAn;
			
			if (TRef.GetHour() >= start_hour)
				TRefAn = TRef.as(CTM::DAILY);
			else if(TRef.GetHour() < end_hour)
				TRefAn = TRef.as(CTM::DAILY) -1;

			if (TRefAn.IsInit())
			{
				file_path = output + ID + TRefAn.GetFormatedString("-%Y-%m-%d") + "-an.gif";
			}
		}
		
		return file_path;
	}

	ERMsg CCreateRadarAnimation::Execute(CCallback& callback)
	{
		ERMsg msg;

		string inputDir = GetDir(INPUT_DIR);
		string outputDir = GetDir(OUTPUT_DIR);
		
		size_t delay = as<size_t>(FRAME_DELAY);
		bool bLoop = as<bool>(LOOP);
		bool bCreateAll = as<bool>(CREATE_ALL);

		callback.AddMessage("Create animation from:");
		callback.AddMessage(inputDir, 1);
		callback.AddMessage("To:");
		callback.AddMessage(outputDir, 1);
		callback.AddMessage("");
		
		msg = CreateMultipleDir(outputDir);


		map<string, StringVector> fileList;
		StringVector tmpList = GetFilesList(inputDir + "*.gif", 2, true);
		for (size_t i = 0; i<tmpList.size(); i++)
		{
			string ID = GetID(tmpList[i]);
			CTRef TRef = GetTRef(tmpList[i]);
			string an_file_path = GetAnimationFilePath(TRef, ID);
			if (!an_file_path.empty())
			{
				if (!FileExists(an_file_path) || bCreateAll)
					fileList[an_file_path].push_back(tmpList[i]);
			}
		}

		
		callback.PushTask("Create " + to_string(fileList.size()) + " animations", fileList.size());
		
		for (auto it = fileList.begin(); it != fileList.end(); it++)
		{
			msg += MakeGIF(it->first, it->second, unsigned short(delay/10), bLoop);
			msg += callback.StepIt();
		}

		callback.PopTask();


		return msg;
	}



}