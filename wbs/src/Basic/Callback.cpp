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
#include "OpenMP.h"

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

		//m_messages.clear();
		m_messageAccumulator.clear();
		m_messageDlgAccumulator.clear();
		m_threadTasks.clear();
		m_mutex.clear();

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
			//m_messages = in.m_messages;
			m_messageAccumulator = in.m_messageAccumulator;
			m_messageDlgAccumulator = in.m_messageDlgAccumulator;


			m_threadTasks = in.m_threadTasks;
			m_phWnd = in.m_phWnd;
		}

		return *this;
	}

	void CCallback::Lock()
	{ 
		int n = omp_get_thread_num();
		m_mutex[n].lock(); 
	}

	void CCallback::Unlock()
	{ 
		int n = omp_get_thread_num();
		m_mutex[n].unlock();
	}

	double CCallback::GetNbStep()
	{ 
		
		return !GetTasks().empty()?GetTasks().top().m_nbSteps:0;
	}

	size_t CCallback::GetNbTasks()
	{ 
		return GetTasks().size();
	}

	size_t CCallback::GetCurrentLevel(){return GetNbTasks();}
	

	CCallbackTaskStack& CCallback::GetTasks()
	{
		int n = omp_get_thread_num();
		return m_threadTasks[n];
	}

	std::string CCallback::GetMessages()
	{ 
		return !GetTasks().empty()?GetTasks().top().m_messages:"";
	}

	double CCallback::GetCurrentStepPos()
	{ 
		return !GetTasks().empty() ? GetTasks().top().m_stepPos:0;
	}

	ERMsg CCallback::SetCurrentStepPos(double stepPos)
	{
		if (!GetTasks().empty())
		{
			Lock();
			GetTasks().top().m_stepPos = stepPos;
			Unlock();
		}
	
	

		return StepIt(0);
	}
	ERMsg CCallback::SetCurrentStepPos(size_t stepPos){ return SetCurrentStepPos((double)stepPos); }

	ERMsg CCallback::StepIt(double stepBy)
	{
		ASSERT(stepBy==0 || !m_threadTasks.empty());
		ERMsg msg;


		//step it only apply on the first tread for the moment
		
		if (!m_threadTasks.empty())
		{
			m_mutex[0].lock();
			//Lock();
			if (!m_threadTasks[0].empty())
			{
				m_threadTasks[0].top().m_stepPos += (stepBy == -1) ? m_threadTasks[0].top().m_stepBy : stepBy;

				if (m_threadTasks[0].top().m_stepPos > m_threadTasks[0].top().m_nbSteps)
					m_threadTasks[0].top().m_stepPos = m_threadTasks[0].top().m_nbSteps;
			}
			//Unlock();
			m_mutex[0].unlock();

			if (!m_threadTasks[0].empty())
			{
				if (m_bPumpMessage && m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))//if single thread, must pump message
				{
					//try to limit the number of message sent
					if (int(GetCurrentStepPercent()) != int(m_threadTasks[0].top().m_oldPos))
					{
						PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 0, 0);
						m_threadTasks[0].top().m_oldPos = GetCurrentStepPercent();
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

				if (GetUserCancel())//&& !m_bCancelled
				{
					//m_bCancelled = true;
					ASSERT(!m_userCancelMsg.empty());
					msg.ajoute(m_userCancelMsg);
				}
			}
			
		}
		return msg;
	}

	double CCallback::GetCurrentStepPercent()
	{ 
		return !GetTasks().empty() ? (GetTasks().top().m_nbSteps != 0 ? std::min(100.0, std::max(0.0, GetTasks().top().m_stepPos*100.0 / GetTasks().top().m_nbSteps)) : 100.0) : 0.0;
	}

	void CCallback::AddMessage(const ERMsg& message, int level)
	{
		for (unsigned int i = 0; i < message.dimension(); i++)
			AddMessage(message[i], level);
	}

	void CCallback::AddMessage(const std::string& message, int level)
	{
		AddMessage(message.c_str(), level);
	}
	void CCallback::AddMessage(const char* message, int level)
	{
		if (!GetTasks().empty())
		{
			static CCriticalSection CS;

			CS.Enter();
			
			level = int(GetCurrentLevel()) + std::max(0, level);

			string levelTabs;
			for (int i = 0; i < level; i++)
				levelTabs += "\t";

			string t = message;
			ReplaceString(t, "\n", "\n" + levelTabs);

			GetTasks().top().m_messages += levelTabs + t + "\n";
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
	}


	void CCallback::DeleteMessages(bool bAccumulation)
	{
		//GetTasks().top().m_messages.clear();
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
		Lock();
		if (nbStep == NOT_INIT)
			nbStep = -1.0;
		
		GetTasks().push(CCallbackTask(description, nbStep, stepBy));

		if (m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))
			SendMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 1, 0);

		Unlock();
		
		
		//if (m_bPumpMessage && m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))//if single thread, must pump message
			//PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 1, 0);
	}

	void CCallback::PopTask()
	{
		if (!m_threadTasks.empty())
		{
			Lock();
			//transfer message to parent
			string messages;
			if (!GetTasks().empty())
				messages = GetTasks().top().m_messages;

			GetTasks().pop();

			if (!GetTasks().empty())
				GetTasks().top().m_messages += messages;

			if (m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))
				SendMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 2, 0);

			Unlock();

			//if (m_bPumpMessage && m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))//if single thread, must pump message
			//PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 2, 0);
			
		}
	}

}//namespace WBSF