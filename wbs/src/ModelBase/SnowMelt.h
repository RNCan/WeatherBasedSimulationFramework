#pragma once

#include <vector>
#include "Basic/UtilTime.h"
#include "Basic/WeatherDefine.h"


namespace WBSF
{

	class CWeatherStation;

	//*****************************************************************************
	//CSnowMeltParam: simulation parameters states
	class CSnowMeltParam
	{
	public:
		CSnowMeltParam()
		{
			hs = 0; rs = 0; SWE = 0;
		}

		double hs;//Snow depth (cm)
		double rs;//Snow density (kg/m³ or g/cm³)
		double SWE;//snow water equivalent (mm)
	};

	//*****************************************************************************
	//CSnowMeltResult: daily output result
	class CSnowMeltResult
	{
	public:

		CSnowMeltResult()
		{
			m_newSWE = m_hs = m_rs = m_SWE = WEATHER::MISSING;
		}

		CTRef m_date;
		double m_newSWE;	//new snow (mm of water)
		double m_hs;	//Snow depth (cm)
		double m_rs;	//Snow density (kg/m³ or g/cm³)
		double m_SWE;	//snow water equivalent (mm)


	};

	//define an new template to compute SnowMelt
	template <class T>
	class CSnowMeltResultVectorBase : public CTReferencedVector < T >
	{
	public:

		CTRef GetSnowMeltDate(short year)const
		{
			static const long NB_DAY_MIN = 7;

			CTRef firstDay;
			short nbDay = 0;

			CTRef begin(year, JULY, 14);
			CTRef end(year, JANUARY, FIRST_DAY);

			for (CTRef d = begin; d >= end&&!firstDay.IsInit(); d--)
			{
				if (at(d).m_hs > 2)//more than 2 cm
				{
					nbDay++;
					if (nbDay > NB_DAY_MIN && !firstDay.IsInit())
					{
						firstDay = d + NB_DAY_MIN;
					}
				}
				else nbDay = 0;
			}

			return firstDay;
		}
	};

	//define array of daily result
	typedef CSnowMeltResultVectorBase<CSnowMeltResult> CSnowMeltResultVector;

	//*****************************************************************************
	//CSnowMelt: main snow computation class
	class CSnowMelt
	{
	public:

		enum TLand{ OPEN, FORESTED, NB_LAND };
		enum TSnowProp { BROWN, KIENZLE, NB_SNOWPROP };

		CSnowMelt(void);
		~CSnowMelt(void);

		void Reset();

		void Compute(const CWeatherStation& weather);
		void Compute(const CWeatherStation& weather, CSnowMeltParam& smp, bool bUpdateBegin);

		//void Compute(const CWeather& weather);
		//void Compute(const CWeather& weather, CSnowMeltParam& smp, bool bUpdateBegin);

		const CSnowMeltResultVector& GetResult()const{ return m_result; }

		void SetLandType(size_t in){ m_landType = in; }
		void SetLon(double lon);

		void SetSnowPropModel(size_t in){ m_snowPropModel = in; }
		void SetTmelt(double in){ m_Tmelt = in; }
		void SetC1(double in){ m_C1 = in; }
		void SetC2(double in){ m_C2 = in; }
		void SetTt(double in){ m_Tt = in; }
		void SetTr(double in){ m_Tr = in; }

	protected:

		double GetSnowProp(double T, size_t m)const;
		double Compaction(double T, double SWE, double hs, double rs, short timeStep)const;
		double GetMelt(double T, double rs, short timeStep)const;
		double GetRainMelt(double T, double rain)const;
		void ComputeTtTmelt();


		CSnowMeltResultVector m_result;

		size_t m_landType;
		size_t m_snowPropModel;

		double m_lon;
		double m_alphaTt;
		double m_betaTt;
		double m_alphaTmelt;
		double m_betaTmelt;

		double m_Tmelt; //Temperature at hich melt starts. Brown et al (2003)
		double m_C1;
		double m_C2;
		double m_Tt;
		double m_Tr;
	};

}