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
#include "Geomatic/Projection.h"


typedef void *projPJ;
class OGRSpatialReference;
class OGRCoordinateTransformation;
class OGRSpatialReference;


namespace WBSF
{

	//**************************************************************************************************************************
	class CProjectionTransformation
	{
	public:
		
		CProjectionTransformation(size_t src = PRJ_NOT_INIT, size_t dst=PRJ_NOT_INIT);
		CProjectionTransformation(CProjection const& src, CProjection const& dst);
		CProjectionTransformation(CProjectionPtr const& src, CProjectionPtr const& dst);
		CProjectionTransformation(const CProjectionTransformation& in);
		~CProjectionTransformation();

		CProjectionTransformation& operator= (const CProjectionTransformation& in);
		CProjectionPtr const& GetSrc()const{return m_src;}
		CProjectionPtr const& GetDst()const{return m_dst;}
		ERMsg Reproject(int nCount, double *x, double *y, double *z = NULL, int *pabSuccess = NULL)const;

		void Set(size_t src, size_t dst);
		void Set(CProjectionPtr const& src, CProjectionPtr const& dst){ m_src = src; m_dst = dst; }

		ERMsg Create();

		OGRCoordinateTransformation const*  operator ->()const{ASSERT( m_poCT ); return m_poCT;}

		bool IsSame()const{return m_src->IsSame(*m_dst);}

	protected:

		bool m_bInit;
		OGRCoordinateTransformation*  m_poCT;
		CProjectionPtr m_src;
		CProjectionPtr m_dst;
	};

	//**************************************************************************************************************************
	class CProjectionTransformationManager
	{
	public:

		typedef std::pair<size_t, size_t> PTKey;
		typedef std::map<PTKey, CProjectionTransformation> PTMap;
		typedef PTMap::value_type  PTLink;

		static CProjectionTransformationManager& GetInstance()
		{
			static CProjectionTransformationManager INSTANCE; // Guaranteed to be destroyed.
			// Instantiated on first use.
			return INSTANCE;
		}

		
		static bool Create(size_t src, size_t dst);
		static bool Create(CProjectionPtr const& src, CProjectionPtr const& dst);
		//static bool Create(CProjection const& src, CProjection const& dst);

		
		
		static CProjectionTransformation const& Get(CProjectionPtr const& src, CProjectionPtr const& dst)
		{
			Create(src, dst);
			return GetInstance().m_links.at(PTKey(src->GetPrjID(), dst->GetPrjID()));
		}

		static CProjectionTransformation const& Get(size_t src, size_t dst)
		{
			Create(src, dst);
			return GetInstance().m_links.at(PTKey(src, dst));
		}

		

	private:

		CProjectionTransformationManager()
		{}

		// Dont forget to declare these two. You want to make sure they
		// are unaccessable otherwise you may accidently get copies of
		// your singleton appearing.
		CProjectionTransformationManager(CProjectionTransformationManager const&);	// Don't Implement
		void operator=(CProjectionTransformationManager const&);		// Don't implement

		PTMap  m_links;
	};

	inline CProjectionPtr GetProjection(const std::string& src){ return CProjectionManager::GetPrj(src.c_str()); }
	inline CProjectionPtr GetProjection(const char* src){ return CProjectionManager::GetPrj(src); }
	inline CProjectionPtr GetProjection(size_t prjID){ return CProjectionManager::GetPrj(prjID); }

	inline CProjectionTransformation GetReProjection(const std::string& srcPrj, const std::string& dstPrj){ return CProjectionTransformation(CProjectionNameManager::GetPrjID(srcPrj.c_str()), CProjectionNameManager::GetPrjID(dstPrj.c_str())); }
	inline CProjectionTransformation GetReProjection(const std::string& srcPrj, size_t dstPrjId){ return CProjectionTransformation(CProjectionNameManager::GetPrjID(srcPrj.c_str()), dstPrjId); }
	inline CProjectionTransformation GetReProjection(size_t srcPrjId, const std::string& dstPrj){ return CProjectionTransformation(srcPrjId, CProjectionNameManager::GetPrjID(dstPrj.c_str())); }
	inline CProjectionTransformation GetReProjection(size_t srcPrjID, size_t dstPrjID){ return  CProjectionTransformation(srcPrjID, dstPrjID); }
}
