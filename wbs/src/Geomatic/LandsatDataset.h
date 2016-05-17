//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Geomatic/GDALBasic.h"


namespace WBSF
{

	typedef unsigned __int8 Color8;


	namespace Landsat
	{
		enum TLandsatBands	{ B1, B2, B3, B4, B5, B6, B7, QA, JD, SCENES_SIZE };
		enum TIndices{ I_B1, I_B2, I_B3, I_B4, I_B5, I_B6, I_B7, I_QA, I_JD, I_NBR, I_EUCLIDEAN, I_NDVI, I_NDMI, I_TCB, I_TCG, I_TCW, NB_INDICES };
		enum TMethod{ M_OR, M_AND};
		TIndices GetIndicesType(const std::string& str);
		TMethod GetIndicesMethod(const std::string& str);
	}

	typedef __int16 LandsatDataType;
	typedef std::array<LandsatDataType, Landsat::SCENES_SIZE> LandsatPixel;
	class CLandsatPixel : public LandsatPixel
	{
	public:

		CLandsatPixel();
		bool IsInit()const;
		bool IsValid()const;

		double GetCloudRatio()const;
		double GetEuclideanDistance(const CLandsatPixel& pixel, bool normalized=false)const;
		double NBR()const;
		double NDVI()const;
		double NDMI()const;
		double TCB()const;
		double TCG()const;
		double TCW()const;
		Color8 R()const;
		Color8 G()const;
		Color8 B()const;



		static double GetDespike(double pre, double spike, double post);

		CTRef GetTRef()const;// { return m_TRef; }
		//void SetTRef(CTRef in){ m_TRef = in; }

	protected:

		//CTRef m_TRef;
	};

	class CIndices
	{
	public:

		Landsat::TIndices	m_type;
		double				m_threshold;
		Landsat::TMethod	m_method;


		CIndices(Landsat::TIndices	type, double threshold, Landsat::TMethod m )
		{
			m_type = type;
			m_threshold = threshold;
			m_method = m;
		}

		bool IsSpiking(const CLandsatPixel& Tm1, const CLandsatPixel& T, const CLandsatPixel& Tp1)const
		{
			bool bRemove = true;

			double pre = 0;
			double spike = 0;
			double post = 0;

			switch (m_type)
			{
			case Landsat::B1:
			case Landsat::B2:
			case Landsat::B3:
			case Landsat::B4:
			case Landsat::B5:
			case Landsat::B6:
			case Landsat::B7:
			case Landsat::QA:
			case Landsat::JD:		
			{
				pre = Tm1[m_type];
				spike = T[m_type];
				post = Tp1[m_type];
				break;
			}
			case Landsat::I_NBR:
			{
				pre = Tm1.NBR();
				spike = T.NBR();
				post = Tp1.NBR();
				break;
			}
			case Landsat::I_EUCLIDEAN:
			{
				pre = Tm1.GetEuclideanDistance(T);
				spike = 0;
				post = Tp1.GetEuclideanDistance(Tp1);
				break;
			}
			case Landsat::I_NDVI:
			{
				pre = Tm1.NDVI();
				spike = T.NDVI();
				post = Tp1.NDVI();
				break;
			}
			case Landsat::I_NDMI:
			{
				pre = Tm1.NDMI();
				spike = T.NDMI();
				post = Tp1.NDMI();
				break;
			}
			case Landsat::I_TCB:
			{
				pre = Tm1.TCB();
				spike = T.TCB();
				post = Tp1.TCB();
				break;
			}
			case Landsat::I_TCG:
			{
				pre = Tm1.TCG();
				spike = T.TCG();
				post = Tp1.TCG();
				break;
			}
			case Landsat::I_TCW:
			{
				pre = Tm1.TCW();
				spike = T.TCW();
				post = Tp1.TCW();
				break;
			}
			default: ASSERT(false);
			}

			return CLandsatPixel::GetDespike(pre, spike, post) < (1 - m_threshold);
		}

		bool IsTrigged(const CLandsatPixel& Tm1, const CLandsatPixel& Tp1)const
		{
			bool bPass = true;
			switch (m_type)
			{
			case Landsat::B1:
			case Landsat::B2:
			case Landsat::B3:
			case Landsat::B4:
			case Landsat::B5:
			case Landsat::B6:
			case Landsat::B7:
			case Landsat::QA:
			case Landsat::JD:			bPass = Tm1[m_type] - Tp1[m_type] < m_threshold; break;
			case Landsat::I_NBR:		bPass = Tm1.NBR() - Tp1.NBR() < m_threshold; break;
			case Landsat::I_EUCLIDEAN:	bPass = Tm1.GetEuclideanDistance(Tp1) < m_threshold; break;
			case Landsat::I_NDVI:		bPass = Tm1.NDVI() - Tp1.NDVI() < m_threshold; break;
			case Landsat::I_NDMI:		bPass = Tm1.NDMI() - Tp1.NDMI() < m_threshold; break;
			case Landsat::I_TCB:		bPass = Tm1.TCB() - Tp1.TCB() >= m_threshold; break;
			case Landsat::I_TCG:		bPass = Tm1.TCG() - Tp1.TCG() >= m_threshold; break;
			case Landsat::I_TCW:		bPass = Tm1.TCW() - Tp1.TCW() >= m_threshold; break;
			default: ASSERT(false);
			}

			return bPass;
		}

	};

	class CIndiciesVector : public std::vector < CIndices >
	{
	public:

		bool IsSpiking(const CLandsatPixel& Tm1, const CLandsatPixel& T, const CLandsatPixel& Tp1)const
		{
			bool bRemove = false;
			for (const_iterator it = begin(); it < end() && !bRemove; it++)
			{
				bRemove = it->IsSpiking(Tm1, T, Tp1);
			}

			return bRemove;
		}


		bool IsTrigged(const CLandsatPixel& Tm1, const CLandsatPixel& Tp1)
		{
			bool bPass = !empty() ? front().m_method == Landsat::M_AND : true;
			for (const_iterator it = begin(); it < end(); it++)
			{
				if (it->m_method == Landsat::M_AND)
					bPass = bPass && it->IsTrigged(Tm1, Tp1);
				else
					bPass = bPass || !it->IsTrigged(Tm1, Tp1);
					
			}

			return bPass;
		}

	};

	class CLandsatDataset : public CGDALDatasetEx
	{
	public:

		static const char* SCENE_NAME[Landsat::SCENES_SIZE];

		virtual ERMsg OpenInputImage(const std::string& filePath, const CBaseOptions& option = CBaseOptions());
		virtual void GetBandsHolder(CBandsHolder& bandsHoler)const;
		virtual void UpdateOption(CBaseOptions& option)const;
	};




	class CLandsatWindow : public CRasterWindow
	{
	public:

		CLandsatWindow();
		CLandsatPixel GetPixel(size_t i, int x, int y)const;
	};


}