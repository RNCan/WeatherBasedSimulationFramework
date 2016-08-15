//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/GeoBasic.h"


typedef void *projPJ;
class OGRSpatialReference;
class OGRCoordinateTransformation;
class OGRSpatialReference;


namespace WBSF
{

	//**************************************************************************************************************************
	class CProjection
	{
	public:
		
		CProjection(const char* srs, size_t id, const OGRSpatialReference& SR, projPJ pProjPJ);
		CProjection(bool bWGS84=false);
		CProjection(CProjection const& in);
		~CProjection();

		CProjection& operator =(CProjection const& in);
		

		bool operator ==(CProjection const& in)const;
		bool operator !=(CProjection const& in)const{ return !operator==(in);}

		void Reset(){clear();}
		void clear();

		ERMsg Load(const std::string& name);
		ERMsg Save(const std::string& name);
		
		const char* GetPrjStr() {return m_prjStr.c_str();}
		ERMsg SetPrjStr(const char* in){return Create(in);}
		
		ERMsg Create(const char* prjStr);
		
		const OGRSpatialReference& GetSpatialReference()const;// { return *m_spatialReference; }
		void SetSpatialReference(const OGRSpatialReference& sr);// { m_spatialReference = sr; };
		OGRSpatialReference* GetSpatialReferencePtr();// { return &m_spatialReference; }
		const OGRSpatialReference* GetSpatialReferencePtr()const;// { return &m_spatialReference; }
		bool IsSame(const OGRSpatialReference& SR)const;// {return m_spatialReference.IsSame(&SR) == TRUE; }
		bool IsSame(CProjection const& prj)const { return IsSame(*prj.m_pSpatialReference); }


		projPJ GetProjPJ()const	{ ASSERT( m_pProjPJ!=NULL ); return m_pProjPJ;	}
		
		static ERMsg Reproject(projPJ src, projPJ dst, long point_count, int point_offset,double *x, double *y, double *z );
		
		std::ostream& operator>>(std::ostream &s)const{	s << m_prjStr;	return s;	}
		std::istream& operator<<(std::istream &s){s >> m_prjStr; return s;}
		friend std::ostream& operator<<(std::ostream &s, CProjection const & pt){ pt>>s; return s;}
		friend std::istream& operator>>(std::istream &s, CProjection& pt){ pt<<s;	return s; }



		std::string GetName()const;
	
		bool IsInit()const{return WBSF::IsInit(m_prjID);}
		bool IsUnknown()const{ return WBSF::IsUnknown(m_prjID); }
		bool IsGeographic()const{ return WBSF::IsGeographic(m_prjID); }
		bool IsGeocentric()const{ return WBSF::IsGeocentric(m_prjID); }
		bool IsProjected()const{ return WBSF::IsProjected(m_prjID); }

		std::string GetWKT()const;
		std::string GetPrettyWKT(int nDepth=0)const;
		std::string GetProj4()const;

		size_t GetPrjID()const{ return m_prjID;}

	protected:

		std::string m_prjStr;
		OGRSpatialReference* m_pSpatialReference;
		projPJ m_pProjPJ;
		size_t m_prjID;

	};

	typedef std::shared_ptr<CProjection> CProjectionPtr;

	//**************************************************************************************************************************
	class CProjectionManager
	{
	public:

		typedef std::map<size_t, CProjectionPtr> LinkMap;
		typedef LinkMap::value_type  Link;

		static CProjectionManager& GetInstance()
		{
			static CProjectionManager INSTANCE; // Guaranteed to be destroyed.
			// Instantiated on first use.
			return INSTANCE;
		}

		static size_t GetPrjID(const char* prjStr){ return CProjectionNameManager::GetPrjID(prjStr); }
		static const char* GetPrjStr(size_t prjID){ return CProjectionNameManager::GetPrjStr(prjID); }

		static ERMsg CreateProjection(const char* prjStr);

		static const CProjectionPtr GetPrj(const char* prjStr)
		{
			CreateProjection(prjStr);
			return GetPrj(GetPrjID(prjStr));
		}

		static CProjectionPtr GetUnknownPrj();
		static CProjectionPtr GetPrj(size_t prjID);
		static ERMsg Load(const std::string& filePath, CProjectionPtr& pPrj);

	private:

		CProjectionManager();
		
		// Dont forget to declare these two. You want to make sure they
		// are unaccessable otherwise you may accidently get copies of
		// your singleton appearing.
		CProjectionManager(CProjectionManager const&);	// Don't Implement
		void operator=(CProjectionManager const&);		// Don't implement

		LinkMap  m_links;
	};


}
