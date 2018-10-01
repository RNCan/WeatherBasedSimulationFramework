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
		enum TIndices{ I_INVALID = -1, I_B1, I_B2, I_B3, I_B4, I_B5, I_B6, I_B7, I_QA, I_JD, I_NBR, I_NDVI, I_NDMI, I_TCB, I_TCG, I_TCW, I_ZSW, I_NBR2, I_EVI, I_SAVI, I_MSAVI, I_SR, I_CL, I_HZ, NB_INDICES};

		enum TDomain{ D_INVALID = -1, D_PRE_ONLY, D_POS_ONLY, D_AND, D_OR, NB_DOMAINS };
		enum TOperator{ O_INVALID = -1, O_LOWER, O_GRATER, NB_OPERATORS };
		enum TCorr8 { NO_CORR8 = -1, C_CANADA, C_AUSTRALIA, C_USA, NB_CORR8_TYPE };


		const char* GetBandName(size_t s);
		const char* GetIndiceName(size_t i);
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
		double GetEuclideanDistance(const CLandsatPixel& pixel, CBaseOptions::TRGBTye type = CBaseOptions::NO_RGB)const;
		double NBR()const;
		double NDVI()const;
		double NDMI()const;
		double TCB()const;
		double TCG()const;
		double TCW()const;
		double ZSW()const;
		double NBR2()const;
		double EVI()const;
		double SAVI()const;
		double MSAVI()const;
		double SR() const;
		double CL()const;
		double HZ()const;
		Color8 R(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER)const;
		Color8 G(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER)const;
		Color8 B(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER)const;

		void correction8to7(Landsat::TCorr8 type);

		static double GetDespike(double pre, double spike, double post, double min_trigger);

		CTRef GetTRef()const;
	};

	typedef std::vector<CLandsatPixel>CLandsatPixelVector;

	class CIndices
	{
	public:

		
		Landsat::TIndices	m_type;
		std::string			m_op;
		double				m_threshold;
		double				m_trigger;

		CIndices(Landsat::TIndices	type, std::string op, double threshold, double trigger)
		{
			ASSERT(op == ">" || op == "<");

			m_type = type;
			m_op = op;
			m_threshold = threshold;
			m_trigger = trigger;
		}

		bool IsSpiking(const CLandsatPixel& Tm1, const CLandsatPixel& T, const CLandsatPixel& Tp1)const
		{

			bool bRemove = true;

			double pre = Tm1[m_type];
			double spike = T[m_type];
			double post = Tp1[m_type];

			bool bRep = false;
			if (m_op == "<")
				bRep = CLandsatPixel::GetDespike(pre, spike, post, m_trigger) < (1 - m_threshold);
			else if (m_op == ">")
				bRep = CLandsatPixel::GetDespike(pre, spike, post, m_trigger) > (1 - m_threshold);


			return bRep;
			//bRep = CLandsatPixel::GetDespike(pre, spike, post) < (1 - m_threshold);
		}

		bool IsTrigged(const CLandsatPixel& Tm1, const CLandsatPixel& Tp1)const
		{
			//bool bPass = true;
			double pre = Tm1[m_type]; 
			double pos = Tp1[m_type];
			
			bool bRep = false;
			if (m_op == "<")
				bRep = pre - pos <= m_threshold;
			else if (m_op == ">")
				bRep = pre - pos >= m_threshold;
			return bRep;
		}
/*
		bool IsTrigged(const CLandsatPixel& Tm1, const CLandsatPixel& T, const CLandsatPixel& Tp1)const
		{

			bool bRemove = true;

			double T1 = Tm1[m_type];
			double T2 = T[m_type];
			double T3 = Tp1[m_type];

			
			bool bRep = false;
			if (m_op == "<")
				bRep = (T1 - T2 <= m_threshold) || (T2 - T3 <= m_threshold);
			else if (m_op == ">")
				bRep = (T1 - T2 >= m_threshold) || (T2 - T3 >= m_threshold);
			return bRep;
		}
*/

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
		ERMsg CreateIndices(size_t i, const std::string filePath, Landsat::TIndices type);

		std::string GetCommonName()const;
		std::string GetCommonImageName(size_t i)const;
		std::string GetCommonBandName(size_t b)const;
		std::string GetSpecificBandName(size_t i)const;
		std::string GetSpecificBandName(size_t i, size_t j)const;
		std::string GetSubname(size_t i, std::string format, size_t b = NOT_INIT)const;

	protected:
		
		std::vector<CLandsatFileInfo> m_info;
	};




	class CLandsatWindow : public CRasterWindow
	{
	public:

		CLandsatWindow();
		CLandsatWindow(const CRasterWindow& in);
		CLandsatWindow(const CLandsatWindow& in);
		CLandsatPixel GetPixel(size_t i, int x, int y)const;
		CLandsatPixel GetPixelMean(size_t i, int x, int y, int buffer)const;
		CLandsatPixel GetPixelMedian(size_t f, size_t l, int x, int y, int buffer=0)const;
		CLandsatPixel GetPixelMedian(int x, int y, int buffer=0)const { return GetPixelMedian(0, GetNbScenes() -1, x, y, buffer); }
		bool GetPixel(size_t i, int x, int y, CLandsatPixel& pixel)const;

		Landsat::TCorr8 m_corr8;
	};


}