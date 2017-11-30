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
		enum TLandsatFormat	{ F_UNKNOWN=-1, F_OLD, F_NEW, NB_FORMATS };
		enum TLandsatBands	{ B1, B2, B3, B4, B5, B6, B7, QA, JD, SCENES_SIZE };
		

		enum TIndices{ I_INVALID = -1, I_B1, I_B2, I_B3, I_B4, I_B5, I_B6, I_B7, I_QA, I_JD, I_NBR, I_NDVI, I_NDMI, I_TCB, I_TCG, I_TCW, NB_INDICES };

		enum TDomain{ D_INVALID = -1, D_PRE_ONLY, D_POS_ONLY, D_AND, D_OR, NB_DOMAINS };
		enum TOperator{ O_INVALID = -1, O_LOWER, O_GRATER, NB_OPERATORS };
		enum TCorr8 { NO_CORR8 = -1, C_CANADA, C_AUSTRALIA, C_USA, NB_CORR8_TYPE };


		const char* GetSceneName(size_t s);
		TDomain GetIndiceDomain(const std::string& str);
		TIndices GetIndiceType(const std::string& str);
		TOperator GetIndiceOperator(const std::string& str);
		
		TLandsatFormat GetFormatFromName(const std::string& title);
		__int16 GetCaptorFromName(const std::string& title);
		__int16 GetPathFromName(const std::string& title);
		__int16 GetRowFromName(const std::string& title);
		CTRef GetTRefFromName(const std::string& title);

		TCorr8 GetCorr8(const std::string& str);
	}

	typedef __int16 LandsatDataType;
	typedef std::array<LandsatDataType, Landsat::SCENES_SIZE> LandsatPixel;
	class CLandsatPixel : public LandsatPixel
	{
	public:

		
	
		CLandsatPixel();
		void Reset();

		using LandsatPixel::operator[];
		LandsatDataType operator[](const Landsat::TIndices& i)const;
		LandsatDataType operator[](const Landsat::TIndices& i){ return ((const CLandsatPixel*)(this))->operator[](i); }
		
		bool IsInit()const;
		bool IsValid()const;
		bool IsBlack()const{ return (at(Landsat::B4) == 0 && at(Landsat::B5) == 0 && at(Landsat::B3) == 0); }

		double GetCloudRatio()const;
		double GetEuclideanDistance(const CLandsatPixel& pixel, bool normalized = false)const;
		double NBR()const;
		double NDVI()const;
		double NDMI()const;
		double TCB()const;
		double TCG()const;
		double TCW()const;
		Color8 R()const;
		Color8 G()const;
		Color8 B()const;

		void correction8to7(Landsat::TCorr8 type);

		static double GetDespike(double pre, double spike, double post);

		CTRef GetTRef()const;
	};

	typedef std::vector<CLandsatPixel>CLandsatPixelVector;

	class CIndices
	{
	public:

		
		Landsat::TIndices	m_type;
		std::string			m_op;
		double				m_threshold;

		CIndices(Landsat::TIndices	type, std::string op, double threshold)
		{
			ASSERT(op == ">" || op == "<");

			m_type = type;
			m_op = op;
			m_threshold = threshold;
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
			//case Landsat::I_EUCLIDEAN:
			//{
			//	pre = Tm1.GetEuclideanDistance(T);
			//	spike = 0;
			//	post = Tp1.GetEuclideanDistance(Tp1);
			//	break;
			//}
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

			bool bRep = false;
			if (m_op == "<")
				bRep = CLandsatPixel::GetDespike(pre, spike, post) < (1 - m_threshold);
			else if (m_op == ">")
				bRep = CLandsatPixel::GetDespike(pre, spike, post) > (1 - m_threshold);


			return bRep;
			//bRep = CLandsatPixel::GetDespike(pre, spike, post) < (1 - m_threshold);
		}

		bool IsTrigged(const CLandsatPixel& Tm1, const CLandsatPixel& Tp1)const
		{
			//bool bPass = true;
			double pre = 0;
			double pos = 0;
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
			case Landsat::JD:			pre = Tm1[m_type]; pos = Tp1[m_type] ; break;
			case Landsat::I_NBR:		pre = Tm1.NBR(); pos = Tp1.NBR(); break;
			case Landsat::I_NDVI:		pre = Tm1.NDVI(); pos = Tp1.NDVI();break;
			case Landsat::I_NDMI:		pre = Tm1.NDMI(); pos = Tp1.NDMI();break;
			case Landsat::I_TCB:		pre = Tm1.TCB() ; pos = Tp1.TCB();break;
			case Landsat::I_TCG:		pre = Tm1.TCG() ; pos = Tp1.TCG();break;
			case Landsat::I_TCW:		pre = Tm1.TCW() ; pos = Tp1.TCW();break;
			default: ASSERT(false);
			}
			
			bool bRep = false;
			if (m_op == "<")
				bRep = pre - pos <= m_threshold;
			else if (m_op == ">")
				bRep = pre - pos >= m_threshold;
			return bRep;
		}


		static bool IsValidOp(std::string op)
		{
			return op == "<" || op == ">";
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
			bool bPass = true;
			for (const_iterator it = begin(); it < end(); it++)
			{
				bPass = bPass && it->IsTrigged(Tm1, Tp1);
			}

			return bPass;
		}

	};

	class CLandsatFileInfo
	{
	public:

		CLandsatFileInfo()
		{
			m_format = Landsat::F_UNKNOWN;
			m_captor = -32768;
			m_path = -32768;
			m_row = -32768;
		}

		Landsat::TLandsatFormat m_format;
		__int16 m_captor;
		__int16 m_path;
		__int16 m_row;
		CTRef m_TRef;
	};

	class CLandsatDataset : public CGDALDatasetEx
	{
	public:

		//static const char* SCENE_NAME[Landsat::SCENES_SIZE];

		virtual ERMsg OpenInputImage(const std::string& filePath, const CBaseOptions& options = CBaseOptions());
		virtual ERMsg CreateImage(const std::string& filePath, CBaseOptions options);
		virtual void GetBandsHolder(CBandsHolder& bandsHoler)const;
		virtual void UpdateOption(CBaseOptions& options)const;
		virtual void Close(const CBaseOptions& options = CBaseOptions());

		void InitFileInfo();

		const std::vector<CLandsatFileInfo>& GetFileInfo()const { return m_info; }
	
		ERMsg CreateRGB(size_t i, const std::string filePath, CBaseOptions::TRGBTye type);
		std::string GetCommonBandName(size_t i);

	protected:
		
		std::vector<CLandsatFileInfo> m_info;
	};




	class CLandsatWindow : public CRasterWindow
	{
	public:

		CLandsatWindow();
		CLandsatPixel GetPixel(size_t i, int x, int y)const;
		CLandsatPixel CLandsatWindow::GetPixelMean(size_t i, int x, int y, int buffer)const;
		bool GetPixel(size_t i, int x, int y, CLandsatPixel& pixel)const;

		Landsat::TCorr8 m_corr8;
	};


}