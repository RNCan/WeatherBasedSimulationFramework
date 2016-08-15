#include "Basic/CSV.h"
#include "KNearestNeighbor.h"



using namespace std;
namespace WBSF
{

	enum TF{ FREQUENCY = -1 };

	int GetK(int i)
	{
		int div = int(i / 29);
		int modulo = i % 29;

		if (modulo >= 24 && modulo <= 26)
			return div * 3 + 0;

		if (modulo >= 18)
			return div * 3 + 2;

		if (modulo >= 10)
			return div * 3 + 1;

		return div * 3 + 0;

		//int div = int(i/29);
		//return div;
	}
	CMDPoint::CMDPoint(INT_PTR dim)
	{
		m_dataPts.resize(dim);
	}

	CMDPoint::CMDPoint(const CMDPoint& point)
	{
		operator = (point);
	}

	CMDPoint::~CMDPoint()
	{}


	CMDPoint& CMDPoint::operator = (const CMDPoint& point)
	{
		m_dataPts.operator=(point.m_dataPts);

		return *this;
	}

	CMDPoint& CMDPoint::Set(ANNpointArray points, long size, long dim, int index)
	{
		ASSERT(index < size);

		m_dataPts.resize(dim);
		for (int j = 0; j < dim; j++)
		{
			m_dataPts[j] = points[index][j];
		}

		return *this;
	}


	//*****************************************************************
	CMDPointVector::CMDPointVector(bool bStandardized, INT_PTR dim, INT_PTR size)
	{
		m_bStandardized = bStandardized;
		//	m_dim = dim;
		m_dimStats.resize(dim);
		m_dimNames.resize(dim);
		if (dim>0 && size>0)
			resize(size);
	}

	CMDPointVector::~CMDPointVector()
	{
	}

	void CMDPointVector::resize(size_t nNewSize)
	{
		ASSERT(m_dimStats.size() > 0);

		//INT_PTR nOldSize = _CMDPointVector::size();
		//_CMDPointVector::reserve(nGrowBy);
		_CMDPointVector::resize(nNewSize);

		for (iterator it = begin(); it < end(); it++)
		{
			it->SetNbDimension(GetNbDimension());
		}
	}

	CMDPointVector& CMDPointVector::operator=(const CMDPointVector& in)
	{
		if (&in != this)
		{
			_CMDPointVector::operator=(in);
			m_dimStats = in.m_dimStats;
			m_dimNames = in.m_dimNames;
		}

		return *this;
	}

	void CMDPointVector::push_back(const CMDPoint& newElement)
	{
		if (m_dimStats.size() == 0)
		{
			ASSERT(size() == 0);
			m_dimStats.resize(newElement.GetNbDimension());
			m_dimNames.resize(newElement.GetNbDimension());
		}
		ASSERT(newElement.GetNbDimension() == GetNbDimension());

		_CMDPointVector::push_back(newElement);

		for (size_t i = 0; i < m_dimStats.size(); i++)
			m_dimStats[i] += newElement[i];

	}

	void CMDPointVector::insert(INT_PTR nIndex, const CMDPoint& newElement)
	{
		ASSERT(newElement.GetNbDimension() == GetNbDimension());
		_CMDPointVector::insert(begin() + nIndex, newElement);

		for (size_t i = 0; i < m_dimStats.size(); i++)
			m_dimStats[i] += newElement[i];
	}


	ANNpointArray CMDPointVector::GetANNpointArray(const vector<int>& in)const
	{
		const CMDPointVector& me = *this;


		ANNpointArray ptsArray = annAllocPts((int)size(), (int)in.size());
		for (size_t i = 0; i < size(); i++)
		{
			ASSERT(i == 0 || at(i - 1).GetNbDimension() == at(i).GetNbDimension());

			for (size_t j = 0; j < in.size(); j++)
			{
				ASSERT(in[j] < (int)at(i).GetNbDimension());
				if (m_bStandardized)
				{
					double e = m_dimStats[in[j]][STD_DEV];
					if (e>0)
						ptsArray[i][j] = (me[i][in[j]] - m_dimStats[in[j]][MEAN]) / e;
					else ptsArray[i][j] = 0;
				}
				else
				{

					ptsArray[i][j] = me[i][in[j]];
				}
			}
		}

		//ANNpointArray ptsArray = annAllocPts((int)size(), (int)198);
		//for(size_t i=0; i<size(); i++)
		//{
		//	ASSERT( i==0 || at(i-1).GetNbDimension() == at(i).GetNbDimension());
		//
		//	for(size_t j=0; j<in.size(); j++)
		//		ptsArray[i][GetK(in[j])]=0;
		//	
		//	
		//	for(size_t j=0; j<in.size(); j++)
		//	{
		//		ASSERT( in[j] < (int)at(i).GetNbDimension() );
		//		int k = in[j];

		//		if( m_bStandardized )
		//		{
		//			double e = m_dimStats[k][STD_DEV];
		//			if( e>0)
		//				ptsArray[i][GetK(in[j])] += (me[i][k]-m_dimStats[k][MEAN])/e;
		//			
		//		}
		//		else
		//		{
		//			ptsArray[i][GetK(in[j])] += me[i][k];
		//		}
		//	}
		//}

		return ptsArray;
	}


	ANNpoint CMDPointVector::GetANNpoint(const CMDPoint& pt, const vector<int>& in)const
	{
		ASSERT(pt.GetNbDimension() == GetNbDimension());

		ANNpoint	q = annAllocPt((int)in.size());
		for (size_t i = 0; i < in.size(); i++)
		{
			int k = in[i];
			if (m_bStandardized)
			{
				ASSERT(abs(sqrt(m_dimStats[k][VARIANCE]) - m_dimStats[k][STD_DEV]) < 0.000001);
				q[i] = (pt[k] - m_dimStats[k][MEAN]) / m_dimStats[k][STD_DEV];
			}
			else
			{
				q[i] = pt[k];
			}
		}
		/*
			ANNpoint	q = annAllocPt(198);
			for(size_t i=0; i<in.size(); i++)
			q[GetK(in[i])]=0;

			for(size_t i=0; i<in.size(); i++)
			{
			int k = in[i];
			if( m_bStandardized )
			{
			ASSERT( abs( sqrt(m_dimStats[k][VARIANCE]) - m_dimStats[k][STD_DEV]) < 0.000001);
			q[GetK(in[i])] += (pt[k]-m_dimStats[k][MEAN])/m_dimStats[k][STD_DEV];
			}
			else
			{
			q[GetK(in[i])] += pt[k];
			}
			}
			*/
		return q;
	}

	void CMDPointVector::SetANNpointArray(ANNpointArray points, long size, long dim)
	{
		m_dimStats.clear();
		m_dimStats.resize(dim);
		m_dimNames.resize(dim);
		resize(size);

		for (int i = 0; i < size; i++)
		{
			ASSERT(i == 0 || at(i - 1).GetNbDimension() == at(i).GetNbDimension());
			for (int j = 0; j < dim; j++)
			{
				at(i)[j] = points[i][j];
				m_dimStats[j] += points[i][j];
			}
		}

	}
	//*******************************************************************
	//predictor
	double CPredictor::GetValue(const CSearchResultVector& result, double T)const
	{
		double value = 0;

		const CPredictor& me = *this;

		if (T >= 0)
		{
			double xSum = result.GetXSum(T);
			for (size_t k = 0; k < result.size(); k++)
			{
				ASSERT(me[result[k].m_index] != FLT_MAX); //it's not no data

				double weight = result[k].GetXTemp(T) / xSum;
				value += me[result[k].m_index] * weight;
			}
		}
		else
		{
			//sorting by value
			CPredictorMap index;
			int maxValue = 1;

			for (size_t k = 0; k < result.size(); k++)
			{

				size_t v = Round(me[result[k].m_index]);
				CPredictorMap::iterator it = index.find(v);
				if (it != index.end())
				{
					it->second++;
					if (it->second > maxValue)
						maxValue = it->second;
				}
				else
				{
					index.insert(CPredictorPair(v, 1));
				}
			}

			ASSERT(index.size() > 0);

			//CPredictorMap index2 = converse_map(index);
			//sort(index2.begin(), index2.end());

			int nbMaxValue = 0;

			//for(CPredictorMap::const_reverse_iterator it=index2.rbegin(); it!=index2.rend(); it++)
			//if( it->first ==  index2.rbegin()->first)
			for (CPredictorMap::const_iterator it = index.begin(); it != index.end(); it++)
				if (it->second == maxValue)
					nbMaxValue++;

			//ici le rand ne sera pas toujours pareil a cause d'OMP
			size_t bestIndex = Rand(1, nbMaxValue);

			for (CPredictorMap::const_iterator it = index.begin(); it != index.end(); it++)
			{
				if (it->second == maxValue)
					bestIndex--;

				if (bestIndex == 0)
				{
					value = (double)it->first;
					break;
				}
			}
		}

		return value;
	}

	void CPredictor::Init()const
	{
		CPredictor& me = const_cast<CPredictor&>(*this);
		//look the number of different value
		for (const_iterator it = begin(); it != end(); it++)//, j++
		{
			ASSERT(*it != FLT_MAX);

			size_t v = Round(*it);
			CPredictorMap::iterator it2 = me.m_class.find(v);
			if (it2 == m_class.end())
			{
				me.m_class.insert(CPredictorPair(v, 0));
			}
		}

		//define class by order
		int i = 0;
		for (CPredictorMap::iterator it = me.m_class.begin(); it != me.m_class.end(); it++, i++)
			it->second = i;
	}

	void CPredictor::Reset()
	{
		m_class.clear();
		//	m_posix.clear();
	}

	//*****************************************************************
	//CKNearestNeighbor

	CKNearestNeighbor::CKNearestNeighbor(bool bStandardized) :
		m_X(bStandardized)
	{
		m_pTreeRoot = NULL;
		m_pDataPts = NULL;
	}

	CKNearestNeighbor::~CKNearestNeighbor()
	{
		Reset();
	}

	void CKNearestNeighbor::Reset()
	{
		ResetTree();
		m_X.clear();
		m_Y.clear();
	}


	vector<size_t> CKNearestNeighbor::GetColumnPos(const StringVector& line, const string& str, bool bExact)
	{
		vector<size_t> posVector;

		for (size_t i = 0; i < line.size(); i++)
		{
			if (bExact)
			{
				if (IsEqualNoCase(line[i], str))
					posVector.push_back(i);
			}
			else
			{
				if (IsEqualNoCase(line[i].substr(str.length()), str) && line[i].length() > str.length())
					posVector.push_back(i);
			}
		}

		return posVector;
	}

	vector<size_t> CKNearestNeighbor::GetCoordinate(const StringVector& header)
	{
		vector<size_t> coord;

		vector<size_t> posX = GetColumnPos(header, "X", true);
		vector<size_t> posY = GetColumnPos(header, "Y", true);
		if (posX.size() == 1 && posY.size() == 1)
		{
			coord.push_back(posX[0]);
			coord.push_back(posY[0]);
		}


		return coord;
	}


	vector<size_t> CKNearestNeighbor::GetExtraColumnsPos(const StringVector& header, const vector<int>& XPos, const vector<int>& YPos, const vector<int>& coordinatePos)
	{
		vector<size_t> extraPos;

		for (size_t i = 0; i < header.size(); i++)
		{
			if (find(XPos.begin(), XPos.end(), i) == XPos.end() &&
				find(YPos.begin(), YPos.end(), i) == YPos.end() &&
				find(coordinatePos.begin(), coordinatePos.end(), i) == coordinatePos.end())
			{
				extraPos.push_back(i);
			}
		}

		return extraPos;
	}

	vector<string> GetSpeciesList(const string& dataX)
	{
		vector<string> species;
		species.reserve(200);

		std::stringstream tmp(dataX);
		StringVector test;

		set<size_t> speciesIDpos = CSVIterator(tmp).Header().FindAll("SpeciesID", true, true);
		if (speciesIDpos.size() == 1)
		{
			tmp.seekg(0);

			string firstSpecies;

			for (CSVIterator it(tmp); it != CSVIterator() && (*it)[*speciesIDpos.begin()] != firstSpecies; ++it)
			{
				if (firstSpecies.empty())
					firstSpecies = (*it)[*speciesIDpos.begin()];

				species.push_back((*it)[*speciesIDpos.begin()]);
			}
		}

		return species;
	}

	ERMsg CKNearestNeighbor::LoadFromString(const string& dataX, const string& dataY)
	{
		ERMsg msg;

		vector<string> species = GetSpeciesList(dataX);

		if (species.empty())
		{
			msg.ajoute("Invalid input file. Need to have one column named SpeciesID");
			return msg;
		}

		std::stringstream streamX(dataX);
		std::stringstream streamY(dataY);


		CSVIterator itX(streamX);
		CSVIterator itY(streamY);


		CSVRow const& headerX = itX.Header();
		CSVRow const& headerY = itY.Header();

		set<size_t> Xset = headerX.FindAll("X_");
		set<size_t> Yset = headerY.FindAll("Y_");

		vector<size_t> Xpos(Xset.begin(), Xset.end());
		vector<size_t> Ypos(Yset.begin(), Yset.end());
		vector<size_t> coordinatePos;


		if (Xpos.empty())
		{
			msg.ajoute("Invalid input file. Doesn't contain X value with prefix: X_");
			return msg;
		}


		m_X.SetNbDimension(Xpos.size()*species.size());
		for (size_t s = 0; s < species.size(); s++)
			for (size_t i = 0; i < Xpos.size(); i++)
				m_X.SetDimensionName(s*Xpos.size() + i, headerX[Xpos[i]] + "_" + species[s]);

		m_Y.resize(Ypos.size());
		for (size_t i = 0; i < Ypos.size(); i++)
			m_Y[i].SetName(headerY[Ypos[i]]);


		for (int x = 0; itX != CSVIterator(); x++)
		{
			m_X.resize(m_X.size() + 1);
			m_X[x].SetNbDimension(Xpos.size()*species.size());
			for (size_t s = 0; s < species.size(); s++, ++itX)
			{
				ASSERT(itX != CSVIterator());
				CSVRow const& line = *itX;

				for (size_t i = 0; i < Xpos.size(); i++)
				{
					if (Xpos[i] < (int)line.size())
					{
						if (line[Xpos[i]] != "NA")
						{
							m_X[x][s*Xpos.size() + i] = ToFloat(line[Xpos[i]]);
						}
						else
						{
							msg.ajoute("Reading error at line : " + ToString(i + 1));
							msg.ajoute("Row with missing values (\"NA\") not allowed.");
							return msg;
						}
					}
					else
					{
						msg.ajoute("Reading error at line : " + ToString(i + 1));
						msg.ajoute("Expected number of columns : " + ToString(Ypos.size()));
						msg.ajoute("Number of columns find : " + ToString(line.size()));
						return msg;
					}
				}
			}
		}


		//for(size_t i=0; i<Ypos.size(); i++)
		for (; itY != CSVIterator(); ++itY)
		{
			CSVRow const& line = *itY;
			for (size_t i = 0; i < Ypos.size(); i++)
			{
				if (Ypos[i] < (int)line.size())
				{
					if (line[Ypos[i]] != "NA")
					{
						m_Y[i].push_back(ToFloat(line[Ypos[i]]));
					}
					else
					{
						msg.ajoute("Reading error at line : " + ToString(i + 1));
						msg.ajoute("Row with missing values (\"NA\") not allowed.");
						return msg;
					}

				}
				else
				{
					msg.ajoute("Reading error at line : " + ToString(i + 1));
					msg.ajoute("Expected number of columns : " + ToString(Ypos.size()));
					msg.ajoute("Number of columns find : " + ToString(line.size()));
					return msg;
				}
			}
		}


		return msg;
	}

	void CKNearestNeighbor::ResetTree()
	{
		if (m_pDataPts)
		{
			annDeallocPts(m_pDataPts);
			m_pDataPts = NULL;
		}

		if (m_pTreeRoot)
		{
			delete m_pTreeRoot;
			m_pTreeRoot = NULL;
			annClose();//delete global variable
		}
	}



	ERMsg CKNearestNeighbor::Init()const
	{
		CKNearestNeighbor& me = const_cast<CKNearestNeighbor&>(*this);

		ERMsg msg;

		try
		{
			if (m_in.empty())
			{
				//no subset, take all dimensions
				for (size_t i = 0; i < GetNbDimension(); i++)
					me.m_in.push_back((int)i);
			}

			me.ResetTree();
			me.m_pDataPts = m_X.GetANNpointArray(m_in);
			me.m_pTreeRoot = (ANNkd_tree*)new ANNkd_tree(m_pDataPts, (int)size(), (int)m_in.size());
			//me.m_pTreeRoot = (ANNkd_tree*)new ANNkd_tree(m_pDataPts, (int)size(), (int)198 );

		}
		catch (...)
		{
			msg.ajoute("Unable to initialize KNN. Not enough memory.");
		}

		return msg;
	}



	ERMsg CKNearestNeighbor::Search(const CMDPoint& pt, long nbPoint, CSearchResultVector& result, double eps)const
	{
		ERMsg msg;

		if (m_pTreeRoot == NULL)
		{
			msg = Init();
			if (!msg)
				return msg;
		}




		//Find point event if they have not enough points
		int nbPointSearch = (int)min(nbPoint, (long)size());
		ASSERT(nbPointSearch <= (int)size());
		result.clear();
		if (size() > 0)
		{

			ASSERT(pt.GetNbDimension() == GetNbDimension());
			result.resize(nbPointSearch);

			try
			{
				ANNidxArray	nn_idx = new ANNidx[nbPointSearch];			// allocate near neighbor indices
				ANNdistArray	dd = new ANNdist[nbPointSearch];		// allocate near neighbor distance
				ANNpoint q = m_X.GetANNpoint(pt, m_in);


				m_pTreeRoot->annPkSearch(q, nbPointSearch, nn_idx, dd, eps);
				for (int i = 0; i < nbPointSearch; i++)
				{
					result[i].m_index = nn_idx[i];
					dd[i] = sqrt(dd[i]);		// un-squared distance
					result[i].m_distance = dd[i];
				}

				//caller must free memory
				annDeallocPt(q);
				delete[] nn_idx;
				delete[] dd;

				ASSERT(result.size() == nbPointSearch);
			}
			catch (...)
			{
				msg.ajoute("Exception in ANN search");
			}
		}

		if (result.size() != nbPoint)
			msg.ajoute("Searching error: not enough points");


		return msg;
	}
}