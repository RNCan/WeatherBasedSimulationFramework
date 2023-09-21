//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework 
//******************************************************************************
#include "stdafx.h"
#include <float.h>
#include <limits.h>
#include <share.h>
#include <iostream>
#include <wtypes.h>
#include <chrono>

#include "MTParser/MTParser.h"
#include "Basic/OpenMP.h"
#include "Basic/Statistic.h"
#include "Basic/CSV.h"
#include "Basic/UtilStd.h" 
#include "Basic/UtilMath.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDAL.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/GDALBasic.h"
#include "geomatic/LandsatDataset1.h"

#include "WeatherBasedSimulationString.h"

#include "Basic/xmlite.h"

using namespace std;
namespace WBSF
{

	//**********************************************************************************
	//CGDALDatasetEx
	void CDataWindow::GetSlopeAndAspect(int x, int y, double& slope, double& aspect)const
	{
		slope = aspect = -999;

		if (m_pData != NULL)
		{
			ASSERT(IsValid(x, y));

			float window[3][3] = { 0 };

			for (int xx = 0; xx < 3; xx++)
			{
				for (int yy = 0; yy < 3; yy++)
				{
					if (IsValid(x + xx - 1, y + yy - 1))
						window[yy][xx] = at(x + xx - 1, y + yy - 1);
					else
						window[yy][xx] = at(x, y);//center cell if invalid edge
				}
			}


			//double scale = m_bProjected ? 1 : 111120;
			//if( IsValidSlopeWindow(window, m_noData ) )
			double ewres = m_extents.XRes();
			double nsres = m_extents.YRes();
			if (m_extents.IsGeographic())
			{
				//compute local resolution in meteres
				CGeoExtents pixel_extents = m_extents.GetPixelExtents(CGeoPointIndex(x, y));
				ewres = pixel_extents.CenterLeft().GetDistance(pixel_extents.CenterRight());
				nsres = -pixel_extents.UpperMidle().GetDistance(pixel_extents.LowerMidle());//sort north resolution is negativ
			}

			//blockExtents.GetPixelExtents(bool bInMeters);

			WBSF::GetSlopeAndAspect(window, ewres, nsres, 1, slope, aspect);
		}
	}
	//**********************************************************************************
	//CGDALDatasetEx


	CGDALDatasetEx::CGDALDatasetEx()
	{
		m_poDataset = NULL;
		m_bVRT = false;
		m_bMultipleImages = false;
		m_bOpenUpdate = false;
	}

	CGDALDatasetEx::~CGDALDatasetEx()
	{
		Close(CBaseOptions());
	}

	void CGDALDatasetEx::Close(const CBaseOptions& options)
	{
		if (IsOpen())
		{
			if (m_bOpenUpdate)
			{
				if (options.m_bComputeStats)
					ComputeStats(options.m_bQuiet);

				if (!options.m_overviewLevels.empty())
					BuildOverviews(options.m_overviewLevels, options.m_bQuiet);

				if (options.m_bComputeHistogram)
					ComputeHistogram(options.m_bQuiet);
			}


			if (m_bMultipleImages)
			{
				for (size_t i = 0; i < m_poDatasetVector.size(); i++)
					CloseVRTBand(i);

				//build vrt must be done after closing images to have valid images.
				BuildVRT(options);


				m_poDatasetVector.clear();
				m_bMultipleImages = false;
			}
			else
			{
				::GDALClose(m_poDataset);
				m_poDataset = NULL;
			}

			m_pProjection.reset();
			m_filePath.clear();
			m_extents.clear();
			m_bVRT = false;
			m_bOpenUpdate = false;
			m_internalExtents.clear();
			m_internalName.clear();
			m_virtualBands.clear();
			m_scenesPeriod.clear();

			//this vector is intended to keep each band of a VRT file open for writing
			ASSERT(m_poDataset == NULL);
			ASSERT(m_poDatasetVector.empty());

			//temporal section of the dataset
		}
	}

	short CGDALDatasetEx::GetDataType(size_t i)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);
		GDALRasterBand* pBand = me.GetRasterBand(i);

		short eType = pBand->GetRasterDataType();
		return eType;
	}

	ERMsg CGDALDatasetEx::VerifyNoData(double nodata, size_t i)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::VerifyNoData(nodata, eType);
	}

	double CGDALDatasetEx::GetBandLimit(size_t i, bool bLow)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::GetTypeLimit(eType, bLow);
	}

	double CGDALDatasetEx::PostTreatment(double v, size_t i, double shiftedBy, bool bRound)
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		if (v == MISSING_DATA)
		{
			int bSuccess = 0;
			double noData = GetRasterBand(i)->GetNoDataValue(&bSuccess);
			if (bSuccess)
				v = noData;
		}

		return LimitToBound(v, i, shiftedBy, bRound);
	}

	double CGDALDatasetEx::LimitToBound(double v, size_t i, double shiftedBy, bool bRound)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::LimitToBound(v, eType, shiftedBy, bRound);
	}

	double CGDALDatasetEx::GetDefaultNoData(size_t i)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::GetDefaultNoData(eType);
	}

	bool CGDALDatasetEx::HaveNoData(size_t i)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));

		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);

		int bSuccess = 0;
		me.GetRasterBand(i)->GetNoDataValue(&bSuccess);
		return bSuccess != 0;
	}

	double CGDALDatasetEx::GetNoData(size_t i)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(GetRasterBand(i));
		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);

		return WBSF::GetNoData(me.GetRasterBand(i));
	}

	vector<double> CGDALDatasetEx::GetNoData()const
	{
		vector<double> noData(GetRasterCount());
		for (size_t b = 0; b < GetRasterCount(); b++)
			noData[b] = GetNoData(b);

		return noData;
	}



	GDALRasterBand * CGDALDatasetEx::GetRasterBand(size_t i)
	{
		ASSERT(i < GetRasterCount());
		return m_bMultipleImages ? m_poDatasetVector[i]->GetRasterBand(1) : m_poDataset->GetRasterBand(int(i) + 1);

	}

	const GDALRasterBand * CGDALDatasetEx::GetRasterBand(size_t i)const
	{
		ASSERT(i < GetRasterCount());
		return m_bMultipleImages ? m_poDatasetVector[i]->GetRasterBand(1) : m_poDataset->GetRasterBand(int(i) + 1);

	}


	ERMsg CGDALDatasetEx::OpenInputImage(const string& filePath, const CBaseOptions& options)
	{
		ASSERT(!IsOpen());

		ERMsg msg;

		msg = WBSF::OpenInputImage(filePath, &m_poDataset, options.m_srcNodata, options.m_bUseDefaultNoData, options.m_bReadOnly);
		if (msg)
		{
			m_extents = WBSF::GetExtents(m_poDataset);

			const char* desc = m_poDataset->GetDriver()->GetDescription();
			m_bVRT = strcmp(desc, "VRT") == 0;

			msg = CProjectionManager::CreateProjection(m_poDataset->GetProjectionRef());

			if (msg)
			{
				m_pProjection = CProjectionManager::GetPrj(m_poDataset->GetProjectionRef());
				m_internalExtents.reserve(GetRasterCount());
				m_internalName.reserve(GetRasterCount());
				//m_poDataset->GetFileList();

				//Load internal rect
				map<int, int> xBlockSizes;
				map<int, int> yBlockSizes;
				XNode xmlDataset;
				XNodes bandsDef;
				if (m_bVRT)
				{
					ifStream file;

					if (file.open(filePath))
					{
						string str = file.GetText();

						xmlDataset.Load(str.c_str());
						bandsDef = xmlDataset.GetChilds("VRTRasterBand");
						ASSERT(bandsDef.size() == GetRasterCount());

						//if (!bandsDef.empty())
						if (bandsDef.size() == 1)
						{
							//LPXNode node = bandsDef.front();
							//When there is no nodata, vrt with -separate have singleSource
							//if (node && node->GetChilds("ComplexSource").size() != 1)
							//{
							//	//VRT file without -separate options. 
							//	considerate VRT with only one band as standard image
							m_bVRT = false;
							//}
						}

						file.close();
					}
				}

				for (size_t j = 0; j < GetRasterCount(); j++)
				{
					string name;
					string virtualBandEquation;

					CGeoExtents extents = m_extents;

					if (m_bVRT && bandsDef.size() == GetRasterCount())
					{
						LPXNode pSourceFileName = bandsDef[j]->Find("SourceFilename");
						if (pSourceFileName)
						{
							bool bRelativeToVRT = ToBool(pSourceFileName->GetAttrValue("relativeToVRT"));
							if (bRelativeToVRT)
							{
								name = GetPath(filePath);
								name += pSourceFileName->value;
							}
							else
							{
								name = pSourceFileName->value;
							}
						}


						CGeoSize size;
						CGeoSize blockSize;
						LPXNode pSourceProperties = bandsDef[j]->Find("SourceProperties");
						if (pSourceProperties)
						{
							size.m_x = atoi(pSourceProperties->GetAttrValue("RasterXSize"));
							size.m_y = atoi(pSourceProperties->GetAttrValue("RasterYSize"));
							blockSize.m_x = atoi(pSourceProperties->GetAttrValue("BlockXSize"));
							blockSize.m_y = atoi(pSourceProperties->GetAttrValue("BlockYSize"));
							xBlockSizes[blockSize.m_x]++;
							yBlockSizes[blockSize.m_y]++;
						}

						LPXNode pDstRect = bandsDef[j]->Find("DstRect");
						if (pDstRect && pDstRect->attrs.size() == 4 && size.m_x > 0 && size.m_y > 0)
						{
							CGeoRectIndex rect;
							rect.m_x = atoi(pDstRect->GetAttrValue("xOff"));
							rect.m_y = atoi(pDstRect->GetAttrValue("yOff"));
							rect.m_xSize = atoi(pDstRect->GetAttrValue("xSize"));
							rect.m_ySize = atoi(pDstRect->GetAttrValue("ySize"));

							extents = CGeoExtents(m_extents.XYPosToCoord(rect), size, blockSize);
						}

						//if its an virtual band, load equation
						LPCSTR pSubClassName = bandsDef[j]->GetAttrValue("subClass");
						if (pSubClassName && IsEqual(pSubClassName, "VRTDerivedRasterBand"))
						{
							LPCSTR pPixelFuncName = bandsDef[j]->GetChildValue("PixelFunctionType");
							if (pPixelFuncName&& IsEqual(pPixelFuncName, "ImageCalculator"))
							{
								LPXNode pMeta = bandsDef[j]->GetChild("Metadata");
								if (pMeta)
								{
									XNodes nodes = pMeta->GetChilds("MDI");
									for (XNodes::const_iterator it = nodes.begin(); it != nodes.end(); it++)
									{
										LPCSTR pKey = (*it)->GetAttrValue("key");
										if (pKey && IsEqual(pKey, "Equation"))
										{
											virtualBandEquation = (*it)->value;
											break;
										}
									}
								}
							}
						}
					}//if bandDef size == raster count (VRT)

					m_internalExtents.push_back(extents);
					m_internalName.push_back(name);
					m_virtualBands.push_back(virtualBandEquation);
				}//for raster count

				if (m_bVRT && !xBlockSizes.empty() && !yBlockSizes.empty())
				{
					//Get the most popular block sise for x and y
					std::map< int, int > xFlipedMap = converse_map(xBlockSizes);
					std::map< int, int > yFlipedMap = converse_map(yBlockSizes);

					m_extents.m_xBlockSize = xFlipedMap.rbegin()->second;
					m_extents.m_yBlockSize = yFlipedMap.rbegin()->second;

				}
			}//if msg
		}//if msg
		else
		{
			msg.ajoute("Unable to open " + filePath);
		}



		if (msg)
			m_filePath = filePath;



		return msg;
	}

	ERMsg CGDALDatasetEx::CreateImage(const string& filePath, const CBaseOptions& options)
	{
		ERMsg msg;


		if (!options.m_bOverwrite)
		{
			//verify that the file does'nt exist
			if (FileExists(filePath))
			{
				msg.ajoute("ERROR: Output file \"" + filePath + "\" already exist. Delete the file or use options -overwrite.");
				return msg;
			}
		}

		GDALDriver* poDriverOut = (GDALDriver*)GDALGetDriverByName(options.m_format.c_str());
		if (poDriverOut == NULL)
		{
			msg.ajoute(string("ERROR: Unable to find driver : ") + options.m_format);
			return msg;
		}


		const char* desc = poDriverOut->GetDescription();
		m_bVRT = strcmp(desc, "VRT") == 0;
		if (m_bVRT)
		{
			m_bMultipleImages = true;
			m_poDatasetVector.resize(options.m_nbBands);
			m_internalName.resize(options.m_nbBands);

			if (options.m_bOpenBandAtCreation)
			{
				StringVector bandsName(options.m_VRTBandsName, "|,;");
				ASSERT(options.m_nbBands < 65000);
				CBaseOptions optionsB(options);
				optionsB.m_format = "GTiff";
				optionsB.m_nbBands = 1;
				for (size_t i = 0; i < options.m_nbBands&&msg; i++)
				{
					if (bandsName.size() == options.m_nbBands)
					{
						m_internalName[i] = filePath;
						SetFileName(m_internalName[i], GetFileName(bandsName[i]));
					}
					else
					{
						m_internalName[i] = filePath;
						SetFileName(m_internalName[i], GetFileTitle(filePath) + "_B" + to_string(i + 1) + ".tif");
					}

					CGDALDatasetEx band;
					msg = band.CreateImage(m_internalName[i], optionsB);
					if (msg)
						SetVRTBand(i, band.Detach());
				}
			}
		}
		else
		{
			char **papszOptions = NULL;
			for (size_t i = 0; i < options.m_createOptions.size(); i++)
				papszOptions = CSLAddString(papszOptions, options.m_createOptions[i].c_str());

			m_poDataset = poDriverOut->Create(filePath.c_str(), options.m_extents.m_xSize, options.m_extents.m_ySize, int(options.m_nbBands), (GDALDataType)options.m_outputType, papszOptions);

			CPLFree(papszOptions);
			papszOptions = NULL;

			if (m_poDataset == NULL)
			{
				msg.ajoute(CPLGetLastErrorMsg());
				return msg;
			}


			//init output image with input image
			CGeoTransform GT;
			options.m_extents.GetGeoTransform(GT);
			m_poDataset->SetGeoTransform(GT);
			m_poDataset->SetProjection(options.m_prj.c_str());


			for (size_t i = 0; i < options.m_nbBands&&msg; i++)
			{
				double dstNoData = options.m_dstNodata;
				if (dstNoData == MISSING_NO_DATA)
					if (options.m_bUseDefaultNoData)
						dstNoData = WBSF::GetDefaultNoData(options.m_outputType);

				//if no data isn't supply, take the input no data (if we have the same number of band) if present, if not take smaller value of the type
				if (dstNoData != MISSING_NO_DATA)
				{
					msg += WBSF::VerifyNoData(dstNoData, options.m_outputType);
					GetRasterBand(i)->SetNoDataValue(dstNoData);
				}
			}
		}

		if (msg)
		{
			m_pProjection = GetProjection(options.m_prj);
			m_filePath = filePath;
			m_extents = options.m_extents;
			m_bOpenUpdate = true;
			//Dataset()->GetAccess() == GA_Update;
		}


		return msg;
	}

	int CGDALDatasetEx::GetRasterXSize()const { return m_extents.m_xSize; }
	int CGDALDatasetEx::GetRasterYSize()const { return m_extents.m_ySize; }
	size_t CGDALDatasetEx::GetRasterCount()const { return m_bMultipleImages ? (int)m_poDatasetVector.size() : m_poDataset->GetRasterCount(); }

	void CGDALDatasetEx::SetVRTBand(size_t i, GDALDataset* pDataset)
	{
		ASSERT(m_bMultipleImages);
		ASSERT(i <= m_poDatasetVector.size());
		ASSERT(pDataset);

		char* pList = NULL;

		char** papszFileList = pDataset->GetFileList();
		int fileCount = CSLCount(papszFileList);
		if (fileCount == 1)
			m_internalName[i] = papszFileList[0];

		CSLDestroy(papszFileList);

		m_poDatasetVector[i] = pDataset;
	}

	void CGDALDatasetEx::CloseVRTBand(size_t  i)
	{
		ASSERT(m_bMultipleImages);
		ASSERT(i < m_poDatasetVector.size());

		if (m_poDatasetVector[i])
		{
			::GDALClose((GDALDatasetH)m_poDatasetVector[i]);
			m_poDatasetVector[i] = NULL;
		}
	}

	ERMsg CGDALDatasetEx::BuildVRT(const CBaseOptions& options)
	{
		ERMsg msg;



		if (!options.m_bQuiet)
			cout << "Build VRT..." << endl;

		string listFilePath = m_filePath;
		SetFileExtension(listFilePath, "_list.txt");
		ofStream file;
		msg = file.open(listFilePath);
		if (msg)
		{
			//size_t scenesSize = max(1, options.m_scenesSize);


			ASSERT(m_internalName.size() == GetRasterCount());
			//cout << "Build VRT nb scenes = " << GetNbScenes() << endl;
			//cout << "Build VRT scenes size = " << GetSceneSize() << endl;

			for (size_t i = 0; i < GetNbScenes(); i++)//for all segment
			{
				bool bUseIt = true;
				if (options.m_bRemoveEmptyBand)
				{
					for (size_t bb = 0; bb < GetSceneSize() && bUseIt; bb++)//for all segment
					{
						double min = 0;
						double max = 0;
						double mean = 0;
						double stddev = 0;

						size_t b = i * GetSceneSize() + bb;
						GDALRasterBand * pBand = GetRasterBand(b);
						if (pBand->GetStatistics(true, true, &min, &max, &mean, &stddev) != CE_None)
							bUseIt = false;
					}
				}

				for (size_t bb = 0; bb < GetSceneSize(); bb++)//for all segment
				{
					size_t b = i * GetSceneSize() + bb;

					if (bUseIt)
						file << m_internalName[b] << endl;
				}
			}

			//else
			//{
			//	ASSERT(m_internalName.size() == GetRasterCount());
			//	for (size_t b = 0; b < m_internalName.size(); b++)//for all segment
			//	{
			//		bool bUseIt = true;
			//		if (options.bRemoveEmptyBand)
			//		{
			//			double min = 0;
			//			double max = 0;
			//			double mean = 0;
			//			double stddev = 0;

			//			GDALRasterBand * pBand = GetRasterBand(b);
			//			if (pBand->GetStatistics(true, true, &min, &max, &mean, &stddev) != CE_None)
			//				bUseIt = false;
			//		}

			//		if (bUseIt)
			//			file << m_internalName[b] << endl;
			//	}
			//}
			file.close();
		}

		string command = "GDALBuildVRT.exe -separate -overwrite -input_file_list \"" + listFilePath + "\" \"" + m_filePath + "\"" + (options.m_bQuiet ? " -q" : "");
		msg = WinExecWait(command);

		return msg;
	}

	void CGDALDatasetEx::GetBandsHolder(CBandsHolder& bandsHolder)const
	{
		//reset 

		//add bands
		for (size_t j = 0; j < GetRasterCount(); j++)
		{
			CSingleBandHolderPtr ptr = GetSingleBandHolder(j);
			bandsHolder.AddBand(ptr);
		}

		bandsHolder.SetExtents(GetExtents());
		bandsHolder.SetScenePeriod(m_scenesPeriod);

		if (!m_bVRT)
			bandsHolder.SetIOCPU(1);

	}

	//string CGDALDatasetEx::GetBandName(int bandNo)const
	//{
	//	ASSERT(bandNo >= 1 && bandNo <= GetRasterCount());
	//	string name = GetFileTitle(m_filePath) + ":" + m_bandMetaData[bandNo-1].c_str();
	//
	//	return name;
	//}
	//std::string CGDALDatasetEx::GetBandMetaData(size_t b, const std::string& type)
	//{
	//	ASSERT(band >= 1 && band <= GetRasterCount());
	//	if (m_bVRT)
	//	{
	//		ASSERT(m_internalName.size() == GetRasterBand());
	//	}
	//}

	double CGDALDatasetEx::ReadPixel(size_t i, int x, int y)const
	{
		ASSERT(i < GetRasterCount());
		ASSERT(x >= 0 && x < Dataset()->GetRasterXSize());
		ASSERT(y >= 0 && y < Dataset()->GetRasterYSize());
		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);

		double v = 0;
		GDALRasterBand *pBand = me.GetRasterBand(i);
		pBand->RasterIO(GF_Read, x, y, 1, 1, &v, 1, 1, GDT_Float64, 0, 0);

		return v;
	}

	CSingleBandHolderPtr CGDALDatasetEx::GetSingleBandHolder(size_t i)const
	{
		ASSERT(i < GetRasterCount());

		CSingleBandHolderPtr pBandHolder;
		if (m_virtualBands[i].empty())
		{
			pBandHolder = CSingleBandHolderPtr(new CSingleBandHolder(m_poDataset, i, m_internalName[i]));
		}
		else
		{
			pBandHolder = CSingleBandHolderPtr(new CVirtualBandHolder(m_virtualBands[i]));
		}

		if (i < m_internalExtents.size())
			pBandHolder->SetInternalMapExtents(m_internalExtents[i]);

		return pBandHolder;
	}

	void CGDALDatasetEx::UpdateOption(CBaseOptions& options)const
	{
		ASSERT(GetRasterCount() > 0);

		if (options.m_format.empty())
		{
			GDALDriver* poDriverOut = m_poDataset->GetDriver();
			options.m_format = poDriverOut->GetDescription();
		}
		
		

		//short 
		if (options.m_outputType == GDT_Unknown)
			options.m_outputType = const_cast<GDALRasterBand *>(GetRasterBand(0))->GetRasterDataType();

		if (options.m_nbBands == UNKNOWN_POS)
			options.m_nbBands = GetRasterCount();

		if (options.m_prj.empty())
			options.m_prj = m_poDataset->GetProjectionRef();

		//CGeoExtents inputExtents = GetExtents();

		//step1 : initialization of the extent rect
		if (!((CGeoRect&)options.m_extents).IsInit())
		{
			((CGeoRect&)options.m_extents) = m_extents;//set entire extent
		}
		else
		{
			options.m_extents.SetPrjID(GetPrj()->GetPrjID());//set only projection of input map
		}

		//step2 : initialization of the extent size
		if (options.m_extents.m_xSize == 0)
		{
			double xRes = m_extents.XRes();
			if (options.m_bRes)
				xRes = options.m_xRes;

			options.m_extents.m_xSize = TrunkLowest(abs(options.m_extents.Width() / xRes));
			options.m_extents.m_xMax = options.m_extents.m_xMin + options.m_extents.m_xSize * abs(xRes);

		}


		if (options.m_extents.m_ySize == 0)
		{
			double yRes = m_extents.YRes();
			if (options.m_bRes)
				yRes = options.m_yRes;


			options.m_extents.m_ySize = TrunkLowest(abs(options.m_extents.Height() / yRes));
			options.m_extents.m_yMin = options.m_extents.m_yMax - options.m_extents.m_ySize * abs(yRes);
		}

		//step3: initialization of block size
		if (options.m_extents.m_xBlockSize == 0)
			options.m_extents.m_xBlockSize = m_extents.m_xBlockSize;

		if (options.m_extents.m_yBlockSize == 0)
		{
			options.m_extents.m_yBlockSize = m_extents.m_yBlockSize;
			if (options.m_extents.m_yBlockSize > 1)
			{
				if( options.m_createOptions.Find("TILED", false, false) ==-1)
					options.m_createOptions.push_back("TILED=YES");
				if (options.m_createOptions.Find("BLOCKXSIZE", false, false) == -1)
					options.m_createOptions.push_back("BLOCKXSIZE=" + to_string(options.m_extents.m_xBlockSize));
				if (options.m_createOptions.Find("BLOCKYSIZE", false, false) == -1)
					options.m_createOptions.push_back("BLOCKYSIZE=" + to_string(options.m_extents.m_yBlockSize));
			}
		}

		if (!options.m_period.IsInit())
			options.m_period = GetPeriod();

		//if memoryLimit is set, override block size
		if (options.m_memoryLimit > 0)
		{
			CGeoSize size = ComputeBlockSize(options.m_memoryLimit, options.m_extents, options.m_period, options.m_TM);
			options.m_extents.m_xBlockSize = size.m_x;
			options.m_extents.m_yBlockSize = size.m_y;
		}

		//Limit block size to the size of the image
		options.m_extents.m_xBlockSize = min(options.m_extents.m_xBlockSize, m_poDataset->GetRasterXSize());
		options.m_extents.m_yBlockSize = min(options.m_extents.m_yBlockSize, m_poDataset->GetRasterYSize());

		//get the nearest grid cell
		if (options.m_bTap)
			options.m_extents.AlignTo(m_extents);


		if (options.m_dstNodata == MISSING_NO_DATA)
		{
			//if there is no output nodata and there is no image input nodata then
			//All types excep Byte will used the minimum value of the type
			if (options.m_bUseDefaultNoData)
				options.m_dstNodata = WBSF::GetDefaultNoData(options.m_outputType);
		}

		if (options.m_dstNodataEx == MISSING_NO_DATA)
		{
			//take dst no data by default
			options.m_dstNodataEx = options.m_dstNodata;
		}


		//Add same compression if not specified
		//if (options.m_createOptions.Find("COMPRESS", false, false) == -1)
		//{
		//	char** test = const_cast<GDALRasterBand*>(GetRasterBand(0))->GetMetadata("IMAGE_STRUCTURE");
		//	const char* pVal = const_cast<GDALRasterBand*>(GetRasterBand(0))->GetMetadataItem("IMAGE_STRUCTURE");
		//	//const char* pVal = m_poDataset->GetMetadataItem("COMPRESSION");
		//	
		//	if(pVal!=NULL)
		//		options.m_createOptions.push_back("COMPRESS="+ string(pVal));
		//}
	}



	void CGDALDatasetEx::BuildOverviews(const vector<int>& list, bool bQuiet)
	{
		if (IsOpen())
		{ 
			//pszResampling 	one of "NEAREST", "GAUSS", "CUBIC", "AVERAGE", "MODE", "AVERAGE_MAGPHASE" or "NONE" controlling the downsampling method applied.
			//nOverviews 	number of overviews to build.
			//panOverviewList 	the list of overview decimation factors to build.
			//nListBands 	number of bands to build overviews for in panBandList. Build for all bands if this is 0.
			//panBandList 	list of band numbers.
			//pfnProgress 	a function to call to report progress, or NULL.
			//pProgressData 	application data to pass to the progress function.
			//Returns:
			//CE_None on success or CE_Failure if the operation doesn't work.
			//For example, to build overview level 2, 4 and 8 on all bands the following call could be made:

			//   int       anOverviewList[3] = { 2, 4, 8 };
			if (!list.empty())
			{
				if (m_bMultipleImages)
				{
					if (!bQuiet)
						cout << "Build Overview..." << endl;

					for (size_t i = 0; i < m_poDatasetVector.size(); i++)
					{
						if (m_poDatasetVector[i])
						{
							if (!bQuiet)
								cout << FormatA("%-4.4s", FormatA("B%d", i + 1).c_str()) << ": ";

							m_poDatasetVector[i]->BuildOverviews("NEAREST", (int)list.size(), const_cast<int *>(list.data()), 0, NULL, bQuiet ? GDALDummyProgress : GDALTermProgress, NULL);
						}
					}

				}
				else
				{
					if (!bQuiet)
						cout << "Build Overview: ";

					Dataset()->BuildOverviews("NEAREST", (int)list.size(), const_cast<int *>(list.data()), 0, NULL, bQuiet ? GDALDummyProgress : GDALTermProgress, NULL);
				}
			}
		}
	}

	void CGDALDatasetEx::ComputeStats(bool bQuiet)
	{
		if (IsOpen())
		{
			if (!bQuiet)
				cout << "Compute stats..." << endl;


			double dfMin, dfMax, dfMean, dfStdDev;
			for (size_t i = 0; i < GetRasterCount(); i++)
			{
				if (!bQuiet)
					cout << FormatA("%-4.4s", FormatA("B%d", i + 1).c_str()) << ": ";

				if (GetRasterBand(i))
					GetRasterBand(i)->ComputeStatistics(false, &dfMin, &dfMax, &dfMean, &dfStdDev, (bQuiet) ? GDALDummyProgress : GDALTermProgress, NULL);
			}

		}
	}

	void CGDALDatasetEx::ComputeHistogram(bool bQuiet)
	{
		if (IsOpen())
		{
			if (!bQuiet)
				cout << "Compute histogram..." << endl;


			for (size_t i = 0; i < GetRasterCount(); i++)
			{
				if (!bQuiet)
					cout << FormatA("%-4.4s", FormatA("B%d", i + 1).c_str()) << ": ";

				if (GetRasterBand(i))
				{
					int nBucketCount = 0;
					GUIntBig *panHistogram = NULL;
					double dfMin = 0, dfMax = 0;
					GetRasterBand(i)->GetDefaultHistogram(&dfMin, &dfMax, &nBucketCount, &panHistogram, TRUE, (bQuiet) ? GDALDummyProgress : GDALTermProgress, NULL);
					CPLFree(panHistogram);
				}
			}
		}

	}


	void CGDALDatasetEx::GetBandsMetaData(BandsMetaData& meta_data)const
	{
		meta_data.resize(GetRasterCount());
		/* ==================================================================== */
		/*      Loop over bands.                                                */
		/* ==================================================================== */
		for (size_t b = 0; b < GetRasterCount(); b++)
		{
			const GDALRasterBand* pBand = GetRasterBand(b); 
			

			if (pBand->GetDescription() != NULL
				&& strlen(pBand->GetDescription()) > 0)
				meta_data[b]["description"] = pBand->GetDescription();

			char **papszMetadata = const_cast<GDALRasterBand*>(pBand)->GetMetadata();
			if (papszMetadata != NULL && *papszMetadata != NULL)
			{
				for (int i = 0; papszMetadata[i] != NULL; i++)
				{
					char *pszKey = NULL;
					const char *pszValue = CPLParseNameValue(papszMetadata[i], &pszKey);
					if (pszKey)
					{
						meta_data[b][pszKey] = pszValue;
						CPLFree(pszKey);
					}
				}
			}

		}

	}

	std::string CGDALDatasetEx::GetCommonBandName()
	{
		size_t common_begin = MAX_PATH;//common begin
		for (size_t i = 0; i < GetRasterCount() - 1; i++)
		{
			for (size_t j = i; j < GetRasterCount(); j++)
			{
				string title0 = GetFileTitle(GetInternalName(i));
				string title1 = GetFileTitle(GetInternalName(j));
				size_t k = 0;//common begin
				while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
					k++;

				common_begin = min(common_begin, k);
			}
		}

		string common;
		if (common_begin != MAX_PATH)
			common = GetFileTitle(GetInternalName(0)).substr(common_begin);

		return common;
	}

	void CGDALDatasetEx::GetGeoTransform(CGeoTransform GT)const
	{
		(m_bMultipleImages ? m_poDatasetVector[0] : m_poDataset)->GetGeoTransform(GT);
	}


	//****************************************************************************
	//  section pour Créer des LOCs à partir de DEM.

	//****************************************************************************
	// Sommaire:    Génère un LOC à partir du DEM.
	//
	// Description: GetRegularCoord crée un LOC sur une grille régulière. 
	//
	// Entrée:      int nbPointX: le nombre de point en X.
	//              int nbPointY: le nombre de point en Y.
	//              bool bExp:  true: donne l'exposition
	//                          false: met 0.
	//              
	//
	// Sortie:      int: MAP_SUCCESS ou code d'erreur(voir mapping.h)
	//              CLocArray& locArray: un LOC
	//
	// Note:        Les points où il n'y a pas de données sont enlevés. Le nombres
	//              de stations dans un LOC peut être inférieur à nbPointX*nbPointY.
	//****************************************************************************
	class CExtractInfo
	{
	public:

		enum TType { EXTREM, DENSITY, NB_TYPE };

		class CInfo
		{
		public:

			double operator[](int i)const { return m_stat[i]; }

			CGridPoint m_lowest;
			CGridPoint m_highest;
			CStatistic m_stat;
		};

		typedef CMatrix<CInfo> CInfoMatrix;

		int m_nbPoint;
		double m_factor;
		bool m_bExposition;
		CGeoExtents m_extents;


		CExtractInfo();
		ERMsg Execute(CGDALDatasetEx& map, CCallback& callback = DEFAULT_CALLBACK);

		int GetNbExtremPoint()const { return GetNbBox(EXTREM) * 2; }
		int GetNbBox()const { return GetNbBox(DENSITY); }
		void Add(const CGridPoint& pt);

		void GetExtremPoints(CGridPointVector& pts)const;

		double operator[](int i)const { return m_stat[i]; }
		double GetPBase()const;
		double GetP(const CGeoPoint& ptIn);

	protected:

		int GetNbBox(int t)const;
		CGeoPointIndex GetPosIndex(const CGeoPoint& ptIn, int t);

		int m_nbSub[NB_TYPE];
		CInfoMatrix m_info[NB_TYPE];
		int m_nbBox;

		CStatistic m_stat;//statistic for the entire map.
		double m_sumStdDev;
	};

	ERMsg CGDALDatasetEx::GetRegularCoord(int nbPointLat, int nbPointLon, bool bExpo, const CGeoRect& rect, CGridPointVector& points, CCallback& callback)
	{
		ASSERT(IsOpen());
		ASSERT(nbPointLat > 0 && nbPointLon > 0);

		ERMsg msg;

		points.clear();

		CGeoExtents extents = GetExtents();
		CGeoExtents sub_extents = extents;
		if (rect.IsInit())
		{
			((CGeoRect&)sub_extents) = rect;
			//if (GetPrjID() != rect.GetPrjID())
				//sub_extents.Reproject(GetReProjection(rect.GetPrjID(), GetPrjID()));

			sub_extents.IntersectExtents(extents);
		}


		double x_inc = sub_extents.m_xSize / nbPointLon;
		double y_inc = sub_extents.m_ySize / nbPointLat;

		std::set<pair<int, int>> indexes;

		for (size_t i = 0; i < nbPointLat; i++)
		{
			for (size_t j = 0; j < nbPointLon; j++)
			{
				CGeoPointIndex xy(int(x_inc*j), int(y_inc*i));
				CGeoPoint pt = sub_extents.XYPosToCoord(xy);
				xy = extents.CoordToXYPos(pt);
				indexes.insert(make_pair(xy.m_x, xy.m_y));
			}
		}

		CProjectionPtr pPrj = GetPrj();
		CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));

		Randomize();//init the first time

		CBandsHolderMT bandHolder(3);
		msg += bandHolder.Load(*this);

		if (!msg)
			return msg;

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		string comment = FormatMsg(IDS_MAP_GENERATEREGULAR, m_filePath);
		callback.PushTask(comment, XYindex.size());



		for (size_t xy = 0; xy < XYindex.size() && msg; xy++)//for all blocks
		{
			int xBlock = XYindex[xy].first;
			int yBlock = XYindex[xy].second;
			CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
			bandHolder.LoadBlock(blockExtents);

			CDataWindowPtr window = bandHolder.GetWindow(0);

			//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
			for (int y = 0; y < blockExtents.m_ySize&&msg; y++)
			{
				for (int x = 0; x < blockExtents.m_xSize&&msg; x++)
				{
					if (window->IsValid(x, y))
					{
						CGeoPointIndex xy(x, y);
						CGeoPoint pt = blockExtents.XYPosToCoord(xy);
						xy = extents.CoordToXYPos(pt);

						auto it = indexes.find(make_pair(xy.m_x, xy.m_y));

						if (it != indexes.end())
						{
							CGridPoint pt;
							((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
							pt.m_z = window->at(x, y);
							if (bExpo)
								window->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);

							if (pPrj->IsProjected())
							{
								CGeoPoint ptGeo(pt);
								ptGeo.Reproject(PT);
								pt.m_latitudeDeg = ptGeo.m_y;
							}

							points.push_back(pt);
							msg += callback.StepIt(0);
						}

					}
				}
			}

			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	//****************************************************************************
	// Sommaire:    Génère un LOC à partir du DEM.
	//
	// Description: GetRandomCoord crée un LOC de points aléatoires.
	//
	// Entrée:      int nbPoint: le nombres de stations que l'on veut dans le LOC
	//              bool bExp:  true: donne l'exposition
	//                          false: met 0.
	//              
	//
	// Sortie:      int: MAP_SUCCESS ou code d'erreur(voir mapping.h)
	//              LOCArray& locArray: un LOC
	//
	// Note:        Le nombre de station dans le LOC == nbPoint. 
	//****************************************************************************



	ERMsg CGDALDatasetEx::GetRandomCoord(int nbPoint, bool bExpo, bool bExtrem, double factor, const CGeoRect& rect, CGridPointVector& locations, CCallback& callback)
	{
		ASSERT(IsOpen());
		ASSERT(nbPoint > 0);

		ERMsg msg;

		CGridPointVector locArrayTmp;
		locArrayTmp.reserve(nbPoint);
		locations.reserve(nbPoint);
		CGeoExtents extents = GetExtents();
		if (rect.IsInit())
		{
			((CGeoRect&)extents) = rect;
			if (GetPrjID() != rect.GetPrjID())
				extents.Reproject(GetReProjection(rect.GetPrjID(), GetPrjID()));
		}


		size_t nbPixels = extents.GetNbPixels();
		double Pbase = (double)nbPoint / nbPixels;

		CExtractInfo info;
		info.m_nbPoint = nbPoint;
		info.m_factor = factor;
		info.m_bExposition = bExpo;
		info.m_extents = extents;

		if (bExtrem || factor > 0)
		{
			msg = info.Execute(*this, callback);
			if (msg && info[NB_VALUE] > 0)
			{
				callback.AddMessage("NbCells = " + ToString(info[NB_VALUE]));

				if (bExtrem)
					callback.AddMessage("NbExtremPoint = " + ToString(info.GetNbExtremPoint()));
				callback.AddMessage("NbBox = " + ToString(info.GetNbBox()));

				Pbase = info.GetPBase();
			}
		}

		if (!msg)
			return msg;

		int nbRun = 0;
		CProjectionPtr pPrj = GetPrj();
		CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));

		const auto& start = std::chrono::high_resolution_clock::now();
		//	high_resolution_clock::now();

		Randomize();//init the first time
		// loop until the good number of points
		while ((locations.size() + locArrayTmp.size() + info.GetNbExtremPoint()) < nbPoint && msg)
		{
			//m_extents = GetExtents();
			string comment = FormatMsg(IDS_MAP_GENERATERANDOM, m_filePath);
			callback.PushTask(comment, nbPixels);

			CBandsHolderMT bandHolder(3);
			msg += bandHolder.Load(*this);

			if (!msg)
				return msg;

			vector<pair<int, int>> XYindex = extents.GetBlockList();

			for (int xy = 0; xy < (int)XYindex.size() && msg; xy++)//for all blocks
			{
				int xBlock = XYindex[xy].first;
				int yBlock = XYindex[xy].second;
				CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
				bandHolder.LoadBlock(blockExtents);

				CDataWindowPtr window = bandHolder.GetWindow(0);

				//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
				for (int y = 0; y < blockExtents.m_ySize; y++)
				{
					for (int x = 0; x < blockExtents.m_xSize; x++)
					{
						if (window->IsValid(x, y))
						{
							CGridPoint pt;
							((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
							pt.m_z = window->at(x, y);
							if (bExpo)
								window->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);

							//randomize point coordinate in the cell for kriging
							pt.m_x += Rand(-0.5, 0.5)*m_extents.XRes();
							pt.m_y += Rand(-0.5, 0.5)*m_extents.YRes();

							double p = info.GetP(pt)*Pbase;
							if (Randu() < p)
							{
								if (pPrj->IsProjected())
								{
									CGeoPoint ptGeo(pt);
									ptGeo.Reproject(PT);
									pt.m_latitudeDeg = ptGeo.m_y;
								}

								locations.push_back(pt);
							}
							else if (locations.size() < nbPoint && Randu() < 2 * p)//all can have a second chance
							{
								if (pPrj->IsProjected())
								{
									CGeoPoint ptGeo(pt);
									ptGeo.Reproject(PT);
									pt.m_latitudeDeg = ptGeo.m_y;
								}

								locArrayTmp.push_back(pt);
							}
						}

						msg += callback.StepIt();
					}
				}
			}
			 
			const auto& now = std::chrono::high_resolution_clock::now();
			//const auto& stop = high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();


			//if (nbRun++ > 10)
			if(duration>600)
			{   // si le nombre de tour est > 10, on à un problème
				//unable to found points
				msg.ajoute(GetString(IDS_MAP_UNABLE_CREATE_LOC));
				break;
			}

			callback.AddMessage("NbPoint = " + ToString(locations.size() + locArrayTmp.size()));
			callback.PopTask();


		}//while

		if (msg)
		{
			//complete locArrayTmp1
			size_t nbPointTmp = locations.size() + info.GetNbExtremPoint();
			if (nbPointTmp < nbPoint)
			{
				size_t needed = nbPoint - nbPointTmp;
				for (size_t i = 0; i < needed; i++)
				{
					size_t index = (size_t)Rand(0, int(locArrayTmp.size()) - 1);
					ASSERT(index < locArrayTmp.size());
					locations.push_back(locArrayTmp[index]);
					locArrayTmp.erase(locArrayTmp.begin() + index);
				}
			}
			else
			{
				size_t extra = nbPointTmp - nbPoint;
				for (size_t i = 0; i < extra; i++)
				{
					size_t index = (size_t)Rand(0, int(locations.size()) - 1);
					ASSERT(index < locations.size());
					locations.erase(locations.begin() + index);
				}
			}

			info.GetExtremPoints(locations);
			ASSERT(locations.size() == nbPoint);
		}

		//classify.ClassifyNaturalBreak(5);

		//CStdioFile file( "c:\\temp\\testLoc.csv", CFile::modeCreate|CFile::modeWrite);
		//for(int i=0; i<classify.GetSize(); i++)
		//{
		//	string line;
		//	line.Format( "%lf,%lf,%lf\n", classify[i].m_x[LOWEST], classify[i].m_x[HIGHEST], classify[i].m_x[MEAN]);
		//	file.WriteString(line);
		//}
		//file.Close();


		return msg;
	}


	CGeoSize CGDALDatasetEx::ComputeBlockSize(double memoryLimit, const CGeoExtents& extents, CTPeriod period, CTM TM)const
	{
		//1- Get block size 
		CGeoSize meanSize;
		int nbRaster = 0;

		bool bTemporal = !m_scenesPeriod.empty() && period.IsInit();

		for (size_t i = 0; i < GetRasterCount(); i++)
		{
			if (extents.IsRectIntersect(m_internalExtents[i]))
			{
				size_t sceneNo = bTemporal ? i / GetSceneSize() : UNKNOWN_POS;
				if (!bTemporal || (m_scenesPeriod[sceneNo].IsInit() && period.IsIntersect(m_scenesPeriod[sceneNo])))
				{
					CGeoSize size = m_internalExtents[i].GetSize();
					meanSize += size;
					nbRaster++;
				}
			}
		}

		meanSize /= nbRaster;

		//2- Get First estimate of number of block
		CGeoSize size = extents.GetSize();
		CGeoExtents blockManager(extents, size, meanSize);


		//3- Get max layer per block
		int nbLayerMax = max(1, GetMaxBlockSizeZ(blockManager, period, TM));

		//4- Evaluate block size int respect of memroy cache
		double rxy = (double)blockManager.m_xBlockSize / blockManager.m_yBlockSize;
		double yBlockSize = sqrt(memoryLimit / (sizeof(DataType)*rxy*nbLayerMax));
		double xBlockSize = rxy * yBlockSize;
		CGeoSize blockSize(max(1, min(extents.m_xSize, (int)ceil(xBlockSize))), max(1, min(extents.m_ySize, (int)ceil(yBlockSize))));

		double memMax = (double)blockSize.m_x*blockSize.m_y*nbLayerMax * sizeof(DataType);

		//m_extents.SetBlockSize(blockSize);
		return blockSize;
	}


	int CGDALDatasetEx::GetMaxBlockSizeZ(CGeoExtents extents, CTPeriod period, CTM TM)const
	{
		bool bTemporal = !m_scenesPeriod.empty() && period.IsInit();
		CTTransformation TT(period, TM);

		size_t nbScene = bTemporal ? GetNbScenes() : GetRasterCount();
		size_t sceneSize = bTemporal ? GetSceneSize() : 1;
		size_t nbCluster = bTemporal ? TT.GetNbCluster() : 1;
		CStatisticVector nbRaster(nbCluster);

		for (size_t k = 0; k < nbScene; k++)
		{
			if (extents.IsRectIntersect(m_internalExtents[k]))
			{
				for (size_t i = 0; i < nbCluster; i++)
				{
					CTPeriod p = bTemporal ? m_scenesPeriod[k] : CTPeriod();
					p.Transform(TM);
					if (!bTemporal || p.IsInside(TT.GetClusterTRef(i)))
					{
						nbRaster[i] += sceneSize;
					}
				}
			}
		}

		CStatistic nbRasterAll;
		for (int l = 0; l < nbCluster; l++)
			if (nbRaster[l][NB_VALUE] > 0)
				nbRasterAll += nbRaster[l][SUM];

		int nbRasterMax = nbRasterAll.IsInit() ? (int)nbRasterAll[HIGHEST] : 0;

		return nbRasterMax;
	}

	void CGDALDatasetEx::FlushCache(double yMax)
	{
		for (size_t k = 0; k < GetRasterCount(); k++)
		{
			if (k >= m_internalExtents.size() || yMax < m_internalExtents[k].m_yMin)
				GetRasterBand(k)->FlushCache();
		}
	}
	//*****************************************************************************************
	//CSingleBandHolder
	/*CPLErr GetHistogram( GDALRasterBand *poBand, float *panHistogram )
	 {
		 int        nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
		 int        iXBlock, iYBlock;
		 GByte      *pabyData=NULL;
		 memset( panHistogram, 0, sizeof(float) * 256 );

		 CPLAssert( poBand->GetRasterDataType() == GDT_Byte );
		 poBand->GetBlockSize( &nXBlockSize, &nYBlockSize );
		 nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
		 nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;
		 pabyData = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize);
		 for( iYBlock = 0; iYBlock < nYBlocks; iYBlock++ )
		 {
			 for( iXBlock = 0; iXBlock < nXBlocks; iXBlock++ )
			 {
				 int        nXValid, nYValid;
				 poBand->ReadBlock( iXBlock, iYBlock, pabyData );
				 // Compute the portion of the block that is valid
				 // for partial edge blocks.
				 if( (iXBlock+1) * nXBlockSize > poBand->GetXSize() )
					 nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
				 else
					 nXValid = nXBlockSize;

				 if( (iYBlock+1) * nYBlockSize > poBand->GetYSize() )
					 nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
				 else
					 nYValid = nYBlockSize;
				 // Collect the histogram counts.
				 for( int iY = 0; iY < nYValid; iY++ )
				 {
					 for( int iX = 0; iX < nXValid; iX++ )
					 {
						 panHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
					 }
				 }
			 }
		 }
	 }
	 */

	CSingleBandHolder::CSingleBandHolder(GDALDataset* pDataset, size_t i, string bandName)
	{
		//ASSERT( pDataset);
		m_pParent = NULL;
		m_bandName = bandName;
		m_pDataset = pDataset;
		m_bandNo = i;
		m_bDontLoadContantBand = false;
		m_bConstantBand = false;
		m_captor = Landsat1::GetCaptorFromName(GetFileTitle(bandName));

		if (pDataset)
		{
			m_datasetExtent = WBSF::GetExtents(m_pDataset);
			m_noData = WBSF::GetNoData(pDataset->GetRasterBand(int(i) + 1));

			CProjectionPtr pPrj = CProjectionManager::GetPrj(pDataset->GetProjectionRef());

			//double adfGeoTransform[6] = {0};
			//pDataset->GetGeoTransform(adfGeoTransform);

			//m_nsres = adfGeoTransform[5];
			//m_ewres = adfGeoTransform[1];
			//m_bProjected = pPrj->IsProjected();//on pourrai mettre le ID à la place
		}

		m_maskDataUsed = DataTypeMin;
		m_bExclude = false;

	}

	CSingleBandHolder::CSingleBandHolder(const CSingleBandHolder& in)
	{
		m_pParent = in.m_pParent;
		m_bandName = in.m_bandName;
		m_pDataset = in.m_pDataset;
		m_bandNo = in.m_bandNo;
		m_bDontLoadContantBand = in.m_bDontLoadContantBand;
		m_bConstantBand = in.m_bConstantBand;

		if (m_pDataset)
		{
			m_datasetExtent = in.m_datasetExtent;
			m_noData = in.m_noData;
			m_maskDataUsed = in.m_maskDataUsed;
			m_internalMapExtents = in.m_internalMapExtents;
			m_dataRect = in.m_dataRect;
			m_extents = in.m_extents;
			//m_nsres=in.m_nsres;
			//m_ewres=in.m_ewres;
			//m_bProjected=in.m_bProjected;
			m_captor = in.m_captor;
		}


		m_bExclude = false;
		m_data.clear();
	}

	CSingleBandHolder::~CSingleBandHolder()
	{
		FlushCache(-9999999999);
	}

	void CSingleBandHolder::FlushCache(double yMax)
	{
		if (yMax < m_extents.m_yMin)
		{
			DataVector vi;

			// clean it up in C++03
			// no need to clear() first
			m_data.swap(vi);

			// clean it up in C++0x
			// not a one liner, but much more idiomatic
			vi.clear();
			vi.shrink_to_fit();

			m_dataRect.SetRectEmpty();
			m_data.clear();
		}

	}


	//windowSize devrait être en unité(m) et non en pixel
	void CSingleBandHolder::Init(int windowSize)
	{
		ASSERT(!m_extents.IsRectEmpty());

		CGeoExtents mapExtents = m_internalMapExtents;
		mapExtents.IntersectExtents(m_extents);

		int halfWindow = windowSize / 2;
		double xRes = fabs(mapExtents.XRes());
		double yRes = fabs(mapExtents.YRes());
		mapExtents.m_xMin -= halfWindow * xRes;
		mapExtents.m_xMax += halfWindow * xRes;
		mapExtents.m_yMin -= halfWindow * yRes;
		mapExtents.m_yMax += halfWindow * yRes;
		mapExtents.m_xSize += 2 * halfWindow;
		mapExtents.m_ySize += 2 * halfWindow;
		mapExtents.m_xBlockSize += 2 * halfWindow;
		mapExtents.m_yBlockSize += 2 * halfWindow;
		mapExtents.IntersectExtents(m_datasetExtent);

		if (!m_bExclude && !mapExtents.IsEmpty())
		{
			m_dataRect = m_extents.CoordToXYPos(mapExtents); //portion of valid map in extents coordinate
			CGeoRectIndex loadRect = m_datasetExtent.CoordToXYPos(mapExtents); //get map rect to load in map coordinate

			if (!m_dataRect.IsRectEmpty() && !loadRect.IsRectEmpty())
			{
				//ASSERT(loadRect.Width() <= m_dataRect.Width());
				//ASSERT(loadRect.Height() <= m_dataRect.Height());
				ASSERT(loadRect.m_x >= 0 && loadRect.m_x <= m_pDataset->GetRasterXSize());
				ASSERT(loadRect.m_y >= 0 && loadRect.m_y <= m_pDataset->GetRasterYSize());
				ASSERT(loadRect.m_xSize >= 0 && loadRect.m_xSize <= m_pDataset->GetRasterXSize());
				ASSERT(loadRect.m_ySize >= 0 && loadRect.m_ySize <= m_pDataset->GetRasterYSize());

				//m_data.resize(loadRect.Height()*loadRect.Width());
				//m_data.resize(max(loadRect.m_xSize*loadRect.m_ySize, m_dataRect.Height()*m_dataRect.Width()));
				m_data.resize(m_dataRect.Height()*m_dataRect.Width());
				ASSERT(m_data.size() == m_dataRect.Width()*m_dataRect.Height());

				if (loadRect.m_xSize > m_dataRect.Width())
					loadRect.m_xSize = m_dataRect.Width();

				if (loadRect.m_ySize > m_dataRect.Height())
					loadRect.m_ySize = m_dataRect.Height();

				ASSERT(loadRect.Width() <= m_dataRect.Width());
				ASSERT(loadRect.Height() <= m_dataRect.Height());

				GDALRasterBand* pBand = m_pDataset->GetRasterBand(int(m_bandNo) + 1);//1 base
				pBand->RasterIO(GF_Read, loadRect.m_x, loadRect.m_y, loadRect.m_xSize, loadRect.m_ySize, &(m_data[0]), m_dataRect.m_xSize, m_dataRect.m_ySize, GDT_Float32, NULL, NULL);
			}
			else
			{
				m_dataRect.SetRectEmpty();
				m_data.clear();
			}

			//	int        nXBlocks, nYBlocks, nXBlockSize, nYBlockSize;
			//	int        iXBlock, iYBlock;
			//	GByte      *pabyData;
			//	memset(panHistogram, 0, sizeof(int) * 256);
			//	CPLAssert(poBand->GetRasterDataType() == GDT_Byte);
			//	poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);
			//	nXBlocks = (poBand->GetXSize() + nXBlockSize - 1) / nXBlockSize;
			//	nYBlocks = (poBand->GetYSize() + nYBlockSize - 1) / nYBlockSize;
			//	pabyData = (GByte *)CPLMalloc(nXBlockSize * nYBlockSize);
			//	for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++)
			//	{
			//		for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++)
			//		{
			//			int        nXValid, nYValid;
			//			poBand->ReadBlock(iXBlock, iYBlock, pabyData);
			//			// Compute the portion of the block that is valid
			//			// for partial edge blocks.
			//			if ((iXBlock + 1) * nXBlockSize > poBand->GetXSize())
			//				nXValid = poBand->GetXSize() - iXBlock * nXBlockSize;
			//			else
			//				nXValid = nXBlockSize;
			//			if ((iYBlock + 1) * nYBlockSize > poBand->GetYSize())
			//				nYValid = poBand->GetYSize() - iYBlock * nYBlockSize;
			//			else
			//				nYValid = nYBlockSize;
			//			// Collect the histogram counts.
			//			for (int iY = 0; iY < nYValid; iY++)
			//			{
			//				for (int iX = 0; iX < nXValid; iX++)
			//				{
			//					panHistogram[pabyData[iX + iY * nXBlockSize]] += 1;
			//				}
			//			}
			//		}
			//	}
			//}

			//pBand->RasterIO(GF_Read, loadRect.m_x, loadRect.m_y, loadRect.m_xSize, loadRect.m_ySize, &(m_data[0]), loadRect.m_xSize, loadRect.m_ySize, GDT_Float32, NULL, NULL);
		}
		else
		{
			m_dataRect.SetRectEmpty();
			m_data.clear();
		}
	}


	//CGeoSize CSingleBandHolder::GetBlockSize ()const
	//{ 
	//	CGeoSize size(1024,1024);
	//	
	//	if( m_pDataset )
	//	{
	//		GDALRasterBand* pBand = m_pDataset->GetRasterBand(m_bandNo);
	//		pBand->GetBlockSize (&size.m_x, &size.m_x);
	//	}
	//
	//	return size;
	//}


	//*************************************************************************************************************
	void CBandsHolder::clear()
	{
		m_bandHolder.clear();
		m_pMaskBandHolder.reset();

		m_entireExtents.clear();

		m_nbScenes = 0;
		m_sceneSize = 0;
		m_entirePeriod.clear();//extraction period ??
		m_scenesPeriod.clear();//period of scenes
		//m_maxWindowSize;
		//m_IOCPU;
		//m_memoryLimit;
		//m_maskDataUsed;
		m_bEmpty = true;
	}

	ERMsg CBandsHolder::Load(const CGDALDatasetEx& inputDS, bool bQuiet, const CGeoExtents& entireExtents, CTPeriod entirePeriod)
	{
		ERMsg msg;

		if (!bQuiet)
			_tprintf(_T("Init band holder using %d IO threads ...\n"), m_IOCPU);


		inputDS.GetBandsHolder(*this);
		if (entireExtents.IsInit())
			m_entireExtents = entireExtents;

		if (entirePeriod.IsInit())
			m_entirePeriod = entirePeriod;

		m_nbScenes = inputDS.GetNbScenes();
		m_sceneSize = inputDS.GetSceneSize();

		for (int i = 0; i < (int)m_bandHolder.size(); i++)
			msg += m_bandHolder[i]->Load(this);

		if (!bQuiet)
		{
			cout << "Memory block: " << m_entireExtents.XNbBlocks() << " Blocks per line x " << m_entireExtents.YNbBlocks() << " Lines" << endl;
			cout << "Memory block size: " << GetBlockSizeX() << " Cols x " << GetBlockSizeY() << " Rows x " << GetBlockSizeZ() << " Bands (max)" << endl;
		}

		return msg;
	}


	void CBandsHolder::LoadBlock(CGeoExtents extents, CTPeriod p)
	{
		if (!p.IsInit())
			p = m_entirePeriod;


		if (m_pMaskBandHolder.get())
		{
			m_pMaskBandHolder->SetMaskDataUsed(m_maskDataUsed);
			m_pMaskBandHolder->SetExtents(extents);
			m_pMaskBandHolder->Init(m_maxWindowSize);
			if (m_pMaskBandHolder->GetDataRect().IsRectEmpty())//if one band of the scene is empty we skip this image
			{
				m_bEmpty = true;
				return;
			}
			else
			{
				const DataVector* pData = m_pMaskBandHolder->GetData();
				ASSERT(pData);

				bool bEmpty = true;
				for (size_t i = 0; i < pData->size() && bEmpty; i++)
				{
					if ((*pData).at(i) == m_maskDataUsed)
						bEmpty = false;
				}

				if (bEmpty)
				{
					m_bEmpty = true;
					return;
				}
			}

		}

		//	bool bTemporal = m_bandTRef.size()==m_bandHolder.size();
		bool bTemporal = !m_scenesPeriod.empty();

		double totalMem = 0;
		//m_xCurBlock=xBlock;
		//m_yCurBlock=yBlock;

		//load bands
		int nbScenes = int(bTemporal ? m_scenesPeriod.size() : size());
		int scenesSize = int(bTemporal ? size() / m_scenesPeriod.size() : 1);
#pragma omp parallel for schedule(static, 1)  num_threads( m_IOCPU ) if(m_IOCPU>1)

		for (int i = 0; i < nbScenes; i++)
		{
			for (int j = 0; j < scenesSize; j++)
			{
				m_bandHolder[i*scenesSize + j]->ExcludeBand(bTemporal&&m_scenesPeriod[i].IsInit() && !p.IsIntersect(m_scenesPeriod[i]));
				m_bandHolder[i*scenesSize + j]->SetExtents(extents);
				m_bandHolder[i*scenesSize + j]->Init(m_maxWindowSize);
				if (m_bandHolder[i*scenesSize + j]->GetDataRect().IsRectEmpty())//if one band of the scene is empty we skip this image
				{
					for (int jj = 0; jj < scenesSize; jj++)
					{
						m_bandHolder[i*scenesSize + jj]->ExcludeBand(true);
						m_bandHolder[i*scenesSize + jj]->SetExtents(extents);
						m_bandHolder[i*scenesSize + jj]->Init(m_maxWindowSize);
					}

					j = scenesSize;
				}

				totalMem += m_bandHolder[i]->GetData()->size() * sizeof(float);
			}
		}

		m_bEmpty = true;
		for (size_t i = 0; i < m_bandHolder.size() && m_bEmpty; i++)
		{
			CGeoRectIndex rect = m_bandHolder[i]->GetDataRect();
			m_bEmpty = rect.IsRectEmpty();
		}


	}

	void CBandsHolder::LoadBlock(CGeoExtents extents, boost::dynamic_bitset<size_t> selected_bands)
	{
		ASSERT(selected_bands.size() == size());

		if (m_pMaskBandHolder.get())
		{
			m_pMaskBandHolder->SetMaskDataUsed(m_maskDataUsed);
			m_pMaskBandHolder->SetExtents(extents);
			m_pMaskBandHolder->Init(m_maxWindowSize);

			if (m_pMaskBandHolder->GetDataRect().IsRectEmpty())//if one band of the scene is empty we skip this image
			{
				m_bEmpty = true;
				return;
			}
			else
			{
				const DataVector* pData = m_pMaskBandHolder->GetData();
				ASSERT(pData);

				bool bEmpty = true;
				for (size_t i = 0; i < pData->size() && bEmpty; i++)
				{
					if ((*pData).at(i) == m_maskDataUsed)
						bEmpty = false;
				}

				if (bEmpty)
				{
					m_bEmpty = true;
					return;
				}
			}
		}

		double totalMem = 0;

		//load bands

#pragma omp parallel for schedule(static, 1)  num_threads( m_IOCPU ) if(m_IOCPU>1)
		for (__int64 i = 0; i < (__int64)size(); i++)
		{
			//bool bSkipScene = false;
			m_bandHolder[i]->ExcludeBand(!selected_bands[i]);
			m_bandHolder[i]->SetExtents(extents);
			m_bandHolder[i]->Init(m_maxWindowSize);
			totalMem += m_bandHolder[i]->GetData()->size() * sizeof(float);
		}

		m_bEmpty = true;
		for (size_t i = 0; i < m_bandHolder.size() && m_bEmpty; i++)
		{
			CGeoRectIndex rect = m_bandHolder[i]->GetDataRect();
			m_bEmpty = rect.IsRectEmpty();
		}


	}
	//
	//CGeoSize CBandsHolder::ComputeBlockSize(const CGeoExtents& extents, CTPeriod period)const
	//{
	////1- Get block size 
	//	CGeoSize meanSize;
	//	
	//	bool bTemporal = !m_scenesPeriod.empty();
	//
	//	for(size_t i=0; i<m_bandHolder.size(); i++)
	//	{	
	//		if( extents.IsRectIntersect(m_bandHolder[i]->GetInternalMapExtents()) )
	//		{
	//			size_t sceneNo = bTemporal ? i / m_scenesPeriod.size() : UNKNOWN_POS;
	//			if (!bTemporal || period.IsIntersect(m_scenesPeriod[sceneNo]))
	//			{
	//				CGeoExtents extents = m_bandHolder[i]->GetInternalMapExtents();
	//				CGeoSize size = extents.GetSize();
	//				meanSize += size;
	//			}
	//		}
	//	}
	//
	//	meanSize/=(int)m_bandHolder.size();
	//
	////2- Get First estimate of number of block
	//	CGeoSize size = extents.GetSize();
	//	CGeoExtents blockManager(extents, size, meanSize );
	//
	//
	////3- Get max layer per block
	//	int nbLayerMax = Max(1, GetBlockSizeZ(-1, -1, period));
	//
	////4- Evaluate block size int respect of memroy cache
	//	double rxy = (double)blockManager.m_xBlockSize/blockManager.m_yBlockSize;
	//	double yBlockSize = sqrt(m_memoryLimit/(sizeof(DataType)*rxy*nbLayerMax ));
	//	double xBlockSize = rxy*yBlockSize;
	//	CGeoSize blockSize (Max(1, Min(extents.m_xSize,(int)ceil(xBlockSize))),Max(1, Min(extents.m_ySize, (int)ceil(yBlockSize))));
	//
	//	double memMax = (double)blockSize.m_x*blockSize.m_y*nbLayerMax*sizeof(DataType);
	//	
	//	//m_extents.SetBlockSize(blockSize);
	//	return blockSize;
	//}
	//

	int CBandsHolder::GetBlockSizeZ(int i, int j, CTPeriod p)const
	{
		CGeoExtents extents = m_entireExtents;
		if (i >= 0 && j >= 0)
			extents = m_entireExtents.GetBlockExtents(i, j);

		if (!p.IsInit())
			p = m_entirePeriod;

		//CTTransformation TT();

		bool bTemporal = !m_scenesPeriod.empty() && m_entirePeriod.IsInit();
		ASSERT(!bTemporal || p.IsInit());
		ASSERT(!bTemporal || p.GetNbSegments() == 1);//only continue period is accepted
		//size_t nbSegment = bTemporal ? p.GetNbSegments() : 1ull;
		//CStatisticVector nbRaster(nbSegment);
		int nbRasterMax = 0;
		for (size_t k = 0; k < (int)m_bandHolder.size(); k++)
		{
			ASSERT(!m_bandHolder[k]->GetInternalMapExtents().IsRectEmpty());

			if (extents.IsRectIntersect(m_bandHolder[k]->GetInternalMapExtents()))
			{
				size_t sceneNo = bTemporal ? k / (size() / m_scenesPeriod.size()) : UNKNOWN_POS;
				if (!bTemporal || (m_scenesPeriod[sceneNo].IsInit() && p.IsIntersect(m_scenesPeriod[sceneNo])))
				{
					//m_scenesPeriod[sceneNo].GetNbSegment(.GetNbSegement(p)
					//assume m_scenesPeriod[sceneNo] ans p is annual
					//int l = bTemporal ? p.GetSegmentIndex(m_scenesPeriod[sceneNo].Begin()) : 0;
					//if(l>=0 && l<nbRaster.size())
						//nbRaster[l] += 1;

					nbRasterMax++;
				}
			}
		}

		/*	CStatistic nbRasterAll;
			for (int l = 0; l < nbSegment; l++)
				if (nbRaster[l][NB_VALUE] > 0)
					nbRasterAll += nbRaster[l][SUM];
	*/
	//int nbRasterMax = (nbRasterAll[NB_VALUE] > 0) ? (int)nbRasterAll[HIGHEST] : 0;
		nbRasterMax += (m_pMaskBandHolder.get() != NULL) ? 1 : 0;


		return nbRasterMax;
	}

	void CBandsHolder::FlushCache(double yMax)
	{
		//release bands
#pragma omp parallel for schedule(static, 1)  num_threads( m_IOCPU ) if(m_IOCPU>1)
		for (int i = 0; i < (int)m_bandHolder.size(); i++)
		{
			m_bandHolder[i]->FlushCache(yMax);
		}

		if (m_pMaskBandHolder.get())
		{
			m_pMaskBandHolder->FlushCache(yMax);
		}

	}

	//get the real number of band for this bands holder 
	vector<bool> CBandsHolder::GetIntersectBands()const
	{
		//if( !p.IsInit() )
		vector<bool> bIntersectBand(m_bandHolder.size());

		CGeoExtents extents = m_entireExtents;
		bool bTemporal = !m_scenesPeriod.empty();
		ASSERT(!bTemporal || m_entirePeriod.IsInit());

		//int nbSegment = bTemporal?p.GetNbSegment():1;
		CStatistic nbRaster;
		for (size_t k = 0; k < m_bandHolder.size(); k++)
		{
			ASSERT(!m_bandHolder[k]->GetInternalMapExtents().IsRectEmpty());

			if (extents.IsRectIntersect(m_bandHolder[k]->GetInternalMapExtents()))
			{
				size_t sceneNo = bTemporal ? k / (size() / m_scenesPeriod.size()) : UNKNOWN_POS;
				if (!bTemporal || (m_scenesPeriod[sceneNo].IsInit() && m_entirePeriod.IsInside(m_scenesPeriod[sceneNo])))
				{
					bIntersectBand[k] = true;
				}
			}
		}

		return bIntersectBand;
	}
	//
	//bool iequals(const string& a, const string& b)
	//{
	//    size_t sz = a.size();
	//    if (b.size() != sz)
	//        return false;
	//    for (unsigned int i = 0; i < sz; ++i)
	//        if (tolower(a[i]) != tolower(b[i]))
	//            return false;
	//    return true;
	//}

	size_t CBandsHolder::GetBandByName(const string& name)const
	{

		size_t pos = NOT_INIT;

		if (!name.empty())
		{
			if (name[0] == 'b' || name[0] == 'B')
			{
				string no = name.substr(1);
				if (no.find_first_not_of("0123456789") == string::npos)
				{
					pos = stoi(no) - 1;
					if (pos > m_bandHolder.size())
						pos = NOT_INIT;
				}
			}
		}

		return pos;
	}

	CGeoExtents CBandsHolder::GetMinimumDataExtents(bool bQuiet)
	{
		CGeoExtents extents;

		CGeoPointIndex pt1;
		CGeoPointIndex pt2;

		if (GetXLimit(true, pt1.m_x, bQuiet))
			if (GetYLimit(true, pt1.m_y, bQuiet))
				if (GetXLimit(false, pt2.m_x, bQuiet))
					if (GetYLimit(false, pt2.m_y, bQuiet))
					{
						CGeoRectIndex rect(pt1, pt2);
						extents = CGeoExtents(m_entireExtents.XYPosToCoord(rect), rect.GetGeoSize(), m_entireExtents.GetBlockSize());
					}

		extents.NormalizeRect();
		return extents;
	}

	bool CBandsHolder::GetYLimit(bool bMin, int& value, bool bQuiet)
	{

		CStatistic stat;

		//CGeoExtents blocks = GetExtents();
		int xFirstBlock = 0;
		int xLastBlock = m_entireExtents.XNbBlocks();
		int yFirstBlock = bMin ? 0 : m_entireExtents.YNbBlocks() - 1;
		int yLastBlock = bMin ? m_entireExtents.YNbBlocks() : -1;
		int xBlockInc = xFirstBlock < xLastBlock ? 1 : -1;
		int yBlockInc = yFirstBlock < yLastBlock ? 1 : -1;

		for (int yBlock = yFirstBlock; yBlock != yLastBlock && stat[NB_VALUE] == 0; yBlock += yBlockInc)
		{
			for (int xBlock = xFirstBlock; xBlock != xLastBlock; xBlock += xBlockInc)
			{
				bool bFound = false;
				//CGeoSize blockSize = blocks.GetBlockSize(xBlock,yBlock);
				CGeoExtents blockExtents = m_entireExtents.GetBlockExtents(xBlock, yBlock);


#pragma omp critical(AccessBlock)
				{
					LoadBlock(blockExtents, m_entirePeriod);
				}

				if (!IsEmpty())
				{
					vector<CDataWindowPtr> input;// (GetRasterCount());
					GetWindow(input);
					//for(size_t i=0; i<input.size(); i++)
						//input[i]=GetWindow(int(i));

					CGeoSize blockSize = blockExtents.GetSize();

					int xFirst = 0;
					int xLast = blockSize.m_x;
					int yFirst = bMin ? 0 : blockSize.m_y - 1;
					int yLast = bMin ? blockSize.m_y : -1;
					int xInc = xFirst < xLast ? 1 : -1;
					int yInc = yFirst < yLast ? 1 : -1;

					for (int y = yFirst; y != yLast && !bFound; y += yInc)
					{
						for (int x = xFirst; x != xLast && !bFound; x += xInc)
						{
							for (size_t z = 0; z < input.size() && !bFound; z++)
							{
								if (input[z]->IsValid(x, y))
								{
									stat += m_entireExtents.GetBlockRect(xBlock, yBlock).m_y + y;
									bFound = true;
								}
							}//z
						} //x
					}//y
				}//empty block
			}//block x
		}//block y

		if (!bQuiet)
			for (int i = 0; i < 20; i++)
				cout << ".";

		value = (int)(bMin ? stat[LOWEST] : stat[HIGHEST]);

		return stat[NB_VALUE] > 0;
	}

	bool CBandsHolder::GetXLimit(bool bMin, int& value, bool bQuiet)
	{
		CStatistic stat;

		CGeoExtents blocks = GetExtents();
		int xFirstBlock = bMin ? 0 : blocks.XNbBlocks() - 1;
		int xLastBlock = bMin ? blocks.XNbBlocks() : -1;
		int yFirstBlock = 0;
		int yLastBlock = blocks.YNbBlocks();
		int xBlockInc = xFirstBlock < xLastBlock ? 1 : -1;
		int yBlockInc = yFirstBlock < yLastBlock ? 1 : -1;


		for (int xBlock = xFirstBlock; xBlock != xLastBlock && stat[NB_VALUE] == 0; xBlock += xBlockInc)
		{
			for (int yBlock = yFirstBlock; yBlock != yLastBlock; yBlock += yBlockInc)
			{
				bool bFound = false;
				CGeoExtents blockExtents = blocks.GetBlockExtents(xBlock, yBlock);
				CGeoSize blockSize = blocks.GetBlockSize(xBlock, yBlock);

#pragma omp critical(AccessBlock)
				{
					LoadBlock(blockExtents, m_entirePeriod);
				}

				if (!IsEmpty())
				{
					vector<CDataWindowPtr> input(GetRasterCount());
					for (size_t i = 0; i < input.size(); i++)
						input[i] = GetWindow(int(i));

					int xFirst = bMin ? 0 : blockSize.m_x - 1;
					int xLast = bMin ? blockSize.m_x : -1;
					int yFirst = 0;
					int yLast = blockSize.m_y;
					int xInc = xFirst < xLast ? 1 : -1;
					int yInc = yFirst < yLast ? 1 : -1;

					for (int x = xFirst; x != xLast && !bFound; x += xInc)
					{
						for (int y = yFirst; y != yLast && !bFound; y += yInc)
						{
							for (size_t z = 0; z < input.size() && !bFound; z++)
							{
								if (input[z]->IsValid(x, y))
								{
									stat += blocks.GetBlockRect(xBlock, yBlock).m_x + x;
									bFound = true;
								}
							}//z
						} //x
					}//y
				}//empty block
			}//block x
		}//block y

		if (!bQuiet)
			for (int i = 0; i < 20; i++)
				_tprintf(_T("."));

		value = (int)(bMin ? stat[LOWEST] : stat[HIGHEST]);

		return stat[NB_VALUE] > 0;
	}

	//************************************************************************

	CMTParserVariable::CMTParserVariable(std::string name, double noData)
	{
		m_name = name;
		m_value = 0;
		m_noData = noData;
	}

	CMTParserVariable::CMTParserVariable(const CMTParserVariable& in)
	{
		operator = (in);
	}

	CMTParserVariable& CMTParserVariable::operator = (const CMTParserVariable& in)
	{
		if (&in != this)
		{
			m_name = in.m_name;
			m_value = in.m_value;
			m_noData = in.m_noData;
		}

		return *this;
	}

	bool CMTParserVariable::operator == (const CMTParserVariable& in)const
	{
		return m_name == in.m_name;
	}

	bool CMTParserVariable::operator != (const CMTParserVariable& in)const
	{
		return !operator==(in);
	}

	//************************************************************************
	//CVirtualBandHolder

	CVirtualBandHolder::CVirtualBandHolder(const std::string& formula)
	{
		m_formula = formula;
		m_pParser = new MTParser;
	}

	CVirtualBandHolder::~CVirtualBandHolder()
	{
		delete m_pParser;
		m_pParser = NULL;
	}

	//CVirtualBandHolder::CVirtualBandHolder(const CVirtualBandHolder& in)
	//{
	//	operator = (in);
	//}
	//
	//CVirtualBandHolder& CVirtualBandHolder::operator = (const CVirtualBandHolder& in)
	//{
	//	if( &in != this)
	//	{
	//		//m_bManageSrcNoData = in.m_bManageSrcNoData;
	//		m_formula = in.m_formula;
	//		m_parser = in.m_parser;
	//		m_vars = in.m_vars;
	//	}
	//
	//	return *this;
	//}
	//

	ERMsg CVirtualBandHolder::Load(CBandsHolder* pParent)
	{

		ERMsg msg;
		msg = CSingleBandHolder::Load(pParent);

		if (!msg || m_formula.empty())
			return msg;


		CBandsHolder& bandHolder = *m_pParent;


		try
		{
			m_pParser->enableAutoVarDefinition(true);
			m_pParser->compile(convert(m_formula).c_str());
		}
		catch (MTParserException e) 
		{
			msg.ajoute(UTF8(e.m_description.c_str()));
		}

		if (msg)
		{
			//Set variable
			m_vars.resize(m_pParser->getNbUsedVars());

			if (m_pParser->getNbUsedVars() > 0)
			{
				CGeoExtents extents;
				for (unsigned int v = 0; v < m_pParser->getNbUsedVars(); v++)
				{
					m_vars[v].m_name = UTF8(m_pParser->getUsedVar(v));

					size_t pos = bandHolder.GetBandByName(m_vars[v].m_name);

					if (pos < m_vars.size())
					{
						//init noData value
						m_pParser->redefineVar(m_pParser->getUsedVar(v).c_str(), &(m_vars[v].m_value));
						m_vars[v].m_noData = bandHolder[pos]->GetNoData();
						m_bandPos.push_back(pos);

						extents.UnionExtents(bandHolder[pos]->GetInternalMapExtents());
					}
					else
					{
						msg.ajoute(string("ERROR: unknown variable: ") + m_vars[v].m_name.c_str());
					}
				}

				//update internal extent with only used bands
				m_internalMapExtents = extents;
			}

			m_datasetExtent = bandHolder.GetExtents();

			/*if(msg)
			{
				m_bandToData.clear();
				m_variableToData.clear();

				m_variableToData.resize(m_virtualBands.size());
				for(size_t i=0; i<m_virtualBands.size(); i++)
				{
					const CMTParserVariableVector& var = m_virtualBands[i].GetVars();

					m_variableToData[i].resize(var.size());
					for( size_t j=0; j<m_variableToData[i].size(); j++)
					{
						int pos = GetBandByName(var[j].m_name);

						vector<int>::const_iterator item = std::search_n( m_bandToData.begin(), m_bandToData.end(), 1, pos);

						if( item == m_bandToData.end() )
						{
							//not already in the list
							m_variableToData[i][j] = (int)m_bandToData.size();
							m_bandToData.push_back(pos);
						}
						else
						{
							//already in the list
							m_variableToData[i][j] = int(item-m_bandToData.begin());
						}
					}
				}

				*/
		}

		return msg;
	}

	void CVirtualBandHolder::Init(int windowSize)
	{
		ASSERT(!m_extents.IsRectEmpty());

		CGeoExtents mapExtents = m_internalMapExtents;
		mapExtents.IntersectExtents(m_extents);

		int halfWindow = windowSize / 2;
		double xRes = fabs(mapExtents.XRes());
		double yRes = fabs(mapExtents.YRes());
		mapExtents.m_xMin -= halfWindow * xRes;
		mapExtents.m_xMax += halfWindow * xRes;
		mapExtents.m_yMin -= halfWindow * yRes;
		mapExtents.m_yMax += halfWindow * yRes;
		mapExtents.m_xSize += 2 * halfWindow;
		mapExtents.m_ySize += 2 * halfWindow;
		mapExtents.m_xBlockSize += 2 * halfWindow;
		mapExtents.m_yBlockSize += 2 * halfWindow;
		mapExtents.IntersectExtents(m_datasetExtent);

		if (!m_bExclude && !mapExtents.IsRectEmpty())
		{
			m_dataRect = m_extents.CoordToXYPos(mapExtents); //portion of valid map in extents coordinate
			CGeoRectIndex loadRect = m_datasetExtent.CoordToXYPos(mapExtents); //get map rect to load in map coordinate

			ASSERT(loadRect.Width() == m_dataRect.Width());
			ASSERT(loadRect.Height() == m_dataRect.Height());

			m_data.resize(loadRect.Height()*loadRect.Width());

			//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
			for (int y = 0; y < loadRect.Height(); y++)
			{
				for (int x = 0; x < loadRect.Width(); x++)
				{
					vector<float> vars(m_bandPos.size());
					for (size_t z = 0; z < m_bandPos.size(); z++)
						vars[z] = m_pParent->GetPixel(m_bandPos[z], x, y);

					m_data[y*loadRect.Width() + x] = (float)Evaluate(vars);
				}
			}
		}
		else
		{
			m_dataRect.SetRectEmpty();
			m_data.clear();
		}

	}


	double CVirtualBandHolder::Evaluate(const vector<float>& vars)
	{
		ASSERT(vars.size() == m_vars.size());

		double value = MISSING_NO_DATA;
		bool bMissingValue = false;


		for (size_t i = 0; i < m_vars.size() && !bMissingValue; i++)
		{
			m_vars[i].m_value = vars[i];
			bMissingValue = !m_vars[i].IsValid();
		}

		if (!bMissingValue)
		{
			try
			{
				value = m_pParser->evaluate();
			}
			catch (MTParserException& e)
			{
				wstring test = e.m_description.c_str();
				//string msg = UTF16_UTF8(e.m_description.c_str());
			}
		}

		return value;
	}



	CExtractInfo::CExtractInfo()
	{
		//ASSERT( nbPt>=0);
		//m_nbPoint = nbPt;
		//m_factor = Max(0.0, factor);
		//m_bExposition=bExp;
	}


	ERMsg CExtractInfo::Execute(CGDALDatasetEx& map, CCallback& callback)
	{
		ERMsg msg;

		CBandsHolderMT bandHolder(3);
		msg += bandHolder.Load(map);

		if (!msg)
			return msg;

		CProjectionPtr pPrj = map.GetPrj();
		CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));

		m_factor = max(0.0, m_factor);
		m_nbBox = 0;
		m_sumStdDev = -1;
		for (int i = 0; i < NB_TYPE; i++)
			m_nbSub[i] = 0;
		//m_extents = map.GetExtents();
		//m_extents.Reproject(PT);

		int nbPoint = max(1, int(floor(m_nbPoint*0.05)));
		m_nbSub[EXTREM] = int(sqrt((float)nbPoint));
		int nbCellByBox = min(10000, max(100, (m_extents.GetNbPixels() / 3600)));//minimum of 100 cells by box and maximum of 10000
		m_nbSub[DENSITY] = min(120, max(1, m_extents.GetNbPixels() / nbCellByBox));//minimum of 1x1 box and maximum of 120x120
		m_info[EXTREM].resize(m_nbSub[EXTREM], m_nbSub[EXTREM]);
		m_info[DENSITY].resize(m_nbSub[DENSITY], m_nbSub[DENSITY]);

		callback.PushTask(GetString(IDS_STR_EXTRACT_INFO), m_extents.GetNbPixels());

		vector<pair<int, int>> XYindex = m_extents.GetBlockList();

		size_t n = 0;
		for (int xy = 0; xy < (int)XYindex.size(); xy++)//for all blocks
		{
			int xBlock = XYindex[xy].first;
			int yBlock = XYindex[xy].second;
			CGeoExtents blockExtents = m_extents.GetBlockExtents(xBlock, yBlock);

			bandHolder.LoadBlock(blockExtents);

			vector<CDataWindowPtr> input;
			bandHolder.GetWindow(input);

			//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
			for (int y = 0; y < blockExtents.m_ySize; y++)
			{
				for (int x = 0; x < blockExtents.m_xSize; x++)
				{
					if (input[0]->IsValid(x, y))
					{
						CGridPoint pt;
						((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
						pt.m_z = input[0]->at(x, y);
						if (m_bExposition)
							input[0]->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);

						if (pPrj->IsProjected())
						{
							CGeoPoint ptGeo(pt);
							ptGeo.Reproject(PT);
							pt.m_latitudeDeg = ptGeo.m_y;
						}

						Add(pt);
					}
					msg += callback.StepIt();
				}
			}
		}


		m_nbBox = GetNbBox(DENSITY);

		CClassify classify;
		for (size_t x = 0; x < m_info[DENSITY].size_x(); x++)
		{
			for (size_t y = 0; y < m_info[DENSITY].size_y(); y++)
			{
				if (m_info[DENSITY][x][y][NB_VALUE] > 0)
					classify.Add(pow(m_info[DENSITY][x][y][STD_DEV], m_factor), 1);
			}
		}
		classify.ClassifyEqualInterval(30);

		callback.PopTask();

		return msg;
	}


	CGeoPointIndex CExtractInfo::GetPosIndex(const CGeoPoint& ptIn, int t)
	{
		ASSERT(m_extents.PtInRect(ptIn));
		ASSERT(!m_extents.IsRectEmpty());
		ASSERT(t >= 0 && t < NB_TYPE);
		ASSERT(m_nbSub[t] > 0);


		CGeoPoint pt(ptIn);
		CGeoPointIndex xy(-1, -1);

		pt -= m_extents.LowerLeft();

		xy.m_x = min(m_nbSub[t] - 1, (int)((m_nbSub[t] * pt.m_x / m_extents.Width()) + 0.0000001));
		xy.m_y = min(m_nbSub[t] - 1, (int)((m_nbSub[t] * pt.m_y / m_extents.Height()) + 0.0000001));

		ASSERT(xy.m_x >= 0 && xy.m_x < m_nbSub[t]);
		ASSERT(xy.m_y >= 0 && xy.m_y < m_nbSub[t]);
		//int i= xy.y*m_nbSub+xy.x;
		//ASSERT( i>=0 && i<m_nbSub*m_nbSub);

		return xy;
	}

	double CExtractInfo::GetPBase()const
	{
		return (double)m_nbPoint / m_stat[NB_VALUE];
	}

	double CExtractInfo::GetP(const CGeoPoint& pt)
	{
		double p = 1;
		if (!m_extents.IsRectNull() && m_factor > 0)
		{
			ASSERT(m_nbBox > 0);
			if (m_sumStdDev == -1)
			{
				m_sumStdDev = 0;
				for (size_t x = 0; x < m_info[DENSITY].size_x(); x++)
				{
					for (size_t y = 0; y < m_info[DENSITY].size_y(); y++)
					{
						if (m_info[DENSITY][x][y][NB_VALUE] > 0)
							m_sumStdDev += pow(m_info[DENSITY][x][y][STD_DEV], m_factor);
					}
				}
			}

			CGeoPointIndex xy = GetPosIndex(pt, DENSITY);
			if (m_info[DENSITY][xy.m_x][xy.m_y][NB_VALUE] > 0 && m_sumStdDev > 0)
				p = (pow(m_info[DENSITY][xy.m_x][xy.m_y][STD_DEV], m_factor)) / m_sumStdDev;

			p = max(0.0001, p*m_nbBox);
		}

		return p;
	}

	void CExtractInfo::Add(const CGridPoint& pt)
	{
		ASSERT(pt.m_z > -999);

		//zero is a dangerous value because often use as no data or see level
		if (pt.m_z != 0)
		{
			for (int t = 0; t < NB_TYPE; t++)
			{
				CGeoPointIndex xy = GetPosIndex(pt, t);
				//if it's the lowest point
				if (m_info[t][xy.m_x][xy.m_y][NB_VALUE] == 0 || pt.m_z < m_info[t][xy.m_x][xy.m_y][LOWEST])
					m_info[t][xy.m_x][xy.m_y].m_lowest = pt;

				//if it's the highest point
				if (m_info[t][xy.m_x][xy.m_y][NB_VALUE] == 0 || pt.m_z > m_info[t][xy.m_x][xy.m_y][HIGHEST])
					m_info[t][xy.m_x][xy.m_y].m_highest = pt;

				m_info[t][xy.m_x][xy.m_y].m_stat += pt.m_z;
			}
		}

		m_stat += pt.m_z;

	}

	void CExtractInfo::GetExtremPoints(CGridPointVector& pts)const
	{
		for (size_t x = 0; x < m_info[EXTREM].size_x(); x++)
		{
			for (size_t y = 0; y < m_info[EXTREM].size_y(); y++)
			{
				if (m_info[EXTREM][x][y].m_stat[NB_VALUE] > 0)
				{
					pts.push_back(m_info[EXTREM][x][y].m_lowest);
					pts.push_back(m_info[EXTREM][x][y].m_highest);
				}
			}
		}
	}

	int CExtractInfo::GetNbBox(int t)const
	{
		int count = 0;
		for (size_t x = 0; x < m_info[t].size_x(); x++)
		{
			for (size_t y = 0; y < m_info[t].size_y(); y++)
			{
				if (m_info[t][x][y][NB_VALUE] > 0)
					count++;
			}
		}

		return count;
	}



	//vector<size_t> CGeoCoordFile::GetCoordinate(const StringVector& header, const string X, const string Y)
	//{
	//	vector<size_t> coord;
	//
	//	set<size_t> posX = header.FindAll(X);
	//	set<size_t> posY = header.FindAll(Y);
	//	if (posX.size() == 1 && posY.size() == 1)
	//	{
	//		coord.push_back(*posX.begin());
	//		coord.push_back(*posY.begin());
	//	}
	//
	//
	//	return coord;
	//}

	ERMsg CGeoCoordFile::ManageProjection(size_t dstPrjID)
	{
		ERMsg msg;

		size_t srcPrjID = m_xy.GetPrjID();


		if (srcPrjID == PRJ_NOT_INIT)
		{
			cout << "WARNING: Unknown projection for coordinates points, assuming to be the same projection as the input image." << endl;
			m_xy.SetPrjID(dstPrjID);
			srcPrjID = dstPrjID;
		}

		if (srcPrjID != PRJ_NOT_INIT && dstPrjID != PRJ_NOT_INIT && srcPrjID != dstPrjID)
		{
			const CProjectionTransformation& PT = CProjectionTransformationManager::Get(srcPrjID, dstPrjID);
			if (!PT.IsSame())
			{
				cout << "WARNING: input coordinates and input images have different projection. Re-projection on the fly will be done." << endl;
				msg = m_xy.Reproject(PT);

			}
		}

		return msg;
	}

	ERMsg CGeoCoordFile::Load(const string& filePath, const string X, const string Y)
	{
		ERMsg msg;

		ifStream file;
		auto myloc = std::locale();
		file.imbue(myloc);

		msg = file.open(filePath);

		if (msg)
		{
			string prjFilePath(filePath);
			SetFileExtension(prjFilePath, ".prj");

			CProjection prj;
			if (FileExists(prjFilePath))
			{
				//

				msg = prj.Load(prjFilePath);

				if (!msg)
					return msg;

				m_prjID = prj.GetPrjID();
			}



			CSVIterator loop(file);
			m_header = ToString(loop.Header(), "", ",", "");

			//vector<size_t> coordinatePos = GetCoordinate(loop.Header(), X, Y);
			set<size_t> posX = loop.Header().FindAll(X, false, true);
			set<size_t> posY = loop.Header().FindAll(Y, false, true);
			if (posX.size() != 1 || posY.size() != 1)
			{
				msg.ajoute("Invalid header for coordinates (X,Y). " + ToString(posX.size() + posY.size()) + " column(s) match for header \"" + X.c_str() + "\" and \"" + Y.c_str() + "\".");
				return msg;
			}

			m_Xcol = *posX.begin();
			m_Ycol = *posY.begin();

			m_xy.reserve(file.length() / (loop.Header().size() * 10));
			reserve(file.length() / (loop.Header().size() * 10));


			for (int i = 0; loop != CSVIterator(); ++loop, ++i)
			{
				if (loop->size() == loop.Header().size())  //&& coordinatePos[0] < loop->size() && coordinatePos[1] < loop->size())
				{
					CGeoPoint xy(prj.GetPrjID());
					xy.m_x = stod(loop->at(m_Xcol));
					xy.m_y = stod(loop->at(m_Ycol));
					m_xy.push_back(xy);
					push_back(loop->GetLastLine());
				}
				else
				{
					msg.ajoute("Invalid CSV file: " + filePath);
					msg.ajoute("Reading error at line: " + ToString(i + 1));
					msg.ajoute("Line:\t" + loop->GetLastLine());
					return msg;
				}
			}
		}


		return msg;
	}


	ERMsg CGeoCoordFile::Save(const string& filePath, bool bSaveDefinitionFile)
	{
		ERMsg msg;

		ofStream file;
		auto myloc = locale();
		file.imbue(myloc);


		msg = file.open(filePath);




		if (msg)
		{
			std::vector<char> buffer(1024 * 1024);
			file.rdbuf()->pubsetbuf(&buffer.front(), buffer.size());

			file << m_header << endl;

			for (StringVector::const_iterator it = begin(); it != end(); it++)
			{
				file.write(*it);
				file.write("\n");
			}

			file.close();

			//save projection
			if (m_prjID != PRJ_NOT_INIT)
			{
				string prjFilePath(filePath);
				SetFileExtension(prjFilePath, ".prj");
				CProjectionPtr pPrj = GetProjection(m_prjID);
				msg = pPrj->Save(prjFilePath);
			}

			if (bSaveDefinitionFile && m_Xcol != NOT_INIT && m_Ycol != NOT_INIT)
			{


				//write definition file
				string csvtFilePath(filePath);
				SetFileExtension(csvtFilePath, ".csvt");

				if (file.open(csvtFilePath))
				{
					StringVector header(m_header, ",;");
					for (size_t i = 0; i < header.size(); i++)
					{
						if (i > 0)
							file << ",";
						if (i == m_Xcol)
							file << "CoordX";
						else if (i == m_Ycol)
							file << "CoordY";
						else
							file << "Real";
					}

					file << endl;
					file.close();
				}//if open def file
			}//if sane def file

		}//if msg

		return msg;
	}


	ERMsg CGeoCoordTimeFile::Load(const string& filePath, const string X, const string Y, const std::string Time)
	{
		ERMsg msg;
		msg = CGeoCoordFile::Load(filePath, X, Y);
		if (msg)
		{
			StringVector header(m_header, ",;");
			set<size_t> posTime = header.FindAll(Time, false, true);
			if (posTime.size() == 1)
			{
				m_time.reserve(size());

				for (size_t i = 0; i < size(); ++i)
				{
					StringVector tmp(at(i), ",;");
					if (*posTime.begin() < tmp.size())
						m_time.push_back(tmp[*posTime.begin()]);
					else
						msg.ajoute("Error at line : " + at(i));
				}
			}
			else
			{
				msg.ajoute("Invalid column header time name " + std::to_string(posTime.size()) + " column(s) match for header \"" + Time + "\".");
				return msg;
			}

		}


		return msg;
	}


	//*************************************************************************************************************************************************************



	//
	//
	//CPLErr VirtualBandFunction(void **papoSources, int nSources, void *pData,
	//	int nXSize, int nYSize,
	//	GDALDataType eSrcType, GDALDataType eBufType,
	//	int nPixelSpace, int nLineSpace)
	//{
	//	ASSERT(false);
	//
	//	//int ii, iLine, iCol;
	//	//double pix_val;
	//	//double x0, x3, x4, x8;
	//
	//	//// ---- Init ----
	//	//if (nSources != 4) return CE_Failure;
	//
	//	//// ---- Set pixels ----
	//	//for (iLine = 0; iLine < nYSize; iLine++)
	//	//{
	//	//	for (iCol = 0; iCol < nXSize; iCol++)
	//	//	{
	//	//		ii = iLine * nXSize + iCol;
	//	//		/* Source raster pixels may be obtained with SRCVAL macro */
	//	//		x0 = SRCVAL(papoSources[0], eSrcType, ii);
	//	//		x3 = SRCVAL(papoSources[1], eSrcType, ii);
	//	//		x4 = SRCVAL(papoSources[2], eSrcType, ii);
	//	//		x8 = SRCVAL(papoSources[3], eSrcType, ii);
	//
	//	//		pix_val = sqrt((x3*x3 + x4*x4) / (x0*x8));
	//
	//	//		GDALCopyWords(&pix_val, GDT_Float64, 0,
	//	//			((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
	//	//			eBufType, nPixelSpace, 1);
	//	//	}
	//	//}
	//
	//	// ---- Return success ----
	//	return CE_None;
	//}

}