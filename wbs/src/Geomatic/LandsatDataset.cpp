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
#include <algorithm>
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

#include "Geomatic/LandsatDataset.h"

using namespace std;
using namespace WBSF::Landsat;

namespace WBSF
{




	const char* CLandsatDataset::SCENE_NAME[SCENES_SIZE] = { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "QA", "JD" };

	CTRef GetByName(const string& fielpath)
	{
		CTRef TRef;
		string fileTitle = GetFileTitle(fielpath);
		if (fileTitle.length() >= 16)
		{
			size_t landsat = size_t(fileTitle[2] - '0');
			if (landsat == 5 || landsat == 7 || landsat == 8)
			{
				int year = ToInt(fileTitle.substr(9, 4));
				size_t Jday = ToSizeT(fileTitle.substr(13, 3));
				if (year >= 1950 && year <= 2050 && Jday >= 1 && Jday <= 366)
					TRef = CJDayRef(year, Jday - 1);
			}
		}

		return TRef;
	}


	TIndices Landsat::GetIndicesType(const std::string& str)
	{
		static const char* TYPE_NAME[NB_INDICES] = { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "QA", "JD", "NBR", "Euclidean", "NDVI", "NDMI", "TCB", "TCG", "TCW" };
		size_t type = UNKNOWN_POS;
		for (size_t i = 0; i < NB_INDICES&&type == UNKNOWN_POS; i++)
			if (IsEqualNoCase(str, TYPE_NAME[i]))
				type = i;

		return (TIndices)type;
	}

	TMethod Landsat::GetIndicesMethod(const std::string& str)
	{
		static const char* MODE_NAME[NB_INDICES] = { "OR", "AND"};
		size_t type = UNKNOWN_POS;
		for (size_t i = 0; i < NB_INDICES&&type == UNKNOWN_POS; i++)
			if (IsEqualNoCase(str, MODE_NAME[i]))
				type = i;

		return (TMethod)type;
	}

	ERMsg CLandsatDataset::OpenInputImage(const std::string& filePath, const CBaseOptions& options)
	{
		ERMsg msg = CGDALDatasetEx::OpenInputImage(filePath, options);
		if (msg)
		{
			//temporal section
			ASSERT(options.m_scenesSize == SCENES_SIZE);

			if ((GetRasterCount() % options.m_scenesSize) == 0)
			{
				bool bFindPeriod=false;
				size_t nbScenes = size_t(GetRasterCount() / options.m_scenesSize);
				m_scenesPeriod.resize(nbScenes);
				for (size_t s = 0; s < nbScenes; s++)
				{
					CTPeriod period;

					//try to identify by name
					if (IsVRT())
					{
						CTRef TRef = GetByName(GetInternalName(s*options.m_scenesSize));
						if (TRef.IsInit())
							period = CTPeriod(TRef, TRef);
					}

					if (!period.IsInit())
					{
						//try to find by the date layers
						double TRefMin = 0;
						double TRefMax = 0;
						if (GetRasterBand(s * options.m_scenesSize + JD)->GetStatistics(true, true, &TRefMin, &TRefMax, NULL, NULL) == CE_None)
						{
							period.Begin() = options.GetTRef(int(TRefMin));
							period.End() = options.GetTRef(int(TRefMax));
						}
					}

					if (period.IsInit())
					{
						bFindPeriod = true;
						m_scenesPeriod[s] = period;
					}
						
				}//for all scenes

				if (!bFindPeriod)
				{
					msg.ajoute("ERROR: Unable to get temporal reference from images");
				}
			}
			else
			{
				msg.ajoute("ERROR: input image bands (" + ToString(GetRasterCount()) + ") count must be a multiple of temporal information (" + ToString(options.m_scenesSize) + ")");
			}
		}
		//m_bandMetaData.resize(GetNbScenes());

		//char** papszFileList = m_poDataset->GetFileList();
		//int fileCount = CSLCount(papszFileList);
		////if (fileCount >== GetRasterCount())
		////{
		//	for (int i = 0; i<GetNbScenes(); i++)
		//	{
		//		size_t fileIndex = i*GetSceneSize() + 1;
		//		if (fileIndex<fileCount)
		//		{
		//			string title = GetFileTitle(papszFileList[fileIndex]);
		//			if (title.length() >= 9)
		//			{
		//				SetImageMetaData(i, "Path", title.substr(3, 3));
		//				SetImageMetaData(i, "Row", title.substr(6, 3));
		//			}
		//		}
		//	}
		////}

		//CSLDestroy(papszFileList);
		//}

		return msg;
	}

	void CLandsatDataset::GetBandsHolder(CBandsHolder& bandsHoler)const
	{
		CGDALDatasetEx::GetBandsHolder(bandsHoler);

		//don't load date when the set the possibility of 
		//for (int i = 0; i < GetNbScenes(); i++)
		//bandsHoler[i*NB_BAND_PER_IMAGE + CMergeImagesOption::I_IMAGE_DATE]->m_b;
	}

	void CLandsatDataset::UpdateOption(CBaseOptions& option)const
	{
		//CMergeImagesOption& option = dynamic_cast<CMergeImagesOption&>(optionIn);

		CGDALDatasetEx::UpdateOption(option);

		if (!option.m_period.IsInit())
		{
			option.m_period = GetPeriod();
			//expand period over the year
			option.m_period.Begin().SetJDay(0);
			option.m_period.End().SetJDay(LAST_DAY);
		}


		//compute number of output bands
		//if (option.m_TM.Type() == CTM::DAILY)
		//{
		//copy scenes period
		//option.m_scenesPeriod = GetScenePeriod();
		//}

	}

	CLandsatWindow::CLandsatWindow() : CRasterWindow(SCENES_SIZE)
	{}

	CLandsatPixel CLandsatWindow::GetPixel(size_t i, int x, int y)const
	{
		CLandsatPixel pixel;
		for (size_t z = 0; z < SCENES_SIZE; z++)
		{
			size_t ii = i*SCENES_SIZE + z;
			pixel[z] = (LandsatDataType)at(ii)->at(x, y);
		}



		return pixel;
	}

	//****************************************************************************************************************


	CLandsatPixel::CLandsatPixel()
	{
		__int16 noData = (__int16)WBSF::GetDefaultNoData(GDT_Int16);
		fill(noData);
	}

	bool CLandsatPixel::IsInit()const
	{
		__int16 noData = (__int16)WBSF::GetDefaultNoData(GDT_Int16);

		bool bIsInit = false;
		for (size_t z = 0; z < SCENES_SIZE&&!bIsInit; z++)
			bIsInit |= at(z) != noData;

		return bIsInit;
	}

	CTRef CLandsatPixel::GetTRef()const
	{
		CTRef TRef;
		if (at(JD) != (__int16)WBSF::GetDefaultNoData(GDT_Int16))
			TRef = CBaseOptions::GetTRef(CBaseOptions::JDAY1970, at(JD));

		return TRef;
	}


	bool CLandsatPixel::IsValid()const
	{
		__int16 noData = (__int16)WBSF::GetDefaultNoData(GDT_Int16);

		bool bIsInit = true;
		for (size_t z = 0; z < SCENES_SIZE&&bIsInit; z++)
			bIsInit = at(z) != noData;

		return bIsInit;
	}

	double CLandsatPixel::GetCloudRatio()const
	{
		return at(B6) != 0 ? (double)at(B1) / at(B6) : 99999;
	}

	double CLandsatPixel::GetEuclideanDistance(const CLandsatPixel& pixel)const
	{
		return sqrt(
			Square((double)pixel[B3] - at(B3)) +
			Square((double)pixel[B4] - at(B4)) +
			Square((double)pixel[B5] - at(B5))
			);
	}


	double CLandsatPixel::NBR()const
	{
		return (at(B4) + at(B7)) != 0 ? ((double)at(B4) - at(B7)) / (at(B4) + at(B7)) : -32768;
	}


	double CLandsatPixel::NDVI()const
	{
		return (at(B4) + at(B3)) != 0 ? ((double)at(B4) - at(B3)) / (at(B4) + at(B3)) : -32768;
	}

	double CLandsatPixel::NDMI()const
	{
		return (at(B4) + at(B5)) != 0 ? ((double)at(B4) - at(B5)) / (at(B4) + at(B5)) : -32768;
	}

	double CLandsatPixel::TCB()const
	{
		static const double F[7] = { 0.2043, 0.4158, 0.5524, 0.5741, 0.3124, 0.0000, 0.2303 };
		return F[B1] * at(B1) + F[B2] * at(B2) + F[B3] * at(B3) + F[B4] * at(B4) + F[B5] * at(B5) + F[B7] * at(B7);
	}

	double CLandsatPixel::TCG()const
	{
		static const double F[7] = { -0.1603, -0.2819, -0.4934, 0.7940, 0.0002, 0.000, -0.1446 };
		return F[B1] * at(B1) + F[B2] * at(B2) + F[B3] * at(B3) + F[B4] * at(B4) + F[B5] * at(B5) + F[B7] * at(B7);
	}

	double CLandsatPixel::TCW()const
	{
		static const double F[7] = { 0.0315, 0.2021, 0.3102, 0.1594, 0.6806, 0.000, -0.6109 };
		return F[B1] * at(B1) + F[B2] * at(B2) + F[B3] * at(B3) + F[B4] * at(B4) + F[B5] * at(B5) + F[B7] * at(B7);
	}

	double CLandsatPixel::GetDespike(double pre, double spike, double post)
	{
		if (spike - ((post + pre) / 2) == 0)
			return 1;

		double diff1 = (post - pre);
		double diff2 = spike - (post + pre) / 2;
		return fabs(diff1 / diff2);
		//return fabs((post.NBR() - pre.NBR()) / (NBR() - ((post.NBR() + pre.NBR())/2)));
	}

	//double CLandsatPixel::GetDespike(const CLandsatPixel& pre, const CLandsatPixel& post)const
	//{
	//	if (NBR() - ((post.NBR() + pre.NBR()) / 2) == 0)
	//		return 1;
	//
	//	double test1 = (post.NBR() - pre.NBR())*1000;
	//	double test2 = NBR() * 1000;
	//	double test3 = (post.NBR() + pre.NBR()) / 2 * 1000;
	//	return fabs(test1 / (test2 - test3));
	//	//return fabs((post.NBR() - pre.NBR()) / (NBR() - ((post.NBR() + pre.NBR())/2)));
	//}

}