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
#include <mutex>

#include "ogr_spatialref.h"
#include "srsinfo.h"
#include "Geomatic/ProjectionTransformation.h"


static std::mutex MUTEX;

using namespace std;

namespace WBSF
{

	//********************************************************************************

	CProjectionTransformation::CProjectionTransformation(size_t src, size_t dst) :
		m_src(CProjectionManager::GetPrj(src)), m_dst(CProjectionManager::GetPrj(dst))
	{
		m_bInit = false;
		m_poCT = NULL;
	}

	void CProjectionTransformation::Set(size_t src, size_t dst){ Set(CProjectionManager::GetPrj(src), CProjectionManager::GetPrj(dst)); }

	CProjectionTransformation::CProjectionTransformation(CProjection const& src, CProjection const& dst) :
		m_src( new CProjection(src)), m_dst(new CProjection(dst))
	{
		m_bInit = false;
		m_poCT = NULL;
	}

	CProjectionTransformation::CProjectionTransformation(CProjectionPtr const& src, CProjectionPtr const& dst) :
		m_src(src), m_dst(dst)
	{
		m_bInit = false;
		m_poCT = NULL;
	}

	CProjectionTransformation::CProjectionTransformation(const CProjectionTransformation& in) :
		m_src(in.m_src), m_dst(in.m_dst)
	{
		m_bInit = false;
		m_poCT = NULL;
	}

	CProjectionTransformation& CProjectionTransformation::operator = (const CProjectionTransformation& in)
	{
		if (&in != this)
		{
			m_bInit = false;
			OGRCoordinateTransformation::DestroyCT(m_poCT);
			m_poCT = NULL;
			m_src = in.m_src;
			m_dst = in.m_dst;
		}
		return *this;
	}

	CProjectionTransformation::~CProjectionTransformation()
	{
		OGRCoordinateTransformation::DestroyCT(m_poCT);
		m_poCT = NULL;
		m_bInit = false;
	}


	ERMsg CProjectionTransformation::Create()
	{
		ERMsg msg;

		MUTEX.lock();

		if (!m_bInit)
		{
			//bool bSame = m_src->IsSame(*m_dst);
			m_poCT = OGRCreateCoordinateTransformation(m_src->GetSpatialReferencePtr(), m_dst->GetSpatialReferencePtr());
			if (m_poCT == NULL)
			{

				msg.ajoute("Failed to create coordinate transformation between the\n"
					"following coordinate systems.  This may be because they\n"
					"are not transformable, or because projection services\n"
					"(PROJ.4 DLL/.so) could not be loaded.\n");

				msg.ajoute("Source:\n" + m_src->GetPrettyWKT(1) + "\nTarget:\n" + m_dst->GetPrettyWKT());
			}

			m_bInit = true;
		}

		MUTEX.unlock();

		return msg;
	}

	ERMsg CProjectionTransformation::Reproject(int nCount, double *x, double *y, double *z, int *pabSuccess)const
	{
		ERMsg msg;
		if (!m_bInit)
		{
			msg = const_cast<CProjectionTransformation*>(this)->Create();
		}

		if (msg && m_poCT)
		{
			if (!m_poCT->TransformEx(nCount, x, y, z, pabSuccess))
			{
				msg.ajoute("Some points are failed in re-projection");
			}
		}

		return msg;
	}



	//*******************************************************************************
	bool CProjectionTransformationManager::Create(size_t src, size_t dst)
	{
		PTMap::const_iterator it = GetInstance().m_links.find(PTKey(src, dst));
		bool bCreate = (it == GetInstance().m_links.end());
		if (bCreate)
			GetInstance().m_links[PTKey(src, dst)].Set(src, dst);

		ASSERT(GetInstance().m_links.find(PTKey(src, dst)) != GetInstance().m_links.end());

		return bCreate;
	}

	bool CProjectionTransformationManager::Create(CProjectionPtr const& src, CProjectionPtr const& dst)
	{
		PTMap::const_iterator it = GetInstance().m_links.find(PTKey(src->GetPrjID(), dst->GetPrjID()));
		bool bCreate = (it == GetInstance().m_links.end());
		if (bCreate)
			GetInstance().m_links[PTKey(src->GetPrjID(), dst->GetPrjID())].Set(src, dst);

		ASSERT(GetInstance().m_links.find(PTKey(src->GetPrjID(), dst->GetPrjID())) != GetInstance().m_links.end());

		return bCreate;
	}


}