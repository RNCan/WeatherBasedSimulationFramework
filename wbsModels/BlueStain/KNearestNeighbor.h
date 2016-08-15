#pragma once

#include <deque>
#include <map>
#include "ANN/ANN.h"
#include "ERMsg/ERMsg.h"
#include "Basic/UtilMath.h"
#include "Basic/UtilStd.h"
#include "Basic/statistic.h"

namespace WBSF
{

	class CSearchResult
	{
	public:

		CSearchResult(long index = 0, double distance = 0)
		{
			m_index = index; m_distance = distance;
		}

		bool operator ==(const CSearchResult& results)const
		{
			//warning: only the index is take in the == operator
			return m_index == results.m_index;
		}
		bool operator !=(const CSearchResult& results)const
		{
			return !((*this) == results);
		}


		double GetXTemp(double power)const
		{
			double d = m_distance + 0.0000000000001; //Max( 0.0000000000001, m_distance); 
			return pow(d, -power);
		}

		long m_index;
		double m_distance;

	};


	typedef std::deque<CSearchResult> _CSearchResultVector;
	class CSearchResultVector : public _CSearchResultVector
	{
	public:

		double GetXSum(double power)const
		{
			double xSum = 0;

			for (const_iterator it = begin(); it < end(); it++)
				xSum += it->GetXTemp(power);

			return xSum;
		}

		std::vector<double> GetWeight(double T)const
		{
			std::vector<double> weight(size());
			double xSum = GetXSum(T);

			std::vector<double>::iterator itw = weight.begin();
			for (const_iterator it = begin(); it < end(); it++, itw++)
			{
				*itw = it->GetXTemp(T) / xSum;
			}

			return weight;
		}
	};



	//muti-Dimensional point
	class CMDPoint
	{
	public:
		CMDPoint(INT_PTR dim = 0);
		CMDPoint(const CMDPoint& point);
		~CMDPoint();

		CMDPoint& operator = (const CMDPoint& point);
		CMDPoint& Set(ANNpointArray points, long size, long dim, int index);

		size_t GetNbDimension()const{ return m_dataPts.size(); }
		void SetNbDimension(INT_PTR dim){ m_dataPts.resize(dim); }
		double& operator[](INT_PTR index){ return m_dataPts[index]; }
		const double& operator[](INT_PTR index)const{ return m_dataPts[index]; }

	private:

		std::deque<double> m_dataPts;
	};


	typedef std::deque<CMDPoint> _CMDPointVector;
	class CMDPointVector : public _CMDPointVector
	{
	public:
		CMDPointVector(bool bStandardized, INT_PTR dim = 0, INT_PTR size = 0);
		~CMDPointVector();


		void resize(size_t nNewSize);
		CMDPointVector& operator=(const CMDPointVector& in);
		void push_back(const CMDPoint& newElement);
		void insert(INT_PTR nIndex, const CMDPoint& newElement);
		//void SetAtGrow(INT_PTR nIndex, const CMDPoint& newElement );
		CMDPoint& operator[](INT_PTR i){ return at(i); }
		const CMDPoint& operator[](INT_PTR i)const{ return at(i); }

		size_t GetNbDimension()const{ return m_dimStats.size(); }
		void SetNbDimension(size_t dim)
		{
			if (dim != m_dimStats.size())
			{
				m_dimStats.resize(dim);
				m_dimNames.resize(dim);

				for (iterator it = begin(); it < end(); it++)
				{
					it->SetNbDimension(dim);
				}


			}
		}

		const std::string& GetDimensionName(size_t i)const{ return m_dimNames[i]; }
		void SetDimensionName(size_t i, const std::string& name){ m_dimNames[i] = name; }

		ANNpointArray	GetANNpointArray(const std::vector<int>& in)const;

		void SetANNpointArray(ANNpointArray points, long size, long dim);
		ANNpoint GetANNpoint(const CMDPoint& pt, const std::vector<int>& in)const;

		const CStatistic& GetStatistic(size_t i)const{ return m_dimStats[i]; }

	private:


		CStatisticVector m_dimStats;
		StringVector m_dimNames;
		bool m_bStandardized;

	};

	//******************************************************************************
	class CKNearestNeighbor;
	typedef std::map<size_t, int> CPredictorMap;
	typedef std::pair<size_t, int> CPredictorPair;
	class CPredictor : public std::deque < float >
	{
	public:

		CPredictor(size_t size = 0) :
			std::deque<float>(size)//,
		{}

		const std::string& GetName()const{ return m_name; }
		void SetName(const std::string& name){ m_name = name; }
		double GetValue(const CSearchResultVector& result, double T)const;

		//Get nb class must be call first
		size_t GetNbClass()const
		{
			if (m_class.empty())
				Init();

			return m_class.size();
		}



		int GetClassIndex(int value)const
		{
			if (m_class.empty())
				Init();

			CPredictorMap::const_iterator it = m_class.find(value);
			return it->second;
		}

		size_t GetClassValue(int index)const
		{
			if (m_class.empty())
				Init();

			ASSERT(index >= 0 && index < (int)m_class.size());

			CPredictorMap::const_iterator it = MapSearchByValue(m_class, index);
			return it->first;
		}


	protected:

		void Reset();
		void Init()const;

		std::string m_name;



		CPredictorMap m_class;
	};


	class CPredictorVector : public std::deque < CPredictor >
	{
	public:
	};

	class CExtraColumn : public std::vector < std::string >
	{
	public:
		std::string m_name;
	};

	typedef std::vector<CExtraColumn> CExtraColumnVector;


	class CKNearestNeighbor
	{
	public:
		CKNearestNeighbor(bool bStandardized);
		~CKNearestNeighbor();

		void Reset();
		void ResetTree();

		ERMsg LoadFromString(const std::string& dataX, const std::string& dataY);

		const CMDPointVector& GetX()const{ return m_X; }
		const CPredictorVector& GetY()const{ return m_Y; }
		void SetSubset(std::vector<int> in){ m_in = in; }
		const std::vector<int>& GetSubset()const{ return m_in; }


		void GetPoints(CMDPointVector& points);

		ERMsg Init()const;
		ERMsg Search(const CMDPoint& pt, long nbPoint, CSearchResultVector& result, double eps = 0.0)const;

		size_t GetNbPredictor(){ return m_Y.size(); }
		size_t GetNbDimension()const{ return m_X.GetNbDimension(); }
		size_t size()const{ return m_X.size(); }
		//const CExtraColumnVector& GetExtraColumns()const{return m_extraColumns;}

		const CMDPoint& operator[](size_t i)const{ return m_X[i]; }
		const CPredictor& operator()(size_t i)const{ return m_Y[i]; }

	private:


		CMDPointVector m_X;
		CPredictorVector m_Y;
		//	CGeoPointVector m_geoCoordinate;
		//CExtraColumnVector m_extraColumns;

		std::vector<int> m_in;
		static std::vector<size_t> GetColumnPos(const StringVector& line, const std::string& prefix, bool bExact = false);
		static std::vector<size_t> GetCoordinate(const StringVector& header);
		static std::vector<size_t> GetExtraColumnsPos(const StringVector& header, const std::vector<int>& XPos, const std::vector<int>& YPos, const std::vector<int> & coordinatePos);

		ANNkd_tree* m_pTreeRoot;
		ANNpointArray m_pDataPts;		// data points
	};

}