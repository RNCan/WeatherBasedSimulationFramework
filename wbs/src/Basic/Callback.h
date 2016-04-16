//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#ifndef __CALLBACK_H
#define __CALLBACK_H

#pragma once

#include <mutex>
#include <string>
#include <wtypes.h>
#include <algorithm>
#include <stack>

#include "basic/ERMsg.h"
#include "Basic/Event.h"



namespace WBSF
{

#define WM_MY_THREAD_MESSAGE	WM_APP+100

	class CCallbackTask
	{
	public:

		CCallbackTask(std::string description="", double nbSteps = 0, double stepBy = 1)
		{
			m_description = description;
			m_nbSteps = nbSteps;
			m_stepBy = stepBy;
			m_stepPos = 0;
			m_oldPos = -1;
		}

		std::string m_description;
		double m_nbSteps;
		double m_stepBy;
		double m_stepPos;
		double m_oldPos;
		std::string m_messages;
	};
	
	//typedef std::vector<CCallbackTask> CCallbackTaskStack;
	class CCallbackTaskStack : public std::stack < CCallbackTask >
	{
	public:
		using std::stack<CCallbackTask>::c; // expose the container

		CCallbackTask& back(){ return top(); }
		const CCallbackTask& back()const{ return top(); }
	};

class CCallback  
{
public:

	static CCallback DEFAULT_CALLBACK;

	CCallback();
	~CCallback();

	void Reset();
	void clear(){ Reset(); }

	void PushTask(const std::string& description, double nbStep, double stepBy);
	void PushTask(const std::string& description, size_t nbStep, size_t stepBy = 1){ PushTask(description, (double)nbStep, (double)stepBy); }
	void PopTask();

	double GetCurrentStepPercent();
	double GetNbStep();// { return !m_tasks.empty() ? m_tasks.back().m_nbStep : 0; }
	size_t GetNbTasks();// { return m_tasks.size(); }

	
    ERMsg StepIt(double stepBy = -1);
	double GetCurrentStepPos();// { ASSERT(!m_tasksVector.empty());  return !m_tasks.empty() ? m_tasks.back().m_stepPos : 0; }
	ERMsg SetCurrentStepPos(double stepPos);// { ASSERT(!m_tasksVector.empty());  if (!m_tasks.empty()) m_tasks.back().m_stepPos = stepPos; return StepIt(0); }
	ERMsg SetCurrentStepPos(size_t stepPos);// { ASSERT(!m_tasksVector.empty());  if (!m_tasks.empty()) m_tasks.back().m_stepPos = (double)stepPos; return StepIt(0); }

	
	bool GetUserCancel()const
	{
		CCallback& me = const_cast<CCallback &> (*this);
		return me.m_cancelEvent.isSet();
	}

	void SetUserCancel(bool bCancel = true){ bCancel ? m_cancelEvent.set() : m_cancelEvent.unset(); }
	void SetPause(bool bPause){ bPause ? m_pauseEvent.set() : m_pauseEvent.unset(); }
	void WaitPause();




	void SetPumpMessage(bool in){m_bPumpMessage=in;}
	void SetUserCancelMessage(const std::string& msg){ m_userCancelMsg=msg;}

	size_t GetCurrentLevel(); 

	void AddMessage(const std::string& message, int level = -1);
	void AddMessage(const char* message, int level = -1);
	void AddMessage(const ERMsg& message, int level = -1);
	void DeleteMessages(bool bAccumulation = false);
	std::string GetDlgMessageText()const{ return m_messageDlgAccumulator; }
	std::string GetCurrentTaskMessageText()const{ return m_messageAccumulator; }
	std::string GetMessages();


	void SetWnd(HWND* phWnd){ m_phWnd=phWnd;}



	void Lock();
	void Unlock();
	CCallbackTaskStack& GetTasks();
	

protected:

	CCallback(const CCallback& callback);
	CCallback& operator=(const CCallback& callback);


    //std::string m_messages;
	std::string m_messageAccumulator;
	std::string m_messageDlgAccumulator;


	//std::map<int, CCallbackTaskStack> m_threadTasks;
	//std::map<int, std::mutex> m_mutex;
	
	CCallbackTaskStack m_threadTasks;
	std::mutex m_mutex;
	
	manual_reset_event m_cancelEvent;
	manual_reset_event m_pauseEvent;
	bool m_bPumpMessage;
	

	std::string m_userCancelMsg;
	HWND* m_phWnd;
	
};



}

//shortcut to default callback
static WBSF::CCallback& DEFAULT_CALLBACK = WBSF::CCallback::DEFAULT_CALLBACK;

#endif