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

#include <boost\thread.hpp>


class manual_reset_event
{
public:
	manual_reset_event(bool signaled = false)
		: signaled_(signaled)
	{
	}

	void set()
	{
		{
			boost::lock_guard<boost::mutex> lock(m_);
			signaled_ = true;
		}

		// Notify all because until the event is manually
		// reset, all waiters should be able to see event signalling
		cv_.notify_all();
	}

	void unset()
	{
		boost::lock_guard<boost::mutex> lock(m_);
		signaled_ = false;
	}


	void wait()
	{
		//boost::lock_guard<boost::mutex> lock(m_);
		boost::unique_lock<boost::mutex> lock(m_);
		while (!signaled_)
		{
			cv_.wait(lock);
		}
	}

	bool isSet()const{ return signaled_; }
private:

	boost::mutex m_;
	boost::condition_variable cv_;
	bool signaled_;
};
