﻿//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <crtdbg.h>
#include <vector>
#include <array>
#include <memory>

#include "basic/ERMsg.h"
#include "Basic/WeatherDefine.h"
#include "Basic/Location.h"
#include "Basic/WeatherDataSection.h"
#include "Basic/Statistic.h"

struct IAgent;


namespace WBSF
{
	//*************************************************************
	//CHourlyData
	class CWeatherDay;
	class CWeatherMonth;
	class CWeatherYear;
	class CWeatherYears;
	class CWeatherAccumulator;
	class CWeatherCorrections;
	class COverheat;
	class CDailyWaveVector;
	struct CDataInterface;


	double G(double Rn, double LAI = -9999);

	//**************************************************************************************************************
	//CWeatherStatistic 

	template <int T>
	class CWeatherStatisticBase : public std::array<CStatistic, T>
	{
	public:

		CWeatherStatisticBase()
		{
			m_bInit = false;
		}

		void clear()
		{
			std::for_each(std::array<CStatistic, T>::begin(), std::array<CStatistic, T>::end(), [this](CStatistic& ob) { ob.clear(); });
			m_period.clear();
			m_bInit = false;
		}

		CWeatherStatisticBase& operator=(const CWeatherStatisticBase& in)
		{
			if (&in != this)
			{
				std::array<CStatistic, T>::operator=(in);
				m_bInit = in.m_bInit;
				m_period = in.m_period;
			}

			return *this;
		}

		CWVariables GetVariables()const
		{
			CWVariables variables;
			for (size_t v = 0; v < std::array<CStatistic, T>::size(); v++)
				variables[v] = std::array<CStatistic, T>::at(v).IsInit();

			return variables;
		}

		CWVariablesCounter GetVariablesCount(bool bOneCountPerDay = false)const
		{
			CWVariablesCounter variables;
			for (size_t v = 0; v < std::array<CStatistic, T>::size(); v++)
			{
				if (std::array<CStatistic, T>::at(v).IsInit())
				{
					variables[v].first += size_t(bOneCountPerDay ? 1 : std::array<CStatistic, T>::at(v)[NB_VALUE]);
					variables[v].second += m_period;// CTPeriod(m_TRef, m_TRef);//m_period can not be init yet
				}
			}
			return variables;
		}

		bool HaveData()const
		{
			bool bHaveData = false;
			for (size_t v = 0; v < std::array<CStatistic, T>::size() && !bHaveData; v++)
				bHaveData = std::array<CStatistic, T>::at(v).IsInit();

			return bHaveData;
		}

		bool m_bInit;
		CTPeriod m_period;
	};

	typedef CWeatherStatisticBase<HOURLY_DATA::NB_VAR_H> CWeatherStatistic;
	typedef CWeatherStatisticBase<HOURLY_DATA::NB_VAR_ALL> CWeatherExStatistic;

	//**************************************************************************************************************
	//CWeatherAccumulator

	class CWeatherAccumulator
	{
	public:

		int m_deltaHourMin;
		std::array<int, HOURLY_DATA::NB_VAR_H> m_minimumHours;
		std::array<int, HOURLY_DATA::NB_VAR_H> m_minimumDays;

		bool TRefIsChanging(CTRef Tref, int shift = 0)const { return GTRef().IsInit() && (Tref - shift).Transform(m_TM) != (m_lastTRef - shift).Transform(m_TM); }
		CTRef GetTRef()const { return GTRef().Transform(m_TM); }
		CTM GetTM()const { return m_TM; }


		CWeatherAccumulator(const CTM& TM);

		void ResetStat()
		{
			m_bStatComputed = false;
			m_variables.clear();
		}

		void ResetMidnight()
		{

			m_midnightVariables.clear();
			for (size_t h = 0; h < m_midnightTRefMatrix.size(); h++)
				m_midnightTRefMatrix[h].fill(0);

			//Reset next day stat
			//m_midnightVariables2 = m_midnightVariablesTmp;
			//m_midnightTRefMatrix2 = m_midnightTRefMatrixTmp;

			//m_midnightVariablesTmp.clear();
			//for (size_t h = 0; h<m_midnightTRefMatrixTmp.size(); h++)
			//	m_midnightTRefMatrixTmp[h].fill(0);
		}

		void ResetNoon()
		{
			//fill current day stat with from the stat last day
			m_noonVariables = m_noonVariablesTmp;
			m_noonTRefMatrix = m_noonTRefMatrixTmp;

			//Reset next day stat
			m_noonVariablesTmp.clear();
			for (size_t h = 0; h < m_noonTRefMatrixTmp.size(); h++)
				m_noonTRefMatrixTmp[h].fill(0);
		}

		void Reset06()
		{
			//fill current day stat with from the stat last day
			m_06Variables = m_06VariablesTmp;
			m_06TRefMatrix = m_06TRefMatrixTmp;

			//Reset next day stat
			m_06VariablesTmp.clear();
			for (size_t h = 0; h < m_06TRefMatrixTmp.size(); h++)
				m_06TRefMatrixTmp[h].fill(0);
		}

		void Reset18()
		{
			//fill current day stat with from the stat last day
			m_18Variables = m_18VariablesTmp;
			m_18TRefMatrix = m_18TRefMatrixTmp;

			//Reset next day stat
			m_18VariablesTmp.clear();
			for (size_t h = 0; h < m_18TRefMatrixTmp.size(); h++)
				m_18TRefMatrixTmp[h].fill(0);
		}

		void Reset22()
		{
			//fill current day stat with from the stat last day
			m_22Variables = m_22VariablesTmp;
			m_22TRefMatrix = m_22TRefMatrixTmp;

			//Reset next day stat
			m_22VariablesTmp.clear();
			for (size_t h = 0; h < m_22TRefMatrixTmp.size(); h++)
				m_22TRefMatrixTmp[h].fill(0);
		}


		const CStatistic& GetStat(size_t v)const;
		const CStatistic& operator[](size_t v)const { return GetStat(v); }

		ERMsg Add(const StringVector& data, const CWeatherFormat& format);
		void Add(CTRef Tref, size_t v, double value) { if (!WEATHER::IsMissing(value)) Add(Tref, v, CStatistic(value)); }
		void Add(CTRef Tref, size_t v, const CStatistic& value);

	protected:

		CTRef GTRef()const { return m_lastTRef; }

		CTM m_TM;
		CTRef m_lastTRef;


		static const CStatistic EMPTY_STAT;

		const CStatistic& GetStat(size_t v, int sourcePeriod)const;
		void ComputeStatistic()const;

		CWeatherStatistic m_midnightVariables;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_midnightTRefMatrix;

		//CWeatherStatistic m_midnightVariablesTmp;
		//std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_midnightTRefMatrixTmp;


		CWeatherStatistic m_noonVariables;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_noonTRefMatrix;

		CWeatherStatistic m_noonVariablesTmp;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_noonTRefMatrixTmp;


		CWeatherStatistic m_06Variables;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_06TRefMatrix;

		CWeatherStatistic m_06VariablesTmp;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_06TRefMatrixTmp;



		CWeatherStatistic m_18Variables;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_18TRefMatrix;

		CWeatherStatistic m_18VariablesTmp;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_18TRefMatrixTmp;

		CWeatherStatistic m_22Variables;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_22TRefMatrix;

		CWeatherStatistic m_22VariablesTmp;
		std::array<std::array<size_t, HOURLY_DATA::NB_VAR_H>, 24> m_22TRefMatrixTmp;



		bool m_bStatComputed;
		CWeatherStatistic m_variables;

	};

	//**************************************************************************************************************
	//COverheat

	class COverheat
	{
	public:

		COverheat(double overheat = 0)
		{
			m_overheat = overheat;
		}

		virtual double GetOverheat(const CWeatherDay& weather, size_t h, size_t hourTmax = 15)const;
		virtual double GetT(const CWeatherDay& weather, size_t h, size_t hourTmax = 15)const;

		virtual double GetTmin(const CWeatherDay& weather)const;
		virtual double GetTmax(const CWeatherDay& weather)const;

	protected:

		double m_overheat;
	};

	//**************************************************************************************************************
	//CDailyWaveVector

	class CDailyWaveVector : public CTReferencedVector<float> //: public CTReferencedVector<CHourlyData>
	{
	public:
	};


	class CWeatherStation;
	//**************************************************************************************************************
	//CDataInterface


	struct CDataInterface
	{
	public:

		void SetData(const CWeatherAccumulator& weatherStat);
		const CStatistic& operator[](HOURLY_DATA::TVarH v)const { return GetData(v); }
		CStatistic& operator[](HOURLY_DATA::TVarH v) { return GetData(v); }
		CStatistic operator[](HOURLY_DATA::TVarEx v)const { return GetVarEx(v); }
		CStatistic operator[](HOURLY_DATA::TVarEx v) { return GetVarEx(v); }

		virtual inline const CDataInterface& Get(CTRef ref)const = 0;
		virtual inline CDataInterface& Get(CTRef ref) = 0;
		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const = 0;
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v) = 0;
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat) = 0;
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const = 0;
		virtual CTRef GetTRef()const = 0;
		virtual CWVariables GetVariables()const = 0;
		virtual CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const = 0;
		virtual CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const = 0;
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const = 0;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true) = 0;
		virtual inline const CDataInterface* GetParent()const = 0;
		virtual inline CDataInterface* GetParent() = 0;
		virtual inline CWeatherYears& GetWeatherYears() { return GetParent()->GetWeatherYears(); }
		virtual inline const CWeatherYears& GetWeatherYears()const { return GetParent()->GetWeatherYears(); }
		virtual inline CWeatherStation* GetWeatherStation() { return GetParent()->GetWeatherStation(); }
		virtual inline const CWeatherStation* GetWeatherStation()const { return GetParent()->GetWeatherStation(); }
		virtual inline bool IsHourly()const = 0;
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const = 0;
		virtual CStatistic GetTimeLength()const = 0;					//return time length [s]
		virtual double GetNetRadiation(double& Fcd)const = 0;
		virtual void Reset() = 0;

		inline bool HavePrevious()const;
		inline bool HaveNext()const;

		template<class T>
		inline const T& GetPreviousI()const
		{
			return static_cast<const T&>(HavePrevious() ? Get(GetTRef() - 1) : *this);
		}

		template<class T>
		inline const T& GetNextI()const
		{
			return static_cast<const T&>(HaveNext() ? Get(GetTRef() + 1) : *this);
		}

	};



	//**************************************************************************************************************
	//CHourlyData

	typedef std::array<float, HOURLY_DATA::NB_VAR_H> CWeatherVariables;
	class CHourlyData : public CDataInterface, public CWeatherVariables
	{
	public:
		CHourlyData()
		{
			fill(WEATHER::MISSING);
			m_pParent = NULL;
		}

		CHourlyData(const CHourlyData& in) { operator=(in); }
		CHourlyData& operator=(const CHourlyData& in);

		using CWeatherVariables::operator[];
		const float& operator[](HOURLY_DATA::TVarH v)const { return CWeatherVariables::at(v); }
		float& operator[](HOURLY_DATA::TVarH v) { return CWeatherVariables::at(v); }
		float operator[](HOURLY_DATA::TVarEx v)const { CStatistic stat = GetVarEx(v); return stat.IsInit() ? float(stat[MEAN]) : WEATHER::MISSING; }
		float operator[](HOURLY_DATA::TVarEx v) { CStatistic stat = GetVarEx(v); return stat.IsInit() ? float(stat[MEAN]) : WEATHER::MISSING; }

		float at(size_t v)const { return (v < size()) ? CWeatherVariables::at(v) : operator[](HOURLY_DATA::TVarEx(v)); }
		float at(size_t v) { return (v < size()) ? CWeatherVariables::at(v) : operator[](HOURLY_DATA::TVarEx(v)); }

		void Initialize(CTRef TRef, CWeatherDay* pParent = NULL)
		{
			m_TRef = TRef;
			if (pParent)
				m_pParent = pParent;
		}

		virtual CWVariables GetVariables()const
		{
			CWVariables variables;
			for (size_t v = 0; v < size(); v++)
				variables[v] = !WEATHER::IsMissing(at(v));

			return variables;
		}

		virtual CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter variables;
			for (size_t v = 0; v < size(); v++)
			{
				if (!WEATHER::IsMissing(at(v)))
				{
					variables[v].first += 1;
					variables[v].second += m_TRef;
				}
			}

			return variables;
		}

		bool HaveData()const
		{
			bool bHaveData = false;
			for (size_t i = 0; i < size() && !bHaveData; i++)
				bHaveData = !WEATHER::IsMissing(at(i));

			return bHaveData;
		}

		virtual inline const CDataInterface& Get(CTRef ref)const;
		virtual inline CDataInterface& Get(CTRef ref);
		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const;
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v);
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat);
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const;
		virtual inline bool IsYearInit(int year)const;
		virtual CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const;
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const override;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true)override;
		virtual CTRef GetTRef()const { return m_TRef; }
		virtual bool IsHourly()const { return true; }
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const;
		virtual CStatistic GetTimeLength()const { return CStatistic(60 * 60); }
		//void ComputeTRange();


		double K()const { return (!WEATHER::IsMissing(at(HOURLY_DATA::H_TAIR))) ? at(HOURLY_DATA::H_TAIR) + 273.15 : WEATHER::MISSING; }
		double GetLatentHeatOfVaporization()const;
		double GetExtraterrestrialRadiation()const;
		virtual double GetNetRadiation(double& Fcd)const;
		virtual void Reset();


		virtual inline const CDataInterface* GetParent()const;
		virtual inline CDataInterface* GetParent();

		inline const CLocation& GetLocation()const;
		//inline const CHourlyData& GetPrevious()const;
		//inline const CHourlyData& GetNext()const;

		inline const CHourlyData& GetPrevious()const { return GetPreviousI<CHourlyData>(); }
		inline const CHourlyData& GetNext()const { return GetNextI<CHourlyData>(); }

	protected:

		CTRef m_TRef;
		CWeatherDay* m_pParent;

	};

	//**************************************************************************************************************
	//CWeatherDay

	typedef std::array<CHourlyData, 24> C24Hours;
	typedef std::unique_ptr<C24Hours> C24HoursPtr;
	class CWeatherDay : public CDataInterface
	{
	public:


		CWeatherDay()
		{
			m_pParent = NULL;
		}

		CWeatherDay(const CWeatherDay& in) { m_pParent = NULL; operator=(in); }
		CWeatherDay& operator=(const CWeatherDay& in);
		bool operator==(const CWeatherDay& in)const;
		bool operator!=(const CWeatherDay& in)const { return !operator==(in); }

		void Initialize(CTRef TRef, CWeatherMonth* pParent = NULL)
		{
			if (pParent)
				m_pParent = pParent;

			m_TRef = TRef;
		}

		virtual CWVariables GetVariables()const
		{
			CWVariables variables;
			if (IsHourly())
			{
				for (size_t h = 0; h < size(); h++)
					variables |= at(h).GetVariables();
			}
			else
			{
				variables = m_dailyStat.GetVariables();
			}

			return variables;
		}

		virtual CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter variables;
			if (IsHourly())
			{
				CWVariablesCounter tmp;
				for (size_t h = 0; h < size(); h++)
					tmp += at(h).GetVariablesCount();

				if (bDailyOnly)
				{
					//we do this here to avoid create statistic in this function
					for (size_t v = 0; v < tmp.size(); v++)
					{
						if (tmp[v].first > 0)
						{
							variables[v].first += 1;
							variables[v].second += CTPeriod(m_TRef, m_TRef);//m_period can not be init yet
						}
					}
				}
				else
				{
					variables = tmp;
				}

			}
			else
			{
				//update period
				//if (!m_dailyStat.m_bInit)
				//{
					//CWeatherDay& me = const_cast<CWeatherDay&>(*this);
					//me.m_dailyStat.m_period.clear();
					//
					//if (HaveData())
					//	me.m_dailyStat.m_period = CTPeriod(m_TRef, m_TRef);

					//me.m_dailyStat.m_bInit = true;
				//}

				variables = m_dailyStat.GetVariablesCount(true);
			}

			return variables;
		}

		bool HaveData()const
		{
			bool bHaveData = false;

			if (IsHourly())
			{
				for (size_t i = 0; i < size() && !bHaveData; i++)
					bHaveData = at(i).HaveData();
			}
			else
			{
				bHaveData = m_dailyStat.HaveData();
			}

			return bHaveData;
		}

		size_t size()const { return 24; }
		typedef C24Hours::iterator iterator;
		typedef C24Hours::const_iterator const_iterator;

		C24Hours::iterator begin() { ManageHourlyData(); assert(m_pHourlyData.get()); m_dailyStat.m_bInit = false; return m_pHourlyData->begin(); };
		C24Hours::iterator end() { ManageHourlyData();  assert(m_pHourlyData.get()); return m_pHourlyData->end(); };
		C24Hours::const_iterator begin()const { ManageHourlyData();  assert(m_pHourlyData.get()); return m_pHourlyData->begin(); };
		C24Hours::const_iterator end()const { ManageHourlyData();  assert(m_pHourlyData.get()); return m_pHourlyData->end(); };

		CHourlyData& at(size_t i) { ManageHourlyData();  assert(m_pHourlyData.get());  m_dailyStat.m_bInit = false;  return(*m_pHourlyData)[i]; }
		const CHourlyData& at(size_t i)const { ManageHourlyData();  assert(m_pHourlyData.get()); return (*m_pHourlyData)[i]; }
		CHourlyData& operator[](size_t i) { return at(i); }
		const CHourlyData& operator[](size_t i)const { return at(i); }
		CStatistic& operator[](HOURLY_DATA::TVarH v) { if (HourlyDataExist())CompileDailyStat(); return m_dailyStat[v]; }
		const CStatistic& operator[](HOURLY_DATA::TVarH v)const { if (HourlyDataExist())CompileDailyStat(); return m_dailyStat[v]; }

		CStatistic operator[](HOURLY_DATA::TVarEx v)const { if (HourlyDataExist())CompileDailyStat(); return GetVarEx(v); }
		CStatistic operator[](HOURLY_DATA::TVarEx v) { if (HourlyDataExist())CompileDailyStat(); return GetVarEx(v); }



		//get daylight mean temperature approximation (deg C)
		double GetTdaylight()const;
		//double K()const{ if (HourlyDataExist())CompileDailyStat(); return (m_dailyStat[HOURLY_DATA::H_TNTX].IsInit()) ? m_dailyStat[HOURLY_DATA::H_TNTX][MEAN] + 273.15 : WEATHER::MISSING; }
		double GetDayLength()const;

		CTM GetTM()const { return CTM(IsHourly() ? CTM::HOURLY : CTM::DAILY); }
		CTPeriod GetEntireTPeriod()const { return GetEntireTPeriod(GetTM()); }
		CTPeriod GetEntireTPeriod(CTM TM)const { return CTPeriod(CTRef(m_TRef.GetYear(), m_TRef.GetMonth(), m_TRef.GetDay(), FIRST_HOUR, TM), CTRef(m_TRef.GetYear(), m_TRef.GetMonth(), m_TRef.GetDay(), LAST_HOUR, TM)); }

		const CStatistic& GetStat(HOURLY_DATA::TVarH v)const { CompileDailyStat(); return m_dailyStat[v]; }
		void CompileDailyStat(bool bFoceCompile = false)const;
		//void ComputeTRange();

		virtual inline const CDataInterface& Get(CTRef ref)const;
		virtual inline CDataInterface& Get(CTRef ref);
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat);
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const;
		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const { if (HourlyDataExist())CompileDailyStat(); return m_dailyStat[v]; }
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v) { if (HourlyDataExist())CompileDailyStat(); return m_dailyStat[v]; }
		virtual inline bool IsYearInit(int year)const;
		virtual CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const;
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const override;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true)override;
		virtual CTRef GetTRef()const;
		virtual CDataInterface& operator[](CTRef ref) { return Get(ref); }
		virtual const CDataInterface& operator[](CTRef ref)const { return Get(ref); }
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const;
		virtual CStatistic GetTimeLength()const { return CStatistic(24 * 60 * 60); }
		virtual double GetNetRadiation(double& Fcd)const;
		virtual void Reset();

		double GetAllenT(double h, size_t hourTmin = 3, size_t hourTmax = 15, double PolarDayLength = 3)const;
		double GetDoubleSine(double h, double PolarDayLength = 3)const;
		double GetSineExponential(double h, size_t method = SE_SAVAGE, double PolarDayLength = 3)const;
		double GetSinePower(double h, double PolarDayLength = 3)const;
		double GetErbs(double h, double PolarDayLength = 3)const;
		//double GetPolarInterpol(double h)const;

		void ManageHourlyData()const
		{
			CWeatherDay* pMe = const_cast<CWeatherDay*>(this);
			if (pMe->IsHourly())
			{
				if (!pMe->m_pHourlyData.get())
				{

					pMe->m_pHourlyData.reset(new C24Hours);
					CTRef TRef = m_TRef;
					TRef.Transform(CTM(CTM::HOURLY));
					for (C24Hours::iterator it = pMe->m_pHourlyData->begin(); it != pMe->m_pHourlyData->end(); it++, TRef++)
						it->Initialize(TRef, pMe);
				}
			}
			else
			{
				pMe->m_pHourlyData.reset();//reset hourly memory
			}
		}

		virtual inline bool IsHourly()const;
		virtual inline const CDataInterface* GetParent()const;
		virtual inline CDataInterface* GetParent();
		inline const CLocation& GetLocation()const;
		//inline const CWeatherDay& GetPrevious()const;
		//inline const CWeatherDay& GetNext()const;
		inline const CWeatherDay& GetPrevious()const { return GetPreviousI<CWeatherDay>(); }
		inline const CWeatherDay& GetNext()const { return GetNextI<CWeatherDay>(); }


		void ComputeHourlyTair(size_t method);
		void ComputeHourlyPrcp();
		void ComputeHourlyTdew();
		void ComputeHourlyRelH();
		void ComputeHourlyWndS();
		void ComputeHourlyWnd2();
		void ComputeHourlyWndD();
		void ComputeHourlySRad();
		void ComputeHourlyPres();
		void ComputeHourlyVariables(CWVariables variables, std::string options);

		static double GetSn(size_t n, size_t h);
		static double GetS(size_t h);


	protected:

		virtual bool HourlyDataExist()const { return m_pHourlyData.get() != NULL; }

		C24HoursPtr m_pHourlyData;
		CWeatherStatistic m_dailyStat;

		CWeatherMonth* m_pParent;
		CTRef m_TRef;

	};

	typedef CWeatherDay CDay;

	//**************************************************************************************************************
	//CWeatherMonth

	typedef std::array<CWeatherDay, 31> C31Days;
	class CWeatherMonth : public CDataInterface, public C31Days
	{
	public:

		CWeatherMonth()
		{
			m_pParent = NULL;
		}

		CWeatherMonth(const CWeatherMonth& in) { m_pParent = NULL; operator=(in); }
		CWeatherMonth& operator=(const CWeatherMonth& in);
		bool operator==(const CWeatherMonth& in)const;
		bool operator!=(const CWeatherMonth& in)const { return !operator==(in); }


		void Initialize(CTRef TRef, CWeatherYear* pParent = NULL)
		{
			if (pParent)
				m_pParent = pParent;

			TRef.m_type = CTRef::DAILY;
			for (size_t d = 0; d < TRef.GetNbDayPerMonth(); d++)
			{
				TRef.m_day = d;
				at(d).Initialize(TRef, this);
			}
		}

		size_t size()const { return GetTRef().GetNbDayPerMonth(); }
		CWeatherMonth::iterator end() { return C31Days::begin() + size(); }
		CWeatherMonth::const_iterator end()const { return C31Days::begin() + size(); }

		virtual CWVariables GetVariables()const
		{
			CWVariables variables;
			for (size_t d = 0; d < size(); d++)
				variables |= at(d).GetVariables();

			return variables;
		}

		virtual CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter variables;
			for (size_t d = 0; d < size(); d++)
				variables += at(d).GetVariablesCount(bDailyOnly);

			return variables;
		}

		bool HaveData()const
		{
			bool bHaveData = false;
			for (size_t i = 0; i < size() && !bHaveData; i++)
				bHaveData = at(i).HaveData();

			return bHaveData;
		}


		using C31Days::operator[];

		CStatistic& GetStat(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod()) { CompileStat(p); return m_stat[v]; }
		const CStatistic& GetStat(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& GetStat(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& operator[](const HOURLY_DATA::TVarH& v)const { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarEx& v)const { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarH& v) { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarEx& v) { return GetStat(v); }
		const CStatistic& operator()(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }
		const CStatistic& operator()(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }

		size_t GetNbDays()const { return GetTRef().GetNbDayPerMonth(); }
		CTM GetTM()const { return CTM(IsHourly() ? CTM::HOURLY : CTM::DAILY); }
		CTPeriod GetEntireTPeriod()const { return GetEntireTPeriod(GetTM()); }
		CTPeriod GetEntireTPeriod(CTM TM)const { return CTPeriod(front().GetEntireTPeriod(TM).Begin(), at(GetNbDays() - 1).GetEntireTPeriod(TM).End()); }


		virtual inline const CDataInterface& Get(CTRef ref)const;
		virtual inline CDataInterface& Get(CTRef ref);
		virtual inline bool IsYearInit(int year)const;
		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const { return GetStat(v); }
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v) { return GetStat(v); }
		virtual CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const;
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const override;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true)override;
		virtual CTRef GetTRef()const;
		virtual CDataInterface& operator[](const CTRef& ref) { return Get(ref); }
		virtual const CDataInterface& operator[](const CTRef& ref)const { return Get(ref); }
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat) { m_stat[v] = stat; }
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const { stat = m_stat[v]; return stat.IsInit(); }
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const;
		virtual CStatistic GetTimeLength()const { return CStatistic((double)GetNbDays() * 24 * 60 * 60); }
		virtual double GetNetRadiation(double& Fcd)const;
		virtual void Reset();

		void ResetStat();

		void ManageHourlyData()const
		{
			for (const_iterator it = begin(); it != end(); it++)
				it->ManageHourlyData();
		}

		virtual inline bool IsHourly()const;
		virtual inline const CDataInterface* GetParent()const;
		virtual inline CDataInterface* GetParent();
		inline const CLocation& GetLocation()const;
		//inline const CWeatherMonth& GetPrevious()const;
		//inline const CWeatherMonth& GetNext()const;
		inline const CWeatherMonth& GetPrevious()const { return GetPreviousI<CWeatherMonth>(); }
		inline const CWeatherMonth& GetNext()const { return GetNextI<CWeatherMonth>(); }




	protected:

		CWeatherYear* m_pParent;

		void CompileStat(const CTPeriod& p = CTPeriod())const;
		CWeatherExStatistic m_stat;
	};

	typedef CWeatherMonth CMonth;

	//**************************************************************************************************************
	//CWeatherYear

	typedef std::array<CWeatherMonth, 12> C12Months;
	class CWeatherYear : public CDataInterface, public C12Months
	{
	public:

		CWeatherYear(CTRef TRef = CTRef(), CWeatherYears* pParent = NULL)
		{
			m_pParent = NULL;
			Initialize(TRef, pParent);
		}

		CWeatherYear(const CWeatherYear& in) { m_pParent = NULL; operator=(in); }


		CWeatherYear& operator=(const CWeatherYear& in);

		void Initialize(CTRef TRef, CWeatherYears* pParent = NULL)
		{
			if (pParent)
				m_pParent = pParent;

			TRef.m_type = CTRef::MONTHLY;
			for (size_t m = 0; m < 12; m++)
			{
				TRef.m_month = m;
				at(m).Initialize(TRef, this);
			}
		}

		virtual CWVariables GetVariables()const
		{
			CWVariables variables;
			for (size_t m = 0; m < size(); m++)
				variables |= at(m).GetVariables();

			return variables;
		}

		virtual CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter variables;
			for (size_t m = 0; m < size(); m++)
				variables += at(m).GetVariablesCount(bDailyOnly);

			return variables;
		}

		void ManageHourlyData()
		{
			for (iterator it = begin(); it != end(); it++)
				it->ManageHourlyData();
		}


		bool HaveData()const
		{
			bool bHaveData = false;
			for (size_t i = 0; i < size() && !bHaveData; i++)
				bHaveData = at(i).HaveData();

			return bHaveData;
		}

		bool IsComplete(CWVariables variables)const;

		size_t GetNbDays()const { return WBSF::GetNbDaysPerYear(GetTRef().GetYear()); }


		using C12Months::operator[];
		CStatistic& GetStat(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod()) { CompileStat(p); return m_stat[v]; }
		const CStatistic& GetStat(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& GetStat(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& operator[](const HOURLY_DATA::TVarH& v)const { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarEx& v)const { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarH& v) { return GetStat(v); }
		const CStatistic& operator[](const HOURLY_DATA::TVarEx& v) { return GetStat(v); }
		const CStatistic& operator()(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }
		const CStatistic& operator()(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }

		short GetNbMonth()const { return 12; }
		CTM GetTM()const { return CTM(IsHourly() ? CTM::HOURLY : CTM::DAILY); }
		CTPeriod GetEntireTPeriod()const { return GetEntireTPeriod(GetTM()); }
		CTPeriod GetEntireTPeriod(CTM TM)const { return CTPeriod(front().front().GetEntireTPeriod(TM).Begin(), back().back().GetEntireTPeriod(TM).End()); }

		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const { return GetStat(v); }
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v) { return GetStat(v); }
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat) { m_stat[v] = stat; }
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const { stat = m_stat[v]; return stat.IsInit(); }
		virtual inline bool IsYearInit(int year)const;
		virtual CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const;
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const override;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true)override;
		virtual CTRef GetTRef()const;
		virtual inline const CDataInterface& Get(CTRef ref)const;
		virtual inline CDataInterface& Get(CTRef ref);
		virtual CDataInterface& operator[](CTRef ref) { return Get(ref); }
		virtual const CDataInterface& operator[](CTRef ref)const { return Get(ref); }
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const;
		virtual CStatistic GetTimeLength()const { return CStatistic((double)GetNbDays() * 24 * 60 * 60); }
		virtual double GetNetRadiation(double& Fcd)const;
		virtual void Reset();

		void ResetStat();

		virtual inline bool IsHourly()const;
		virtual inline const CDataInterface* GetParent()const;
		virtual inline CDataInterface* GetParent();
		inline const CLocation& GetLocation()const;
		//inline const CWeatherYear& GetPrevious()const;
		//inline const CWeatherYear& GetNext()const;
		inline const CWeatherYear& GetPrevious()const { return GetPreviousI<CWeatherYear>(); }
		inline const CWeatherYear& GetNext()const { return GetNextI<CWeatherYear>(); }


		//void ComputeTRange();
		const CWeatherDay& GetDay(size_t Jday)const { CJDayRef TRef(GetTRef().GetYear(), Jday); return at(TRef.GetMonth()).at(TRef.GetDay()); }
		CWeatherDay& GetDay(size_t Jday) { CJDayRef TRef(GetTRef().GetYear(), Jday); return at(TRef.GetMonth()).at(TRef.GetDay()); }
		inline const CHourlyData& GetHour(CTRef ref)const;
		inline CHourlyData& GetHour(CTRef ref);
		inline const CWeatherDay& GetDay(CTRef ref)const;
		inline CWeatherDay& GetDay(CTRef ref);
		inline const CWeatherMonth& GetMonth(CTRef ref)const;
		inline CWeatherMonth& GetMonth(CTRef ref);




		ERMsg SaveData(const std::string& filePath, CTM TM = CTM(), char separator = ',')const;
		ERMsg SaveData(std::ostream& file, CTM TM, const CWeatherFormat& format, char separator)const;
		//ERMsg LoadData(const std::string& filePath, double nodata = -999.0, bool bResetContent = true);
		//ERMsg Parse(const std::string& str, double nodata);

	protected:

		void CompileStat(const CTPeriod& p = CTPeriod())const;
		CWeatherExStatistic m_stat;
		CWeatherYears* m_pParent;
	};

	inline const CHourlyData& CWeatherYear::GetHour(CTRef ref)const { const CWeatherYear& me = *this; return me[ref.GetMonth()][ref.GetDay()][ref.GetHour()]; }
	inline CHourlyData& CWeatherYear::GetHour(CTRef ref) { CWeatherYear& me = *this; return me[ref.GetMonth()][ref.GetDay()][ref.GetHour()]; }
	inline const CWeatherDay& CWeatherYear::GetDay(CTRef ref)const { const CWeatherYear& me = *this; return me[ref.GetMonth()][ref.GetDay()]; }
	inline CWeatherDay& CWeatherYear::GetDay(CTRef ref) { CWeatherYear& me = *this; return me[ref.GetMonth()][ref.GetDay()]; }
	inline const CWeatherMonth& CWeatherYear::GetMonth(CTRef ref)const { const CWeatherYear& me = *this; return me[ref.GetMonth()]; }
	inline CWeatherMonth& CWeatherYear::GetMonth(CTRef ref) { CWeatherYear& me = *this; return me[ref.GetMonth()]; }


	typedef CWeatherYear CYear;
	typedef std::shared_ptr<CWeatherYear> CWeatherYearPtr;


	//**************************************************************************************************************
	//CWeatherYear

	class CWeatherStation;

	typedef std::map<int, CWeatherYearPtr> CWeatherYearMap;
	class CWeatherYears : public CDataInterface, public CWeatherYearMap
	{

	public:

		static CTRef GetLastTref(const std::string& filepath);

		std::array<std::string, HOURLY_DATA::NB_ADD> m_addName;

		CWeatherYears(bool bIsHourly = false);
		CWeatherYears(const CWeatherYears& in);
		void clear();

		void Initialize(CWeatherStation* pParent = NULL)
		{
			clear();
			//		m_bModified = false;
			m_bCompilingHourly = false;
			m_pParent = pParent;
		}



		CWeatherYears& operator=(const CWeatherYears& in);
		CWeatherYears& append(const CWeatherYears& in);
		bool operator==(const CWeatherYears& in)const;
		bool operator!=(const CWeatherYears& in)const { return !operator==(in); }


		size_t size()const { return CWeatherYearMap::size(); }
		size_t GetNbYears()const { return CWeatherYearMap::size(); }

		CWeatherYear& operator[](const size_t& y) { return at(I2Year(y)); }
		const CWeatherYear& operator[](const size_t& y)const { return at(I2Year(y)); }

		CWeatherYear& operator[](int year) { assert(year <= 0 || (year >= 1500 && year <= 2100)); return CreateYear(year); }
		const CWeatherYear& operator[](int year)const { assert(year <= 0 || (year >= 1500 && year <= 2100)); return at(year); }//throw exception?

		CWeatherYear& at(int year) { return  *CWeatherYearMap::at(year); }
		const CWeatherYear& at(int year)const { return *CWeatherYearMap::at(year); }
		CWeatherYear& at(size_t y) { assert(y < size()); assert(CWeatherYearMap::at(I2Year(y)).get() != NULL); return *CWeatherYearMap::at(I2Year(y)); }//throw exception?
		const CWeatherYear& at(const size_t& y)const { assert(y < size()); assert(CWeatherYearMap::at(I2Year(y)).get() != NULL); return *CWeatherYearMap::at(I2Year(y)); }//throw exception?
		CDataInterface& operator[](CTRef ref) { return Get(ref); }
		const CDataInterface& operator[](CTRef ref)const { return Get(ref); }

		const CStatistic& GetStat(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& GetStat(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { CompileStat(p); return m_stat[v]; }
		const CStatistic& operator()(const HOURLY_DATA::TVarH& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }
		const CStatistic& operator()(const HOURLY_DATA::TVarEx& v, const CTPeriod& p = CTPeriod())const { return GetStat(v, p); }
		const CStatistic& operator[](HOURLY_DATA::TVarH v)const { return GetStat(v); }
		const CStatistic& operator[](HOURLY_DATA::TVarEx v)const { return GetStat(v); }

		CWeatherYear& CreateYear(CTRef TRef)
		{
			return CreateYear(TRef.GetYear());
		}

		void CreateYears(const CTPeriod& p)
		{
			CreateYears(p.Begin().GetYear(), p.GetNbYears());
		}

		void CreateYears(int firstYear, size_t nbYears)
		{
			for (size_t y = 0; y < nbYears; y++)
				CreateYear(firstYear + int(y));
		}

		CWeatherYear& CreateYear(int year)
		{
			assert(year != -999);

			if (find(year) == end())
			{
				//m_years.insert(year);
				CWeatherYearPtr pYear(new CWeatherYear(CTRef(year), this));
				insert(make_pair(year, pYear));
			}

			assert(CWeatherYearMap::at(year).get());
			return at(year);
		}

		bool IsYearInit(int year)const { return find(year) != end(); }
		bool IsYearInit(std::set<int> years)const
		{
			bool bRep = true;
			for (std::set<int> ::const_iterator it = years.begin(); it != years.end() && bRep; it++)
				bRep = IsYearInit(*it);

			return bRep;
		}

		bool HaveData()const
		{
			bool bHaveData = false;

			for (const_iterator it = begin(); it != end() && !bHaveData; it++)
				bHaveData = it->second->HaveData();


			return bHaveData;
		}

		size_t GetNbDays()const
		{
			size_t nbDays = 0;
			for (const_iterator it = begin(); it != end(); it++)
				nbDays += it->second->GetNbDays();

			return nbDays;
		}

		CWVariables GetVariables()const
		{
			CWVariables variables;
			for (const_iterator it = begin(); it != end(); it++)
				variables |= it->second->GetVariables();

			return variables;
		}

		CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter variables;
			for (const_iterator it = begin(); it != end(); it++)
				//if( at(y).get() )
				variables += it->second->GetVariablesCount(bDailyOnly);

			return variables;
		}

		bool IsComplete(CWVariables variables, CTPeriod period = CTPeriod())const;
		void CleanUnusedVariable(CWVariables variables);

		//inline const CWeatherYear& GetPrevious(CTRef ref)const;
		//inline const CWeatherYear& GetNext(CTRef ref)const;

		CTM GetTM()const { return CTM(IsHourly() ? CTM::HOURLY : CTM::DAILY); }
		int GetFirstYear()const { return empty() ? YEAR_NOT_INIT : begin()->first; }
		int GetLastYear()const { return empty() ? YEAR_NOT_INIT : rbegin()->first; }
		CTPeriod GetEntireTPeriod()const { return GetEntireTPeriod(GetTM()); }
		CTPeriod GetEntireTPeriod(CTM TM)const { return empty() ? CTPeriod() : CTPeriod(CTRef(GetFirstYear(), FIRST_MONTH, FIRST_DAY, FIRST_HOUR, TM), CTRef(GetLastYear(), LAST_MONTH, LAST_DAY, LAST_HOUR, TM)); }
		void SetHourly(bool bHourly) { m_bHourly = bHourly; ManageHourlyData(); }
		virtual bool IsHourly()const { return m_bHourly; }
		bool IsDaily()const { return !m_bHourly; }

		void ManageHourlyData()
		{
			for (iterator it = begin(); it != end(); it++)
				it->second->ManageHourlyData();
		}

		virtual inline const CDataInterface* GetParent()const { ASSERT(false); return NULL; }//call GetWeatherStation Instead
		virtual inline CDataInterface* GetParent() { ASSERT(false); return NULL; }//call GetWeatherStation Instead
		virtual inline CWeatherYears& GetWeatherYears();
		virtual inline const CWeatherYears& GetWeatherYears()const;
		virtual inline CWeatherStation* GetWeatherStation();
		virtual inline const CWeatherStation* GetWeatherStation()const;

		ERMsg SaveData(const std::string& filePath, CTM TM = CTM(), char separator = ',')const;
		ERMsg SaveData(std::ostream& file, CTM TM, char separator)const;
		ERMsg SaveData(std::ostream& file, CTM TM, const CWeatherFormat& format, char separator)const;
		ERMsg LoadData(const std::string& filePath, double nodata = -999.0, bool bResetContent = true, const CWeatherYearSectionMap& sectionToLoad = CWeatherYearSectionMap());
		ERMsg Parse(const std::string& str, double nodata = -999.0);
		CDailyWaveVector& GetHourlyGeneration(CDailyWaveVector& t, size_t method = HG_DOUBLE_SINE, size_t step = 4, double PolarDayLength = 3, const COverheat& overheat = COverheat()) const;

		int I2Year(const size_t& y)const { assert(y < size());  return std::next(begin(), y)->first; }
		std::set<int> GetYears()const;


		const CWeatherFormat& GetFormat()const { return m_format; }
		void SetFormat(const CWeatherFormat& in) { m_format = in; }

		inline const CDataInterface& Get(CTRef ref)const;
		inline CDataInterface& Get(CTRef ref);
		inline const CLocation& GetLocation()const;
		inline const CHourlyData& GetHour(CTRef ref)const;
		inline CHourlyData& GetHour(CTRef ref);
		inline const CWeatherDay& GetDay(CTRef ref)const;
		inline CWeatherDay& GetDay(CTRef ref);
		inline const CWeatherMonth& GetMonth(CTRef ref)const;
		inline CWeatherMonth& GetMonth(CTRef ref);


		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CWeatherYearMap>(*this);
		}



		virtual const CStatistic& GetData(HOURLY_DATA::TVarH v)const { return m_stat[v]; }
		virtual CStatistic& GetData(HOURLY_DATA::TVarH v) { return m_stat[v]; }
		virtual void SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat) { m_stat[v] = stat; }
		virtual bool GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const { stat = m_stat[v]; return stat.IsInit(); }
		virtual CTRef GetTRef()const { return CTRef(0, 0, 0, 0, CTM(CTM::ANNUAL, CTM::OVERALL_YEARS)); }
		virtual void WriteStream(std::ostream& stream, const CWVariables& variable, bool asStat = true)const override;
		virtual void ReadStream(std::istream& stream, const CWVariables& variable, bool asStat = true)override;
		virtual CStatistic GetVarEx(HOURLY_DATA::TVarEx v)const;
		virtual CStatistic GetTimeLength()const { return CStatistic((double)GetNbDays() * 24 * 60 * 60); }					//return time length [s]
		virtual double GetNetRadiation(double& Fcd)const;
		virtual void Reset();

		void ResetStat();

		bool IsCompilingHourly()const { return m_bCompilingHourly; }
		void IsCompilingHourly(bool in = true) { m_bCompilingHourly = in; }
		void CompleteSnow();
		bool ComputeHourlyVariables(CWVariables variables = CWAllVariables(), std::string options = "");
		bool IsHourlyComputed()const { return m_bHourlyComputed; }


	protected:

		bool m_bHourlyComputed;//compute only once
		bool m_bHourly;
		bool m_bCompilingHourly;

		void CompileStat(const CTPeriod& p = CTPeriod())const;


		CWeatherExStatistic m_stat;

		CWeatherStation* m_pParent;
		CWeatherFormat m_format;
		//size_t m_hourlyGenerationMethod;
	};


	class CWeatherStation : public CLocation, public CWeatherYears
	{
	public:

		IAgent* m_pAgent;
		DWORD m_hxGridSessionID;

		CWeatherStation(bool bIsHourly = false);
		CWeatherStation(const CWeatherStation& in);

		void clear();

		CWeatherStation& operator=(const CWeatherStation& in);
		bool operator==(const CWeatherStation& in)const;
		bool operator!=(const CWeatherStation& in)const { return !operator==(in); }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CLocation>(*this);
			ar & boost::serialization::base_object<CWeatherYears>(*this);
		}

		virtual ERMsg SaveData(const std::string& filePath, CTM TM = CTM(), char separator = ',')const { return CWeatherYears::SaveData(filePath, TM, separator); }


		void FillGaps();
		void ApplyCorrections(const CWeatherCorrections& corrections);

		void WriteStream(std::ostream& stream, bool asStat = true)const;
		ERMsg ReadStream(std::istream& stream, bool asStat = true);



	};

	typedef CWeatherStation CSimulationPoint;

	//****************************************************************************************************************
	inline bool CHourlyData::IsYearInit(int year)const { _ASSERTE(m_pParent); return m_pParent->IsYearInit(year); }
	inline bool CWeatherDay::IsYearInit(int year)const { _ASSERTE(m_pParent); return m_pParent->IsYearInit(year); }
	inline bool CWeatherMonth::IsYearInit(int year)const { _ASSERTE(m_pParent); return m_pParent->IsYearInit(year); }
	inline bool CWeatherYear::IsYearInit(int year)const { _ASSERTE(m_pParent); return m_pParent->IsYearInit(year); }

	inline const CDataInterface& CHourlyData::Get(CTRef ref)const { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline CDataInterface& CHourlyData::Get(CTRef ref) { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline const CDataInterface& CWeatherDay::Get(CTRef ref)const { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline CDataInterface& CWeatherDay::Get(CTRef ref) { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline const CDataInterface& CWeatherMonth::Get(CTRef ref)const { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline CDataInterface& CWeatherMonth::Get(CTRef ref) { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline const CDataInterface& CWeatherYear::Get(CTRef ref)const { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline CDataInterface& CWeatherYear::Get(CTRef ref) { _ASSERTE(m_pParent); return m_pParent->Get(ref); }
	inline const CDataInterface& CWeatherYears::Get(CTRef ref)const { return const_cast<CWeatherYears*>(this)->Get(ref); }
	inline CDataInterface& CWeatherYears::Get(CTRef ref)
	{
		assert(ref.GetYear() <= 0 || ref.GetYear() >= 1500 && ref.GetYear() <= 2200);

		CWeatherYears& me = *this;
		return
			(ref.GetType() == CTRef::HOURLY) ? (CDataInterface&)me[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()] :
			(ref.GetType() == CTRef::DAILY) ? (CDataInterface&)me[ref.GetYear()][ref.GetMonth()][ref.GetDay()] :
			(ref.GetType() == CTRef::MONTHLY) ? (CDataInterface&)me[ref.GetYear()][ref.GetMonth()] : (CDataInterface&)me[ref.GetYear()];
	}

	inline const CHourlyData& CWeatherYears::GetHour(CTRef ref)const { const CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()]; }
	inline CHourlyData& CWeatherYears::GetHour(CTRef ref) { CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()][ref.GetDay()][ref.GetHour()]; }
	inline const CWeatherDay& CWeatherYears::GetDay(CTRef ref)const { const CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()][ref.GetDay()]; }
	inline CWeatherDay& CWeatherYears::GetDay(CTRef ref) { CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()][ref.GetDay()]; }
	inline const CWeatherMonth& CWeatherYears::GetMonth(CTRef ref)const { const CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()]; }
	inline CWeatherMonth& CWeatherYears::GetMonth(CTRef ref) { CWeatherYears& me = *this; return me[ref.GetYear()][ref.GetMonth()]; }


	inline const CLocation& CHourlyData::GetLocation()const { return m_pParent->GetLocation(); }
	inline const CLocation& CWeatherDay::GetLocation()const { return m_pParent->GetLocation(); }
	inline const CLocation& CWeatherMonth::GetLocation()const { return m_pParent->GetLocation(); }
	inline const CLocation& CWeatherYear::GetLocation()const { return m_pParent->GetLocation(); }
	inline const CLocation& CWeatherYears::GetLocation()const { assert(m_pParent); return *m_pParent; }



	//inline const CWeatherDay& CWeatherDay::GetPrevious()const
	//{
	//	//static CWeatherDay REPLICATE_FIRST;
	//	CTRef TRef = GetTRef() - 1;
	//	bool bIsNotFirst = IsYearInit(TRef.GetYear());
	//	return bIsNotFirst ? (const CWeatherDay&)Get(TRef) : *this;
	//}
	//
	//
	//inline const CWeatherDay& CWeatherDay::GetNext()const
	//{
	//	//static CWeatherDay REPLICATE_LAST;
	//	CTRef TRef = GetTRef() + 1;
	//	bool bIsNotLast = IsYearInit(TRef.GetYear());
	//	return bIsNotLast ? (const CWeatherDay&)Get(TRef) : *this;
	//}
	//
	//inline const CWeatherMonth& CWeatherMonth::GetPrevious()const
	//{
	//	CTRef TRef = GetTRef() - 1;
	//	return IsYearInit(TRef.GetYear()) ? (const CWeatherMonth&)Get(TRef) : *this;
	//}
	//
	//inline const CWeatherMonth& CWeatherMonth::GetNext()const
	//{
	//	CTRef TRef = GetTRef() + 1;
	//	return (IsYearInit(TRef.GetYear())) ? (const CWeatherMonth&)Get(TRef) : *this;
	//}
	//
	//
	//inline const CWeatherYear& CWeatherYear::GetPrevious()const
	//{
	//	CTRef TRef = GetTRef() - 1;
	//	return (m_pParent->IsYearInit(TRef.GetYear()))? (const CWeatherYear&)Get(TRef): *this;
	//}
	//
	//inline const CWeatherYear& CWeatherYear::GetNext()const
	//{
	//	CTRef TRef = GetTRef() + 1;
	//	return (m_pParent->IsYearInit(TRef.GetYear())) ? (const CWeatherYear&)Get(TRef) : *this;
	//}
	//



	typedef std::vector<CWeatherStation> CWeatherStationVectorBase;
	class CWeatherStationVector : public CWeatherStationVectorBase
	{
	public:

		CWeatherStationVector(size_t s = 0/*, bool bTakeElevation, bool bTakeShoreDistance*/) : CWeatherStationVectorBase(s)
		{
			//m_bTakeElevation = bTakeElevation;
			//m_bTakeShoreDistance = bTakeShoreDistance;
		}


		CWVariables GetVariables()const
		{
			CWVariables variables;
			for (const_iterator it = begin(); it != end(); it++)
				variables |= it->GetVariables();

			return variables;
		}

		CWVariablesCounter GetVariablesCount(bool bDailyOnly = false)const
		{
			CWVariablesCounter counts;
			for (const_iterator it = begin(); it != end(); it++)
				counts += it->GetVariablesCount(bDailyOnly);

			return counts;
		}


		void FillGaps();
		void ApplyCorrections(const CWeatherCorrections& correction);

		//double GetXsum(bool bTakeElevation, bool bTakeShoreDistance);
		CWeightVector GetWeight(CWVariables variables, const CLocation& target, bool bTakeElevation, bool bTakeShoreDistance)const;
		void GetInverseDistanceMean(CWVariables variables, const CLocation& target, CWeatherStation& Data, bool bTakeElevation, bool bTakeShoreDistance)const;


		CTPeriod GetEntireTPeriod()const
		{
			CTPeriod p;
			for (const_iterator it = begin(); it != end(); it++)
				p += it->GetEntireTPeriod();

			return p;
		}

		bool IsComplete(CWVariables variables, CTPeriod period = CTPeriod())const;
		void CleanUnusedVariable(CWVariables variables);

		void GetMean(CWeatherStation& station, CTPeriod p, size_t mergeType)const;
		void MergeStation(CWeatherStation& station, CTM TM, size_t mergeType, size_t priorityRules, std::string& log)const;


	protected:

		//bool m_bTakeElevation;
		//bool m_bTakeShoreDistance;
	};


	typedef CWeatherStationVector CDailyStationVector;
	typedef CWeatherStationVector CSimulationPointVector;
	typedef std::shared_ptr<CWeatherStation> CWeatherStationPtr;



	inline const CDataInterface* CHourlyData::GetParent()const { return m_pParent; }
	inline CDataInterface* CHourlyData::GetParent() { return m_pParent; }
	inline const CDataInterface* CWeatherDay::GetParent()const { return m_pParent; }
	inline CDataInterface* CWeatherDay::GetParent() { return m_pParent; }
	inline const CDataInterface* CWeatherMonth::GetParent()const { return m_pParent; }
	inline CDataInterface* CWeatherMonth::GetParent() { return m_pParent; }
	inline const CDataInterface* CWeatherYear::GetParent()const { return m_pParent; }
	inline CDataInterface* CWeatherYear::GetParent() { return m_pParent; }

	inline CWeatherYears& CWeatherYears::GetWeatherYears() { return *this; }
	inline const CWeatherYears& CWeatherYears::GetWeatherYears()const { return *this; }
	inline CWeatherStation* CWeatherYears::GetWeatherStation() { return m_pParent; }
	inline const CWeatherStation* CWeatherYears::GetWeatherStation()const { return m_pParent; }

	inline bool CWeatherDay::IsHourly()const { return m_pParent->IsHourly(); }
	inline bool CWeatherMonth::IsHourly()const { return m_pParent->IsHourly(); }
	inline bool CWeatherYear::IsHourly()const { return m_pParent->IsHourly(); }

	inline bool CDataInterface::HavePrevious()const { return GetWeatherYears().IsYearInit((GetTRef() - 1).GetYear()); }
	inline bool CDataInterface::HaveNext()const { return GetWeatherYears().IsYearInit((GetTRef() + 1).GetYear()); }

}//namespace WBSF




namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CWeatherStation& station, XmlElement& output)
	{
		//save parent
		writeStruc((const WBSF::CLocation&)station, output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CWeatherStation& station)
	{
		readStruc(input, (WBSF::CLocation&)station);
		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CWeatherStationVector& in, XmlElement& output)
	{
		for (WBSF::CWeatherStationVector::const_iterator it = in.begin(); it != in.end(); it++)
			writeStruc(*it, output.addChild("Location"));
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CWeatherStationVector& out)
	{
		auto test = input.getChildren("Location");

		out.resize(std::distance(test.first, test.second));
		int i = 0;
		for (XmlElement::ChildIterConst2 it = test.first; it != test.second; it++, i++)
		{
			readStruc(*it, out.at(i));
		}

		return true;
	}
}//namespace zen

