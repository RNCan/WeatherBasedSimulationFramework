//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
// 14-09-2023   Rémi Saint-Amant	Port un Linux
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include <float.h>
#include <limits.h>
#include <iostream>
#include <chrono>

#include "external/pugixml/pugixml.hpp"
//#include "external/tinyxml2.h"
#include "basic/OpenMP.h"
#include "geomatic/GDAL.h"
#include "geomatic/UtilGDAL.h"
#include "geomatic/GDALDatasetEx.h"


using namespace std;
//using namespace tinyxml2;
using namespace pugi;

typedef xml_object_range<xml_named_node_iterator> xml_nodes;
namespace WBSF
{
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
					ComputeStats(options.m_bQuiet, options.m_bMulti ? options.m_CPU : 1);

				if (!options.m_overviewLevels.empty())
					BuildOverviews(options.m_overviewLevels, options.m_bQuiet, options.m_bMulti ? options.m_CPU : 1);

				if (options.m_bComputeHistogram)
					ComputeHistogram(options.m_bQuiet, options.m_bMulti ? options.m_CPU : 1);
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
				WBSF::Close(m_poDataset);
				m_poDataset = NULL;
			}

			//        m_pProjection.reset();
			m_filePath.clear();
			m_extents.clear();
			m_bVRT = false;
			m_bOpenUpdate = false;
			m_internalExtents.clear();
			m_internalName.clear();
			//			m_virtualBands.clear();
			m_scenes_def.clear();

			//this vector is intended to keep each band of a VRT file open for writing
			assert(m_poDataset == NULL);
			assert(m_poDatasetVector.empty());

			//temporal section of the dataset
		}
	}

	short CGDALDatasetEx::GetDataType(size_t i)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);
		GDALRasterBand* pBand = me.GetRasterBand(i);

		short eType = pBand->GetRasterDataType();
		return eType;
	}

	ERMsg CGDALDatasetEx::VerifyNoData(double nodata, size_t i)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::VerifyNoData(nodata, eType);
	}

	double CGDALDatasetEx::GetBandLimit(size_t i, bool bLow)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::GetTypeLimit(eType, bLow);
	}

	double CGDALDatasetEx::PostTreatment(double v, size_t i, double shiftedBy, bool bRound)
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

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
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::LimitToBound(v, eType, shiftedBy, bRound);
	}

	double CGDALDatasetEx::GetDefaultNoData(size_t i)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		short eType = GetDataType(i);
		return WBSF::GetDefaultNoData(eType);
	}

	bool CGDALDatasetEx::HaveNoData(size_t i)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));

		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);

		int bSuccess = 0;
		me.GetRasterBand(i)->GetNoDataValue(&bSuccess);
		return bSuccess != 0;
	}

	double CGDALDatasetEx::GetNoData(size_t i)const
	{
		assert(i < GetRasterCount());
		assert(GetRasterBand(i));
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



	GDALRasterBand* CGDALDatasetEx::GetRasterBand(size_t i)
	{
		assert(i < GetRasterCount());
		return m_bMultipleImages ? m_poDatasetVector[i]->GetRasterBand(1) : m_poDataset->GetRasterBand(int(i) + 1);

	}

	const GDALRasterBand* CGDALDatasetEx::GetRasterBand(size_t i)const
	{
		assert(i < GetRasterCount());
		return m_bMultipleImages ? m_poDatasetVector[i]->GetRasterBand(1) : m_poDataset->GetRasterBand(int(i) + 1);

	}


	ERMsg CGDALDatasetEx::OpenInputImage(const string& filePath, const CBaseOptions& options)
	{
		assert(!IsOpen());

		ERMsg msg;

		msg = WBSF::OpenInputImage(filePath, &m_poDataset, options.m_srcNodata, options.m_bUseDefaultNoData, options.m_bReadOnly);
		if (msg &&
			(options.m_scene_extents[0] != NOT_INIT && options.m_scene_extents[0] >= GetNbScenes()) ||
			(options.m_scene_extents[1] != NOT_INIT && options.m_scene_extents[1] >= GetNbScenes()))
			msg.ajoute("Scenes {" + to_string(options.m_scene_extents[0] + 1) + ", " + to_string(options.m_scene_extents[1] + 1) + "} must be in range {1, " + to_string(GetNbScenes()) + "}");


		if (msg)
		{

			m_extents = WBSF::GetExtents(m_poDataset);
			m_scenes_def = options.m_scenes_def;

			const char* desc = m_poDataset->GetDriver()->GetDescription();
			m_bVRT = strcmp(desc, "VRT") == 0;

			//        msg = CProjectionManager::CreateProjection(m_poDataset->GetProjectionRef());

			//if (msg)
			//{
			//OGRSpatialReference  SRS(m_poDataset->GetProjectionRef());
			//m_extents.SetPrjID(SRS.IsGeographic() ? PRJ_GEOGRAPHIC : PRJ_PROJECTED);

			//        char		      *pszProjection;
			//
			//        pszProjection = (char *)GDALGetProjectionRef(hDataset);
			//
			//        hSRS = OSRNewSpatialReference(NULL);

			//          m_pProjection = CProjectionManager::GetPrj(m_poDataset->GetProjectionRef());
			m_internalExtents.reserve(GetRasterCount());
			m_internalName.reserve(GetRasterCount());
			//m_poDataset->GetFileList();

			//Load internal rect
			map<int, int> xBlockSizes;
			map<int, int> yBlockSizes;

			m_internalExtents.resize(GetRasterCount(), m_extents);
			m_internalName.resize(GetRasterCount());



			if (m_bVRT)
			{
				ifStream file;

				if (file.open(filePath))
				{
					string str = file.GetText();

					xml_document xmlDataset;
					xml_parse_result result = xmlDataset.load_string(str.c_str());
					if (!result)
					{
						msg.ajoute(result.description());
						return msg;
					}

					//xml_node root = xmlDataset.root();
					//assert(!root.children().empty());


					//bandsDef = root.children("VRTRasterBand");
					//bandsDef = root.children("VRTRasterBand");
					xml_nodes vrt_bands = xmlDataset.child("VRTDataset").children("VRTRasterBand");
					size_t count = 0;
					for (auto it = vrt_bands.begin(); it != vrt_bands.end(); it++)
						count++;

					assert(count == 1 || count == GetRasterCount());

					if (count == GetRasterCount())
					{
						xml_nodes vrt_bands = xmlDataset.child("VRTDataset").children("VRTRasterBand");
						auto it = vrt_bands.begin();
						for (size_t b = 0; b < GetRasterCount(); b++, it++)
						{
							string name;

							CGeoExtents extents = m_extents;

							assert(!it->child("ComplexSource").empty());

							xml_node source = it->child("ComplexSource");
							if (!source.empty())
							{
								xml_node sourceFileName = source.child("SourceFilename");
								if (!sourceFileName.empty())
								{
									bool bRelativeToVRT = sourceFileName.attribute("relativeToVRT").as_bool();
									if (bRelativeToVRT)
									{
										name = GetPath(filePath);
										name += sourceFileName.text().as_string();
									}
									else
									{
										name = sourceFileName.text().as_string();
									}
								}


								CGeoSize size;
								CGeoSize blockSize;

								xml_node sourceProperties = source.child("SourceProperties");
								if (!sourceProperties.empty())
								{
									size.m_x = sourceProperties.attribute("RasterXSize").as_int();
									size.m_y = sourceProperties.attribute("RasterYSize").as_int();
									blockSize.m_x = sourceProperties.attribute("BlockXSize").as_int();
									blockSize.m_y = sourceProperties.attribute("BlockYSize").as_int();
									xBlockSizes[blockSize.m_x]++;
									yBlockSizes[blockSize.m_y]++;
								}

								xml_node dstRect = source.child("DstRect");
								if (!dstRect.empty() && size.m_x > 0 && size.m_y > 0)
								{
									CGeoRectIndex rect;
									rect.m_x = dstRect.attribute("xOff").as_int();
									rect.m_y = dstRect.attribute("yOff").as_int();
									rect.m_xSize = dstRect.attribute("xSize").as_int();
									rect.m_ySize = dstRect.attribute("ySize").as_int();

									extents = CGeoExtents(m_extents.XYPosToCoord(rect), size, blockSize);
								}
							}
							m_internalExtents[b] = extents;
							m_internalName[b] = name;

						}


					}
					else
					{
						//When there is no nodata, vrt with -separate have singleSource
						//	//VRT file without -separate options.
						//	considerate VRT with only one band as standard image
						m_bVRT = false;

					}

					file.close();
				}
			}



			//
			//			XMLDocument xmlDataset;
			//
			//			XMLElement* bandsDef = nullptr;
			//			if (m_bVRT)
			//			{
			//				ifStream file;
			//
			//				if (file.open(filePath))
			//				{
			//					//assert(false);//todo
			//					string str = file.GetText();
			//
			//					//	XMLDocument xmlDataset;
			//					XMLError err = xmlDataset.Parse(str.c_str());
			//					if (err != XML_SUCCESS)
			//					{
			//						msg.ajoute(xmlDataset.ErrorStr());
			//						return msg;
			//					}
			//
			//					XMLElement* root = xmlDataset.RootElement();
			//					assert(root);
			//
			//					bandsDef = root->FirstChildElement("VRTRasterBand");
			//
			//					size_t count = 0;
			//					for (XMLElement* ele = bandsDef; ele; ele = ele->NextSiblingElement())
			//						count++;
			//
			//					assert(count == 1 || count == GetRasterCount());
			//
			//					if (count != GetRasterCount())
			//					{
			//						//When there is no nodata, vrt with -separate have singleSource
			//						//	//VRT file without -separate options.
			//						//	considerate VRT with only one band as standard image
			//						m_bVRT = false;
			//
			//					}
			//
			//					file.close();
			//				}
			//			}
			//
			//			for (size_t j = 0; j < GetRasterCount(); j++)
			//			{
			//				string name;
			//				// string virtualBandEquation;
			//
			//				CGeoExtents extents = m_extents;
			//				//assert(false);//todo
			//				if (m_bVRT)
			//				{
			//					//XMLElement* bandsDef = xmlDataset.FirstChildElement("VRTRasterBand");
			//					assert(bandsDef->FirstChildElement("ComplexSource"));
			//
			//					XMLElement* pSource = bandsDef->FirstChildElement("ComplexSource");
			//					if (pSource)
			//					{
			//						XMLElement* pSourceFileName = pSource->FirstChildElement("SourceFilename");
			//						//LPXNode pSourceFileName = bandsDef[j]->Find("SourceFilename");
			//						if (pSourceFileName)
			//						{
			//							bool bRelativeToVRT = ToBool(pSourceFileName->Attribute("relativeToVRT"));
			//							if (bRelativeToVRT)
			//							{
			//								name = GetPath(filePath);
			//								name += pSourceFileName->GetText();
			//							}
			//							else
			//							{
			//								name = pSourceFileName->GetText();
			//							}
			//						}
			//
			//
			//						CGeoSize size;
			//						CGeoSize blockSize;
			//						//XMLElement* pSourceProperties = bandsDef->Find("SourceProperties");
			//						XMLElement* pSourceProperties = pSource->FirstChildElement("SourceProperties");
			//						if (pSourceProperties)
			//						{
			//							size.m_x = atoi(pSourceProperties->Attribute("RasterXSize"));
			//							size.m_y = atoi(pSourceProperties->Attribute("RasterYSize"));
			//							blockSize.m_x = atoi(pSourceProperties->Attribute("BlockXSize"));
			//							blockSize.m_y = atoi(pSourceProperties->Attribute("BlockYSize"));
			//							xBlockSizes[blockSize.m_x]++;
			//							yBlockSizes[blockSize.m_y]++;
			//						}
			//
			//						XMLElement* pDstRect = pSource->FirstChildElement("DstRect");
			//						if (pDstRect && size.m_x > 0 && size.m_y > 0)
			//						{
			//							CGeoRectIndex rect;
			//							rect.m_x = atoi(pDstRect->Attribute("xOff"));
			//							rect.m_y = atoi(pDstRect->Attribute("yOff"));
			//							rect.m_xSize = atoi(pDstRect->Attribute("xSize"));
			//							rect.m_ySize = atoi(pDstRect->Attribute("ySize"));
			//
			//							extents = CGeoExtents(m_extents.XYPosToCoord(rect), size, blockSize);
			//						}
			//
			//						//if its an virtual band, load equation
			//						/*LPCSTR pSubClassName = bandsDef[j]->GetAttrValue("subClass");
			//						if (pSubClassName && IsEqual(pSubClassName, "VRTDerivedRasterBand"))
			//						{
			//							LPCSTR pPixelFuncName = bandsDef[j]->GetChildValue("PixelFunctionType");
			//							if (pPixelFuncName && IsEqual(pPixelFuncName, "ImageCalculator"))
			//							{
			//								LPXNode pMeta = bandsDef[j]->GetChild("Metadata");
			//								if (pMeta)
			//								{
			//									XNodes nodes = pMeta->GetChilds("MDI");
			//									for (XNodes::const_iterator it = nodes.begin(); it != nodes.end(); it++)
			//									{
			//										LPCSTR pKey = (*it)->GetAttrValue("key");
			//										if (pKey && IsEqual(pKey, "Equation"))
			//										{
			//											virtualBandEquation = (*it)->value;
			//											break;
			//										}
			//									}
			//								}
			//							}
			//						}*/
			//					}//if bandDef size == raster count (VRT)
			//				}
					//m_internalExtents.push_back(extents);
					//m_internalName.push_back(name);
					//m_virtualBands.push_back(virtualBandEquation);


					//bandsDef = bandsDef->NextSiblingElement();
					//  }//for raster count

			if (m_bVRT && !xBlockSizes.empty() && !yBlockSizes.empty())
			{
				//Get the most popular block sise for x and y
				std::map< int, int > xFlipedMap = converse_map(xBlockSizes);
				std::map< int, int > yFlipedMap = converse_map(yBlockSizes);

				m_extents.m_xBlockSize = xFlipedMap.rbegin()->second;
				m_extents.m_yBlockSize = yFlipedMap.rbegin()->second;

			}
			//}//if msg
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
				std::vector<std::string> bandsName = Tokenize(options.m_VRTBandsName, "|,;");
				assert(options.m_nbBands < 65000);
				CBaseOptions optionsB(options);
				optionsB.m_format = "GTiff";
				optionsB.m_nbBands = 1;
				for (size_t i = 0; i < options.m_nbBands && msg; i++)
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
			char** papszOptions = NULL;
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


			for (size_t i = 0; i < options.m_nbBands && msg; i++)
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

			m_filePath = filePath;
			m_extents = options.m_extents;
			m_bOpenUpdate = true;

			//m_scenesSize = options.m_scenesSize;
			m_scenes_def = options.m_scenes_def;
			//Dataset()->GetAccess() == GA_Update;
		}


		return msg;
	}

	int CGDALDatasetEx::GetRasterXSize()const
	{
		return m_extents.m_xSize;
	}
	int CGDALDatasetEx::GetRasterYSize()const
	{
		return m_extents.m_ySize;
	}
	size_t CGDALDatasetEx::GetRasterCount()const
	{
		return m_bMultipleImages ? (int)m_poDatasetVector.size() : m_poDataset->GetRasterCount();
	}

	void CGDALDatasetEx::SetVRTBand(size_t i, GDALDataset* pDataset)
	{
		assert(m_bMultipleImages);
		assert(i <= m_poDatasetVector.size());
		assert(pDataset);

		//    char* pList = NULL;

		char** papszFileList = pDataset->GetFileList();
		int fileCount = CSLCount(papszFileList);
		if (fileCount == 1)
			m_internalName[i] = papszFileList[0];

		CSLDestroy(papszFileList);

		m_poDatasetVector[i] = pDataset;
	}

	void CGDALDatasetEx::CloseVRTBand(size_t  i)
	{
		assert(m_bMultipleImages);
		assert(i < m_poDatasetVector.size());

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


			assert(m_internalName.size() == GetRasterCount());
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
						GDALRasterBand* pBand = GetRasterBand(b);
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
			//	assert(m_internalName.size() == GetRasterCount());
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

#if _MSVC
		string command = "gdalbuildvrt.exe -separate -overwrite -input_file_list \"" + listFilePath + "\" \"" + m_filePath + "\"" + (options.m_bQuiet ? " -q" : "");
		msg = WinExecWait(command);
#else
		string command = "gdalbuildvrt -separate -overwrite -input_file_list \"" + listFilePath + "\" \"" + m_filePath + "\"" + (options.m_bQuiet ? " -q" : "");
		system(command.c_str());
#endif


		return msg;
	}


	double CGDALDatasetEx::ReadPixel(size_t i, int x, int y)const
	{
		assert(i < GetRasterCount());
		assert(x >= 0 && x < Dataset()->GetRasterXSize());
		assert(y >= 0 && y < Dataset()->GetRasterYSize());
		CGDALDatasetEx& me = const_cast<CGDALDatasetEx&>(*this);

		double v = GetNoData(i);
		GDALRasterBand* pBand = me.GetRasterBand(i);
		pBand->RasterIO(GF_Read, x, y, 1, 1, &v, 1, 1, GDT_Float64, 0, 0);

		return v;
	}



	//Read block by internal block size
	void CGDALDatasetEx::ReadBlock(size_t i, size_t j, CRasterWindow& block)const
	{
		CGeoExtents extents = GetExtents().GetBlockExtents(int(i), int(j));
		CGeoRectIndex dataRect = GetExtents().GetBlockRect(int(i), int(j));

		block.resize(GetRasterCount(), extents);
		for (size_t b = 0; b < GetRasterCount(); b++)
		{
			const CGeoExtents& iExtents = GetInternalExtents(b);
			if (extents.IsIntersect(iExtents))
			{
				GDALRasterBand* poBand = m_poDataset->GetRasterBand(int(b) + 1);//one base in direct load

				block[b].resize(dataRect, dataRect, extents, DataType(poBand->GetNoDataValue()));


				int nXBlockSize, nYBlockSize = 0;
				poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

				assert(nXBlockSize == m_extents.m_xBlockSize);
				assert(nYBlockSize == m_extents.m_yBlockSize);
				GDALDataType type = poBand->GetRasterDataType();
				if (type == GDT_Int16)
				{
					if (nXBlockSize == m_extents.m_xBlockSize && nYBlockSize == m_extents.m_yBlockSize)
						poBand->ReadBlock(int(i), int(j), block[b].data().data());
				}
				else
				{
					poBand->RasterIO(GF_Read, dataRect.m_x, dataRect.m_y, dataRect.m_xSize, dataRect.m_ySize, block[b].data().data(), dataRect.m_xSize, dataRect.m_ySize, GDT_Int16, 0, 0);
				}
			}
		}
	}


	void CGDALDatasetEx::ReadBlock(const CGeoExtents& windowExtents, CRasterWindow& window_data, int rings, int IOCPU, size_t first_scene, size_t last_scene)const
	{
		assert(first_scene < GetNbScenes() && last_scene < GetNbScenes());

		if (first_scene == NOT_INIT)
			first_scene = 0;
		if (last_scene == NOT_INIT)
			last_scene = GetNbScenes();

		assert(first_scene <= last_scene);

		size_t totalMem = 0;

		size_t first_layer = first_scene * GetSceneSize();
		//size_t last_layer = (last_scene +1)* GetSceneSize();
		size_t nb_layer = (last_scene - first_scene + 1) * GetSceneSize();

		int nb_non_empty = 0;
		window_data.resize(nb_layer, windowExtents);
		//#pragma omp parallel for schedule(static, 1)  num_threads( IOCPU ) if(IOCPU>1)
		for (int64_t ii = 0; ii < int64_t(nb_layer); ii++)
		{
			size_t i = first_layer + ii;
			CGeoExtents iExtents = GetInternalExtents(i);
			CGeoExtents loadExtents = ComputeLoadExtent(windowExtents, iExtents, rings);
			if (!loadExtents.IsEmpty())
			{
				CGeoRectIndex windowRect = m_extents.CoordToXYPos(windowExtents); //portion of valid map in extents coordinate
				CGeoRectIndex loadRect = m_extents.CoordToXYPos(loadExtents); //get map rect to load in map coordinate

				if (!windowRect.IsRectEmpty() && !loadRect.IsRectEmpty())
				{
					assert(loadRect.m_x >= 0 && loadRect.m_x <= GetRasterXSize());
					assert(loadRect.m_y >= 0 && loadRect.m_y <= GetRasterYSize());
					assert(loadRect.m_xSize >= 0 && loadRect.m_xSize <= GetRasterXSize());
					assert(loadRect.m_ySize >= 0 && loadRect.m_ySize <= GetRasterYSize());
					assert(loadRect.Width() >= windowRect.Width());
					assert(loadRect.Height() >= windowRect.Height());

					window_data[ii].resize(loadRect, windowRect, windowExtents, DataType(GetNoData(i)));
					GDALRasterBand* pBand = m_poDataset->GetRasterBand(int(i + 1));//1 base
					pBand->RasterIO(GF_Read, loadRect.m_x, loadRect.m_y, loadRect.m_xSize, loadRect.m_ySize, window_data[ii].data().data(), loadRect.m_xSize, loadRect.m_ySize, GDT_Int16, 0, 0);

#pragma omp atomic 
					nb_non_empty++;
				}
			}

			totalMem += window_data.size() * sizeof(DataType);

		}

		if (nb_non_empty == 0)
			window_data.clear();
	}


	CGeoExtents CGDALDatasetEx::ComputeLoadExtent(const CGeoExtents& window_extents, const CGeoExtents& iExtents, int rings)const
	{
		assert(!window_extents.IsRectEmpty());

		CGeoExtents loadExtents = window_extents;
		loadExtents.IntersectExtents(iExtents);

		//	int halfWindow = windowSize / 2;
		bool bx = loadExtents.m_xBlockSize == loadExtents.m_xSize;
		bool by = loadExtents.m_yBlockSize == loadExtents.m_ySize;
		double xRes = fabs(loadExtents.XRes());
		double yRes = fabs(loadExtents.YRes());
		loadExtents.m_xMin -= rings * xRes;
		loadExtents.m_xMax += rings * xRes;
		loadExtents.m_yMin -= rings * yRes;
		loadExtents.m_yMax += rings * yRes;
		loadExtents.m_xSize += 2 * rings;
		loadExtents.m_ySize += 2 * rings;
		if (bx)
			loadExtents.m_xBlockSize = loadExtents.m_xSize;
		if (by)
			loadExtents.m_yBlockSize = loadExtents.m_ySize;


		loadExtents.IntersectExtents(m_extents);
		return loadExtents;
	}
	void CGDALDatasetEx::UpdateOption(CBaseOptions& options)const
	{
		assert(GetRasterCount() > 0);

		if (options.m_format.empty())
		{
			GDALDriver* poDriverOut = m_poDataset->GetDriver();
			options.m_format = poDriverOut->GetDescription();
		}



		//short
		if (options.m_outputType == GDT_Unknown)
			options.m_outputType = const_cast<GDALRasterBand*>(GetRasterBand(0))->GetRasterDataType();

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
			options.m_extents.SetPrjID(m_extents.GetPrjID());//set only projection of input map
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
			/*if (options.m_extents.m_yBlockSize > 1)
			{

				if (Find(options.m_createOptions, "TILED", false, false) == NOT_INIT)
					options.m_createOptions.push_back("TILED=YES");
				if (Find(options.m_createOptions, "BLOCKXSIZE", false, false) == NOT_INIT)
					options.m_createOptions.push_back("BLOCKXSIZE=" + to_string(options.m_extents.m_xBlockSize));
				if (Find(options.m_createOptions, "BLOCKYSIZE", false, false) == NOT_INIT)
					options.m_createOptions.push_back("BLOCKYSIZE=" + to_string(options.m_extents.m_yBlockSize));
			}*/
		}

		//		if (!options.m_period.IsInit())
		//		options.m_period = GetPeriod();

		//if memoryLimit is set, override block size
		if (options.m_memoryLimit > 0)
		{
			CGeoSize size = ComputeBlockSize(options.m_memoryLimit, options.m_extents);
			options.m_extents.m_xBlockSize = size.m_x;
			options.m_extents.m_yBlockSize = size.m_y;
		}

		//Limit block size to the size of the image
		options.m_extents.m_xBlockSize = min(min(options.m_extents.m_xSize, options.m_extents.m_xBlockSize), m_poDataset->GetRasterXSize());
		options.m_extents.m_yBlockSize = min(min(options.m_extents.m_ySize, options.m_extents.m_yBlockSize), m_poDataset->GetRasterYSize());

		//get the nearest grid cell
		if (options.m_bTap)
			options.m_extents.AlignTo(m_extents);

		//scenes selection
		if (options.m_scene_extents[0] == NOT_INIT)
			options.m_scene_extents[0] = 0;
		if (options.m_scene_extents[1] == NOT_INIT)
			options.m_scene_extents[1] = GetNbScenes() - 1;

		if (options.m_dstNodata == MISSING_NO_DATA)
		{
			//if there is no output nodata and there is no image input nodata then
			//All types except Byte will used the minimum value of the type
			if (options.m_bUseDefaultNoData)
				options.m_dstNodata = WBSF::GetDefaultNoData(options.m_outputType);
		}

		if (options.m_dstNodataEx == MISSING_NO_DATA)
		{
			//take dst no data by default
			options.m_dstNodataEx = options.m_dstNodata;
		}


		//Add same compression if not specified
		//    if (Find(options.m_createOptions, "COMPRESS", false, false) == -1)
		//    {
		//    	char** test = const_cast<GDALRasterBand*>(GetRasterBand(0))->GetMetadata("IMAGE_STRUCTURE");
		//    	const char* pVal = const_cast<GDALRasterBand*>(GetRasterBand(0))->GetMetadataItem("IMAGE_STRUCTURE");
		//    	//const char* pVal = m_poDataset->GetMetadataItem("COMPRESSION");
		//
		//    	if(pVal!=NULL)
		//    		options.m_createOptions.push_back("COMPRESS="+ string(pVal));
		//    }
	}



	void CGDALDatasetEx::BuildOverviews(const vector<int>& list, bool bQuiet, int CPU)
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
				if (!bQuiet)
					cout << "Build Overview..." << endl;

				GDALProgressFunc pProgressFunc = (bQuiet || GetRasterCount() > 1) ? GDALDummyProgress : GDALTermProgress;

				char** papszOptions = CSLAddString(CSLAddString(NULL, "-co COMPRESS=LZW"), "-co TILED=YES");

				if (m_bMultipleImages)//apply BuildOverviews over Dataset and not over bands
				{
					int ii = 0;
#pragma omp parallel for num_threads( CPU )
					for (int i = 0; i < GetRasterCount(); i++)
					{
						if (m_poDatasetVector[i])
							m_poDatasetVector[i]->BuildOverviews("NEAREST", (int)list.size(), const_cast<int*>(list.data()), 0, NULL, pProgressFunc, NULL, papszOptions);

#pragma omp critical(PrintF)
						{
							if (!bQuiet && GetRasterCount() > 1)
							{
								ii++;
								GDALStyleProgressBar(double(ii) / GetRasterCount());
							}
						}
					}
				}
				else
				{
					Dataset()->BuildOverviews("NEAREST", (int)list.size(), const_cast<int*>(list.data()), 0, NULL, bQuiet? GDALDummyProgress : GDALTermProgress, NULL, papszOptions);
				}

				CPLFree(papszOptions);
				papszOptions = NULL;

				//if (m_bMultipleImages)
				//{
				//	if (!bQuiet)
				//		cout << "Build Overview..." << endl;

				//	for (size_t i = 0; i < m_poDatasetVector.size(); i++)
				//	{
				//		if (m_poDatasetVector[i])
				//		{
				//			//if (!bQuiet)
				//			//cout << FormatA("%-4.4s", FormatA("B%d", i + 1).c_str()) << ": ";

				//			m_poDatasetVector[i]->BuildOverviews("NEAREST", (int)list.size(), const_cast<int*>(list.data()), 0, NULL, bQuiet ? GDALDummyProgress : GDALTermProgress, NULL);
				//		}
				//	}

				//}
				//else
				//{
				//	if (!bQuiet)
				//		cout << "Build Overview: ";

				//	Dataset()->BuildOverviews("NEAREST", (int)list.size(), const_cast<int*>(list.data()), 0, NULL, bQuiet ? GDALDummyProgress : GDALTermProgress, NULL);
				//}
			}
		}
	}

	void CGDALDatasetEx::ComputeStats(bool bQuiet, int CPU)
	{
		if (IsOpen())
		{
			if (!bQuiet)
				cout << "Compute stats..." << endl;

			GDALProgressFunc pProgressFunc = (bQuiet || GetRasterCount() > 1) ? GDALDummyProgress : GDALTermProgress;


			int ii = 0;
#pragma omp parallel for num_threads( CPU )
			for (int i = 0; i < GetRasterCount(); i++)
			{
				if (GetRasterBand(i))
				{
					double dfMin, dfMax, dfMean, dfStdDev = 0;
					GetRasterBand(i)->ComputeStatistics(false, &dfMin, &dfMax, &dfMean, &dfStdDev, pProgressFunc, NULL);
				}

#pragma omp critical(PrintF)
				{
					if (!bQuiet && GetRasterCount() > 1)
					{
						ii++;
						GDALStyleProgressBar(double(ii) / GetRasterCount());
					}
				}
			}

		}
	}

	void CGDALDatasetEx::ComputeHistogram(bool bQuiet, int CPU)
	{
		if (IsOpen())
		{
			if (!bQuiet)
				cout << "Compute histogram..." << endl;

			GDALProgressFunc pProgressFunc = (bQuiet || GetRasterCount() > 1) ? GDALDummyProgress : GDALTermProgress;


			int ii = 0;
#pragma omp parallel for num_threads( CPU )
			for (int i = 0; i < GetRasterCount(); i++)
			{
				if (GetRasterBand(i))
				{
					int nBucketCount = 0;
					GUIntBig* panHistogram = NULL;
					double dfMin = 0, dfMax = 0;
					GetRasterBand(i)->GetDefaultHistogram(&dfMin, &dfMax, &nBucketCount, &panHistogram, TRUE, pProgressFunc, NULL);
					CPLFree(panHistogram);
				}

#pragma omp critical(PrintF)
				{
					if (!bQuiet && GetRasterCount() > 1)
					{
						ii++;
						GDALStyleProgressBar(double(ii) / GetRasterCount());
					}
				}
			}

			for (size_t i = 0; i < GetRasterCount(); i++)
			{
				if (GetRasterBand(i))
				{
					int nBucketCount = 0;
					GUIntBig* panHistogram = NULL;
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

			char** papszMetadata = const_cast<GDALRasterBand*>(pBand)->GetMetadata();
			if (papszMetadata != NULL && *papszMetadata != NULL)
			{
				for (int i = 0; papszMetadata[i] != NULL; i++)
				{
					char* pszKey = NULL;
					const char* pszValue = CPLParseNameValue(papszMetadata[i], &pszKey);
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
		size_t common_begin = 255;//common begin
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
		if (common_begin != 255)
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
	//	class CExtractInfo
	//	{
	//	public:
	//
	//		enum TType { EXTREM, DENSITY, NB_TYPE };
	//
	//		class CInfo
	//		{
	//		public:
	//
	//			double operator[](int i)const { return m_stat[i]; }
	//
	//			CGridPoint m_lowest;
	//			CGridPoint m_highest;
	//			CStatistic m_stat;
	//		};
	//
	//		typedef CMatrix<CInfo> CInfoMatrix;
	//
	//		int m_nbPoint;
	//		double m_factor;
	//		bool m_bExposition;
	//		CGeoExtents m_extents;
	//
	//
	//		CExtractInfo();
	//		ERMsg Execute(CGDALDatasetEx& map, CCallback& callback = DEFAULT_CALLBACK);
	//
	//		int GetNbExtremPoint()const { return GetNbBox(EXTREM) * 2; }
	//		int GetNbBox()const { return GetNbBox(DENSITY); }
	//		void Add(const CGridPoint& pt);
	//
	//		void GetExtremPoints(CGridPointVector& pts)const;
	//
	//		double operator[](int i)const { return m_stat[i]; }
	//		double GetPBase()const;
	//		double GetP(const CGeoPoint& ptIn);
	//
	//	protected:
	//
	//		int GetNbBox(int t)const;
	//		CGeoPointIndex GetPosIndex(const CGeoPoint& ptIn, int t);
	//
	//		int m_nbSub[NB_TYPE];
	//		CInfoMatrix m_info[NB_TYPE];
	//		int m_nbBox;
	//
	//		CStatistic m_stat;//statistic for the entire map.
	//		double m_sumStdDev;
	//	};
	//
	//	ERMsg CGDALDatasetEx::GetRegularCoord(int nbPointLat, int nbPointLon, bool bExpo, const CGeoRect& rect, CGridPointVector& points, CCallback& callback)
	//	{
	//		assert(IsOpen());
	//		assert(nbPointLat > 0 && nbPointLon > 0);
	//
	//		ERMsg msg;
	//
	//		points.clear();
	//
	//		CGeoExtents extents = GetExtents();
	//		CGeoExtents sub_extents = extents;
	//		if (rect.IsInit())
	//		{
	//			((CGeoRect&)sub_extents) = rect;
	//			//if (GetPrjID() != rect.GetPrjID())
	//				//sub_extents.Reproject(GetReProjection(rect.GetPrjID(), GetPrjID()));
	//
	//			sub_extents.IntersectExtents(extents);
	//		}
	//
	//
	//		double x_inc = sub_extents.m_xSize / nbPointLon;
	//		double y_inc = sub_extents.m_ySize / nbPointLat;
	//
	//		std::set<pair<int, int>> indexes;
	//
	//		for (size_t i = 0; i < nbPointLat; i++)
	//		{
	//			for (size_t j = 0; j < nbPointLon; j++)
	//			{
	//				CGeoPointIndex xy(int(x_inc*j), int(y_inc*i));
	//				CGeoPoint pt = sub_extents.XYPosToCoord(xy);
	//				xy = extents.CoordToXYPos(pt);
	//				indexes.insert(make_pair(xy.m_x, xy.m_y));
	//			}
	//		}
	//
	//		CProjectionPtr pPrj = GetPrj();
	//		CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));
	//
	//		Randomize();//init the first time
	//
	//		CBandsHolderMT bandHolder(3);
	//		msg += bandHolder.Load(*this);
	//
	//		if (!msg)
	//			return msg;
	//
	//		vector<pair<int, int>> XYindex = extents.GetBlockList();
	//
	//		string comment = FormatMsg(IDS_MAP_GENERATEREGULAR, m_filePath);
	//		callback.PushTask(comment, XYindex.size());
	//
	//
	//
	//		for (size_t xy = 0; xy < XYindex.size() && msg; xy++)//for all blocks
	//		{
	//			int xBlock = XYindex[xy].first;
	//			int yBlock = XYindex[xy].second;
	//			CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
	//			bandHolder.LoadBlock(blockExtents);
	//
	//			CDataWindowPtr window = bandHolder.GetWindow(0);
	//
	//			//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
	//			for (int y = 0; y < blockExtents.m_ySize&&msg; y++)
	//			{
	//				for (int x = 0; x < blockExtents.m_xSize&&msg; x++)
	//				{
	//					if (window->IsValid(x, y))
	//					{
	//						CGeoPointIndex xy(x, y);
	//						CGeoPoint pt = blockExtents.XYPosToCoord(xy);
	//						xy = extents.CoordToXYPos(pt);
	//
	//						auto it = indexes.find(make_pair(xy.m_x, xy.m_y));
	//
	//						if (it != indexes.end())
	//						{
	//							CGridPoint pt;
	//							((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
	//							pt.m_z = window->at(x, y);
	//							if (bExpo)
	//								window->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);
	//
	//							if (pPrj->IsProjected())
	//							{
	//								CGeoPoint ptGeo(pt);
	//								ptGeo.Reproject(PT);
	//								pt.m_latitudeDeg = ptGeo.m_y;
	//							}
	//
	//							points.push_back(pt);
	//							msg += callback.StepIt(0);
	//						}
	//
	//					}
	//				}
	//			}
	//
	//			msg += callback.StepIt();
	//		}
	//
	//		callback.PopTask();
	//
	//		return msg;
	//	}
	//
	//
	//	//****************************************************************************
	//	// Sommaire:    Génère un LOC à partir du DEM.
	//	//
	//	// Description: GetRandomCoord crée un LOC de points aléatoires.
	//	//
	//	// Entrée:      int nbPoint: le nombres de stations que l'on veut dans le LOC
	//	//              bool bExp:  true: donne l'exposition
	//	//                          false: met 0.
	//	//
	//	//
	//	// Sortie:      int: MAP_SUCCESS ou code d'erreur(voir mapping.h)
	//	//              LOCArray& locArray: un LOC
	//	//
	//	// Note:        Le nombre de station dans le LOC == nbPoint.
	//	//****************************************************************************
	//
	//
	//
	//	ERMsg CGDALDatasetEx::GetRandomCoord(int nbPoint, bool bExpo, bool bExtrem, double factor, const CGeoRect& rect, CGridPointVector& locations, CCallback& callback)
	//	{
	//		assert(IsOpen());
	//		assert(nbPoint > 0);
	//
	//		ERMsg msg;
	//
	//		CGridPointVector locArrayTmp;
	//		locArrayTmp.reserve(nbPoint);
	//		locations.reserve(nbPoint);
	//		CGeoExtents extents = GetExtents();
	//		if (rect.IsInit())
	//		{
	//			((CGeoRect&)extents) = rect;
	//			if (GetPrjID() != rect.GetPrjID())
	//				extents.Reproject(GetReProjection(rect.GetPrjID(), GetPrjID()));
	//		}
	//
	//
	//		size_t nbPixels = extents.GetNbPixels();
	//		double Pbase = (double)nbPoint / nbPixels;
	//
	//		CExtractInfo info;
	//		info.m_nbPoint = nbPoint;
	//		info.m_factor = factor;
	//		info.m_bExposition = bExpo;
	//		info.m_extents = extents;
	//
	//		if (bExtrem || factor > 0)
	//		{
	//			msg = info.Execute(*this, callback);
	//			if (msg && info[NB_VALUE] > 0)
	//			{
	//				callback.AddMessage("NbCells = " + ToString(info[NB_VALUE]));
	//
	//				if (bExtrem)
	//					callback.AddMessage("NbExtremPoint = " + ToString(info.GetNbExtremPoint()));
	//				callback.AddMessage("NbBox = " + ToString(info.GetNbBox()));
	//
	//				Pbase = info.GetPBase();
	//			}
	//		}
	//
	//		if (!msg)
	//			return msg;
	//
	//		int nbRun = 0;
	//		CProjectionPtr pPrj = GetPrj();
	//		CProjectionTransformation PT(pPrj, CProjectionManager::GetPrj(PRJ_WGS_84));
	//
	//		const auto& start = std::chrono::high_resolution_clock::now();
	//		//	high_resolution_clock::now();
	//
	//		Randomize();//init the first time
	//		// loop until the good number of points
	//		while ((locations.size() + locArrayTmp.size() + info.GetNbExtremPoint()) < nbPoint && msg)
	//		{
	//			//m_extents = GetExtents();
	//			string comment = FormatMsg(IDS_MAP_GENERATERANDOM, m_filePath);
	//			callback.PushTask(comment, nbPixels);
	//
	//			CBandsHolderMT bandHolder(3);
	//			msg += bandHolder.Load(*this);
	//
	//			if (!msg)
	//				return msg;
	//
	//			vector<pair<int, int>> XYindex = extents.GetBlockList();
	//
	//			for (int xy = 0; xy < (int)XYindex.size() && msg; xy++)//for all blocks
	//			{
	//				int xBlock = XYindex[xy].first;
	//				int yBlock = XYindex[xy].second;
	//				CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
	//				bandHolder.LoadBlock(blockExtents);
	//
	//				CDataWindowPtr window = bandHolder.GetWindow(0);
	//
	//				//#pragma omp parallel for schedule(static, 100) num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
	//				for (int y = 0; y < blockExtents.m_ySize; y++)
	//				{
	//					for (int x = 0; x < blockExtents.m_xSize; x++)
	//					{
	//						if (window->IsValid(x, y))
	//						{
	//							CGridPoint pt;
	//							((CGeoPoint&)pt) = blockExtents.XYPosToCoord(CGeoPointIndex(x, y));
	//							pt.m_z = window->at(x, y);
	//							if (bExpo)
	//								window->GetSlopeAndAspect(x, y, pt.m_slope, pt.m_aspect);
	//
	//							//randomize point coordinate in the cell for kriging
	//							pt.m_x += Rand(-0.5, 0.5)*m_extents.XRes();
	//							pt.m_y += Rand(-0.5, 0.5)*m_extents.YRes();
	//
	//							double p = info.GetP(pt)*Pbase;
	//							if (Randu() < p)
	//							{
	//								if (pPrj->IsProjected())
	//								{
	//									CGeoPoint ptGeo(pt);
	//									ptGeo.Reproject(PT);
	//									pt.m_latitudeDeg = ptGeo.m_y;
	//								}
	//
	//								locations.push_back(pt);
	//							}
	//							else if (locations.size() < nbPoint && Randu() < 2 * p)//all can have a second chance
	//							{
	//								if (pPrj->IsProjected())
	//								{
	//									CGeoPoint ptGeo(pt);
	//									ptGeo.Reproject(PT);
	//									pt.m_latitudeDeg = ptGeo.m_y;
	//								}
	//
	//								locArrayTmp.push_back(pt);
	//							}
	//						}
	//
	//						msg += callback.StepIt();
	//					}
	//				}
	//			}
	//
	//			const auto& now = std::chrono::high_resolution_clock::now();
	//			//const auto& stop = high_resolution_clock::now();
	//			auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
	//
	//
	//			//if (nbRun++ > 10)
	//			if(duration>600)
	//			{   // si le nombre de tour est > 10, on à un problème
	//				//unable to found points
	//				msg.ajoute(GetString(IDS_MAP_UNABLE_CREATE_LOC));
	//				break;
	//			}
	//
	//			callback.AddMessage("NbPoint = " + ToString(locations.size() + locArrayTmp.size()));
	//			callback.PopTask();
	//
	//
	//		}//while
	//
	//		if (msg)
	//		{
	//			//complete locArrayTmp1
	//			size_t nbPointTmp = locations.size() + info.GetNbExtremPoint();
	//			if (nbPointTmp < nbPoint)
	//			{
	//				size_t needed = nbPoint - nbPointTmp;
	//				for (size_t i = 0; i < needed; i++)
	//				{
	//					size_t index = (size_t)Rand(0, int(locArrayTmp.size()) - 1);
	//					assert(index < locArrayTmp.size());
	//					locations.push_back(locArrayTmp[index]);
	//					locArrayTmp.erase(locArrayTmp.begin() + index);
	//				}
	//			}
	//			else
	//			{
	//				size_t extra = nbPointTmp - nbPoint;
	//				for (size_t i = 0; i < extra; i++)
	//				{
	//					size_t index = (size_t)Rand(0, int(locations.size()) - 1);
	//					assert(index < locations.size());
	//					locations.erase(locations.begin() + index);
	//				}
	//			}
	//
	//			info.GetExtremPoints(locations);
	//			assert(locations.size() == nbPoint);
	//		}
	//
	//		//classify.ClassifyNaturalBreak(5);
	//
	//		//CStdioFile file( "c:\\temp\\testLoc.csv", CFile::modeCreate|CFile::modeWrite);
	//		//for(int i=0; i<classify.GetSize(); i++)
	//		//{
	//		//	string line;
	//		//	line.Format( "%lf,%lf,%lf\n", classify[i].m_x[LOWEST], classify[i].m_x[HIGHEST], classify[i].m_x[MEAN]);
	//		//	file.WriteString(line);
	//		//}
	//		//file.Close();
	//
	//
	//		return msg;
	//	}


	CGeoSize CGDALDatasetEx::ComputeBlockSize(double memoryLimit, const CGeoExtents& extents)const
	{
		//1- Get block size
		CGeoSize meanSize;
		int nbRaster = 0;

		//	bool bTemporal = !m_scenesPeriod.empty() && period.IsInit();

		for (size_t i = 0; i < GetRasterCount(); i++)
		{
			if (extents.IsRectIntersect(m_internalExtents[i]))
			{
				//		size_t sceneNo = bTemporal ? i / GetSceneSize() : UNKNOWN_POS;
				//	if (!bTemporal || (m_scenesPeriod[sceneNo].IsInit() && period.IsIntersect(m_scenesPeriod[sceneNo])))
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
		int nbLayerMax = max(1, GetMaxBlockSizeZ(blockManager));

		//4- Evaluate block size int respect of memroy cache
		double rxy = (double)blockManager.m_xBlockSize / blockManager.m_yBlockSize;
		double yBlockSize = sqrt(memoryLimit / (sizeof(DataType) * rxy * nbLayerMax));
		double xBlockSize = rxy * yBlockSize;
		CGeoSize blockSize(max(1, min(extents.m_xSize, (int)ceil(xBlockSize))), max(1, min(extents.m_ySize, (int)ceil(yBlockSize))));

		//double memMax = (double)blockSize.m_x*blockSize.m_y*nbLayerMax * sizeof(DataType);

		//m_extents.SetBlockSize(blockSize);
		return blockSize;
	}


	int CGDALDatasetEx::GetMaxBlockSizeZ(CGeoExtents extents)const
	{
		//		bool bTemporal = !m_scenesPeriod.empty() && period.IsInit();
		//		CTTransformation TT(period, TM);
		//
		//		size_t nbScene = bTemporal ? GetNbScenes() : GetRasterCount();
		//		size_t sceneSize = bTemporal ? GetSceneSize() : 1;
		//		size_t nbCluster = bTemporal ? TT.GetNbCluster() : 1;
		//		CStatisticVector nbRaster(nbCluster);
		//
		//		for (size_t k = 0; k < nbScene; k++)
		//		{
		//			if (extents.IsRectIntersect(m_internalExtents[k]))
		//			{
		//				for (size_t i = 0; i < nbCluster; i++)
		//				{
		//					CTPeriod p = bTemporal ? m_scenesPeriod[k] : CTPeriod();
		//					p.Transform(TM);
		//					if (!bTemporal || p.IsInside(TT.GetClusterTRef(i)))
		//					{
		//						nbRaster[i] += sceneSize;
		//					}
		//				}
		//			}
		//		}
		//
		//		CStatistic nbRasterAll;
		//		for (int l = 0; l < nbCluster; l++)
		//			if (nbRaster[l][NB_VALUE] > 0)
		//				nbRasterAll += nbRaster[l][SUM];
		//
		//		int nbRasterMax = nbRasterAll.IsInit() ? (int)nbRasterAll[HIGHEST] : 0;

		int nbRasterMax = int(m_internalExtents.size());
		assert(false);
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

	//**********************************************************************************
	//CGDALDatasetEx
	void CDataWindow::GetSlopeAndAspect(int x, int y, double& slope, double& aspect)const
	{
		slope = aspect = -999;

		if (!m_data.empty())
		{
			assert(IsValid(x, y));

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

			double ewres = m_extents.XRes();
			double nsres = m_extents.YRes();
			if (m_extents.IsGeographic())
			{
				//compute local resolution in meters
				CGeoExtents pixel_extents = m_extents.GetPixelExtents(CGeoPointIndex(x, y));
				ewres = pixel_extents.CenterLeft().GetDistance(pixel_extents.CenterRight());
				nsres = -pixel_extents.UpperMidle().GetDistance(pixel_extents.LowerMidle());//sort north resolution is negative
			}

			WBSF::GetSlopeAndAspect(window, ewres, nsres, 1, slope, aspect);
		}
	}


	CStatisticEx CDataWindow::GetWindowStat(int x, int y, int n_rings)const
	{
		CStatisticEx stat;
		for (int yy = -n_rings; yy <= n_rings; yy++)
		{
			for (int xx = -n_rings; xx <= n_rings; xx++)
			{
				if (IsInside(x + xx, y + yy))
				{
					if (IsValid(x + xx, y + yy))
					{
						stat += at(x + xx, y + yy);
					}
				}
			}
		}

		return stat;
	}
}
