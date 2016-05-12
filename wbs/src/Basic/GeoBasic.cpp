//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"

#include <iostream>

#include "Basic/GeoBasic.h"
#include "Basic/UtilMath.h"
#include "Basic/Statistic.h"


using namespace std;

namespace WBSF
{
	
	CGeoRectIndex& CGeoRectIndex::IntersectRect(const CGeoRectIndex& rect)
	{
		if (&rect != this)
		{
			CGeoRectIndex index;
			index.m_x = max(m_x, rect.m_x);
			index.m_y = max(m_y, rect.m_y);
			index.m_xSize = min(m_x + m_xSize, rect.m_x + rect.m_xSize) - m_x;
			index.m_ySize = min(m_y + m_ySize, rect.m_y + rect.m_ySize) - m_y;

			operator=(index);
		}

		return *this;

	}

	CGeoRectIndex& CGeoRectIndex::UnionRect(const CGeoRectIndex& rect)
	{
	
		if (&rect != this)
		{
			CGeoRectIndex index;
			index.m_x = min(m_x, rect.m_x);
			index.m_y = min(m_y, rect.m_y);
			index.m_xSize = max(m_x + m_xSize, rect.m_x + rect.m_xSize) - m_x;
			index.m_ySize = max(m_y + m_ySize, rect.m_y + rect.m_ySize) - m_y;

			operator=(index);
		}

		return *this;
	}


	void CGeoRectIndex::InflateRect( int w, int n, int e, int s )
	{
		m_x -= w;
		m_y -= s;
		m_ySize += e;
		m_ySize += n;
	}

//************************************************************
	CGeoRect& CGeoRect::IntersectRect(const CGeoRect& rect)
	{
		if (&rect != this && rect.IsInit())
		{
			if (IsInit() )
			{
				ASSERT(m_xMin <= m_xMax);
				ASSERT(m_yMin <= m_yMax);
				ASSERT(rect.m_xMin <= rect.m_xMax);
				ASSERT(rect.m_yMin <= rect.m_yMax);
				ASSERT_PRJ(*this, rect);


				CGeoRect tmp;
				tmp.m_prjID = rect.m_prjID;
				tmp.m_xMin = max(m_xMin, rect.m_xMin);
				tmp.m_xMax = min(m_xMax, rect.m_xMax);
				tmp.m_yMin = max(m_yMin, rect.m_yMin);
				tmp.m_yMax = min(m_yMax, rect.m_yMax);

				if ((tmp.m_xMin > tmp.m_xMax) || (tmp.m_yMin > tmp.m_yMax))
					tmp.SetRectEmpty();

				operator=(tmp);
			}
			else 
			{
				operator=(rect);
			}
		}
		


		return *this;

	}

	

	CGeoRect& CGeoRect::UnionRect(const CGeoRect& rect)
	{
		if (&rect != this && rect.IsInit())
		{
			if (IsInit())
			{
				ASSERT(m_xMin <= m_xMax);
				ASSERT(m_yMin <= m_yMax);
				ASSERT(rect.m_xMin <= rect.m_xMax);
				ASSERT(rect.m_yMin <= rect.m_yMax);
				ASSERT_PRJ(*this, rect);


				CGeoRect tmp;
				tmp.m_prjID = rect.m_prjID;
				tmp.m_xMin = min(m_xMin, rect.m_xMin);
				tmp.m_xMax = max(m_xMax, rect.m_xMax);
				tmp.m_yMin = min(m_yMin, rect.m_yMin);
				tmp.m_yMax = max(m_yMax, rect.m_yMax);

				if ((tmp.m_xMin > tmp.m_xMax) || (tmp.m_yMin > tmp.m_yMax))
					tmp.SetRectEmpty();

				operator=(tmp);
			}
			else
			{
				operator=(rect);
			}
		}

		return *this;
	}

	CGeoRect& CGeoRect::InflateRect( double w, double n, double e, double s )
	{
		m_xMin -= w;
		m_xMax += e;
		m_yMin -= s;
		m_yMax += n;

		return *this;
	}

	CGeoRect& CGeoRect::ExtendBounds(const CGeoPoint& pt)
	{
		if( !IsInit() )
		{
			m_xMin = 1.0E30;
			m_yMin = 1.0E30;
			m_xMax = -1.0E30;
			m_yMax = -1.0E30;
			m_prjID = pt.GetPrjID();
		}

		ASSERT_PRJ( (*this), pt );
		if( m_xMin > pt.m_x )
			m_xMin = pt.m_x;

		if( m_yMin > pt.m_y )
			m_yMin = pt.m_y;

		if( m_xMax < pt.m_x )
			m_xMax = pt.m_x;
    
		if( m_yMax < pt.m_y )
			m_yMax = pt.m_y;
    
		return *this;
	}

	CGeoRect& CGeoRect::ExtendBounds(const CGeoRect& box)
	{
		if( !IsInit() )
		{
			m_xMin = 1.0E30;
			m_yMin = 1.0E30;
			m_xMax = -1.0E30;
			m_yMax = -1.0E30;
			m_prjID = box.GetPrjID();
		}

		ASSERT_PRJ( (*this), box );
		if( m_xMin > box.m_xMin )
			m_xMin = box.m_xMin;

		if( m_yMin > box.m_yMin )
			m_yMin = box.m_yMin;

		if( m_xMax < box.m_xMax )
			m_xMax = box.m_xMax;
    
		if( m_yMax < box.m_yMax )
			m_yMax = box.m_yMax;
 
		return *this;
	}


	CGeoRect&  CGeoRect::OffsetRect(const CGeoPoint& pt)
	{
		ASSERT_PRJ( (*this), pt );
		m_xMin += pt.m_x;
		m_yMax += pt.m_y;
		m_xMax += pt.m_x;
		m_yMin += pt.m_y;

		return *this;
	}

	
CGeoPoint CGeoExtents::XYPosToCoord(const CGeoPointIndex& xy)const
{
	CGeoPoint pt(GetPrjID());
	pt.m_x = m_xMin + (xy.m_x+0.5)/m_xSize*(m_xMax-m_xMin);
    pt.m_y = m_yMax - (xy.m_y+0.5)/m_ySize*(m_yMax-m_yMin);
	
	return pt;
}

CGeoPointIndex CGeoExtents::CoordToXYPos(const CGeoPoint& pt)const
{
	ASSERT( TrunkLowest( 0.99999999999) ==  1);
	ASSERT( TrunkLowest( 0.00000000001) ==  0);
	ASSERT( TrunkLowest(-0.00000000001) ==  0);
	ASSERT( TrunkLowest(-0.50000000000) == -1);
	ASSERT( TrunkLowest(-0.99999999999) == -1);
	ASSERT( TrunkLowest(-1.00000000001) == -1);

	CGeoPointIndex xy;
	
	xy.m_x = TrunkLowest((pt.m_x-m_xMin)/(m_xMax-m_xMin)*m_xSize);
    xy.m_y = TrunkLowest((m_yMax-pt.m_y)/(m_yMax-m_yMin)*m_ySize);

	return xy;
}

void CGeoExtents::NormalizeRect()
{
	CGeoRect::NormalizeRect();
	m_xBlockSize = (int)min( m_xBlockSize, m_xSize);
	m_yBlockSize = (int)min( m_yBlockSize, m_ySize);
}

std::vector<std::pair<int, int>> CGeoExtents::GetBlockList(size_t max_cons_row, size_t max_cons_col)
{
	std::vector<std::pair<int, int>> XYindex;

	if (max_cons_row == NOT_INIT || max_cons_row>YNbBlocks())
		max_cons_row = YNbBlocks();
	
	if (max_cons_col == NOT_INIT || max_cons_col>XNbBlocks())
		max_cons_col = XNbBlocks();
	
	int nbYpass = ceil(YNbBlocks() / max_cons_row);
	int nbXpass = ceil(XNbBlocks() / max_cons_col);
	
	for (int yPass = 0; yPass<nbYpass; yPass++)
		for (int xPass = 0; xPass<nbXpass; xPass++)
			for (int yBlock = 0; yBlock<max_cons_row; yBlock++)
				for (int xBlock = 0; xBlock < max_cons_col; xBlock++)
				{
					int y = int(yPass*max_cons_row) + yBlock;
					int x = int(xPass*max_cons_col) + xBlock;
					if (y<YNbBlocks() && x<XNbBlocks())
						XYindex.push_back(std::pair<int, int>(x, y));
				}
					

	return 	XYindex;
}

void CGeoExtents::GetGeoTransform(CGeoTransform& GT)const
{
	GT[GT_X_LEFT] = m_xMin;
	GT[GT_Y_TOP] = m_yMax;
	GT[GT_CELLSIZE_X] = XRes();
	GT[GT_CELLSIZE_Y] = YRes();
	GT[GT_X_ROTATION] = 0;
	GT[GT_Y_ROTATION] = 0;

}

void CGeoExtents::SetGeoTransform(const CGeoTransform& GT, int Xsize, int Ysize)
{
	m_xSize = Xsize;
	m_ySize = Ysize;
	m_xMin = GT[GT_X_LEFT];
	m_xMax = GT[GT_X_LEFT]+Xsize*GT[GT_CELLSIZE_X];
	m_yMin = GT[GT_Y_TOP]+Ysize*GT[GT_CELLSIZE_Y];
	m_yMax = GT[GT_Y_TOP];

	NormalizeRect();
}

CGeoExtents& CGeoExtents::IntersectExtents(const CGeoExtents& in, int typeRes)
{
	ASSERT(typeRes>=0 && typeRes<NB_TYPE_RES);
	if( &in != this)
	{
		if (IsInit() || in.IsInit())
		{
			const static short STAT_TYPE[NB_TYPE_RES] = { LOWEST, MEAN, HIGHEST };
			CStatistic xRes;
			CStatistic yRes;
			CStatistic xBlock;
			CStatistic yBlock;

			if (IsInit())
			{
				xRes += XRes();
				xBlock += m_xBlockSize;
			}
			if (in.IsInit())
			{
				xRes += in.XRes();
				xBlock += in.m_xBlockSize;
			}

			if (IsInit())
			{
				yRes += YRes();
				yBlock += m_yBlockSize;
			}
			if (in.IsInit())
			{
				yRes += in.YRes();
				yBlock += in.m_yBlockSize;
			}

			IntersectRect(in);
			SetXRes(xRes[STAT_TYPE[typeRes]]);
			SetYRes(-yRes[STAT_TYPE[typeRes]]);

			m_xBlockSize = int(xBlock[STAT_TYPE[typeRes]]);
			m_yBlockSize = int(yBlock[STAT_TYPE[typeRes]]);
		}
	}

	return *this;
}


CGeoExtents& CGeoExtents::UnionExtents(const CGeoExtents& in, int typeRes)
{
	ASSERT(typeRes>=0 && typeRes<NB_TYPE_RES);
	if( &in != this)
	{
		if (IsInit() || in.IsInit() )
		{
			const static short STAT_TYPE[NB_TYPE_RES] = { LOWEST, MEAN, HIGHEST };
			CStatistic xRes;
			CStatistic yRes;
			CStatistic xBlock;
			CStatistic yBlock;

			if (IsInit() )
			{
				xRes += XRes();
				xBlock += m_xBlockSize;
				yRes += YRes();
				yBlock += m_yBlockSize;
			}
			if (in.IsInit())
			{ 
				xRes += in.XRes();
				xBlock += in.m_xBlockSize;
				yRes += in.YRes();
				yBlock += in.m_yBlockSize;
			}

			UnionRect(in);
			SetXRes(xRes[STAT_TYPE[typeRes]]);
			SetYRes(yRes[STAT_TYPE[typeRes]]);

			m_xBlockSize = int(xBlock[MEAN]);
			m_yBlockSize = int(yBlock[MEAN]);
		}
	}

	return *this;
}


CGeoRect CGeoExtents::XYPosToCoord(const CGeoRectIndex& rect)const
{
	CGeoRect geoRect(GetPrjID());

	geoRect.m_xMin = m_xMin + rect.m_x*XRes();
	geoRect.m_xMax = m_xMin + (rect.m_x+rect.Width())*XRes();
	geoRect.m_yMax = m_yMax + rect.m_y*YRes();
	geoRect.m_yMin = m_yMax + (rect.m_y + rect.Height())*YRes();

	return geoRect;
}

CGeoRectIndex CGeoExtents::CoordToXYPos(const CGeoRect& geoRect)const
{
	CGeoPointIndex pt1 = CoordToXYPos(geoRect.UpperLeft());
	CGeoPointIndex pt2 = CoordToXYPos(geoRect.LowerRight());

	return CGeoRectIndex(pt1, pt1.GetDistance(pt2) );
}

CGeoExtents CGeoExtents::GetPixelExtents(CGeoPointIndex xy)const
{
	return CGeoExtents( m_xMin + xy.m_x*XRes(), m_yMax + xy.m_y*YRes(), m_xMin + (xy.m_x+1)*XRes(), m_yMax + (xy.m_y+1)*YRes(), 1,1,1,1,GetPrjID());
}


void CGeoExtents::AlignTo(const CGeoExtents& rect)
{
	if( !IsRectEmpty() )
	{
		
		double floorX = floor(TrunkLowest(((m_xMin - rect.m_xMin) / fabs(rect.XRes()))));
		double floorY = floor(TrunkLowest(((m_yMin - rect.m_yMin) / fabs(rect.YRes()))));
		double ceilX = ceil(TrunkLowest(((m_xMax - rect.m_xMin) / fabs(rect.XRes()))) + 0.5);
		double ceilY = ceil(TrunkLowest(((m_yMax - rect.m_yMin) / fabs(rect.YRes()))) + 0.5);

		double deltaXmin = floorX*fabs(rect.XRes())+rect.m_xMin - m_xMin;
		double deltaYmin = floorY*fabs(rect.YRes())+rect.m_yMin - m_yMin;
		double deltaXmax = ceilX*fabs(rect.XRes())+rect.m_xMin - m_xMax;
		double deltaYmax = ceilY*fabs(rect.YRes())+rect.m_yMin - m_yMax;

		m_xMin += deltaXmin;
		m_yMin += deltaYmin;
		m_xMax += deltaXmax;
		m_yMax += deltaYmax;
	}
}

typedef std::pair<double, CGeoPointIndex>	CDistancePoint;
typedef vector<CDistancePoint> CCDistancePointVector;
static  bool cmp_less_first (CDistancePoint i, CDistancePoint j) { return i.first < j.first; }

void CGeoExtents::GetNearestCellPosition(const CGeoPoint& pt, int nbPoint, CGeoPointVector& ptArray)
{
	ptArray.clear();

	CGeoPointIndexVector ptArrayTmp;
	GetNearestCellPosition(pt, nbPoint, ptArrayTmp);

	ptArray.resize( ptArrayTmp.size() );
	for(size_t i=0; i<ptArrayTmp.size(); i++)
	{
		ptArray[i] = XYPosToCoord(ptArrayTmp[i]);
	}
}


void CGeoExtents::GetNearestCellPosition(const CGeoPointIndex& xy, int nbPoint, CGeoPointIndexVector& ptArray)
{
	CGeoPoint pt = XYPosToCoord(xy);//, true
	GetNearestCellPosition(pt, nbPoint, ptArray);
}

void CGeoExtents::GetNearestCellPosition(const CGeoPoint& pt, int nbPoint, CGeoPointIndexVector& ptArray)
{
    ASSERT( nbPoint > 0);
	ASSERT(IsInside(pt));

	ptArray.clear();
    
	CGeoPointIndex xy = CoordToXYPos(pt);//, true
	ASSERT(IsInside(xy));
	
	int deltaIndex = (int)(sqrt(double(nbPoint-1)));
	
	CCDistancePointVector sortedList;
    
	for(int x=-deltaIndex; x<=deltaIndex; x++)
	{
		for(int y=-deltaIndex; y<=deltaIndex; y++)
		{
			CGeoPointIndex xy_i = xy + CGeoPointIndex(x,y);
			if( IsInside(xy_i ) )
			{
				CGeoPoint tmpPt = XYPosToCoord( xy_i);
			
				double dist = pt.GetDistance(tmpPt);
				CDistancePoint elem(dist, xy_i);
				sortedList.push_back( elem );
			}
		}
	}       
    
	sort(sortedList.begin(), sortedList.end(), cmp_less_first );

	ASSERT( (int)sortedList.size() >= nbPoint );
	for(int i=0; i<nbPoint; i++)
		ptArray.push_back(sortedList[i].second);


	ASSERT( ptArray.size() == nbPoint);
}


	//***********************************************************************************************************
	
	double CGridPoint::GetExposition()const
	{
		//slope must be in percent
		return WBSF::GetExposition(m_latitudeDeg, m_slope, m_aspect);
	}

	void CGridPoint::SetExposition(double latDeg, double expo)
	{
		m_latitudeDeg=latDeg;
		ComputeSlopeAndAspect(latDeg, expo, m_slope, m_aspect);
	}

	std::ostream& CGridPointVector::operator >> (std::ostream &s)const
	{
		
		unsigned __int64 si = size();
		s << si;
		s.write(const_cast<char*>( (char*)data() ), size()*sizeof(CGridPoint));

		return s;
	}

	std::istream& CGridPointVector::operator<<(std::istream &s)
	{
		unsigned __int64 si=0;
		s >> si;
		resize((size_t)si);
		s.read((char*)data(), size()*sizeof(CGridPoint));
		
		return s;
	}

	ERMsg BuildVRT(string filePath, StringVector fileList, bool bQuiet)
	{
		ERMsg msg;

		//save list;
		if (!bQuiet)
			cout << endl << "Build VRT..." << endl;

		string listFilePath(filePath);
		SetFileExtension(listFilePath, "_list.txt");
		ofStream file;
		msg = file.open(listFilePath);
		if (msg)
		{
			//create output images
			for (size_t z = 0; z < fileList.size(); z++)
				file << fileList[z] << endl;

			file.close();
		}

		string command = "GDALBuildVRT.exe -separate -overwrite -input_file_list \"" + listFilePath + "\" \"" + filePath + "\"";
		msg = WinExecWait(command);
		return msg;
	}

}//GeoBasic

