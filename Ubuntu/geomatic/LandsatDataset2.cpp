//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
// 01-01-2016	R�mi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************


#include <algorithm>
#include <bitset>
#include "geomatic/GDAL.h"
#include "geomatic/LandsatDataset2.h"



using namespace std;


namespace WBSF
{

	static double G_INDICES_FACTOR = 1000;
	double Landsat2::INDICES_FACTOR()
	{
		return G_INDICES_FACTOR;
	}
	void Landsat2::INDICES_FACTOR(double f)
	{
		G_INDICES_FACTOR = f;
	}


	const char* Landsat2::GetBandName(size_t s)
	{
		static const char* BANDS_NAME[Landsat2::SCENES_SIZE] = { "B1", "B2", "B3", "B4", "B5", "B7" };

		assert(s < Landsat2::SCENES_SIZE);
		return BANDS_NAME[s];

	}

	const char* Landsat2::GetIndiceName(size_t i)
	{
		static const char* INDICES_NAME[NB_INDICES] = { "B1", "B2", "B3", "B4", "B5", "B7", "NBR", "NDVI", "NDMI", "NDWI", "NDSI", "TCB", "TCG", "TCW", "ZSW", "NBR2", "EVI", "EVI2", "SAVI", "MSAVI", "SR", "HZ",  "LSWI", "VIgreen" };
		assert(i < NB_INDICES);
		return INDICES_NAME[i];
	}

	std::string Landsat2::GetIndiceNames()
	{
		std::string indices;
		for (size_t i = 0; i < NB_INDICES; i++)
		{
			if (!indices.empty())
				indices += ", ";

			indices += GetIndiceName(i);
		}

		return indices;
	}





	//Landsat2::TLandsatFormat Landsat2::GetFormatFromName(const string& title)
	//{
	//	TLandsatFormat format = F_UNKNOWN;
	//	if (!title.empty())
	//	{
	//		if (title.size() >= 25 && title[2] == '0')
	//		{
	//			char captor = char(title[3] - '0');
	//			if (captor == 4 || captor == 5 || captor == 7 || captor == 8)
	//				format = F_NEW;
	//		}
	//		else if (title.size() >= 15)
	//		{
	//			char captor = char(title[2] - '0');
	//			if (captor == 4 || captor == 5 || captor == 7 || captor == 8)
	//				format = F_OLD;
	//		}
	//	}

	//	return format;
	//}

	//LandsatDataType Landsat2::GetCaptorFromName(const string& title)
	//{
	//	LandsatDataType captor = -32768;
	//	if (!title.empty())
	//	{
	//		if (title.size() >= 4)
	//		{
	//			if (title[2] == '0')
	//				captor = char(title[3] - '0');
	//			else
	//				captor = char(title[2] - '0');

	//			if (captor != 4 && captor != 5 && captor != 7 && captor != 8)
	//				captor = -32768;
	//		}
	//	}

	//	return captor;
	//}

	//CTRef Landsat2::GetTRefFromName(const string& title)
	//{
	//	CTRef TRef;

	//	TLandsatFormat format = GetFormatFromName(title);
	//	if (format == F_OLD)
	//	{

	//		//LC80130262016186LGN00_B1
	//		int year = ToInt(title.substr(9, 4));
	//		size_t Jday = ToSizeT(title.substr(13, 3));
	//		if (year >= 1950 && year <= 2050 && Jday >= 1 && Jday <= 366)
	//			TRef = CJDayRef(year, Jday - 1);
	//	}
	//	else if (format == F_NEW)
	//	{
	//		//LC08_L1TP_013026_20170723_20170809_01_T1_B1
	//		int year = ToInt(title.substr(17, 4));
	//		size_t month = ToSizeT(title.substr(21, 2));
	//		size_t day = ToSizeT(title.substr(23, 2));

	//		if (year >= 1950 && year <= 2050 && month >= 1 && month <= 12 && day >= 1 && day <= GetNbDayPerMonth(year, month - 1))
	//			TRef = CTRef(year, month - 1, day - 1);
	//	}


	//	return TRef;
	//}
	//LandsatDataType Landsat2::GetPathFromName(const string& title)
	//{
	//	LandsatDataType path = -32768;

	//	TLandsatFormat format = GetFormatFromName(title);
	//	if (format == F_OLD)
	//	{
	//		path = ToInt(title.substr(3, 3));
	//	}
	//	else if (format == F_NEW)
	//	{
	//		path = ToInt(title.substr(10, 3));
	//	}

	//	return path;
	//}
	//LandsatDataType Landsat2::GetRowFromName(const string& title)
	//{
	//	LandsatDataType row = -32768;

	//	TLandsatFormat format = GetFormatFromName(title);
	//	if (format == F_OLD)
	//	{
	//		row = ToInt(title.substr(6, 3));
	//	}
	//	else if (format == F_NEW)
	//	{
	//		row = ToInt(title.substr(13, 3));
	//	}

	//	return row;
	//}

	Landsat2::TIndices Landsat2::GetIndiceType(const std::string& str)
	{
		TIndices type = I_INVALID;
		for (size_t i = 0; i < NB_INDICES && type == I_INVALID; i++)
			if (IsEqualNoCase(str, GetIndiceName(i)))
				type = (TIndices)i;

		return type;
	}

	//Landsat2::TDomain Landsat2::GetIndiceDomain(const std::string& str)
	//{
	//	static const char* TYPE_NAME[NB_INDICES] = { "PRE", "POS", "AND", "OR" };
	//	TDomain domain = D_INVALID;
	//	for (size_t i = 0; i < NB_INDICES&&domain == D_INVALID; i++)
	//		if (IsEqualNoCase(str, TYPE_NAME[i]))
	//			domain = (TDomain)i;

	//	return domain;
	//}

	//Landsat2::TOperator Landsat2::GetIndiceOperator(const std::string& str)
	//{
	//	static const char* MODE_NAME[NB_OPERATORS] = { "<", ">" };
	//	TOperator op = O_INVALID;
	//	for (size_t i = 0; i < NB_OPERATORS&&op == O_INVALID; i++)
	//		if (IsEqualNoCase(str, MODE_NAME[i]))
	//			op = (TOperator)i;

	//	return op;
	//}

	//Landsat2::TCorr8 Landsat2::GetCorr8(const std::string& str)
	//{
	//	static const char* TYPE_NAME[NB_CORR8_TYPE] = { "CANADA", "AUSTRALIA", "USA" };
	//	TCorr8 corr8 = NO_CORR8;
	//	for (size_t i = 0; i < NB_CORR8_TYPE&&corr8 == NO_CORR8; i++)
	//	{
	//		if (IsEqualNoCase(str, TYPE_NAME[i]))
	//			corr8 = (TCorr8)i;
	//	}

	//	return corr8;
	//}


	using namespace WBSF::Landsat2;

	ERMsg CLandsatDataset::OpenInputImage(const std::string& filePath, const CBaseOptions& options)
	{
		ERMsg msg = CGDALDatasetEx::OpenInputImage(filePath, options);
		if (msg)
		{
			//temporal section
			assert(options.GetSceneSize() == SCENES_SIZE);

			if ((GetRasterCount() % options.GetSceneSize()) == 0)
			{


				// bool bFindPeriod = false;
				//size_t nbScenes = size_t(GetRasterCount() / options.GetSceneSize());
				//m_scenes_def = options.m_scenes_def;
	//            m_scenesPeriod.resize(nbScenes);
				//for (size_t s = 0; s < nbScenes; s++)
				//{
				//	//CTPeriod period;

				//	////try to identify by name
				//	//if (IsVRT())
				//	//{
				//	//	string title = GetFileTitle(GetInternalName(s*options.m_scenesSize));
				//	//	CTRef TRef = GetTRefFromName(title);
				//	//	if (TRef.IsInit())
				//	//		period = CTPeriod(TRef, TRef);
				//	//}

				//	//if (!period.IsInit())
				//	//{
				//	//	//try to find by the date layers
				//	//	double TRefMin = 0;
				//	//	double TRefMax = 0;
				//	//	if (GetRasterBand(s * options.m_scenesSize + JD)->GetStatistics(true, true, &TRefMin, &TRefMax, NULL, NULL) == CE_None)
				//	//	{
				//	//		period.Begin() = options.GetTRef(int(TRefMin));
				//	//		period.End() = options.GetTRef(int(TRefMax));
				//	//	}
				//	//}

				//	/*if (period.IsInit())
				//	{
				//		bFindPeriod = true;
				//		m_scenesPeriod[s] = period;
				//	}*/

				//}//for all scenes

				/*if (!bFindPeriod)
				{
					msg.ajoute("ERROR: Unable to get temporal reference from images");
				}*/
			}
			else
			{
				msg.ajoute("ERROR: input image bands count (" + ToString(int(GetRasterCount())) + ") must be a multiple of LANDSAT scene size (" + ToString(int(options.GetSceneSize())) + ")");
			}

			//InitFileInfo();
		}

		return msg;
	}

	/*void CLandsatDataset::InitFileInfo()
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
	}*/

	ERMsg CLandsatDataset::CreateImage(const std::string& filePath, const CBaseOptions& optionsIn)
	{
		assert(optionsIn.m_nbBands % optionsIn.GetSceneSize() == 0);

		ERMsg msg;


		CBaseOptions options(optionsIn);
		if (options.m_VRTBandsName.empty())
		{
			size_t nbImages = options.m_nbBands / options.GetSceneSize();
			for (size_t i = 0; i < nbImages; i++)
			{
				for (size_t b = 0; b < size_t(options.GetSceneSize()); b++)
				{
					string name = GetFileTitle(filePath);
					if (nbImages > 1)
						name += "_" + FormatA("%02d", i + 1);

					name += string("_") + GetBandName(b) + ".tif|";
					options.m_VRTBandsName += name;
				}
			}
		}

		msg += CGDALDatasetEx::CreateImage(filePath, options);

		return msg;
	}


	void CLandsatDataset::UpdateOption(CBaseOptions& option)const
	{
		//CMergeImagesOption& option = dynamic_cast<CMergeImagesOption&>(optionIn);

		CGDALDatasetEx::UpdateOption(option);

		//		if (!option.m_period.IsInit())
		//		{
		//			option.m_period = GetPeriod();
		//			//expand period over the year
		//			if (option.m_period.IsInit())
		//			{
		//				option.m_period.Begin().SetJDay(0);
		//				option.m_period.End().SetJDay(LAST_DAY);
		//			}
		//		}


			//compute number of output bands
			//if (option.m_TM.Type() == CTM::DAILY)
			//{
			//copy scenes period
			//option.m_scenesPeriod = GetScenePeriod();
			//}

	}

	std::string CLandsatDataset::GetCommonName()const
	{
		//replace the common part by the new name
		size_t common_end = 255;//common begin

		string title0 = GetFileTitle(GetInternalName(0));

		for (size_t j = 1; j < GetRasterCount(); j++)
		{
			string title1 = GetFileTitle(GetInternalName(j));
			size_t k = 0;//common begin
			while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
				k++;

			common_end = min(common_end, k);
		}


		string title = GetFileTitle(GetInternalName(0));
		if (common_end != 255)
			title = title.substr(0, common_end);

		return title;
	}

	std::string CLandsatDataset::GetCommonImageName(size_t i)const
	{

		string title0 = GetFileTitle(GetInternalName(i * SCENES_SIZE));
		//replace the common part by the new name
		size_t common_end = 255;//common begin
		for (size_t j = 1; j < SCENES_SIZE; j++)
		{
			string title1 = GetFileTitle(GetInternalName(i * SCENES_SIZE + j));
			size_t k = 0;//common begin
			while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
				k++;

			common_end = min(common_end, k);
		}

		std::string common = CLandsatDataset::GetCommonName();
		string title = GetFileTitle(GetInternalName(i * SCENES_SIZE));
		if (common_end != 255)
			title = title.substr(common.length(), common_end - (common.length()+1));

		return title;
	}

	std::string CLandsatDataset::GetSubname(size_t i, size_t b)const
	{
		string subName;
		//if (format.empty())
		//{
		//if we not rename
		if (b == NOT_INIT)
		{
			//if specific band is not selected common name for all bans of the image
			subName = WBSF::TrimConst(GetCommonImageName(i), "_");
			if (subName.empty())
				subName = FormatA("%02d", i + 1);

			subName = subName;
		}
		else
		{
			//if specific band is selected, return the specific name this band in the image
			subName = WBSF::TrimConst(GetSpecificBandName(i * SCENES_SIZE + b), "_");
		}
		//}
		//else
		//{
		//	double min = 0;
		//	double max = 0;
		//	double mean = 0;
		//	double stddev = 0;

		//	GDALRasterBand * pBand = const_cast<GDALRasterBand *>(GetRasterBand(i*SCENES_SIZE + JD));
		//	if (pBand->GetStatistics(true, true, &min, &max, &mean, &stddev) == CE_None)
		//	{

		//		CTRef TRef = CTRef(1970, JANUARY, DAY_01) + int(mean) - 1;
		//		WBSF::ReplaceString(format, "%J", to_string(int(mean)));//replace %J by Julina day since 1970


		//		if (format.find("%P") != string::npos)
		//		{
		//			assert(i < m_info.size());
		//			string path_row = WBSF::FormatA("%03d%03d", m_info[i].m_path, m_info[i].m_row);
		//			WBSF::ReplaceString(format, "%P", path_row);//replace %P by path/row
		//		}

		//		subName = TRef.GetFormatedString(format);
		//	}

		//	if (b != NOT_INIT)
		//		subName += string("_") + Landsat2::GetBandName(b);
		//}

		return subName;
	}



	std::string CLandsatDataset::GetCommonBandName(size_t b)const
	{
		std::string common = CLandsatDataset::GetCommonImageName(b / SCENES_SIZE);
		string title = GetFileTitle(GetInternalName(b));
		if (!title.empty())
			title = title.substr(common.length());
		else
			title = FormatA("%d_%s", int(b / SCENES_SIZE) + 1, Landsat2::GetBandName(b % SCENES_SIZE));

		return title;
	}
	std::string CLandsatDataset::GetSpecificBandName(size_t b)const
	{
		std::string common = CLandsatDataset::GetCommonName();
		string title = GetFileTitle(GetInternalName(b));
		if (!title.empty())
			title = title.substr(common.length());
		else
			title = FormatA("%d_%s", int(b / SCENES_SIZE) + 1, Landsat2::GetBandName(b % SCENES_SIZE));

		return title;
	}


	std::string CLandsatDataset::GetSpecificBandName(size_t i, size_t b)const
	{
		return CLandsatDataset::GetSpecificBandName(i * SCENES_SIZE + b);
	}

	void CLandsatDataset::Close(const CBaseOptions& options)
	{
		if (IsOpen())
		{
			if (m_bOpenUpdate)
			{
				//update scene period size before closing to correctly buildt VRT without empty bands
				//size_t nbScenes = size_t(GetRasterCount() / options.GetSceneSize());
				//m_scenesPeriod.resize(nbScenes);


	//            if (options.m_RGBType != CBaseOptions::NO_RGB)
	//            {
	//
	//                //size_t nbImages = GetRasterCount() / SCENES_SIZE;
	//                for (size_t i = 0; i < nbScenes; i++)
	//                {
	//                    string title = GetFileTitle(GetInternalName(i * SCENES_SIZE));
	//                    //string commonName = WBSF::TrimConst(GetCommonBandName(i*SCENES_SIZE),"_");
	//                    string filePath = m_filePath;
	//                    //string title = GetFileTitle(filePath);
	//                    SetFileTitle(filePath, title.substr(0, title.length() - 2) + "RGB");
	////                    ERMsg msg = CreateRGB(i, filePath, options.m_RGBType);
	////                    if (!msg)
	////                    {
	////                        //cout << msg.get;
	////                    }
	//                }
	//            }
			}

			CGDALDatasetEx::Close(options);
		}
	}

	//	ERMsg CLandsatDataset::CreateRGB(size_t iz, const std::string filePath, CBaseOptions::TRGBTye type)
	//
	//	{
	//		ERMsg msg;
	//
	//		ofStream file;
	//
	//		msg = file.open(filePath, ios::out | ios::binary);
	//		if (msg)
	//		{
	//			zen::XmlElement root("VRTDataset");
	//
	//			//zen::writeStruc(*this, doc.root());
	//
	//			root.setAttribute("rasterXSize", to_string(GetRasterXSize()));
	//			root.setAttribute("rasterYSize", to_string(GetRasterYSize()));
	//
	//
	//			zen::XmlElement& srs = root.addChild("SRS");
	//			srs.setValue(GetPrj()->GetPrjStr());
	//
	//			CGeoTransform GT;
	//			GetExtents().GetGeoTransform(GT);
	//			//GetGeoTransform(GT);
	//
	//			string geotrans = to_string(GT[0]);
	//			for (int i = 1; i < 6; i++)
	//				geotrans += ", " + to_string(GT[i]);
	//
	//			root.addChild("GeoTransform").setValue(geotrans);
	//
	//			for (size_t j = 0; j < 3; j++)
	//			{
	//				zen::XmlElement& node = root.addChild("VRTRasterBand");
	//				//
	//				node.setAttribute("dataType", "Int16");
	//				node.setAttribute("band", to_string(j + 1));
	//				node.setAttribute("subClass", "VRTDerivedRasterBand");
	//				node.addChild("NoDataValue").setValue("-32768");
	//				static const char* RGB_NAME[3] = { "red", "green", "blue" };
	//				string pixelValName = string("Landsat.") + RGB_NAME[j] + "(" + CBaseOptions::RGB_NAME[type] + ")";
	//
	//				node.addChild("PixelFunctionType").setValue(pixelValName);
	//				for (size_t b = 0; b < SCENES_SIZE; b++)
	//				{
	//					zen::XmlElement& source = node.addChild("ComplexSource");
	//
	//					//string name = GetFileName(GetInternalName(iz*SCENES_SIZE + b));
	//
	//					string file_path = GetInternalName(iz * SCENES_SIZE + b);
	//					string rel_file_path = WBSF::GetRelativePath(GetPath(filePath), file_path);
	//
	//					zen::XmlElement& sourceFilename = source.addChild("SourceFilename");
	//					sourceFilename.setAttribute("relativeToVRT", rel_file_path.empty() ? "0" : "1");
	//					sourceFilename.setValue(rel_file_path.empty() ? file_path : rel_file_path);
	//
	//					//sourceFilename.setAttribute("relativeToVRT", "1");
	//					//sourceFilename.setValue(name);
	//
	//					int         nBlockXSize, nBlockYSize;
	//					GetRasterBand(iz * SCENES_SIZE + b)->GetBlockSize(&nBlockXSize, &nBlockYSize);
	//
	//					source.addChild("SourceBand").setValue("1");
	//					zen::XmlElement& sourceProperties = source.addChild("SourceProperties");
	//					sourceProperties.setAttribute("RasterXSize", to_string(GetRasterXSize()));
	//					sourceProperties.setAttribute("RasterYSize", to_string(GetRasterYSize()));
	//					sourceProperties.setAttribute("DataType", "Int16");
	//					sourceProperties.setAttribute("BlockXSize", to_string(nBlockXSize));
	//					sourceProperties.setAttribute("BlockYSize", to_string(nBlockYSize));
	//
	//					zen::XmlElement& srcRect = source.addChild("SrcRect");
	//					srcRect.setAttribute("xOff", "0");
	//					srcRect.setAttribute("yOff", "0");
	//					srcRect.setAttribute("xSize", to_string(GetRasterXSize()));
	//					srcRect.setAttribute("ySize", to_string(GetRasterYSize()));
	//
	//					zen::XmlElement& dstRect = source.addChild("DstRect");
	//					dstRect.setAttribute("xOff", "0");
	//					dstRect.setAttribute("yOff", "0");
	//					dstRect.setAttribute("xSize", to_string(GetRasterXSize()));
	//					dstRect.setAttribute("ySize", to_string(GetRasterYSize()));
	//
	//					source.addChild("NODATA").setValue("-32768");
	//
	//				}
	//			}
	//
	//			try
	//			{
	//				std::string stream;
	//				zen::implementation::serialize(root, stream, "\r\n", "    ", 0);//throw ()
	//				file.write(stream);
	//			}
	//			catch (std::exception& e)
	//			{
	//				msg.ajoute(e.what());
	//			}
	//
	//
	//			//string str = file.GetText();
	//
	//			//	xmlDataset.Load(str.c_str());
	//		}
	//
	//		return msg;
	//	}
	//
	//	ERMsg CLandsatDataset::CreateIndices(size_t iz, const std::string filePath, Landsat2::TIndices type)
	//	{
	//		ERMsg msg;
	//
	//		ofStream file;
	//
	//		msg = file.open(filePath, ios::out | ios::binary);
	//		if (msg)
	//		{
	//			zen::XmlElement root("VRTDataset");
	//
	//			//zen::writeStruc(*this, doc.root());
	//
	//			root.setAttribute("rasterXSize", to_string(GetRasterXSize()));
	//			root.setAttribute("rasterYSize", to_string(GetRasterYSize()));
	//
	//
	//			zen::XmlElement& srs = root.addChild("SRS");
	//			srs.setValue(GetPrj()->GetPrjStr());
	//
	//			CGeoTransform GT;
	//			GetExtents().GetGeoTransform(GT);
	//			//GetGeoTransform(GT);
	//
	//			string geotrans = to_string(GT[0]);
	//			for (int i = 1; i < 6; i++)
	//				geotrans += ", " + to_string(GT[i]);
	//
	//			root.addChild("GeoTransform").setValue(geotrans);
	//
	//			//for (size_t j = 0; j < 3; j++)
	//			//{
	//			zen::XmlElement& node = root.addChild("VRTRasterBand");
	//			//
	//			node.setAttribute("dataType", "Int16");
	//			node.setAttribute("band", "1");
	//			node.setAttribute("subClass", "VRTDerivedRasterBand");
	//			node.addChild("NoDataValue").setValue("-32768");
	//
	//			string pixelValName = string("Landsat.") + GetIndiceName(type);
	//
	//			node.addChild("PixelFunctionType").setValue(pixelValName);
	//			for (size_t b = 0; b < SCENES_SIZE; b++)
	//			{
	//				zen::XmlElement& source = node.addChild("ComplexSource");
	//
	//				string file_path = GetInternalName(iz * SCENES_SIZE + b);
	//				string rel_file_path = WBSF::GetRelativePath(GetPath(filePath), file_path);
	//
	//				zen::XmlElement& sourceFilename = source.addChild("SourceFilename");
	//				sourceFilename.setAttribute("relativeToVRT", rel_file_path.empty() ? "0" : "1");
	//				//sourceFilename.setAttribute("shared", "0");
	//				sourceFilename.setValue(rel_file_path.empty() ? file_path : rel_file_path);
	//
	//				int         nBlockXSize, nBlockYSize;
	//				GetRasterBand(iz * SCENES_SIZE + b)->GetBlockSize(&nBlockXSize, &nBlockYSize);
	//
	//				source.addChild("SourceBand").setValue("1");
	//				zen::XmlElement& sourceProperties = source.addChild("SourceProperties");
	//				sourceProperties.setAttribute("RasterXSize", to_string(GetRasterXSize()));
	//				sourceProperties.setAttribute("RasterYSize", to_string(GetRasterYSize()));
	//				sourceProperties.setAttribute("DataType", "Int16");
	//				sourceProperties.setAttribute("BlockXSize", to_string(nBlockXSize));
	//				sourceProperties.setAttribute("BlockYSize", to_string(nBlockYSize));
	//
	//				zen::XmlElement& srcRect = source.addChild("SrcRect");
	//				srcRect.setAttribute("xOff", "0");
	//				srcRect.setAttribute("yOff", "0");
	//				srcRect.setAttribute("xSize", to_string(GetRasterXSize()));
	//				srcRect.setAttribute("ySize", to_string(GetRasterYSize()));
	//
	//				zen::XmlElement& dstRect = source.addChild("DstRect");
	//				dstRect.setAttribute("xOff", "0");
	//				dstRect.setAttribute("yOff", "0");
	//				dstRect.setAttribute("xSize", to_string(GetRasterXSize()));
	//				dstRect.setAttribute("ySize", to_string(GetRasterYSize()));
	//
	//				source.addChild("NODATA").setValue("-32768");
	//
	//			}
	//			//}
	//
	//			try
	//			{
	//				std::string stream;
	//				zen::implementation::serialize(root, stream, "\r\n", "    ", 0);//throw ()
	//				file.write(stream);
	//			}
	//			catch (std::exception& e)
	//			{
	//				msg.ajoute(e.what());
	//			}
	//
	//
	//			//string str = file.GetText();
	//
	//			//	xmlDataset.Load(str.c_str());
	//		}
	//
	//		return msg;
	//	}


	//****************************************************************************************************************
	CLandsatWindow::CLandsatWindow() :
		CRasterWindow(TLandsatBands::SCENES_SIZE)
	{}

	CLandsatWindow::CLandsatWindow(const CRasterWindow& in) :
		CRasterWindow(TLandsatBands::SCENES_SIZE)
	{
		if (&in != this)
			CRasterWindow::operator=(in);

	}

	CLandsatWindow::CLandsatWindow(const CLandsatWindow& in) :
		CRasterWindow(TLandsatBands::SCENES_SIZE)
	{
		if (&in != this)
			CRasterWindow::operator=(in);
	}

	size_t CLandsatWindow::GetPrevious(size_t z, int x, int y)const
	{
		size_t previous = NOT_INIT;
		for (size_t zz = z - 1; zz < GetNbScenes() && previous == NOT_INIT; zz--)
		{
			if (GetPixel(zz, x, y).IsValid())
				previous = zz;
		}

		return previous;
	}

	size_t CLandsatWindow::GetNext(size_t z, int x, int y)const
	{
		size_t next = NOT_INIT;

		for (size_t zz = z + 1; zz < GetNbScenes() && next == NOT_INIT; zz++)
		{
			if (GetPixel(zz, x, y).IsValid())
				next = zz;
		}
		return next;
	}

	CLandsatPixel CLandsatWindow::GetPixel(size_t i, int x, int y)const
	{
		CLandsatPixel pixel;
		for (size_t z = 0; z < SCENES_SIZE; z++)
		{
			size_t ii = i * SCENES_SIZE + z;
			if (at(ii).IsValid(x, y))//don't replace no CLandsatPixel  no data by CLandsatWindow no data. not the same
				pixel[z] = (LandsatDataType)at(ii).at(x, y);
		}

		//if (m_corr8 != NO_CORR8 && at(i*SCENES_SIZE)->GetCaptor() == 8 && pixel.IsValid())
		//	pixel.correction8to7(m_corr8);

		return pixel;
	}


	CLandsatPixel CLandsatWindow::GetPixelMean(size_t i, int x, int y, int n_rings, const std::vector<double>& weight)const
	{
		assert(weight.empty() || size_t(n_rings + 1) == weight.size());
		if (n_rings == 0)
			return GetPixel(i, x, y);

		std::vector<double> w = weight;
		if (w.empty())// no weight
			w.resize(n_rings + 1, 1.0 / square(2 * n_rings + 1));

		CLandsatPixel pixel;

		array<CStatistic, SCENES_SIZE> stat;
		CStatistic stat_w;
		for (int yy = -n_rings; yy <= n_rings; yy++)
		{
			for (int xx = -n_rings; xx <= n_rings; xx++)
			{
				CLandsatPixel p = GetPixel(i, x + xx, y + yy);
				if (p.IsValid())
				{
					for (size_t z = 0; z < SCENES_SIZE; z++)
					{
						size_t r = max(abs(xx), abs(yy));
						assert(r < w.size());
						stat_w += w[r];
						stat[z] += p[z] * w[r];
					}
				}
			}
		}

		if (stat_w.IsInit())
		{
			assert(stat_w[SUM] > 0);
			for (size_t z = 0; z < SCENES_SIZE; z++)
				pixel[z] = DataType(stat[z][SUM] / stat_w[SUM]);
		}

		return pixel;
	}

	//bool CLandsatWindow::GetPixel(size_t i, int x, int y, CLandsatPixel& pixel)const
	//{
	//    assert(i < GetNbScenes());
	//
	//    if (i != NOT_INIT)
	//    {
	//        pixel = GetPixel(i, x, y);
	//        /*for (size_t z = 0; z < SCENES_SIZE; z++)
	//        {
	//        	size_t ii = i * SCENES_SIZE + z;
	//        	pixel[z] = (LandsatDataType)at(ii)->at(x, y);
	//        }*/
	//    }
	//
	//    return i != NOT_INIT && IsValid(i, pixel);
	//}

	CStatistic CLandsatWindow::GetPixelIndiceI(size_t z, Landsat2::TIndices ind, int x, int y, int n_rings)const
	{
		CStatistic stat;
		for (int yy = -n_rings; yy <= n_rings; yy++)
		{
			for (int xx = -n_rings; xx <= n_rings; xx++)
			{
				//if (yy == n_rings || xx == n_rings)//select only pixel of the current ring. RSA (2024-07-24)
				//{
				CLandsatPixel p = GetPixel(z, x + xx, y + yy);
				if (p.IsValid())
				{
					stat += p[ind];
				}
				//}
			}
		}

		return stat;
	}

	LandsatDataType CLandsatWindow::GetPixelIndice(size_t z, Landsat2::TIndices ind, int x, int y, double n_rings)const
	{
		assert(n_rings >= 0 && n_rings < 1000);

		if (n_rings == 0)
			return GetPixel(z, x, y)[ind];


		LandsatDataType val = CLandsatPixel::GetLandsatNoData();

		int n_rings1 = (int)floor(n_rings);
		int n_rings2 = (int)ceil(n_rings);
		assert((n_rings - n_rings1) >= 0);
		assert((n_rings2 - n_rings) >= 0);


		if (n_rings1 == n_rings2)
		{
			assert(n_rings1 == n_rings && n_rings2 == n_rings);
			CStatistic stat_i = GetPixelIndiceI(z, ind, x, y, n_rings1);


			if (stat_i.IsInit())
			{
				val = LandsatDataType(stat_i[MEAN]);
			}
		}
		else
		{
			assert((n_rings2 - n_rings1) == 1);

			CStatistic stat_i1 = GetPixelIndiceI(z, ind, x, y, n_rings1);
			CStatistic stat_i2 = GetPixelIndiceI(z, ind, x, y, n_rings2);

			if (stat_i1.IsInit() && stat_i2.IsInit())
			{
				val = LandsatDataType(stat_i1[MEAN] * (n_rings2 - n_rings) + stat_i2[MEAN] * (n_rings - n_rings1));
			}
			else if (stat_i1.IsInit())
			{
				val = LandsatDataType(stat_i1[MEAN]);
			}
			else if (stat_i2.IsInit())
			{
				val = LandsatDataType(stat_i2[MEAN]);
			}
		}

		return val;
	}


	CLandsatPixel CLandsatWindow::GetPixelMedian(size_t f, size_t l, int x, int y, int n_rings)const
	{
		assert(f < GetNbScenes());
		assert(l < GetNbScenes());
		LandsatDataType noData = CLandsatPixel::GetLandsatNoData();
		bool bAllBandsValid = true;

		array<CStatisticEx, SCENES_SIZE> stat;
		for (size_t i = f; i <= l; i++)
		{
			if (bAllBandsValid)
			{
				for (int yy = 0; yy < 2 * n_rings + 1; yy++)
				{
					for (int xx = 0; xx < 2 * n_rings + 1; xx++)
					{
						CLandsatPixel p = GetPixel(i, x + xx, y + yy);
						if (p.IsValid())
						{
							for (size_t z = 0; z < SCENES_SIZE; z++)
								stat[z] += p[z];
						}
					}
				}
			}
			else
			{
				for (size_t z = 0; z < SCENES_SIZE; z++)
				{
					size_t ii = i * SCENES_SIZE + z;

					for (int yy = 0; yy < 2 * n_rings + 1; yy++)
					{
						for (int xx = 0; xx < 2 * n_rings + 1; xx++)
						{
							LandsatDataType val = (LandsatDataType)at(ii).at(x + xx - n_rings, y + yy - n_rings);
							if (val != noData)
								stat[z] += val;
						}
					}
				}
			}
		}

		bool bValid = true;
		for (size_t z = 0; z < SCENES_SIZE && bValid; z++)
		{
			if (!stat[z].IsInit())
				bValid = false;
		}

		CLandsatPixel pixel;
		if (bValid)
		{
			for (size_t z = 0; z < SCENES_SIZE; z++)
				pixel[z] = DataType(stat[z][MEDIAN]);
		}

		return pixel;
	}

	//humm?????
	//CLandsatPixel CLandsatWindow::GetPixelMedian(size_t f, size_t l, int x, int y, double n_rings)const
	//{
	//    CLandsatPixel out;
	//    //LandsatDataType val = (LandsatDataType)WBSF::GetDefaultNoData(GDT_UInt16);
	//
	//    int n_rings1 = (int)floor(n_rings);
	//    int n_rings2 = (int)ceil(n_rings);
	//    assert((n_rings - n_rings1) >= 0);
	//    assert((n_rings2 - n_rings) >= 0);
	//    assert((n_rings2 - n_rings1) == 1);
	//    if(n_rings1== n_rings2)
	//        return GetPixelMedian(f, l, x, y, n_rings1);
	//
	//    CLandsatPixel i1 = GetPixelMedian(f, l, x, y, n_rings1);
	//    CLandsatPixel i2 = GetPixelMedian(f, l, x, y, n_rings2);
	//
	//    if (i1.IsValid() && i2.IsValid())
	//    {
	//        for (size_t z = 0; z < SCENES_SIZE; z++)
	//            out[z] = LandsatDataType((double)i1[z] * (n_rings2 - n_rings) + (double)i2[z] * (n_rings - n_rings1));
	//    }
	//    else if (i1.IsValid())
	//    {
	//        out = i1;
	//    }
	//    else if (i2.IsValid())
	//    {
	//        out = i2;
	//    }
	//
	//    return out;
	//}


	LandsatDataType CLandsatWindow::GetPixelIndiceMedian(Landsat2::TIndices ind, int x, int y, double n_rings)const
	{
		LandsatDataType val = CLandsatPixel::GetLandsatNoData();

		CStatisticEx stat;
		for (size_t z = 0; z < size(); z++)
		{
			stat += GetPixelIndice(z, ind, x, y, n_rings);
		}

		if (stat.IsInit())
			val = LandsatDataType(stat[MEDIAN]);

		return val;
	}

	CLandsatPixelVector CLandsatWindow::Synthetize(Landsat2::TIndices ind, int x, int y, const std::vector<LandsatDataType>& fit_ind, double n_rings)const
	{
		assert(fit_ind.size() == size());

		CLandsatPixelVector out(size());

		CLandsatPixel median = GetPixelMedian(x, y, 0);
		LandsatDataType median_ind = GetPixelIndiceMedian(ind, x, y, 0);

		CStatisticEx stat;
		for (size_t z = 0; z < size(); z++)
		{
			for (size_t s = 0; s < median.size(); s++)
				out[z][s] = LandsatDataType(median[s] * ((double)fit_ind[z] / median_ind));
		}

		return out;
	}





	//****************************************************************************************************************


	CLandsatPixel::CLandsatPixel()
	{
		Reset();
	}

	//GDALDataType CLandsatPixel::GetGDALDataType()
	//{
	//    return GDT_Int16;
	//}

	LandsatDataType CLandsatPixel::GetLandsatNoData()
	{
		return (LandsatDataType)DefaultNoData;
	}

	void CLandsatPixel::Reset()
	{
		//LandsatDataType noData = (LandsatDataType)WBSF::GetDefaultNoData(GDT_UInt16);
		fill(GetLandsatNoData());
	}

	LandsatDataType CLandsatPixel::operator[](const Landsat2::TIndices& i)const
	{
		LandsatDataType val = GetLandsatNoData();
		if (IsInit(i))
		{
			switch (i)
			{
			case B1:
			case B2:
			case B3:
			case B4:
			case B5:
			case B7:
				val = (LandsatDataType)WBSF::LimitToBound(at(i), GDT_Int16, 1);
				break;
			case I_NBR:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NBR(), GDT_Int16, 1);
				break;
			case I_NDVI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NDVI(), GDT_Int16, 1);
				break;
			case I_NDMI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NDMI(), GDT_Int16, 1);
				break;
			case I_NDWI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NDWI(), GDT_Int16, 1);
				break;
			case I_NDSI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NDSI(), GDT_Int16, 1);
				break;
			case I_TCB:
				val = (LandsatDataType)WBSF::LimitToBound(TCB(), GDT_Int16, 1);
				break;
			case I_TCG:
				val = (LandsatDataType)WBSF::LimitToBound(TCG(), GDT_Int16, 1);
				break;
			case I_TCW:
				val = (LandsatDataType)WBSF::LimitToBound(TCW(), GDT_Int16, 1);
				break;
			case I_ZSW:
				val = (LandsatDataType)WBSF::LimitToBound(ZSW(), GDT_Int16, 1);
				break;
			case I_NBR2:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * NBR2(), GDT_Int16, 1);
				break;
			case I_EVI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * EVI(), GDT_Int16, 1);
				break;
			case I_EVI2:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * EVI2(), GDT_Int16, 1);
				break;
			case I_SAVI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * SAVI(), GDT_Int16, 1);
				break;
			case I_MSAVI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * MSAVI(), GDT_Int16, 1);
				break;
			case I_SR:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * SR(), GDT_Int16, 1);
				break;
				//case I_CL:		val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() *CL(), GDT_Int16, 1); break;
			case I_HZ:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * HZ(), GDT_Int16, 1);
				break;
			case I_LSWI:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * LSWI(), GDT_Int16, 1);
				break;
			case I_VIgreen:
				val = (LandsatDataType)WBSF::LimitToBound(INDICES_FACTOR() * VIgreen(), GDT_Int16, 1);
				break;
			default:
				assert(false);
			}
		}

		return val;
	}

	bool CLandsatPixel::IsInit()const
	{
		LandsatDataType noData = GetLandsatNoData();

		if (IsZero())
			return false;

		bool bIsInit = false;
		for (size_t z = 0; z < SCENES_SIZE && !bIsInit; z++)
			bIsInit |= at(z) != noData;

		return bIsInit;
	}

	bool CLandsatPixel::IsInit(TIndices i)const
	{
		LandsatDataType noData = GetLandsatNoData();

		if (IsZero())
			return false;

		std::bitset<9> need;
		switch (i)
		{
		case B1:
		case B2:
		case B3:
		case B4:
		case B5:
		case B7:need.set(i); break;
		case I_NBR:
			need.set(B4);
			need.set(B7);
			break;
		case I_NDVI:
			need.set(B3);
			need.set(B4);
			break;
		case I_NDMI:
			need.set(B4);
			need.set(B5);
			break;
		case I_NDWI:
			need.set(B4);
			need.set(B5);
			break;
		case I_NDSI:
			need.set(B2);
			need.set(B5);
			break;
		case I_TCB:
			need.set(B1);
			need.set(B2);
			need.set(B3);
			need.set(B4);
			need.set(B5);
			need.set(B7);
			break;
		case I_TCG:
			need.set(B1);
			need.set(B2);
			need.set(B3);
			need.set(B4);
			need.set(B5);
			need.set(B7);
			break;
		case I_TCW:
			need.set(B1);
			need.set(B2);
			need.set(B3);
			need.set(B4);
			need.set(B5);
			need.set(B7);
			break;
		case I_ZSW:
			need.set(B3);
			need.set(B4);
			need.set(B5);
			need.set(B7);
			break;
		case I_NBR2:
			need.set(B5);
			need.set(B7);
			break;
		case I_EVI:
			need.set(B1);
			need.set(B3);
			need.set(B4);
			break;
		case I_EVI2:
			need.set(B3);
			need.set(B4);
			break;
		case I_SAVI:
			need.set(B3);
			need.set(B4);
			break;
		case I_MSAVI:
			need.set(B3);
			need.set(B4);
			break;
		case I_SR:
			need.set(B3);
			need.set(B4);
			break;
			//case I_CL:		need.set(B1); need.set(B6); break;
		case I_HZ:
			need.set(B1);
			need.set(B3);
			break;
		case I_LSWI:
			need.set(B4);
			need.set(B5);
			break;
		case I_VIgreen:
			need.set(B2);
			need.set(B3);
			break;
		default:
			assert(false);
		}

		bool bIsInit = true;
		for (size_t z = 0; z < SCENES_SIZE && bIsInit; z++)
		{
			if (need.test(z))
				bIsInit &= at(z) != noData;
		}

		return bIsInit;
	}


	bool CLandsatPixel::IsValid()const
	{
		LandsatDataType noData = GetLandsatNoData();

		if (IsZero())
			return false;

		bool bIsValid = true;
		for (size_t z = 0; z < SCENES_SIZE && bIsValid; z++)
			bIsValid = at(z) != noData;

		//if (at(JD) < 0)
		//bIsValid = false;

		return bIsValid;
	}

	/*void CLandsatPixel::correction8to7(Landsat2::TCorr8 type)
	{
		assert(type < NB_CORR8_TYPE);

		static const double c0[NB_CORR8_TYPE][SCENES_SIZE - 2] =
		{
			{ 129.3, 98.3, 66.0, 240.3, 113.4, 0.0, 51.6},
			{ 4.1, 28.9, 27.4, 0.4, 25.6, 0.0, -32.7 },
			{ 183.0, 123.0, 123.0, 448.0, 306.0, 0.0, 116.0 }
		};
		static const double c1[NB_CORR8_TYPE][SCENES_SIZE - 2] =
		{
			{ 0.6512, 0.74324, 0.76879, 0.87379, 0.88424, 1.0, 0.86283 },
			{ 0.9747, 0.99779, 1.00446, 0.98906, 0.99467, 1.0, 1.02551 },
			{ 0.8850, 0.93170, 0.93720,	0.83390, 0.86390, 1.0, 0.91650 }
		};

		for (size_t b = 0; b < SCENES_SIZE - 2; b++)
		{
			double newVal = c0[type][b] + c1[type][b] * at(b);
			at(b) = (LandsatDataType)WBSF::LimitToBound(newVal, GDT_UInt16, 1);
		}

	}*/

	/*double CLandsatPixel::GetCloudRatio()const
	{
		return (double)at(B1) / max(0.1, (double)at(B6));
	}*/
	static const double KK = 3.0;

	Color8 CLandsatPixel::R(CBaseOptions::TRGBTye type, const CBandStats& stats)const
	{
		Color8 pix_val = 255;
		switch (type)
		{
		case CBaseOptions::NO_RGB:
			break;

		case CBaseOptions::OLD_NATURAL:     pix_val = Color8(max(0.0, min(254.0, (((double)at(B3) - 2 * 90.0) / (2 * (1000.0 - 90.0))) * 254.0)));       break;
			//case CBaseOptions::LANDWATER:
			  //  pix_val = Color8(max(0.0, min(254.0, (((double)at(B4) + 150.0) / 6150.0) * 254.0)));
		//        break;
		case CBaseOptions::LANDWATER: pix_val = Color8(max(0.0, min(1.0, ((double)at(B4) - stats[B4].m_min) / (stats[B4].m_max - stats[B4].m_min))) * 254); break;
		case CBaseOptions::NATURAL:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B4) - (stats[B4].m_mean - KK * stats[B4].m_sd)) / (2 * KK * stats[B4].m_sd))), 1) * 254.0); break;
		case CBaseOptions::TRUE_COLOR:pix_val = Color8(max(0.0, min(1.0, ((double)at(B1) - 7640) / ((double)8697.0 - 7640.0))) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B1) - (stats[B1].m_min+ (stats[B1].m_max - stats[B1].m_min) * 0.02)) / (stats[B1].m_max- stats[B1].m_min)*0.96)), 1) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR:pix_val = Color8(max(0.0, min(1.0, ((double)at(B4) - 6727) / (29091 - 6727))) * 254.0); break;

				//pix_val = Color8(max(0.0, min(1.0, ((double)at(B4) - stats[B4].m_min) / (stats[B4].m_max - stats[B4].m_min))) * 254); break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B3) - stats[B3].m_min) / (stats[B3].m_max- stats[B3].m_min))), 0.5)* 254.0); break;
		   // case CBaseOptions::TRUE_COLOR:
			 //   pix_val = Color8(max(0.0, min(1.0, (pow(max(0.0, min(1.0, ((double)at(B4) - stats[B4].m_min) / (stats[B4].m_max - stats[B4].m_min))), 0.5) * 254 - 25.0) / (128.0 - 25.0))) * 254.0);
			   // break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B4) - (stats[B4].m_mean - 1*stats[B4].m_sd)) / (2*stats[B4].m_sd))), 0.5)* 254.0); break;

		default:
			assert(false);
		}

		return pix_val;
	}

	Color8 CLandsatPixel::G(CBaseOptions::TRGBTye type, const CBandStats& stats)const
	{
		Color8 pix_val = 255;
		switch (type)
		{
		case CBaseOptions::NO_RGB:
			break;


		case CBaseOptions::OLD_NATURAL: pix_val = Color8(max(0.0, min(254.0, (((double)at(B2) - 2 * 170.0) / (2 * (1050.0 - 170.0))) * 254.0))); break;
			//case CBaseOptions::LANDWATER:
			  //  pix_val = Color8(max(0.0, min(254.0, (((double)at(B5) + 190.0) / 5190.0) * 254.0)));
		//        break;
		case CBaseOptions::LANDWATER: pix_val = Color8(max(0.0, min(1.0, ((double)at(B5) - stats[B5].m_min) / (stats[B5].m_max - stats[B5].m_min))) * 254); break;
		case CBaseOptions::NATURAL:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B5) - (stats[B5].m_mean - KK * stats[B5].m_sd)) / (2 * KK * stats[B5].m_sd))), 1) * 254.0); break;
		case CBaseOptions::TRUE_COLOR:pix_val = Color8(max(0.0, min(1.0, ((double)at(B2) - 7640) / ((double)9570.0 - 7640.0))) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B2) - (stats[B2].m_min + (stats[B2].m_max - stats[B2].m_min) * 0.02)) / (stats[B2].m_max - stats[B2].m_min) * 0.96)), 1) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(max(0.0, min(1.0, ((double)at(B5) - 6582) / (25455 - 6582))) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B2) - stats[B2].m_min) / (stats[B2].m_max - stats[B2].m_min))), 0.5)* 254.0); break;
		   // case CBaseOptions::TRUE_COLOR:
			 //   pix_val = Color8(max(0.0, min(1.0, (pow(max(0.0, min(1.0, ((double)at(B3) - stats[B3].m_min) / (stats[B3].m_max - stats[B3].m_min))), 0.5) * 254 - 25.0) / (128.0 - 25.0))) * 254.0);
			   // break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B3) - (stats[B3].m_mean - 1*stats[B3].m_sd)) / (2*stats[B3].m_sd))), 0.5)* 254.0); break;

		default:
			assert(false);
		}

		return pix_val;


	}
	Color8 CLandsatPixel::B(CBaseOptions::TRGBTye type, const CBandStats& stats)const
	{
		Color8 pix_val = 255;
		switch (type)
		{
		case CBaseOptions::NO_RGB:
			break;
		case CBaseOptions::OLD_NATURAL:     pix_val = Color8(max(0.0, min(254.0, (((double)at(B1) - 2 * 130.0) / (2 * (780.0 - 130.0))) * 254.0)));        break;
			//case CBaseOptions::LANDWATER:
			  //  pix_val = Color8(max(0.0, min(254.0, (((double)at(B3) + 200.0) / 2700.0) * 254.0)));
				//break;
		case CBaseOptions::LANDWATER: pix_val = Color8(max(0.0, min(1.0, ((double)at(B3) - stats[B3].m_min) / (stats[B3].m_max - stats[B3].m_min))) * 254); break;
		case CBaseOptions::NATURAL:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B3) - (stats[B3].m_mean - KK * stats[B3].m_sd)) / (2 * KK * stats[B3].m_sd))), 1) * 254.0); break;
		case CBaseOptions::TRUE_COLOR:pix_val = Color8(max(0.0, min(1.0, ((double)at(B3) - 7510.0) / ((double)9300.0 - 7510.0))) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR:pix_val = Color8(pow(max(0.0, min(1.0, ((double)at(B3) - (stats[B3].m_min + (stats[B3].m_max - stats[B3].m_min) * 0.02)) / (stats[B3].m_max - stats[B3].m_min) * 0.96)), 1) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR: pix_val = Color8(max(0.0, min(1.0, ((double)at(B3) - 6545) / (16364 - 6545))) * 254.0); break;
			//case CBaseOptions::TRUE_COLOR:
			  //  pix_val = Color8(max(0.0, min(1.0, (pow(max(0.0, min(1.0, ((double)at(B2) - stats[B2].m_min) / (stats[B2].m_max - stats[B2].m_min))), 0.5) * 254 - 25.0) / (128.0 - 25.0))) * 254.0);
			   //break;

			//case CBaseOptions::TRUE_COLOR:
		default:
			assert(false);
		}

		return pix_val;


	}

	double CLandsatPixel::GetEuclideanDistance(const CLandsatPixel& pixel, CBaseOptions::TRGBTye type)const
	{
		double r = type != CBaseOptions::NO_RGB ? R(type) - pixel.R(type) : at(B4) - pixel[B4];
		double g = type != CBaseOptions::NO_RGB ? G(type) - pixel.G(type) : at(B5) - pixel[B5];
		double b = type != CBaseOptions::NO_RGB ? B(type) - pixel.B(type) : at(B3) - pixel[B3];

		return sqrt(r * r + g * g + b * b);
	}


	double CLandsatPixel::NBR()const
	{
		return ((double)at(B4) - at(B7)) / max(0.1, double(at(B4)) + at(B7));
	}


	double CLandsatPixel::NDVI()const
	{
		return ((double)at(B4) - at(B3)) / max(0.1, double(at(B4)) + at(B3));
	}

	double CLandsatPixel::NDMI()const
	{
		return ((double)at(B4) - at(B5)) / max(0.1, double(at(B4)) + at(B5));
	}

	double CLandsatPixel::NDWI()const
	{
		return ((double)at(B4) - at(B5)) / max(0.1, double(at(B4)) + at(B5));
	}

	double CLandsatPixel::NDSI()const
	{
		return ((double)at(B2) - at(B5)) / max(0.1, double(at(B2)) + at(B5));
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

	double CLandsatPixel::ZSW()const
	{
		double b3 = square(max(0.0, at(B3) - 141.4518) / 70.56979);
		double b4 = square(max(0.0, at(B4) - 221.1589) / 147.9847);
		double b5 = square(max(0.0, at(B5) - 91.9588) / 130.7777);
		double b7 = square(max(0.0, at(B7) - 68.39219) / 99.17062);
		double ZSW = sqrt(0.25 * (b3 + b4 + b5 + b7)) * 100;
		return ZSW;

	}

	double CLandsatPixel::NBR2()const
	{
		return ((double)at(B5) - at(B7)) / max(0.1, double(at(B5)) + at(B7));
	}

	double CLandsatPixel::EVI()const
	{
		return 2.5 * ((double)at(B4) - at(B3)) / max(0.1, (at(B4) + 6.0 * at(B3) - 7.5 * at(B1) + 1));
	}

	double CLandsatPixel::EVI2()const
	{
		return 2.5 * ((double)at(B4) - at(B3)) / max(0.1, (at(B4) + 2.4 * at(B3) + 1));
	}


	double CLandsatPixel::SAVI()const
	{
		return 1.5 * ((double)at(B4) - at(B3)) / max(0.1, (at(B4) + at(B3) + 0.5));
	}
	double CLandsatPixel::MSAVI()const
	{
		return (2.0 * at(B4) + 1 - sqrt((2.0 * at(B4) + 1) * (2.0 * at(B4) + 1) - 8 * ((double)at(B4) - at(B3)))) / 2;
	}
	double CLandsatPixel::SR() const
	{
		return ((double)at(B4) / max(0.1, (double)at(B3)));
	}

	double CLandsatPixel::HZ()const
	{
		return ((double)at(B1) / max(0.1, (double)at(B3)));
	}

	double CLandsatPixel::LSWI()const
	{
		return ((double)at(B4) - at(B5)) / max(0.1, double(at(B4) + at(B5)));
	}

	double CLandsatPixel::VIgreen()const
	{
		return ((double)at(B2) - at(B3)) / max(0.1, double(at(B2) + at(B3)));
	}


	/*double CLandsatPixel::GetDespike(double pre, double spike, double post, double min_trigger)
	{
		double d1 = (post - pre);
		if (abs(d1) < min_trigger)
			return 1;

		double d2 = spike - (post + pre) / 2;

		if (d2 == 0)
			return 1;

		return fabs(d1 / d2);
	}*/

}
