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
		m_nbTask = 0;
		m_nCurrentTask = -1;
		m_bCancelled = false;

		m_nbStep = 0;
		m_stepBy = 0;
		m_stepPos = 0;
		m_currentLevel = 0;
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
			m_nbTask = in.m_nbTask;
			m_nCurrentTask = in.m_nCurrentTask;
			m_nbStep = in.m_nbStep;
			m_stepBy = in.m_stepBy;
			m_stepPos = in.m_stepPos;
			m_currentLevel = in.m_currentLevel;
			m_oldPos = in.m_oldPos;
			m_phWnd = in.m_phWnd;
		}

		return *this;
	}

	ERMsg CCallback::StepIt(double stepBy)
	{
		ERMsg msg;

		m_stepPos += (stepBy == -1) ? m_stepBy : stepBy;

		if (m_stepPos > m_nbStep)
			m_stepPos = m_nbStep;

		if (m_phWnd && *m_phWnd && ::IsWindow(*m_phWnd))
		{
			//try to limit the number of message sent
			if (int(GetCurrentStepPercent()) != int(m_oldPos))
			{
				PostMessage(*m_phWnd, WM_MY_THREAD_MESSAGE, 0, 0);
				m_oldPos = GetCurrentStepPercent();
			}

			//a revoir
			if (m_bPumpMessage)
			{
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
		}

		if (GetUserCancel() && !m_bCancelled)
		{
			m_bCancelled = true;
			ASSERT(!m_userCancelMsg.empty());
			msg.ajoute(m_userCancelMsg);
		}

		return msg;
	}

	void CCallback::SetNbStep(double nbStep, double stepBy)
	{
		m_nbStep = nbStep;
		m_stepBy = stepBy;
		m_stepPos = 0;
		m_oldPos = -1;
		m_nCurrentTask++;
		if (m_nCurrentTask >= m_nbTask)
			m_nbTask = m_nCurrentTask + 1;

		StepIt(0);
	}


	double CCallback::GetCurrentStepPercent()const{ return m_nbStep != 0 ? std::min(100.0, std::max(0.0, m_stepPos*100.0 / m_nbStep)) : 100.0; }

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
		level = m_currentLevel + std::max(0, level);

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
			if (m_bPumpMessage)
			{
				MSG winMsg;
				while (PeekMessage((LPMSG)&winMsg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage((LPMSG)&winMsg);
					DispatchMessage((LPMSG)&winMsg);
				}
			}
		}

		CS.Leave();
	}


	void CCallback::DeleteMessages(bool bAccumulation)
	{
		m_messages.clear();
		if (bAccumulation)
		{
			m_currentLevel = 0;
			m_messageAccumulator.clear();
		}
	}


	void CCallback::WaitPause()
	{
		//wait when pause is activated
		m_pauseEvent.wait();
	}


}//namespace WBSF