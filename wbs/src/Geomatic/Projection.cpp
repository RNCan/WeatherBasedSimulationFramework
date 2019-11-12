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
//#include <mutex>

#pragma warning(disable: 4275 4251)
#include "cpl_vsi.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_minixml.h"
//#include "cpl_port.h"
//#include "GDAL_priv.h"
#include "proj_api.h"
//#include "ogr_spatialref.h"
//#include "srsinfo.h"
#include "ogr_spatialref.h"
#include "Geomatic/Projection.h"


//static std::mutex MUTEX;

using namespace std;

namespace WBSF
{

	extern int FindSRS(const char *pszInput, OGRSpatialReference &oSRS, int bDebug);

	const OGRSpatialReference& CProjection::GetSpatialReference()const { assert(m_pSpatialReference); return *m_pSpatialReference; }
	void CProjection::SetSpatialReference(const OGRSpatialReference& sr)
	{
		if (m_pSpatialReference)
		{
			OSRDestroySpatialReference(m_pSpatialReference);
			m_pSpatialReference = NULL;
		}

		m_pSpatialReference = sr.Clone();

	};

	OGRSpatialReference* CProjection::GetSpatialReferencePtr() { return m_pSpatialReference; }
	const OGRSpatialReference* CProjection::GetSpatialReferencePtr()const { return m_pSpatialReference; }
	bool CProjection::IsSame(const OGRSpatialReference& SR)const
	{
		if (m_pSpatialReference)
			return m_pSpatialReference->IsSame(&SR) == TRUE;

		return true;
	}


	CProjection::CProjection(bool bWGS84)
	{

		m_pSpatialReference = (OGRSpatialReference *)OSRNewSpatialReference(NULL);
		m_pProjPJ = NULL;
		m_prjID = PRJ_NOT_INIT;


		if (bWGS84)
		{
			m_prjID = PRJ_WGS_84;
			m_prjStr = PRJ_WGS_84_WKT;
			m_pSpatialReference->SetWellKnownGeogCS("WGS84");
		}
	}

	CProjection::CProjection(const char* prjStr, size_t prjID, const OGRSpatialReference& SR, projPJ pProjPJ)
	{
		//if (SR)
		//{
		m_pSpatialReference = SR.Clone();
		//}
		//m_pSpatialReference = (OGRSpatialReference *)OSRNewSpatialReference(prjStr);
		if (m_pSpatialReference)
		{
			m_prjID = prjID;
			m_prjStr = prjStr;
			m_pProjPJ = pProjPJ;
		}
		else
		{
			m_prjID = PRJ_UNKNOWN;
		}
	}

	CProjection::CProjection(CProjection const& in)
	{
		m_pProjPJ = NULL;
		m_prjID = PRJ_NOT_INIT;
		m_pSpatialReference = (OGRSpatialReference *)OSRNewSpatialReference(NULL);

		if (in.IsInit())
		{
			operator=(in);
		}
	}

	CProjection::~CProjection()
	{
		clear();
	}

	CProjection& CProjection::operator =(CProjection const& in)
	{
		if (this != &in)
		{
			if (in.m_prjStr != m_prjStr)
			{
				clear();

				if (!in.m_prjStr.empty())
				{
					Create(in.m_prjStr.c_str());
				}
			}
		}

		return *this;
	}

	void CProjection::clear()
	{
		m_prjStr.clear();
		if (m_pSpatialReference)
		{
			OSRDestroySpatialReference(m_pSpatialReference);
		}


		if (m_pProjPJ)
			pj_free(m_pProjPJ);

		m_pSpatialReference = NULL;
		m_pProjPJ = NULL;
		m_prjID = PRJ_NOT_INIT;

	}


	bool CProjection::operator ==(CProjection const& in)const{ return m_prjStr == in.m_prjStr; }


	ERMsg CProjection::Create(const char* prjStr)
	{
		ERMsg msg;

		if (m_prjStr != prjStr)
		{
			clear();

			//try to fin an existing projection

			msg = CProjectionManager::CreateProjection(prjStr);
			if (msg)
			{
				CProjectionPtr pPrj = CProjectionManager::GetPrj(prjStr);
				m_prjID = pPrj->GetPrjID();
				m_prjStr = pPrj->GetPrjStr();
				m_pSpatialReference = pPrj->GetSpatialReferencePtr()->Clone();
			}
		}

		return msg;
	}

	string CProjection::GetWKT()const
	{
		string WKT;

		if (m_pSpatialReference)
		{
			ASSERT(!m_prjStr.empty());
			char* ptr = NULL;
			if (m_pSpatialReference->exportToWkt(&ptr) == OGRERR_NONE)
			{
				WKT = ptr;
				CPLFree(ptr);
			}
		}

		return WKT;
	}


	string CProjection::GetPrettyWKT(int nDepth)const
	{
		string WKT;

		if (m_pSpatialReference)
		{
			ASSERT(!m_prjStr.empty());

			char* ptr = NULL;
			if (m_pSpatialReference->exportToPrettyWkt(&ptr) == OGRERR_NONE)
			{
				WKT = ptr;
				CPLFree(ptr);
			}
		}

		return WKT;
	}
	string CProjection::GetProj4()const
	{
		string WKT;

		if (m_pSpatialReference)
		{
			ASSERT(!m_prjStr.empty());

			char* ptr = NULL;
			if (m_pSpatialReference->exportToProj4(&ptr) == OGRERR_NONE)
			{
				WKT = ptr;
				CPLFree(ptr);
			}
		}

		return WKT;
	}


	ERMsg CProjection::Load(const std::string& name)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(name);
		if (msg)
		{
			//Register projection
			string src = file.GetText();
			msg = Create(src.c_str());

			file.close();
		}

		return msg;
	}

	ERMsg CProjection::Save(const std::string& name)
	{
		ERMsg msg;


		if (m_pSpatialReference)//don't remove unknow .prj file
		{
			char* ptr = NULL;
			m_pSpatialReference->exportToWkt(&ptr);
			if (ptr)
			{

				fStream file;
				msg = file.open(name);
				if (msg)
				{

					/*if (!ptr)
					{
					ptr = CPLStrdup("GEOGCS[\"UNKNOWN\",DATUM[\"UNKNOWN\",SPHEROID[\"UNKNOWN\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.017453292519943295]]");
					}*/

					file.write(ptr, strlen(ptr));
					file.close();
				}

				CPLFree(ptr);
			}
		}


		return msg;
	}

	ERMsg CProjection::Reproject(projPJ src, projPJ dst, long point_count, int point_offset, double *x, double *y, double *z)
	{
		ERMsg msg;

		if (pj_is_latlong(src))
		{
			if (x != NULL)
				*x *= DEG_TO_RAD;
			if (y != NULL)
				*y *= DEG_TO_RAD;
			if (z != NULL)
				*z *= DEG_TO_RAD;
		}

		if (pj_transform(src, dst, point_count, point_offset, x, y, z) == 0)
		{
			msg.ajoute("ERROR: unable to re-projection point (" + ToString(*x) + ", " + ToString(*y) + ((z != NULL) ? ToString(*z) : "") + ")");
		}

		if (pj_is_latlong(dst))
		{
			if (x != NULL)
				*x *= RAD_TO_DEG;
			if (y != NULL)
				*y *= RAD_TO_DEG;
			if (z != NULL)
				*z *= RAD_TO_DEG;
		}

		return msg;
	}

	string CProjection::GetName()const
	{
		string name = "Unknown";
		if (m_pSpatialReference)
		{
			if (m_pSpatialReference->GetAttrValue("PROJCS") && !IsEqualNoCase(m_pSpatialReference->GetAttrValue("PROJCS"), "unnamed"))
				name = m_pSpatialReference->GetAttrValue("PROJCS");
			else if (m_pSpatialReference->GetAttrValue("PROJECTION"))
				name = m_pSpatialReference->GetAttrValue("PROJECTION");
			else if (m_pSpatialReference->GetAttrValue("GEOGCS"))
				name = m_pSpatialReference->GetAttrValue("GEOGCS");
			else if (m_pSpatialReference->IsGeographic())
				name = "Geographic";
		}

		return name;
	}


	//***********************************************************************
	CProjectionManager::CProjectionManager()
	{
		//Add geographic projection
		//CreateProjection(PRJ_WGS_84_WKT);
		assert(CProjectionNameManager::GetPrjID(PRJ_WGS_84_WKT) == PRJ_WGS_84);
		m_links.insert(Link(PRJ_WGS_84, CProjectionPtr(new CProjection(true))));
	};

	void CProjectionManager::clear()
	{
		CProjectionNameManager::clear();
		GetInstance().m_links.clear();
		GetInstance().m_links.insert(Link(PRJ_WGS_84, CProjectionPtr(new CProjection(true))));
	}

	ERMsg CProjectionManager::CreateProjection(const char* prjStr)
	{
		ERMsg msg;

		if (prjStr && strlen(prjStr) > 0)
		{
			size_t prjID = CProjectionNameManager::GetPrjID(prjStr);


			if (prjID == PRJ_NOT_INIT)
			{
				CProjectionPtr pProjection;

				//Register prj name
				OGRSpatialReference spatialReference;

				if (FindSRS(prjStr, spatialReference, false))
				{
					//try to find it by IsSame
					for (LinkMap::const_iterator it2 = GetInstance().m_links.begin(); it2 != GetInstance().m_links.end() && prjID == PRJ_NOT_INIT; it2++)
					{
						if (it2->second->IsSame(spatialReference))
						{
							prjID = it2->first;
						}
					}

					if (prjID == PRJ_NOT_INIT)
					{
						char * pProj4String = NULL;
						if (spatialReference.exportToProj4(&pProj4String) == OGRERR_NONE)
						{
							projPJ pProjPJ = pj_init_plus(pProj4String);
							if (pProjPJ != NULL)
							{
								pProjection.reset(new CProjection(prjStr, CProjectionNameManager::Set(prjStr), spatialReference, pProjPJ));
							}
							else
							{
								msg.ajoute("Unsupported PROJ4 projection: " + string(prjStr));
							}


							CPLFree(pProj4String);
							pProj4String = NULL;
						
						}
						else
						{
							msg.ajoute("Unable to creating projection with: " + string(prjStr));
						}
					}
				}
				else
				{
					msg.ajoute("Unable to creating projection with: " + string(prjStr));
				}

				if (!msg)
				{
					pProjection.reset(new CProjection(prjStr, PRJ_UNKNOWN, OGRSpatialReference(), NULL));
				}


				if (prjID == PRJ_NOT_INIT)
				{
					//Insert event if the create projection fail
					GetInstance().m_links.insert(Link(pProjection->GetPrjID(), pProjection));
				}
				else
				{
					CProjectionNameManager::SetEquivalent(prjStr, CProjectionNameManager::GetPrjStr(prjID));
					ASSERT(CProjectionNameManager::GetInstance().Exists(prjID));
				}

				ASSERT(!msg || CProjectionNameManager::GetInstance().Exists(prjStr));
				ASSERT(!msg || CProjectionNameManager::GetPrjID(prjStr) != PRJ_NOT_INIT);
			}
			else
			{
				string test = GetPrjStr(prjID);
				int i;
				i = 0;
			}
		}
		return msg;
	}

	//********************************************************************************
	CProjectionPtr CProjectionManager::GetUnknownPrj()
	{
		static CProjectionPtr UNKNOWN_PROJECTION;
		
		if (UNKNOWN_PROJECTION.get() == NULL)
			UNKNOWN_PROJECTION = make_shared<CProjection>();
		
		return UNKNOWN_PROJECTION;
	}

	CProjectionPtr CProjectionManager::GetPrj(size_t prjID)
	{
	

		CProjectionPtr pPrj;
		if (prjID != PRJ_NOT_INIT)
		{
			//When we use GetPrj with prjID, the caller must verify that projection exists
			LinkMap::const_iterator it = GetInstance().m_links.find(prjID);
			ASSERT(it != GetInstance().m_links.end());
			if (it != GetInstance().m_links.end())
			{
				ASSERT(it->second);
				pPrj = it->second;
			}
		}
		else
		{
			pPrj = GetUnknownPrj();
		}

		return pPrj;
	}
	//	const CProjection CProjectionManager::UNKNOWN_PROJECTION;


	ERMsg CProjectionManager::Load(const std::string& filePath, CProjectionPtr& pPrj)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filePath);
		if (msg)
		{
			string src = file.GetText();
			msg = CreateProjection(src.c_str());
			if (msg)
				pPrj = GetPrj(src.c_str());

			file.close();
		}

		return msg;
	}




	

}