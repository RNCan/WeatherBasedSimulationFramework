#pragma once

namespace WBSF
{

	class CWeatherYear;

	//***************************************************************************
	//CSpruceBeetle

	class CSpruceBeetle
	{
	public:

		enum TAspect { NORTH = 0x01, SOUTH = 0x02, HIGH = 0x04, MEDIUM = 0x08, LOW = 0x10 };

		CSpruceBeetle();
		~CSpruceBeetle();

		bool Compute(const CWeatherYear& weatherYear1, const CWeatherYear& weatherYear2);

		static const double SLOPE;
		static const double INTERCEPT;
		static const double NORTH_EFFECT;
		static const double HIGT_EFFECT;
		static const double LOW_EFFECT;

		size_t m_pri15;
		size_t m_day15;
		size_t m_peak;
		size_t m_Hr17;
		double m_propTree;


	private:



		static size_t GetPri15(const CWeatherYear& weatherYear);
		static size_t GetFlightPeak(const CWeatherYear& weatherYear, size_t day15);
		static size_t GetHr17(const CWeatherYear& weatherYear, size_t peakShifted);
		static double GetUnivoltineBroodProportion(size_t Hr17, int mod);
	};
}