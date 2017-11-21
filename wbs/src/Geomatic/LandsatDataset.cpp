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
#include "basic/zenXml.h"


using namespace std;


namespace WBSF
{


	const char* Landsat::GetSceneName(size_t s)
	{
		static const char* SCENE_NAME[SCENES_SIZE] = { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "QA", "JD" };
		ASSERT(s < SCENES_SIZE);
		return SCENE_NAME[s];

	}



	Landsat::TLandsatFormat Landsat::GetFormatFromName(const string& title)
	{
		TLandsatFormat format = F_UNKNOWN;
		if (!title.empty())
		{
			if (title.size() >= 25 && title[2] == '0')
			{
				char captor = char(title[3] - '0');
				if (captor == 4 || captor == 5 || captor == 7 || captor == 8)
					format = F_NEW;
			}
			else if (title.size() >= 9)
			{
				char captor = char(title[2] - '0');
				if (captor == 4 || captor == 5 || captor == 7 || captor == 8)
					format = F_OLD;
			}
		}

		return format;
	}

	__int16 Landsat::GetCaptorFromName(const string& title)
	{
		__int16 captor = -32768;
		if (!title.empty())
		{
			if (title.size() >= 4)
			{
				if (title[2] == '0')
					captor = char(title[3] - '0');
				else
					captor = char(title[2] - '0');

				if (captor != 4 && captor != 5 && captor != 7 && captor != 8)
					captor = -32768;
			}
		}

		return captor;
	}

	CTRef Landsat::GetTRefFromName(const string& title)
	{
		CTRef TRef;

		TLandsatFormat format = GetFormatFromName(title);
		if (format == F_OLD)
		{

			//LC80130262016186LGN00_B1
			int year = ToInt(title.substr(9, 4));
			size_t Jday = ToSizeT(title.substr(13, 3));
			if (year >= 1950 && year <= 2050 && Jday >= 1 && Jday <= 366)
				TRef = CJDayRef(year, Jday - 1);
		}
		else if (format == F_NEW)
		{
			//LC08_L1TP_013026_20170723_20170809_01_T1_B1
			int year = ToInt(title.substr(17, 4));
			size_t month = ToSizeT(title.substr(21, 2));
			size_t day = ToSizeT(title.substr(23, 2));

			if (year >= 1950 && year <= 2050 && month >= 1 && month <= 12 && day >= 1 && day <= GetNbDayPerMonth(year, month - 1))
				TRef = CTRef(year, month - 1, day - 1);
		}


		return TRef;
	}
	__int16 Landsat::GetPathFromName(const string& title)
	{
		__int16 path = -32768;

		TLandsatFormat format = GetFormatFromName(title);
		if (format == F_OLD)
		{
			path = ToInt(title.substr(3, 3));
		}
		else if (format == F_NEW)
		{
			path = ToInt(title.substr(10, 3));
		}

		return path;
	}
	__int16 Landsat::GetRowFromName(const string& title)
	{
		__int16 row = -32768;

		TLandsatFormat format = GetFormatFromName(title);
		if (format == F_OLD)
		{
			row = ToInt(title.substr(6, 3));
		}
		else if (format == F_NEW)
		{
			row = ToInt(title.substr(13, 3));
		}

		return row;
	}

	Landsat::TIndices Landsat::GetIndiceType(const std::string& str)
	{
		static const char* TYPE_NAME[NB_INDICES] = { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "QA", "JD", "NBR", "NDVI", "NDMI", "TCB", "TCG", "TCW" };
		//"Euclidean",
		TIndices type = I_INVALID;
		for (size_t i = 0; i < NB_INDICES&&type == I_INVALID; i++)
			if (IsEqualNoCase(str, TYPE_NAME[i]))
				type = (TIndices)i;

		return type;
	}

	Landsat::TDomain Landsat::GetIndiceDomain(const std::string& str)
	{
		static const char* TYPE_NAME[NB_INDICES] = { "PRE", "POS", "AND", "OR" };
		TDomain domain = D_INVALID;
		for (size_t i = 0; i < NB_INDICES&&domain == D_INVALID; i++)
			if (IsEqualNoCase(str, TYPE_NAME[i]))
				domain = (TDomain)i;

		return domain;
	}

	Landsat::TOperator Landsat::GetIndiceOperator(const std::string& str)
	{
		static const char* MODE_NAME[NB_OPERATORS] = { "<", ">" };
		TOperator op = O_INVALID;
		for (size_t i = 0; i < NB_OPERATORS&&op == O_INVALID; i++)
			if (IsEqualNoCase(str, MODE_NAME[i]))
				op = (TOperator)i;

		return op;
	}


	using namespace WBSF::Landsat;

	ERMsg CLandsatDataset::OpenInputImage(const std::string& filePath, const CBaseOptions& options)
	{
		ERMsg msg = CGDALDatasetEx::OpenInputImage(filePath, options);
		if (msg)
		{
			//temporal section
			ASSERT(options.m_scenesSize == SCENES_SIZE);

			if ((GetRasterCount() % options.m_scenesSize) == 0)
			{
				bool bFindPeriod = false;
				size_t nbScenes = size_t(GetRasterCount() / options.m_scenesSize);
				m_scenesPeriod.resize(nbScenes);
				for (size_t s = 0; s < nbScenes; s++)
				{
					CTPeriod period;

					//try to identify by name
					if (IsVRT())
					{
						string title = GetFileTitle(GetInternalName(s*options.m_scenesSize));
						CTRef TRef = GetTRefFromName(title);
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

			InitFileInfo();
		}

		return msg;
	}

	void CLandsatDataset::InitFileInfo()
	{
		m_info.resize(GetNbScenes());
		if (IsVRT())
		{
			for (size_t i = 0; i < GetNbScenes(); i++)
			{
				std::string title = GetFileTitle(GetInternalName(i*GetSceneSize()));
				m_info[i].m_format = GetFormatFromName(title);
				m_info[i].m_captor = GetCaptorFromName(title);
				m_info[i].m_path = GetPathFromName(title);
				m_info[i].m_row = GetRowFromName(title);
				m_info[i].m_TRef = GetTRefFromName(title);
			}
		}
	}

	ERMsg CLandsatDataset::CreateImage(const std::string& filePath, CBaseOptions options)
	{
		ASSERT(options.m_nbBands%SCENES_SIZE == 0);

		options.IsVRT();

		ERMsg msg;

		if (options.m_VRTBandsName.empty())
		{

			size_t nbImages = options.m_nbBands / SCENES_SIZE;
			for (size_t i = 0; i < nbImages; i++)
			{
				for (size_t b = 0; b < options.m_scenesSize; b++)
				{
					string name = GetFileTitle(filePath);
					if (nbImages > 1)
						name += "_" + FormatA("%02d", i + 1);

					name += string("_") + GetSceneName(b) + ".tif|";
					options.m_VRTBandsName += name;
				}
			}
		}

		msg += CGDALDatasetEx::CreateImage(filePath, options);

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



	std::string CLandsatDataset::GetCommonBandName(size_t i)
	{
		//replace the common part by the new name
		size_t common_begin = MAX_PATH;//common begin

		for (size_t j = 0; j < SCENES_SIZE - 1; j++)
		{
			string title0 = GetFileTitle(GetInternalName(i*SCENES_SIZE + j));
			string title1 = GetFileTitle(GetInternalName(i*SCENES_SIZE + j + 1));
			size_t k = 0;//common begin
			while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
				k++;

			common_begin = min(common_begin, k);
		}


		string common;
		if (common_begin != MAX_PATH)
		{
			string title = GetFileTitle(GetInternalName(i*SCENES_SIZE));
			common = title.substr(0, common_begin);
		}
		

		return common;

	}

	void CLandsatDataset::Close(const CBaseOptions& options)
	{
		if (IsOpen())
		{
			if (options.m_RGBType != CBaseOptions::NO_RGB)
			{

				size_t nbImages = GetRasterCount() / SCENES_SIZE;
				for (size_t i = 0; i < nbImages; i++)
				{
					string commonName = GetCommonBandName(i);
					string filePath = m_filePath;
					SetFileTitle(filePath, commonName + "RGB");
					ERMsg msg = CreateRGB(i, filePath, options.m_RGBType);
					if (!msg)
					{
						//cout << msg.get;
					}
				}
			}

			CGDALDatasetEx::Close(options);
		}
	}

	ERMsg CLandsatDataset::CreateRGB(size_t iz, const std::string filePath, CBaseOptions::TRGBTye type)
	{
		ERMsg msg;

		ofStream file;

		msg = file.open(filePath, ios::out | ios::binary);
		if (msg)
		{
			zen::XmlElement root("VRTDataset");

			//zen::writeStruc(*this, doc.root());

			root.setAttribute("rasterXSize", to_string(GetRasterXSize()));
			root.setAttribute("rasterYSize", to_string(GetRasterYSize()));


			zen::XmlElement& srs = root.addChild("SRS");
			srs.setValue(GetPrj()->GetPrjStr());

			CGeoTransform GT;
			GetExtents().GetGeoTransform(GT);
			//GetGeoTransform(GT);

			string geotrans = to_string(GT[0]);
			for (int i = 1; i < 6; i++)
				geotrans += ", " + to_string(GT[i]);

			root.addChild("GeoTransform").setValue(geotrans);

			for (size_t j = 0; j < 3; j++)
			{
				zen::XmlElement& node = root.addChild("VRTRasterBand");
				//   
				node.setAttribute("dataType", "Int16");
				node.setAttribute("band", to_string(j+1));
				node.setAttribute("subClass", "VRTDerivedRasterBand");
				node.addChild("NoDataValue").setValue("-32768");
				static const char* RGB_NAME[3] = { "red", "green", "blue" };
				string pixelValName = string("Landsat.") + RGB_NAME[j] + "(" + CBaseOptions::RGB_NAME[type] + ")";

				node.addChild("PixelFunctionType").setValue(pixelValName);
				for (size_t b = 0; b < SCENES_SIZE; b++)
				{
					zen::XmlElement& source = node.addChild("ComplexSource");
					
					string name = GetFileName(GetInternalName(iz*SCENES_SIZE + b));
					
					zen::XmlElement& sourceFilename = source.addChild("SourceFilename");
					sourceFilename.setAttribute("relativeToVRT", "1");
					//sourceFilename.setAttribute("shared", "0");
					sourceFilename.setValue(name);

					int         nBlockXSize, nBlockYSize;
					GetRasterBand(iz*SCENES_SIZE + b)->GetBlockSize(&nBlockXSize, &nBlockYSize);

					source.addChild("SourceBand").setValue("1");
					zen::XmlElement& sourceProperties = source.addChild("SourceProperties");
					sourceProperties.setAttribute("RasterXSize", to_string(GetRasterXSize()));
					sourceProperties.setAttribute("RasterYSize", to_string(GetRasterYSize()));
					sourceProperties.setAttribute("DataType", "Int16");
					sourceProperties.setAttribute("BlockXSize", to_string(nBlockXSize));
					sourceProperties.setAttribute("BlockYSize", to_string(nBlockYSize));
					
					zen::XmlElement& srcRect = source.addChild("SrcRect");
					srcRect.setAttribute("xOff", "0");
					srcRect.setAttribute("yOff", "0");
					srcRect.setAttribute("xSize", to_string(GetRasterXSize()));
					srcRect.setAttribute("ySize", to_string(GetRasterYSize()));

					zen::XmlElement& dstRect = source.addChild("DstRect");
					dstRect.setAttribute("xOff", "0");
					dstRect.setAttribute("yOff", "0");
					dstRect.setAttribute("xSize", to_string(GetRasterXSize()));
					dstRect.setAttribute("ySize", to_string(GetRasterYSize()));

					source.addChild("NODATA").setValue("-32768");
					
				}
			}

			try
			{
				std::string stream;
				zen::implementation::serialize(root, stream, "\r\n", "    ", 0);//throw ()
				file.write(stream);
			}
			catch (std::exception& e)
			{
				msg.ajoute( e.what() );
			}

			
			//string str = file.GetText();

			//	xmlDataset.Load(str.c_str());
		}

		return msg;
	}
	//****************************************************************************************************************
	CLandsatWindow::CLandsatWindow() :
		CRasterWindow(SCENES_SIZE),
		m_bCorr8(false)
	{}

	CLandsatPixel CLandsatWindow::GetPixel(size_t i, int x, int y)const
	{
		CLandsatPixel pixel;
		for (size_t z = 0; z < SCENES_SIZE; z++)
		{
			size_t ii = i*SCENES_SIZE + z;
			pixel[z] = (LandsatDataType)at(ii)->at(x, y);
		}

		if (m_bCorr8 && at(i*SCENES_SIZE)->GetCaptor() == 8 && pixel.IsValid())
			pixel.correction8to7();

		return pixel;
	}

	CLandsatPixel CLandsatWindow::GetPixelMean(size_t i, int x, int y, int buffer)const
	{
		LandsatDataType noData = (LandsatDataType)WBSF::GetDefaultNoData(GDT_Int16);


		CLandsatPixel pixel;
		if (GetPixel(i, x, y).IsValid())
		{
			for (size_t z = 0; z < SCENES_SIZE; z++)
			{
				size_t ii = i*SCENES_SIZE + z;

				CStatistic stat;
				for (int yy = 0; yy < 2 * buffer + 1; yy++)
				{
					for (int xx = 0; xx < 2 * buffer + 1; xx++)
					{
						LandsatDataType val = (LandsatDataType)at(ii)->at(x + xx - buffer, y + yy - buffer);
						if (val != noData)
							stat += val;
					}
				}

				if (stat.IsInit())
					pixel[z] = stat[MEAN];

			}
		}

		return pixel;
	}
	bool CLandsatWindow::GetPixel(size_t i, int x, int y, CLandsatPixel& pixel)const
	{
		ASSERT(i < GetNbScenes());

		if (i != NOT_INIT)
		{
			for (size_t z = 0; z < SCENES_SIZE; z++)
			{
				size_t ii = i*SCENES_SIZE + z;
				pixel[z] = (LandsatDataType)at(ii)->at(x, y);
			}
		}

		return i != NOT_INIT && IsValid(i, pixel);
	}


	//****************************************************************************************************************


	CLandsatPixel::CLandsatPixel()
	{
		Reset();
	}

	void CLandsatPixel::Reset()
	{
		__int16 noData = (__int16)WBSF::GetDefaultNoData(GDT_Int16);
		fill(noData);
	}

	LandsatDataType CLandsatPixel::operator[](const Landsat::TIndices& i)const
	{
		LandsatDataType val = (__int16)WBSF::GetDefaultNoData(GDT_Int16);
		if (IsInit())
		{
			switch (i)
			{
			case Landsat::B1:
			case Landsat::B2:
			case Landsat::B3:
			case Landsat::B4:
			case Landsat::B5:
			case Landsat::B6:
			case Landsat::B7:
			case Landsat::QA:
			case Landsat::JD:			val = LandsatPixel::operator[](i); break;
			case Landsat::I_NBR:		val = NBR(); break;
			case Landsat::I_NDVI:		val = NDVI(); break;
			case Landsat::I_NDMI:		val = NDMI(); break;
			case Landsat::I_TCB:		val = TCB(); break;
			case Landsat::I_TCG:		val = TCG(); break;
			case Landsat::I_TCW:		val = TCW(); break;
			default: ASSERT(false);
			}
		}

		return val;
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

		bool bIsValid = true;
		for (size_t z = 0; z < SCENES_SIZE&&bIsValid; z++)
			bIsValid = at(z) != noData;

		if (at(JD) < 0)
			bIsValid = false;

		return bIsValid;
	}

	void CLandsatPixel::correction8to7()
	{
		static const double c0[SCENES_SIZE - 2] = { 0.00041, 0.00289, 0.00274, 0.00004, 0.00256, 0.0, -0.00327 };
		static const double c1[SCENES_SIZE - 2] = { 0.9747, 0.99779, 1.00446, 0.98906, 0.99467, 1.0, 1.02551 };
		for (size_t b = 0; b < SCENES_SIZE - 2; b++)
		{
			double newVal = c0[b] + c1[b] * at(b);
			at(b) = (__int16)WBSF::LimitToBound(newVal, GDT_Int16, 1);
		}

	}

	double CLandsatPixel::GetCloudRatio()const
	{
		return at(B6) != 0 ? (double)at(B1) / at(B6) : -FLT_MAX;
	}

	Color8 CLandsatPixel::R()const{ return Color8(max(0.0, min(254.0, ((at(B4) + 150.0) / 6150.0) * 254.0))); }
	Color8 CLandsatPixel::G()const{ return Color8(max(0.0, min(254.0, ((at(B5) + 190.0) / 5190.0) * 254.0))); }
	Color8 CLandsatPixel::B()const{ return Color8(max(0.0, min(254.0, ((at(B3) + 200.0) / 2700.0) * 254.0))); }

	double CLandsatPixel::GetEuclideanDistance(const CLandsatPixel& pixel, bool normalized)const
	{
		double r = normalized ? R() - pixel.R() : at(B4) - pixel[B4];
		double g = normalized ? G() - pixel.G() : at(B5) - pixel[B5];
		double b = normalized ? B() - pixel.B() : at(B3) - pixel[B3];

		return sqrt(r*r + g*g + b*b);
	}


	double CLandsatPixel::NBR()const
	{
		//return (at(B4) + at(B7)) != 0 ? ((double)at(B4) - at(B7)) / (at(B4) + at(B7)) : -FLT_MAX;
		return ((double)at(B4) - at(B7)) / max(0.1, double(at(B4) + at(B7)));
	}


	double CLandsatPixel::NDVI()const
	{
		//return (at(B4) + at(B3)) != 0 ? ((double)at(B4) - at(B3)) / (at(B4) + at(B3)) : -FLT_MAX;
		return ((double)at(B4) - at(B3)) / max(0.1, double(at(B4) + at(B3)));
	}

	double CLandsatPixel::NDMI()const
	{
		//return (at(B4) + at(B5)) != 0 ? ((double)at(B4) - at(B5)) / (at(B4) + at(B5)) : -FLT_MAX;
		return ((double)at(B4) - at(B5)) / max(0.1, double(at(B4) + at(B5)));
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
		double d1 = (post - pre);
		double d2 = spike - (post + pre) / 2;

		if (d2 == 0)
			return 1;

		return fabs(d1 / d2);
	}

}