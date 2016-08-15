//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>

#include "Geomatic/ShapeFileBase.h"
#include "Geomatic/ShapeFileIndex.h"
#include "Geomatic/ProjectionTransformation.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
namespace WBSF
{


//**************************************************************
CSFShape::CSFShape()
{
    m_shapeType = CShapeFileHeader::SHAPE_NULL;
}


CSFShape::~CSFShape()
{}

__int32 CSFShape::GetLength()const
{   
    return 2;
}

__int32 CSFShape::GetTypeNumber()const
{
    return m_shapeType;
}

CSFShape* CSFShape::GetCopy()const
{
    return new CSFShape;
}

void CSFShape::clear()
{
}

void CSFShape::GetBoundingBox(CGeoRect& )const
{
}

const CSFPoint& CSFShape::GetPoint(int index)const
{
	return CSFPoint::POINT_NULL;
}


void CSFShape::RemoveRing(int ringNo)
{
}
    
int CSFShape::GetRingNo(int nodeNo)const
{
	return -1;
}

bool CSFShape::AjoutePoint(const CGeoPoint& point)
{
    return true;
}

void CSFShape::InsertPoint(int segmentNo, const CGeoPoint& point)
{
}

void CSFShape::RemovePoint(int index)
{
}

double CSFShape::GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo)const
{   
    return DBL_MAX;
}

int CSFShape::GetNearestPointNo(const CGeoPoint& pt)const
{
    double distanceMin = 1e20;
    int no = -1;
    for(int i=0; i<GetNbPoints(); i++)
    {
        double d = GetPoint(i).GetMinimumDistance(pt);
        if( d < distanceMin)
        {
            distanceMin = d;
            no = i;
        }
    }

    return no;
}

int CSFShape::GetNearestSegmentNo(const CGeoPoint& pt)const
{
    int segmentNo = -1;

    GetMinimumDistance(pt, &segmentNo);
    

    //GetNearestPointNo(x, y, bGeographic, &segmentNo);
    ASSERT( segmentNo != -1);

    return segmentNo;
}

void CSFShape::SetPrjID(size_t prjID)
{
}


bool CSFShape::TransformeProjection(CProjectionTransformation const& PT)
{
    return true;
}

void CSFShape::GetCentroid(CGeoPoint& pt)const
{
	pt.clear();
}

double CSFShape::GetArea(int ringNo)const
{
	return 0;
}

void CSFShape::GetAdjustedBoundingBox(CSFBoundingBox& boundingBox)const
{
//	ASSERT( !boundingBox.GetPrj().IsUnknow() );///A ENLEVER
//	CProjection ptr = boundingBox.GetPrj();
    boundingBox.clear();

    for(int i=0; i<GetNbPoints(); i++)
        boundingBox.ExtendBounds(GetPoint(i));


}

bool CSFShape::SometingInRect(const CGeoRect& rect)const
{
    return false;
}

void CSFShape::InversePointOrder(int ringNo)
{
}

bool CSFShape::IsInside(const CGeoPoint& P)const
{
	return false;
}
//**************************************************************

const CSFPoint CSFPoint::POINT_NULL;

CSFPoint::CSFPoint(size_t prjID):
CGeoPoint(prjID)
{
    m_shapeType = CShapeFileHeader::POINT;
}

CSFPoint::CSFPoint(const double& x, const double& y, size_t prjID):
CGeoPoint( x, y, prjID)
{
    m_shapeType = CShapeFileHeader::POINT;
}

CSFPoint::CSFPoint(const CGeoPoint& point)
{
    m_shapeType = CShapeFileHeader::POINT;
    operator= (point);
}

CSFPoint::~CSFPoint()
{
}

CSFPoint& CSFPoint::operator=(const CGeoPoint& point)
{
    if( this != &point)
    {
		CGeoPoint::operator =(point);
    }

    return *this;
}

CSFShape* CSFPoint::GetCopy()const
{
    CSFPoint* pShape = new CSFPoint(*this);
    return pShape;
}

__int32 CSFPoint::GetLength()const
{
    return 10;
}

void CSFPoint::clear()
{
	CGeoPoint::clear();
}
bool CSFPoint::operator == (const CGeoPoint& point)const
{
    return (fabs(point.m_x - m_x) < 0.000001) && (fabs(point.m_y - m_y) < 0.000001);
}
bool CSFPoint::operator != (const CGeoPoint& point)const
{
    return !((*this) == point);
}
void CSFPoint::GetBoundingBox(CGeoRect& box)const
{
    box.SetRectEmpty();
	box.ExtendBounds(*this);
}

bool CSFPoint::AjoutePoint(const CGeoPoint& point)
{
    operator=(point);
    return true;
}

void CSFPoint::InsertPoint(int segmentNo, const CGeoPoint& point)
{
    operator=(point);
}


void CSFPoint::RemovePoint(int index)
{
    clear();
}

//double CSFPoint::GetMinimumDistance(const double& x, const double& y, bool bGeographic, int* pNearestSegmentNo)const
double CSFPoint::GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo)const
{
	
    /*double distance = 0;
    if( bGeographic )
        distance = UtilWin::GetDistance(y,x, m_y, m_x);
    else distance = sqrt( SQUARE(m_x()-x) + SQUARE(m_y()-y) );
    */
    if( pNearestSegmentNo )
        *pNearestSegmentNo = 0;

    return GetDistance(pt);
}


void CSFPoint::GetCentroid(CGeoPoint& pt)const
{
    pt = *this;
}

/*bool CSFPoint::Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, const CShapeFile& shapeFile)
{
    const CGeoRect& rcViewPort = viewPort.GetRect();
    ASSERT( !rcViewPort.IsRectEmpty() );
    ASSERT( !rcBounds.IsRectEmpty() );

    double fLat = m_Y;
    double fLon = m_X;

    bool bSameProjection = (shapeFile.GetProjection() == viewPort.GetProjection() );

    if( !bSameProjection )
    {
        if( !viewPort.CartoCoordToSameUnit(shapeFile.GetProjection(), fLat, fLon) )
        {
            //incapable de tranbsformer les points
            return false;
        }
    }

    ASSERT( rcViewPort.SameUnit(fLat, fLon)  );
    if( rcViewPort.PtInRect(fLat, fLon)  )
    {
        int x=0;
        int y=0;

        shapeFile.CartoCoordToScrCoord(rcBounds, rcViewPort, fLat, fLon, x, y);
        //DrawPoint(pDC, rcBounds, x, y, m_style, m_nSize);
        shapeFile.DrawPoint(pDC, rcBounds, x, y, CShowCarto::CIRCLE, 3);
    }


    


    return true;
}
*/
/*
CSFPoint CSFPoint::GetDistanceXY(const CGeoPoint& pt, bool bGeographic)const
{
    CSFPoint dist;

    if( bGeographic )
    {
        dist.m_y = SIGNE(pt.m_y - m_y())*UtilWin::GetDistance(pt.m_y,m_x(), m_y(), m_x());
        dist.m_x = SIGNE(pt.m_x - m_x())*UtilWin::GetDistance(m_y(), pt.m_x, m_y(), m_x());
    }
    else 
    {
        dist.m_x = (pt.m_x - m_x());
        dist.m_y = (pt.m_y - m_y());
    }

    return dist;
}
*/

void CSFPoint::SetPrjID(size_t prjID)
{
	CGeoPoint::SetPrjID(prjID);
}

bool CSFPoint::TransformeProjection(CProjectionTransformation const& PT)
{
	return CGeoPoint::Reproject( PT );
}

/*void CSFPoint::GetRingPoints(int ring, CSFPointArray& point)
{
    ASSERT( ring == 0);
    point.clear();
    point.push_back( CSFPoint( m_X, m_Y ) );
}

void CSFPoint::SetRingPoints(int ring, const CSFPointArray& point)
{
    ASSERT( ring == 0);
    ASSERT( point.size() <= 1);
    if( point.size() == 1 )
    {
        m_X = point[0].m_x;
        m_Y = point[0].m_y;
    }

}
*/

bool CSFPoint::SometingInRect(const CGeoRect& rect)const
{
    return rect.PtInRect(*this);
}

//**************************************************************
CSFMultiPoint::CSFMultiPoint()
{
    m_shapeType = CShapeFileHeader::MULTIPOINT;
    clear();
}

CSFMultiPoint::~CSFMultiPoint()
{
}

CSFShape* CSFMultiPoint::GetCopy()const
{
    CSFMultiPoint* pShape = new CSFMultiPoint();

    pShape->m_boundingBox = m_boundingBox;
    pShape->m_points = m_points;

    return pShape;

}

__int32 CSFMultiPoint::GetLength()const
{
	return __int32(2 + 16 + 2 + m_points.size() * 8);
}


void CSFMultiPoint::clear()
{
    m_boundingBox.clear();
    m_points.clear();
}


/*bool CSFMultiPoint::operator == (const CSFMultiPoint& point)const
{
    bool bRep = true;

    if( m_points.size() == point.GetrSize() )
    {
        for(int i=0; i<m_points.size(); i++)
        {
            if( m_points[i] != point.m_points[i] )
            {
                bRep = false;
                break;
            }
        }
    }
    else bRep = false;

    return bRep;
}
bool CSFMultiPoint::operator != (const CSFMultiPoint& point)const
{
    return !((*this) == point);
}
*/

void CSFMultiPoint::GetBoundingBox(CGeoRect& box)const
{
     box = m_boundingBox;
}

bool CSFMultiPoint::AjoutePoint(const CGeoPoint& point)
{
	ASSERT( point.IsInit() );

    m_points.push_back( point );
    m_boundingBox.ExtendBounds( point);

    return true;
}

void CSFMultiPoint::InsertPoint(int segmentNo, const CGeoPoint& point)
{
    //m_points.SetAtGrow(segmentNo, point );
	m_points.insert(m_points.begin()+segmentNo, point);
    m_boundingBox.ExtendBounds( point);
}

void CSFMultiPoint::RemovePoint(int index)
{
	m_points.erase(m_points.begin()+index);
    
    GetAdjustedBoundingBox(m_boundingBox);
}


double CSFMultiPoint::GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo)const
{
    double distanceMin = 1e20;
    for(int i=0; i<m_points.size(); i++)
    {
        double d = m_points[i].GetMinimumDistance(pt);
        if( d < distanceMin)
        {
            distanceMin = d;
            if( pNearestSegmentNo )
                *pNearestSegmentNo = i;
        }
    }

    return distanceMin;
}


/*bool CSFMultiPoint::Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, const CShapeFile& shapeFile)
{
    for(int i=0; i<m_points.size(); i++)
    {
        if( !m_points[i].Draw(pDC, rcBounds, viewPort, shapeFile) )
            return false;
        //shapeFile.DrawPoint(pDC, rcBounds, m_points[i].m_x, m_points[i].m_y, CShowCarto::CIRCLE, 4);
    }

    return true;
}
*/

void CSFMultiPoint::SetPrjID(size_t prjID)
{
	for(int i=0; i<m_points.size(); i++)
		((CGeoPoint&)m_points[i]).SetPrjID(prjID);
	
	m_boundingBox.SetPrjID(prjID);
}

bool CSFMultiPoint::TransformeProjection(CProjectionTransformation const& PT)
{
	ASSERT( m_boundingBox.GetPrjID() == PT.GetSrc()->GetPrjID());

    m_boundingBox.clear();

    bool bRep = true;
    for(int i=0; i<m_points.size(); i++)
    {
        if( m_points[i].Reproject(PT) )
		{
			m_boundingBox.ExtendBounds(m_points[i]);
		}
		else
		{
            bRep = false;
			break;
		}
    }

    return bRep;
}

void CSFMultiPoint::GetCentroid(CGeoPoint& pt)const
{
    pt.clear();
    for(int i=0; i<m_points.size(); i++)
    {
        pt += m_points[i];
    }

    if( m_points.size() != 0 )
    {
        pt /= (double)m_points.size();
    }
}

double CSFMultiPoint::GetArea(int ringNo)const
{
	return 0;
}


void CSFMultiPoint::GetRingPoints(int ring, CSFPointArray& point)const
{
    ASSERT( ring == 0);

    point = m_points;
    
}

void CSFMultiPoint::SetRingPoints(int ring, const CSFPointArray& point)
{
    ASSERT( ring == 0);
    m_points = point;
    
    GetAdjustedBoundingBox(m_boundingBox);
}

void CSFMultiPoint::AddRing(const CSFPointArray& point)
{
	m_points.insert(m_points.end(), point.begin(), point.end() );
	GetAdjustedBoundingBox(m_boundingBox);
}

bool CSFMultiPoint::SometingInRect(const CGeoRect& rect)const
{

    bool bRep = false;
    for(int i=0; i<m_points.size(); i++)
    {
        if( rect.PtInRect(m_points[i]) )
        {
            bRep = true;
            break;
        }
    }

    return bRep;
}

void CSFMultiPoint::InversePointOrder(int ringNo)
{
	for(int i=0; i<m_points.size()/2; i++)
	{
		CSFPoint pt = m_points[i];
		m_points[i] = m_points[m_points.size()-i-1];
		m_points[m_points.size()-i-1] = pt;
	}
}

//**************************************************************
CSFPolyLine::CSFPolyLine()
{
    m_shapeType = CShapeFileHeader::POLYLINE;
    clear();
}

CSFPolyLine::~CSFPolyLine()
{
}



CSFShape* CSFPolyLine::GetCopy()const
{
    CSFPolyLine* pShape = new CSFPolyLine();
    
    pShape->m_boundingBox = m_boundingBox;
    pShape->m_beginParts = m_beginParts;
    pShape->m_points = m_points;

    return pShape;

}

__int32 CSFPolyLine::GetLength()const
{
	return __int32 (2 + 16 + 2 + 2 + m_beginParts.size() * 2 + m_points.size() * 8);
}


void CSFPolyLine::clear()
{
    m_boundingBox.clear();
    m_beginParts.clear();
    m_points.resize(0,4);
}


bool CSFPolyLine::AjoutePoint(const CGeoPoint& point)
{
    bool bRep = false;
    ASSERT( m_beginParts.size() > 0); //add ring first

    
    if( m_points.size() == 0 || m_points.back() != point )
    {
        bRep = true;
        m_points.push_back(point);
		m_boundingBox.ExtendBounds(point);
    }

    return bRep;
}

void CSFPolyLine::InsertPoint(int segmentNo, const CGeoPoint& point)
{
    int ringNo = GetRingNo(segmentNo);
    m_points.insert(m_points.begin()+segmentNo+1, point );
    
    AdjusteBeginParts(ringNo, 1);

    m_boundingBox.ExtendBounds( point);
}


void CSFPolyLine::RemovePoint(int nodeNo)
{
	int ringIndex = GetRingNo(nodeNo);
	int first=0, last=0;
	GetPointIndex(ringIndex, first, last);
//	int nbPointInRing = GetNbPoints(ringNo);
	if( (last - first) > 1) 
	{
		if( nodeNo == first )
			SetPoint(last, GetPoint(first+1) );

		if( nodeNo == last )
			SetPoint(first, GetPoint(last-1) );

		m_points.erase(m_points.begin()+nodeNo);
		AdjusteBeginParts(ringIndex, -1);
	}
	else RemoveRing(ringIndex);

	GetAdjustedBoundingBox(m_boundingBox);
}


void CSFPolyLine::RemoveRing(int ringNo)
{
    int first=0;
    int last=0;
    GetPointIndex(ringNo, first, last);
	m_points.erase(m_points.begin() + first, m_points.begin() + last);
    AdjusteBeginParts(ringNo, first-last-1);
	ASSERT( (ringNo == m_beginParts.size()-1)||m_beginParts[ringNo] == m_beginParts[ringNo+1]);
	m_beginParts.erase(m_beginParts.begin() + ringNo);
	

    GetAdjustedBoundingBox(m_boundingBox);

}

void CSFPolyLine::GetRingPoints(int ringNo, CSFPointArray& points)const
{
    ASSERT(ringNo >=0 && ringNo < m_beginParts.size());

    
    int first = 0;
    int last = 0;
    GetPointIndex(ringNo, first, last);

    ASSERT(  m_shapeType != CShapeFileHeader::POLYGON || m_points[first] == m_points[last] );//the first and last point of a ring are the same

    points.resize(last-first+1);
    for(int i=0; i<points.size(); i++)
        points[i] = m_points[first+i];
}

void CSFPolyLine::SetRingPoints(int ringNo, const CSFPointArray& point)
{
    ASSERT(point.size() > 0); //if == 0 use erase Ring
    ASSERT(point[0] == point[point.size()-1]); //the first and the last point must be the same
    ASSERT(ringNo >=0 && ringNo < m_beginParts.size());

    
    //elever les anciens point
    int first = 0;
    int last = 0;
    GetPointIndex(ringNo, first, last);
	m_points.erase(m_points.begin() + first, m_points.begin() + last);
    for(int i=0; i<point.size() ; i++)
		m_points.insert(m_points.begin() + first + i, point[i]);


    int diffRingSize = int(last - first + 1 - point.size());

    if( diffRingSize != 0)
    {
        AdjusteBeginParts(ringNo, diffRingSize);
        //int nbRing = m_beginParts.size();
        //for(int i=ringNo+1; i<nbRing; i++)
          //  m_beginParts[i] += diffRingSize;
    }

    GetAdjustedBoundingBox(m_boundingBox);
}

void CSFPolyLine::AddRing(const CSFPointArray& point)
{
    ASSERT(point.size() > 0); //if == 0 use erase Ring
    ASSERT(point[0] == point[point.size()-1]); //the first and the last point must be the same
	int ringNo = NewRing();

	SetRingPoints(ringNo, point);
}


void CSFPolyLine::GetBoundingBox(CGeoRect& box)const
{
    box = m_boundingBox;
}

double CSFPolyLine::GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo)const
{
    double distanceMin = 1e20;
    //CSFPoint pt(x,y);

    int nbRing = (int)m_beginParts.size();
    for(int i=0; i<nbRing; i++)
    {
        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);
        if( first == last )
        {
            double d = m_points[first].GetMinimumDistance(pt);
            if( d < distanceMin)
            {
                distanceMin = d;
                if( pNearestSegmentNo )
                    *pNearestSegmentNo = first;
            }
        }

        for( int j = first; j <last ; j++)
        {
			
            CGeoDistance U = m_points[j].GetDistanceXY(pt);
            CGeoDistance P = m_points[j+1].GetDistanceXY(pt);

            
            CGeoDistance V = U-P;//(U.m_x-P.m_x, U.m_y-P.m_y); //= m_points[j+1].GetDistanceXY(m_points[j],bGeographic);
            double dV2 = SQUARE( V.m_x ) + SQUARE( V.m_y ) ;
            double c = (V.m_x*U.m_x + V.m_y*U.m_y)/dV2;


            double dPU2 = SQUARE(c*V.m_x) + SQUARE(c*V.m_y);

            double d = 0;

            //Si la perpendiculaire ce situ entre les deux points.
            if( c > 0 && dPU2 < dV2 )
            {
                d = fabs( (U.m_x*V.m_y) - (U.m_y*V.m_x) )/ sqrt( dV2 ) ;
                ASSERT( d >= 0);
                //ASSERT( d < min( d1, d2));
                //ASSERT( SQUARE(d) + dPU2 - SQUARE(test1) < 0.0001);
            }
            else
            {
                double d1 = m_points[j].GetMinimumDistance(pt);
                double d2 = m_points[j+1].GetMinimumDistance(pt);
                d = min( d1, d2);
            }


            if( d < distanceMin)
            {
                distanceMin = d;
                if( pNearestSegmentNo )
                    *pNearestSegmentNo = j;
            }

		        
        }
    }

    return distanceMin;
}

bool CSFPolyLine::SometingInRect(int ringIndex, const CGeoRect& rect)const
{
    int first = 0;
    int last = 0;
    GetPointIndex(ringIndex, first, last);

	if( first <= last )//if the ring isn't empty
	{
		char lastPosition = rect.GetTestPosition(m_points[first]);
		for( int j = first+1; j <=last ; j++)
		{
			char position = rect.GetTestPosition(m_points[j]);
			if( !(lastPosition & position) )
				return true;
	    
			lastPosition = position;
		}
	}

    return false;
}

bool CSFPolyLine::SometingInRect(const CGeoRect& rect)const
{
	ASSERT( rect.GetPrjID() == m_boundingBox.GetPrjID() );

	bool bRep = false;
    if( m_boundingBox.IsRectIntersect(rect) )
    {
        int nbRing = (int)m_beginParts.size();
        for(int i=0; i<nbRing; i++)
        {
			if( SometingInRect(i, rect) )
			{
				bRep = true;
				break;
			}
		}
	}
     
    return bRep;

}

void CSFPolyLine::InversePointOrder(int ringNo)
{
	ASSERT( ringNo >= 0 && ringNo < m_beginParts.size());

	CSFPointArray point;
	GetRingPoints(ringNo, point);

	for(int i=0; i<point.size()/2; i++)
	{
		CSFPoint pt = point[i];
		point[i] = point[point.size()-i-1];
		point[point.size()-i-1] = pt;
	}

    SetRingPoints(ringNo, point);
}

/*t CSFPolyLine::GetNearestPointNo(const double& x, const double& y, bool bGeographic)const
{
    int no = -1;
    double distanceMin = 1e20;
    CSFPoint pt(x,y);

    int nbRing = m_beginParts.size();
    for(int i=0; i<nbRing; i++)
    {
        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);

        for( int j = first; j <last ; j++)
        {
            CSFPoint U = m_points[j].GetDistanceXY(pt, bGeographic);
            CSFPoint P = m_points[j+1].GetDistanceXY(pt, bGeographic);

//            double test1 = sqrt( SQUARE(U.m_x) + SQUARE(U.m_y)); 
  //          double test2 = sqrt( SQUARE(P.m_x) + SQUARE(P.m_y)); 
            
            CSFPoint V(U.m_x-P.m_x, U.m_y-P.m_y); //= m_points[j+1].GetDistanceXY(m_points[j],bGeographic);
            double dV2 = SQUARE( V.m_x ) + SQUARE( V.m_y ) ;
            double c = (V.m_x*U.m_x + V.m_y*U.m_y)/dV2;
    //        CSFPoint Proj(c*V.m_x, c*V.m_y );


            double dPU2 = SQUARE(c*V.m_x) + SQUARE(c*V.m_y);

            double d = 0;

            //Si la perpendiculaire ce situ entre les deux points.
            if( c > 0 && dPU2 < dV2 )
            {
                d = fabs( (U.m_x*V.m_y) - (U.m_y*V.m_x) )/ sqrt( dV2 ) ;
                ASSERT( d >= 0);
                //ASSERT( d < min( d1, d2));
                //ASSERT( SQUARE(d) + dPU2 - SQUARE(test1) < 0.0001);
            }
            else
            {
                double d1 = m_points[j].GetMinimumDistance(x,y,bGeographic);
                double d2 = m_points[j+1].GetMinimumDistance(x,y,bGeographic);
                d = min( d1, d2);
            }

            if( d < distanceMin)
            {
                no = j;
                distanceMin = d;
            }
        }
    }

    return no;

}
*/

/*bool CSFPolyLine::Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, const CShapeFile& shapeFile)
{
    const CGeoRect& rcViewPort = viewPort.GetRect();

    ASSERT( !rcViewPort.IsRectEmpty() );
    ASSERT( !rcBounds.IsRectEmpty() );
    bool bRep = true;    


    bool bSameProjection = (shapeFile.GetProjection() == viewPort.GetProjection() );

    ASSERT(m_beginParts.size()>0);

    
    int nbRing = m_beginParts.size();
    for(int i=0; i<nbRing; i++)
    {
        char lastPosition = 0;
    	char position = 0;
        bool bFirst = true;

        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);


        for( int j = first; j <=last ; j++)
        {
            double fLat = m_points[j].m_y;
            double fLon = m_points[j].m_x;

            if( !bSameProjection )
                if( !viewPort.CartoCoordToSameUnit(shapeFile.GetProjection(), fLat, fLon) )
                {
                    //incapable de tranbsformer les points
                    bRep = false;
                    break;
                }

            ASSERT( rcViewPort.SameUnit(fLat, fLon)  );

            int x=0;
            int y=0;
		    
		    lastPosition = position;
		    position = rcViewPort.GetTestPosition(fLat, fLon);
		    shapeFile.CartoCoordToScrCoord(rcBounds, rcViewPort, fLat, fLon, x, y);

		    if( !(lastPosition & position) )
		    {
			    
			    if(bFirst)
                    pDC->MoveTo(x, y);
                else
				    pDC->LineTo(x, y);
		    }
		    else 
		    {
			    //segment nonvisible
			    pDC->MoveTo(x, y);
		    }

		    if( bFirst )
			    bFirst = false;
			    
        }
    }


    return bRep;

}
*/
void CSFPolyLine::SetPrjID(size_t prjID)
{
	for(int i=0; i<m_points.size(); i++)
		((CGeoPoint&)m_points[i]).SetPrjID(prjID);
	
	m_boundingBox.SetPrjID(prjID);
}

bool CSFPolyLine::TransformeProjection(CProjectionTransformation const& PT)
{
	ASSERT( m_boundingBox.GetPrjID() == PT.GetSrc()->GetPrjID());

    m_boundingBox.clear();

    bool bRep = true;
    for(int i=0; i<m_points.size(); i++)
    {
        if( m_points[i].Reproject(PT) )
		{
			m_boundingBox.ExtendBounds(m_points[i]);
		}
		else
		{
			bRep = false;
			break;
		}

		
    }

    return bRep;
}

int CSFPolyLine::GetRingNo(int nodeNo)const
{
    ASSERT( nodeNo >= 0);
    ASSERT( m_beginParts.size() == 0 || m_beginParts[0] == 0);

    int no=-1;
    int nSize = (int)m_beginParts.size();
    for( int i=nSize-1; i>= 0; i--)
    {
        if( nodeNo >= m_beginParts[i] )
        {
            no = i;
            break;
        }
    }

    return no;
}

void CSFPolyLine::AdjusteBeginParts(int ringNo, int shift)
{
    ASSERT( ringNo >= 0 && ringNo < m_beginParts.size());

    int nSize = (int)m_beginParts.size();
    for( int i=ringNo+1; i<nSize; i++)
        m_beginParts[i] += shift;

    
}

double CSFPolyLine::GetArea(int ringNo)const
{
	ASSERT( ringNo>= -1 && ringNo < m_beginParts.size());
    double airTotal = 0;

	int nbRing = (int)m_beginParts.size();
	int firstRing = ringNo;
	int lastRing = ringNo+1;
	
	if( ringNo == -1)
	{
		firstRing = 0;
		lastRing = nbRing;
	}

    
    for(int i=firstRing; i<lastRing; i++)
    {
        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);
        
        for( int j = first; j <last; j++)
        {
			double air = CGeoPoint::GetDefaultArea(m_points[j], m_points[j+1]);
            airTotal += air;
        }
    }

	return airTotal/2;
}

void CSFPolyLine::GetCentroid(CGeoPoint& pt)const
{
    double airTotal = 0;
    pt.clear();

    //a vérifier
    //ne fonctionne pas avec des vecteurs
	int nbRing = (int)m_beginParts.size();
    for(int i=0; i<nbRing && i<1; i++) //on ne fait que le premier polygone
    {
        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);
        
        
        for( int j = first; j <last; j++)
        {
            double air = CGeoPoint::GetDefaultArea(m_points[j], m_points[j+1]);
            CGeoPoint ptCentroid = CGeoPoint::GetDefaultCentroid(m_points[j], m_points[j+1]);
            airTotal += air;

            pt += ptCentroid*air;
        }
        
        pt /= airTotal;
    }

}

//-------------------------------------------------
// GetNbIntersection()
//
// Permet de calculer le nombre d'intersections entre un
// segment et le polygone
//-------------------------------------------------
int CSFPolyLine::GetNbIntersection(const CGeoSegment& segment)const
{
    int nbIntersection = 0;
    
	//Si le segment n'est un point 
	if( !segment.IsSegmentEmpty() )
	{
		//Pour tous les segements du polygone
		// on calcul le nombre d'intersections
		int nbRing = (int)m_beginParts.size();
		for(int i=0; i<nbRing; i++)
		{
			int first = 0;
			int last = 0;
			GetPointIndex(i, first, last);
	    	for( int j = first; j <last; j++)
			{
				CGeoSegment segTmp(m_points[j], m_points[j+1]);
				nbIntersection += segment.IsSegmentIntersect(segTmp);
			}
		}
	}

	return nbIntersection;
}


void CSFPolyLine::GetItersectionPoints(const CGeoSegment& segment, CSFPointArray& points)const
{
	points.clear();
    // le premier segment est en fait la ligne de hachurage
//    int nbSeg = GetNbIntersection(segment);
  //  if (nbSeg > 0)//si le nombre d'intersections est suppérieur à 0
	//{
		// pour toutes les segments du polygone
        // on trouve les points d'intersections
		int nbRing = (int)m_beginParts.size();
		for(int i=0; i<nbRing; i++)
		{
			int first = 0;
			int last = 0;
			GetPointIndex(i, first, last);
	    	for( int j = first; j <last; j++)
			{
				CGeoSegment segTmp(m_points[j], m_points[j+1]);
    
				if (segment.IsSegmentIntersect(segTmp))
				{
					points.push_back( segTmp.GetIntersectionPoint(segment) );
				}
			}
		}
		//On trie les points d'intersections
//        SortCoordElem intersectionArray
	//}
}



//**************************************************************
CSFPolygon::CSFPolygon()
{
    //m_pInternalEleminationData = NULL;
    m_shapeType = CShapeFileHeader::POLYGON;
    clear();
}
CSFPolygon::~CSFPolygon()
{
    //if ( m_pInternalEleminationData )
      //  delete [] m_pInternalEleminationData;

}

CSFPolygon::CSFPolygon(const CSFPolygon& poly)
{
	m_shapeType = CShapeFileHeader::POLYGON;
	operator=(poly);
}

void CSFPolygon::clear()
{
    CSFPolyLine::clear();

    m_internalElimination = false;
    m_internalOutsideGrid.clear();
    m_internalInsideGrid.clear();

    m_cellWidth = 0;
    m_cellHeight = 0;
    m_nbCols = 0;

    //if ( m_pInternalEleminationData )
      //  delete [] m_pInternalEleminationData;

//    m_pInternalEleminationData = NULL;
  //  m_centroid.clear();
    //m_distanceFromCentroid = 0;

}

CSFPolygon& CSFPolygon::operator=(const CSFPolygon& poly)
{
	if( &poly != this )
	{
		m_boundingBox = poly.m_boundingBox;
		m_beginParts = poly.m_beginParts;
		m_points = poly.m_points;
	}
	return *this;
}

CSFShape* CSFPolygon::GetCopy()const
{
    CSFPolygon* pShape = new CSFPolygon();
    
    pShape->m_boundingBox = m_boundingBox;
    pShape->m_beginParts = m_beginParts;
    pShape->m_points = m_points;

    //???? est-ce qu'on copy

//    pShape->m_internalElimination = m_internalElimination;
  //  pShape->m_centroid = m_centroid;
    //pShape->m_distanceFromCentroid = m_distanceFromCentroid;

    return pShape;

}

bool CSFPolygon::IsInside(const CGeoPoint& P)const
{
	ASSERT( m_boundingBox.GetPrjID() == P.GetPrjID());
    int count = 0;

    if( m_boundingBox.PtInRect(P) )
    {
        if( m_internalElimination )
        {
            int x = int( (P.m_x - m_boundingBox.m_xMin) /m_cellWidth + 0.000000001);
            int y = int( (P.m_y - m_boundingBox.m_yMin) /m_cellHeight + 0.000000001);

            int i = y*m_nbCols + x;
            if( m_internalOutsideGrid[i] )
                return false;
            if( m_internalInsideGrid[i] )
                return true;
        }

		int nbRing = (int)m_beginParts.size();
        for(int i=0; i<nbRing; i++)
        {
            count += GetXRayCount(i, P);
        }
    }

    return (count%2)!=0;
}


int CSFPolygon::GetXRayCount(int no, const CGeoPoint& P)const
{
    int first = 0;
    int last = 0;
    GetPointIndex(no, first, last);	
    int numverts = last - first+1;
	if( numverts == 0)
		return 0;

    const CSFPoint *pPoint = m_points.data();
	double vtx1(0);
	double vty1(0);
    double vtx0=pPoint[first].m_x;
	double vty0=pPoint[first].m_y;
    double dv0 = vty0-P.m_y;
    bool yflag0= dv0 >= 0.0;
    bool yflag1 = false;

	int crossings=0;
	for(int j=1;j<numverts;j++) //1 pour ne pas prendre le premier point
    {
		//cleverness: bobble between filling endpoints of edges, so
		//that the previous edge's shared endpoint is maintained 
		if(j & 0x1) //odd j
        { 
            vtx1=pPoint[first+j].m_x;
			vty1=pPoint[first+j].m_y;
			yflag1=(vty1 >= P.m_y);
		}
		else 
        {
            vtx0=pPoint[first+j].m_x;
			vty0=pPoint[first+j].m_y;
			yflag0=(dv0=vty0-P.m_y) >=0;
			
		}
		//check if points not both above/below X axis (ray can't cross)
		if(yflag0 != yflag1) 
        {
			//check if points on same side of Y axis*/
            bool xflag0 = vtx0>=P.m_x;
			if( xflag0 == (vtx1>=P.m_x)) 
            {
				if(xflag0) crossings++; //segment on right-hand side of point. Ray Crosses
			}
			else 
            {
				//compute intersection of edge & ray. Note if > point's X
                crossings += ((vtx0-dv0*(vtx1-vtx0)/(vty1-vty0) ) >= P.m_x)?1:0;
			}
		}
	}

	return crossings;
}


bool CSFPolygon::IsValid() const
{
    bool bRep = true;

	int nbRing = (int)m_beginParts.size();
    for(int i=0; i<nbRing; i++)
    {
        int first = 0;
        int last = 0;
        GetPointIndex(i, first, last);

        if( first > last ||
			m_points[first] != m_points[last] )
        {
            bRep = false;
            break;
        }
    }

    return bRep;
}


void CSFPolygon::Replace(int first, int last, const CSFPoint& pt)
{
    int no = GetRingNo(first);
    ASSERT( no == GetRingNo(last) );
    ASSERT( last - first >= 1);

	m_points.erase(m_points.begin() + first, m_points.begin() + last );
	m_points.insert(m_points.begin() + first, pt);

    AdjusteBeginParts(no, last-first);

    m_internalElimination = false;
}

void CSFPolygon::ComputeInternalElimination(int nbCols, int nbRows)
{
    ASSERT( nbCols != 0 && nbRows != 0);
    
    m_internalElimination = false;

    m_internalOutsideGrid.resize(nbCols * nbRows);
    m_internalInsideGrid.resize(nbCols * nbRows);

    m_nbCols = nbCols;
    m_cellWidth = m_boundingBox.Width()/nbCols;
    m_cellHeight = m_boundingBox.Height()/nbRows;

    CGeoRect box(m_boundingBox);
	box.m_xMax = m_boundingBox.m_xMin + m_cellWidth;
    box.m_yMax = m_boundingBox.m_yMin + m_cellHeight;

    for(int i=0; i<nbCols * nbRows; i++)
    {
        int x = i%nbCols;
        int y = (int)(i/nbCols);
        CGeoRect rect(box);
		rect += CGeoPoint(x*m_cellWidth, y*m_cellHeight, box.GetPrjID() );
    
        if( SometingInRect( rect ) )
        {
            //unknown state
            m_internalOutsideGrid.set(i, false);
			m_internalInsideGrid.set(i, false);
        }
        else
        {
            //ici les 4 points de la boite sont soit à l'intérieur
            // soit à l'extérieur. On test donc un des point pour
            // connaitre l'état de la boite
            m_internalInsideGrid.set( i, IsInside( rect.LowerLeft() ) );
            m_internalOutsideGrid.set( i, !m_internalInsideGrid[i] );
        }
    }

    m_internalElimination = true;
    
}


//**************************************************************
CSFRecord::CSFRecord(int type):
m_number(0),
//m_length(0),
m_pShape(NULL)
{
    m_pShape = GetShape(type);
}

CSFRecord::CSFRecord(const CSFRecord& record):
m_number(record.m_number),
//m_length(record.m_length),
m_pShape(NULL)
{
    m_pShape = record.m_pShape->GetCopy();
}

CSFRecord& CSFRecord::operator=(const CSFRecord& record)
{
    ASSERT( this != &record);

    if( this != &record)
    {
        delete m_pShape;
        m_pShape = NULL;

        m_number = record.m_number;
        //m_length = record.m_length;
        m_pShape = record.m_pShape->GetCopy();
    }

    return *this;
}

CSFRecord::~CSFRecord()
{
    delete m_pShape;
    m_pShape = NULL;
}

CSFShape* CSFRecord::GetShape(int type)
{
    CSFShape* pShape = NULL;
    switch(type)
    {
    case CShapeFileHeader::SHAPE_NULL: pShape = new CSFShape();break;
    case CShapeFileHeader::POINT: pShape = new CSFPoint();break;
    case CShapeFileHeader::POLYGON: pShape = new CSFPolygon(); break;
    case CShapeFileHeader::POLYLINE: pShape = new CSFPolyLine(); break;
    case CShapeFileHeader::MULTIPOINT:pShape = new CSFMultiPoint(); break;
    default: ASSERT(false);
    }

    return pShape;
}

//**************************************************************


CShapeFileBase::CShapeFileBase()
{
    clear();
    //m_geoFileInfo.SetGeoFileType( CGeoFileInfo::FT_SHAPE_FILE );
    
}

CShapeFileBase::CShapeFileBase(const CShapeFileBase& shapeFile)
{
    //m_geoFileInfo.SetGeoFileType(CGeoFileInfo::FT_SHAPE_FILE);
    operator=(shapeFile);
}

CShapeFileBase& CShapeFileBase::operator=(const CShapeFileBase& shapeFile)
{
    if( &shapeFile != this)
    {
        CShapeFileBase::clear();
        //CGeoFileBase::operator=(shapeFile);
  
        m_header = shapeFile.m_header;
        m_infoDBF = shapeFile.m_infoDBF;

        m_records.resize(shapeFile.m_records.size());
        for(int i=0; i<m_records.size(); i++)
        {
            m_records[i] = new CSFRecord( *(shapeFile.m_records[i]) );
        }

//		SynchronizeProjection();
    }

    return *this;
}

void CShapeFileBase::SynchronizeProjection()
{
	//update boundingBox projection here
	CGeoRect rect = m_header.GetBoundingBox();
	
	rect.SetPrjID(GetPrjID());
	m_header.SetBoundingBox( rect ); 

	//update all record projection
	for(int i=0; i<m_records.size(); i++)
	{
		m_records[i]->GetShape().SetPrjID( GetPrjID() );
	}
}

CShapeFileBase::~CShapeFileBase()
{
    CShapeFileBase::clear();
}


void CShapeFileBase::clear()
{
//    CGeoFileBase::clear();

    m_header.clear();
    m_infoDBF.clear();

    for(int i=0; i<m_records.size(); i++)
    {
        delete m_records[i];
        m_records[i] = NULL;
    }

    m_records.clear();
}

ERMsg CShapeFileBase::Open(const string& filePath, int openMode)
{
    ERMsg msg;
    
    if( openMode == modeRead )
          CShapeFileBase::clear();

    
    if( openMode == modeRead )
    {
		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::archive::binary_iarchive io(file, boost::archive::no_header);
				msg = CShapeFileHeader::ReadHeader(m_header, io);
			}
			catch (boost::archive::archive_exception e)
			{
				msg.ajoute(e.what());
			}
			
			file.close();
        }
    }
    

	if(msg)
		m_filePath=filePath;

    return msg;
}


ERMsg CShapeFileBase::Close(bool bSaveInfoFile)
{
    return ERMsg(); //CGeoFileBase::Close(bSaveInfoFile);
}


ERMsg CShapeFileBase::Read(const string& filePath)
{
    ERMsg msg;

    CShapeFileBase::clear();

	ifStream file;
	msg = file.open(filePath, ios::in | ios::binary);

	if (msg)
	{
		short headerLength = 0;
		short recordLength = 0;

		try
		{
			boost::archive::binary_iarchive io(file, boost::archive::no_header);

			msg = CShapeFileHeader::ReadHeader(m_header, io);
			if (msg)
			{
				std::fstream::pos_type endOfFile = m_header.GetFileLength() * 2;
				//std::fstream::pos_type pos = file.tellg();
				//CSFRecord* pRecord=NULL;
				while (file.tellg()<endOfFile)
				{
					CSFRecord* pRecord = GetRecord(io);
					
					if (pRecord != NULL)
						m_records.push_back(pRecord);
				}
					

				ASSERT(m_records.size() == 0 || m_header.GetBoundingBox().IsRectNormal());
			}
		}
		catch (boost::archive::archive_exception e)
		{
			msg.ajoute(e.what());
		}

		file.close();


		//read DBF info
		if (msg)
		{
			msg = ReadDBF(GetDBFName(filePath));

			if (msg)
			{
				//read projection
				if (FileExists(GetPrjFilePath(filePath)))
					msg = CProjectionManager::Load(GetPrjFilePath(filePath), m_pPrj);

				SynchronizeProjection();
				//AutoSetXYUnits();
			}
		}

    }
   
    
    

    return msg;
}



ERMsg CShapeFileBase::Write(const string& filePath)
{
    ERMsg msg;


	ofStream file;
	msg = file.open(filePath, ios::out | ios::binary);
	if (msg)
	{
		try
		{
			// write map instance to archive
			boost::archive::binary_oarchive io(file, boost::archive::no_header);
			m_header.WriteHeader(io);
			
			int nbRecord = (int)m_records.size();
			for (int i = 0; i < nbRecord; i++)
				m_records[i]->Write(io);
		}
		catch (boost::archive::archive_exception e)
		{
			msg.ajoute(e.what());
		}
			
		file.close();


		if( msg)
		{
			msg = WriteIndex(filePath);
			if( msg )
				msg = WriteDBF(filePath);
            

			m_pPrj->Save(GetPrjFilePath(filePath).c_str());
			
		}
    }
   
    
    
    return msg;
}


void CShapeFileBase::AddShape(const CSFShape& shape, const CDBFRecord& shapeInfo)
{
	int pos = (int)m_records.size();
    AddShape(shape);
    m_infoDBF.SetRecord(pos, shapeInfo);
}

void CShapeFileBase::AddShape(const CSFShape& shape)
{
	ASSERT(m_infoDBF.GetNbRecord() == m_records.size());

	int pos = (int)m_records.size();
    m_records.resize(pos+1);
	m_infoDBF.SetNbRecord(pos+1);

	ASSERT(m_header.GetShapeType() ==  (CShapeFileHeader::TShape) shape.GetTypeNumber());
    
    m_records[pos] = new CSFRecord( shape.GetTypeNumber() );
    m_records[pos]->SetNumber(pos+1);
    m_records[pos]->SetShape(shape);

    CSFBoundingBox box;
    m_records[pos]->GetShape().GetBoundingBox(box);
    m_header.ExtendBounds(box);
    m_header.SetFileLength(m_header.GetFileLength() + shape.GetLength() + 4);
}

void CShapeFileBase::SetShape(int shapeNo, const CSFShape& shape)
{
	ASSERT(m_infoDBF.GetNbRecord() == m_records.size());
    ASSERT(m_header.GetShapeType() ==  (CShapeFileHeader::TShape) shape.GetTypeNumber());
    
    m_records[shapeNo]->SetShape(shape);
    UpdateHeader();
    
}

void CShapeFileBase::RemoveShape(int shapeNo)
{
    //m_header.SetFileLength(m_header.GetFileLength() - m_records[shapeNo]->GetLength() - 4);

    delete m_records[shapeNo];
	m_records.erase(m_records.begin() + shapeNo);
    m_infoDBF.RemoveRecord(shapeNo);
    for(int i=shapeNo; i<m_records.size(); i++)
    {
        m_records[i]->SetNumber(i+1);
    }

    //ajuster le header
    UpdateHeader();
    
}

/*void AddPoint(CSFPoint& point, int index, const string& label);
void AddVector(CSFVector& vector, int index, const string& label);

void CShapeFileBase::AddPolygon(CSFPolygon& poly, int index, const string& label)
{
	int pos = m_records.size();
	
	AddShape(poly);
	m_infoDBF[pos].SetElement(0, pos);
	m_infoDBF[pos].SetElement(1, index);
	m_infoDBF[pos].SetElement(2, label);
	
}
*/

void CShapeFileBase::SetShapeType(CShapeFileHeader::TShape type )
{
    
	ASSERT(m_records.size() == 0);
	m_header.SetShapeType( type );
/*	switch(type)
    {
    case CShapeFileHeader::SHAPE_NULL: 
    case CShapeFileHeader::POINT: 
    case CShapeFileHeader::POLYGON:
    case CShapeFileHeader::POLYLINE: 
    case CShapeFileHeader::MULTIPOINT: m_infoDBF.ResetField();break;
    default: ASSERT(false);
    }
*/

}


ERMsg CShapeFileBase::WriteIndex(const string& filePath)const
{
    ERMsg msg;

    CShapeFileIndex shx;
    GetShapeFileIndex(shx);
    
    msg = shx.Write(GetIndexName(filePath));

    return msg;
}

string CShapeFileBase::GetIndexName(const string& filePath)
{
    string indexFilePath = filePath;
	SetFileExtension(indexFilePath, ".shx");
	return indexFilePath;
}

string CShapeFileBase::GetDBFName(const string& filePath)
{
    string DBFFilePath = filePath;
	SetFileExtension(DBFFilePath, ".dbf");

    return DBFFilePath;
}

string CShapeFileBase::GetPrjFilePath(const string& filePath)
{
	string prjFilePath = filePath;
	SetFileExtension(prjFilePath, ".prj");

	return prjFilePath;
}


ERMsg CShapeFileBase::WriteDBF(const string& filePath)const
{
    return m_infoDBF.Write(GetDBFName(filePath)); 
}

ERMsg CShapeFileBase::ReadDBF(const string& filePath)
{
    ERMsg msg;

   
    msg = m_infoDBF.Read(GetDBFName(filePath)); 

	ASSERT( m_infoDBF.GetNbRecord() == m_records.size());

	if( msg && m_infoDBF.GetNbRecord() != m_records.size())
	{
		msg.ajoute(FormatMsg(IDS_MAP_BAD_DBF_FILE, filePath));
	}

    return msg;
}

void CShapeFileBase::CheckOrCreateDBF()
{
	if( m_infoDBF.GetNbRecord() != m_records.size())
	{
		//create default dbf file
		m_infoDBF.ResetField();
		m_infoDBF.AddField("Values", "8n");
		int nSize = (int)m_records.size();
		for(int i=0; i<nSize; i++)
			m_infoDBF.CreateNewRecord().SetElement(0, i+1);
	}
    
}
void CShapeFileBase::GetShapeFileIndex(CShapeFileIndex& shx)const
{
    //CShapeFileIndex index;
//    index.SetFileLength(50 + nSize* 4);

    shx.SetHeader(m_header);
	int nSize = (int)m_records.size();

    for(int i=0; i<nSize; i++)
    {
        shx.AddRecordInfo( m_records[i]->GetLength() );
    }

}

//permet de savoir s'il y a au moin un vecteur qui passe ce rectangle
// ne pas confondre avec IsRectIntersect
bool CShapeFileBase::SometingInRect(const CGeoRect& rect)const
{
//    ASSERT( rect.IsGeographic() == m_projection().IsGeographic() );

    if( !rect.IsRectIntersect(GetBoundingBox()) )
        return false;
        

    bool bRep = false;

	int nSize = (int)m_records.size();
    for(int i=0; i<nSize; i++)
    {
        const CSFShape& shape = m_records[i]->GetShape();

        if( shape.SometingInRect(rect) )
        {
            bRep = true;
            break;
        }
    }

     
    return bRep;
}


bool CShapeFileBase::IsRectIntersect(const CGeoRect& rect)const
{
    //ASSERT( rect.IsGeographic() == m_projection().IsGeographic() );

    if( !rect.IsRectIntersect(GetBoundingBox()) )
        return false;
    
	bool bRep = true;

	if( !SometingInRect(rect) )
	{
		//ici les 4 points de la boite sont soit à l'intérieur
        // soit à l'extérieur. On test donc un des point pour
        // connaitre l'état de la boite
        bRep = IsInside( rect.LowerLeft() );
	}
	 
    return bRep;
}

//dans la meme projection
bool CShapeFileBase::IsInside(const CGeoPoint& ptIn, int* pPolyNo )const
{
	//ASSERT( pt.GetPrjID() == GetPrjID() );

	if( pPolyNo )
        *pPolyNo = -1;

	CGeoPoint pt(ptIn);
	if(pt.GetPrjID() != GetPrjID() )
	{

		if (!pt.Reproject(CProjectionTransformationManager::Get(pt.GetPrjID(), GetPrjID())))
			return false;
	}

	bool bRep = false;
    if( m_header.GetShapeType( ) == CShapeFileHeader::POLYGON )
    {
		int nSize = (int)m_records.size();
        for(int i=0; i<nSize; i++)
        {
            ASSERT( m_records[i]->GetShape().GetType() == CShapeFileHeader::POLYGON );
            const CSFPolygon& poly = (CSFPolygon&) (m_records[i]->GetShape());
            
            if( poly.IsInside(pt) )
            {
                bRep = true;
				if(pPolyNo)
					*pPolyNo = i;
                break;
            }
            
        }
    }
    else
    {
        bRep = m_header.GetBoundingBox().PtInRect(pt);
    }

    return bRep;

}

//retourne la distance en Km
//pt must be in the same projection
double CShapeFileBase::GetMinimumDistance(const CGeoPoint& ptIn, int* pPolyNo)const
{
    double distanceMin = 1e20;
	
    if( pPolyNo )
        *pPolyNo = -1;

	CGeoPoint pt(ptIn);
	if( pt.GetPrjID() != GetPrjID() )
	{
		if (!pt.Reproject(CProjectionTransformationManager::Get(pt.GetPrjID(), GetPrjID())))
			return DBL_MAX;
	}

    CShapeFileHeader::TShape shapeType = m_header.GetShapeType( );
    if( shapeType != CShapeFileHeader::SHAPE_NULL  )//&& (shapeType != CShapeFileHeader::POLYGON || PtInShapeFile(lat, lon))
    {
		int nSize = (int)m_records.size();
        for(int i=0; i<nSize; i++)
        {
            const CSFShape& shape = m_records[i]->GetShape();
            double d = shape.GetMinimumDistance(pt);
//if the distance is the same we favorise point in the shape
			if( d < distanceMin ||
				( fabs( d - distanceMin) < 0.0000001 && shape.IsInside( pt) ))
            {
                if( pPolyNo )
                    *pPolyNo = i;
                distanceMin = d;
            }
        }
    }
    else distanceMin = DBL_MAX;
    
    return distanceMin;
}

void CShapeFileBase::Create(const CShapeFileBase& shapeFile, const set<int>& recordNo)
{
    CShapeFileBase::clear();

    m_header.SetShapeType( shapeFile.m_header.GetShapeType() );

	//copy dbf definition
    m_infoDBF.SetTableField( shapeFile.m_infoDBF.GetTableField() );
	
    //int nSize = ;
	for (int i = 0; i<shapeFile.m_records.size(); i++)
    {
		//if( UtilWin::FindInArray(recordNo, i) != -1)
		if (recordNo.find(i) != recordNo.end())
        {
			int pos = (int)m_records.size();
            AddShape( shapeFile.m_records[i]->GetShape()  );
	    	m_infoDBF.SetRecord(pos, shapeFile.m_infoDBF[i]);
        }
    }

}


ERMsg CShapeFileBase::Create( const CShapeFileBase& shapeFile, int vectorZoneNo, int uniqueID)
{
    StringVector zoneArray;
    zoneArray.push_back(shapeFile.GetDBF().GetElementStringForUniqueID(vectorZoneNo, uniqueID));
    
    string fieldName = shapeFile.GetDBF().GetTableField()[vectorZoneNo].GetName();


    return Create( shapeFile, fieldName, zoneArray);
}

ERMsg CShapeFileBase::Create( const CShapeFileBase& shapeFile, const string& fieldName, const StringVector& zoneArray)
{
	ASSERT(! fieldName.empty() );
    ASSERT( zoneArray.size() > 0);

	ERMsg msg;

	int fieldNo = shapeFile.m_infoDBF.GetFieldIndex(fieldName);
	if( fieldNo != -1 )
	{
		msg = Create( shapeFile, fieldNo, zoneArray);
	}
	else
    {
		msg.ajoute(FormatMsg(IDS_MAP_FIELD_NOT_FOUND, fieldName));
    }

	return msg;
}

ERMsg CShapeFileBase::Create( const CShapeFileBase& shapeFile, int fieldNo, const StringVector& zoneArray)
{
    ASSERT( fieldNo>= -1 && fieldNo<shapeFile.m_infoDBF.GetNbField() );
    ASSERT( zoneArray.size() > 0);

	ERMsg msg; 
    

    if( fieldNo >= 0 && fieldNo<shapeFile.m_infoDBF.GetNbField())
    {
	    CShapeFileBase::clear();

		//set shape file type
		m_header.SetShapeType( shapeFile.m_header.GetShapeType() );
		//set a tomporary file name
		//m_geoFileInfo.SetFilePath("__tmp__.shp");
		//set projection
		//m_projection() = shapeFile.m_projection();

		//create default dbf file
		m_infoDBF.SetTableField( shapeFile.m_infoDBF.GetTableField() );
		

		int nSize = (int)shapeFile.m_records.size();
	    for(int i=0; i<nSize; i++)
        {
            string nameSearch = shapeFile.m_infoDBF[i][fieldNo].GetElement();
			//StringVector::const_iterator it = ;
			if (std::find(zoneArray.begin(), zoneArray.end(), nameSearch) != zoneArray.end())
            {
				int pos = (int)m_records.size();
                AddShape( shapeFile.m_records[i]->GetShape()  );
	    	    m_infoDBF.SetRecord(pos, shapeFile.m_infoDBF[i]);
            }
        }
    }
    else //create shape from record no
    {
		//create a array of record
		set<int> recordNo;

		for(int i=0; i<zoneArray.size(); i++)
		{
			int index = ToInt(zoneArray[i]);
			ASSERT( index >= 0 && index < shapeFile.GetNbShape() );
			recordNo.insert( index );
		}
		
		Create( shapeFile, recordNo);
		
    }

    return msg;
}

bool CShapeFileBase::TransformeProjection(CProjectionTransformation const& PT)
{
	ASSERT( PT.GetSrc()->GetPrjID() == GetPrjID());

	if( !PT.IsSame() )
    //if( m_projection != projection )
    {
		for(int i=0; i<m_records.size(); i++)
		{
			if( !m_records[i]->GetShape().TransformeProjection(PT) )
			{
				RemoveShape(i);
				i--;
			}
		}
	    
		//m_projection = PT.GetDstPrj();

		//update the shapefile header
		UpdateHeader();
    }

	return true;
}


CShapeFileHeader::TShape CShapeFileBase::GetShapeType( const string& filePath )
{
    CShapeFileHeader::TShape type(CShapeFileHeader::SHAPE_NULL);
	
	ifStream file;
	if (file.open(filePath, ios::in | ios::binary))
	{
		try
		{
			boost::archive::binary_iarchive io(file, boost::archive::no_header);
			
			CShapeFileHeader header;
			if (CShapeFileHeader::ReadHeader(header, io))
				type = header.GetShapeType();
		}
		catch (...)
		{
		}

		file.close();
	}
    
    return type;

}


void CShapeFileBase::ComputeInternalElimination(int nbCols, int nbRows)
{
	if( nbCols > 200)
		nbCols = 200;
	if( nbRows > 200)
		nbRows = 200;

    if( m_header.GetShapeType() == CShapeFileHeader::POLYGON )
    {
        double totalsurface = GetBoundingBox().Height() * GetBoundingBox().Width();

		int nSize = (int)m_records.size();
        for(int i=0; i<nSize; i++)
        {
            
            ASSERT( m_records[i]->GetShape().GetType() == CShapeFileHeader::POLYGON );
            CSFPolygon& poly = (CSFPolygon&) (m_records[i]->GetShape());

            CGeoRect boxPoly;
            poly.GetBoundingBox(boxPoly);
            double surface = boxPoly.Height() * boxPoly.Width();
            //if( surface/totalsurface > 0.1 )
            if( nbCols*surface/totalsurface > 5 && nbRows*surface/totalsurface > 5)
                poly.ComputeInternalElimination( int(nbCols*surface/totalsurface) , int(nbRows*surface/totalsurface)  );
        }
    }

}


void CShapeFileBase::UpdateHeader()
{
    CGeoRect bounds(GetPrjID());
    int shapesSize = 0;
    int nSize = (int)m_records.size();
    for(int i=0; i<nSize; i++)
    {
        CSFBoundingBox box;
        m_records[i]->GetShape().GetBoundingBox( box );
        bounds.ExtendBounds(box);
        shapesSize += m_records[i]->GetLength() + 4;
    }

    SetBoundingBox(bounds);     //On remplace le BoundingBox
    m_header.SetFileLength(50 + shapesSize );  //on remplace la grosseur du fichier
	
}

void CShapeFileBase::GetInformation( string& information )const
{
    //CGeoFileBase::GetInformation( information );

    string line;
    information += GetString(IDS_MAP_INFO_SHAPEFILE );
    information += '\n';
 
    if( m_records.size() != 0)
    {
        string tmp;
        tmp = FormatA("%d", m_records.size() );
        line = FormatMsg(IDS_MAP_INFO_NB_RECORD, tmp );
        information += line;
        information += '\n';

        
        tmp = FormatA("%d", m_infoDBF.GetNbField() );
        line = FormatMsg(IDS_MAP_INFO_NB_FIELD, tmp );
        information += line;
        information += '\n';

        information += GetString(IDS_MAP_INFO_FIELD_DESCRIPTION );
        information += '\n';
       
        const CTableField& field= m_infoDBF.GetTableField();
        for(int i=0; i< m_infoDBF.GetNbField(); i++)
        {
            
            tmp = FormatA( "%-25.25s\t%-5c\t%-9d\t%-13d\n", field[i].GetName(), field[i].GetType(), field[i].GetLength(), field[i].GetDecimalCount() );
            information += tmp;
        }
    }
    else
    { 
        CShapeFileIndex index;

        //string indexFilePath;
        
        if( index.Read( GetIndexName(m_filePath) ) )
        {
            string tmp = FormatA("%d", index.GetNbRecord() );
            line = FormatMsg(IDS_MAP_INFO_NB_RECORD, tmp );
            information += line;
            information += '\n';
        }
		
		CDBF3 dbf;
		if( dbf.Read( GetDBFName(m_filePath) ) )
        {
			string tmp;
			tmp = FormatA("%d", dbf.GetNbField() );
			line = FormatMsg(IDS_MAP_INFO_NB_FIELD, tmp);
			information += line;
			information += '\n';

			information += GetString(IDS_MAP_INFO_FIELD_DESCRIPTION );
			information += '\n';

			const CTableField& field= dbf.GetTableField();
			for(int i=0; i< field.size(); i++)
			{
				tmp = FormatA( "%-25.25s\t%-5c\t%-9d\t%-13d\n", field[i].GetName(), field[i].GetType(), field[i].GetLength(), field[i].GetDecimalCount() );
				information += tmp;
			}
       }
    }
}

void CShapeFileBase::GetFieldArray(StringVector& fields, bool bGetAll)const
{
	fields.clear();

	const CTableField& tbf = m_infoDBF.GetTableField();
	for(int i=0; i<tbf.size(); i++)
	{
		bool bShow = true;
		if( !bGetAll )
		{
			bShow = tbf[i].GetType()==CDBFField::CHARACTER  ||
						   (tbf[i].GetType()==CDBFField::NUMBER && tbf[i].GetDecimalCount()==0);
				
		}

		if( bShow)
		{
			fields.push_back(tbf[i].GetName() );
		}
	}
}

void CShapeFileBase::GetFieldValues(int fieldIndex, StringVector& values)const
{
	values.clear();

	int size = m_infoDBF.GetNbRecord();
	for(int i=0; i<size; i++)
	{
		string itemName;
		if( fieldIndex >= 0)
			itemName= m_infoDBF[i][fieldIndex];
		else 
			itemName = ToString(i+1);

		//if( UtilWin::FindInArray( values, itemName) != -1)
		if (std::find(values.begin(), values.end(), itemName) != values.end() )
		{
			values.push_back( itemName );
		}	
	}
}
}