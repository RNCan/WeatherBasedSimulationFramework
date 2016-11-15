#pragma once


#include <deque>
#include <map>
#include <dlib/statistics.h>



#include "ANN/ANN.h"
#include "Basic/ERMsg.h"
#include "Basic/statistic.h"
#include "Basic/UtilMath.h"
#include "Basic/UtilStd.h"
#include "Geomatic/GDALBasic.h"


class CSearchResult
{
public:

	CSearchResult(long index=0, double distance=0, double geoDistance=0)
	{
		m_index=index;
		m_distance=distance;
		m_geoDistance = geoDistance;
	}
	
	bool operator ==(const CSearchResult& results)const
	{
		//warning: only the index is take in the == operator
		return m_index == results.m_index;
	}
	bool operator !=(const CSearchResult& results)const
	{
		return !((*this)== results);
	}
	
	
	double GetXTemp(double T, bool bGeoWeight)const
	{
		double d = m_distance + (bGeoWeight?m_geoDistance:0) + 0.0000000000001;
		return pow(d,-T);
	}

	long m_index;
	double m_distance;
	double m_geoDistance;
	
};


typedef std::deque<CSearchResult> _CSearchResultVector;
class CSearchResultVector: public _CSearchResultVector
{
public:

	double GetXSum(double T, bool bGeoWeight)const
	{
		double xSum=0;
		
		for(const_iterator it=begin(); it<end(); it++)
			xSum += it->GetXTemp(T, bGeoWeight);

		return xSum;
	}

	std::vector<double> GetWeight(double T, bool bGeoWeight)const
	{
		std::vector<double> weight(size());
		double xSum = GetXSum(T, bGeoWeight);

		std::vector<double>::iterator itw = weight.begin();
		for(const_iterator it=begin(); it<end(); it++, itw++)
		{
			*itw = it->GetXTemp(T, bGeoWeight) / xSum;
		}

		return weight;
	}
};



//muti-dimensional point + geographic coordinate
class CMDPoint : public WBSF::CGeoPoint
{
public:
	CMDPoint(size_t dim = 0);
	CMDPoint(const CMDPoint& point);
	~CMDPoint();

	CMDPoint& operator = (const CMDPoint& point);

	size_t GetNbDimension()const{ return m_spectrals.size(); }
	void SetNbDimension(size_t dim){ m_spectrals.resize(dim); }
	double& operator[](size_t index){ return m_spectrals[index]; }
	const double& operator[](size_t index)const{ return m_spectrals[index]; }

protected:
	
	std::deque<double> m_spectrals;
	
};

class CKNearestNeighbor;
typedef std::deque<CMDPoint> _CMDPointVector;
class CMDPointVector : public _CMDPointVector
{
public:
	CMDPointVector(bool bStandardized, INT_PTR dim=0, INT_PTR size=0);
	~CMDPointVector();

	
	friend CKNearestNeighbor;

	void resize(size_t nNewSize);
	CMDPointVector& operator=(const CMDPointVector& in );
	void push_back(const CMDPoint& newElement);
	void insert(INT_PTR nIndex, const CMDPoint& newElement );
	//void SetAtGrow(INT_PTR nIndex, const CMDPoint& newElement );
	CMDPoint& operator[](INT_PTR i){ return at(i); }
	const CMDPoint& operator[](INT_PTR i)const{ return at(i); }

	size_t GetNbDimension()const{return m_stats.size();}
	void SetNbDimension(size_t dim)
	{
		if( dim != m_stats.size())
		{
			m_stats.resize(dim);
			m_dimNames.resize(dim);

			for(iterator it=begin(); it<end(); it++)
			{
				it->SetNbDimension(dim);
			}

			
		}
	}

	const std::string& GetDimensionName(size_t i)const{ return m_dimNames[i]; }
	void SetDimensionName(size_t i, const std::string& name){ m_dimNames[i] = name; }


	ANNpointArray GetANNpointArray()const;
	void InitANNpoint(const CMDPoint& pt, ANNpoint q)const;
	ANNpoint GetANNpoint(const CMDPoint& pt)const;

	const WBSF::CStatistic& GetStatistic(size_t i)const{ return m_stats[i]; }

	size_t GetPrjID()const
	{ 
		return empty() ? WBSF::PRJ_NOT_INIT : front().GetPrjID(); 
	}
	
	void SetPrjID(size_t prjID)
	{ 
		for (CMDPointVector::iterator it = begin(); it!= end(); it++)
			it->SetPrjID(prjID);
	}

protected:


	WBSF::CStatisticVector m_stats;
	WBSF::StringVector m_dimNames;
	bool m_bStandardized;
	dlib::matrix<float> m_Ltrans;
	
};

//******************************************************************************

typedef std::map<size_t, int> CPredictorMap;
typedef std::pair<size_t, int> CPredictorPair;
class CPredictor : public std::deque<float>
{
public:

	CPredictor(size_t size=0):
	  std::deque<float>(size)
	{
	}
	
	const std::string& GetName()const{ return m_name;}
	void SetName(const std::string& name){ m_name=name;}
	double GetValue(const CSearchResultVector& result, double T, bool bGeoWeight)const;

	void clear(){ Reset(); }

	//Get nb class must be call first
	size_t GetNbClass()const
	{
		if( m_class.empty())
			Init();
		
		return m_class.size();
	}

	
	
	int GetClassIndex(size_t value)const
	{
		if( m_class.empty())
			Init();

		CPredictorMap::const_iterator it = m_class.find(value);
		
		return it->second;
	}

	size_t GetClassValue(int index)const
	{
		if( m_class.empty())
			Init();

		ASSERT( index>=0 && index<m_class.size() );

		CPredictorMap::const_iterator it = WBSF::MapSearchByValue(m_class, index);
		return it->first;
	}

	const WBSF::CStatistic& GetStatistic()const{ InitStats();  return m_stats; }


protected:

	
	void Reset();
	void Init()const;
	void InitStats()const;
	
	

	std::string m_name;
	CPredictorMap m_class;
	WBSF::CStatistic m_stats;

};


class CPredictorVector: public std::deque<CPredictor> 
{
public:

	friend CKNearestNeighbor;

	//double GetValue(size_t p, const CSearchResultVector& result, double T)const;

protected:

	dlib::matrix<float> m_Rtrans;
};


//*******************************************************************************************

class CExtraColumn: public std::vector<std::string>
{
public:
	std::string m_name;
};

typedef std::vector<CExtraColumn> CExtraColumnVector;


class CKNearestNeighbor
{
public:
	CKNearestNeighbor(bool bStandardized, bool bUseCCA = false);
	~CKNearestNeighbor();

	void Reset();
	void Init();
	ERMsg LoadCSV(const std::string& filePath, const std::string& XHeader = "X", const std::string& YHeader = "Y", const std::string& Xprefix = "X_", const std::string& Yprefix = "Y_");
	

	const CMDPointVector& GetX()const{ return m_X; }
	const CPredictorVector& GetY()const{ return m_Y; }
//	void GetPoints( CMDPointVector& points);
	
	size_t size()const{ return m_X.size(); }
	size_t GetNbPredictor(){  return m_Y.size(); }
	size_t GetNbDimension()const{return m_X.GetNbDimension();}
	bool HaveGeoCoordinates()const{ return m_geoPos.size()==2; }
	const CExtraColumnVector& GetExtraColumns()const{return m_extraColumns;}
	size_t GetPrjID()const{ return m_X.GetPrjID(); }
	void SetPrjID(size_t prjID){ m_X.SetPrjID(prjID); }

	const CMDPoint& operator[](size_t i)const{ return m_X[i];}
	const CPredictor& operator()(size_t i)const{ return m_Y[i];}

	

	ERMsg Search(const CMDPoint& pt, long nbPoint, CSearchResultVector& result, double eps = 0.0)const;

private:


	void ResetANN();
	void InitANN();
	bool IsANNInit()const{ return m_pTreeRoot != NULL; }
	void InitCCA();

	std::vector<int> m_Xpos;
	std::vector<int> m_Ypos;
	std::vector<int> m_geoPos;
	std::vector<int> m_extraPos;

	CMDPointVector m_X;
	CPredictorVector m_Y;
	//WBSF::CGeoPointVector m_geoCoordinates;
	

	CExtraColumnVector m_extraColumns;
	bool m_bUseCCA;//Use Canonical Correletaion Analyses


	ANNkd_tree* m_pTreeRoot;
	ANNpointArray m_pDataPts;		// data points

	static std::vector<int> GetColumnsPos(const WBSF::StringVector& line, const std::string& prefix, bool bExact = false);
	static std::vector<int> GetCoordinates(const WBSF::StringVector& header, const std::string& XHeader, const std::string& YHeader);
	static std::vector<int> GetExtraColumnsPos(const WBSF::StringVector& header, const std::vector<int>& XPos, const std::vector<int>& YPos, const std::vector<int> & coordinatePos);
		
};

