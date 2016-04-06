//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"
#include "Basic/Callback.h"
#include "Basic/UtilStd.h"
#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{

	CCallback CCallback::DEFAULT_CALLBACK;



	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CCallback::CCallback() :
		m_cancelEvent(FALSE),
		m_pauseEvent(TRUE)
	{
		Reset();
	}


	CCallback::~CCallback()
	{
		Reset();
	}

	void CCallback::Reset()
	{
		SetUserCancelMessage(WBSF::GetString(IDS_BSC_USER_BREAK));
		SetUserCancel(false);
		SetPause(false);

		m_messages.clear();
		m_messageAccumulator.clear();
		m_messageDlgAccumulator.clear();

		m_phWnd = NULL;
		m_bPumpMessage = false;
	}

	CCallback::CCallback(const CCallback& callback)
	{
		operator=(callback);
	}

	CCallback& CCallback::operator=(const CCallback& in)
	{
		if (&in != this)
		{
			m_messages = in.m_messages;
			m_messageAccumulator = in.m_messageAccumulator;
			m_messageDlgAccumulator = in.m_messageDlgAccumulator;


			m_tasks = in.m_tasks;
			m_phWnd = in.m_phWnd;
		}

		return *this;
	}

	ERMsg CCallback::StepIt(double stepBy)
	{
		ASSERT(stepBy==0 || !m_tasks.empty());
		ERMsg msg;

		if (!m_tasks.empty())
		{
			m_tasks.back().m_stepPos += (stepBy == -1) ? m_tasks.back().m_stepBy : stepBy;

			if (m_tasks.back().m_stepPos > m_tasks.back().m_nbStep)
				m_tasks.back().m_stepPos = m_tasks.back().m_nbStep;

			if (m_bPumpMessage && m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))//if single thread, must pump message
			{
				//try to limit the number of message sent
				if (int(GetCurrentStepPercent()) != int(m_tasks.back().m_oldPos))
				{
					PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 0, 0);
					m_tasks.back().m_oldPos = GetCurrentStepPercent();
				}
		
				MSG winMsg;
				while (PeekMessage((LPMSG)&winMsg, NULL, 0, 0, PM_REMOVE))
				{
					if ((winMsg.message != WM_QUIT)
						&& (winMsg.message != WM_CLOSE)
						&& (winMsg.message != WM_DESTROY)
						&& (winMsg.message != WM_NCDESTROY)
						&& (winMsg.message != WM_HSCROLL)
						&& (winMsg.message != WM_VSCROLL))
					{
						TranslateMessage((LPMSG)&winMsg);
						DispatchMessage((LPMSG)&winMsg);
					}
				}
				
			}

			if (GetUserCancel() )//&& !m_bCancelled
			{
				//m_bCancelled = true;
				ASSERT(!m_userCancelMsg.empty());
				msg.ajoute(m_userCancelMsg);
			}
		}

		return msg;
	}

	//void CCallback::SetNbStep(double nbStep, double stepBy)
	//{
	//	PushTask(m_description, nbStep, stepBy);
	//	//

	//	//m_nbStep = nbStep;
	//	//m_stepBy = stepBy;
	//	//m_stepPos = 0;
	//	//m_oldPos = -1;
	//	//m_nCurrentTask++; don't incrément task anymore. It's the responsability od the PrograssBar Dialoge owner
	//	//if (m_nCurrentTask >= m_nbTask)
	//	//m_nbTask = m_nCurrentTask + 1;

	//	
	//}


	double CCallback::GetCurrentStepPercent()const{ return !m_tasks.empty() ? (m_tasks.back().m_nbStep != 0 ? std::min(100.0, std::max(0.0, m_tasks.back().m_stepPos*100.0 / m_tasks.back().m_nbStep)) : 100.0) : 0.0; }

	void CCallback::AddMessage(const ERMsg& message, int level)
	{
		for (unsigned int i = 0; i < message.dimension(); i++)
		{
			AddMessage(message[i], level);
		}
	}

	void CCallback::AddMessage(const std::string& message, int level)
	{
		AddMessage(message.c_str(), level);
	}
	void CCallback::AddMessage(const char* message, int level)
	{
		static CCriticalSection CS;

		CS.Enter();
		level = int(GetCurrentLevel()) + std::max(0, level);

		string levelTabs;
		for (int i = 0; i < level; i++)
			levelTabs += "\t";

		string t = message;
		ReplaceString(t, "\n", "\n" + levelTabs);

		m_messages += levelTabs + t + "\n";
		m_messageAccumulator += levelTabs + t + "\n";
		m_messageDlgAccumulator += levelTabs + t + "\n";

		if (m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))
		{
			PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 0, 0);
			/*if (m_bPumpMessage)
			{
				MSG winMsg;
				while (PeekMessage((LPMSG)&winMsg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage((LPMSG)&winMsg);
					DispatchMessage((LPMSG)&winMsg);
				}
			}*/
		}

		CS.Leave();
	}


	void CCallback::DeleteMessages(bool bAccumulation)
	{
		m_messages.clear();
		if (bAccumulation)
		{
			//m_currentLevel = 0;
			m_messageAccumulator.clear();
		}
	}


	void CCallback::WaitPause()
	{
		//wait when pause is activated
		//m_pauseEvent.wait();
	}

	void CCallback::PushTask(const std::string& description, double nbStep, double stepBy)
	{
		//Lock();
		m_tasks.push(CCallbackTask(description, nbStep, stepBy));
		//Unlock();
		//StepIt(0);
	}

	void CCallback::PopTask()
	{
		if (!m_tasks.empty())
		{
			//Lock();
			m_tasks.pop();
			//Unlock();
		}
	}

}//namespace WBSF