#pragma once

class CDuration
{
protected:
	LARGE_INTEGER m_liStart;
	LARGE_INTEGER m_liStop;

	LONGLONG m_llFrequency;
	LONGLONG m_llCorrection;

public:
	CDuration(void);

	void Start(void);
	void Stop(void);
	double GetDuration(void) const;
	double GetDurationMs(void) const{return GetDuration()/1000;}
	double Elapsed(void) const{return GetDuration()/1000000;}
};

inline CDuration::CDuration(void)
{
	LARGE_INTEGER liFrequency;

	QueryPerformanceFrequency(&liFrequency);
	m_llFrequency = liFrequency.QuadPart;

	// Calibration
	Start();
	Stop();

	m_llCorrection = m_liStop.QuadPart-m_liStart.QuadPart;
}

inline void CDuration::Start(void)
{
	// Ensure we will not be interrupted by any other thread for a while
	Sleep(0);
	QueryPerformanceCounter(&m_liStart);
}

inline void CDuration::Stop(void)
{
	QueryPerformanceCounter(&m_liStop);
}

inline double CDuration::GetDuration(void) const
{
	return (double)(m_liStop.QuadPart-m_liStart.QuadPart-m_llCorrection)*1000000.0 / m_llFrequency;
}
