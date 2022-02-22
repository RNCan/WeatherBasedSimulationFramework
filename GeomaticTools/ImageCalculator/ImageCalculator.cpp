//ImagesCalculator.exe
// 3.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 3.0.1	10/10/2018	Rémi Saint-Amant	Compile with VS 2017. Add -BLOCK_THREADS	
// 3.0.0	03/11/2017	Rémi Saint-Amant	Compile with GDAL 2.02
// 2.0.1	13/06/2015	Rémi Saint-Amant	Add -hist option
// 2.0.0    11/03/2015	Rémi Saint-Amant	New Equation form "E=mc²"
// 1.7.5	05/02/2015	Rémi Saint-Amant	Bug correction in UnionExtent
// 1.7.4	30/01/2015	Rémi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 1.7.3    27/01/2015	Rémi Saint-Amant	Compile with GDAL 1.11.1
// 1.7.2    25/10/2014	Rémi Saint-Amant	bug correction in help
// 1.7.1    24/07/2014	Rémi Saint-Amant	Bug corrections
// 1.7.0	26/06/2014	Rémi Saint-Amant	GDAL 1.11, UNICODE, VC 2013
// 1.6.4	22/07/2013	Rémi Saint-Amant	Add progress for -Overview and -Stats
// 1.6.3	22/07/2013	Rémi Saint-Amant	Recompilation of cleaned code. Add -stats options
//											Better management of noData NAN and infinite number
//											Limit of no data to -1.0e38 and -1.0e308
// 1.6.2	18/04/2013	Rémi Saint-Amant	new compilation, bug correction in mask
// 1.6.1	12/04/2013  Rémi Saint-Amant	new compilation without debug info. bug correction in write time.
// 1.6.0	27/01/2013  Rémi Saint-Amant	Read by block instead of by row
// 1.5.0	06/11/2012	Rémi Saint-Amant	Improvements
//											ToDo : include new frameworks for image
// 1.4.0	05/04/2012  Rémi Saint-Amant	correction of bug in ManageNoData
// 1.3.0	03/04/2012  Rémi Saint-Amant	correction of bug in omp 
// 1.2.0	15/03/2012	Rémi Saint-Amant	Little changes
// 1.1.0	14/02/2012	Rémi Saint-Amant	Bug correction in the srcNoData
// 1.0.0	30/01/2012	Rémi Saint-Amant	Initial version

//-ot INT16 -ManageSrcNoData -dstnodata "255" -co "COMPRESS=LZW" -multi -overwrite -Equation "if([1][1]==1,if([2][1]>GetNoData(2,1) & [3][1]>GetNoData(3,1),2,if([2][1]>-999999 | [3][1]> -32768,1,0)),255)" "D:\Travail\LucGuindon\MathImage\InputNoData\graticule1.tif" "D:\Travail\LucGuindon\MathImage\InputNoData\Biomass_NFI1.tif" "D:\Travail\LucGuindon\MAthImage\InputNoData\TOTBM_2501.tif" "D:\Travail\LucGuindon\MathImage\Output\test.tif"
//-ot FLOAT32 -multi -overwrite -Equation "sqrt([1][1]*[2][1]-[3][1][-1][2])" -Equation "[1][2]+[2][1]-[3][1]" "D:\Travail\LucGuindon\MathImage\Input\2005_B1A_JA_N.tif" "D:\Travail\LucGuindon\MathImage\Input\2005_B2A_JA_N.tif" "D:\Travail\LucGuindon\MathImage\Input\2005_B6A_JA_N.tif" "D:\Travail\LucGuindon\MathImage\output\test1.tif" 
//-ot int16 -dstnodata "-32768" -co "COMPRESS=LZW" -multi -overwrite -Equation "if([1][1]==0,if([3][1]==[4][1],[2][1],[1][1]),[1][1])"  D:\Travail\LucGuindon\MathImage\subsetTroubleshootingMathImage\for_origin1.tif D:\Travail\LucGuindon\MathImage\subsetTroubleshootingMathImage\for_ori_md11.tif D:\Travail\LucGuindon\MathImage\subsetTroubleshootingMathImage\for_yrnum_ori.tif D:\Travail\LucGuindon\MathImage\subsetTroubleshootingMathImage\for_yrnum_ori_md.tif D:\Travail\LucGuindon\MathImage\subsetTroubleshootingMathImage\for_ori_complete.tif
//-multi -ot Int16 -co "COMPRESS=LZW" -overwrite -Equation "( _1_2_ - _1_1_ ) / ( _1_2_ + _1_1_  )" -Equation "( _1_6_ - _1_5_ ) / ( _1_6_ + _1_5_ )" "D:\Travail\LucGuindon\DecisionTree25m\test_tif.tif" "D:\Travail\LucGuindon\MathImage\Output\testMultiBands.tif"

//-multi -ot Float32 -co "COMPRESS=LZW" -overwrite -Equation "( [1][1] - [2][1]  )" -Equation "( _2_1 - _1_2)" "D:\Travail\LucGuindon\MathImage\subset4test\input.tif" "D:\Travail\LucGuindon\MathImage\subset4test\output3.tif"
//-multi -ot Float32 -co "COMPRESS=LZW" -overwrite -Equation "[1][1]*[2][1]" "D:\Travail\LucGuindon\Input\Small\2005_B1.tif" "D:\Travail\LucGuindon\Input\Small\2005_B2.tif" "D:\Travail\LucGuindon\MathImage\subset4test\output3.tif"
//-CPU 2 -multi -Equation "[1][1] * 199900607" -co "COMPRESS=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -overwrite -ot Int32 "U:\GIS\#documents\TestCodes\MergeImages\Test0\Input\LE70020261999182EDC00_MASKcs.tif" "U:\GIS\#documents\TestCodes\MergeImages\Test0\Input\LE70020261999182EDC00_Date.tif"
//-stats -of VRT -ot Int32 -dstNoData -9999 -overview {2,4,8,16} -co COMPRESS=LZW -overwrite -e "T1=i1b1+i2b2" -e "T2=i1b1-i2b2" "U:\GIS\#documents\TestCodes\ImagesCalculator\Input\2004.tif" "U:\GIS\#documents\TestCodes\ImagesCalculator\Input\2004_2005.vrt" "U:\GIS\#documents\TestCodes\ImagesCalculator\Output\Test2.vrt"

#include "stdafx.h"
#include <unordered_set>
#include <iostream>

#include "Basic/OpenMP.h"
#include "Basic/Ermsg.h"
#include "Basic/Timer.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/ImageParser.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

static const char* version = "3.1.0";


using namespace std;
using namespace WBSF;


class CBandsHolderCalculator;

class CImageCalculatorOption: public CBaseOptions 
{
public:
	
	
	CImageCalculatorOption()
	{
		m_bManageSrcNoData=false; 

		m_appDescription = "This software compute bands from many inputs images into destination image using user define math equation. This Software use MTParser (Mathieu Jacques) to evaluate equation.";

		static const COptionDef OPTIONS[] = 
		{
			{ "-ManageSrcNoData", 0, "", false, "Manage no data in equation with the function GetNoData(ImageNo, BandNo)." },
			{"-Equation",1,"\"formulas\"",true,"Equation to evaluate. One equation per output band. Variables in equation must have the form \"Name=i#b#\" where Name is the nameof the output variable (use when output is VRT), # is 1 to the number of images or bands."},
			{ "-e", 1, "\"formulas\"", true, "Equilvalent to -Equation." },
			{"srcfile",0,"",true, "Input images file path (no limits)."},
			{"dstfile",0,"",false, "Output image file path."}
		};

		
		for(int i=0; i<sizeof(OPTIONS)/sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] = 
		{
			{"Input image","srcfile","1","*","","Multi-layer input"},
			{"Output Image", "dstfile","1","*","","Same as input file"},
		};

		for(int i=0; i<sizeof(IO_FILE_INFO)/sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);
		

	}

	virtual ERMsg ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);
		
		if( msg )
		{
			if( m_filesPath.size() < 2 )
			{
				msg.ajoute("ERROR: Invalid argument line. At least 2 images is needed: input image and destination image.\n");
				msg.ajoute("Argument found: ");
				for (size_t i = 0; i < m_filesPath.size(); i++)
					msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
			}

			if( m_equation.size() == 0)
				msg.ajoute("ERROR: Invalid argument line. At least 1 equation must be define.\n");
		}

		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		if (IsEqual(argv[i], "-Equation") || IsEqual(argv[i], "-e"))
        {
			string name;
			string equation = argv[++i];
			string::size_type pos = equation.find('=');
			if (pos != string::npos)
			{
				name = equation.substr(0, pos);
				equation = equation.substr(pos+1);
			}
			else
			{
				name = "Equation" + ToString(m_equation.size() + 1);
			}

			m_equation.push_back(equation);
			m_name.push_back(name);
        }
		else if( IsEqual(argv[i], "-ManageSrcNoData") )
		{
			m_bManageSrcNoData=true;
		}
		else
		{
			//it's a base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}
		
		return msg;
	}

	
	StringVector m_equation;
	StringVector m_name;
	bool m_bManageSrcNoData;
};


//****************************************************************************************
//CBandsHolderCalculator

class CBandsHolderCalculator: public CBandsHolder
{
public:

	CBandsHolderCalculator(int maxWindowSize=1, double memoryLimit=0, int IOCPU=1):CBandsHolder(maxWindowSize, memoryLimit, IOCPU)
	{}
	
	ERMsg LoadEx(const CGDALDatasetExVector& inputDSVector, const CImageCalculatorOption& option);
	
	virtual double Evaluate(int virtualBandNo, const std::vector<float>& variable){	return m_equations[virtualBandNo].Evaluate(variable); }
	const CImageBandPosVector& GetImageBandToData()const{ return m_imageBandToData; }
	const CVariableToData& GetVariableToData()const{ return m_variableToData; }
	const CImageParserVector& GetVirtualBandVector()const{ return m_equations;}

protected:

	CImageParserVector m_equations;

	CImageBandPosVector m_imageBandToData;
	CVariableToData m_variableToData;
};


ERMsg CBandsHolderCalculator::LoadEx(const CGDALDatasetExVector& inputDSVector, const CImageCalculatorOption& option)
{
	ERMsg msg;

	if(!option.m_bQuiet) 
		cout << "Init band holder using " << m_IOCPU << " IO threads ..." << endl; 

	const StringVector& formulas = option.m_equation;

	//create virtual band
	unordered_set<string> varList;

	m_equations.clear();
	m_equations.resize(formulas.size());
	for(size_t i=0; i<formulas.size()&&msg; i++)
	{
		m_equations[i].m_bManageSrcNoData = option.m_bManageSrcNoData;
		m_equations[i].m_formula = formulas[i];
		msg += m_equations[i].Compile(inputDSVector, option.m_CPU);
		
		if(msg)
		{
			const CMTParserVariableVector& vars = m_equations[i].GetVars();
			for(CMTParserVariableVector::const_iterator it=vars.begin(); it!=vars.end(); it++)
				varList.insert(it->m_name);
		}
	}
	
	if(!msg )
		return msg;

	//now create the link variable bandHolder
	m_imageBandToData.clear();
	m_variableToData.clear();

	m_variableToData.resize(m_equations.size());
	for(size_t i=0; i<m_equations.size(); i++)
	{
		const CMTParserVariableVector& var = m_equations[i].GetVars();
		
		m_variableToData[i].resize(var.size());
		for( size_t j=0; j<m_variableToData[i].size()&&msg; j++)
		{
			CImageBandPos IBpos = GetImageBand(var[j]);

			if(msg)
			{
				vector< pair<size_t, size_t> >::const_iterator item = std::search_n(m_imageBandToData.begin(), m_imageBandToData.end(), 1, IBpos);
			
				if( item == m_imageBandToData.end() )
				{
					//not already in the list
					m_variableToData[i][j] = (int)m_imageBandToData.size();
					m_imageBandToData.push_back(IBpos);
				}
				else
				{
					//already in the list
					m_variableToData[i][j] = int(item-m_imageBandToData.begin());
				}
			}
		}
	}

	//now, add band to band holder
	for(CImageBandPosVector::const_iterator it=m_imageBandToData.begin(); it!=m_imageBandToData.end(); it++)
	{
		CSingleBandHolderPtr pBandHolder = inputDSVector[it->first].GetSingleBandHolder(it->second);
		AddBand(pBandHolder);
	}
	
	m_entireExtents = option.GetExtents();

	for(int i=0; i<(int)m_bandHolder.size(); i++)
		msg += m_bandHolder[i]->Load(this);


	if( !option.m_bQuiet )
	{
		cout << "Memory block: " << m_entireExtents.XNbBlocks() << " Blocks per line x " << m_entireExtents.YNbBlocks() << " Lines" << endl;
		cout << "Memory block size: " << GetBlockSizeX() << " Cols x " << GetBlockSizeY() << " Rows x " << GetBlockSizeZ() << " Bands (max)" << endl;
	}

	return msg;
}

typedef CBandsHolderMTTemplate<CBandsHolderCalculator> CBandsHolderCalculatorMTBase;
class CBandsHolderCalculatorMT: public CBandsHolderCalculatorMTBase
{
public:
	CBandsHolderCalculatorMT(int maxWindowSize=1, double  memoryLimit=0, int IOCPU=1, int nbThread=1):
		CBandsHolderCalculatorMTBase(maxWindowSize, memoryLimit, IOCPU, nbThread)
	{}
	

	ERMsg LoadEx(const CGDALDatasetExVector& inputDSVector, const  CImageCalculatorOption& optionIn)
	{
		CImageCalculatorOption option(optionIn);
		ERMsg msg = CBandsHolderCalculator::LoadEx(inputDSVector, option);

		if (msg)
		{
			option.m_bQuiet=true;
			for(size_t i=0; i<m_thread.size(); i++)
				m_thread[i]->LoadEx(inputDSVector, option);
		}

		
		return msg;
	}
};


//****************************************************************************************

class CImageCalculator
{
public:

	std::string GetDescription(){ return  std::string("ImageCalculator version ") + version + " (" + __DATE__ + ")"; }
	ERMsg Execute();

	ERMsg OpenAll(CGDALDatasetExVector& inputDSVector,CGDALDatasetEx& maskDS,CGDALDatasetEx& outputDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder);
	void ProcessBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder, CGDALDatasetExVector& inputDSVector, CGDALDatasetEx& outputDS, vector< vector< vector<float>>>& output);
	void WriteBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder, CGDALDatasetEx& outputDS, vector< vector< vector<float>>>& output);
	void CloseAll(CGDALDatasetExVector& inputDSVector,CGDALDatasetEx& maskDS,CGDALDatasetEx& outputDS);

	CImageCalculatorOption m_options;
};



ERMsg CImageCalculator::OpenAll(CGDALDatasetExVector& inputDSVector, CGDALDatasetEx& maskDS,CGDALDatasetEx& outputDS)
{
	ERMsg msg;
	
	if(!m_options.m_bQuiet) 
		cout << "Open input image..." << endl;


	size_t nbImages = m_options.m_filesPath.size();
	//Get input file path in
	StringVector filePathIn(nbImages-1);
	for(size_t i=0; i<nbImages-1; i++)
		filePathIn[i] = m_options.m_filesPath[i];
	
	string filePathOut = m_options.m_filesPath[nbImages-1];


	inputDSVector.resize(filePathIn.size());
	for(size_t i=0; i<filePathIn.size()&&msg; i++)
		msg += inputDSVector[i].OpenInputImage(filePathIn[i], m_options );

	if( msg && !m_options.m_maskName.empty() )
	{
		if(!m_options.m_bQuiet) 
			cout << "Open mask..." << endl;

		msg += maskDS.OpenInputImage( m_options.m_maskName );
	}

	if(msg)
	{

		if(!m_options.m_bQuiet) 
			cout << "Open output image..." << endl;

		inputDSVector[0].UpdateOption(m_options);

		CImageCalculatorOption options = m_options;
		options.m_nbBands = (int)options.m_equation.size();
		//if extent is not define, take the extents of all input images
		if (options.m_extents.IsRectEmpty())
			((CGeoRect&)options.m_extents) = inputDSVector.GetExtents();
		
		for (size_t i = 0; i<options.m_name.size(); i++)
		{
			options.m_VRTBandsName += GetFileTitle(filePathOut) + string("_") + options.m_name[i] + ".tif|";
		}

		
		msg += outputDS.CreateImage(filePathOut, options);
	}
	
	return msg;
}




ERMsg CImageCalculator::Execute()
{
	ERMsg msg;

	size_t nbImages = m_options.m_filesPath.size();

	if( !m_options.m_bQuiet )
	{
		
		cout << "Output: " << m_options.m_filesPath[nbImages - 1] << endl;
		cout << "From:   " << endl;
		for(size_t i=0; i<nbImages-1; i++)
			cout << "   " << m_options.m_filesPath[i] << endl;
		if( !m_options.m_maskName.empty() )
			cout << "Mask:   " << m_options.m_maskName << endl;
		cout << "Equations:" << endl;
		for(size_t i=0; i<m_options.m_equation.size(); i++)
			cout << "   " << m_options.m_equation[i] << endl;

		cout << endl;
	}


	CGDALDatasetExVector inputDSVector;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;

	msg = OpenAll(inputDSVector,maskDS,outputDS);

	if(msg)
	{
		if( !m_options.m_bQuiet )		
			cout << "Compile Equations..." << endl << endl;

		CBandsHolderCalculatorMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);
		if( maskDS.IsOpen() )
			bandHolder.SetMask( maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed );
		
		msg += bandHolder.LoadEx(inputDSVector, m_options);
		if(!msg)
			return msg;
	
		if(!m_options.m_bQuiet && m_options.m_bCreateImage) 
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

		
		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
			
		vector<pair<int,int>> XYindex = extents.GetBlockList(10,10);
		
		omp_set_nested(1);//for at leat IOCPU 
		#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti)
		for(int xy=0; xy<(int)XYindex.size(); xy++)
		{
			int threadBlockNo = ::omp_get_thread_num();
			int xBlock=XYindex[xy].first;
			int yBlock=XYindex[xy].second;
			CGeoSize blockSize = extents.GetBlockSize(xBlock,yBlock);
			
			vector< vector< vector<float> > > output;

			ReadBlock(xBlock, yBlock, (CBandsHolderCalculator&)bandHolder[threadBlockNo]);
			ProcessBlock(xBlock, yBlock, (CBandsHolderCalculator&)bandHolder[threadBlockNo], inputDSVector, outputDS, output);
			WriteBlock(xBlock, yBlock, (CBandsHolderCalculator&)bandHolder[threadBlockNo], outputDS, output);
		}//block xy
		
		bandHolder.FlushCache();//clean memory
		CloseAll(inputDSVector, maskDS, outputDS);
	}

	return msg;
}


void CImageCalculator::ReadBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder)
{
	#pragma omp critical(BlockIORead)
	{
		m_options.m_timerRead.Start();
		bandHolder.LoadBlock(xBlock, yBlock);
		m_options.m_timerRead.Stop();
	}
}

void CImageCalculator::ProcessBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder, CGDALDatasetExVector& inputDSVector, CGDALDatasetEx& outputDS, vector< vector< vector<float>>>& output)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock,yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;
	

	const CImageBandPosVector& imageBandToData = bandHolder.GetImageBandToData();
	const CVariableToData& variableToData = bandHolder.GetVariableToData();
	int nbVariables = (int)imageBandToData.size();
	int nbFormulas = (int)bandHolder.GetVirtualBandVector().size();

	if( bandHolder.IsEmpty() )
	{
		#pragma omp atomic
		m_options.m_xx += (min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		
		m_options.UpdateBar();
		return;
	}

	//#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		vector<CDataWindowPtr> input;
		bandHolder.GetWindow(input);

		output.resize(nbFormulas);
		for(int v=0; v<nbFormulas; v++)
		{
			output[v].resize(blockSize.m_y);
			for(int y=0; y<output[v].size(); y++)
				output[v][y].resize(blockSize.m_x);
		}

		
		//Load x,z 
		#pragma omp parallel for /*schedule(static, 100) */num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti)
		for(int y=0; y<blockSize.m_y; y++)
		{
			for(int x=0; x<blockSize.m_x; x++)
			{
				//for all equation (band)
				for(size_t z=0; z<variableToData.size(); z++)
				{
					vector<float> vars(variableToData[z].size());
					for(size_t v=0; v<variableToData[z].size(); v++)
					{
						size_t pos = variableToData[z][v];
						vars[v] = input[pos]->at(x,y);
					}
								
					double value = bandHolder.Evaluate((int)z, vars);
					output[z][y][x] = (float)outputDS.PostTreatment(value);
				}

				#pragma omp atomic 
					m_options.m_xx++;
			} 

			m_options.UpdateBar();
		}//y

		m_options.m_timerProcess.Stop();
	}//process block
}


void CImageCalculator::WriteBlock(int xBlock, int yBlock, CBandsHolderCalculator& bandHolder, CGDALDatasetEx& outputDS, vector< vector< vector<float>>>& output)
{
	#pragma omp critical(BlockIOWrite)
	{
		if (!output.empty())
		{
			m_options.m_timerWrite.Start();
		
			CGeoExtents extents = bandHolder.GetExtents();
			CGeoRectIndex outputRect=extents.GetBlockRect(xBlock,yBlock);

			for(int z=0; z<outputDS.GetRasterCount(); z++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(z);
				for(int y=0; y<(int)output[z].size(); y++)
					pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y+y, outputRect.Width(), 1, &(output[z][y][0]), outputRect.Width(), 1, GDT_Float32, 0, 0  );
			}

			m_options.m_timerWrite.Stop();
		}
	
	}//write block
}

void CImageCalculator::CloseAll(CGDALDatasetExVector& inputDSVector, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	if(!m_options.m_bQuiet)
		cout << "Close..." << endl;

	for( CGDALDatasetExVector::iterator it=inputDSVector.begin(); it!=inputDSVector.end(); it++)
		it->Close();

	maskDS.Close();

	m_options.m_timerWrite.Start();
	
	//if(m_options.m_bComputeStats)
		//outputDS.ComputeStats(m_options.m_bQuiet);

	//if( !m_options.m_overviewLevels.empty() )
		//outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);

	//if (m_options.m_bComputeHistogram)
		//outputDS.ComputeHistogram( m_options.m_bQuiet);

	


	outputDS.Close(m_options);
	
	m_options.m_timerWrite.Stop();
	m_options.PrintTime();
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));
	GDALAllRegister();
	
	CTimer timer(true);

	CImageCalculator imageCalulator;
	ERMsg msg = imageCalulator.m_options.ParseOptions(argc, argv);

	if (!msg || !imageCalulator.m_options.m_bQuiet)
		cout << imageCalulator.GetDescription() << endl;

	if( msg )
		msg = imageCalulator.Execute();

	if( !msg)
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if (!imageCalulator.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}


