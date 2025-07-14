//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <assert.h>
//#include <Windows.h>
#include <vector>
#include <array>
//#include "Basic/UtilStd.h"

namespace WBSF
{

//type of statistic
//LOWEST:	The lowest value of the data
//MEAN	:	The mean of the data
//SUM	:	The sum of the data
//SUM²	:	The sum of square of each data
//STD_DEV	: Standard Deviation over sample
//STD_DEV_OVER_POP : Standard Deviation over the entire population (devide by N instead of N-1)
//STD_ERR	: Standard Error
//COEF_VAR	: The coeficiant of variation of the data
//VARIANCE	: The variance of the data
//HIGHEST	: The highest value of the data
//TSS		: total sum of square
//QUADRATIC_MEAN :
//RANGE		: difference between and highest and the lowest values
//NB_VALUE  : The number of values added in the statistic
//
enum TStat { LOWEST, MEAN, SUM, SUM2, STD_DEV, STD_DEV_OVER_POP, STD_ERR, COEF_VAR, VARIANCE, HIGHEST, TSS, QUADRATIC_MEAN, RANGE, NB_VALUE, NB_STAT_TYPE };
//MAD		: Mean Absolute Deviation
//SKEWNESS	: Skewness mean
//KURTOSIS	: Kurtosis mean
//MEDIAN	: median of the data
//MODE		: return the most common value. Usually use with integer values
//Q¹		: todo : first quartile value
//Q³		: todo : thirdth quartile value
//INTER_Q	: todo : the inter quartile value (Q³-Q¹)
enum TStatEx { MAD = NB_STAT_TYPE, SKEWNESS, KURTOSIS, MEDIAN, Q2 = MEDIAN, MODE, Ql, Q1, Q3, Qh, INTER_Q, NB_STAT_TYPE_EX };
//MEAN_X	: mean of x
//MEAN_Y	: mean of y
//INTERCEP	: intercep of the regression
//SLOPE		: the slope of the regression
//COVARIANCE: The covariance between x and y
//CORRELATION:
//BIAS		: Bias
//MAE		: Mean Absolute Error between x and y
//RMSE		: Residual Mean of Square Error between x and y
//RSS		: Residual Sum of Square between x and y
//COEF_D	: Coeficient of determination
//COEF_C	: Coeficient of correlation
//STAT_R²	: R²
enum TStatXY { MEAN_X = NB_STAT_TYPE_EX, MEAN_Y, INTERCEPT, SLOPE, COVARIANCE, CORRELATION, BIAS, MAE, RMSE, RSS, COEF_D, COEF_C, STAT_R2, NB_STATXY_TYPE };
//INTERCEPT_THEIL_SEN: Intercep of of Theil-sen computation
//SLOPE_THEIL_SEN: Slope of Theil-sen computation
//NEGATIVE_LOG_LIKELIHOOD:   negative log of Likelihood
enum TStatXYEx {INTERCEPT_THEIL_SEN=NB_STATXY_TYPE, SLOPE_THEIL_SEN, LIKELIHOOD, NB_STATXY_TYPE_EX};

inline bool IsTemporalStat(size_t stat)
{
    return stat==LOWEST||stat==MEAN||stat==HIGHEST;
}

//*************************************************************
//CStatistic

class CStatistic
{

public:

    static const std::array<const char*, NB_STATXY_TYPE_EX> NAME;


    CStatistic();
    CStatistic(const CStatistic& in)
    {
        operator=(in);
    }
    CStatistic(double value)
    {
        operator=(value);
    }

    void Reset();
    void clear()
    {
        Reset();
    }


    double operator[](size_t type)const;
    CStatistic& Add(double value)
    {
        return operator+=(value);
    }
    CStatistic& insert(double value)
    {
        return operator+=(value);
    }
    CStatistic& operator+=(double value);
    CStatistic& operator+=(const CStatistic& statistic);
    bool operator==(const CStatistic& in)const;
    bool operator!=(const CStatistic& in)const
    {
        return !operator==(in);
    }

    CStatistic operator+(const CStatistic& in)
    {
        return CStatistic(*this)+=in;
    }
    CStatistic& operator=(const CStatistic& in);
    CStatistic& operator=(double value)
    {
        Reset();
        return operator+=(value);
    }
    operator bool()const
    {
        return IsInit();
    }
    operator double()const
    {
        return operator[](WBSF::MEAN);
    }

    bool empty()const
    {
        return m_nbValues==0;
    }
    bool IsInit()const
    {
        return !empty();
    }

    static double GetVMiss();
    static void SetVMiss(double vmiss);

    static const char* GetName(size_t s)
    {
        assert(s<NB_STATXY_TYPE_EX);
        return NAME[s];
    }
    static const char* GetTitle(size_t s);
    static void ReloadString();

//    static double square(double v){return v*v;}

protected:

    size_t m_nbValues;

    double m_lowest;
    double m_sum;
    double m_sum2;
    double m_hightest;


    //static const char* TITLE[NB_STATXY_TYPE_EX];
    static std::vector<std::string> TITLE;
};

typedef std::vector<CStatistic>CStatisticVector;


//*************************************************************
//CStatisticEx : keep in memory all elements for MAD computation
class CStatisticEx: public CStatistic
{
public:

    CStatisticEx();
    void Reset();
    void clear()
    {
        Reset();
    }

    double operator[](size_t type)const;
    CStatisticEx& operator=(const CStatisticEx& in);
    CStatisticEx& Add(double value)
    {
        return operator+=(value);
    }
    CStatisticEx& insert(double value)
    {
        return operator+=(value);
    }
    CStatisticEx& operator+=(double value);
    CStatisticEx& operator+=(const CStatisticEx& statistic);

    //static const char* GetName(int i){ assert(i >= 0 && i<NB_STAT_TYPE_EX); return i<NB_STAT_TYPE ? CStatistic::GetName(i) : NAME[i - NB_STAT_TYPE]; }

    //const std::vector<double>& GetValues(){ return m_values; }
    const std::vector<double>& operator()(void)const
    {
        return m_values;
    }
    size_t size()const
    {
        return m_values.size();
    }
    double operator()(size_t i)const
    {
        return m_values[i];
    }
    double get_sorted(size_t i)const
    {
        sort();
        return m_sorted[i];
    }
    void sort()const;//sort is const for convenience

    double percentil(double p)const;

protected:

    std::vector<double> m_values;
    std::vector<double> m_sorted;
};


//*************************************************************
//CStatisticW: keep in memory for weighted statistic
class CStatisticXW
{
public:

    CStatisticXW();
    CStatisticXW(const CStatisticXW& in)
    {
        operator=(in);
    }
    CStatisticXW(double value, double weight)
    {
        Add(value, weight);
    }

    void Reset();
    void clear()
    {
        Reset();
    }

    double operator[](size_t type)const;
    CStatisticXW& Add(double value, double weight);
    CStatisticXW& insert(double value, double weight)
    {
        return Add(value, weight);
    };
    CStatisticXW& operator+=(const CStatisticXW& statistic);
    bool operator==(const CStatisticXW& in)const;
    bool operator!=(const CStatisticXW& in)const
    {
        return !operator==(in);
    }

    CStatisticXW operator+(const CStatisticXW& in)
    {
        return CStatisticXW(*this) += in;
    }
    CStatisticXW& operator=(const CStatisticXW& in);

    bool empty()const
    {
        return m_x.empty();
    }
    bool IsInit()const
    {
        return !empty();
    }

    const CStatisticEx& X()const
    {
        return m_x;
    }
    const CStatisticEx& W()const
    {
        return m_w;
    }
    double X(size_t i)const
    {
        return m_x[i];
    }
    double W(size_t i)const
    {
        return m_w[i];
    }

    const std::vector<double>& x()const
    {
        return m_x();
    }
    const std::vector<double>& w()const
    {
        return m_w();
    }
    double x(size_t i)const
    {
        return m_x(i);
    }
    double w(size_t i)const
    {
        return m_w(i);
    }

protected:

    CStatisticEx m_x;
    CStatisticEx m_w;
    double m_wx;
    double m_wx2;

};


//*************************************************************
//CStatisticXY

//for observation/simulated statistics
//template <class T>
class CStatisticXY
{

public:

    CStatisticXY();
    CStatisticXY(const CStatisticXY& in)
    {
        operator=(in);
    }
    ~CStatisticXY() {};
    void Reset();
    void clear()
    {
        Reset();
    }

    CStatisticXY& operator=(const CStatisticXY& in);
    double operator[](size_t type)const;
    CStatisticXY& Add(double x, double y);//x = pred, y = obs
    CStatisticXY& insert(double x, double y)
    {
        return Add(x, y);
    }
    CStatisticXY& operator+=(const CStatisticXY& statistic);

    const CStatistic& GetX()const
    {
        return m_x;
    }
    const CStatistic& GetY()const
    {
        return m_y;
    }
    const CStatistic& X()const
    {
        return m_x;
    }
    const CStatistic& Y()const
    {
        return m_y;
    }
    double X(size_t t)const
    {
        return m_x[t];
    }
    double Y(size_t t)const
    {
        return m_y[t];
    }

    static double GetVMiss();
    static void SetVMiss(double vmiss);

protected:

    CStatistic m_x;
    CStatistic m_y;
    double m_lowest;//lowest error
    double m_xy;//x*y
    double m_e;//x-y		Error
    double m_rs;//(x-y)²	Residual Square
    double m_ae;//|x-y|		Absolute Error
    double m_hightest;	//highest error
};

typedef std::vector<CStatisticXY>CStatisticXYVector;
typedef std::vector<CStatisticXYVector> CStatisticXYMatrix;

typedef CStatisticXY CStatistic2;
typedef std::vector<CStatisticXY>CStatistic2Vector;


class CStatisticXYEx : public CStatisticXY
{
public:

    CStatisticXYEx();
    CStatisticXYEx(const CStatisticXYEx& in)
    {
        operator=(in);	   //need copy constructor for m_CS
    }
    ~CStatisticXYEx() {};

    void Reset();
    void clear()
    {
        Reset();
    }

    CStatisticXYEx& operator=(const CStatisticXYEx& in);
    double operator[](size_t type)const;
    CStatisticXYEx& Add(double x, double y);//x = pred, y = obs
    CStatisticXYEx& insert(double x, double y)
    {
        return Add(x, y);
    }
    CStatisticXYEx& operator+=(const CStatisticXYEx& statistic);

    const std::vector<double>& x()const
    {
        return m_xValues;
    }
    const std::vector<double>& y()const
    {
        return m_yValues;
    }

    double x(size_t i)const
    {
        return m_xValues[i];
    }
    double y(size_t i)const
    {
        return m_yValues[i];
    }

    std::vector<double> GetCookDistance()const;

protected:

    std::vector<double> m_xValues;
    std::vector<double> m_yValues;

//	CCriticalSection m_CS;

};

typedef std::vector<CStatisticXYEx>CStatisticXYExVector;



//*************************************************************
//Weighted CStatisticXY

class CStatisticXYW
{
public:

    CStatisticXYW();
    CStatisticXYW(const CStatisticXYW& in)
    {
        operator=(in);
    }
    CStatisticXYW(double x, double y, double w)
    {
        insert(x, y, w);
    }

    void Reset();
    void clear()
    {
        Reset();
    }

    CStatisticXYW& operator=(const CStatisticXYW& in);
    double operator[](size_t type)const;
    CStatisticXYW&  Add(double x, double y, double w);//x = pred, y = obs, w=weight
    CStatisticXYW&  insert(double x, double y, double w);//x = pred, y = obs, w=weight
    CStatisticXYW&  operator+=(const CStatisticXYW& statistic);

    const CStatisticXW& X()const
    {
        return m_xw;
    }
    const CStatisticXW& Y()const
    {
        return m_yw;
    }
    const CStatisticEx& W()const
    {
        return m_xw.W();
    }
    double X(size_t t)const
    {
        return m_xw.X(t);
    }
    double Y(size_t t)const
    {
        return m_yw.X(t);
    }
    double W(size_t t)const
    {
        return m_xw.W(t);
    }


    const std::vector<double>& x()const
    {
        return m_xw.x();
    }
    const std::vector<double>& y()const
    {
        return m_yw.x();
    }
    const std::vector<double>& w()const
    {
        return m_xw.w();
    }

    double x(size_t i)const
    {
        return m_xw.x(i);
    }
    double y(size_t i)const
    {
        return m_yw.x(i);
    }
    double w(size_t i)const
    {
        return m_xw.w(i);
    }

protected:

    CStatisticXW m_xw;
    CStatisticXW m_yw;

    double m_wxy;	//w(x*y)
    double m_we;	//w(x-y)		Error
    double m_wrs;	//w(x-y)²	Residual Square
    double m_wae;	//w|x-y|	Absolute Error
};

//**************************************************************
//Classification

template <class T1, class T2>
class CXY
{
public:

    CXY(T1 x=0, T2 y=0)
    {
        m_x=x;
        m_y=y;
    }


    T1 m_x;
    T2 m_y;

    bool operator ==(const CXY& in)const
    {
        return m_x==in.m_x && m_y == in.m_y;
    }
    bool operator !=(const CXY& in)const
    {
        return !operator==(in);
    }
    bool operator >(const CXY& in)const
    {
        return m_x > in.m_x;
    }
    bool operator >=(const CXY& in)const
    {
        return m_x >= in.m_x;
    }
    bool operator <(const CXY& in)const
    {
        return m_x < in.m_x;
    }
    bool operator <=(const CXY& in)const
    {
        return m_x <= in.m_x;
    }
};

template <class T1, class T2>
class CXYVector : public std::vector< CXY<T1,T2> >
{
};


class CXYStat
{
public:
    CStatistic m_x;
    CStatistic m_y;
};

typedef std::vector<CXYStat> CXYStatVector;
class CClassify
{
public:

    CClassify();
    void Reset();
    void clear()
    {
        Reset();
    }

    void Add(double x, double y);
    void ClassifyManual(std::vector<double> classLimit);
    void ClassifyNaturalBreak(short nbClass=15);
    void ClassifyEqualInterval(short nbClass=15);

    int GetSize()const
    {
        return (int)m_stat.size();
    }
    size_t size()const
    {
        return m_stat.size();
    }
    const CXYStat& operator [](int i)const
    {
        assert(i>=0 && i<(int)m_stat.size());
        return m_stat[i];
    }

private:

    CXYStatVector  m_stat;
    CXYVector<double, double> m_values;
};

class CLinearEquation
{
public:

    CLinearEquation(double a0=0, double a1=0)
    {
        m_a0 = a0;
        m_a1 = a1;
    }

    CLinearEquation(const CStatisticXY& stat)
    {
        FromStat(stat);
    }

    double operator()(double x)
    {
        return m_a0 + x*m_a1;
    }

    void FromStat(const CStatisticXY& stat)
    {
        m_a0 = stat[WBSF::INTERCEPT];
        m_a1 = stat[WBSF::SLOPE];
    }

    double a0()const
    {
        return m_a0;
    }
    double a1()const
    {
        return m_a1;
    }

private:

    double m_a0;
    double m_a1;
};

}//namespace WBSF

