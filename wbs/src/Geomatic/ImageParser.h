//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <vector>

#include "Geomatic/GDALBasic.h"
#include "MTParser/MTParser.h"


namespace WBSF
{

	typedef std::pair<size_t, size_t>  CImageBandPos;
	typedef std::vector<CImageBandPos> CImageBandPosVector;
	CImageBandPos GetImageBand(const CMTParserVariable& var);

	typedef std::vector< MTParser > MTParserVector;
	typedef std::vector<CMTParserVariable> CMTParserVariableVector;


	class CImageParser
	{
	public:

		bool m_useDefaultNoData;
		bool m_bManageSrcNoData;
		std::string m_formula;

		CImageParser();
		CImageParser(const CImageParser& in);
		CImageParser& operator = (const CImageParser& in);

		ERMsg Compile(const CGDALDatasetExVector& inputDSVector, int nbThread = 1);
		double Evaluate(const std::vector<float>& variable);
		CMTParserVariableVector& GetVars(){ return m_vars[0]; }

	protected:

		std::string m_optimizedFormula;
		std::vector<CMTParserVariableVector> m_vars;
		MTParserVector m_parser;

		static std::string ReplaceOldStyle(std::string formula);
	};

	typedef std::vector<CImageParser> CImageParserVector;
	typedef std::vector< std::vector<MTDOUBLE> > NoDataVector;

	class GetNoDataFct : public MTFunctionI
	{

	public:



		GetNoDataFct(){}

		void SetNoData(NoDataVector& noData)
		{
			m_noData = noData;
		}

	protected:

		GetNoDataFct(const GetNoDataFct& obj)
		{
			m_noData = obj.m_noData;
		}

		virtual const MTCHAR* getSymbol(){ return _T("GetNoData"); }

		virtual const MTCHAR* getHelpString(){ return _T("GetNoData(imageNo, bandNo)"); }
		virtual const MTCHAR* getDescription()
		{
			return _T("Return the no data of the image/band");
		}
		virtual int getNbArgs(){ return 2; }
		virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
		{
			ASSERT(nbArgs == 2);
			size_t imageNo = size_t(pArg[0]) - 1;
			size_t bandNo = size_t(pArg[1]) - 1;

			MTDOUBLE val = 0;

			if (imageNo >= 0 && imageNo < m_noData.size() &&
				bandNo >= 0 && bandNo < m_noData[imageNo].size())
			{
				val = m_noData[imageNo][bandNo];
			}
			else
			{
				MTExcepData data(MTPARSINGEXCEP_InternalError);
				MTParserException exceps;
				std::string desc = FormatA("Invalid argument in function GetNoData(%d,%d)", imageNo + 1, bandNo + 1);

				exceps.add(__LINE__, _T(__FILE__), 0, data, convert(desc).c_str());
				throw(exceps);
			}

			return val;
		}

		virtual MTFunctionI* spawn(){ return new GetNoDataFct(); }

		NoDataVector m_noData;
	};
	//
	//class IsValidFct : public MTFunctionI
	//{
	//
	//public:
	//
	//
	//
	//	IsValidFct(){}
	//
	//	void SetNoData(NoDataVector& noData)
	//	{
	//		m_noData = noData;
	//	}
	//
	//protected:
	//
	//	IsValidFct(const IsValidFct& obj)
	//	{
	//		m_noData = obj.m_noData;
	//	}
	//
	//	virtual const MTCHAR* getSymbol(){ return _T("IsValid"); }
	//
	//	virtual const MTCHAR* getHelpString(){ return _T("IsValid(imageNo, bandNo)"); }
	//	virtual const MTCHAR* getDescription()
	//	{
	//		return _T("Return 1 if the image/band is a valid pixel. 0 otherwhise.");
	//	}
	//	virtual int getNbArgs(){ return 2; }
	//	virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
	//	{
	//		ASSERT(nbArgs == 2);
	//		size_t imageNo = size_t(pArg[0]) - 1;
	//		size_t bandNo = size_t(pArg[1]) - 1;
	//
	//		MTDOUBLE val = 0;
	//
	//		if (imageNo >= 0 && imageNo<m_noData.size() &&
	//			bandNo >= 0 && bandNo<m_noData[imageNo].size())
	//		{
	//			val = m_noData[imageNo][bandNo];
	//		}
	//		else
	//		{
	//			MTExcepData data(MTPARSINGEXCEP_InternalError);
	//			MTParserException exceps;
	//			std::string desc = FormatA("Invalid argument in function IsValid(%d,%d)", imageNo + 1, bandNo + 1);
	//
	//			exceps.add(__LINE__, _T(__FILE__), 0, data, convert(desc).c_str());
	//			throw(exceps);
	//		}
	//
	//		return val;
	//	}
	//
	//	virtual MTFunctionI* spawn(){ return new GetNoDataFct(); }
	//
	//	NoDataVector m_noData;
	//};

	class GetPixelFct : public MTFunctionI
	{

	public:

		GetPixelFct()
		{
			//m_x=-1;
		}

		//void SetMultibandHolder(CMultiBandHolder* pBandHolder)
		//{
		//	m_pBandHolder=pBandHolder;
		//}

		//void SetVariablePos(const CVariablePos& variablePos)
		//{
		//	m_variablePos = variablePos;
		//}

		//void SetCurX(int x)
		//{
		//	m_x=x;
		//}

		//impossible de lire des valeur disposer de facon aléatoire avec avec un band holder
		//dinbc ouvrir les fihcier et aller lire la pixel directement
		void SetImagesFilePath(const StringVector& filePaths)
		{
			m_filePaths = filePaths;
			m_DS.resize(m_filePaths.size());
		}

	protected:

		GetPixelFct(const GetPixelFct& in)
		{
			m_filePaths = in.m_filePaths;
			//m_DS=obj.m_DS;
		}

		virtual const MTCHAR* getSymbol(){ return _T("GetPixel"); }

		virtual const MTCHAR* getHelpString(){ return _T("GetPixel(imageNo, bandNo)"); }
		virtual const MTCHAR* getDescription()		{ return _T("Return the current pixel value of the image/band. Image and band begin at 1."); }
		virtual int getNbArgs(){ return 2; }
		virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);

		virtual MTFunctionI* spawn(){ return new GetPixelFct(); }

		//CMultiBandHolderPtr m_pBandHolder;
		//CVariablePos m_variablePos;
		//int m_x;

		StringVector m_filePaths;
		std::vector<CGDALDatasetEx> m_DS;
	};


	//**********************************************************************************************
	typedef std::vector<MTDOUBLE> MTDOUBLEVector;
	class GetNoDataFct2 : public MTFunctionI
	{

	public:

		GetNoDataFct2(){}

		void SetNoData(const MTDOUBLEVector& noData)
		{
			m_noData = noData;
		}

	protected:

		GetNoDataFct2(const GetNoDataFct2& obj)
		{
			m_noData = obj.m_noData;
		}

		virtual const MTCHAR* getSymbol(){ return _T("GetNoData"); }

		virtual const MTCHAR* getHelpString(){ return _T("GetNoData(bandNo)"); }
		virtual const MTCHAR* getDescription()
		{
			return _T("Return the no data of the band");
		}
		virtual int getNbArgs(){ return 1; }
		virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
		{
			ASSERT(nbArgs == 1);
			size_t bandNo = size_t(pArg[0]) - 1;


			MTDOUBLE val = 0;

			if (bandNo >= 0 && bandNo < m_noData.size())
			{
				val = m_noData[bandNo];
			}
			else
			{
				MTExcepData data(MTPARSINGEXCEP_InternalError);
				MTParserException exceps;
				std::string desc = FormatA("Invalid argument in function GetNoData(%d)", bandNo + 1);

				exceps.add(__LINE__, _T(__FILE__), 0, data, convert(desc).c_str());
				throw(exceps);
			}

			return val;
		}

		virtual MTFunctionI* spawn(){ return new GetNoDataFct2(); }

		MTDOUBLEVector m_noData;
	};



}