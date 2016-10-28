#pragma once

//#include "SCCallback.h"
//#include "tmplEx\SortedArray.h"
#include "XMLite.h"
#include "statistic.h"

class CKNearestNeighbor;
class GDALDataset;

class CSpectralInterpol
{
public:

	enum TFrequency{ FREQUENCY=-1};
	enum TMEMBER {INPUT_IMAGE_FILEPATH, /*INPUT_MASK,*/ INPUT_TRANINGSET_FILEPATH, POWER, NB_POINTS, OUTPUT_IMAGE_FILEPATH, CELL_TYPE, NO_DATA_IN, NO_DATA_OUT, STANDARDIZED, STANDARDIZED_FROM_IMAGE, UNIFIED_NO_DATA, CREATE_ERROR, CREATE_NEAREST_NEIGHTBORG, NB_MEMBER};
	
	static const char* GetMemberName(int i){ ASSERT(i>=0&&i<NB_MEMBER); return MEMBER_NAME[i]; }
	static const char* GetXMLFlag(){ return XML_FLAG;}

	CSpectralInterpol(void);
	~CSpectralInterpol(void);

	void Reset();

	ERMsg Load( CString filePath ){m_projectFilePath=filePath; return XLoad( filePath, *this); }
	ERMsg Save( CString filePath  ){m_projectFilePath=filePath; return XSave( filePath, *this ); }
	void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
	void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }
	CString GetMember(int i, LPXNode& pRoot=NULL_ROOT)const;
	void SetMember(int i, const CString& str, const LPXNode& pRoot=NULL_ROOT);

	ERMsg Interpol(CSCCallBack& callback)const;
	ERMsg XValidation(CSCCallBack& callback)const;

	CString m_projectFilePath;
	CString m_inputImageFilePath;
	CString m_trainingSetFilePath;
	CString m_outputImageFilePath;
	int m_nbPoints;
	double m_power;
	float m_noDataIn;
	float m_noDataOut;
	//CString m_mask;
	int m_cellType;
	bool m_bStandardized;
	bool m_bStandardizedFromImage;
	bool m_bAllBandNoData;
	bool m_bCreateError;
	bool m_bCreateNearestNeightBorg;

protected:

	
	ERMsg GetStatistic(CFL::CStatisticVector& stats, CSCCallBack& callback)const;
	ERMsg LoadDataTable(CKNearestNeighbor& dataTable, bool bShowInfo, CSCCallBack& callback)const;
	ERMsg VerifyInput(CKNearestNeighbor& dataTable, GDALDataset* poDataset)const;
	ERMsg OpenInputImage(GDALDataset** poDataset)const;
	ERMsg OpenOutputImage(GDALDataset* poDataset, GDALDataset** poDstDS)const;
	//static CString GetMaskFilePath(const CString& imageFilePath);
	//ERMsg OpenMaskImage(GDALDataset** poDataset)const;


	//bool LoadMask()const;
	//CSortedArray<int, int> m_maskArray;

	static const char* XML_FLAG;
	static const char* MEMBER_NAME[NB_MEMBER];

};
