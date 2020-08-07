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
	const size_t CCreateRadarAnimation::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_UPDATER, T_PATH, T_DATE, T_DATE, T_STRING, T_STRING, T_STRING, T_BOOL, T_BOOL };
	const UINT CCreateRadarAnimation::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_ANIMATION_P;
	const UINT CCreateRadarAnimation::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_ANIMATION;


	const char* CCreateRadarAnimation::CLASS_NAME() { static const char* THE_CLASS_NAME = "CreateRadarAnimation";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateRadarAnimation::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateRadarAnimation::CLASS_NAME(), (createF)CCreateRadarAnimation::create);



	CCreateRadarAnimation::CCreateRadarAnimation(void)
	{}

	CCreateRadarAnimation::~CCreateRadarAnimation(void)
	{}


	std::string CCreateRadarAnimation::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case RADAR_INPUT:	str = GetUpdaterList(CUpdaterTypeMask(false, false, false, false, false, false, true)); break;
		};


		return str;
	}

	std::string CCreateRadarAnimation::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case START_DATE:  str = (CTRef::GetCurrentTRef() - 1).GetFormatedString("%Y-%m-%d"); break;
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
		{
			CTRef begin(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, start_hour);
			CTRef end(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, end_hour);
			p = CTPeriod(begin, end + 24);
		}

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
		string radar_id;



		string title = GetFileTitle(filePath);
		StringVector tmp(title, "_");
		ASSERT(tmp.size() == 4 || tmp.size() == 5);

		if (tmp.size() >= 2)
			radar_id = tmp[1];

		/*if (title.length() > 16)
		{
			ID = title.substr(13, 3);
		}*/

		return radar_id;
	}
	string CCreateRadarAnimation::GetAnimationFilePath(CTRef TRef, string ID)const
	{
		string file_path;

		string output = GetDir(OUTPUT_DIR);
		CTPeriod period = GetPeriod();
		size_t start_hour = as<size_t>(FIRST_HOUR);
		size_t end_hour = as<size_t>(LAST_HOUR);
		ASSERT(end_hour <= start_hour);

		if (period.IsInside(TRef))
		{
			CTRef TRefAn;

			if (TRef.GetHour() >= start_hour)
				TRefAn = TRef.as(CTM::DAILY);
			else if (TRef.GetHour() < end_hour)
				TRefAn = TRef.as(CTM::DAILY) - 1;

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

		CTaskPtr pTask = m_pProject->GetTask(UPDATER, Get(RADAR_INPUT));

		if (pTask.get() != NULL)
		{
			string outputDir = GetDir(OUTPUT_DIR);

			size_t delay = as<size_t>(FRAME_DELAY);
			bool bLoop = as<bool>(LOOP);



			callback.AddMessage("Create animation from:");
			callback.AddMessage(Get(RADAR_INPUT), 1);
			callback.AddMessage("To:");
			callback.AddMessage(outputDir, 1);
			callback.AddMessage("");

			msg = CreateMultipleDir(outputDir);

			CTPeriod p = GetPeriod().as(CTM::DAILY);


			map<string, StringVector> animationList;
			map<string, StringVector> radarList;

			msg += pTask->GetRadarList(p, radarList, callback);

			//StringVector dir = GetDirectoriesList(inputDir + "*");
			//callback.PushTask("Get file list", radarList.size());
			for (auto it = radarList.begin(); it != radarList.end() && msg; it++)
			{
				string radar_id = it->first;
				const StringVector& tmpList = it->second;
				for (size_t i = 0; i < tmpList.size() && msg; i++)
				{
					CTRef TRef = GetTRef(tmpList[i]);
					string an_file_path = GetAnimationFilePath(TRef, radar_id);
					if (!an_file_path.empty())
						animationList[an_file_path].push_back(tmpList[i]);

					msg += callback.StepIt(0);
				}
			}

			//clean radar list
			for (auto it = animationList.begin(); it != animationList.end() && msg; )
			{
				if (NeedUpdate(*it))
					it++;
				else
					it = animationList.erase(it);

				msg += callback.StepIt(0);
			}

			//callback.PopTask();



			callback.PushTask("Create " + to_string(animationList.size()) + " animations", animationList.size());

			for (auto it = animationList.begin(); it != animationList.end() && msg; it++)
			{

				msg += MakeGIF(it->first, it->second, unsigned short(delay / 10), bLoop);
				msg += callback.StepIt();
			}

			callback.PopTask();


		}
		else
		{
			msg.ajoute(FormatMsg(IDS_TASK_NOT_EXIST, Get(RADAR_INPUT)));
		}



		//string inputDir = GetDir(INPUT_DIR);


		return msg;
	}

	bool CCreateRadarAnimation::NeedUpdate(const pair<string, StringVector>& it)
	{
		bool bNeedUpdate = false;

		bool bCreateAll = as<bool>(CREATE_ALL);

		string an_file_path = it.first;

		if (!FileExists(an_file_path) || bCreateAll)
		{
			bNeedUpdate = true;
		}
		else
		{
			CFileInfo ani_info = GetFileInfo(an_file_path);
			const StringVector& tmpList = it.second;
			for (size_t i = 0; i < tmpList.size() && !bNeedUpdate; i++)
			{
				CFileInfo info = GetFileInfo(tmpList[i]);

				if (info.m_time >= ani_info.m_time)
					bNeedUpdate = true;
				//msg += callback.StepIt(0);
				//}
			}
		}

		return bNeedUpdate;
	}


}