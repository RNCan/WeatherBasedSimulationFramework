#include "stdafx.h"


#include "Basic/CSV.h"
#include "Geomatic/GDALBasic.h"
#include "KNearestNeighbor.h"



using namespace std;
using namespace WBSF;


enum TF{FREQUENCY=-1};


CMDPoint::CMDPoint(size_t dim)
{
	m_spectrals.resize(dim);
}
 
CMDPoint::CMDPoint(const CMDPoint& point)
{
	operator = (point);
}

CMDPoint::~CMDPoint()
{}


CMDPoint& CMDPoint::operator = (const CMDPoint& point)
{
	CGeoPoint::operator = (point);
	m_spectrals.operator=(point.m_spectrals);

	return *this;
}
//
//CMDPoint& CMDPoint::Set( ANNpointArray points, long size, long dim, int index)
//{
//	ASSERT( index < size);
//
//	m_dataPts.resize(dim);
//	for(int j=0; j<dim; j++)
//	{
//		m_dataPts[j] = points[index][j];
//	}
//
//	return *this;
//}


//*****************************************************************
CMDPointVector::CMDPointVector(bool bStandardized, INT_PTR dim, INT_PTR size)
{
	m_bStandardized=bStandardized;
	m_stats.resize(dim);
	m_dimNames.resize(dim);
	if( dim>0 && size>0)
		resize(size);
}

CMDPointVector::~CMDPointVector()
{
}

void CMDPointVector::resize(size_t nNewSize)
{
	ASSERT( m_stats.size() > 0);

	_CMDPointVector::resize(nNewSize);

	for(iterator it=begin(); it<end(); it++)
	{
		it->SetNbDimension(GetNbDimension());
	}
}

CMDPointVector& CMDPointVector::operator=(const CMDPointVector& in )
{
	if( &in != this)
	{
		_CMDPointVector::operator=(in);
		m_stats = in.m_stats;
		m_dimNames = in.m_dimNames;
	}

	return *this;
}

void CMDPointVector::push_back(const CMDPoint& newElement)
{
	if(m_stats.size() == 0)
	{
		ASSERT(size() == 0);
		m_stats.resize( newElement.GetNbDimension() );
		m_dimNames.resize( newElement.GetNbDimension() );
	}
	ASSERT( newElement.GetNbDimension() == GetNbDimension() );
	
	_CMDPointVector::push_back(newElement);
	
	for(size_t i=0; i<m_stats.size(); i++)
		m_stats[i] += newElement[i];

}

void CMDPointVector::insert(INT_PTR nIndex, const CMDPoint& newElement )
{
	ASSERT( newElement.GetNbDimension() == GetNbDimension());
	_CMDPointVector::insert(begin()+nIndex, newElement );

	for(size_t i=0; i<m_stats.size(); i++)
		m_stats[i] += newElement[i];
}


void CMDPointVector::InitANNpoint(const CMDPoint& pt, ANNpoint q)const
{
	for (int k = 0; k < GetNbDimension(); k++)
	{
		if (m_Ltrans.nr()>0)//if cca init
		{
			for (int kk = 0; kk < m_Ltrans.nc(); kk++)
				q[kk] += ((pt[k] - m_stats[k][MEAN]) / m_stats[k][STD_DEV])*m_Ltrans(k, kk);
		}
		else if (m_bStandardized)
		{
			q[k] = (pt[k] - m_stats[k][MEAN]) / m_stats[k][STD_DEV];
		}
		else
		{
			q[k] = pt[k];
		}
	}
}

ANNpoint CMDPointVector::GetANNpoint(const CMDPoint& pt)const
{
	ASSERT(pt.GetNbDimension() == GetNbDimension());

	size_t nbDims = m_Ltrans.nc() > 0 ? m_Ltrans.nc() : m_dimNames.size(); // if CCA the dimention reduce to the number of projector
	ANNpoint	q = annAllocPt((int)nbDims);
	InitANNpoint(pt, q);

	return q;
}


ANNpointArray CMDPointVector::GetANNpointArray()const
{
	//const CMDPointVector& me = *this;
	size_t nbDims = m_Ltrans.nc() > 0 ? m_Ltrans.nc() : m_dimNames.size(); // if CCA the dimention reduce to the number of projector
	ANNpointArray ptsArray = annAllocPts((int)size(), (int)nbDims);


	for (int i = 0; i<size(); i++)
	{
		ASSERT(i == 0 || at(i - 1).GetNbDimension() == at(i).GetNbDimension());
		InitANNpoint(at(i), ptsArray[i]);
	}


	return ptsArray;
}

//*******************************************************************
//predictor
double CPredictor::GetValue(const CSearchResultVector& result, double T, bool bGeoWeight)const
{
	double value=0;

	const CPredictor& me = *this;

	if( T>=0)
	{
		double xSum = result.GetXSum(T, bGeoWeight);
		for(size_t k=0; k<result.size(); k++)
		{
			ASSERT( me[result[k].m_index] != FLT_MAX); //it's not no data

			double weight = result[k].GetXTemp(T, bGeoWeight) / xSum;
			value += me[result[k].m_index]*weight;
		}
	}
	else
	{
		//sorting by value
		CPredictorMap index;
		int maxValue=1;

		for(size_t k=0; k<result.size(); k++)
		{
			
			size_t v = Round(me[result[k].m_index]);
			CPredictorMap::iterator it = index.find(v);
			if(it != index.end() )
			{
				it->second++;
				if( it->second>maxValue)
					maxValue=it->second;
			}
			else 
			{
				index.insert( CPredictorPair(v, 1) );
			}
		}

		ASSERT( index.size() > 0);

		int nbMaxValue = 0;
		
		for(CPredictorMap::const_iterator it=index.begin(); it!=index.end(); it++)
			if( it->second == maxValue)
				nbMaxValue++;

		//ici le rand ne sera pas toujours pareil a cause d'OMP
		int bestIndex = Rand(1, nbMaxValue);
				
		for(CPredictorMap::const_iterator it=index.begin(); it!=index.end(); it++)
		{
			if( it->second == maxValue)
				bestIndex--;

			if( bestIndex==0)
			{
				value = it->first;
				break;
			}
		}
	}
	
	return value;
}

void CPredictor::Init()const
{
	CPredictor& me = const_cast<CPredictor&>(*this);
	me.m_stats.Reset();

	//look the number of different value
	for(const_iterator it=begin(); it!=end(); it++)
	{
		ASSERT( *it != FLT_MAX );
		
		size_t v = Round(*it);
		CPredictorMap::iterator it2 = me.m_class.find(v);
		if(it2 == m_class.end() )
		{
			me.m_class.insert( CPredictorPair(v, 0) );
		}
	}
	
	//define class by order
	int i=0;
	for(CPredictorMap::iterator it=me.m_class.begin(); it!=me.m_class.end(); it++, i++)
		it->second = i;
}

void CPredictor::InitStats()const
{
	if (!m_stats.IsInit())
	{
		CPredictor& me = const_cast<CPredictor&>(*this);

		//look the number of different value
		for (const_iterator it = begin(); it != end(); it++)
		{
			ASSERT(*it != FLT_MAX);

			//init stats
			me.m_stats += *it;
		}
	}
}

void CPredictor::Reset()
{
	m_class.clear();
}

//*****************************************************************
//CKNearestNeighbor


CKNearestNeighbor::CKNearestNeighbor(bool bStandardized, bool bUseCCA) :
m_X(bStandardized)
{
	m_pTreeRoot = NULL;
	m_pDataPts = NULL;
	m_bUseCCA = bUseCCA;
}

CKNearestNeighbor::~CKNearestNeighbor()
{
	Reset();
}

void CKNearestNeighbor::Reset()
{
	//ResetTree();
	m_X.clear();
	m_Y.clear();
}

std::vector<int> CKNearestNeighbor::GetColumnsPos(const StringVector& lines, const string& str, bool bExact)
{
	vector<int> posVector;

	for(int i=0; i<lines.size(); i++)
	{
		//string line = lines[i];
		//line.erase(std::remove(line.begin(), line.end(), '"'), line.end());

		if( bExact )
		{
			if (IsEqualNoCase(lines[i], str))
				posVector.push_back(i);
		}
		else
		{
			if (IsEqualNoCase(lines[i].substr(0, str.length()), str) && lines[i].length() > str.length())
				posVector.push_back(i);
		}
	}

	return posVector;
}

vector<int> CKNearestNeighbor::GetCoordinates(const StringVector& header, const string& XHeader, const string& YHeader)
{
	vector<int> coord;

	vector<int> posX = GetColumnsPos(header, XHeader, true);
	vector<int> posY = GetColumnsPos(header, YHeader, true);
	if( posX.size()==1 && posY.size()==1 )
	{
		coord.push_back(posX[0]);
		coord.push_back(posY[0]);
	}
	
	
	return coord;
}


vector<int> CKNearestNeighbor::GetExtraColumnsPos(const StringVector& header, const vector<int>& XPos, const vector<int>& YPos, const vector<int> & coordinatePos)
{
	vector<int> extraPos;

	for(int i=0; i<header.size(); i++)
	{
		if( find( XPos.begin(), XPos.end(), i) == XPos.end() &&
			find( YPos.begin(), YPos.end(), i) == YPos.end() &&
			find( coordinatePos.begin(), coordinatePos.end(), i) == coordinatePos.end() )
		{
			extraPos.push_back(i);
		}
	}

	return extraPos;
}

ERMsg CKNearestNeighbor::LoadCSV(const string& filePath, const string& XHeader, const string& YHeader, const string& Xprefix, const string& Yprefix)
{
	ERMsg msg;

	
	ifStream file;
	msg = file.open(filePath);

	if( msg)
	{
		string prjFilePath(filePath);
		SetFileExtension(prjFilePath, ".prj");

		CProjection prj;
		if (FileExists(prjFilePath))
		{
			msg = prj.Load(prjFilePath);

			if (!msg)
				return msg;
		}

		//m_geoCoordinates.SetPrjID(prj.GetPrjID());
		

		CSVIterator loop(file);
		const StringVector& header = loop.Header();

		m_Xpos = GetColumnsPos(header, Xprefix);
		m_Ypos = GetColumnsPos(header, Yprefix);
		m_geoPos = GetCoordinates(header, XHeader, YHeader);
		

		if( m_Xpos.empty() )
		{
			msg.ajoute("Invalid input file. Doesn't contain X value with prefix: "+ Xprefix);
			msg.ajoute("Verify that your file doesn't contain quotes" + Xprefix);
			return msg;
		}

		if (m_geoPos.size() > 2)
		{
			msg.ajoute("Multiple coordinate (X or Y) values");
			return msg;
		}

	
		m_X.SetNbDimension(m_Xpos.size());
		for(size_t i=0; i<m_Xpos.size(); i++)
			m_X.SetDimensionName( i, header[m_Xpos[i]] );

		m_Y.resize(m_Ypos.size());
		for(size_t i=0; i<m_Ypos.size(); i++)
			m_Y[i].SetName( header[m_Ypos[i]] );
		
		
		m_extraPos = GetExtraColumnsPos(header, m_Xpos, m_Ypos, m_geoPos);
		m_extraColumns.resize(m_extraPos.size());
		for (size_t i = 0; i<m_extraPos.size(); i++)
			m_extraColumns[i].m_name = header[m_extraPos[i]];


		//read all lines of the file
		for (int l = 1; loop != CSVIterator(); ++loop, ++l)
		{
			CMDPoint pt(m_Xpos.size());
			
			for(size_t i=0; i<m_Xpos.size(); i++)
			{
				if (m_Xpos[i] < loop->size())
				{
					if( (*loop)[m_Xpos[i]] != "NA" )
					{
						pt[i] = ToFloat((*loop)[m_Xpos[i]]);
					}
					else 
					{
						msg.ajoute("Invalid CSV file: " + filePath);
						msg.ajoute("Reading error at line : " +ToString(l));
						msg.ajoute("Row with missing values (\"NA\") not allowed.");
						return msg;
					}

				}
				else 
				{
					msg.ajoute("Invalid CSV file: " + filePath);
					msg.ajoute("Reading error at line : " +ToString(l));
					msg.ajoute("Expected number of columns : " + ToString(m_Xpos.size()));
					msg.ajoute("Number of columns find : " + ToString(loop->size()));
					return msg;
				}
			}
			
			if (m_geoPos.size() == 2)
			{
				if (m_geoPos[0] < loop->size() && m_geoPos[1] < loop->size())
				{
					//CGeoPoint coordinate;
					pt.SetPrjID(prj.GetPrjID());
					pt.m_x = ToDouble((*loop)[m_geoPos[0]]);
					pt.m_y = ToDouble((*loop)[m_geoPos[1]]);
					//m_geoCoordinates.push_back(coordinate);
				}
				else
				{
					msg.ajoute("Invalid CSV file: " + filePath);
					msg.ajoute("Reading error at line : " + ToString(l));
					return msg;
				}
			}

			m_X.push_back(pt);


			for(size_t i=0; i<m_Ypos.size(); i++)
			{
				if (m_Ypos[i] < loop->size())
				{
					if( (*loop)[m_Ypos[i]] != "NA" )
					{
						m_Y[i].push_back(ToFloat((*loop)[m_Ypos[i]]));
					}
					else 
					{
						msg.ajoute("Invalid CSV file: " + filePath);
						msg.ajoute("Reading error at line : " +ToString(l));
						msg.ajoute("Row with missing values (\"NA\") not allowed.");
						return msg;
					}
					
				}
				else 
				{
					msg.ajoute("Invalid CSV file: " + filePath);
					msg.ajoute("Reading error at line : " +ToString(l));
					msg.ajoute("Expected number of columns : " + ToString(m_Ypos.size()));
					msg.ajoute("Number of columns find : " +ToString(loop->size()));
					return msg;
				}
			}
			
			for(size_t i=0; i<m_extraPos.size(); i++)
			{
				if (m_extraPos[i] < loop->size())
				{
					m_extraColumns[i].push_back((*loop)[m_extraPos[i]]);
				}
				else 
				{
					msg.ajoute("Invalid CSV file: " + filePath);
					msg.ajoute("Reading error at line : " + ToString(l));
					return msg;
				}
			}
		}
	}

	
	return msg;
}



void CKNearestNeighbor::InitCCA()
{
	//const CKNearestNeighbor& me = *this;

	dlib::matrix<float> L(size(), GetNbDimension());
	dlib::matrix<float> R(size(), GetNbPredictor());

	for (size_t i = 0; i<GetNbDimension(); i++)
	{
		const CStatistic& stats = m_X.GetStatistic(i);

		for (size_t j = 0; j<m_X.size(); j++)
		{
			double e = stats[STD_DEV];
			if (e>0)
				L(j, i) = (m_X[j][i] - stats[MEAN]) / e;
			else
				L(j, i) = 0;
		}
	}

	for (size_t i = 0; i < GetNbPredictor(); i++)
	{
		const CStatistic& stats = m_Y[i].GetStatistic();

		for (size_t j = 0; j<m_Y[i].size(); j++)
		{
			double e = stats[STD_DEV];

			if (e>0)
				R(j, i) = (m_Y[i][j] - stats[MEAN]) / e;
			else
				R(j, i) = 0;
		}
	}
	

	dlib::matrix<float, 0, 1> CCA = dlib::cca(L, R, m_X.m_Ltrans, m_Y.m_Rtrans, GetNbPredictor(), max(size_t(5), GetNbDimension() - GetNbPredictor()));

	//adjust standard deviation to unity
	CStatistic stats;
	for (size_t i = 0; i <L.nr(); i++)
	{
		CStatistic statsTmp;
		for (size_t j = 0; j < L.nc(); j++)
			statsTmp += L(i, j)*m_X.m_Ltrans(j, 0);//take the first

		stats += statsTmp[SUM];
	}

	double sd = stats[STD_DEV];
	m_X.m_Ltrans /= sd;
	m_Y.m_Rtrans /= sd;

	auto factor = diagm(CCA);//create a diagonal matrix from vector R²
	m_X.m_Ltrans = m_X.m_Ltrans * factor;
	
}


void CKNearestNeighbor::Init()
{
	if (m_bUseCCA)
		InitCCA();

	InitANN();
	
}


void CKNearestNeighbor::ResetANN()
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

void CKNearestNeighbor::InitANN()
{
	ResetANN();
	m_pDataPts = m_X.GetANNpointArray();

	size_t nbDims = m_bUseCCA ? GetNbPredictor() : GetNbDimension(); // if CCA the dimention is equal to the number of projector
	m_pTreeRoot = (ANNkd_tree*)new ANNkd_tree(m_pDataPts, (int)m_X.size(), (int)nbDims);
}

ERMsg CKNearestNeighbor::Search(const CMDPoint& pt, long nbPoint, CSearchResultVector& result, double eps)const
{
	ASSERT(IsANNInit());
		
	ERMsg msg;

	//Find point event if they have not enougth points
	long nbPointSearch = min( nbPoint, (long)size());
	ASSERT( nbPointSearch <= size());
	result.clear();
	if( size() > 0)
	{
		ASSERT( pt.GetNbDimension() == GetNbDimension());
		
		result.resize(nbPointSearch);

		ANNidxArray	nn_idx = new ANNidx[nbPointSearch];			// allocate near neigh indices
	    ANNdistArray	dd = new ANNdist[nbPointSearch];			// allocate near neighbor dists
		ANNpoint		 q = m_X.GetANNpoint(pt);
		
		
		m_pTreeRoot->annPkSearch(q, nbPointSearch, nn_idx, dd, eps);
		for(int i=0; i<nbPointSearch; i++)
		{
			result[i].m_index = nn_idx[i];
			dd[i] = sqrt(dd[i]);		// unsquare distance
			result[i].m_distance = dd[i];
			result[i].m_geoDistance = HaveGeoCoordinates() ? pt.GetDistance(m_X[nn_idx[i]]) / 1000 : -9999;
		}
		
		//caller must freed memory
		annDeallocPt(q);
		delete [] nn_idx;
		delete [] dd;

		ASSERT( result.size() == nbPointSearch);
	}

	if( result.size() != nbPoint)
		msg.ajoute("Searching error: not enough points");
	

	return msg;
}
