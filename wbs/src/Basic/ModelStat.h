//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************

#pragma once

#include <vector>
#include <CrtDbg.h>

#include "Basic/UtilMath.h"
#include "Basic/UtilTime.h"
#include "Basic/Statistic.h"
#include "Basic/Mtrx.h"
#include "../ModelBase/WeatherBasedSimulation.h"

namespace WBSF
{
	class CIndividualInfo
	{
	public:
		CIndividualInfo(CTRef creationDate = CTRef(), double age = 0, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0, double scaleFactor = 0)
		{
			m_creationDate = creationDate;
			m_age = age;
			m_sex = sex;
			m_bFertil = bFertil;
			m_generation = generation;
			m_scaleFactor = scaleFactor;
		}


		CTRef m_creationDate;
		double m_age;
		TSex m_sex;
		bool m_bFertil;
		size_t m_generation;
		double m_scaleFactor;
	};

	class CInitialPopulation : public std::vector < CIndividualInfo >
	{
	public:

		CInitialPopulation(size_t nbObjects, double initialPopulation, double age, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0);
		CInitialPopulation(CTRef peakDay = CTRef(), double sigma = 0, size_t nbObjects = 400, double initialPopulation = 100, double age = 0, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0);
		void Initialize(size_t nbObjects, double initialPopulation, double age, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0);
		void Initialize(CTRef peakDay, double sigma = 0, size_t nbObjects = 400, double initialPopulation = 100, double age = 0, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0);


		void UpdateYear(int year)
		{
			for (iterator it = begin(); it != end(); it++)
				it->m_creationDate.m_year = year;
		}
	};




	class CTStatMatrix;

	typedef std::vector<double> _CModelStat;
	class CModelStat : public _CModelStat
	{
	public:


		CModelStat(size_t nbStat = 0, double initValue = 0) :_CModelStat(nbStat, initValue){}
		void Reset(double v)
		{
			for (size_t i = 0; i < size(); i++)
				at(i) = v;
		}


		double GetPourcent(size_t stat, size_t reference)const
		{
			_ASSERTE(stat >= 0 && stat < size());
			_ASSERTE(reference >= 0 && reference < size());

			return at(reference)>0 ? at(stat) / at(reference) * 100 : 0;
		}


		CModelStat& operator +=(const CModelStat& stat)
		{
			_ASSERTE(size() == stat.size());
			for (size_t i = 0; i < size(); i++)
				at(i) += stat.at(i);

			return *this;
		}

		double GetAverageInstar(size_t f, size_t F, size_t l, bool bStopDevLast = false)const
		{
			const CModelStat& me = (CModelStat&)(*this);

			double nbValue = me[f];
			double sum = me[f] * F;

			for (size_t i = f; i <= l; i++)
			{
				int iStopDev = (i == l && bStopDevLast) ? -1 : 0;

				nbValue += me[f + i];
				sum += me[f + i] * (F + i + iStopDev);
			}

			_ASSERTE(nbValue >= 0 && sum >= 0);
			return nbValue > 0 ? sum / nbValue : -999;
		}

	};

	class CTStatMatrix;
	typedef std::vector<CModelStat> CModelStatVectorBase;
	class CModelStatVector : public CModelStatVectorBase
	{
	public:

		CModelStatVector(size_t size = 0, CTRef firstDate = CTRef(), short nbStat = 0, double initValue = 0, const std::string& header = ""){ m_missingValue = -9999; Init(size, firstDate, nbStat, initValue, header); }
		CModelStatVector(const CTPeriod& p, size_t nbStat = 0, double initValue = 0, const std::string& header = ""){ m_missingValue = -9999; Init(p, nbStat, initValue, header); }
		
		CModelStatVector(const CModelStatVector& in, CTM TM=CTM(), size_t stat = MEAN);
		CModelStatVector(const CModelStatVector& in, const CTTransformation& TT, size_t stat);
		CModelStatVector(const CTStatMatrix& in, size_t stat);

		void Init(size_t size, CTRef firstDate, size_t nbStat, double initValue = 0, const std::string& header = ""){ m_firstTRef = firstDate; m_nbStat = nbStat; m_header = header; resize(size, initValue); }
		void Init(const CTPeriod& p, size_t nbStat, double initValue = 0, const std::string& header = ""){ Init(p.GetNbRef(), p.Begin(), nbStat, initValue, header); }
		void Init(const CModelStatVector& in, CTM TM=CTM(), size_t stat = MEAN);
		void Init(const CModelStatVector& in, const CTTransformation& TT, size_t stat);
		
		void Init(const CTStatMatrix& in, size_t stat);

		void Reset(){ Init(0, CTRef(), 0, 0); }

		size_t GetRows()const{ return size(); }
		size_t GetCols()const{ return m_nbStat; }
		void resize(size_t size, size_t nbStat, double defaultValue){ m_nbStat = nbStat; resize(size, defaultValue); }
		void resize(size_t size, double defaultValue = 0)
		{
			CModelStatVectorBase::resize(size);
			for (size_t i = 0; i < size; i++)
				at(i).resize(m_nbStat, defaultValue);
		}

		CModelStatVector& operator=(const CModelStatVector& in);
		CModelStatVector& swap(CModelStatVector& in);

		bool HaveData()const;
		__int32 GetFirstIndex(size_t stat, const std::string& op, double threshold, int nbDayBefore = 1, const CTPeriod& p = CTPeriod())const;
		__int32 GetLastIndex(size_t stat, const std::string& op, double threshold, int nbDayAfter = 1, const CTPeriod& p = CTPeriod())const;
		CTRef GetFirstTRef(size_t v, const std::string& op, double threshold = 0, int nbDayBefore = 1, const CTPeriod& p = CTPeriod())const{ int i = GetFirstIndex(v, op, threshold, nbDayBefore, p); return i == -1 ? CTRef() : m_firstTRef + i; }
		CTRef GetLastTRef(size_t v, const std::string& op, double threshold = 0, int nbDayAfter = 1, const CTPeriod& p = CTPeriod())const{ int i = GetLastIndex(v, op, threshold, nbDayAfter, p); return i == -1 ? CTRef() : m_firstTRef + i; }
		CTPeriod GetTPeriod(size_t v, const std::string& op, double threshold = 0, int nbDayBefore = 1, int nbDayAfter = 1, const CTPeriod& p = CTPeriod())const{ return CTPeriod(GetFirstTRef(v, op, threshold, nbDayBefore, p), GetLastTRef(v, op, threshold, nbDayAfter, p)); }
		
		CStatistic GetStat(size_t v, const CTPeriod& period = CTPeriod())const;

		CTRef GetFirstTRef()const{ return m_firstTRef; }
		void SetFirstTRef(CTRef firstDate){ m_firstTRef = firstDate; }
		CTRef GetLastTRef()const{ return m_firstTRef + (size() - 1); }

		using CModelStatVectorBase::at;
		const CModelStat& at(CTRef d)const{ ASSERT(d.GetTM() == m_firstTRef.GetTM()); return at(d - m_firstTRef); }
		CModelStat& at(CTRef d){ ASSERT(d.GetTM() == m_firstTRef.GetTM());  return at(d - m_firstTRef); }
		using CModelStatVectorBase::operator[];
		const CModelStat& operator [](CTRef d)const{ return at(d); }
		CModelStat& operator [](CTRef d){ return at(d); }
		size_t Insert(CTRef r, const CModelStat& row);

		CTM GetTM()const{ return m_firstTRef.GetTM(); }
		CTPeriod GetTPeriod()const{ return CTPeriod(m_firstTRef, m_firstTRef + (size() - 1)); }
		bool IsInside(CTRef d)const{ return GetTPeriod().IsInside(d); }
		__int32 GetNbDay(size_t v, const CMathEvaluation& op, const CTPeriod& p, bool bConsecutive, double MISSING = -999)const;

		void WriteStream(std::ostream& stream)const;
		ERMsg ReadStream(std::istream& stream);

		void ConvertValue(float value1, float value2);
		size_t GetNbStat()const{ return m_nbStat; }
		CInitialPopulation GetInitialPopulation(size_t var, size_t nbObject = 400, double initialPopulation = 100, double age = 0, TSex sex = RANDOM_SEX, bool bFertil = false, size_t generation = 0, CTPeriod p = CTPeriod())const;
		CInitialPopulation GetInitialPopulation(size_t var, CTPeriod p)const{ return GetInitialPopulation(var, 400, 100, 0, RANDOM_SEX, false, 0, p); }
		void Transform(const CTM& TM, size_t s);//= WBSF::MEAN
		void Transform(const CTTransformation& TT, size_t s );//= WBSF::MEAN
		

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath)const;

		double GetMissing()const{ return m_missingValue; }
		void SetMissing(double missing){ m_missingValue = missing; }

		static bool test_op(double value, const std::string& op, double threshold);

	protected:


		CTRef m_firstTRef;
		size_t m_nbStat;
		double m_missingValue;
		std::string m_header;
	};


	template <__int32 T, const char * U = NULL>
	class CModelStatVectorTemplate : public CModelStatVector
	{
	public:

		CModelStatVectorTemplate(size_t size = 0, CTRef firstDate = CTRef(), double initValue = 0) :CModelStatVector(size, firstDate, T, initValue, U ? U : ""){}
		CModelStatVectorTemplate(const CTPeriod& p, double initValue = 0) :CModelStatVector(p, T, initValue, U ? U : ""){}
		void Init(size_t size = 0, CTRef firstDate = CTRef(), double initValue = 0){ CModelStatVector::Init(size, firstDate, T, initValue, U ? U : ""); }
		void Init(const CTPeriod& p, double initValue = 0){ CModelStatVector::Init(p, T, initValue, U ? U : ""); }


	};


	//*********************************************************************************************************


	template <class T>
	class CTReferencedMatrixIterator
	{
	public:

		CTReferencedMatrixIterator(CMatrix<T>& matrix, size_t row) :
			m_matrix(matrix),
			m_row(row)//actual level of the reference
		{}

		T& operator[](size_t col)	{ return m_matrix[m_row][col]; }
		const T& operator[](size_t col)const{ return m_matrix[m_row][col]; }

	protected:

		CMatrix<T>& m_matrix;
		size_t m_row;
	};

	template <class T>
	class CTReferencedMatrix : public CMatrix < T >
	{
	public:

		CTPeriod m_period;

		CTReferencedMatrix(CTPeriod period = CTPeriod(), size_t nbVars = 0)
		{
			Init(period, nbVars);
		}

		void Init(CTPeriod period = CTPeriod(), size_t nbVars = 0)
		{
			m_period = period;
			resize_rows(m_period.GetNbRef());
			resize_cols(nbVars);
		}
		

		CTReferencedMatrix(const CTReferencedMatrix& in){ operator=(in); }

		CTReferencedMatrix& operator=(const CTReferencedMatrix& in)
		{
			if (&in != this)
			{
				CMatrix<T>::operator =(in);
				m_period = in.m_period;
			}

			return *this;
		}

		CTRef GetFirstTRef()const{ return m_period.Begin(); }
		CTM GetTM()const{ return m_period.GetTM(); }
		bool IsInit()const{ return m_period.IsInit(); }

		using CMatrix<T>::operator[];
		CTReferencedMatrixIterator<T> operator[](const CTRef& TRef)const{ return at(TRef); }
		CTReferencedMatrixIterator<T> operator[](const CTRef& TRef){ return at(TRef); }
		CTReferencedMatrixIterator<T> at(const CTRef& TRef)const{ return CTReferencedMatrixIterator<T>(*this, TRef - m_period.Begin()); }
		CTReferencedMatrixIterator<T> at(const CTRef& TRef){ return CTReferencedMatrixIterator<T>(*this, TRef - m_period.Begin()); }

		CTRef GetTRef(size_t i)const{ return (m_period.Begin() + i); }

		T& operator()(CTRef TRef, size_t col)
		{
			CMatrix<T>& me = *this;
			return me[TRef - m_period.Begin()][col];
		}

		const T& operator()(CTRef TRef, size_t col)const
		{
			CMatrix<T>& me = *this;
			return me[TRef - m_period.Begin()][col];
		}
	};


	typedef CTReferencedMatrix<CStatistic> CTStatMatrixBase;
	class CTStatMatrix : public CTStatMatrixBase
	{
	public:

		CTStatMatrix(CTPeriod period = CTPeriod(), size_t nbVar=0);
		CTStatMatrix(const CModelStatVector& input, const CTM& TM = CTM());
		CTStatMatrix(const CModelStatVector& input, const CTTransformation& TT);

		void Init(const CModelStatVector& in, const CTM& TM = CTM());
		void Init(const CModelStatVector& in, const CTTransformation& TT);
		
		void Transform(const CTM& TM);
		void Transform(const CTTransformation& TT);
	};


	struct ModelInterface
	{
		
		void Transform(const CTM& TM, const CModelStatVector& in, CModelStatVector& out, size_t s){ Transform(CTTransformation(in.GetTPeriod(), TM), in, out, s); }
		void Transform(const CTTransformation& TT, const CModelStatVector& in, CModelStatVector& out, size_t s);
		void Transform(const CTM& TM, const CModelStatVector& in, CTStatMatrix& out){ Transform(CTTransformation(in.GetTPeriod(), TM), in, out); }
		void Transform(const CTTransformation& TT, const CModelStatVector& in, CTStatMatrix& out);

	};


}//namespace WBSF