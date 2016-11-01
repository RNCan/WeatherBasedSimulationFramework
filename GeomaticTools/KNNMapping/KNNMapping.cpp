// program to analyzed bands and report some information on changing
//***********************************************************************
//									 
//***********************************************************************
// version
// 1.6.1    01/11/2016  Rémi Saint-Amant	Bug correction 
// 2.6.0	11/02/2016	Rémi Saint-Amant	New project structuration. Read data with "".
// 2.5.2	30/01/2015	Rémi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 2.5.1    27/01/2015	Rémi Saint-Amant	Compile with GDAL 1.11.1
// 2.5.0	27/10/2014	Rémi Saint-Amant	Add geographic weight. Add Canocical Correlation Annalysis (CCA).
// 2.4.0	28/06/2014	Rémi Saint-Amant	GDAL 1.11, UNICODE, VC 2013
// 2.3.5	25/07/2013	Rémi Saint-Amant	Add X and Y header option. Remove infoOnly, replace -noImage by -NoResult.
// 2.3.4	22/07/2013	Rémi Saint-Amant	Add output of extra column
//											Bug correction in geo distance in CSV file and images
//											add -stats option
// 2.3.3	02/07/2013	Rémi Saint-Amant	Correction of a bug in CSV mode
// 2.3.2    20/06/2013	Rémi Saint-Amant	NB_THREAD_PROCESS = 2
// 2.3.1	20/06/2013	Rémi Saint-Amant	Problem with multi-threaded mask!
// 2.3.0	06/06/2013	Rémi Saint-Amant	Problem with block and thread!
// 2.2.11	28/05/2013	Rémi Saint-Amant	Problem with block and thread!
// 2.2.10   24/05/2013  Rémi Saint-Amant	Add stats automatically
// 2.2.9    24/05/2013  Rémi Saint-Amant	Add Overview and compute standard deviation over population
// 2.2.8    20/05/2013  Rémi Saint-Amant	Add file information
// 2.2.7    14/05/2013  Rémi Saint-Amant	correction of bug in geoStats
// 2.2.6    01/03/2013	Rémi Saint-Amant	correction of error 
// 2.2.5    29/04/2013  Rémi Saint-Amant	correction of noData double bugs and correction of error (use N-1 instead of N)
// 2.2.4    18/04/2013  Rémi Saint-Amant	new version of with parallel process/IO
// 2.2.3    18/04/2013  Rémi Saint-Amant	mix of version 2.2.1 and 2.2.2
// 2.2.2    17/04/2013  Rémi Saint-Amant	Optimization problems
// 2.2.1    16/04/2013	Rémi Saint-Amant	Add NN distance image
// 2.2.0	14/10/2012	Rémi Saint-Amant	bug correction
// 2.1.0	26/10/2012	Rémi Saint-Amant	Some improvement
// 2.0.0	11/10/2012	Rémi Saint-Amant	Creation from windows version

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>



#pragma warning(disable: 4275 4251)
#include "gdal/Include/gdal_priv.h"
#include "gdal/Include/ogr_spatialref.h"


#include "Basic/OpenMP.h"
#include "Basic/UtilMath.h"
#include "Basic/CSV.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"

#include "KNearestNeighbor.h" 


//TEST Image
//-K 6 -T 0 -Standardized -Error -NearestNeighbor -GeoDistanceStat -ot float32 -dstnodata "-2147483648" -blockSize 64 64 -multi -co "compress=LZW" -overwrite "D:\Travail\LucGuindon\KNNMapping\subset4test\Input\Training.csv" "D:\Travail\LucGuindon\KNNMapping\subset4test\Input\Input.tif" "D:\Travail\LucGuindon\KNNMapping\subset4test\Output\output.tif"
//-te -2073500 7137250 -2073250 7137500 -ot FLOAT32 -dstnodata "-2147483648" -k 6 -t 0 -standardized -NearestNeighbor -Error -GeoDistanceStat -multi -overwrite D:\Travail\LucGuindon\KNNMapping\TestCSV\Training.csv D:\Travail\LucGuindon\KNNMapping\TestCSV\Test.tif D:\Travail\LucGuindon\KNNMapping\TestCSV\To.tif//-dstNoData -9999 -InfoOnly -co "COMPRESS=LZW" -wm 800 -multi -overwrite "D:\Travail\NicolasMansuy\KNNMapping\C_0_15na.csv" "D:\Travail\NicolasMansuy\KNNMapping\topo_clim_MOD.VRT" "D:\Travail\NicolasMansuy\KNNMapping\Test.tif"
//-BlockSize 2048 2048 -dstNoData -9999 --config GDAL_CACHEMAX 4096 -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -K 1 -co "COMPRESS=LZW" -multi -overwrite -mask "U:\GIS\#documents\TestCodes\KNNMapping\Test3\Input\AGE_40_60_AT_MAR_mask.tif" -maskValue 1  "U:\GIS\#documents\TestCodes\KNNMapping\Test3\Input\ALL.SPECIES_biomass_AT_MAR_std_40_60.csv" "U:\GIS\#documents\TestCodes\KNNMapping\Test3\Input\AT_MAR_no_stand.VRT" "U:\GIS\#documents\TestCodes\KNNMapping\Test3\output\Test_40_60.tif"
//-te 1950000 6900000 2050000 7000000  
//-te 2025000 6952000 2226000 7154000 -ot FLOAT32 -k 6 -dstnodata "-9999" -t 0 --config GDAL_CACHEMAX 512 -standardized -NearestNeighbor -Error -GeoDistanceStat -multi -blocksize 128 128 -co compress=LZW -co bigtiff=YES -co tiled=YES -co BLOCKXSIZE=1024 -co BLOCKYSIZE=1024 -overview {4,8,16} -stats -overwrite U:\GIS\#projets\LAB\ANALYSE\20121025_allCanada\Knn\kNNmapping\lasso26x_kNNmapping_v233\test_subset\_ynewList_newlasso26xDec2012_csv.csv U:\GIS\#projets\LAB\ANALYSE\20121025_allCanada\Knn\kNNmapping\lasso26x_kNNmapping_v233\test_subset\_ynewList_newlasso26xDec2012_vrt.vrt U:\GIS\#projets\LAB\ANALYSE\20121025_allCanada\Knn\kNNmapping\lasso26x_kNNmapping_v233\test_subset\TestRemi.tif

//Test CSV
//-k 6 -t 0 -standardized -NearestNeighbor -Error -Info -GeoDistanceStat -multi  -overwrite D:\Travail\LucGuindon\KNNMapping\TestCSV\Training.csv D:\Travail\LucGuindon\KNNMapping\TestCSV\Test.csv D:\Travail\LucGuindon\KNNMapping\TestCSV\TestOut.csv
//-k 6 -t 0 -standardized -overwrite "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV1\Input\Meteo100.csv" "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV1\Input\Rimouski50.csv" "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV1\Output\Rimouski50.csv"
//-k 6 -t 0 -standardized -overwrite "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV2\Input\Training.csv" "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV2\Input\TestCSV2.csv" "U:\GIS\#documents\TestCodes\KNNMapping\TestCSV2\Output\TestCSV2.csv"

//-Overview "2 4 8 16" -te -2080000 7000000 -1500000 720000 -ot FLOAT32 -IOCPU 2 -k 6 -dstnodata "-9999" -t 0 --config GDAL_CACHEMAX 512 -standardized -NearestNeighbor -Error -GeoDistanceStat -multi -blocksize 128 128 -co compress=LZW -co bigtiff=YES -co tiled=YES -co BLOCKXSIZE=128 -co BLOCKYSIZE=128 -overwrite "K:\#projets\LAB\ANALYSE\20121025_allCanada\Knn\kNNmapping\lasso26x_kNNmapping_v2210\test_subset\_ynewList_newlasso26xDec2012_csv.csv" "K:/#projets/LAB/ANALYSE/20121025_allCanada/Knn/kNNmapping/lasso26x_kNNmapping_v2210/test_subset/_ynewList_newlasso26xDec2012_vrt.vrt" "D:\Travail\LucGuindon\KNNMapping\Output\output.tif"





using namespace std;
using namespace WBSF;


static const char* version = "2.6.1";
static const int NB_THREAD_PROCESS = 2;


enum TFilePath {TRAINING_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH};
enum TStandardizeType{ FROM_TRAINING, FROM_INPUT, NB_STANDARDIZE_TYPE};
static const char* STANDARDIZE_TYPE_NAME[NB_STANDARDIZE_TYPE] = {"Training", "Input"};
static int GetStandardizeType(const string& typeName)
{
	int type = -1;
	for(int i=0; i<NB_STANDARDIZE_TYPE; i++)
	{
		if (IsEqualNoCase(typeName, STANDARDIZE_TYPE_NAME[i]))
		{
			type=i;
			break;
		}
	}

	return type;
}

enum TCoordinateStat{ THE_LOWEST, THE_MEAN, THE_STD_DEV, THE_HIGHEST, NB_STAT};
static const short COORDINATE_STAT[NB_STAT] = { LOWEST, MEAN, STD_DEV_OVER_POP, HIGHEST};
enum TNN{ NN_INDEX, NN_GD, NN_SD, NN_WEIGHT, NB_NN};

//typedef TNT::Matrix<int> CKappaMatrix;
typedef CMatrix<int> CKappaMatrix;

class CNNInfo
{
public:
	CNNInfo()
	{
		m_index=0;
		m_geoDistance=0;
		m_distance=0;
		m_weight=0;
	}

	int m_index;
	double m_geoDistance;
	double m_distance;
	double m_weight;
	vector<double> m_Y;
};
	
class CPredictorInfo
{
public:
	CPredictorInfo()
	{
		m_observed=0;
		m_simulated=0;
		m_observedError=0;
		m_simulatedError=0;
	}

	double m_observed;
	double m_observedError;
	double m_simulated;
	double m_simulatedError;
};

typedef vector<CPredictorInfo> CPredictorInfoVector;

class CBacthInfo
{
public:
	CBacthInfo()
	{
		m_index=0;
	}

	int m_index;
	CPredictorInfoVector m_predictor;
	vector<CNNInfo> m_NN;
	CStatistic  m_geoDistanceStat;
};

static const size_t BATCH_SIZE = 5000;
typedef vector<CBacthInfo> CCBacthInfoVector;



class CKNNMappingOption : public CBaseOptions
{
public:

	enum TFrequency{ FREQUENCY=-1};

	CKNNMappingOption():
	CBaseOptions() 
	{
		m_K=10;
		m_T=2;
		m_bStandardize=false;
		m_bInfo=false;
		m_bCreateError=false;
		m_bCreateNearestNeighbor=false;
		m_bCreateGeoDistanceStat=false;
		m_bGeoWeight = false;
		m_bCca = false;
		m_Xprefix = "X_";
		m_Yprefix = "Y_";
		m_XHeader = "X";
		m_YHeader = "Y";
		m_bCSVFormat = false;
				

		m_appDescription = "This software do KNN (K nearest neighbor) mapping";

		static const COptionDef OPTIONS[] = 
		{
			{"-K",1,"nbPoints",false,"Number of nearest neighbor to search. 10 by default."},
			{"-T",1,"power",false,"Power of weight for distance. 2 by default."},
			{"-Frequency",0,"",false,"Use frequency instead of distance. Use -T or -Frequency."}, 
			{"-Standardized",0,"",false,"Standardize each X from training set."},
			{ "-GeoWeight", 0, "", false, "Add geographic coordinates (X,Y) to compute KNN weight." },
			{ "-CCA", 0, "", false, "Use Canonical Correlation Analysis to compute KNN weight." },
			{"-Info",0,"",false,"Output Xvalidation, nearest neighbor information, geographic statistics and statistics of the training dataset."},
			{"-Error",0,"",false,"Create error (image or CSV) for each Y values."},
			{"-NearestNeighbor",0,"",false,"Create nearest neighbor (image or CSV) for each K."},
			{"-GeoDistanceStat",0,"",false,"Create geographic (X,Y) distance statistics (min, mean, SD, max) for all values (in km when a projection file is available)."},
			{"-Xprefix",1,"str",false,"Prefix in label to recognize variables (X). \"X_\" by default."},
			{"-Yprefix",1,"str",false,"Prefix in label to recognize predictors (Y). \"Y_\" by default."},
			{"-X",1,"str",false,"File header title for X coordinates. \"X\" by default."},
			{"-Y",1,"str",false,"File header title for Y coordinates. \"Y\" by default."},
			{"trainingFile",0,"",false, "Training dataset file in CSV format. A projection file (.prj) with the same name as the CSV file can be supply."},
			{"srcfile",0,"",false, "Input file path(CSV or image)."},
			{"dstfile",0,"",false, "Output file path(CSV or image)."}
		};
		
		for(int i=0; i<sizeof(OPTIONS)/sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] = 
		{
			{"Input Training","Dataset","","X_ + Y_ + 2(X,Y)","","Training dataset file can contain any number variables and predictors. One variable or predictor by columns. By default variable begin with X_ and predictor by Y_. Optionally, but recommended, X and Y for cartographic coordinates. The projection of cartographic coordinate can be define in a .prj file with the same name as the training file. All other column will be ignored"},
			{"Input Image or CSV", "srcfile","","Number of X_","","The input image (or CSV file) must have the same number of bands (columns) and the bands must be in the same order than the training dataset"},
			{"Output Images or CSV", "dstfile","One image per predictor (if image)","One columns per predictor (if CSV)","The KNN predicted value of the first Y_|... for each Y_",""},
			{"Optional Output", "Information","Up to 7 files","","_Xval: Observed and predicted values and errors|_XvalNN: Nearest neighbor information|_XvalGeoStats: cartographic distance statistics|_X_stats: X_ statistics|_Y_stats: Y_ statistics|_kappa: Kappa values|_kappaStats: Kappa statistics",""},
			{"Optional Output", "_error","One image per predictor (if image)","One columns per predictor (if CSV)","Estimation of error",""},
			{"Optional Output", "_NN","1","K x 4","The nearest neighbor index|Cartographic distance (km)|Spectral distance|Weight|... for each K",""},
			{"Optional Output", "_GeoStats","1","4","Lowest geographic distance (km)|mean geographic distance (km)|Standard deviation of the distance|Highest geographic distance (km)",""}
		};

		for(int i=0; i<sizeof(IO_FILE_INFO)/sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
	}

	
	virtual ERMsg ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);
				
		if( msg && m_filesPath.size() != NB_FILE_PATH  )
		{
			msg.ajoute("Invalid argument line. 3 files are needed: training CSV file, the source (CSV or image) and destination (CSV or image).");
		}
		
		if (msg && IsEqualNoCase(GetFileExtension(m_filesPath[1]), ".CSV") )
			m_bCSVFormat = true;

		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		
		if( IsEqual(argv[i],"-K") )
        {
			m_K = atoi(argv[++i]);
        }
		else if( IsEqual(argv[i],"-T") )
        {
			m_T = atof(argv[++i]);
        }
		else if( IsEqual(argv[i],"-Frequency") )
		{
			m_T = FREQUENCY;
		}
		else if( IsEqual(argv[i],"-Standardized") )
        {
			m_bStandardize = true;
        }
		else if( IsEqual(argv[i],"-Error") )
        {
			m_bCreateError = true;
        }
		else if( IsEqual(argv[i],"-NearestNeighbor")  )
        {
			m_bCreateNearestNeighbor = true;
        }
		else if( IsEqual(argv[i],"-GeoDistanceStat")  )
        {
			m_bCreateGeoDistanceStat = true;
        }
		else if( IsEqual(argv[i],"-Info")  )
        {
			m_bInfo = true;
        }
		else if( IsEqual(argv[i],"-GeoWeight")  )
        {
			m_bGeoWeight = true;
        }
		else if (IsEqual(argv[i], "-CCA"))
		{
			m_bCca = true;
		}
		else if( IsEqual(argv[i],"-Xprefix")  )
        {
			m_Xprefix  = argv[++i];
        }
		else if( IsEqual(argv[i],"-Yprefix") )
        {
			m_Yprefix  = argv[++i];
        }
		else if( IsEqual(argv[i],"-X")  )
        {
			m_XHeader = argv[++i];
        }
		else if( IsEqual(argv[i],"-Y") )
        {
			m_YHeader = argv[++i];
        }
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}
		
		return msg;
	}

	void UpdateOption(const CGDALDatasetEx& inputDS)
	{
		inputDS.UpdateOption(*this);
		//all additional image are in float32
		m_dstNodataEx = ::LimitToBound( m_dstNodataEx, GDT_Float32, 0, false);
	}


	int m_K;
	double m_T;

	bool m_bStandardize;
	bool m_bCreateError;
	bool m_bCreateNearestNeighbor;
	bool m_bCreateGeoDistanceStat;
	bool m_bInfo;
	bool m_bGeoWeight;
	bool m_bCca;
	string m_Xprefix;
	string m_Yprefix;
	string m_XHeader;
	string m_YHeader;
	bool m_bCSVFormat;
};


//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************
class CKNNMapping
{
public:

	ERMsg Execute();
	ERMsg ExecuteCSV(CKNearestNeighbor& KNN);
	ERMsg ExecuteImage(CKNearestNeighbor& KNN);
	ERMsg XValidation(CKNearestNeighbor& KNN);


	ERMsg OpenAll(CKNearestNeighbor& KNN, CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS,	CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS);
	void AllocateMemory(size_t nbPredictor, CGeoSize blockSize, vector< vector< vector<float> > >& output, vector< vector< vector<float> > >& error, vector< vector< vector<float> > >& NN, vector< vector< vector<float> > >& geoDistance);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
	void ProcessBlock(CKNearestNeighbor& KNN, int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& inputDS, vector< vector< vector<float>>>& output, vector< vector< vector<float>>>& error, vector< vector< vector<float>>>& NN, vector< vector< vector<float>>>& geoDistance);
	void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS, vector< vector< vector<float>>>& output, vector< vector< vector<float>>>& error, vector< vector< vector<float>>>& NN, vector< vector< vector<float>>>& geoDistance);
	void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS,	CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS);

	float GetReplacedPixel(CDataWindowPtr& window);
	float GetReplacedPixel(int z, vector<CDataWindowPtr>& windows);

	CKNNMappingOption m_options;
	std::string m_unit;

	

	static string GetDescription() { return  string("KNNMapping version ") + version + " (" + __DATE__ + ")\n" ; }
	static double GetKappa(const CKappaMatrix& kappa);
};




ERMsg CKNNMapping::Execute()
{
	ERMsg msg;

	if( !m_options.m_bQuiet )		
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[TRAINING_FILE_PATH] << endl;

		if( !m_options.m_maskName.empty() )
			cout << "Mask:   " << m_options.m_maskName << endl;
		
		cout << "K= " << m_options.m_K << endl;
		if( m_options.m_T==CKNNMappingOption::FREQUENCY )
			cout << "T= Evaluation by frequency of occurrence" << endl;
		else 
			cout << "T= " << ToString(m_options.m_T,2) << endl;


		cout << endl << "Load training set..." << endl;
	}

	
	CKNearestNeighbor KNN(m_options.m_bStandardize, m_options.m_bCca);
	msg += KNN.LoadCSV(m_options.m_filesPath[TRAINING_FILE_PATH], m_options.m_XHeader, m_options.m_YHeader, m_options.m_Xprefix, m_options.m_Yprefix );

	if (!KNN.HaveGeoCoordinates() && (m_options.m_bCreateGeoDistanceStat || m_options.m_bGeoWeight))
		msg.ajoute("ERROR: No geographic coordinates informations (X,Y) was found in the training dataset. Impossible to compute geographic distances.");

	if(msg)
	{
		//init here to avoid problem in multi-threading
		KNN.Init();
		
		if( !m_options.m_bQuiet )
		{
			CProjectionPtr pPrj = CProjectionManager::GetPrj(KNN.GetPrjID());
			string prjName = pPrj? pPrj->GetName() : "Unknown";

			m_unit = (pPrj && pPrj->IsInit()) ? "km" : "??";//distance of know projection always output in km

			cout << "Number of training samples: " << KNN.size() << endl;
			cout << "Number of dimensions:       " << KNN.GetNbDimension() << endl;
			cout << "Number of predictors:       " << KNN.GetNbPredictor() << endl;
			cout << "Projection of predictor:    " << prjName << endl;
	
			if (!KNN.HaveGeoCoordinates())
				cout << "WARNING: no geographic coordinates informations was found in the training dataset" << endl;
		}


		if( m_options.m_bInfo)
			msg = XValidation(KNN);

		if( m_options.m_bCreateImage )
		{
			if( m_options.m_bCSVFormat )
				msg = ExecuteCSV(KNN);
			else msg = ExecuteImage(KNN);
		}
	}


	return msg;
}



//************************************************************************************************************************************************
//************************************************************************************************************************************************
//************************************************************************************************************************************************
//************************************************************************************************************************************************
//From CSV
ERMsg CKNNMapping::ExecuteCSV(CKNearestNeighbor& KNN)
{
	ERMsg msg;

	SetFileExtension(m_options.m_filesPath[OUTPUT_FILE_PATH], ".csv");

	if( !m_options.m_bOverwrite && FileExists( m_options.m_filesPath[OUTPUT_FILE_PATH] ) )
	{
		msg.ajoute("ERROR: Output file already exist. Delete the file or use option -overwrite.");
		return msg;
	}

	//CRandomGenerator randomNumber;
	
	ofStream outputFile;
	ofStream errorFile;
	ofStream NNFile;
	ofStream geoDistanceStatFile;
	

	if( !m_options.m_bQuiet )		
		cout << endl << "Process input CSV..." << endl;

	//a revoir pour charger au fure et a mesure
	CKNearestNeighbor X(false);
	msg += X.LoadCSV(m_options.m_filesPath[1], m_options.m_XHeader, m_options.m_YHeader, m_options.m_Xprefix, m_options.m_Yprefix);
	
	if( msg && X.GetNbDimension() != KNN.GetNbDimension() )
		msg.ajoute( "ERROR: The number of X variables in the input CSV file ("+ToString(X.GetNbDimension())+") is not equal to the number of X variables in the training dataset ("+ToString(KNN.GetNbDimension())+")\n");

	if (!X.HaveGeoCoordinates() && (m_options.m_bCreateGeoDistanceStat || m_options.m_bGeoWeight))
		msg.ajoute("ERROR: no geographic coordinates (X,Y) information was found in the input CSV file");

	if(msg)
	{
		//update option for writing: result have 11 bands
		msg += outputFile.open( m_options.m_filesPath[OUTPUT_FILE_PATH] );

		if( m_options.m_bCreateError)
		{
			string filePath(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			SetFileTitle(filePath, GetFileTitle(filePath) + "_error");
			msg += errorFile.open( filePath);
		}

		if( m_options.m_bCreateNearestNeighbor)
		{
			string filePath(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			SetFileTitle(filePath, GetFileTitle(filePath) + "_NN");
			msg += NNFile.open( filePath);
		}

		
		if( m_options.m_bCreateGeoDistanceStat)
		{
			string filePath(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			SetFileTitle(filePath, GetFileTitle(filePath) + "_geoStats");
			msg += geoDistanceStatFile.open( filePath);
		}
	}
	

	if(msg)
	{

		//write header
		string tmp="No";
		if (X.HaveGeoCoordinates())
		{
			//for(size_t p=0; p<.GetNb.GetExtraColumns().size(); p++)
			tmp+=",X,Y";
		}

		for(size_t p=0; p<X.GetExtraColumns().size(); p++)
			tmp+=","+X.GetExtraColumns().at(p).m_name;

		for(size_t p=0; p<KNN.GetNbPredictor(); p++)
			tmp+=","+KNN(p).GetName();
		outputFile.write(tmp + "\n");

		if( m_options.m_bCreateError)
		{
			string tmp="No";
			for(size_t p=0; p<KNN.GetNbPredictor(); p++)
				tmp+=","+KNN(p).GetName();

			errorFile.write(tmp + "\n");
		}

		if( m_options.m_bCreateNearestNeighbor)
		{
			string tmp="No,k,index,geoDistance("+m_unit+"),Distance,Weight(%)";
			for(size_t p=0; p<KNN.GetNbPredictor(); p++)
				tmp+=","+KNN(p).GetName();
			NNFile.write(tmp + "\n");
		}

		if( m_options.m_bCreateGeoDistanceStat)
		{
			string tmp="No";
			for(size_t i=0; i<NB_STAT; i++)
				tmp+=string(",")+ CStatistic::GetName( COORDINATE_STAT[i] ) + " ("+m_unit+")";
			geoDistanceStatFile.write(tmp + "\n");
		}


		size_t nbDimension = KNN.GetX().GetNbDimension();
		size_t nbPredictor = KNN.GetNbPredictor();

		//const CGeoPointVector& geoPointsTraning = KNN.GetCoordinates();
		//const CGeoPointVector& geoPointsInput = X.GetCoordinates();
		
		
		CCBacthInfoVector batchInfo((size_t)min( X.size(), BATCH_SIZE));
		
		int nbBatch = (int)ceil((double)X.size()/BATCH_SIZE);
		m_options.ResetBar(nbBatch);

		for(int b=0; b<nbBatch; b++)
		{
			//for all point in the table
			size_t  bacthSize = (size_t ) min(BATCH_SIZE, X.size()-b*BATCH_SIZE);
			if( batchInfo.size() != bacthSize)
				batchInfo.resize(bacthSize);

			//#pragma omp parallel for schedule(static, 100) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
			for(int ii=0; ii<bacthSize; ii++)
			{
				int i = b*BATCH_SIZE+ii;
				batchInfo[ii].m_index=i;

				//spectral coordinate
				CMDPoint pt(X[i]);
				//CGeoPointVector geoPt;
				//if (!X.GetCoordinates().empty())
					//geoPt = X.GetCoordinates()[i];
			
				//look for the nearest point
				CSearchResultVector result;
				KNN.Search(pt, m_options.m_K, result);
				ASSERT(result.size() == m_options.m_K);

				batchInfo[ii].m_predictor.resize(KNN.GetNbPredictor());
				//compute the mean for all Y
				for( int p=0; p<KNN.GetNbPredictor(); p++)
				{
					double predictor = KNN(p).GetValue(result, m_options.m_T, m_options.m_bGeoWeight);
					batchInfo[ii].m_predictor[p].m_simulated = predictor;

					if( m_options.m_bCreateError )
					{
						double sum²=0;
						for(size_t k=0; k<result.size(); k++)
							sum²+=Square(KNN(p).at(result[k].m_index)-predictor);

						if( predictor!= 0)
							batchInfo[ii].m_predictor[p].m_simulatedError = sqrt(sum²/max(1ull,result.size()-1))/predictor*100;
						else batchInfo[ii].m_predictor[p].m_simulatedError = 0;
					}
				}//for all predictor
					
				if( m_options.m_bCreateNearestNeighbor )
				{
					//match file
					batchInfo[ii].m_NN.resize(result.size());

					vector<double> weight = result.GetWeight(m_options.m_T, m_options.m_bGeoWeight);
					ASSERT( weight.size() == result.size() );

					for(int k=0; k<result.size(); k++)
					{
						batchInfo[ii].m_NN[k].m_index = result[k].m_index;
						batchInfo[ii].m_NN[k].m_distance = result[k].m_distance;
						batchInfo[ii].m_NN[k].m_geoDistance = result[k].m_geoDistance;
						batchInfo[ii].m_NN[k].m_weight = weight[k]*100;

						batchInfo[ii].m_NN[k].m_Y.resize(KNN.GetNbPredictor());
						for(int p=0; p<KNN.GetNbPredictor(); p++)
							batchInfo[ii].m_NN[k].m_Y[p] = KNN(p).at(result[k].m_index);

						if (m_options.m_bCreateGeoDistanceStat)
							batchInfo[ii].m_geoDistanceStat += result[k].m_geoDistance;
					}
				}
			
				
				//{
				//	ASSERT( !geoPointsInput.empty() && !geoPointsTraning.empty());
				//	for(int k=0; k<result.size(); k++)
				//	{
				//		double distance = geoPointsInput[i].GetDistance( geoPointsTraning[result[k].m_index] )/1000;
				//		batchInfo[ii].m_geoDistanceStat += distance;
				//	}
				//}
			}//ii for computing
			
			//write data to file
			for(int ii=0; ii<bacthSize; ii++)
			{
				int i = batchInfo[ii].m_index;

				string tmp=ToString(batchInfo[ii].m_index+1);
				if (X.HaveGeoCoordinates())
					tmp+=","+ ToString(X[i].m_x) + "," + ToString(X[i].m_y);

				for(size_t p=0; p<X.GetExtraColumns().size(); p++)
					tmp+=","+X.GetExtraColumns().at(p).at( batchInfo[ii].m_index );
				

				for(size_t p=0; p<batchInfo[ii].m_predictor.size(); p++)
				{
					tmp+="," +ToString(batchInfo[ii].m_predictor[p].m_simulated,10);
				}

				outputFile.write(tmp + "\n");
				if( m_options.m_bCreateError )
				{
					string tmp=ToString(batchInfo[ii].m_index+1);
					for(size_t p=0; p<batchInfo[ii].m_predictor.size(); p++)
					{
						tmp+="," +ToString(batchInfo[ii].m_predictor[p].m_simulatedError,10);
					}
					errorFile.write(tmp+"\n");
				}

				if(m_options.m_bCreateNearestNeighbor)
				{
				
					for(size_t k=0; k<batchInfo[ii].m_NN.size(); k++)
					{
						string tmp=ToString(batchInfo[ii].m_index+1) + "," + ToString(k+1)+ ","+ ToString(int(batchInfo[ii].m_NN[k].m_index+1))+ ","+ ToString(batchInfo[ii].m_NN[k].m_geoDistance, 3) + "," + ToString(batchInfo[ii].m_NN[k].m_distance,10) + "," + ToString(batchInfo[ii].m_NN[k].m_weight,10);
					
						for(size_t p=0; p<batchInfo[ii].m_NN[k].m_Y.size(); p++)
							tmp+="," +ToString(batchInfo[ii].m_NN[k].m_Y[p],10);

						NNFile.write(tmp + "\n");
					}
				}
			

				if(m_options.m_bCreateGeoDistanceStat)
				{
					string tmp=ToString(batchInfo[ii].m_index+1);
					for(size_t i=0; i<NB_STAT; i++)
					{
						tmp += "," + ToString(batchInfo[ii].m_geoDistanceStat[COORDINATE_STAT[i]],10);
					}

					geoDistanceStatFile.write(tmp + "\n");
				}
			}//ii for writing


			#pragma omp atomic 
				m_options.m_xx++;

				
			m_options.UpdateBar();
		}//for batch 
	

		outputFile.close();

		if( m_options.m_bCreateError )
		{
			errorFile.close();
		}
		
		if( m_options.m_bCreateNearestNeighbor )
		{
			NNFile.close();
		}
		
		if(m_options.m_bCreateGeoDistanceStat)
		{
			geoDistanceStatFile.close();
		}
	}//msg

    
    
	return msg;
}

//************************************************************************************************************************************************
//************************************************************************************************************************************************
//************************************************************************************************************************************************
//************************************************************************************************************************************************
//From Image


ERMsg CKNNMapping::ExecuteImage(CKNearestNeighbor& KNN)
{
	ERMsg msg;

	//CRandomGenerator randomNumber;
	GDALAllRegister();
	
	CGDALDatasetEx inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetExVector outputDS;
	CGDALDatasetExVector errorDS;
	CGDALDatasetEx NNDS;
	CGDALDatasetEx geoDistanceStatDS;

	msg = OpenAll(KNN,inputDS,maskDS,outputDS,errorDS,NNDS,geoDistanceStatDS);

	if(msg)
	{
		if( !m_options.m_bQuiet && m_options.m_bCreateImage)		
		{
			cout << "Output type:    " << GDALGetDataTypeName( (GDALDataType) outputDS[0].GetDataType() ) << endl;
			cout << "Output no data: " << ToString(outputDS[0].GetNoData(0), 20) << endl;
		}
		
		if (KNN.GetPrjID() == PRJ_NOT_INIT)
		{
			cout << "WARNING: Unknown projection for predictor, assume to be the same projection as the input image." << endl;
			KNN.SetPrjID(inputDS.GetPrjID());
		}

		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		if( maskDS.IsOpen() )
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents);

		if(!msg)
			return msg;
		
		if(!m_options.m_bQuiet && m_options.m_bCreateImage) 
			cout << "Create output images (" << outputDS[0]->GetRasterXSize() << " C x " << outputDS[0]->GetRasterYSize() << " R x " << outputDS[0].GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

		const vector<double>& noData = inputDS.GetNoData(); 
		CGeoExtents extents = bandHolder[0].GetExtents();
		///extents.SetProjectionRef( inputDS->GetProjectionRef() );
		
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);
		
		vector<pair<int,int>> XYBlock = extents.GetBlockList();
		
		omp_set_nested(1);
		#pragma omp parallel for shared(bandHolder,inputDS) schedule(static, 1) num_threads(NB_THREAD_PROCESS)  if (m_options.m_bMulti)
		for(int xy=0; xy<(int)XYBlock.size(); xy++)
		{
			int blockThreadNo = ::omp_get_thread_num();
			int xBlock=XYBlock[xy].first;
			int yBlock=XYBlock[xy].second;
				
			vector< vector< vector<float> > > output;
			vector< vector< vector<float> > > error;
			vector< vector< vector<float> > > NN;
			vector< vector< vector<float> > > geoDistance;

			ReadBlock(xBlock, yBlock, bandHolder[blockThreadNo]);
			ProcessBlock(KNN,xBlock, yBlock, bandHolder[blockThreadNo], inputDS, output, error, NN, geoDistance);
			WriteBlock(xBlock, yBlock, bandHolder[blockThreadNo], outputDS, errorDS, NNDS, geoDistanceStatDS, output, error, NN, geoDistance);
		}//xyBlock

		CloseAll(inputDS, maskDS, outputDS, errorDS, NNDS, geoDistanceStatDS);
	}

    
    return msg;
}


ERMsg CKNNMapping::OpenAll(CKNearestNeighbor& KNN, CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS,	CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS)
{
	ERMsg msg;
	
	if( !m_options.m_bQuiet )		
		cout << endl << "Open input images..." << endl;
	
	msg = inputDS.OpenInputImage(m_options.m_filesPath[INPUT_FILE_PATH], m_options);

	if( msg && inputDS.GetRasterCount() != KNN.GetX().GetNbDimension() )
		msg.ajoute( "ERROR: The number of bands in input image ("+ToString(inputDS.GetRasterCount())+") is not equal to the number of X variables in the data table ("+ToString(KNN.GetX().GetNbDimension())+")");

	if(msg)
		m_options.UpdateOption( inputDS );

	if(msg && !m_options.m_bQuiet) 
	{
		CGeoExtents extents = inputDS.GetExtents();
		CProjectionPtr pPrj = inputDS.GetPrj();
		string prjName = pPrj ? pPrj->GetName() : "Unknown";

		cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
		cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
		cout << "    Projection     = " << prjName << endl;
	}


	if( msg && !m_options.m_maskName.empty() )
	{
		if( !m_options.m_bQuiet )		
			cout << "Open mask..." << endl;

		msg += maskDS.OpenInputImage( m_options.m_maskName );
	}

	if(msg)
	{
		if( KNN.GetNbPredictor() == 0)
		{
			m_options.m_bCreateImage = false;
			m_options.m_bCreateError = false;
		}
		
		
		if( m_options.m_bCreateImage )
		{
			if( !m_options.m_bQuiet )		
				cout << "Open output images..." << endl;
			
			outputDS.resize(KNN.GetNbPredictor());
			for(int i=0; i<KNN.GetNbPredictor()&&msg; i++)
			{
				if( msg && m_options.m_bCreateImage )
				{
					CKNNMappingOption option(m_options);
					option.m_nbBands = 1;
					string filePath(option.m_filesPath[OUTPUT_FILE_PATH]);
					SetFileTitle(filePath, GetFileTitle(filePath) + "_" + KNN(i).GetName());

					msg += outputDS[i].CreateImage(filePath, option);
				}
			}
		}
		
		if( m_options.m_bCreateError)
		{
			if( !m_options.m_bQuiet )		
				cout << "Open error images..." << endl;

			errorDS.resize(KNN.GetNbPredictor());

			for(int i=0; i<KNN.GetNbPredictor()&&msg; i++)
			
			{
				CKNNMappingOption option(m_options);
				option.m_nbBands = 1;
				option.m_outputType = GDT_Float32;
				option.m_dstNodata = option.m_dstNodataEx;
				string filePath(option.m_filesPath[OUTPUT_FILE_PATH]);
				SetFileTitle(filePath, GetFileTitle(filePath) + "_" + KNN(i).GetName() + "_error");
				msg += errorDS[i].CreateImage(filePath, option);
				
			}
		}
		
		if( msg && m_options.m_bCreateNearestNeighbor)
		{
			if( !m_options.m_bQuiet )		
				cout << "Open nearest neighbor image...";

			CKNNMappingOption option(m_options);
			option.m_nbBands = option.m_K*NB_NN;//index, geoDistance, distance, weight
			option.m_outputType = GDT_Float32;
			option.m_dstNodata = option.m_dstNodataEx;
			string filePath(option.m_filesPath[OUTPUT_FILE_PATH]);
			SetFileTitle(filePath, GetFileTitle(filePath) + "_NN");
			msg += NNDS.CreateImage(filePath, option);
		}

		if( msg && m_options.m_bCreateGeoDistanceStat)
		{
			if( !m_options.m_bQuiet )		
				cout << "Open distance image...";

			CKNNMappingOption option(m_options);
			option.m_nbBands = NB_STAT;
			option.m_outputType = GDT_Float32;
			option.m_dstNodata = option.m_dstNodataEx;
			string filePath(option.m_filesPath[OUTPUT_FILE_PATH]);
			SetFileTitle(filePath, GetFileTitle(filePath) + "_GeoStats");
			msg += geoDistanceStatDS.CreateImage(filePath, option);
		}
	}
	
	if( !m_options.m_bQuiet )		
		cout << endl;

	return msg;
}

void CKNNMapping::AllocateMemory(size_t nbPredictor, CGeoSize blockSize, vector< vector< vector<float> > >& output, vector< vector< vector<float> > >& error, vector< vector< vector<float> > >& NN, vector< vector< vector<float> > >& geoDistance)
{
	if( m_options.m_bCreateImage )
	{
		output.resize(nbPredictor);
		for(size_t p=0; p<output.size(); p++)
		{
			output[p].resize(blockSize.m_y);
			for(size_t j=0; j<output[p].size(); j++)
				output[p][j].resize(blockSize.m_x, (float)m_options.m_dstNodata );
		}
	}

	if( m_options.m_bCreateError )
	{
		error.resize(nbPredictor);
		for(size_t p=0; p<error.size(); p++)
		{
			error[p].resize(blockSize.m_y);
			for(size_t j=0; j<error[p].size(); j++)
				error[p][j].resize(blockSize.m_x, (float)m_options.m_dstNodataEx);
		}
	}
		
	if( m_options.m_bCreateNearestNeighbor )
	{
		NN.resize(m_options.m_K*NB_NN);
		for(size_t k=0; k<NN.size(); k++)
		{
			NN[k].resize(blockSize.m_y);
			for(size_t j=0; j<NN[k].size(); j++)
				NN[k][j].resize(blockSize.m_x, (float)m_options.m_dstNodataEx );
		}
	}
		
	if(m_options.m_bCreateGeoDistanceStat)
	{
		geoDistance.resize(NB_STAT);
		for(size_t s=0; s<geoDistance.size(); s++)
		{
			geoDistance[s].resize(blockSize.m_y);
			for(size_t j=0; j<geoDistance[s].size(); j++)
				geoDistance[s][j].resize(blockSize.m_x, (float)m_options.m_dstNodataEx );
		}
	}
}

void CKNNMapping::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
	#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();
		
		bandHolder.LoadBlock(xBlock,yBlock);
		
		m_options.m_timerRead.Stop();
	}
}

void CKNNMapping::ProcessBlock(CKNearestNeighbor& KNN, int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& inputDS, vector< vector< vector<float>>>& output, vector< vector< vector<float>>>& error, vector< vector< vector<float>>>& NN, vector< vector< vector<float>>>& geoDistance)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock,yBlock);
	int nbCells = extents.m_xSize*extents.m_ySize;

	if( bandHolder.IsEmpty() )
	{
		#pragma omp atomic
			m_options.m_xx+=(min(nbCells,blockSize.m_x*blockSize.m_y));
		
		m_options.UpdateBar();
		return;
	}

	#pragma omp critical(ProcessBlock)
	{
		vector<CDataWindowPtr> input;
		bandHolder.GetWindow(input);

		AllocateMemory(KNN.GetNbPredictor(), blockSize, output, error, NN, geoDistance);

				
		m_options.m_timerProcess.Start();
		
		#pragma omp parallel for schedule(static, 100) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
		for(int y=0; y<blockSize.m_y; y++)
		{
			for(int x=0; x<blockSize.m_x; x++)
			{
				//geographic coordinate
				CGeoExtents blockExtents = extents.GetBlockExtents(xBlock,yBlock);
				CGeoPoint coordinate = blockExtents.XYPosToCoord( CGeoPointIndex(x,y) );
				//const CGeoPointVector& geoPointsTraning = KNN.GetCoordinate();
				//const CGeoPointVector& geoPointsInput = X.GetCoordinate();

				//spectral coordinate
				CMDPoint pt(KNN.GetNbDimension());
				
				bool bValid = true;
				for(int z=0; z<KNN.GetNbDimension()&&bValid; z++)
				{
					pt[z] = input[z]->at(x,y);
					bValid = !_isnan(pt[z]) && _finite(pt[z]) && bandHolder.IsValid(z, (DataType)pt[z]);
				}

				if( bValid )
				{
					//look for the nearest point
					CSearchResultVector result;
					KNN.Search(pt, m_options.m_K, result);

					ASSERT(result.size() == m_options.m_K);

					//compute the mean for all Y
					for( int p=0; p<KNN.GetNbPredictor(); p++)
					{
						//if (m_options.m_bCca)
							//KNN.Search(pt, p, m_options.m_K, result);

						double predictor = KNN(p).GetValue(result, m_options.m_T, m_options.m_bGeoWeight);
						
						output[p][y][x] = (float)predictor;

						if( m_options.m_bCreateError )
						{
							double sum²=0;
							for(size_t k=0; k<result.size(); k++)
							{
								sum²+=Square(KNN(p).at(result[k].m_index)-output[p][y][x]);
							}

							double stdDev = sqrt(sum²/max(1ull,result.size()-1));
							if( output[p][y][x]!= 0)
								error[p][y][x] = (float)(stdDev/output[p][y][x]*100);
							else error[p][y][x] = 0;
						}
					}//for all predictor
					
					if( m_options.m_bCreateNearestNeighbor )
					{
						vector<double> weight = result.GetWeight(m_options.m_T, m_options.m_bGeoWeight);
						ASSERT( weight.size() == result.size() );

						//const CGeoPointVector& geoPoints = KNN.GetCoordinates();
						for(int k=0; k<result.size(); k++)
						{
							NN[k*NB_NN][y][x] = (float)result[k].m_index+1;
											
							if( KNN.HaveGeoCoordinates() )
							{
								//double distance = coordinate.GetDistance( geoPoints[result[k].m_index] )/1000;
								NN[k*NB_NN + 1][y][x] = (float)result[k].m_geoDistance;
							}

							NN[k*NB_NN+2][y][x] = (float)result[k].m_distance;
							NN[k*NB_NN+3][y][x] = (float)weight[k]*100;
						}
					}
			
					if(m_options.m_bCreateGeoDistanceStat)
					{
						CStatistic stat;
										
						//const CGeoPointVector& geoPoints = KNN.GetCoordinates();
						//ASSERT( !geoPoints.empty() );
										
						for(int k=0; k<result.size(); k++)
						{
							//double distance = coordinate.GetDistance( geoPoints[result[k].m_index] )/1000;
							stat += result[k].m_geoDistance;
						}

						for(int i=0; i<NB_STAT; i++)
							geoDistance[i][y][x] = (float)stat[COORDINATE_STAT[i]];
					}

				}//valid

				#pragma omp atomic
					m_options.m_xx++;
			}//for all x

			m_options.UpdateBar();
		}//y

		m_options.m_timerProcess.Stop();
	}//process block
}


void CKNNMapping::WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS, vector< vector< vector<float>>>& output, vector< vector< vector<float>>>& error, vector< vector< vector<float>>>& NN, vector< vector< vector<float>>>& geoDistance)
{
	#pragma omp critical(BlockIO)
	{
		m_options.m_timerWrite.Start();

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoRectIndex outputRect=extents.GetBlockRect(xBlock,yBlock);

		
		for(size_t p=0; p<output.size(); p++)
		{
			if( m_options.m_bCreateImage )
			{
				ASSERT( outputDS[p]->GetRasterCount() == 1);
				GDALRasterBand *pBand = outputDS[p].GetRasterBand(0);
								
				//transfer block into single memory
				vector<float> tmp(outputRect.Width()*outputRect.Height() );
				//tmp.reserve(outputRect.Width()*outputRect.Height() );
				for(int y=0; y<(int)output[p].size(); y++)
					for(int x=0; x<(int)output[p][y].size(); x++)
						tmp[y*output[p][y].size()+x] = (float)outputDS[p].PostTreatment(output[p][y][x]);

				pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0  );
			}

			if( m_options.m_bCreateError )
			{
				GDALRasterBand *pBand = errorDS[p].GetRasterBand(0);
				
				vector<float> tmp;
				tmp.reserve(outputRect.Width()*outputRect.Height() );
				for(int y=0; y<(int)output[p].size(); y++)
					tmp.insert(tmp.end(), error[p][y].begin(), error[p][y].end());

				pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0  );
			}
		}

		if(m_options.m_bCreateNearestNeighbor)
		{
			for(size_t k=0; k<NN.size(); k++)
			{
				GDALRasterBand *pBand = NNDS.GetRasterBand(k);
				
				vector<float> tmp;
				tmp.reserve(outputRect.Width()*outputRect.Height() );
				for(int y=0; y<(int)NN[k].size(); y++)
					tmp.insert(tmp.end(), NN[k][y].begin(), NN[k][y].end());

				pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0  );
			}
		}
			
		if(m_options.m_bCreateGeoDistanceStat)
		{
			for(size_t s=0; s<geoDistance.size(); s++)
			{
				GDALRasterBand *pBand = geoDistanceStatDS.GetRasterBand(s);
				for(int y=0; y<(int)geoDistance[s].size(); y++)
					pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y+y, outputRect.Width(), 1, &(geoDistance[s][y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0  );
			}
		}

		m_options.m_timerWrite.Stop();
	}//write block
}

void CKNNMapping::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS,	CGDALDatasetExVector& outputDS, CGDALDatasetExVector& errorDS, CGDALDatasetEx& NNDS, CGDALDatasetEx& geoDistanceStatDS)
{
	if( !m_options.m_bQuiet )		
		cout << endl << "Close all images..." << endl;

	//  Close the case file and free allocated memory  
	inputDS.Close();
	maskDS.Close();
		
	m_options.m_timerWrite.Start();

	for(size_t p=0; p<outputDS.size(); p++)
	{
		outputDS[p].ComputeStats(p==0?m_options.m_bQuiet:true);
		outputDS[p].BuildOverviews(m_options.m_overviewLevels, p==0?m_options.m_bQuiet:true);
		outputDS[p].Close();
	}

	for(size_t p=0; p<errorDS.size(); p++)
	{
		errorDS[p].ComputeStats(true);
		errorDS[p].BuildOverviews(m_options.m_overviewLevels, true);
		errorDS[p].Close();
	}
		
	NNDS.ComputeStats(true);
	NNDS.BuildOverviews(m_options.m_overviewLevels, true);
	NNDS.Close();
	
	geoDistanceStatDS.ComputeStats(true);
	geoDistanceStatDS.BuildOverviews(m_options.m_overviewLevels, true);
	geoDistanceStatDS.Close(); 
	

	m_options.m_timerWrite.Stop();

	m_options.PrintTime();
	
}


double CKNNMapping::GetKappa(const CKappaMatrix& kappa)
{
	ASSERT( kappa.cols() == kappa.rows());

	double diagonal=0;
	double sum=0;
	std::vector<int> X(kappa.cols());
	std::vector<int> Y(kappa.rows());

	for(size_t y=0; y<kappa.cols(); y++)
	{
		for (size_t x = 0; x<kappa.rows(); x++)
		{
			if( x==y)
				diagonal+=kappa[y][x];

			X[x] += kappa[y][x];
			Y[y] += kappa[y][x];
			sum += kappa[y][x];
		}
	}


	ASSERT( sum > 0);
	double prA = diagonal/sum;
	double prE = 0;
	for(int i=0; i<kappa.cols(); i++)
		prE += X[i]*Y[i]/(sum*sum);


	return (prA-prE)/(1-prE);
}


ERMsg CKNNMapping::XValidation(CKNearestNeighbor& KNN)
{
	ERMsg msg;
	
	if( !m_options.m_bQuiet )		
		cout << "Output KNN informations..." << endl;
	//Randomize(FIXED_SEED);//fixed seed randomization for selecting best index

	ofStream file1;
	ofStream file2;
	ofStream file3;
	ofStream file4;
	ofStream file5;
	ofStream file6;
	ofStream file7;

	string filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
	SetFileName(filePath, GetFileTitle(filePath) + "_Xval.csv");
	msg += file1.open( filePath);

	filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
	SetFileName(filePath, GetFileTitle(filePath) + "_XvalNN.csv");
	msg += file2.open( filePath);

	filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
	SetFileName(filePath, GetFileTitle(filePath) + "_XvalGeoStats.csv");
	msg += file3.open( filePath);

	filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
	SetFileName(filePath, GetFileTitle(filePath) + "_Y_stats.csv");
	msg += file4.open( filePath);

	filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
	SetFileName(filePath, GetFileTitle(filePath) + "_X_stats.csv");
	msg += file5.open( filePath);


	if( m_options.m_T==CKNNMappingOption::FREQUENCY )
	{
		filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
		SetFileName(filePath, GetFileTitle(filePath) + "_kappa.csv");
		msg += file6.open( filePath);

		filePath = m_options.m_filesPath[OUTPUT_FILE_PATH];
		SetFileName(filePath, GetFileTitle(filePath) + "_kappaStats.csv");
		msg += file7.open( filePath);
	}
	

	if(!msg)
		return msg;

	size_t nbDimension = KNN.GetNbDimension();

	
//write header
	file1.write("pt,Y,Observed,ObservedError(%),Predicted,PredictedError(%)\n");
	
	file2.write("pt,k,Index,GeoDistance(" + m_unit + "),Distance,Weight(%)");
	for(int p=0; p<KNN.GetNbPredictor(); p++)
		file2.write("," + KNN(p).GetName());
	file2.write("\n");
	
	string tmp="No";
	for(size_t i=0; i<NB_STAT; i++)
		tmp+=string(",")+ CStatistic::GetName( COORDINATE_STAT[i] )+" ("+m_unit+")";
	file3.write(tmp + "\n");

	file4.write("Y,Bias,MAD,RMSE,RMSE(%),R²(det),R²(corr)\n");
	
	file5.write("X,Mean,StandardDeviation\n");

	if( m_options.m_T==CKNNMappingOption::FREQUENCY)
		file7.write("Y,Kappa\n");

		

	CStatisticXYExVector stat(KNN.GetNbPredictor());
	//const CGeoPointVector& geoPoints = KNN.GetCoordinates();
	CCBacthInfoVector batchInfo((size_t)min( KNN.size(), BATCH_SIZE));

	int nbBatch = (int)ceil((double)KNN.size()/BATCH_SIZE);
	m_options.ResetBar(nbBatch);
	

	for(int b=0; b<nbBatch; b++)
	{
		//for all point in the table
		size_t  bacthSize = (size_t ) min(BATCH_SIZE, KNN.size()-b*BATCH_SIZE);
		if( batchInfo.size() != bacthSize)
			batchInfo.resize(bacthSize);
	
		#pragma omp parallel for schedule(static, 100) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
		for(int ii=0; ii<bacthSize; ii++)
		{
			int i = b*BATCH_SIZE+ii;
			CMDPoint pt(KNN[i]);

			//look for the nearest point
			CSearchResultVector result;
			KNN.Search(pt, m_options.m_K+1, result);

			ASSERT(result[0].m_distance == 0);
			result.pop_front();
		
			batchInfo[ii].m_index = i;
			batchInfo[ii].m_predictor.resize(KNN.GetNbPredictor());
			for(int p=0; p<KNN.GetNbPredictor(); p++)
			{
				double observed = KNN(p).at(i);
				double predictor= KNN(p).GetValue(result, m_options.m_T, m_options.m_bGeoWeight);
				
				
				batchInfo[ii].m_predictor[p].m_observed = observed;
				batchInfo[ii].m_predictor[p].m_simulated = predictor;
				
				double sumObs²=0;
				double sumSim²=0;
				for(size_t k=0; k<result.size(); k++)
				{
					sumObs²+=Square(KNN(p).at(result[k].m_index)-observed);
					sumSim²+=Square(KNN(p).at(result[k].m_index)-predictor);
				}
				
				if( observed!=0)
					batchInfo[ii].m_predictor[p].m_observedError = sqrt(sumObs²/max(1ull,result.size()-1))/observed*100;
				else batchInfo[ii].m_predictor[p].m_observedError = 0;
				
				if( predictor!=0)
					batchInfo[ii].m_predictor[p].m_simulatedError = sqrt(sumSim²/max(1ull,result.size()-1))/predictor*100;
				else batchInfo[ii].m_predictor[p].m_simulatedError = 0;

				//#pragma omp critical(AddStat)
				stat[p].Add(predictor, observed);
			}//for p

			//nearest neighbor file
			batchInfo[ii].m_NN.resize(result.size());
			vector<double> weight = result.GetWeight(m_options.m_T, m_options.m_bGeoWeight);
			ASSERT( weight.size() == result.size() );

			//
			for(size_t k=0; k<result.size(); k++)
			{
				//double distance = -9999;
				
//				if (KNN.HaveGeoCoordinates())
				//	distance = result[k].m_geoDistance;// KNN[i].GetDistance(KNN[result[k].m_index]) / 1000;

				batchInfo[ii].m_NN[k].m_index = result[k].m_index;
				batchInfo[ii].m_NN[k].m_distance = result[k].m_distance;
				batchInfo[ii].m_NN[k].m_geoDistance = result[k].m_geoDistance;
				batchInfo[ii].m_NN[k].m_weight = weight[k]*100;
				
				batchInfo[ii].m_NN[k].m_Y.resize(KNN.GetNbPredictor());
				for(int p=0; p<KNN.GetNbPredictor(); p++)
					batchInfo[ii].m_NN[k].m_Y[p] = KNN(p).at(result[k].m_index);
				
				batchInfo[ii].m_geoDistanceStat += result[k].m_geoDistance;
			}
		}//for ii computing
		

		
		//write files for the batch
		for(CCBacthInfoVector::const_iterator it=batchInfo.begin(); it!=batchInfo.end(); it++)
		{
			int i = it->m_index;
			
			//write Xval files
			for(int p=0; p<KNN.GetNbPredictor(); p++)
			{
				string tmp=ToString(it->m_index+1)+","+KNN(p).GetName();
				tmp+= "," + ToString(it->m_predictor[p].m_observed,10) + "," + ToString(it->m_predictor[p].m_observedError,10);
				tmp+= "," + ToString(it->m_predictor[p].m_simulated,10)+ "," + ToString(it->m_predictor[p].m_simulatedError,10);

				file1.write(tmp + "\n");
			}//for p

			//write match file
			for(size_t k=0; k<it->m_NN.size(); k++)
			{
				string tmp=ToString(it->m_index+1)+","+ToString(k+1)+","+ToString(it->m_NN[k].m_index+1)+","+ToString(it->m_NN[k].m_geoDistance,10)+"," + ToString(it->m_NN[k].m_distance,10)+","+ToString(it->m_NN[k].m_weight,10);
				for(int p=0; p<KNN.GetNbPredictor(); p++)
					tmp += ","+ToString(it->m_NN[k].m_Y[p],10);

				file2.write(tmp + "\n");
			}

			//write geoStats
			string tmp=ToString(it->m_index+1);
			for(size_t s=0; s<NB_STAT; s++)
			{
				tmp += "," + ToString(it->m_geoDistanceStat[COORDINATE_STAT[s]],10);
			}

			file3.write(tmp + "\n");
		}//for ii writing

		#pragma omp atomic 
			m_options.m_xx++; 
				
		m_options.UpdateBar();
		
	}//for all batch

	
	//stats
	for(int p=0; p<KNN.GetNbPredictor(); p++)
	{
		string tmp = FormatA( "%s,%.2lf,%.2lf,%.2lf,%.2lf,%.3lf,%.3lf\n", KNN(p).GetName(), stat[p][BIAS], stat[p][MAE], stat[p][RMSE], stat[p][RMSE]/stat[p].GetY()[MEAN]*100, stat[p][COEF_D], stat[p][STAT_R²]);
		file4.write(tmp);
	}

	for(size_t i=0; i<KNN.GetNbDimension(); i++)
	{
		const CStatistic& stats = KNN.GetX().GetStatistic(i);
		file5.write(KNN.GetX().GetDimensionName(i) + "," + ToString(stats[MEAN], 10) + "," + ToString(stats[STD_DEV], 10) + "\n");
	}
	
	
	//create kappa stats
	if( m_options.m_T==CKNNMappingOption::FREQUENCY)
	{
		if( !m_options.m_bQuiet)
			cout << "Output Kappa..." << endl;

		for(int p=0; p<KNN.GetNbPredictor(); p++)
		{
			int nbClass = (int) KNN(p).GetNbClass();

			CKappaMatrix kappa(nbClass, nbClass);
			
			//for all point in the table
			#pragma omp parallel for schedule(static, 100) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)
			for(int i=0; i<KNN.size(); i++)
			{
				CMDPoint pt(KNN[i]);

				//look for the nearest point
				//TODO: pour le moment, seulment la premier prédicteur
				CSearchResultVector result;
				KNN.Search(pt, m_options.m_K+1, result);

				ASSERT(result[0].m_distance == 0);
				result.pop_front();

				//compute the mean
				double predictor = KNN(p).GetValue(result, m_options.m_T, m_options.m_bGeoWeight);

				size_t iR = KNN(p).GetClassIndex( Round(KNN(p).at(i)) );
				size_t iC = KNN(p).GetClassIndex(Round(predictor));
				int& ref = kappa[iR][iC];

				#pragma omp atomic
					ref++;
				//	kappa[iR][iC]++;
				

				if( !m_options.m_bQuiet && i%int(ceil(KNN.size()*KNN.GetNbPredictor()/80.0)) == 0)
					cout << ".";
			}

			string tmp = "Class (Y="+KNN(p).GetName()+")";
			for(int x=0; x<kappa.cols(); x++)
				tmp += "," + ToString(KNN(p).GetClassValue(x) );
		
			file6.write(tmp + "\n");
			
			for (size_t y = 0; y<kappa.rows(); y++)
			{
				tmp = ToString(KNN(p).GetClassValue((int)y) );
				for (size_t x = 0; x<kappa.cols(); x++)
					tmp += "," + ToString(kappa[y][x]);
				file6.write(tmp + "\n");
			}

			double k = GetKappa(kappa);
			file7.write(KNN(p).GetName() + "," + ToString(k, 3) + "\n");
		}
	}
	
	file1.close();
	file2.close();
	file3.close();
	file4.close();
	file5.close();

	if( file6.is_open() )
		file6.close();

	if (file7.is_open())
		file7.close();

	return msg;
}



int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));
	CTimer timer(true);

	CKNNMapping KNNMApping;
	ERMsg msg = KNNMApping.m_options.ParseOptions(argc, argv);

	if( !msg || !KNNMApping.m_options.m_bQuiet )
		cout << KNNMApping.GetDescription() << endl;


	if( msg )  
		msg = KNNMApping.Execute();

	if( !msg)  
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if( !KNNMApping.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



