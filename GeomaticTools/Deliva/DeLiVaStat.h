#pragma once

#include <bitset>
#include "Basic/Statistic.h"
//#include "Basic/XMLite.h"
#include "Geomatic/UtilGDAL.h"


namespace WBSF
{

	class CCallback;
	class CGeoPoint;
//	class CMFCGDALDataset;

	class CLiDARStat : public CStatisticEx
	{
	public:

		enum { LMEAN, LMIN, LMAX, LRGE, LCAND, LQDMH, LSTDB, LMBSTD, LSKEW, LKURT, LHPR05, LHPR10, LHPR25, LHPR50, LHPR75, LHPR90, LHPR95, LSTRA0, LSTRA1, LSTRA2, LSTRA3, LSTRA4, LSTRA5, LSTRA6, LSTRA7, NB_VARIABLES };//NB_CELLS, 
		static const char* GetID(int i) { ASSERT(i >= 0 && i < NB_VARIABLES); return VARIABLES_NAME[i][0]; }
		static const char* GetName(int i) { ASSERT(i >= 0 && i < NB_VARIABLES); return VARIABLES_NAME[i][1]; }
		static size_t GetStat(const std::string& in);


		CLiDARStat();

		void Reset();

		double operator[](size_t type);
		CLiDARStat& operator+=(double value) { m_bSorted = false; CStatisticEx::operator+=(value); return *this; }
		CLiDARStat& operator+=(const CLiDARStat& in) { m_bSorted = false; CStatisticEx::operator+=(in); return *this; }
		CLiDARStat& operator=(double value) { Reset(); operator+=(value); return *this; }

		
		//void GetXML(//XNode** pRoot)const;


	protected:


		double GetPourcentil(double x)const;
		double GetRatio(double low, double hight)const;

		bool m_bSorted;


		static const std::array<std::array<const char*, 2>, CLiDARStat::NB_VARIABLES> VARIABLES_NAME;
	};

	
	class CLiDARStatSelection: public std::bitset<CLiDARStat::NB_VARIABLES>
	{
	public:
		ERMsg FromString(const std::string& in);
	};




	//class CDeriveLiDARVariable
	//{
	//public:

	//	//enum TMEMBER { INPUT_GRID_FILEPATH, SCALE_FACTOR_IN, RESOLUTION, VARIABLES, SCALE_FACTOR_OUT, CREATE_DATASET, INPUT_DATASET_FILEPATH, OUTPUT_DATASET_FILEPATH, CREATE_GRID, OUTPUT_GRID_FILEPATH, CELL_TYPE, NB_MEMBER };
	//	//static const char* GetMemberName(int i) { ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
	//	//static const char* GetXMLFlag() { return XML_FLAG; }

	//	CDeriveLiDARVariable(void);
	//	~CDeriveLiDARVariable(void);

	//	void Reset();



	//	//ERMsg Load(std::string filePath) { m_projectFilePath = filePath; return XLoad(filePath, *this); }
	//	//ERMsg Save(std::string filePath) { m_projectFilePath = filePath; return XSave(filePath, *this); }
	//	//void GetXML(LPXNode& pRoot)const { XGetXML(*this, pRoot); }
	//	//void SetXML(const LPXNode pRoot) { XSetXML(*this, pRoot); }
	//	//std::string GetMember(int i, LPXNode& pRoot = NULL_ROOT)const;
	//	//void SetMember(int i, const std::string& str, const LPXNode& pRoot = NULL_ROOT);


	//	ERMsg Execute(std::string m_inputImageFilePath, std::string m_inputImageFilePath, CCallback& callback);


	//	std::string m_inputImageFilePath;
	//	std::string m_outputImageFilePath;
	//	std::string m_inputDatasetFilePath;
	//	std::string m_outputDatasetFilePath;
	//	double m_resolution;
	//	CIntArrayEx m_variables;
	//	bool m_bCreateGrid;
	//	bool m_bCreateDataset;
	//	double m_scaleFactorIn;
	//	double m_scaleFactorOut;
	//	int m_cellType;

	//	std::string m_projectFilePath;

	//protected:

	//	static void GetProfileCell(CMFCGDALDataset* poDataset, const CGeoPoint& pt, double resolution, CArray<UtilWin::CFloatArray>& profileArray);
	//	//void FillMapInfo(const CMapBilFile& inputBIL, CMapBilFile& outputBIL, int nbVariables, double resolution);
	//	static GDALDataset* CreateOutputDataset(GDALDataset* poDataset, GDALDriver* poDriverOut, const std::string& outputImageFilePath, int nbVariables, double resolution, GDALDataType cellType);
	//	void ComputeStatistic(const CArray<UtilWin::CFloatArray>& profileArray, int firstCol, int nextCol, CLiDARStat& stat);
	//	ERMsg ExtractGrid(CCallback& callback);
	//	ERMsg ExtractDataset(CCallback& callback);


	//	//static const char* XML_FLAG;
	//	//static const char* MEMBER_NAME[NB_MEMBER];

	//};

}