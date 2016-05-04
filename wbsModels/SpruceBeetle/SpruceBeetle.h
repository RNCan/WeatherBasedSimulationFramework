#pragma once

class CWeatherYear;

//***************************************************************************
//CSpruceBeetle

class CSpruceBeetle
{
public:

	enum TAspect {NORTH=0x01,SOUTH=0x02,HIGH=0x04,MEDIUM=0x08,LOW=0x10};

    CSpruceBeetle();
    ~CSpruceBeetle();

	bool Compute(const CWeatherYear& weatherYear1, const CWeatherYear& weatherYear2);

	static const double SLOPE;
	static const double INTERCEPT;
	static const double NORTH_EFFECT;
	static const double HIGT_EFFECT;
	static const double LOW_EFFECT;

	int m_pri15;
	int m_day15;
	int m_peak;
	int m_Hr17;
	double m_propTree;


private:

	

	static int GetPri15(const CWeatherYear& weatherYear);
	static int GetFlightPeak(const CWeatherYear& weatherYear, int day15);
	static int GetHr17(const CWeatherYear& weatherYear, int peakShifted);
	static double GetUnivoltineBroodProportion(int Hr17, int mod);
};
