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


#include <string>
#include <wtypes.h>
#include <algorithm>

#include "basic/ERMsg.h"
#include "Basic/Event.h"



namespace WBSF
{

#define WM_MY_THREAD_MESSAGE	WM_APP+100



class CCallback  
{
public:
	CCallback();
	~CCallback();
    
    static CCallback DEFAULT_CALLBACK;

    CCallback( const CCallback& callback);
    CCallback& operator=( const CCallback& callback);
	void Reset();
	void clear(){ Reset(); }

	
	int GetNbTask()const{return m_nbTask;}
	void SetNbTask(int nbTask){	m_nbTask = nbTask; m_nCurrentTask = -1; }
	void SetNbTask(size_t nbTask){ m_nbTask = (int)nbTask; m_nCurrentTask = -1; }
	void AddTask(int nbTasks=1){ m_nbTask += nbTasks; }
	void AddTask(size_t nbTasks){ m_nbTask += (int)nbTasks; }
	void SkipTask(int nbTasks = 1)
	{ 
		m_nCurrentTask += nbTasks;
		if( m_nCurrentTask>=m_nbTask )
			m_nbTask = m_nCurrentTask+1;
	}

	int GetCurrentTaskNo()const{return m_nCurrentTask;}
	const std::string& GetTaskDescription()const{return m_description;}
	void SetCurrentDescription(const std::string& description){ SetCurrentDescription(description.c_str()); }
	void SetCurrentDescription(const char* description){ m_description = description; StepIt(0); }
	double GetCurrentStepPercent()const;
	
	
    void AddMessage(const std::string& message, int level=-1);
	void AddMessage(const char* message, int level=-1);
    void AddMessage(const ERMsg& message, int level=-1);
	void DeleteMessages(bool bAccumulation=false);
	std::string GetDlgMessageText()const{return m_messageDlgAccumulator;}
	std::string GetCurrentTaskMessageText()const{return m_messageAccumulator;}
	const std::string& GetMessages()const{return m_messages;}
	

	double GetCurrentStepPos()const{return m_stepPos;}
	ERMsg SetCurrentStepPos(double stepPos){ m_stepPos = stepPos; return StepIt(0);  }
	ERMsg SetCurrentStepPos(size_t stepPos){ m_stepPos = (double)stepPos; return StepIt(0); }

	double GetNbStep()const{return m_nbStep;}
	void SetNbStep(size_t nbStep, double stepBy = 1){ SetNbStep((double)nbStep, stepBy); }
	void SetNbStep(double nbStep, double stepBy);
	
    ERMsg StepIt(double stepBy = -1);
	
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

	void PushLevel(){m_currentLevel++;}
	void PopLevel(){m_currentLevel--;}

	int GetCurrentLevel()const{return m_currentLevel;}
	void SetCurrentLevel(int in){m_currentLevel=in;}

	void SetWnd(HWND* phWnd){ m_phWnd=phWnd;}

	
protected:


    std::string m_messages;
	std::string m_messageAccumulator;
	std::string m_messageDlgAccumulator;
    std::string m_description;
    double m_nbStep;
	double m_stepBy;
    double m_stepPos;
	double m_oldPos;

	
	manual_reset_event m_cancelEvent;
	manual_reset_event m_pauseEvent;

    int m_nbTask;
    int m_nCurrentTask;
	int m_currentLevel;
	bool m_bPumpMessage;
	bool m_bCancelled;

	std::string m_userCancelMsg;
	HWND* m_phWnd;//progressDlg
};



}

//shortcut to default callback
static WBSF::CCallback& DEFAULT_CALLBACK = WBSF::CCallback::DEFAULT_CALLBACK;

#endif