//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************


//"D:\Travaux\Landsat\Landsat(2000-2018)\Input\Landsat_2000-2018(2).vrt" "D:\Travaux\Landsat\Landsat(2000-2018)\Output\test3.vrt" -of VRT -overwrite -co "COMPRESS=LZW"   -te 1022538.9 6663106.0 1040929.5 6676670.7 -multi -SpikeThreshold 0.75


#include <boost/filesystem.hpp>


#include <cmath>
#include <array>
#include <utility>
#include <iostream>



#include "basic/OpenMP.h"
#include "geomatic/GDAL.h"
#include "geomatic/LandTrendUtil.h"
#include "geomatic/LandTrendCore.h"


#include "DesawtoothDirect.h"



using namespace std;
using namespace WBSF::Landsat2;
using namespace LTR;



namespace WBSF
{
	const size_t CDesawtoothDirect::NB_THREAD_PROCESS = 2;
	//*********************************************************************************************************************

	ERMsg CDesawtoothDirect::Execute()
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
		{
			cout << "Output: " << m_options.m_filesPath[CDesawtoothOption::OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[CDesawtoothOption::INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;

		}


		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;
		CGDALDatasetEx cloudsDS;

		msg = OpenAll(inputDS, maskDS, cloudsDS, outputDS);
		if (!msg)
			return msg;


		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images (" << outputDS.GetRasterXSize() << " C x " << outputDS.GetRasterYSize() << " R x " << outputDS.GetRasterCount() << " B) with " << m_options.m_CPU << " threads..." << endl;

		CGeoExtents extents = m_options.m_extents;
		m_options.ResetBar((size_t)extents.m_xSize * extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();
		map<int, bool> treadNo;

		omp_set_nested(1);//for IOCPU
		#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			CRasterWindow inputData;
			OutputData outputData;

			ReadBlock(inputDS, cloudsDS, xBlock, yBlock, inputData);
			ProcessBlock(xBlock, yBlock, inputData, outputData);
			WriteBlock(xBlock, yBlock, outputDS, outputData);
		}//for all blocks

		CloseAll(inputDS, maskDS, outputDS);

		return msg;
	}



	ERMsg CDesawtoothDirect::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& cloudsDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[CDesawtoothOption::INPUT_FILE_PATH], m_options);

		if (msg)
		{
			inputDS.UpdateOption(m_options);

			if (!m_options.m_bQuiet)
			{
				CGeoExtents extents = inputDS.GetExtents();
				//            CProjectionPtr pPrj = inputDS.GetPrj();
				//            string prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
				//          cout << "    Projection     = " << prjName << endl;
				cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
				//cout << "    Scene size     = " << inputDS.GetSceneSize() << endl;
				//cout << "    Nb. Scenes     = " << inputDS.GetNbScenes() << endl;

				if (inputDS.GetRasterCount() < 2)
					msg.ajoute("Desawtooth need at least 2 bands");
			}
		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg && !m_options.m_CloudsMask.empty())
		{
			if (!m_options.m_bQuiet)
				cout << "Open clouds image..." << endl;
			msg += cloudsDS.OpenInputImage(m_options.m_CloudsMask);
			if (msg)
			{
				cout << "clouds Image Size      = " << cloudsDS->GetRasterXSize() << " cols x " << cloudsDS->GetRasterYSize() << " rows x " << cloudsDS.GetRasterCount() << " bands" << endl;

				if (cloudsDS.GetRasterCount() != inputDS.GetNbScenes())
					msg.ajoute("Invalid clouds image. Number of bands in clouds image (+" + to_string(cloudsDS.GetRasterCount()) + ") is not equal the number of scenes (" + to_string(inputDS.GetNbScenes()) + ") of the input image.");

				if (cloudsDS.GetRasterXSize() != inputDS.GetRasterXSize() ||
					cloudsDS.GetRasterYSize() != inputDS.GetRasterYSize())
					msg.ajoute("Invalid clouds image. Image size must have the same size (x and y) than the input image.");
			}
		}

		if (msg && m_options.m_bCreateImage)
		{
			size_t nb_bands = m_options.m_scene_extents[1] - m_options.m_scene_extents[0] + 1;
			CDesawtoothOption options(m_options);
			options.m_scenes_def.clear();
			options.m_nbBands = nb_bands;

			if (!m_options.m_bQuiet)
			{
				cout << endl;
				cout << "Open output images..." << endl;
				cout << "    Size           = " << options.m_extents.m_xSize << " cols x " << options.m_extents.m_ySize << " rows x " << options.m_nbBands << " bands" << endl;
				cout << "    Extents        = X:{" << ToString(options.m_extents.m_xMin) << ", " << ToString(options.m_extents.m_xMax) << "}  Y:{" << ToString(options.m_extents.m_yMin) << ", " << ToString(options.m_extents.m_yMax) << "}" << endl;
				cout << "    NbBands        = " << options.m_nbBands << endl;
				//cout << "    Nb. Scenes     = " << nb_scenes << endl;
			}

			std::string indices_name = Landsat2::GetIndiceName(m_options.m_indice);
			string filePath = options.m_filesPath[CDesawtoothOption::OUTPUT_FILE_PATH];

			//replace the common part by the new name
			for (size_t zz = 0; zz < nb_bands; zz++)
			{
				size_t z = m_options.m_scene_extents[0] + zz;
				string subName = FormatA("%02d", z + 1);//inputDS.GetSubname(z);// +"_" + indices_name;
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";

			}

			msg += outputDS.CreateImage(filePath, options);
		}

		return msg;
	}

	void CDesawtoothDirect::ReadBlock(CGDALDatasetEx& inputDS, CGDALDatasetEx& cloudsDS, int xBlock, int yBlock, CRasterWindow& block_data)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerRead.start();

			CGeoExtents extents = m_options.m_extents.GetBlockExtents(xBlock, yBlock);
			inputDS.ReadBlock(extents, block_data, int(ceil(m_options.m_rings)), m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);

			if (cloudsDS.IsOpen())
			{
				assert(cloudsDS.GetRasterCount() == inputDS.GetNbScenes());
				assert(cloudsDS.GetRasterXSize() * cloudsDS.GetRasterYSize() == inputDS.GetRasterXSize() * inputDS.GetRasterYSize());


				CRasterWindow clouds_block;
				cloudsDS.ReadBlock(extents, clouds_block, int(ceil(m_options.m_rings)), m_options.m_IOCPU, m_options.m_scene_extents[0], m_options.m_scene_extents[1]);
				assert(block_data.size() == clouds_block.size());
				DataType cloudNoData = (DataType)cloudsDS.GetNoData(0);

				for (size_t i = 0; i < clouds_block.size(); i++)
				{
					assert(block_data[i].data().size() == clouds_block[i].data().size());

					boost::dynamic_bitset<> validity(clouds_block[i].data().size(), true);

					for (size_t xy = 0; xy < clouds_block[i].data().size(); xy++)
						validity.set(xy, clouds_block[i].data()[xy] == 0 || clouds_block[i].data()[xy] == cloudNoData);

					//set this validity for all scene bands
					assert(validity.size() == block_data[i].data().size());
					for (size_t ii = 0; ii < block_data.GetSceneSize(); ii++)
						block_data.at(i * block_data.GetSceneSize() + ii).SetValidity(validity);
				}
			}

			m_options.m_timerRead.stop();
		}
	}



	void CDesawtoothDirect::ProcessBlock(int xBlock, int yBlock, const CRasterWindow& window, OutputData& outputData)
	{
		CGeoExtents extents = m_options.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

		if (window.empty())
		{
			int nbCells = blockSize.m_x * blockSize.m_y;

#pragma omp atomic
			m_options.m_xx += nbCells;
			m_options.UpdateBar();

			return;
		}



		//init memory
		if (m_options.m_bCreateImage)
		{
			outputData.resize(window.GetNbScenes());
			for (size_t s = 0; s < outputData.size(); s++)
				outputData[s].insert(outputData[s].begin(), blockSize.m_x * blockSize.m_y, m_options.m_dstNodata);
		}



#pragma omp critical(ProcessBlock)
		{
			m_options.m_timerProcess.start();


			#pragma omp parallel for num_threads( m_options.m_CPU ) if (m_options.m_bMulti )
			for (int y = 0; y < blockSize.m_y; y++)
			{
				for (int x = 0; x < blockSize.m_x; x++)
				{
					int xy = y * blockSize.m_x + x;

					//Get pixel
					CRealArray years = ::convert(allpos(window.size()));
					CRealArray data(window.size());
					CBoolArray goods(window.size());

					size_t m_first_valid = NOT_INIT;
					size_t m_last_valid = NOT_INIT;
					if (m_options.m_bBackwardFill || m_options.m_bForwardFill)
					{
						for (size_t z = 0; z < window.size(); z++)
						{
							bool bValid = window.IsValid(z, x, y);
							if (m_options.m_bBackwardFill && bValid && m_first_valid == NOT_INIT)
								m_first_valid = z;

							if (m_options.m_bForwardFill && bValid)
								m_last_valid = z;
						}
					}


					for (size_t z = 0; z < window.size(); z++)
					{
						size_t zz = z;

						if (m_first_valid != NOT_INIT && zz < m_first_valid)
							zz = m_first_valid;
						if (m_last_valid != NOT_INIT && zz > m_last_valid)
							zz = m_last_valid;


						goods[z] = window.IsValid(zz, x, y);

						if (goods[z])
						{
							CStatistic stat = window[zz].GetWindowStat(x, y, int(m_options.m_rings));//Note: pas la même définition de ring que dans landsat!
							assert(stat[NB_VALUE] > 0);
							data[z] = stat[MEAN];
						}
					}

					size_t nbVal = sum(goods);
					if (nbVal > m_options.m_minneeded)//at least one valid pixel
					{
						
						if (m_options.m_year_by_year)
						{
							assert(m_options.m_desawtooth_val < 1.0);

							for (size_t z = 0; z < window.size(); z++)
							{
								if (goods[z])
								{

									CRealArray data3(3);
									CBoolArray goods3(false, 3);

									data3[1] = data[z];
									goods3[1] = true;
									for (size_t zz = z - 1; zz < goods.size() && !goods3[0]; zz--)
									{
										if (goods[zz])
										{
											goods3[0] = true;
											data3[0] = data[zz];
										}
									}

									if (!goods3[0])
									{
										goods3[0] = true;
										data3[0] = data3[1];
									}

									for (size_t zz = z + 1; zz < goods.size() && !goods3[2]; zz++)
									{
										if (goods[zz])
										{
											goods3[2] = true;
											data3[2] = data[zz];
										}
									}

									if (!goods3[2])
									{
										goods3[2] = true;
										data3[2] = data3[1];
									}

									CRealArray output_corr_factore(data3.size());
									CRealArray all_y = desawtooth(data3, goods3, m_options.m_desawtooth_val, &output_corr_factore);
									assert(all_y.size() == data3.size());
									double val = max(GetTypeLimit(m_options.m_outputType, true), min(GetTypeLimit(m_options.m_outputType, false), output_corr_factore[1]));
									outputData[z][xy] = val;
								}
							}

						}
						else
						{
							CRealArray output_corr_factore(data.size());

							assert(m_options.m_desawtooth_val < 1.0);
							CRealArray all_y = desawtooth(data, goods, m_options.m_desawtooth_val, &output_corr_factore);
							assert(all_y.size() == window.size());


							for (size_t z = 0; z < window.size(); z++)
							{
								if (goods[z])
								{
									double val = max(GetTypeLimit(m_options.m_outputType, true), min(GetTypeLimit(m_options.m_outputType, false), output_corr_factore[z]));
									outputData[z][xy] = val;
								}
							}
						}//if year_by_year

					}//if min needed valid pixel

#pragma omp atomic
					m_options.m_xx++;

				}//for x
				m_options.UpdateBar();
			}//for y

			m_options.m_timerProcess.stop();

		}//critical process
	}


	void CDesawtoothDirect::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData)
	{
#pragma omp critical(BlockIO)
		{
			m_options.m_timerWrite.start();

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
					if (!outputData.empty())
					{
						assert(outputData.size() == outputDS.GetRasterCount());
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(outputData[z][0]), outputRect.Width(), outputRect.Height(), GDT_Float64, 0, 0);
					}
					else
					{
						double noData = outputDS.GetNoData(z);
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noData, 1, 1, GDT_Float64, 0, 0);
					}
				}
			}




			m_options.m_timerWrite.stop();
		}
	}

	void CDesawtoothDirect::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.m_timerWrite.start();

		outputDS.Close(m_options);

		m_options.m_timerWrite.stop();

	}

}
