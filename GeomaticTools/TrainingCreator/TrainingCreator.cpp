//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 1.0.0	20/09/2018	Rémi Saint-Amant	Creation 


#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost\multi_array.hpp>
#include <iostream>


#include "TrainingCreator.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/UtilMath.h"
//#include "CSVFile.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "ogr_spatialref.h"


//TEST Image
//=ALEA.ENTRE.BORNES(-2341000,3010750), =ALEA.ENTRE.BORNES(5860250,8500000)



//-dstNoData -32768 -overwrite -ref "D:\Travaux\CloudCleaner\Input\puff_partout\puff_partout_median.vrt" "D:\Travaux\CloudCleaner\Input\puff_partout\puff_partout.tif" "D:\Travaux\CloudCleaner\Model\TraningSource.csv" "D:\Travaux\CloudCleaner\Model\Traning2005-2007.csv"



using namespace std;
using namespace WBSF::Landsat;


namespace WBSF
{
	const char* CTrainingCreator::VERSION = "1.0.0";
	const int CTrainingCreator::NB_THREAD_PROCESS = 2;
	const char * CTrainingCreator::CONDITION_NAME[NB_CONDITION] = { "AllValid", "AtLeastOneValid", "AtLeastOneMissing", "AllMissing" };

	template <class T>
	std::string TestToString(T val, int pres = -1)
	{
		std::string str;
		bool bReal = std::is_same<T, float>::value || std::is_same<T, double>::value;
		if (bReal)
		{
			std::ostringstream st;
			st.imbue(std::locale("C"));
			if (pres < 0)
			{
				st << val;
			}
			else
			{
				st << std::fixed << std::setprecision(pres) << val;
			}

			str = st.str();
			size_t pos = str.find('.');
			if (pos != std::string::npos)//if it's a real;
			{
				int i = (int)str.length() - 1;
				while (i >= 0 && str[i] == '0')
					i--;

				if (i >= 0 && str[i] == '.') i--;
				str = str.substr(0, i + 1);
				if (str.empty())
					str = "0";
			}
		}
		else
		{
			std::ostringstream st;
			st.imbue(std::locale("C"));
			if (pres < 0)
			{
				st << val;
			}
			else
			{
				st << std::setfill(' ') << std::setw(pres) << val;
			}

			str = st.str();
		}



		return str;
	}

	CTrainingCreatorOption::CTrainingCreatorOption() :CBaseOptions(false)
	{
		m_scenesSize = SCENES_SIZE;
		m_precision = 4;
		m_nbPixels = { {1,1} };
		m_bExportAllBand = false;
		m_bExportAll = false;

		m_appDescription = "This software extract LANDSAT pixel from LANDSAT image and coordinates/time's file";

		static const char* DEFAULT_OPTIONS[] = { "-srcnodata", "-dstnodata", "-q", "-overwrite", "-te", "-mask", "-maskValue", "-multi", "-CPU", "-IOCPU", "-wm", "-BlockSize", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);

		AddOption("-period");
		static const COptionDef OPTIONS[] =
		{
			//{ "-Condition", 1, "type", true, "Add conditions to the extraction. 4 possibility of condition can be define: \"AllValid\", \"AtLeastOneValid\", \"AtLeastOneMissing\", \"AllMissing\". No conditions are define by default (all will be output)." },
			{ "-X", 1, "str", false, "File header title for X coordinates. \"X\" by default." },
			{ "-Y", 1, "str", false, "File header title for Y coordinates. \"Y\" by default." },
			{ "JD", 1,"str",false, "File header title for event Julian day 1970 . \"JD\" by default."  },
			{ "-Code", 1, "str", false, "File header title for dependant variable. \"Code\" by default." },
			{ "-nbPixels", 1, "before after", false, "Number of valid pixels to find before and after the event. 1 2 by default." },
			//{ "-Median", 0, "", false, "Extract the median of all scenes and add it at the end of the training file. false by default." },
			{ "-Ref", 1, "refImage", false, "Add reference image value (for example median) at the end of the training file. All scene of the reference imnage will be added." },
			{ "-ExportAllBand", 0, "", false, "Export all bands(B1..JD). B1..B7 by default." },
			{ "-ExportAll", 0, "", false, "Export all input columns. Export only dependant and independant varaibles by default." },
			{ "-windows", 1, "pixels", false, "Buffer windows of extraction. 0 by default." },
			{ "-prec", 1, "precision", false, "Output precision. 4 by default." },
			{ "Image", 0, "", false, "Landsat Image to extract information." },
			{ "srcfile", 0, "", false, "Input coordinate file path (CSV)" },
			{ "dstfile", 0, "", false, "Output information file path (CSV)." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "Image", "", "9", "", "" },
			{ "Input", "srcfile", "", "4 or more", "ID|X|Y|JD|Code|other information...", "The columns order is not important. The coordinates must have a column header \"X\" and \"Y\". A line's ID is recommended because line order is not kept in extraction" },
			{ "Output", "dstfile", "", "", "Code|7*(3+ref)...", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);


		m_XHeader = "X";
		m_YHeader = "Y";
		m_THeader = "JD";
		m_IHeader = "Code";
	}


	ERMsg CTrainingCreatorOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		if (msg && m_filesPath.size() != 3)
		{
			msg.ajoute("Invalid argument line. 3 files are needed: image file, the coordinates (CSV) and destination (CSV).\n");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}


		return msg;
	}

	ERMsg CTrainingCreatorOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		if (IsEqual(argv[i], "-Condition"))
		{
			string conditionName = TrimConst(argv[++i]);

			int condition = -1;
			for (int j = 0; j < NB_CONDITION; j++)
			{
				if (IsEqualNoCase(conditionName, CONDITION_NAME[j]))
				{
					condition = j;
					break;
				}
			}

			if (condition >= 0)
				m_condition.push_back(condition);
			else
				msg.ajoute(conditionName + " is an invalid Condition. See documentation.");
		}
		else if (IsEqual(argv[i], "-X"))
		{
			m_XHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-Y"))
		{
			m_YHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-JD"))
		{
			m_THeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-Code"))
		{
			m_IHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-nbPixels"))
		{
			m_nbPixels[0] = as<int>(argv[++i]);
			m_nbPixels[1] = as<int>(argv[++i]);
		}
		else if (IsEqual(argv[i], "-windows"))
		{
			//m_THeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-Ref"))
		{
			m_refFilePath = argv[++i];
		}
		else if (IsEqual(argv[i], "-prec"))
		{
			m_precision = ToInt(argv[++i]);
		}
		else if (IsEqual(argv[i], "-ExportAllBand"))
		{
			m_bExportAllBand = true;
		}
		else if (IsEqual(argv[i], "-ExportAll"))
		{
			m_bExportAll = true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	bool CTrainingCreatorOption::IsRemoved(const string& str)const
	{
		ASSERT(!m_condition.empty());

		bool bRemoved = false;

		string strNodata = ToString(m_dstNodata, m_precision);
		StringVector input(str, ",");

		for (size_t i = 0; i < m_condition.size() && !bRemoved; i++)
		{
			switch (m_condition[i])
			{
			case ALL_VALID:
				for (size_t z = 0; z < input.size() && !bRemoved; z++)
					bRemoved = input[z] == strNodata;
				break;
			case AT_LEAST_ONE_VALID:
				bRemoved = true;
				for (size_t z = 0; z < input.size() && bRemoved; z++)
					bRemoved = input[z] == strNodata;
				break;
			case AT_LEAST_ONE_MISSING:
				bRemoved = true;
				for (size_t z = 0; z < input.size() && bRemoved; z++)
					bRemoved = input[z] != strNodata;
				break;
			case ALL_MISSING:
				for (size_t z = 0; z < input.size() && !bRemoved; z++)
					bRemoved = input[z] != strNodata;
				break;
			default: ASSERT(false);
			}
		}



		return bRemoved;
	}


	//*************************************************************************************************************************************************************

	//#include <boost/format.hpp>

	ERMsg CTrainingCreator::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl;
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
			cout << "Using:  " << m_options.m_filesPath[IMAGE_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CLandsatDataset inputDS;
		CGDALDatasetEx maskDS;
		CGeoCoordTimeFile iFile;
		CGeoCoordTimeFile oFile;
		CLandsatDataset refDS;

		msg = OpenAll(inputDS, maskDS, refDS, iFile, oFile);

		if (msg)
		{

			CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

			CBandsHolderMT bandHolderRef;
			if (refDS.IsOpen())
			{
				bandHolderRef = CBandsHolderMT(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

				if (maskDS.IsOpen())
					bandHolderRef.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

				msg += bandHolderRef.Load(refDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);
			}

			if (!msg)
				return msg;


			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				printf("Extract %I64u points with %d threads...\n", iFile.size(), m_options.m_bMulti ? m_options.m_CPU : 1);


			CGeoExtents extents = m_options.GetExtents();
			m_options.m_xxFinal = extents.YNbBlocks()*extents.XNbBlocks()*iFile.size();

			//**************************************

			omp_set_nested(1);//for IOCPU
			boost::dynamic_bitset<size_t> treated(iFile.size());

			for (int yBlock = 0; yBlock < extents.YNbBlocks(); yBlock++)
			{
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)	
				for (int xBlock = 0; xBlock < extents.XNbBlocks(); xBlock++)
				{
					int thread_no = ::omp_get_thread_num();

					//CGeoExtents extents = bandHolder[blockThreadNo].GetExtents();
					CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
					if (AtLeastOnePointIn(blockExtents, iFile))
					{
						ReadBlock(xBlock, yBlock, bandHolder[thread_no], bandHolderRef[thread_no]);
						ProcessBlock(xBlock, yBlock, bandHolder[thread_no], bandHolderRef[thread_no], iFile, oFile, treated);
					}
					else
					{
#pragma omp atomic
						m_options.m_xx += (int)iFile.m_xy.size();

						m_options.UpdateBar();
					}

				}//for xBloxk
			}//for yblock



			//add noData to untreated line
			string strNodata = TestToString(m_options.m_dstNodata, m_options.m_precision);
			for (size_t i = 0; i < oFile.size(); i++)
			{
				if (!treated[i])
				{
					size_t nbPixel = m_options.GetNbPixels() + bandHolderRef.GetNbScenes();
					for (size_t z = 0; z < nbPixel; z++)
						for (size_t b = 0; b < m_options.nbBandExport(); z++)
							oFile[i] += ',' + strNodata;
				}
			}


			//apply condition
			if (!m_options.m_condition.empty())
			{
				for (CGeoCoordFile::iterator it = iFile.begin(); it != iFile.end(); it++)
				{
					if (m_options.IsRemoved(*it))
					{
						//remove xy coordinate
						/*size_t pos = std::distance(iFile.begin(), it);
						iFile.m_xy.erase(iFile.m_xy.begin() + pos);

						it = iFile.erase(it);*/
					}
					else
					{
						oFile.push_back(*it);
					//	it++;
					}

				}
			}

			m_options.m_timerWrite.Start();
			msg = oFile.Save(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			m_options.m_timerWrite.Stop();

			CloseAll(inputDS, maskDS, refDS);
		}


		return msg;

	}

	bool CTrainingCreator::AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordTimeFile& ioFile)
	{
		bool bAtLeastOne = false;
		for (int i = 0; i < ioFile.m_xy.size() && !bAtLeastOne; i++)
			bAtLeastOne = blockExtents.IsInside(ioFile.m_xy[i]);

		return bAtLeastOne;
	}

	ERMsg CTrainingCreator::OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& refDS, CGeoCoordTimeFile& iFile, CGeoCoordTimeFile& oFile)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[IMAGE_FILE_PATH], m_options);
		if (msg)
			inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj.get() ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NoData         = " << inputDS.GetNoData(0) << endl;

		}

		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg && !m_options.m_refFilePath.empty())
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Open references..." << endl;

			msg += refDS.OpenInputImage(m_options.m_refFilePath, m_options);
			if (refDS.GetExtents() != inputDS.GetExtents())
			{
				msg.ajoute("Invalid reference's image extents. Reference image must have exactly the same extents than the input image including block size.");
			}
		}


		if (msg)
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Load coordinates..." << endl;

			CTimer timeLoadCSV(true);
			msg = iFile.Load(m_options.m_filesPath[INPUT_FILE_PATH], m_options.m_XHeader, m_options.m_YHeader, m_options.m_THeader);
			timeLoadCSV.Stop();

			if (msg)
			{
				if (!m_options.m_bQuiet)
				{
					CProjectionPtr pPrj = CProjectionManager::GetPrj(iFile.m_xy.GetPrjID());
					string prjName = pPrj ? pPrj->GetName() : "Unknown";

					cout << "    Size           = " << to_string(iFile.m_xy.size()) << " points" << endl;
					cout << "    Projection     = " << prjName << endl;
					cout << "    Time to load   = " << SecondToDHMS(timeLoadCSV.Elapsed()) << endl;
					cout << endl;
				}

				msg = iFile.ManageProjection(inputDS.GetPrjID());
			}
		}


		if (msg)
		{

			if (!m_options.m_bOverwrite && FileExists(m_options.m_filesPath[OUTPUT_FILE_PATH]))
			{
				msg.ajoute("ERROR: Output file already exist. Delete the file or use option -overwrite.");
				return msg;
			}

			ofStream fileOut;
			msg = fileOut.open(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			if (msg)
			{
				fileOut.close();

				//if (!m_options.m_bExportAll)
				//	ioFile.m_header = m_options.m_IHeader + ",";
				
				
				


				if (m_options.m_bExportAll)
				{
					oFile = iFile;
					oFile.m_header = iFile.m_header;
					oFile.m_xy = iFile.m_xy;
					oFile.m_Xcol = iFile.m_Xcol;
					oFile.m_Ycol = iFile.m_Ycol;
					oFile.SetPrjID(iFile.GetPrjID());
					oFile.m_time = iFile.m_time;
				}
				else
				{
					oFile.m_header = m_options.m_IHeader;
					oFile.SetPrjID(iFile.GetPrjID());
					
					//add only Code
					StringVector header(iFile.m_header, ",;");
					set<size_t> posCode = header.FindAll(m_options.m_IHeader);
					if (posCode.size() == 1)
					{
						oFile.reserve(iFile.size());

						for (size_t i = 0; i < iFile.size(); ++i)
						{
							StringVector tmp(iFile.at(i), ",;");
							oFile.push_back(tmp[*posCode.begin()]);
						}
					}
					else
					{
						msg.ajoute("Invalid column header for dependant variable name " + std::to_string(posCode.size()) + " column(s) match for header \"" + m_options.m_IHeader + "\".");
						return msg;
					}
				}
					

				for (size_t i = 0; i < m_options.GetNbPixels(); i++)
				{
					for (size_t j = 0; j < m_options.nbBandExport(); j++)
					{
						string title = "t" + std::to_string(i + 1) + "_" + Landsat::GetBandName(j);

						if (!inputDS.GetInternalName((int)i).empty())
							title = GetFileTitle(inputDS.GetInternalName((int)i));

						oFile.m_header += "," + title;
					}
				}

				//add ref cols
				for (size_t i = 0; i < refDS.GetNbScenes(); i++)
				{
					for (size_t j = 0; j < m_options.nbBandExport(); j++)
					{
						string title = "r" + std::to_string(i + 1) + "_" + Landsat::GetBandName(j);

						if (!inputDS.GetInternalName((int)i).empty())
							title = GetFileTitle(inputDS.GetInternalName((int)i));

						oFile.m_header += "," + title;
					}
				}
			}
		}

		return msg;
	}

	void CTrainingCreator::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CBandsHolder& bandHolderRef)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.Start();
			bandHolder.LoadBlock(xBlock, yBlock);
			if(bandHolderRef.GetRasterCount()>0)
				bandHolderRef.LoadBlock(xBlock, yBlock);

			m_options.m_timerRead.Stop();
		}
	}


	void CTrainingCreator::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CBandsHolder& bandHolderRef, CGeoCoordTimeFile& iFile, CGeoCoordTimeFile& oFile, boost::dynamic_bitset<size_t>& treated)
	{
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
		CGeoRectIndex blockRect = blockExtents.GetPosRect();
		string strNodata = ToString(m_options.m_dstNodata, m_options.m_precision);

		if (bandHolder.IsEmpty())
		{
#pragma omp atomic		
			m_options.m_xx += (int)iFile.m_xy.size();

			m_options.UpdateBar();

			return;
		}


#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.Start();

			//vector<CDataWindowPtr> input;
			//bandHolder.GetWindow(input);
			CLandsatWindow window = bandHolder.GetWindow();
			size_t nbScenes = bandHolder.GetNbScenes();

			CLandsatWindow windowRef = bandHolderRef.GetWindow();
			size_t nbRefScenes = bandHolderRef.GetNbScenes();


			//process all point
			for (int i = 0; i < iFile.m_xy.size(); i++)
			{
				//for all point in the table
				CGeoPoint coordinate = iFile.m_xy[i];
				CGeoPointIndex xy = blockExtents.CoordToXYPos(coordinate);

				//find position in the block
				if (blockRect.IsInside(xy))
				{
					ASSERT(!treated[i]);
					treated.set(i);

					size_t nbPixels = m_options.GetNbPixels();
					CLandsatPixelVector pixels(nbPixels + nbRefScenes);

					__int16 time = as<__int16>(iFile.m_time[i]);
					size_t z = NOT_INIT;
					for (size_t zz = 0; zz < nbScenes && z == NOT_INIT; zz++)
						if (window.GetPixel(zz, xy.m_x, xy.m_y).at(JD) >= time)
							z = zz;

					
					if (z != NOT_INIT)
					{
						for (size_t zz = 0; zz < m_options.m_nbPixels[0] && (z-zz-1)< nbScenes; zz++)
							pixels[m_options.m_nbPixels[0] - zz - 1] = window.GetPixel(z-zz-1, xy.m_x, xy.m_y);

						pixels[m_options.m_nbPixels[0]] = window.GetPixel(z, xy.m_x, xy.m_y);

						for (size_t zz = 0; zz < m_options.m_nbPixels[1] && (z + zz + 1) < nbScenes; zz++)
							pixels[m_options.m_nbPixels[0] + zz + 1] = window.GetPixel(z+zz+1, xy.m_x, xy.m_y);
					}

					//add reference at the end
					for (size_t i = 0; i < nbRefScenes; i++)
					{
						pixels[nbPixels + i] = windowRef.GetPixel(i, xy.m_x, xy.m_y);
					}


					for (size_t z = 0; z < pixels.size(); z++)
					{
						if (pixels[z].IsValid())
						{
							for (size_t zz = 0; zz < m_options.nbBandExport(); zz++)
							{
								DataType v = pixels[z][zz];
								oFile[i] += ',' + ToString(v, m_options.m_precision);
							}
						}
						else
						{
							for (size_t zz = 0; zz < m_options.nbBandExport(); zz++)
							{
								oFile[i] += ',' + strNodata;
							}
						}
					}
				}

#pragma omp atomic 
				m_options.m_xx++;

				m_options.UpdateBar();
			}//for all points
		}//omp critical
	}//ProcessBlock


	void CTrainingCreator::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& refDS)
	{
		inputDS.Close();
		maskDS.Close();
		refDS.Close();

		m_options.PrintTime();
	}
}