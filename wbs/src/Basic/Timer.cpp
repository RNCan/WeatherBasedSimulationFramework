// CTimer - a simple Win32 performance counter wrapper
// by Dean Wyant dwyant@mindspring.com

#include "stdafx.h"
#include "Timer.h"

// Declare and initialize static member vars that get set only once and never change
__int64 CTimer::m_Freq = 0; 
__int64 CTimer::m_Adjust = 0; 

// All functions defined inline for speed. After all, the performance counter is 
// supposed to be able to time very short events fairly accurately.



BOOL CTimer::IsSupported()
{ // Returns FALSE if performance counter not supported.
  // Call after constructing at least one CTimer
  return (m_Freq > 1);
}

const double CTimer::Resolution()const
{ // Returns timer resolution in seconds
  return 1.0/(double)m_Freq; 
}

const double CTimer::Resolutionms()const
{ // Returns timer resolution in milliseconds
  return 1000.0/(double)m_Freq; 
}

const double CTimer::Resolutionus()const
{ // Returns timer resolution in microseconds
  return 1000000.0/(double)m_Freq; 
}



