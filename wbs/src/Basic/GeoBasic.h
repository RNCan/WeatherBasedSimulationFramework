//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#ifndef __GEOBASIC_H
#define __GEOBASIC_H

#pragma once

#define NOMINMAX 
#include <vector>
#include <algorithm>
#include <boost\bimap.hpp>
#include <float.h>

#include "basic/ERMsg.h"
#include "basic/zenXml.h"
#include "Basic/UtilStd.h"
#include "Basic/UtilMath.h"


#ifdef _DEBUG
#define ASSERT_PRJ(x,y) ASSERT((x).GetPrjID()==(y).GetPrjID() || ((x).IsGeographic() && (y).IsGeographic()))
#else
	#define ASSERT_PRJ(x,y)
#endif


namespace WBSF
{
	static const double MISSING_NO_DATA = FLT_MAX;
	static const double MISSING_DATA = FLT_MAX;
	
	
	static const double EPSILON_NODATA = EPSILON_DATA;
	static const double EPSILON_COORDINATES = 0.0000000001;
	
	static const size_t PRJ_NOT_INIT = (size_t)-1;
	static const size_t PRJ_UNKNOWN = (size_t)-2;
	static const size_t PRJ_WGS_84 = 0;
	
	static const size_t PRJ_GEOGRAPHIC_BASE = 0;
	static const size_t PRJ_GEOCENTRIC_BASE = 100;
	static const size_t PRJ_PROJECTED_BASE = 1000;
	static const size_t PRJ_PROJECTED_END = 10000;
	static const char* PRJ_WGS_84_WKT = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9108\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
	static const char* PRJ_WGS_84_PROJ4 = "+proj=longlat";
	

	inline bool IsInit(size_t ID)		{ return ID != PRJ_NOT_INIT&&ID!=PRJ_UNKNOWN; }
	inline bool IsUnknown(size_t ID)	{ return ID == PRJ_NOT_INIT || ID == PRJ_UNKNOWN; }
	inline bool IsGeographic(size_t ID)	{return ID<PRJ_GEOCENTRIC_BASE; }
	inline bool IsGeocentric(size_t ID)	{ return ID >= PRJ_GEOCENTRIC_BASE && ID<PRJ_PROJECTED_BASE; }
	inline bool IsProjected(size_t ID)	{ return ID >= PRJ_PROJECTED_BASE && ID < PRJ_PROJECTED_END; }
	ERMsg BuildVRT(std::string filePath, StringVector fileList, bool bQuiet);

//*****************************************************************************************************
//CProjectionNameManager

	class CProjectionNameManager
	{
		public:

			typedef  std::map<std::string, std::string> EquivalentPrj;
			typedef  boost::bimap<std::string, size_t> LinkMap;
			typedef  LinkMap::value_type  Link;

			static CProjectionNameManager& GetInstance()
			{
				static CProjectionNameManager INSTANCE; // Guaranteed to be destroyed.
				// Instantiated on first use.
				return INSTANCE;
			}

			
			static void SetEquivalent(const char* prjStr1, const char* prjStr2)
			{
				GetInstance().m_equivalentPrj[prjStr1] = prjStr2;
			}

			static size_t Set(const char* prjStr)
			{
				if( prjStr!=NULL && strlen(prjStr)>0 )
				{
					EquivalentPrj::const_iterator eq = GetInstance().m_equivalentPrj.find(prjStr);
					LinkMap::left_const_iterator it = GetInstance().m_links.left.end();
					if (eq == GetInstance().m_equivalentPrj.end())
						it = GetInstance().m_links.left.find(prjStr);
					else
						it = GetInstance().m_links.left.find(GetInstance().m_equivalentPrj[prjStr]);
					
					if( it == GetInstance().m_links.left.end())
					{
						int baseId = PRJ_GEOGRAPHIC_BASE;
						
						if (Find(prjStr, std::string("GEOCCS")))
							baseId = PRJ_GEOCENTRIC_BASE;
						else if (Find(prjStr, std::string("PROJCS")))
							baseId = PRJ_PROJECTED_BASE;
						else if (Find(prjStr, std::string("+proj")))
						{
							if (!Find(prjStr, std::string("latlong")) && !Find(prjStr, std::string("longlat")))
								baseId = PRJ_PROJECTED_BASE;
						}
							
						
							
						GetInstance().m_links.insert(Link(prjStr, baseId + GetInstance().m_links.size()));
						it = GetInstance().m_links.left.find(prjStr);
					}

					return it->second;
				}

				return PRJ_NOT_INIT;
			}

			static bool Exists(size_t prjID)
			{
				LinkMap::right_const_iterator it = GetInstance().m_links.right.find(prjID);
				return it !=  GetInstance().m_links.right.end();
			}

			static bool Exists(const char* prjStr )
			{
				EquivalentPrj::const_iterator eq = GetInstance().m_equivalentPrj.find(prjStr);
				LinkMap::left_const_iterator it = GetInstance().m_links.left.end();
				if (eq == GetInstance().m_equivalentPrj.end())
					it = GetInstance().m_links.left.find(prjStr);
				else
					it = GetInstance().m_links.left.find(GetInstance().m_equivalentPrj[prjStr]);

				//LinkMap::left_const_iterator it = GetInstance().m_links.left.find(prjStr);
				return it !=  GetInstance().m_links.left.end();
			}		

			static size_t GetPrjID(const char* prjStr )
			{
				if( prjStr!=NULL && strlen(prjStr)>0 )
				{
					EquivalentPrj::const_iterator eq = GetInstance().m_equivalentPrj.find(prjStr);
					LinkMap::left_const_iterator it = GetInstance().m_links.left.end();
					if (eq == GetInstance().m_equivalentPrj.end())
						it = GetInstance().m_links.left.find(prjStr);
					else
						it = GetInstance().m_links.left.find(GetInstance().m_equivalentPrj[prjStr]);

					//LinkMap::left_const_iterator it = GetInstance().m_links.left.find(prjStr);
					if( it ==  GetInstance().m_links.left.end())
						return PRJ_NOT_INIT;

					return it->second;
				}
				
				return PRJ_NOT_INIT;
			}

			static const char* GetPrjStr(size_t ID)
			{
				static const char* EMPTY_STR = "";

				if(ID!=PRJ_NOT_INIT)
				{
					LinkMap::right_const_iterator it = GetInstance().m_links.right.find(ID);
					ASSERT(it !=  GetInstance().m_links.right.end());

					if( it !=  GetInstance().m_links.right.end())
						return it->second.c_str();
				}

				return EMPTY_STR;
			}
		
		
		private:

			CProjectionNameManager() 
			{
				//Add geographic projection
				m_links.insert( Link( PRJ_WGS_84_WKT, PRJ_WGS_84 ) );
			};                   // Constructor? (the {} brackets) are needed here.
			// Dont forget to declare these two. You want to make sure they
			// are unaccessable otherwise you may accidently get copies of
			// your singleton appearing.
			CProjectionNameManager(CProjectionNameManager const&);	// Don't Implement
			void operator=(CProjectionNameManager const&);		// Don't implement

			LinkMap  m_links;
			EquivalentPrj m_equivalentPrj;
	};

	//class CProjectionTransformationManager;

//*****************************************************************************************************
//CGeoRef
	
	class CGeoRef
	{
	public:

		CGeoRef(size_t prjID=PRJ_NOT_INIT):m_prjID(prjID)	{}
		CGeoRef(const CGeoRef& in):m_prjID(in.m_prjID)	{}
		~CGeoRef(){}
		CGeoRef& operator =(const CGeoRef& in){m_prjID=in.m_prjID;return *this;}
		bool operator ==(const CGeoRef& in)const{ return m_prjID==in.m_prjID;}
		bool operator !=(const CGeoRef& in)const{ return !operator ==(in);}
		void clear(){m_prjID=PRJ_NOT_INIT;}

		
		size_t GetPrjID()const{ return m_prjID;}
		void SetPrjID(size_t prjID){m_prjID = prjID;}
		
		bool IsInit()const{return WBSF::IsInit(m_prjID);}
		bool IsUnknown()const{ return WBSF::IsUnknown(m_prjID); }
		bool IsGeographic()const{ return WBSF::IsGeographic(m_prjID); }
		bool IsGeocentric()const{ return WBSF::IsGeocentric(m_prjID); }
		bool IsProjected()const{ return WBSF::IsProjected(m_prjID); }


		//CGeoPoint GetCentroid()const; a faire
		


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_prjID;
		}
		friend boost::serialization::access;


		std::ostream& operator>>(std::ostream &s)const{	s << ((m_prjID!=PRJ_NOT_INIT)?m_prjID:-1);	return s;	}
		std::istream& operator<<(std::istream &s){s >> m_prjID; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoRef& pt){ pt>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoRef& pt){ pt<<s;	return s; }

		template <class T>
		void Reproject( const T& PT)
		{
			ASSERT( PT.GetSrc()->GetPrjID() == GetPrjID() );

			SetPrjID(PT.GetDst()->GetPrjID());
		}
		
		//bool ReprojectToGeographic();
		
	protected:
		
		size_t m_prjID;
	};


//*****************************************************************************************************
//CGeoPoint
	class CGeoPoint3D;
	class CGeoDistance;
	class CGeoPoint: public CGeoRef
	{
	public:

		union{double m_x;double m_lon;};
		union{double m_y;double m_lat;};

		CGeoPoint(size_t prjID=PRJ_NOT_INIT):CGeoRef(prjID ),m_x(0),m_y(0){}
		CGeoPoint(double x, double y, size_t prjID=PRJ_NOT_INIT):CGeoRef(prjID ),m_x(x),m_y(y){}
		CGeoPoint(const CGeoPoint& in) :CGeoRef(in), m_x(in.m_x), m_y(in.m_y){}
		CGeoPoint& operator =(const CGeoPoint& in) { CGeoRef::operator=(in); m_x = in.m_x;  m_y = in.m_y; return *this; }
		bool operator ==(const CGeoPoint& in)const{ ASSERT_PRJ((*this), in); return fabs(m_x-in.m_x)<EPSILON_COORDINATES && fabs(m_y-in.m_y)<EPSILON_COORDINATES;}
		bool operator !=(const CGeoPoint& in)const{ return !operator ==(in);}

		void clear(){ CGeoRef::clear();  m_x = 0; m_y = 0; };
		const double& operator () (size_t i)const{ return i<1 ? m_x : m_y; }
		double& operator () (size_t i){ return i <1 ? m_x : m_y; }
		CGeoPoint operator -( ) const{return CGeoPoint(-m_x, -m_y, m_prjID);}
		CGeoPoint& operator *=(double v){m_x*=v;m_y*=v;	return *this;	}
		CGeoPoint& operator /=(double v){m_x/=v;m_y/=v;	return *this;}
		CGeoPoint& operator +=(double v){m_x+=v;m_y+=v;	return *this;}
		CGeoPoint& operator +=(const CGeoDistance& pt);
		CGeoPoint& operator -=(const CGeoDistance& pt);

		friend CGeoDistance operator -(const CGeoPoint& pt1, const CGeoPoint& pt2);
		friend CGeoPoint operator +(const CGeoPoint& pt1, const CGeoDistance& pt2);
		friend CGeoPoint operator *(double value, const CGeoPoint& point)	{return CGeoPoint ( point.m_x*value, point.m_y*value, point.m_prjID );	}
		friend CGeoPoint operator /(double value , const CGeoPoint& point)	{return CGeoPoint ( point.m_x/value, point.m_y/value, point.m_prjID );	}
		friend CGeoPoint operator +(double value, const CGeoPoint& point)	{return CGeoPoint ( point.m_x+value, point.m_y+value, point.m_prjID );	}
		friend CGeoPoint operator -(double value , const CGeoPoint& point) 	{return CGeoPoint ( point.m_x-value, point.m_y-value, point.m_prjID );	}
		friend CGeoPoint operator *(const CGeoPoint& point, double value )	{return CGeoPoint ( point.m_x*value, point.m_y*value, point.m_prjID );	}
		friend CGeoPoint operator /(const CGeoPoint& point, double value )	{return CGeoPoint ( point.m_x/value, point.m_y/value, point.m_prjID );	}
		friend CGeoPoint operator +(const CGeoPoint& point, double value )	{return CGeoPoint ( point.m_x+value, point.m_y+value, point.m_prjID );	}
		friend CGeoPoint operator -(const CGeoPoint& point, double value )	{return CGeoPoint ( point.m_x-value, point.m_y-value, point.m_prjID );	}

		double GetDistance(const CGeoPoint& pt)const
		{
			//ASSERT_PRJ((*this), pt);

			double distance = 0;
			if( IsGeographic() )
			{
				ASSERT( pt.IsGeographic() );
				distance = WBSF::GetDistance(pt.m_y, pt.m_x, m_y, m_x);
			}
			else 
			{
				ASSERT( pt.IsProjected() );
				distance = sqrt( Square(m_x-pt.m_x) + Square(m_y-pt.m_y) );
			}
			return distance;
		}

		inline CGeoDistance GetDistanceXY(const CGeoPoint& pt)const;

		inline CGeoPoint3D GetGeocentric()const;
		inline void SetXY(const CGeoPoint3D& pt);
		void SetXY(double a, double b, double c)
		{
			m_prjID = PRJ_WGS_84;
			m_x = Rad2Deg(atan2(b, a)) - 180;
			m_y = 90 - Rad2Deg(acos(c / (6371 * 1000)));
		}

		
		static double GetDefaultArea(const CGeoPoint& pt1, const CGeoPoint& pt2){return -((pt1.m_x*pt2.m_y) - (pt1.m_y*pt2.m_x));}
		//-------------------------------------------------
		// GetArea()
		//
		// Permet de calculer l'aire entre trois points
		// On reverse le sense pour tenir compte des spécification des shapefile
		//-------------------------------------------------
		static double GetArea(const CGeoPoint& pt1, const CGeoPoint& pt2, const CGeoPoint& pt3)
		{
			ASSERT_PRJ(pt1,pt2);
			ASSERT_PRJ(pt2,pt3);
			return -((pt2.m_x*pt3.m_y) - (pt2.m_y*pt3.m_x) - (pt1.m_x*pt3.m_y) + (pt1.m_y*pt3.m_x) + (pt1.m_x*pt2.m_y) - (pt1.m_y*pt2.m_x));
		}

		static CGeoPoint GetDefaultCentroid(const CGeoPoint& pt1, const CGeoPoint& pt2)
		{
			ASSERT_PRJ(pt1,pt2);
			return CGeoPoint( (pt1.m_x+pt2.m_x)/3, (pt1.m_y+pt2.m_y)/3, pt1.GetPrjID());
		}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_x & m_y;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{	s << m_x << " " << m_y /*<< " " << ((CGeoRef&)*this)*/;	return s;	}
		std::istream& operator<<(std::istream &s){s >> m_x >> m_y /*>> ((CGeoRef&)*this)*/; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoPoint& pt){ pt>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoPoint& pt){ pt<<s;	return s; }

		template <class T>
		ERMsg Reproject(const T& PT)
		{
			CGeoRef::Reproject(PT);
			return PT.Reproject(1, &m_x, &m_y);
		}
	};

	class CGeoDistance : public CGeoPoint
	{
	public:
		CGeoDistance(size_t prjID = PRJ_NOT_INIT) :CGeoPoint(prjID){}
		CGeoDistance(double x, double y, size_t prjID = PRJ_NOT_INIT) : CGeoPoint(x, y, prjID){}
		CGeoDistance(const CGeoPoint& in) :CGeoPoint(in){}
	};

	inline CGeoPoint& CGeoPoint::operator += (const CGeoDistance& pt) { ASSERT_PRJ(*this, pt); m_x += pt.m_x; m_y += pt.m_y; return *this; }
	inline CGeoPoint& CGeoPoint::operator -=(const CGeoDistance& pt) { ASSERT_PRJ(*this, pt); m_x -= pt.m_x; m_y -= pt.m_y; return *this; }

	inline CGeoDistance operator -(const CGeoPoint& pt1, const CGeoPoint& pt2)	{ ASSERT_PRJ(pt1, pt2); return CGeoPoint(pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y, pt1.m_prjID); }
	inline CGeoPoint operator +(const CGeoPoint& pt1, const CGeoDistance& pt2)	{ ASSERT_PRJ(pt1, pt2); return CGeoPoint(pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y, pt1.m_prjID); }

	inline CGeoDistance CGeoPoint::GetDistanceXY(const CGeoPoint& pt)const
	{
		ASSERT_PRJ((*this), pt);
		CGeoDistance dist(0, 0, m_prjID);

		if (IsGeographic())
		{
			dist.m_y = Signe(pt.m_y - m_y)*WBSF::GetDistance(pt.m_y, m_x, m_y, m_x);
			dist.m_x = Signe(pt.m_x - m_x)*WBSF::GetDistance(m_y, pt.m_x, m_y, m_x);
		}
		else
		{
			dist.m_x = (pt.m_x - m_x);
			dist.m_y = (pt.m_y - m_y);
		}

		return dist;
	}

//*****************************************************************************************************
//CGeoSegment

	class CGeoDistance3D;

	class CGeoPoint3D : public CGeoPoint
	{
	public:
		
		union{ double m_z; double m_alt; double m_elev; };//altitude in meters


		CGeoPoint3D(size_t prjID = PRJ_NOT_INIT) :CGeoPoint(prjID), m_z(0){}
		CGeoPoint3D(double x, double y, double z, size_t prjID = PRJ_NOT_INIT) : CGeoPoint(x,y,prjID), m_z(z){}
		CGeoPoint3D(const CGeoPoint3D& in) :CGeoPoint(in), m_z(in.m_z){}
		CGeoPoint3D& operator =(const CGeoPoint3D& in) { CGeoPoint::operator=(in); m_z = in.m_z; return *this; }
		bool operator ==(const CGeoPoint3D& in)const{ ASSERT_PRJ((*this), in); return CGeoPoint::operator==(in) && fabs(m_z - in.m_z)<EPSILON_COORDINATES; }
		bool operator !=(const CGeoPoint3D& in)const{ return !operator ==(in); }

		void clear(){ CGeoRef::clear();  m_x = 0; m_y = 0; m_z = 0; };
		const double& operator () (size_t i)const{ return (i < 2 ? CGeoPoint::operator()(i) : m_z); }
		double& operator () (size_t i){ return i <2 ? CGeoPoint::operator()(i) : m_z; }
		CGeoPoint3D operator -() const{ return CGeoPoint3D(-m_x, -m_y, -m_z, m_prjID); }
		CGeoPoint3D& operator *=(double v){ m_x *= v; m_y *= v; m_z *= v;	return *this; }
		CGeoPoint3D& operator /=(double v){ m_x /= v; m_y /= v; m_z /= v;	return *this; }
		CGeoPoint3D& operator +=(double v){ m_x += v; m_y += v; m_z += v;	return *this; }
		//CGeoPoint3D& operator +=(const CGeoPoint3D& pt) { ASSERT_PRJ(*this, pt); m_x += pt.m_x; m_y += pt.m_y; m_z += pt.m_z; return *this; }
		inline CGeoPoint3D& operator +=(const CGeoDistance3D& pt);
		CGeoPoint3D& operator -=(const CGeoDistance3D& pt);

		friend CGeoDistance3D operator -(const CGeoPoint3D& pt1, const CGeoPoint3D& pt2);
		friend CGeoPoint3D operator +(const CGeoPoint3D& pt1, const CGeoDistance3D& pt2);
		friend CGeoPoint3D operator *(double value, const CGeoPoint3D& point)	{ return CGeoPoint3D(point.m_x*value, point.m_y*value, point.m_z*value, point.m_prjID); }
		friend CGeoPoint3D operator /(double value, const CGeoPoint3D& point)	{ return CGeoPoint3D(point.m_x / value, point.m_y / value, point.m_z / value, point.m_prjID); }
		friend CGeoPoint3D operator +(double value, const CGeoPoint3D& point)	{ return CGeoPoint3D(point.m_x + value, point.m_y + value, point.m_z + value, point.m_prjID); }
		friend CGeoPoint3D operator -(double value, const CGeoPoint3D& point) 	{ return CGeoPoint3D(point.m_x - value, point.m_y - value, point.m_z - value, point.m_prjID); }
		friend CGeoPoint3D operator *(const CGeoPoint3D& point, double value)	{ return CGeoPoint3D(point.m_x*value, point.m_y*value, point.m_z*value, point.m_prjID); }
		friend CGeoPoint3D operator /(const CGeoPoint3D& point, double value)	{ return CGeoPoint3D(point.m_x / value, point.m_y / value, point.m_z / value, point.m_prjID); }
		friend CGeoPoint3D operator +(const CGeoPoint3D& point, double value)	{ return CGeoPoint3D(point.m_x + value, point.m_y + value, point.m_z + value, point.m_prjID); }
		friend CGeoPoint3D operator -(const CGeoPoint3D& point, double value)	{ return CGeoPoint3D(point.m_x - value, point.m_y - value, point.m_z - value, point.m_prjID); }

		double GetDistance(const CGeoPoint3D& pt)const
		{
			double distance = 0;
			if (IsGeographic())
			{
				ASSERT(pt.IsGeographic());
				distance = sqrt(Square(WBSF::GetDistance(pt.m_y, pt.m_x, m_y, m_x)) + Square(m_z - pt.m_z));
			}
			else
			{
				ASSERT(pt.IsProjected());
				distance = sqrt(Square(m_x - pt.m_x) + Square(m_y - pt.m_y) + Square(m_z - pt.m_z) );
			}
			return distance;
		}





		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint>(*this);
			ar & m_z;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{ CGeoPoint::operator>>(s);  s << " " << m_z;	return s; }
		std::istream& operator<<(std::istream &s){ CGeoPoint::operator<<(s);  s >> m_z; return s; }
		friend std::ostream& operator<<(std::ostream &s, const CGeoPoint3D& pt){ pt >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CGeoPoint3D& pt){ pt << s;	return s; }


		template <class T>
		ERMsg Reproject(const T& PT)
		{
			CGeoRef::Reproject(PT);
			return PT.Reproject(1, &m_x, &m_y, &m_z);
		}
	};

	class CGeoDistance3D : public CGeoPoint3D
	{
	public:

		CGeoDistance3D(size_t prjID = PRJ_NOT_INIT) :CGeoPoint3D(prjID){}
		CGeoDistance3D(double x, double y, double z, size_t prjID = PRJ_NOT_INIT) : CGeoPoint3D(x, y, z, prjID){}
		CGeoDistance3D(const CGeoPoint3D& in) :CGeoPoint3D(in){}
		//CGeoDistance3D(const CGeoDistance3D& in) :CGeoPoint3D(in){}
	};

	inline CGeoPoint3D& CGeoPoint3D::operator += (const CGeoDistance3D& pt) { ASSERT_PRJ(*this, pt); m_x += pt.m_x; m_y += pt.m_y; m_z += pt.m_z; return *this; }
	inline CGeoPoint3D& CGeoPoint3D::operator -= (const CGeoDistance3D& pt) { ASSERT_PRJ(*this, pt); m_x -= pt.m_x; m_y -= pt.m_y; m_z -= pt.m_z; return *this; }
	inline CGeoPoint3D operator + (const CGeoPoint3D& pt1, const CGeoDistance3D& pt2)	{ ASSERT_PRJ(pt1, pt2); return CGeoPoint3D(pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y, pt1.m_z + pt2.m_z, pt1.m_prjID); }
	inline CGeoDistance3D operator -(const CGeoPoint3D& pt1, const CGeoPoint3D& pt2)	{ ASSERT_PRJ(pt1, pt2); return CGeoDistance3D(pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y, pt1.m_z - pt2.m_z, pt1.m_prjID); }
//*****************************************************************************************************
//CGeoSegment
	
	class CGeoSegment: public CGeoRef
	{
	public:

		CGeoPoint m_pt1;
		CGeoPoint m_pt2;

		CGeoSegment(){};
		CGeoSegment(const CGeoPoint& pt1, const CGeoPoint& pt2){m_pt1=pt1;m_pt2=pt2;};

		void clear(){m_pt1.clear();m_pt2.clear();};
		const CGeoPoint& GetPt1()const{return m_pt1;};
		void SetPt1(const CGeoPoint& pt){m_pt1=pt;};
		const CGeoPoint& GetPt2()const{return m_pt2;};
		void SetPt2(const CGeoPoint& pt){m_pt2=pt;};

		//-------------------------------------------------
		// GetArea()
		//
		// Permet de calculer l'aire entre un segment et un point
		//-------------------------------------------------
		double GetArea(const CGeoPoint& pt)const{			return CGeoPoint::GetArea( m_pt1, m_pt2, pt);}

		//-------------------------------------------------
		// SegmentIntersect()
		//
		// Permet de déterminer si deux segments se croisent
		//-------------------------------------------------
		bool IsSegmentNull()const	{ return m_pt1.m_x == 0 && m_pt1.m_y == 0 && m_pt2.m_x == 0 && m_pt2.m_y == 0; }
		bool IsSegmentEmpty()const	{ return m_pt1 == m_pt2; }
		bool IsSegmentIntersect(const CGeoSegment& segment)const 
		{
			bool bSegmentIntersect = false;
			//On calcul l'aire des deux triangles
			double aire1 = GetArea(segment.m_pt1);
			double aire2 = GetArea(segment.m_pt2);
    
			//On vérifie les quatres cas de non-intersection
			if ( !(aire1 > 0 && aire2 > 0) && !(aire1 < 0 && aire2 < 0) )
			{
				aire1 = segment.GetArea(m_pt1);
				aire2 = segment.GetArea(m_pt2);
				if ( !(aire1 > 0 && aire2 > 0) && !(aire1 < 0 && aire2 < 0) )
					bSegmentIntersect = true;
			}

			return bSegmentIntersect;
		}

		//-------------------------------------------------
		// GetIntersectionPoint()
		//
		// Permet d'obtenir le point d'intersection entre deux segments
		//-------------------------------------------------
		CGeoPoint GetIntersectionPoint(const CGeoSegment& segment)const
		{
			//On calcul lambda
			double lambda = GetArea(segment.m_pt1) - GetArea(segment.m_pt2);

			//Si lambda est différent de zéro
			if (fabs(lambda) > EPSILON_COORDINATES )
				lambda = GetArea(segment.m_pt1) / lambda;
			else
				lambda = 1;
	
			//On trouve le point d'intersection
			return CGeoPoint ((1 - lambda) * segment.m_pt1.m_x + lambda * segment.m_pt2.m_x, (1 - lambda) * segment.m_pt1.m_y + lambda * segment.m_pt2.m_y, segment.GetPrjID());
		}

		template <class T>
		ERMsg Reproject(const T& PT)
		{
			ERMsg msg;
			CGeoRef::Reproject(PT);
			msg += m_pt1.Reproject(PT);
			msg += m_pt2.Reproject(PT);
			return msg;
		}
	};


//*****************************************************************************************************
//CGeoRect

	class CGeoRect: public CGeoRef
	{
	public:

		double m_xMin; 
		double m_xMax; 
		double m_yMin; 
		double m_yMax;

		CGeoRect(size_t prjID = PRJ_NOT_INIT) :CGeoRef(prjID), m_xMin(-DBL_MAX), m_yMin(-DBL_MAX), m_xMax(-DBL_MAX), m_yMax(-DBL_MAX){}
		CGeoRect(double xMin, double yMin, double xMax, double yMax, size_t prjID=PRJ_NOT_INIT):CGeoRef(prjID),m_xMin(xMin),m_yMin(yMin),m_xMax(xMax),m_yMax(yMax)
		{NormalizeRect();}

		CGeoRect(const CGeoPoint& pt1, const CGeoPoint& pt2, size_t prjID=PRJ_NOT_INIT):CGeoRef(prjID),m_xMin(pt1.m_x),m_yMin(pt1.m_y),m_xMax(pt2.m_x),m_yMax(pt2.m_y)
		{NormalizeRect();}
		
		CGeoRect(const CGeoRect& in){ operator =(in);	}

		CGeoRect& operator =(const CGeoRect& in)
		{
			if( &in != this)
			{
				CGeoRef::operator =(in);
				m_xMin = in.m_xMin;
				m_yMin = in.m_yMin;
				m_xMax = in.m_xMax;
				m_yMax = in.m_yMax;
			}

			return *this;
		}

		bool operator ==(const CGeoRect& in)const
		{
			//ASSERT_PRJ((*this), in);

			bool bEqual=false;
			//if( CGeoRef::operator ==(in) )
				if( fabs( m_xMin - in.m_xMin) < EPSILON_COORDINATES)
					if( fabs( m_xMax - in.m_xMax) < EPSILON_COORDINATES)
						if( fabs(m_yMin - in.m_yMin) < EPSILON_COORDINATES)
							if( fabs(m_yMax - in.m_yMax) < EPSILON_COORDINATES)
								bEqual = true;

			return bEqual;
		}

		bool operator !=(const CGeoRect& in)const{ return !operator ==(in);}
		

		void clear(){ m_xMin = m_xMax = m_yMin = m_yMax = -DBL_MAX; }
		void Reset(){ clear(); }
		void SetRectEmpty()	{	m_xMin = m_xMax = m_yMin = m_yMax = 0;	}


		double Width()const{ return m_xMax - m_xMin; }
		double Height()const{ return m_yMax - m_yMin; }
		double Left()const{ return std::min(m_xMax, m_xMin);}
		double Top()const{ return std::max(m_yMax, m_yMin); }
		CGeoPoint UpperRight()const{return CGeoPoint(m_xMax, m_yMax, m_prjID);}
		CGeoPoint LowerLeft()const{return CGeoPoint(m_xMin,m_yMin, m_prjID);}
		CGeoPoint LowerRight()const{return CGeoPoint(m_xMax, m_yMin, m_prjID);}
		CGeoPoint UpperLeft()const{return CGeoPoint(m_xMin, m_yMax, m_prjID);}
		CGeoPoint GetCentroid()const{ return CGeoPoint((m_xMin + m_xMax) / 2, (m_yMin + m_yMax)/2, m_prjID); }

		void NormalizeRect()
		{
			if (m_xMin > m_xMax)
				Switch(m_xMin, m_xMax);
			if (m_yMin > m_yMax)
				Switch(m_yMin, m_yMax);
		}

		CGeoRect& UnionRect(const CGeoRect& rect);
		CGeoRect& IntersectRect(const CGeoRect& rect);
		
		static CGeoRect UnionRect(const CGeoRect& rect1, const CGeoRect& rect2){ return CGeoRect(rect1).UnionRect(rect2); }
		static CGeoRect IntersectRect(const CGeoRect& rect1, const CGeoRect& rect2){ return CGeoRect(rect1).IntersectRect(rect2); }

		void Inflate(float percent);
		char GetTestPosition(const double& x, const double& y)const
		{
			char tmp = 0;
			if(x < m_xMin) tmp|=0x01; //0001;
			if(x > m_xMax) tmp|=0x02; //0010;
			if(y < m_yMin) tmp|=0x04; //0100;
			if(y > m_yMax) tmp|=0x08; //1000;
	
			return tmp;
		}

		char GetTestPosition(const CGeoPoint& pt)const{	return GetTestPosition(pt.m_x, pt.m_y);	}

		CGeoRect& operator -=(const CGeoPoint& point){ return OffsetRect( -point );		}
		CGeoRect& operator +=(const CGeoPoint& point){ return OffsetRect( point );		}
		CGeoRect& operator -=(double offset){ return operator+=(-offset); }
		CGeoRect& operator +=(double offset){ m_xMin += offset;m_xMax += offset;m_yMin += offset;m_yMax += offset;return *this;	}
		CGeoRect& InflateRect( double w, double n, double e, double s );
		CGeoRect& ExtendBounds(const CGeoPoint& pt);
		CGeoRect& ExtendBounds(const CGeoRect& box);
		CGeoRect& OffsetRect(const CGeoPoint& pt);

		bool IsInit()const{ return CGeoRef::IsInit() || m_xMin != -DBL_MAX || m_xMax != -DBL_MAX || m_yMin != -DBL_MAX || m_yMax != -DBL_MAX; }
		bool IsInside(const CGeoPoint& pt)const{return PtInRect(pt);}
		bool IsInside(const CGeoRect& rect)const{ ASSERT_PRJ((*this), rect); return rect.m_xMin >= m_xMin && rect.m_xMin <= m_xMax && rect.m_xMax >= m_xMin && rect.m_xMax <= m_xMax && rect.m_yMin >= m_yMin && rect.m_yMin <= m_yMax && rect.m_yMax >= m_yMin && rect.m_yMax <= m_yMax; }
		bool PtInRect(const CGeoPoint& pt)const{ASSERT_PRJ((*this), pt); return pt.m_x>=m_xMin && pt.m_x<=m_xMax && pt.m_y>=m_yMin && pt.m_y<=m_yMax;}
		bool IsRectEmpty()const{return fabs( m_xMin-m_xMax) < EPSILON_COORDINATES || fabs(m_yMin-m_yMax) < EPSILON_COORDINATES;}
		bool IsEmpty()const{ return IsRectEmpty(); }
		bool IsRectNormal()const{return (m_xMin <= m_xMax) && (m_yMin <= m_yMax);}
		bool IsNormal()const{ return IsRectNormal(); }
		bool IsRectNull( ) const{return (m_xMin==0 && m_yMax==0 && m_xMax==0 && m_yMin==0);	}
		bool IsNull() const{ return IsRectNull(); }
		bool IsRectIntersect( const CGeoRect& rect)const{ASSERT_PRJ((*this), rect);return !(GetTestPosition(rect.LowerLeft()) & GetTestPosition(rect.UpperRight()));	}
		bool IsIntersect(const CGeoRect& rect)const{ return IsRectIntersect(rect); }
		


		//static double Min(double a, double b){return a<b?a:b;}
		//static double Max(double a, double b){return a>b?a:b;}


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_xMin & m_xMax & m_yMin & m_yMax;
		}
		friend boost::serialization::access;


		
		std::ostream& operator>>(std::ostream &s)const{ s << m_xMin << " " << m_xMax << " " << m_yMin << " " << m_yMax; return s; }
		std::istream& operator<<(std::istream &s){ s >> m_xMin >> m_xMax >> m_yMin >> m_yMax; return s; } /*>>((CGeoRef&)*this)*/
		friend std::ostream& operator<<(std::ostream &s, const CGeoRect& rect){ rect>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoRect& rect){ rect<<s;	return s; }

		template <class T>
		ERMsg Reproject(const T& PT)
		{
			ERMsg msg;

			//T const& prj1 = prjManager.GetPrj(GetPrjID());
			//T const& prj2 = prjManager.GetPrj(geoRef.GetPrjID());
			//a faire une série de point sur le rectangle...
			CGeoRef::Reproject(PT);
			msg += PT.Reproject(1, &m_xMin, &m_yMin);
			msg += PT.Reproject(1, &m_xMax, &m_yMax);

			return msg;
		}
	};

	//*****************************************************************************************************
	//CGeoRect3D

	class CGeoRect3D : public CGeoRect
	{
	public:

		double m_zMin;
		double m_zMax;

		CGeoRect3D(size_t prjID = PRJ_NOT_INIT) :CGeoRect(prjID), m_zMin(-DBL_MAX), m_zMax(-DBL_MAX){}
		CGeoRect3D(double xMin, double yMin, double zMin, double xMax, double yMax, double zMax, size_t prjID = PRJ_NOT_INIT) :CGeoRect(xMin, yMin, xMax, yMax, prjID), m_zMin(zMin), m_zMax(zMax)
		{
			NormalizeRect();
		}

		CGeoRect3D(const CGeoPoint3D& pt1, const CGeoPoint3D& pt2) : CGeoRect(pt1.m_x, pt1.m_y, pt2.m_x, pt2.m_y, pt1.GetPrjID()), m_zMin(pt1.m_z), m_zMax(pt2.m_z)
		{
			NormalizeRect();
		}

		CGeoRect3D(const CGeoRect3D& in){ operator =(in); }

		CGeoRect3D& operator =(const CGeoRect3D& in)
		{
			if (&in != this)
			{
				CGeoRect::operator =(in);
				m_zMin = in.m_zMin;
				m_zMax = in.m_zMax;
			}

			return *this;
		}

		bool operator ==(const CGeoRect3D& in)const
		{
			bool bEqual = false;
			if( CGeoRect::operator ==(in) )
			if (fabs(m_zMin - in.m_zMin) < EPSILON_COORDINATES)
				if (fabs(m_zMax - in.m_zMax) < EPSILON_COORDINATES)
					bEqual = true;

			return bEqual;
		}

		bool operator !=(const CGeoRect3D& in)const{ return !operator ==(in); }


		void clear(){ CGeoRect::clear();  m_zMin = m_zMax = -DBL_MAX; }
		void Reset(){ clear(); }
		void SetRectEmpty()	{ CGeoRect::SetRectEmpty();  m_zMin = m_zMax = 0; }


		double Thickness()const{ return m_zMax - m_zMin; }
		
		//double Left()const{ return Min(m_xMax, m_xMin); }
		//double Top()const{ return Max(m_yMax, m_yMin); }
		//CGeoPoint UpperRight()const{ return CGeoPoint(m_xMax, m_yMax, m_prjID); }
		//CGeoPoint LowerLeft()const{ return CGeoPoint(m_xMin, m_yMin, m_prjID); }
		//CGeoPoint LowerRight()const{ return CGeoPoint(m_xMax, m_yMin, m_prjID); }
		//CGeoPoint UpperLeft()const{ return CGeoPoint(m_xMin, m_yMax, m_prjID); }
		CGeoPoint3D GetCentroid()const{ return CGeoPoint3D((m_xMin + m_xMax) / 2, (m_yMin + m_yMax) / 2, (m_zMin + m_zMax) / 2, m_prjID); }

		void NormalizeRect()
		{
			CGeoRect::NormalizeRect();

			if (m_zMin > m_zMax)
				Switch(m_zMin, m_zMax);
		}



		
		CGeoRect3D& UnionRect(const CGeoRect3D& rect);
		CGeoRect3D& IntersectRect(const CGeoRect3D& rect);

		static CGeoRect3D UnionRect(const CGeoRect3D& rect1, const CGeoRect3D& rect2){ return CGeoRect3D(rect1).UnionRect(rect2); }
		static CGeoRect3D IntersectRect(const CGeoRect3D& rect1, const CGeoRect3D& rect2){ return CGeoRect3D(rect1).IntersectRect(rect2); }

		//void Inflate(float percent);
		char GetTestPosition(const double& x, const double& y, const double& z)const
		{
			char tmp = 0;
			if (x < m_xMin) tmp |= 0x01; //000001;
			if (x > m_xMax) tmp |= 0x02; //000010;
			if (y < m_yMin) tmp |= 0x04; //000100;
			if (y > m_yMax) tmp |= 0x08; //001000;
			if (z < m_zMin) tmp |= 0x10; //010000;
			if (z > m_zMax) tmp |= 0x20; //100000;

			return tmp;
		}

		char GetTestPosition(const CGeoPoint3D& pt)const{ return GetTestPosition(pt.m_x, pt.m_y, pt.m_z); }

		CGeoRect3D& operator -=(const CGeoPoint3D& point){ return OffsetRect(-point); }
		CGeoRect3D& operator +=(const CGeoPoint3D& point){ return OffsetRect(point); }
		CGeoRect3D& operator -=(double offset){ return operator+=(-offset); }
		CGeoRect3D& operator +=(double offset){ m_xMin += offset; m_xMax += offset; m_yMin += offset; m_yMax += offset; m_zMin += offset; m_zMax += offset; return *this; }
//CGeoRect3D& InflateRect(double w, double n, double e, double s);
		CGeoRect3D& ExtendBounds(const CGeoPoint3D& pt);
		CGeoRect3D& ExtendBounds(const CGeoRect3D& box);
		CGeoRect3D& OffsetRect(const CGeoPoint3D& pt);

		bool IsInit()const{ return CGeoRect::IsInit() || m_zMin != -DBL_MAX || m_zMax; }
		bool IsInside(const CGeoPoint3D& pt)const{ return PtInRect(pt); }
		bool IsInside(const CGeoRect3D& rect)const{ return CGeoRect::IsInside(rect) && rect.m_zMin >= m_zMin && rect.m_zMin <= m_zMax; }
		//bool PtInRect(const CGeoPoint& pt)const{ ASSERT_PRJ((*this), pt); return pt.m_x >= m_xMin && pt.m_x <= m_xMax && pt.m_y >= m_yMin && pt.m_y <= m_yMax; }
		bool IsEmpty()const{ return CGeoRect::IsEmpty() && ( fabs(m_zMin - m_zMax) < EPSILON_COORDINATES ); }
		bool IsNormal()const{ return CGeoRect::IsNormal() && (m_zMin <= m_zMax); }
		bool IsNull() const{ return CGeoRect::IsNull()&& (m_zMin == 0 && m_zMax == 0); }
		//bool IsRectIntersect(const CGeoRect3D& rect)const{ return !(GetTestPosition(rect.LowerLeft()) & GetTestPosition(rect.UpperRight())); }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoRect>(*this);
			ar & m_zMin & m_zMin;
		}
		friend boost::serialization::access;


		std::ostream& operator>>(std::ostream &s)const{ CGeoRect::operator>>(s);  s << " " << m_zMin << " " << m_zMax; return s; }
		std::istream& operator<<(std::istream &s){ CGeoRect::operator<<(s);  s >> m_zMin >> m_zMax; return s; }
		friend std::ostream& operator<<(std::ostream &s, const CGeoRect3D& rect){ rect >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CGeoRect3D& rect){ rect << s;	return s; }

		template <class T>
		ERMsg Reproject(const T& PT)
		{
			ERMsg msg;

		//	T const& prj1 = prjManager.GetPrj(GetPrjID());
			//T const& prj2 = prjManager.GetPrj(geoRef.GetPrjID());

			CGeoRef::Reproject(PT);
			msg += PT.Reproject(1, &m_xMin, &m_yMin, &m_zMin);
			msg += PT.Reproject(1, &m_xMax, &m_yMax, &m_zMax);

			return msg;
		}
	};


		
//*****************************************************************************************************
//CGeoVector

	template<class U>
	class CGeoVector: public std::vector<U>
	{
	public:
		
		template <class T>
		ERMsg Reproject( const T& PT)
		{
			ERMsg msg;

			for (std::vector<U>::iterator it = begin(); it != end(); it++)
				msg += it->Reproject(PT);

			return msg;
		}

		size_t GetPrjID()const{ return empty()?PRJ_NOT_INIT:front().GetPrjID();}
		void SetPrjID(size_t ID)
		{
			for (std::vector<U>::iterator it = begin(); it != end(); it++)
				it->SetPrjID(ID);
		}
	};

	typedef CGeoVector<CGeoPoint> CGeoPointVector;
	typedef CGeoVector<CGeoRect> CGeoRectVector;
//*****************************************************************************************************
//Size

	/*class CGeoSize
	{
	public:

		int m_x;
		int m_y;

		CGeoSize(int xSize = 0, int ySize = 0) :m_x(xSize), m_y(ySize){}
		void clear(){ m_x = 0; m_y = 0; };
		bool operator ==(const CGeoSize& in)const{return m_x==in.m_x&&m_y==in.m_y;}
		bool operator !=(const CGeoSize& in)const{return operator==(in); }

		CGeoSize& operator *=(int v )	{m_x*=v;m_y*=v;	return *this;}
		CGeoSize& operator /=(int v )	{m_x=(int)ceil((double)m_x/v);m_y=(int)ceil((double)m_y/v);	return *this;}//ceil by default
		CGeoSize& operator +=(int v )	{m_x+=v;m_y+=v;return *this;}
		CGeoSize& operator -=(int v )	{m_x-=v;m_y-=v;return *this;}
	
	
		CGeoSize& operator +=(const CGeoSize& v )	{m_x+=v.m_x;m_y+=v.m_y;return *this;}
		CGeoSize& operator -=(const CGeoSize&  v )	{m_x-=v.m_x;m_y-=v.m_y;return *this;}
		CGeoSize& operator *=(const CGeoSize&  v )	{m_x*=v.m_x;m_y*=v.m_y;return *this;}
		CGeoSize& operator /=(const CGeoSize&  v )	{m_x=(int)ceil((double)m_x/v.m_x);m_y=(int)ceil((double)m_y/v.m_y);return *this;}//ceil by default
		
	
		friend CGeoSize operator -(const CGeoSize& pt1, const CGeoSize& pt2){return CGeoSize( pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y );}
		friend CGeoSize operator +(const CGeoSize& pt1, const CGeoSize& pt2){return CGeoSize( pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y );}
		friend CGeoSize operator *(const CGeoSize& pt1, const CGeoSize& pt2){return CGeoSize( pt1.m_x * pt2.m_x, pt1.m_y * pt2.m_y );}
		friend CGeoSize operator /(const CGeoSize& pt1, const CGeoSize& pt2){return CGeoSize( (int)ceil((double)pt1.m_x / pt2.m_x), (int)ceil((double)pt1.m_y / pt2.m_y) );}//ceil by default
		friend CGeoSize operator *(int value, const CGeoSize& point) {return CGeoSize ( point.m_x*value, point.m_y*value );}
		friend CGeoSize operator /(int value, const CGeoSize& point) {return CGeoSize ( point.m_x/value, point.m_y/value );}
		friend CGeoSize operator +(int value, const CGeoSize& point) {return CGeoSize ( point.m_x+value, point.m_y+value );}
		friend CGeoSize operator -(int value, const CGeoSize& point) {return CGeoSize ( point.m_x-value, point.m_y-value );}
		friend CGeoSize operator *(const CGeoSize& point, int value) {return CGeoSize ( point.m_x*value, point.m_y*value );}
		friend CGeoSize operator /(const CGeoSize& point, int value) {return CGeoSize ( point.m_x/value, point.m_y/value );}
		friend CGeoSize operator +(const CGeoSize& point, int value) {return CGeoSize ( point.m_x+value, point.m_y+value );}
		friend CGeoSize operator -(const CGeoSize& point, int value) {return CGeoSize ( point.m_x-value, point.m_y-value );}

		friend CGeoSize operator *(double v, const CGeoSize& point) {return CGeoSize ( (int)ceil(point.m_x*v), (int)ceil(point.m_y*v ));}
		friend CGeoSize operator /(double v, const CGeoSize& point) {return CGeoSize ( (int)ceil(point.m_x/v), (int)ceil(point.m_y/v ));}
		friend CGeoSize operator +(double v, const CGeoSize& point) {return CGeoSize ( (int)ceil(point.m_x+v), (int)ceil(point.m_y+v ));}
		friend CGeoSize operator -(double v, const CGeoSize& point) {return CGeoSize ( (int)ceil(point.m_x-v), (int)ceil(point.m_y-v ));}
		friend CGeoSize operator *(const CGeoSize& point, double v) {return CGeoSize ( (int)ceil(point.m_x*v), (int)ceil(point.m_y*v ));}
		friend CGeoSize operator /(const CGeoSize& point, double v) {return CGeoSize ( (int)ceil(point.m_x/v), (int)ceil(point.m_y/v ));}
		friend CGeoSize operator +(const CGeoSize& point, double v) {return CGeoSize ( (int)ceil(point.m_x+v), (int)ceil(point.m_y+v ));}
		friend CGeoSize operator -(const CGeoSize& point, double v) {return CGeoSize ( (int)ceil(point.m_x-v), (int)ceil(point.m_y-v ));}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_x & m_y;
		}

		std::ostream& operator>>(std::ostream &s)const{ s << m_x << " " << m_y ;	return s; }
		std::istream& operator<<(std::istream &s){s >> m_x >> m_y; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoSize& size){ size>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoSize& size){ size<<s;	return s; }
	};

	class CGeoSize3D : public CGeoSize
	{
	public:
		int m_z;

		CGeoSize3D(int x = -1, int y = -1, int z = -1) : CGeoSize(x, y), m_z(z)	{}

		void clear(){ CGeoSize::clear(); m_z = -1; }
		bool operator ==(const CGeoSize3D& in)const{ return CGeoSize::operator==(in) && m_z == in.m_z; }
		bool operator !=(const CGeoSize3D& in)const{ return !operator==(in); }

		CGeoSize3D operator -() const{ return CGeoSize3D(-m_x, -m_y, -m_z); }
		CGeoSize3D& operator *=(int in) { m_x *= in; m_y *= in;	m_z *= in;	return *this; }
		CGeoSize3D& operator /=(int in)	{ m_x /= in; m_y /= in; m_z /= in;	return *this; }
		CGeoSize3D& operator +=(int in) { m_x += in; m_y += in;	m_z += in;	return *this; }
		CGeoSize3D& operator -=(int in)	{ m_x -= in; m_y -= in; m_z -= in;	return *this; }
		bool operator >(const CGeoSize3D& in){ return m_x > in.m_x  && m_y>in.m_y && m_z > in.m_z; }
		bool operator >=(const CGeoSize3D& in){ return m_x >= in.m_x  && m_y >= in.m_y&& m_z >= in.m_z; }
		bool operator <(const CGeoSize3D& in){ return m_x < in.m_x  && m_y<in.m_y&& m_z < in.m_z; }
		bool operator <=(const CGeoSize3D& in){ return m_x <= in.m_x  && m_y <= in.m_y&& m_z <= in.m_z; }

		friend CGeoSize3D operator -(const CGeoSize3D& pt1, const CGeoSize3D& pt2){ return CGeoSize3D(pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y, pt1.m_z - pt2.m_z); }
		friend CGeoSize3D operator +(const CGeoSize3D& pt1, const CGeoSize3D& pt2){ return CGeoSize3D(pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y, pt1.m_z + pt2.m_z); }
		friend CGeoSize3D operator *(int value, const CGeoSize3D& point) { return CGeoSize3D(point.m_x*value, point.m_y*value, point.m_z*value); }
		friend CGeoSize3D operator /(int value, const CGeoSize3D& point) { return CGeoSize3D(point.m_x / value, point.m_y / value, point.m_z / value); }
		friend CGeoSize3D operator +(int value, const CGeoSize3D& point) { return CGeoSize3D(point.m_x + value, point.m_y + value, point.m_z + value); }
		friend CGeoSize3D operator -(int value, const CGeoSize3D& point) { return CGeoSize3D(point.m_x - value, point.m_y - value, point.m_z - value); }
		friend CGeoSize3D operator *(const CGeoSize3D& point, int value){ return CGeoSize3D(point.m_x*value, point.m_y*value, point.m_z*value); }
		friend CGeoSize3D operator /(const CGeoSize3D& point, int value){ return CGeoSize3D(point.m_x / value, point.m_y / value, point.m_z / value); }
		friend CGeoSize3D operator +(const CGeoSize3D& point, int value){ return CGeoSize3D(point.m_x + value, point.m_y + value, point.m_z + value); }
		friend CGeoSize3D operator -(const CGeoSize3D& point, int value){ return CGeoSize3D(point.m_x - value, point.m_y - value, point.m_z - value); }
		friend CGeoSize3D operator /(const CGeoSize3D& point, const CGeoSize3D& size){ return CGeoSize3D(int(point.m_x / size.m_x), int(point.m_y / size.m_y), int(point.m_z / size.m_z)); }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPointIndex>(*this);
			ar & m_z;
		}

		std::ostream& operator>>(std::ostream &s)const{ CGeoSize::operator>>(s) << " " << m_z;	return s; }
		std::istream& operator<<(std::istream &s){ CGeoSize::operator<<(s) >> m_z; return s; }
		friend std::ostream& operator<<(std::ostream &s, const CGeoSize3D& pt){ pt >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CGeoSize3D& pt){ pt << s;	return s; }
	};
	*/
//*****************************************************************************************************
//CGeoPointIndex
	
	class CGeoSize;

	class CGeoPointIndex
	{
	public:

		int m_x;
		int m_y;

		CGeoPointIndex(int x = -1, int y = -1):m_x(x), m_y(y){}

		void clear(){ m_x = -1; m_y = -1; };
		bool operator ==(const CGeoPointIndex& in)const{return m_x==in.m_x&&m_y==in.m_y;}
		bool operator !=(const CGeoPointIndex& in)const{return !operator==(in); }

		CGeoPointIndex operator -( ) const{return CGeoPointIndex(-m_x, -m_y);}
		CGeoPointIndex& operator *=(double in) { m_x = int(m_x*in); m_y = int(m_y *in);	return *this; }
		CGeoPointIndex& operator /=(double in)	{ assert(in != 0);  m_x = int(m_x / in); m_y = int(m_y / in);	return *this; }
		CGeoPointIndex& operator +=(int in) { m_x += in; m_y += in;	return *this; }
		CGeoPointIndex& operator -=(int in)	{ m_x -= in; m_y -= in; return *this; }
		
		inline CGeoPointIndex& operator +=(const CGeoSize& in);
		inline CGeoPointIndex& operator -=(const CGeoSize& in);
		inline CGeoPointIndex& operator *=(const CGeoSize& in);
		inline CGeoPointIndex& operator /=(const CGeoSize& in);

		bool operator >(const CGeoPointIndex& in){ return m_x > in.m_x  && m_y>in.m_y; }
		bool operator >=(const CGeoPointIndex& in){ return m_x >= in.m_x  && m_y>=in.m_y; }
		bool operator <(const CGeoPointIndex& in){ return m_x < in.m_x  && m_y<in.m_y; }
		bool operator <=(const CGeoPointIndex& in){ return m_x <= in.m_x  && m_y <= in.m_y; }
	
		inline CGeoSize GetDistance(const CGeoPointIndex& pt)const;
		inline friend CGeoSize GetDistance(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2);
		//inline friend CGeoSize GetDistance(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2);
		inline friend CGeoSize operator -(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2);
		inline friend CGeoPointIndex operator +(const CGeoPointIndex& pt1, const CGeoSize& pt2);
		inline friend CGeoPointIndex operator /(const CGeoPointIndex& point, const CGeoSize& size);// { return CGeoPointIndex(int(point.m_x / size.m_x), int(point.m_y / size.m_y)); }

		friend CGeoPointIndex operator *(int value, const CGeoPointIndex& point) {return CGeoPointIndex ( point.m_x*value, point.m_y*value );}
		friend CGeoPointIndex operator /(int value , const CGeoPointIndex& point) {return CGeoPointIndex ( point.m_x/value, point.m_y/value );}
		friend CGeoPointIndex operator +(int value, const CGeoPointIndex& point) {return CGeoPointIndex ( point.m_x+value, point.m_y+value );}
		friend CGeoPointIndex operator -(int value , const CGeoPointIndex& point) {return CGeoPointIndex ( point.m_x-value, point.m_y-value );}
		friend CGeoPointIndex operator *(const CGeoPointIndex& point, int value ){return CGeoPointIndex ( point.m_x*value, point.m_y*value );}
		friend CGeoPointIndex operator /(const CGeoPointIndex& point, int value ){return CGeoPointIndex ( point.m_x/value, point.m_y/value );}
		friend CGeoPointIndex operator +(const CGeoPointIndex& point, int value ){return CGeoPointIndex ( point.m_x+value, point.m_y+value );}
		friend CGeoPointIndex operator -(const CGeoPointIndex& point, int value ){return CGeoPointIndex ( point.m_x-value, point.m_y-value );}

		
		
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_x & m_y;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{	s << m_x << " " << m_y ;	return s;	}
		std::istream& operator<<(std::istream &s){s >> m_x >> m_y; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoPointIndex& pt){ pt>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoPointIndex& pt){ pt<<s;	return s; }

	};

	typedef std::vector<CGeoPointIndex> CGeoPointIndexVector;

	

	class CGeoSize : public CGeoPointIndex
	{
	public:

		CGeoSize() :CGeoPointIndex(){}
		CGeoSize(int x, int y) : CGeoPointIndex(x, y){}
		CGeoSize(const CGeoPointIndex& in) : CGeoPointIndex(in){}
	};


	inline CGeoSize CGeoPointIndex::GetDistance(const CGeoPointIndex& pt)const{ return CGeoSize(abs(pt.m_x - m_x), abs(pt.m_y - m_y)); }
	inline CGeoPointIndex& CGeoPointIndex::operator +=(const CGeoSize& in) { m_x += in.m_x; m_y += in.m_y;	return *this; }
	inline CGeoPointIndex& CGeoPointIndex::operator -=(const CGeoSize& in) { m_x -= in.m_x; m_y -= in.m_y;	return *this; }
	inline CGeoPointIndex& CGeoPointIndex::operator *=(const CGeoSize& in) { m_x = int(m_x*in.m_x); m_y = int(m_y*in.m_y);	return *this; }
	inline CGeoPointIndex& CGeoPointIndex::operator /=(const CGeoSize& in) { m_x = int(m_x*in.m_x); m_y = int(m_y*in.m_y);	return *this; }

	inline CGeoSize GetDistance(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2){ return pt1.GetDistance(pt2); }
	inline CGeoSize operator -(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2){ return pt1.GetDistance(pt2); }
	inline CGeoPointIndex operator +(const CGeoPointIndex& pt1, const CGeoSize& pt2){ return CGeoPointIndex(pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y); }
	inline CGeoPointIndex operator /(const CGeoPointIndex& point, const CGeoSize& size){ return CGeoPointIndex(int(point.m_x / size.m_x), int(point.m_y / size.m_y)); }
	//*****************************************************************************************************
	//CGeoPoint3DIndex
	class CGeoSize3D;

	class CGeoPoint3DIndex : public CGeoPointIndex
	{
	public:

		int m_z;


		CGeoPoint3DIndex(int x = -1, int y = -1, int z = -1) : CGeoPointIndex(x, y), m_z(z)	{}

		void clear(){ CGeoPointIndex::clear(); m_z = -1; }
		bool operator ==(const CGeoPoint3DIndex& in)const{ return CGeoPointIndex::operator==(in) && m_z == in.m_z; }
		bool operator !=(const CGeoPoint3DIndex& in)const{ return !operator==(in); }

		CGeoPoint3DIndex operator -() const{ return CGeoPoint3DIndex(-m_x, -m_y, -m_z); }
		CGeoPoint3DIndex& operator *=(int in) { m_x *= in; m_y *= in;	m_z *= in;	return *this; }
		CGeoPoint3DIndex& operator /=(int in)	{ m_x /= in; m_y /= in; m_z /= in;	return *this; }
		CGeoPoint3DIndex& operator +=(int in) { m_x += in; m_y += in;	m_z += in;	return *this; }
		CGeoPoint3DIndex& operator -=(int in)	{ m_x -= in; m_y -= in; m_z -= in;	return *this; }
		bool operator >(const CGeoPoint3DIndex& in){ return m_x > in.m_x  && m_y>in.m_y && m_z > in.m_z; }
		bool operator >=(const CGeoPoint3DIndex& in){ return m_x >= in.m_x  && m_y >= in.m_y&& m_z >= in.m_z; }
		bool operator <(const CGeoPoint3DIndex& in){ return m_x < in.m_x  && m_y<in.m_y&& m_z < in.m_z; }
		bool operator <=(const CGeoPoint3DIndex& in){ return m_x <= in.m_x  && m_y <= in.m_y&& m_z <= in.m_z; }

		inline CGeoSize3D GetDistance(const CGeoPoint3DIndex& pt)const;
		inline friend CGeoSize3D GetDistance(const CGeoPoint3DIndex& pt1, const CGeoPoint3DIndex& pt2);
		inline friend CGeoSize3D operator -(const CGeoPoint3DIndex& pt1, const CGeoPoint3DIndex& pt2);
		inline friend CGeoPoint3DIndex operator +(const CGeoPoint3DIndex& pt1, const CGeoSize3D& pt2);
		inline friend CGeoPoint3DIndex operator /(const CGeoPoint3DIndex& point, const CGeoSize3D& size);

		friend CGeoPoint3DIndex operator *(int value, const CGeoPoint3DIndex& point) { return CGeoPoint3DIndex(point.m_x*value, point.m_y*value, point.m_z*value); }
		friend CGeoPoint3DIndex operator /(int value, const CGeoPoint3DIndex& point) { return CGeoPoint3DIndex(point.m_x / value, point.m_y / value, point.m_z/value); }
		friend CGeoPoint3DIndex operator +(int value, const CGeoPoint3DIndex& point) { return CGeoPoint3DIndex(point.m_x + value, point.m_y + value, point.m_z+value); }
		friend CGeoPoint3DIndex operator -(int value, const CGeoPoint3DIndex& point) { return CGeoPoint3DIndex(point.m_x - value, point.m_y - value, point.m_z-value); }
		friend CGeoPoint3DIndex operator *(const CGeoPoint3DIndex& point, int value){ return CGeoPoint3DIndex(point.m_x*value, point.m_y*value, point.m_z*value); }
		friend CGeoPoint3DIndex operator /(const CGeoPoint3DIndex& point, int value){ return CGeoPoint3DIndex(point.m_x / value, point.m_y / value, point.m_z/value); }
		friend CGeoPoint3DIndex operator +(const CGeoPoint3DIndex& point, int value){ return CGeoPoint3DIndex(point.m_x + value, point.m_y + value, point.m_z+value); }
		friend CGeoPoint3DIndex operator -(const CGeoPoint3DIndex& point, int value){ return CGeoPoint3DIndex(point.m_x - value, point.m_y - value, point.m_z-value); }


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPointIndex>(*this);
			ar & m_z;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{ CGeoPointIndex::operator>>(s) << " " << m_z;	return s; }
		std::istream& operator<<(std::istream &s){ CGeoPointIndex::operator<<(s) >> m_z; return s; }
		friend std::ostream& operator<<(std::ostream &s, const CGeoPoint3DIndex& pt){ pt >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CGeoPoint3DIndex& pt){ pt << s;	return s; }

	};

	class CGeoSize3D : public CGeoPoint3DIndex
	{
	public:

		CGeoSize3D() :CGeoPoint3DIndex(){}
		CGeoSize3D(int x, int y, int z) : CGeoPoint3DIndex(x, y, z){}
		CGeoSize3D(const CGeoPoint3DIndex& in) :CGeoPoint3DIndex(in){}
	};

	CGeoSize3D CGeoPoint3DIndex::GetDistance(const CGeoPoint3DIndex& pt)const{ return CGeoSize3D(abs(pt.m_x - m_x), abs(pt.m_y - m_y), abs(pt.m_z - m_z)); }
	inline CGeoSize3D GetDistance(const CGeoPoint3DIndex& pt1, const CGeoPoint3DIndex& pt2){ return pt1.GetDistance(pt2); }
	inline CGeoSize3D operator -(const CGeoPoint3DIndex& pt1, const CGeoPoint3DIndex& pt2){ return CGeoSize3D(pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y, pt1.m_z - pt2.m_z); }
	inline CGeoPoint3DIndex operator +(const CGeoPoint3DIndex& pt1, const CGeoSize3D& pt2){ return CGeoPoint3DIndex(pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y, pt1.m_z + pt2.m_z); }
	inline CGeoPoint3DIndex operator /(const CGeoPoint3DIndex& point, const CGeoSize3D& size){ return CGeoPoint3DIndex(int(point.m_x / size.m_x), int(point.m_y / size.m_y), int(point.m_z / size.m_z)); }

	typedef CGeoPoint3DIndex CGeoBlock3DIndex;
//*****************************************************************************************************
//CGeoRectIndex

	class CGeoRectIndex
	{
	public:

		int m_x;
		int m_y;
		int m_xSize;
		int m_ySize;

		CGeoRectIndex(int x=-1, int y=-1, int xSize=0, int ySize=0)
		{
			ASSERT(xSize >= 0 && ySize >= 0);
			m_x = x;
			m_y = y;
			m_xSize = xSize;
			m_ySize = ySize;
			//NormalizeRect();
		}

		CGeoRectIndex(const CGeoPointIndex& pt, const CGeoSize& size)
		{
			m_x = pt.m_x;
			m_y = pt.m_y;
			m_xSize = size.m_x;
			m_ySize = size.m_y;
			//NormalizeRect();
		}

		CGeoRectIndex(const CGeoPointIndex& pt1, const CGeoPointIndex& pt2)
		{
			m_x = (std::min)(pt1.m_x, pt2.m_x);
			m_y = (std::min)(pt1.m_y, pt2.m_y);
			m_xSize = abs(pt2.m_x - pt1.m_x)+1;
			m_ySize = abs(pt2.m_y - pt1.m_y)+1;
			//NormalizeRect();
		}

		

		void clear(){ m_x = 0; m_y = 0; m_xSize = 0; m_ySize = 0; };
		int Width()const{ return m_xSize; }
		int Height()const{ return m_ySize; }
		size_t size()const{ return m_xSize*m_ySize; }
		CGeoSize GetGeoSize()const{ return CGeoSize(m_xSize, m_ySize); }

		CGeoPointIndex UpperRight()const{ return CGeoPointIndex(m_x + std::max(m_xSize - 1, 0), m_y); }
		CGeoPointIndex LowerLeft()const { return CGeoPointIndex(m_x, m_y + std::max(m_ySize - 1, 0)); }
		CGeoPointIndex LowerRight()const{ return CGeoPointIndex(m_x + std::max(m_xSize - 1, 0), m_y + std::max(m_ySize - 1, 0)); }
		CGeoPointIndex UpperLeft()const { return CGeoPointIndex(m_x, m_y); }

		CGeoRectIndex& UnionRect(const CGeoRectIndex& rect);
		CGeoRectIndex& IntersectRect(const CGeoRectIndex& rect);
		static CGeoRectIndex UnionRect(const CGeoRectIndex& rect1, const CGeoRectIndex& rect2){ return CGeoRectIndex(rect1).UnionRect(rect2); }
		static CGeoRectIndex IntersectRect(const CGeoRectIndex& rect1, const CGeoRectIndex& rect2){ return CGeoRectIndex(rect1).IntersectRect(rect2); }
		
		//void Inflate(float percent);
		void InflateRect( int w, int n, int e, int s );
		void DeflateRect(int w, int n, int e, int s){ InflateRect(-w,-n,-e,-s); }

		bool IsInside(const CGeoPointIndex& pt){ return PtInRect(pt.m_x, pt.m_y);}
		bool IsInside(int x, int y){ return PtInRect(x, y);}
		bool PtInRect(const CGeoPointIndex& pt)const{ return PtInRect(pt.m_x, pt.m_y);}
		bool PtInRect(int x, int y)const
		{
			//include in left and top and exclude on right and bottom
			return x>=m_x && (x-m_x)<m_xSize && y>=m_y && (y-m_y)<m_ySize;
		}


		char GetTestPosition(int x, double y)const
		{
			char tmp = 0;
			if(x < m_x) tmp|=0x01; //0001;
			if (x >= m_x+m_xSize) tmp |= 0x02; //0010;
			if(y < m_y) tmp|=0x04; //0100;
			if (y >= m_y+m_ySize) tmp |= 0x08; //1000;
	
			return tmp;
		}

		char GetTestPosition(const CGeoPointIndex& pt)const
		{
			return GetTestPosition(pt.m_x, pt.m_y);
		}
	
		/*void NormalizeRect()
		{
			int nTemp=0;
			if (m_xMin > m_xSize)
			{
				nTemp = m_xMin;
				m_xMin = m_xSize;
				m_xSize = m_xMin;
			}
			if (m_yMin > m_ySize)
			{
				nTemp = m_yMin;
				m_yMin = m_ySize;
				m_ySize = m_yMin;
			}
		}*/

		bool IsRectNull( ) const
		{
			return m_xSize == 0 && m_ySize == 0;
		}

		bool IsRectIntersect( const CGeoRectIndex& rect)const
		{
			if( IsRectEmpty() || rect.IsRectEmpty() )
				return false;

			char pos1 = GetTestPosition(rect.LowerLeft());
			char pos2 = GetTestPosition(rect.UpperRight());

			return !(pos1 & pos2);
		}

		//bool IsRectNormal()const;
		bool IsRectEmpty()const{ return m_xSize == 0 || m_ySize==0; }
		void OffsetRect(const CGeoPointIndex& pt)
		{
			m_x += pt.m_x;
			m_y += pt.m_y;
		}

		void SetRectEmpty()	{	clear();	}

		bool IsInside(const CGeoRectIndex& rect)const
		{
			return rect.UpperLeft() >= UpperLeft() && rect.LowerRight() <= LowerRight();
				//rect.m_x >= m_x && (rect.m_x - m_x )<= m_xSize && 
				//rect.m_xSize >= m_xMin && rect.m_xSize <= m_xSize && 
				//rect.m_yMin >= m_yMin && rect.m_yMin <= m_ySize && rect.m_ySize >= m_yMin && rect.m_ySize <= m_ySize;
		}

		void operator -=(const CGeoPointIndex& point)
		{
			OffsetRect( -point );
		}

		void operator +=(const CGeoPointIndex& point)
		{
			OffsetRect( point );
		}

		void operator +=(int in)
		{
			m_x += in;
			m_y += in;
		}
	
		void operator -=(int in)
		{
			m_x -= in;
			m_y -= in;
		}

		CGeoRectIndex operator -()const
		{
			return CGeoRectIndex(-(m_x + m_xSize), -(m_y + m_ySize), m_xSize, m_ySize);
		}

		CGeoRectIndex operator -(const CGeoPointIndex& pt)const
		{
			return CGeoRectIndex( m_x - pt.m_x, m_y - pt.m_y, m_xSize, m_ySize);
		}

		CGeoRectIndex operator +(const CGeoPointIndex& pt)const
		{
			return CGeoRectIndex( m_x + pt.m_x, m_y + pt.m_y, m_xSize, m_ySize);
		}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_x & m_y & m_xSize & m_ySize;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{s << m_x << " " << m_y << " " << m_xSize  << " " << m_ySize; return s;	}
		std::istream& operator<<(std::istream &s){s >> m_x >> m_y >> m_xSize >> m_ySize; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoRectIndex& rect){ rect>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoRectIndex& rect){ rect<<s;	return s; }
	};

	typedef std::vector<CGeoRectIndex> CGeoRectIndexVector;

//*****************************************************************************************************
//CGeoTransform

	enum TGeoTransform{GT_X_LEFT,GT_CELLSIZE_X,GT_X_ROTATION,GT_Y_TOP,GT_Y_ROTATION,GT_CELLSIZE_Y};
	class CGeoTransform
	{
	public:

		
		CGeoTransform(double left=0,double top=0,double cellSizeX=0,double cellSizeY=0)
		{
			m_GT[GT_X_LEFT] = left;
			m_GT[GT_CELLSIZE_X] = cellSizeX;
			m_GT[GT_X_ROTATION] = 0;
			m_GT[GT_Y_TOP] = top;
			m_GT[GT_Y_ROTATION] = 0;
			m_GT[GT_CELLSIZE_Y] = cellSizeY;
		}

		operator double*(){ return &(m_GT[0]);}
		double operator [](int i)const{ ASSERT(i>=0&&i<6); return m_GT[i];}
		double& operator [](int i){ ASSERT(i>=0&&i<6); return m_GT[i];}

		double m_GT[6];
	};

	//*****************************************************************************************************
	//CGeoBlockIndex
	typedef CGeoPointIndex CGeoBlockIndex;

//*****************************************************************************************************
//CGeoExtents

	class CGeoExtents: public CGeoRect
	{
	public:

		enum TRes { RES_MIN, RES_MEAN, RES_MAX, NB_TYPE_RES};

		int m_xSize;
		int m_ySize;
		int m_xBlockSize;
		int m_yBlockSize;


		CGeoExtents(double xMin = -DBL_MAX, double yMin = -DBL_MAX, double xMax = -DBL_MAX, double yMax = -DBL_MAX, int xSize = 0, int ySize = 0, int xBlockSize = 0, int yBlockSize = 0, size_t prjID = PRJ_NOT_INIT) :
		CGeoRect(xMin, yMin, xMax, yMax, prjID)
		{
			m_xSize=xSize;
			m_ySize=ySize;
			m_xBlockSize = xBlockSize;
			m_yBlockSize = yBlockSize;
		}

	
		CGeoExtents(const CGeoRect& geoRect, CGeoSize size, CGeoSize blockSize=CGeoSize(0,0)):CGeoRect(geoRect)
		{
			m_xSize=size.m_x;
			m_ySize=size.m_y;
			m_xBlockSize = blockSize.m_x;
			m_yBlockSize = blockSize.m_y;
		}

		
		void clear(){ CGeoRect::clear(); m_xSize = 0; m_ySize = 0; m_xBlockSize = 0; m_yBlockSize = 0; }
	
		CGeoExtents& operator =(const CGeoExtents& in)
		{
			if( &in != this)
			{
				CGeoRect::operator =(in);
				m_xSize=in.m_xSize;
				m_ySize=in.m_ySize;
				m_xBlockSize=in.m_xBlockSize;
				m_yBlockSize=in.m_yBlockSize;
			}

			return *this;
		}

		bool operator ==(const CGeoExtents& in)const
		{
			bool bEqual=true;
		
			if( CGeoRect::operator !=(in) )bEqual=false;
			if( m_xSize!=in.m_xSize)bEqual=false;
			if( m_ySize!=in.m_ySize)bEqual=false;
			if( m_xBlockSize!=in.m_xBlockSize)bEqual=false;
			if( m_yBlockSize!=in.m_yBlockSize)bEqual=false;

			return bEqual;
		}

		bool operator !=(const CGeoExtents& in)const{ return !operator==(in);}

		bool IsInit() const{ return CGeoRect::IsInit() && m_xSize>0 && m_ySize>0; }
		double XRes()const{ return m_xSize>0?(m_xMax - m_xMin)/m_xSize:0; }
		void SetXRes(double in){ m_xSize = std::max(1, (int)Round(fabs((m_xMax - m_xMin)/in))); }
		double YRes()const{ return m_ySize>0?(m_yMin - m_yMax)/m_ySize:0; }
		void SetYRes(double in){ m_ySize = std::max(1, (int)Round(fabs((m_yMin-m_yMax)/in))); }

		bool IsEmpty()const{ return m_xSize==0 || m_ySize==0 || CGeoRect::IsEmpty(); }
		
		CGeoPoint XYPosToCoord(const CGeoPointIndex& xy)const;//, bool bLimitToBound=false
		CGeoRect XYPosToCoord(const CGeoRectIndex& rect)const;//, bool bLimitToBound=false
		CGeoPointIndex CoordToXYPos(const CGeoPoint& pt)const;//, bool bLimitToBound=false
		CGeoRectIndex CoordToXYPos(const CGeoRect& pt)const;//, bool bLimitToBound=false
		CGeoExtents GetPixelExtents(CGeoPointIndex xy)const;

		CGeoRectIndex GetPosRect()const{ return CGeoRectIndex(0,0,m_xSize, m_ySize);}
		void AlignTo(const CGeoExtents& rect);

		using CGeoRect::IsInside;
		bool IsInside(CGeoPointIndex pt)const{ return IsInside( pt.m_x, pt.m_y); }
		bool IsInside(int x, int y)const{return (x>=0 && x<m_xSize && y>=0 && y<m_ySize);}
	
		CGeoExtents& IntersectExtents(const CGeoExtents& in, int typeRes=RES_MIN);
		CGeoExtents& UnionExtents(const CGeoExtents& in, int typeRes=RES_MIN);

		void GetGeoTransform(CGeoTransform& GT)const;
		void SetGeoTransform(const CGeoTransform& GT, int XSize, int YSize);
		void GetNearestCellPosition(const CGeoPoint& pt, int nbPoint, CGeoPointVector& ptArray);
		void GetNearestCellPosition(const CGeoPoint& pt, int nbPoint, CGeoPointIndexVector& ptArray);
		void GetNearestCellPosition(const CGeoPointIndex& pt, int nbPoint, CGeoPointIndexVector& ptArray);

		void NormalizeRect();
	
		int XNbBlocks()const{ return (int)ceil( (double)m_xSize/m_xBlockSize );}
		int YNbBlocks()const{ return (int)ceil( (double)m_ySize/m_yBlockSize );}
		int GetNbBlocks()const{ return XNbBlocks()*YNbBlocks(); }
		void SetNbBlocks(int xNbBlocks, int yNbBlocks){ m_xBlockSize=(int)ceil( (double)m_xSize/xNbBlocks ); m_yBlockSize=(int)ceil( (double)m_ySize/yNbBlocks );}
		int GetNbPixels()const{ return m_xSize*m_ySize; }
		
		CGeoBlockIndex GetBlockIndex(int x, int y)const{ assert(IsInside(x,y)); return CGeoPointIndex(int(x / m_xBlockSize), int(y / m_yBlockSize)); }
		CGeoBlockIndex GetBlockIndex(CGeoPointIndex pt)const{ assert(IsInside(pt)); return pt / CGeoSize(m_xBlockSize, m_yBlockSize); }
		CGeoBlock3DIndex GetBlockIndex(CGeoPoint3DIndex pt)const{ assert(IsInside(pt)); return pt / CGeoSize3D(m_xBlockSize, m_yBlockSize, 1); }
		 

		CGeoExtents GetBlockExtents(int i, int j)const
		{
			ASSERT( i>=0 && i<XNbBlocks() );
			ASSERT( j>=0 && j<YNbBlocks() );
		
			double blockWidth = fabs(m_xBlockSize*XRes());
			double blockHeight = fabs(m_yBlockSize*YRes());
			CGeoRectIndex rect(m_xBlockSize*i, j*m_yBlockSize, m_xBlockSize, m_yBlockSize);
			rect.IntersectRect(GetPosRect());
			CGeoExtents extents(XYPosToCoord(rect), rect.GetGeoSize(), GetBlockSize() );
			extents.SetPrjID(m_prjID);
			//CGeoExtents extents(m_xMin+(i)*blockWidth, m_yMax-(j+1)*blockHeight,m_xMin+(i+1)*blockWidth, m_yMax-j*blockHeight,m_xBlockSize,m_yBlockSize,m_xBlockSize,m_yBlockSize,m_prjID);
			//extents.IntersectExtents(*this);
			//ASSERT(extents==extents2);

			return extents;
		}

		CGeoRectIndex GetBlockRect(int i, int j)const
		{
			CGeoRectIndex rect(i*m_xBlockSize, j*m_yBlockSize, m_xBlockSize, m_yBlockSize);
			rect.IntersectRect(GetPosRect());
			return rect;
		}

		CGeoSize GetSize()const{ return CGeoSize(m_xSize, m_ySize); }
		CGeoSize GetBlockSize(int i=-1, int j=-1)const
		{
			ASSERT( i>=-1 && i<XNbBlocks() );
			ASSERT( j>=-1 && j<YNbBlocks() );

			if(i==-1||j==-1)
				return CGeoSize(m_xBlockSize, m_yBlockSize);//theory block size

			CGeoRectIndex rect = GetBlockRect(i, j);
			return rect.GetGeoSize();//real block size

		}
		void SetBlockSize( CGeoSize size ){ m_xBlockSize=size.m_x; m_yBlockSize=size.m_y;}
	
	
		std::vector<std::pair<int, int>> GetBlockList(size_t max_cons_row = NOT_INIT, size_t max_cons_col = NOT_INIT);
		

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoRect>(*this);
			ar & m_xSize & m_ySize & m_xBlockSize & m_yBlockSize;
		}
		friend boost::serialization::access;
		
		std::ostream& operator>>(std::ostream &s)const{ s << ((CGeoRect&)*this) << " " << m_xSize << " " << m_ySize << " " << m_xBlockSize  << " " << m_yBlockSize;	return s;	}
		std::istream& operator<<(std::istream &s){ s >> ((CGeoRect&)*this) >> m_xSize >> m_ySize >> m_xBlockSize >> m_yBlockSize; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGeoExtents& extents){ extents>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGeoExtents& extents){ extents<<s;	return s; }


		template <class T>
		ERMsg Reproject( const T& PT)
		{
			return CGeoRect::Reproject(PT);
		}

	};
	

	
//*****************************************************************************************************
//CGridPoint

	class CGridPoint : public CGeoPoint3D
	{
	public:

		//union { double m_z; double m_elevation;};
		double m_latitudeDeg;//latitude in degre for exposition
		double m_slope;//slope in %
		double m_aspect;//aspect in ° from north
		double m_event;
		

		
		CGridPoint(double x = -DBL_MAX, double y = -DBL_MAX, double z = -DBL_MAX, double slope = 0, double aspect = 0, double f = 0, double latitudeDeg = -999, size_t prjID = PRJ_NOT_INIT) :
		  CGeoPoint3D(x,y,z, prjID)
		{
			m_slope		= slope;//slope in %
			m_aspect	= aspect;//aspect in ° from north
			m_event		= f;//event
			m_latitudeDeg= latitudeDeg;
		}
		 
		void Reset(){clear();}
		void clear()
		{
			CGeoPoint3D::clear();
			m_slope		= -999;
			m_aspect    = -999;
			m_event		= 0;
			m_latitudeDeg = -999;
		}

		double operator[](int i)const
		{
			_ASSERTE(i>=0&&i<4); 
			double v=0;
			switch(i)
			{
			case 0:v=m_x; break;
			case 1:v=m_y; break;
			case 2:v=m_z; break;
			case 3:v=GetExposition(); break;
			default: _ASSERTE(false);
			}
			return v;
		}

		//return geocentric return x,y,z of geographic coordinate on a sphere
		double operator()(int i)const
		{
			ASSERT(i>=0&&i<5); 
			ASSERT( IsGeographic() );
			ASSERT(m_x>=-180&&m_x<=180); 
			ASSERT(m_y>=-90&&m_y<=90); 

			double xx = Deg2Rad(m_x + 180);
			double yy = Deg2Rad(90 - m_y);
	
			double v = -DBL_MAX;
			switch(i)
			{
			case 0:v=6371*1000*cos(xx)*sin(yy); break;
			case 1:v=6371*1000*sin(xx)*sin(yy); break;
			case 2:v=6371*1000*cos(yy); break;
			case 3:v=m_z;break;
			case 4:v=GetExposition(); break;
			default: _ASSERTE(false);
			}
			return v;
		}
		

		double GetExposition()const;
		void SetExposition(double latDeg, double expo);
		

		
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint3D>(*this);
			ar & m_latitudeDeg & m_slope & m_aspect &  m_event;
		}
		friend boost::serialization::access;

		std::ostream& operator>>(std::ostream &s)const{ s << ((CGeoPoint3D&)*this) << " " << m_latitudeDeg << " " << m_slope << " " << m_aspect << " " << m_event;	return s;	}
		std::istream& operator<<(std::istream &s){ s >> ((CGeoPoint3D&)*this) >> m_latitudeDeg >> m_slope >> m_aspect >> m_event; return s;}
		friend std::ostream& operator<<(std::ostream &s, const CGridPoint& in){ in>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CGridPoint& out){ out<<s;	return s; }
	};

//*****************************************************************************************************
//CGridPointVector

	
	class CGridPointVector : public CGeoVector<CGridPoint>
	{
	public:

		CGridPointVector()
		{
			Reset();
		}

		void Reset()
		{
			//m_bGeographic=false;
			clear();
		}

		//int size()const{ return (int)std::vector<CGridPoint>::size(); }

		//get distance of 2 points
		double CGridPointVector::GetDistance(int i, int j)const
		{
			return at(i).GetDistance(at(j));
		}

		

		static size_t GetHashForGridPoint(const CGridPoint & v)
		{
			size_t f = 0;
			
			f += 1 * std::hash<double>()(v.m_x);
			f += 2 * std::hash<double>()(v.m_y);
			f += 3 * std::hash<double>()(v.m_z);
			f += 4 * std::hash<double>()(v.GetExposition());
			f += 5 * std::hash<double>()(v.m_event);

			return f;
		}

		size_t GetCheckSum()const
		{
			size_t f = 0;
			for(size_t i=0; i<size(); i++)
				f += ((i + 1)*(GetHashForGridPoint(at(i))));

			return f;
		}

		bool HaveExposition()const
		{
			bool bExposition=false;
			for(const_iterator p=begin(); p<end()&&!bExposition; p++)
			{
				if( p->m_slope > 0 || p->m_aspect > 0)
					bExposition=true;
			}

			return bExposition;
		}
	
		//bool m_bGeographic;


		std::ostream& operator>>(std::ostream &s)const;
		std::istream& operator<<(std::istream &s);
		friend std::ostream& operator<<(std::ostream &s, const CGridPointVector& in){ in >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CGridPointVector& out){ out << s;	return s; }
	};

	typedef std::shared_ptr<CGridPointVector> CGridPointVectorPtr;
	typedef std::vector<float> CGridLine;


	inline void CGeoPoint::SetXY(const CGeoPoint3D& pt){ SetXY(pt.m_x, pt.m_y, pt.m_z); }
	inline CGeoPoint3D CGeoPoint::GetGeocentric()const
	{
		ASSERT(m_lat >= -90 && m_lat <= 90);

		double y = 360.0;
		double lon = m_lon;
		if (lon>180)
			lon -= 360;
		if (lon<-180)
			lon += 360;

		ASSERT(lon >= -180 && lon <= 180);

		double xx = Deg2Rad(lon + 180);
		double yy = Deg2Rad(90 - m_lat);

		return CGeoPoint3D(6371 * 1000 * cos(xx)*sin(yy), 6371 * 1000 * sin(xx)*sin(yy), 6371 * 1000 * cos(yy), PRJ_GEOCENTRIC_BASE);
	}



	//*****************************************************************************************************
	//CNewGeoFileInfo
	class CNewGeoFileInfo
	{
	public:

		std::string m_filePath;
		std::string m_description;
		std::string m_fileType;
		std::string m_projection;
		std::string m_info;
		std::string m_zUnits;
		std::string m_noData;
	};


}//namespace WBSF







namespace zen
{
//*****************************************************************************************************
//
	template <> inline
		void writeStruc(const WBSF::CGeoPoint& pt, XmlElement& output)
	{
		std::string str = ToString(pt);
		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoPoint& pt)
	{
		std::string str;
		input.getValue(str);
		pt = WBSF::ToObject<WBSF::CGeoPoint>(str);

		return true;
	}
//*****************************************************************************************************
//	
	template <> inline
		void writeStruc(const WBSF::CGeoSize& size, XmlElement& output)
	{
		std::string str = ToString(size);
		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoSize& size)
	{
		std::string str;
		input.getValue(str);
		size = WBSF::ToObject<WBSF::CGeoSize>(str);

		return true;
	}
//*****************************************************************************************************
//
	template <> inline
		void writeStruc(const WBSF::CGeoPointIndex& pt, XmlElement& output)
	{
		std::string str = ToString(pt);
		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoPointIndex& pt)
	{
		std::string str;
		input.getValue(str);
		pt = WBSF::ToObject<WBSF::CGeoPointIndex>(str);

		return true;
	}

	//*****************************************************************************************************
//

	template <> inline
		void writeStruc(const WBSF::CGeoRect& rect, XmlElement& output)
	{
		std::string str = ToString(rect);
		output.setValue( str );
	}
	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoRect& rect)
	{
		std::string str;
		input.getValue(str);
		rect = WBSF::ToObject<WBSF::CGeoRect>(str);

		return true;
	}
//*****************************************************************************************************
//

	template <> inline
		void writeStruc(const WBSF::CGeoRectIndex& rect, XmlElement& output)
	{
		std::string str = ToString(rect);
		output.setValue( str );
	}
	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoRectIndex& rect)
	{
		std::string str;
		input.getValue(str);
		rect = WBSF::ToObject<WBSF::CGeoRectIndex>(str);

		return true;
	}
//*****************************************************************************************************
//
	template <> inline
		void writeStruc(const WBSF::CGeoExtents& rect, XmlElement& output)
	{
		std::string str = ToString(rect);
		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGeoExtents& rect)
	{
		std::string str;
		input.getValue(str);
		rect = WBSF::ToObject<WBSF::CGeoExtents>(str);

		return true;
	}

//*****************************************************************************************************
//

	template <> inline
		void writeStruc(const WBSF::CGridPoint& pt, XmlElement& output)
	{
		std::string str = ToString(pt);
		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGridPoint& pt)
	{
		std::string str;
		input.getValue(str);
		pt = WBSF::ToObject<WBSF::CGridPoint>(str);

		return true;
	}

//*****************************************************************************************************
//

}


#endif