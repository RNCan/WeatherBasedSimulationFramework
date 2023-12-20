//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************
// version
// 2.0.1	20/12/2023	Rémi Saint-Amant	change INT16 by UINT16
// 2.0.0	08/09/2023	Rémi Saint-Amant	Use classe 2 with 6 bands
// 1.2.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 1.1.4	29/06/2018  Rémi Saint-Amant	Add -Rename
// 1.1.3	29/06/2018  Rémi Saint-Amant	Add -Virtual
// 1.1.2	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 1.1.1	15/11/2017	Rémi Saint-Amant	remove multi-thread : bad performance
// 1.1.0	02/11/2017	Rémi Saint-Amant	Compile with GDAL 2.02
// 1.0.0	21/12/2016	Rémi Saint-Amant	Creation


//-blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite -Clouds "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Model\V4_SR_DTD1_cloudv4_skip100_200" "U:\GIS1\LANDSAT_SR\LCC\1999-2006.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test2\Output\Test.vrt"
//-stats -Type Oldest -TT OverallYears -of VRT -ot Int16 -blockSize 1024 1024 -co "compress=LZW" -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -IOCPU 3 -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\Input\Test1999-2014.vrt" "U:\GIS\#documents\TestCodes\LandsatWarp\Test0\output\Test.vrt"
//-stats -Virtual -NoResult -Scenes 3 4 -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite "D:\Travaux\CloudCleaner\Input\34 ans\Te1.vrt" "D:\Travaux\CloudCleaner\Output\34 ans\Te1.vrt2"


#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>

#include "Landsat2RGB.h"
#include "basic/OpenMP.h"
#include "basic/UtilMath.h"
#include "geomatic/GDAL.h"


using namespace std;
using namespace WBSF::Landsat2;


namespace WBSF
{
const char* CLandsat2RGB::VERSION = "2.0.1";

//*********************************************************************************************************************

CLandsat2RGBOption::CLandsat2RGBOption()
{

    m_RGBType = NATURAL;
    m_outputType = GDT_Byte;
    m_scenes_def = { B1,B2,B3,B4,B5,B7 };
    //m_scenes = { { NOT_INIT, NOT_INIT } };
    m_dstNodata = 255;
    m_bust = { { 0, 255 } };
    m_bVirtual = false;



    m_appDescription = "This software transform Landsat images (composed of " + to_string(SCENES_SIZE) + " bands) into RGB image.";


    static const COptionDef OPTIONS[] =
    {
        { "-RGB", 1, "t", false, "RGB Type. Type can be Natural or LandWater. NATURAL by default." },
        //			{ "-SceneSize", 1, "size", false, "Number of images per scene. 9 by default." },//override scene size definition
        //			{ "-Scenes", 2, "first last", false, "Select a first and the last scene (1..nbScenes) to clean cloud. All scenes are selected by default." },
        { "-Virtual", 0, "", false, "Create virtual (.vrt) output file based on input files. Combine with -NoResult, this avoid to create new files. " },
        { "-Bust", 2, "min max", false, "replace busting pixel (lesser than min or greater than max) by no data. 0 and 255 by default." },
        { "srcfile", 0, "", false, "Input image file path." },
        { "dstfile", 0, "", false, "Output image file path." }
    };

    for (size_t i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
        AddOption(OPTIONS[i]);

    //AddOption("-RGB");
    //AddOption("-Rename");
    RemoveOption("-ot");
    RemoveOption("-CPU");//no multi thread in inner loop

    static const CIOFileInfoDef IO_FILE_INFO[] =
    {
        { "Input Image", "srcfile", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|Date: date of image(Julian day 1970 or YYYYMMDD format)|... for all scenes", "" },
        { "Output Image", "dstfile", "nbScenes", "3 color (RGB) image", "B1:red|B2:green|B3:blue", "" },
    };

    for (size_t i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
        AddIOFileInfo(IO_FILE_INFO[i]);
}

ERMsg CLandsat2RGBOption::ParseOption(int argc, char* argv[])
{
    ERMsg msg = CBaseOptions::ParseOption(argc, argv);

    assert(NB_FILE_PATH == 2);
    if (msg && m_filesPath.size() != NB_FILE_PATH)
    {
        msg.ajoute("ERROR: Invalid argument line. 2 files are needed: the source and destination image.");
        msg.ajoute("Argument found: ");
        for (size_t i = 0; i < m_filesPath.size(); i++)
            msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
    }

    m_outputType = GDT_Byte;

    return msg;
}

ERMsg CLandsat2RGBOption::ProcessOption(int& i, int argc, char* argv[])
{
    ERMsg msg;

    if (IsEqual(argv[i], "-RGB"))
    {
        string str = argv[++i];

        m_RGBType = NO_RGB;
        for (size_t i = 0; i < NB_RGB && m_RGBType == NO_RGB; i++)
            if (IsEqualNoCase(str, RGB_NAME[i]))
                m_RGBType = static_cast<TRGBTye>(i);


        if(m_RGBType == NO_RGB)
            msg.ajoute("Bad RGB type format. RGB type format must be \"Natural\", \"LandWater\" or \"TrueColor\"");

    }
    else if (IsEqual(argv[i], "-Bust"))
    {
        m_bust[0] = ToInt(argv[++i]);
        m_bust[1] = ToInt(argv[++i]);
    }
    else if (IsEqual(argv[i], "-Virtual"))
    {
        m_bVirtual = true;
    }
    else
    {
        //Look to see if it's a know base option
        msg = CBaseOptions::ProcessOption(i, argc, argv);
    }


    return msg;
}


ERMsg CLandsat2RGB::Execute()
{
    ERMsg msg;

    if (!m_options.m_bQuiet)
    {
        cout << "Output: " << m_options.m_filesPath[CLandsat2RGBOption::OUTPUT_FILE_PATH] << endl;
        cout << "From:   " << m_options.m_filesPath[CLandsat2RGBOption::INPUT_FILE_PATH] << endl;

        if (!m_options.m_maskName.empty())
            cout << "Mask:   " << m_options.m_maskName << endl;
    }

    GDALAllRegister();

    CLandsatDataset inputDS;
    CGDALDatasetEx maskDS;
    vector<CGDALDatasetEx> outputDS;
    msg = OpenAll(inputDS, maskDS, outputDS);
    if (!msg)
        return msg;

    if (m_options.m_bCreateImage)
    {
        size_t nbScenedProcess = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
        CGeoExtents extents = m_options.GetExtents();
        m_options.ResetBar(nbScenedProcess * extents.m_xSize * extents.m_ySize);
        vector<pair<int, int>> XYindex = extents.GetBlockList(); //extents.GetBlockList(5, 5);

        m_options.m_stats.resize(nbScenedProcess);
        //if (m_options.m_RGBType == CBaseOptions::TRUE_COLOR)
        {
            //Get statistic of the image
            for (size_t zz = 0; zz < nbScenedProcess; zz++)
            {
                for (size_t b = B2; b <= B5; b++)
                {
                    size_t z = (m_options.m_scene_extents[0] + zz) * SCENES_SIZE + b;
                    GDALRasterBand* pBand = inputDS.GetRasterBand(z);
                    pBand->GetStatistics(false, true, &m_options.m_stats[zz][b].m_min, &m_options.m_stats[zz][b].m_max, &m_options.m_stats[zz][b].m_mean, &m_options.m_stats[zz][b].m_sd);
                }
            }
        }


        omp_set_nested(1);//for IOCPU
        #pragma omp parallel for schedule(static, 1) num_threads( m_options.m_BLOCK_THREADS ) if (m_options.m_bMulti)
        for (int b = 0; b < (int)XYindex.size(); b++)
        {
            int xBlock = XYindex[b].first;
            int yBlock = XYindex[b].second;

            Landsat2::CLandsatWindow block_data;
            ReadBlock(inputDS, xBlock, yBlock, block_data);
            for (size_t zz = 0; zz < nbScenedProcess; zz++)
            {
                size_t z = m_options.m_scene_extents[0] + zz;

                OutputData outputData;
                ProcessBlock(xBlock, yBlock, block_data, z, outputData);
                WriteBlock(xBlock, yBlock, outputDS[zz], outputData);
            }
        }//for all blocks


    }

    if (msg && m_options.m_bVirtual)
        CreateVirtual(inputDS);


    CloseAll(inputDS, maskDS, outputDS);

    return msg;
}



ERMsg CLandsat2RGB::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS)
{
    ERMsg msg;

    if (!m_options.m_bQuiet)
        cout << endl << "Open input image..." << endl;

    msg = inputDS.OpenInputImage(m_options.m_filesPath[CLandsat2RGBOption::INPUT_FILE_PATH], m_options);
    if (!msg)
        return msg;

    inputDS.UpdateOption(m_options);


    if (msg && !m_options.m_bQuiet)
    {
        /*if (m_options.m_period.IsInit())
        {
        	const std::vector<CTPeriod>& scenesPeriod = inputDS.GetScenePeriod();
        	CTPeriod p = m_options.m_period;

        	set<size_t> selected;

        	for (size_t s = 0; s < scenesPeriod.size(); s++)
        	{
        		if (p.IsIntersect(scenesPeriod[s]))
        			selected.insert(s);
        	}

        	if (!selected.empty())
        	{
        		m_options.m_scenes[0] = *selected.begin();
        		m_options.m_scenes[1] = *selected.rbegin();
        	}
        	else
        	{
        		msg.ajoute("No input scenes intersect -PeriodTrait (" + m_options.m_period.GetFormatedString("%1 %2", "%F") + ").");
        	}
        }*/


        CGeoExtents extents = inputDS.GetExtents();
        //CProjectionPtr pPrj = inputDS.GetPrj();
        //string prjName = pPrj ? pPrj->GetName() : "Unknown";

       /* if (m_options.m_scenes[0] == NOT_INIT)
            m_options.m_scenes[0] = 0;

        if (m_options.m_scenes[1] == NOT_INIT)
            m_options.m_scenes[1] = inputDS.GetNbScenes() - 1;*/

        
        //	const std::vector<CTPeriod>& p = inputDS.GetScenePeriod();

        //set the period to the period in the scene selection
//			size_t nbSceneLoaded = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
        /*CTPeriod period;
        for (size_t z = 0; z < nbSceneLoaded; z++)
        	period.Inflate(p[m_options.m_scenes[0] + z]);*/

        //if (m_options.m_period.IsInit())
        //				cout << "    User's Input loading period  = " << m_options.m_period.GetFormatedString() << endl;

        cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
        cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
        //cout << "    Projection     = " << prjName << endl;
        cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
        cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
        cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;
        //			cout << "    Entire period  = " << inputDS.GetPeriod().GetFormatedString() << " (nb scenes = " << inputDS.GetNbScenes() << ")" << endl;
        //		cout << "    Loaded period  = " << period.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;


        //	m_options.m_period = period;

        if (inputDS.GetSceneSize() != SCENES_SIZE)
            cout << "WARNING: the number of bands per scene (" << to_string(inputDS.GetSceneSize()) << ") is different than the inspected number (" << to_string(SCENES_SIZE) << ")" << endl;

        //if (m_options.m_scene >= inputDS.GetNbScenes())
        //msg.ajoute("Scene " + ToString(m_options.m_scene+1) + "must be smaller thant the number of scenes of the input image");
    }

    if (msg && !m_options.m_maskName.empty())
    {
        if (!m_options.m_bQuiet)
            cout << "Open mask..." << endl;

        msg += maskDS.OpenInputImage(m_options.m_maskName);
    }


    if (msg && m_options.m_bCreateImage)
    {
        size_t nbScenedProcess = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;

        outputDS.resize(nbScenedProcess);
        //replace the common part by the new name
        set<string> subnames;
        for (size_t zz = 0; zz < nbScenedProcess; zz++)
        {
            size_t z = m_options.m_scene_extents[0] + zz;

            CLandsat2RGBOption options(m_options);
            string filePath = options.m_filesPath[CLandsat2RGBOption::OUTPUT_FILE_PATH];
            string path = GetPath(filePath);

            options.m_nbBands = 3;
            options.m_outputType = GDT_Byte;
            options.m_format = "GTiff";

            string subName = inputDS.GetSubname(z);
            filePath = path + GetFileTitle(filePath) + "_" + subName + "_RGB.tif";

            msg += outputDS[zz].CreateImage(filePath, options);
        }


        if (!m_options.m_bQuiet)
        {
            cout << endl;
            cout << "Open output images..." << endl;
            
            cout << "    Size           = " << m_options.m_extents.m_xSize << " cols x " << m_options.m_extents.m_ySize << " rows x  3 bands" << endl;
            cout << "    Extents        = X:{" << ToString(m_options.m_extents.m_xMin) << ", " << ToString(m_options.m_extents.m_xMax) << "}  Y:{" << ToString(m_options.m_extents.m_yMin) << ", " << ToString(m_options.m_extents.m_yMax) << "}" << endl;
            cout << "    Nb images      = " << nbScenedProcess << endl;

//            cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
  //          cout << "    Scene size     = " << m_options.GetSceneSize() << endl;
    //        cout << "    Nb. Scenes     = " << nbScenedProcess << endl;
        }


    }


    return msg;
}


void CLandsat2RGB::ReadBlock(Landsat2::CLandsatDataset& inputDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data)
{
    #pragma omp critical(BlockIO)
    {
        m_options.m_timerRead.start();

        CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
        inputDS.CGDALDatasetEx::ReadBlock(extents, block_data, 1, m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);
        m_options.m_timerRead.stop();
    }
}


void CLandsat2RGB::ProcessBlock(int xBlock, int yBlock, Landsat2::CLandsatWindow& window, size_t z, OutputData& outputData)
{
    CGeoExtents extents = m_options.GetExtents();
    CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
    size_t zz = z - m_options.m_scene_extents[0];

    if (window.empty())
    {
        #pragma omp atomic
        m_options.m_xx += blockSize.m_x * blockSize.m_y;

        m_options.UpdateBar();
        return;
    }

    //CLandsatWindow window = static_cast<CLandsatWindow&>(bandHolder.GetWindow());

    if (m_options.m_bCreateImage)
    {
        Color8 dstNodata = (Color8)m_options.m_dstNodata;
        outputData.resize(3);
        for (size_t j = 0; j < outputData.size(); j++)
            outputData[j].resize(blockSize.m_x * blockSize.m_y, dstNodata);
    }

    m_options.m_timerProcess.start();


    for (size_t y = 0; y < size_t(blockSize.m_y); y++)
    {
        for (size_t x = 0; x < size_t(blockSize.m_x); x++)
        {
            assert(z < window.size());
            CLandsatPixel pixel = window.GetPixel(z, (int)x, (int)y);
            if (pixel.IsValid())
            {
                bool bIsBlack = pixel.IsBlack();

                if (!bIsBlack)
                {

                    Color8 R = pixel.R(m_options.m_RGBType, m_options.m_stats[zz]);
                    Color8 G = pixel.G(m_options.m_RGBType, m_options.m_stats[zz]);
                    Color8 B = pixel.B(m_options.m_RGBType, m_options.m_stats[zz]);

                    bool bIsBust = m_options.IsBusting(R, G, B);

                    if (!bIsBust)
                    {
                        outputData[0][y * blockSize.m_x + x] = R;
                        outputData[1][y * blockSize.m_x + x] = G;
                        outputData[2][y * blockSize.m_x + x] = B;
                    }
                }
            }

            #pragma omp atomic
            m_options.m_xx++;

        }
    }

    m_options.UpdateBar();
    m_options.m_timerProcess.stop();


}

void CLandsat2RGB::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData)
{
    #pragma omp critical(BlockIO)
    {
        m_options.m_timerWrite.start();


        //CGeoExtents extents = bandHolder.GetExtents();
        //CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
        //CTPeriod period = m_options.GetTTPeriod();
        //int nbSegment = period.GetNbRef();

        if (outputDS.IsOpen())
        {
            CGeoExtents extents = outputDS.GetExtents();
            CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

            assert(outputRect.m_x >= 0 && outputRect.m_x < outputDS.GetRasterXSize());
            assert(outputRect.m_y >= 0 && outputRect.m_y < outputDS.GetRasterYSize());
            assert(outputRect.m_xSize > 0 && outputRect.m_xSize <= outputDS.GetRasterXSize());
            assert(outputRect.m_ySize > 0 && outputRect.m_ySize <= outputDS.GetRasterYSize());

            for (size_t z = 0; z < outputData.size(); z++)
            {
                GDALRasterBand* pBand = outputDS.GetRasterBand(z);
                if (!outputData[z].empty())
                {
                    pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Byte, 0, 0);
                }
                else
                {
                    Color8 noData = (Color8)outputDS.GetNoData(z);
                    pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Byte, 0, 0);
                }
            }
        }


        m_options.m_timerWrite.stop();
    }
}


ERMsg CLandsat2RGB::CreateVirtual(CLandsatDataset& inputDS)
{
    ERMsg msg;

    size_t nbScenedProcess = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
    for (size_t zz = 0; zz < nbScenedProcess; zz++)
    {
        size_t z = m_options.m_scene_extents[0] + zz;

        string subName = WBSF::TrimConst(inputDS.GetCommonImageName(z), "_");
        std::string filePath = m_options.m_filesPath[CLandsat2RGBOption::OUTPUT_FILE_PATH];
        filePath = GetPath(filePath) + GetFileTitle(filePath) + "_" + subName + "_RGB.vrt";
        //msg += inputDS.CreateRGB(z, filePath, (CBaseOptions::TRGBTye)m_options.m_RGBType);
    }

    return msg;
}

void CLandsat2RGB::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS)
{
    inputDS.Close();
    maskDS.Close();

    m_options.m_timerWrite.start();

    for (size_t i = 0; i < outputDS.size(); i++)
        outputDS[i].Close(m_options);

    m_options.m_timerWrite.stop();
    //m_options.PrintTime();
}


}
