//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 19-10-2018	        Rémi Saint-Amant	Creation
//******************************************************************************
#include "stdafx.h"

//#include <time.h>
#include "Basic/Statistic.h"
//#include "Basic/CSV.h"
#include "Basic/Callback.h"
//#include "Basic/OpenMP.h"
//#include "Basic/WaterTemperature.h"
#include "Basic/CSV.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/GDAL.h"
#include "Geomatic/SfcGribsDatabase.h"
//#include "cctz/time_zone_info.h"


#include "WeatherBasedSimulationString.h"


using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	//*********************************************************************
	ERMsg CGribsDB::load(const std::string& file_path)
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(file_path);





		if (msg)
		{
			string path = GetPath(file_path);
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					CTRef TRef;
					TRef.FromFormatedString((*loop)[0], "", "-", 1);
					assert(TRef.IsValid());

					insert(make_pair(TRef, GetAbsolutePath(path, (*loop)[1])));
				}
			}
		}

		return msg;
	}

	ERMsg CGribsDB::save(const std::string& file_path)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(file_path);
		if (msg)
		{
			string path = GetPath(file_path);
			file << "TRef,FilePath";
			for (auto it = begin(); it != end(); it++)
			{
				file << it->first.GetFormatedString("%Y-%m-%d-%H") << ",";
				file << GetRelativePath(path, it->second) << endl;
			}

			file.close();
		}

		return msg;
	}


	CTPeriod CGribsDB::GetEntireTPeriod()const
	{
		CTPeriod p;
		std::set<CTRef> TRefs = GetAllTRef();
		if (!TRefs.empty())
			p = CTPeriod(*TRefs.begin(), *TRefs.rbegin());

		return p;
	}

	std::set<CTRef> CGribsDB::GetAllTRef()const
	{
		std::set<CTRef> TRefs;
		ASSERT(!empty());
		if (!empty())
		{
			for (const_iterator it = begin(); it != end(); it++)
				TRefs.insert(it->first);
		}

		return TRefs;
	}

	//*********************************************************************

	ERMsg CIncementalDB::load(const std::string& file_path)
	{
		ERMsg msg;

		clear();

		if (FileExists(file_path))
		{
			ifStream file;
			msg = file.open(file_path);
			if (msg)
			{
				for (CSVIterator loop(file, ",", true); loop != CSVIterator(); ++loop)
				{
					if (loop->size() == 5)
					{
						string ID = (*loop)[0];
						CTRef TRef;
						TRef.FromFormatedString((*loop)[0], "%Y-%m-%d-%H");
						CFileStamp stamp;
						stamp.m_filePath = (*loop)[1];
						stamp.m_time = stoi((*loop)[2]);
						stamp.m_size = stoi((*loop)[3]);
						stamp.m_attribute = stoi((*loop)[4]);

						insert(make_pair(TRef, stamp));
					}
				}

				file.close();

			}
		}

		return msg;
	}

	ERMsg CIncementalDB::save(const std::string& file_path)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(file_path);
		if (msg)
		{
			file << "Date,Name,LastUpdate,Size,Attribute";
			for (auto it = begin(); it != end(); it++)
			{
				//const CIncementalInfo& rec = at(i);
				file << it->first.GetFormatedString("%Y-%m-%d-%H") << ",";
				file << it->second.m_filePath << ",";
				file << it->second.m_time << ",";
				file << it->second.m_size << ",";
				file << it->second.m_attribute << endl;
			}
			file.close();
		}

		return msg;
	}


	ERMsg CIncementalDB::GetInvalidPeriod(const CGribsDB& gribs, CTPeriod& p_invalid)const
	{
		ERMsg msg;

		p_invalid.Reset();

		std::set<CTRef> TRefs;
		msg = GetInvalidTRef(gribs, TRefs);
		if (msg)
		{
			if (!TRefs.empty())
				p_invalid = CTPeriod(*TRefs.begin(), *TRefs.rbegin());
		}

		return msg;
	}

	ERMsg CIncementalDB::GetInvalidTRef(const CGribsDB& gribs, std::set<CTRef>& invalid)const
	{
		ERMsg msg;

		invalid.clear();
		for (auto it = gribs.begin(); it != gribs.end(); it++)
		{
			const_iterator it_find = find(it->first);
			if (it_find != end())
			{
				CFileStamp info_new;
				msg += info_new.SetFileStamp(it->second);
				if (msg)
				{
					const CFileStamp& info_old = it_find->second;
					if (info_new.m_filePath != info_old.m_filePath ||
						info_new.m_time != info_old.m_time)
					{
						invalid.insert(it->first);
					}

				}
			}
			else
			{
				invalid.insert(it->first);
			}
		}


		return msg;
	}

	ERMsg CIncementalDB::Update(const CGribsDB& gribs)
	{
		ERMsg msg;

		for (auto it = gribs.begin(); it != gribs.end() && msg; it++)
		{
			CFileStamp info_new;
			msg += info_new.SetFileStamp(it->second);
			if (msg)
			{
				at(it->first) = info_new;
			}
		}


		return msg;
	}


	//**************************************************************************************************************
	//CSfcVariableLine

	CSfcVariableLine::CSfcVariableLine(size_t nXBlockSize, int dataType)
	{
		int dataSize = GDALGetDataTypeSize((GDALDataType)dataType) / sizeof(char);
		m_ptr = new char[nXBlockSize * dataSize];
		m_xBlockSize = nXBlockSize;
		m_dataType = dataType;
	}

	//double CSfcVariableLine::GetValue(int x)const
	//{
	//	//size_t pos = size_t(y) * m_xBlockSize + x;
	//	return GetValue(x);
	//}

	double CSfcVariableLine::get_value(size_t x)const
	{
		double value = 0;
		switch (m_dataType)
		{
		case GDT_Byte:		value = double(((char*)m_ptr)[x]); break;
		case GDT_UInt16:	value = double(((unsigned __int16*)m_ptr)[x]); break;
		case GDT_Int16:		value = double(((__int16*)m_ptr)[x]); break;
		case GDT_UInt32:	value = double(((unsigned __int32*)m_ptr)[x]); break;
		case GDT_Int32:		value = double(((__int32*)m_ptr)[x]); break;
		case GDT_Float32:	value = double(((float*)m_ptr)[x]); break;
		case GDT_Float64:	value = double(((double*)m_ptr)[x]); break;
		}

		return value;
	}

	//void CSfcVariableLine::SetValue(int x, double value)
	//{
	//	//size_t pos = size_t(y) * m_xBlockSize + x;
	//	SetValue(x, value);
	//}

	void CSfcVariableLine::set_value(size_t x, double value)
	{
		switch (m_dataType)
		{
		case GDT_Byte:		((char*)m_ptr)[x] = (char)value; break;
		case GDT_UInt16:	((unsigned __int16*)m_ptr)[x] = (unsigned __int16)value; break;
		case GDT_Int16:		((__int16*)m_ptr)[x] = (__int16)value; break;
		case GDT_UInt32:	((unsigned __int32*)m_ptr)[x] = (unsigned __int32)value; break;
		case GDT_Int32:		((__int32*)m_ptr)[x] = (__int32)value; break;
		case GDT_Float32:	((float*)m_ptr)[x] = (float)value; break;
		case GDT_Float64:	((double*)m_ptr)[x] = (double)value; break;
		}
	}


	//	std::mutex CSfcDatasetCached::m_mutex;

		//**************************************************************************************************************
		//CSfcDatasetCached

	CSfcDatasetCached::CSfcDatasetCached()
	{
		m_bands.fill(UNKNOWN_POS);
	}

	void CSfcDatasetCached::get_weather(const CGeoPointIndex& xy, CHourlyData& data)const
	{
		if (m_extents.IsInside(xy))
		{
			CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
			if (!is_cached(xy.m_y))
				me.load_block(xy.m_y);

			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (m_bands[v] != NOT_INIT)
				{
					assert(m_lines[xy.m_y] != nullptr);
					assert(is_block_inside(xy.m_y));

					data[v] = m_lines[xy.m_y]->at(v)->get_value(xy.m_x);



					switch (v)
					{
					//case H_PRCP:	ASSERT(m_units[v]=="kg/(m^2)"); data[v] = float(data[v]*3600.0); break; //[kg/(m²s)] --> [mm/hour]
					case H_WNDS:	ASSERT(m_units[v] == "[m/s]"); data[v] = float(data[v] * 3600.0 / 1000.0); break; //[m/s] --> [km/h]
					case H_PRES:	ASSERT(m_units[v] == "[Pa]"); data[v] = float(data[v] / 100.0); break; //[Pa] --> [hPa]
					case H_SNDH:	ASSERT(m_units[v] == "[m]"); data[v] = float(data[v] / 100.0); break; //[m] --> [cm]	
					}

					//switch (v)
					//{
					//case V_TMIN:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
					//case V_TMAX:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
					//case V_PRCP:	dataII[ii] = (float)(data[i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
					////case V_SPEH:	dataII[ii] = data[i];  break;
					//case V_SPEH:	dataII[ii] = (float)(data[i] * 1000); break; //kg[H2O]/kg[air] --> g[H2O]/kg[air]
					//case V_WNDS:	dataII[ii] = (float)kph(meters_per_second(data[i])).get(); break; //m/s --> km/h
					//default: ASSERT(false);
					//}
				}
				else if (v == H_WNDS || v == H_WNDD)
				{
					if (m_bands[H_ADD1] != NOT_INIT && m_bands[H_ADD2] != NOT_INIT)
					{
						double U = m_lines[xy.m_y]->at(H_ADD1)->get_value(xy.m_x);
						double V = m_lines[xy.m_y]->at(H_ADD2)->get_value(xy.m_x);
						if (v == H_WNDS )
							data[v] = (float)sqrt(U*U+V*V);
						else if( v == H_WNDD)
							data[v] = (float)Rad2Deg(atan2(U, V));
					}
				}
				else if (v == H_SRAD)
				{
					if (m_bands[H_ADD1] != NOT_INIT && m_bands[H_ADD2] != NOT_INIT)
					{
						double U = m_lines[xy.m_y]->at(H_ADD1)->get_value(xy.m_x);
						double V = m_lines[xy.m_y]->at(H_ADD2)->get_value(xy.m_x);
						if (v == H_WNDS)
							data[v] = (float)sqrt(U*U + V * V);
						else if (v == H_WNDD)
							data[v] = (float)Rad2Deg(atan2(U, V));
					}
				}

			}
		}
	}

	float CSfcDatasetCached::get_variable(const CGeoPointIndex& xy, size_t v)const
	{
		ASSERT(m_extents.m_yBlockSize == 1);
		ASSERT(v < m_bands.size());

		size_t b = get_band(v);
		if (!m_extents.IsInside(xy) || b == UNKNOWN_POS)
			return WEATHER::MISSING;

		//CGeoBlockIndex ij = m_extents.GetBlockIndex(xy);
		ASSERT(m_extents.GetBlockIndex(xy).m_y == xy.m_y);//block y index is data y index

		//CGeoPointIndex xy = xy - m_extents.GetBlockRect(ij.m_x, ij.m_y).UpperLeft();
		//ASSERT(m_extents.GetBlockExtents(ij.m_x, ij.m_y).GetPosRect().IsInside(xy));

		CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
		if (!is_cached(xy.m_y))
			me.load_block(xy.m_y);

		assert(m_lines[xy.m_y]);
		assert(is_block_inside(xy.m_y));

		return m_lines[xy.m_y]->at(v)->get_value(xy.m_x);

	}



	size_t CSfcDatasetCached::get_var(const string& strVar)
	{
		size_t var = UNKNOWN_POS;

		if (strVar == "TMP")//temperature [C]
			var = H_TAIR;
		//else if (strVar == "PRATE")//Precipitation rate [kg/(m^2 s)]
		else if (strVar == "APCP")//Total precipitation [kg/(m^2)]
			var = H_PRCP;
		else if (strVar == "DPT")//dew point [C]
			var = H_TDEW;
		else if (strVar == "RH")//relative humidity [%]
			var = H_RELH;
		else if (strVar == "UGRD")//u wind [m/s]
			var = H_ADD1;
		else if (strVar == "VGRD")//u wind [m/s]
			var = H_ADD2;
		else if (strVar == "WIND")//wind speed [m/s]
			var = H_WNDS;
		else if (strVar == "WDIR")//wind direction [deg true]
			var = H_WNDD;
		///* 232 */ {"DTRF", "Downward total radiation flux [W/m^2]"},
		///* 115 */ {"LWAVR", "Long wave [W/m^2]"},
		///* 116 */{ "SWAVR", "Short wave [W/m^2]" },
		else if (strVar == "GRAD")//Global radiation [W/m^2]
			var = H_SRAD;
		else if (strVar == "DSWRF")//Downward short-wave radiation flux [W/(m^2)]
			var = H_EXRA;//it only to get a place: to be revised
		else if (strVar == "DLWRF")//Downward long-wave radiation flux [W/(m^2)]
			var = H_SWRA;//it only to get a place: to be revised
		else if (strVar == "PRES")//Pressure [Pa]
			var = H_PRES;
		//		else if (strVar == "SRWEQ")//Snowfall rate water equiv. [kg/m^2/s]
			//		var = H_SNOW;
		else if (strVar == "ASNOW")//Frozen precipitation (e.g. snowfall) [Kg/m^2]   ou Total snowfall [m]????
			var = H_SNOW;
		else if (strVar == "WEASN")//Water equivalent snow [Kg/m^2]
			var = H_SNOW;
		//"ARAIN", "Liquid precipitation (rainfall) [Kg/m^2]"},
		else if (strVar == "SNOD")//Snow depth[m]
			var = H_SNDH;
		else if (strVar == "WEASD")//Water equivalent of accumulated snow depth [kg/(m^2)]
			var = H_SWE;

		//GRDPS: WE AFR, WE APE, WE ARN, WE ASN




		return var;
	}

	bool CSfcDatasetCached::is_sfc(const string& strLevel)
	{
		bool bSfc = false;


		if (strLevel == "sfc" || strLevel == "surface" || strLevel == "2 m above ground" || strLevel == "2 m above gnd" || strLevel == "10 m above ground" || strLevel == "10 m above gnd")
		{
			bSfc = true;
		}

		return bSfc;
	}

	std::string GetDefaultUnit(size_t var)
	{
		std::string unit;
		switch (var)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX: 
		case H_TDEW: unit = "[C]";  break;
		case H_SNOW:
		case H_SWE:
		case H_PRCP: unit = "[kg/(m^2)]";  break;
		case H_RELH: unit = "[%]";  break;
		case H_ADD1:
		case H_ADD2:
		case H_WNDS:
		case H_WND2: unit = "[m/s]";  break;
		case H_WNDD: unit = "[deg true]";  break;
		case H_SRAD: unit = "[W/(m^2)]";  break;
		case H_PRES: unit = "[Pa]";  break;
		case H_SNDH: unit = "[m]";  break;
		}

		return unit;
	}

	ERMsg CSfcDatasetCached::open(const std::string& filePath, bool bOpenInv)
	{
		ERMsg msg;

		msg = CGDALDatasetEx::OpenInputImage(filePath);
		if (msg)
		{
			init_cache();

			//Load band positions (not the same for all images
			if (bOpenInv)
			{
				string invFilePath(filePath);
				SetFileExtension(invFilePath, ".inv");
				
				if (m_bVRT)
				{
					//HRDPS gribs
					//find variable from file name
					ASSERT(m_internalName.size() == GetRasterCount());
					for (size_t i = 0; i < m_internalName.size(); i++)
					{
						//PRATE is only for forecast hours, not for the analyses hour... hummm
						string title = GetFileTitle(m_internalName[i]);
						StringVector tmp(title, "_");
						ASSERT(tmp.size() == 9);

						string strVar = tmp[3];
						string strType = tmp[4];
						string strLevel = tmp[5];
						size_t var = get_var(strVar);

						if (var < NB_VAR_EX  && m_variables.test(var))
						{
							bool bScf = false;
							if (strType == "SFC" || strType == "TGL")
							{
								if (strLevel == "0" || strLevel == "2" || strLevel == "10")
									bScf = true;
							}

							if (bScf)
							{
								m_bands[var] = i;
								m_units[var] = GetDefaultUnit(var);
							}

						}
					}
				}
				else
				{
					//get info from metadata
					BandsMetaData meta_data;

					GetBandsMetaData(meta_data);
					ASSERT(meta_data.size() == GetRasterCount());


					for (size_t i = 0; i < meta_data.size(); i++)
					{

						string strVar = meta_data[i]["GRIB_ELEMENT"];
						string strName = meta_data[i]["GRIB_SHORT_NAME"];
						string strUnit = meta_data[i]["GRIB_UNIT"];
						StringVector description(meta_data[i]["description"], " =[]\"");
						if (description.size() > 3 && (description[2] == "ISBL" || description[2] == "SFC" || description[2] == "HTGL" || description[2] == "HYBL"))
						{
							if (!description.empty() && description[0].find('-') != NOT_INIT)
								description.clear();

							//description.empty();

							size_t var = get_var(strVar);
							if (var < NB_VAR_EX  && m_variables.test(var) && !description.empty())
							{
								bool bSfc = false;

								if (description[2] == "SFC" || description[2] == "HTGL")
								{
									size_t level = as<size_t>(description[0]);
									if (level <= 10)
									{
										bSfc = true;
									}
								}

								if (bSfc)
								{
									m_bands[var] = i;
									m_units[var] = strUnit;
								}
							}
						}
					}


					/*if (FileExists(invFilePath))
				{
					ifStream file;
					msg = file.open(invFilePath);
					if (msg)
					{
						size_t i = 0;
						for (CSVIterator loop(file, ":", false); loop != CSVIterator() && msg; ++loop, i++)
						{

							ASSERT(loop->size() >= 6);
							if (loop->size() >= 6)
							{
								string strVar = (*loop)[3];
								string strLevel = (*loop)[4];
								size_t var = get_var(strVar);

								if (var < m_variables.size() && m_variables.test(var))
								{
									bool bSfc = is_sfc(strLevel);
									if (bSfc)
									{
										m_bands[var] = i;
										m_units[var] = GetDefaultUnit(var);
									}
								}
							}
							else
							{
								if (!TrimConst(loop->GetLastLine()).empty())
									msg.ajoute("Bad .inv file : " + invFilePath);
							}
						}
					}
				}
				else*/
				//	{
				}
				//}
			}
		}


		return msg;
	}

	void CSfcDatasetCached::close()
	{
		if (CGDALDatasetEx::IsOpen())
		{
			m_lines.clear();
			Dataset()->FlushCache();
			CGDALDatasetEx::Close();
		}
	}

	void CSfcDatasetCached::init_cache()const
	{
		assert(IsOpen());
		if (!is_cache_init())
		{
			CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
			ASSERT(m_extents.XNbBlocks() == 1);
			me.m_lines.resize(m_extents.YNbBlocks());
		}
	}

	void CSfcDatasetCached::load_block(size_t y)
	{
		//		m_mutex.lock();
		if (!is_cached(y))
		{
			m_lines[y].reset(new CSfcWeatherLine);

			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (m_bands[v] != NOT_INIT)
				{
					size_t b = m_bands[v];

					assert(m_lines[y]->at(v) == NULL);

					//CTimer readTime(TRUE);

					//copy part of the date
					CGeoExtents ext = GetExtents().GetBlockExtents(0, int(y));
					//CGeoRectIndex ind = GetExtents().CoordToXYPos(ext);

					GDALRasterBand* poBand = m_poDataset->GetRasterBand(int(b) + 1);//one base in direct load

					if (m_bVRT)
					{
						int nXBlockSize = m_extents.m_xBlockSize;
						int nYBlockSize = m_extents.m_yBlockSize;


						GDALDataType type = poBand->GetRasterDataType();
						if (type == GDT_Float64)
							type = GDT_Float32;

						CSfcVariableLine* pBlockTmp = new CSfcVariableLine(nXBlockSize, type);
						poBand->RasterIO(GF_Read, 0 * nXBlockSize, int(y) * 1, nXBlockSize, 1, pBlockTmp->m_ptr, nXBlockSize, 1, type, 0, 0);
						poBand->FlushBlock(0, int(y));
						poBand->FlushCache();

						m_lines[y]->at(v).reset(pBlockTmp);
					}
					else
					{
						int nXBlockSize, nYBlockSize;
						poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

						ASSERT(nXBlockSize == m_extents.m_xBlockSize);
						ASSERT(nYBlockSize == m_extents.m_yBlockSize);
						GDALDataType type = poBand->GetRasterDataType();
						CSfcVariableLine* pBlockTmp = new CSfcVariableLine(nXBlockSize, type);
						poBand->ReadBlock(0, int(y), pBlockTmp->m_ptr);
						poBand->FlushBlock(0, int(y));
						poBand->FlushCache();

						if (type == GDT_Float64)
						{
							CSfcVariableLine* pBlockData = new CSfcVariableLine(nXBlockSize, GDT_Float32);
							for (size_t x = 0; x < nXBlockSize; x++)
								pBlockData->set_value(x, pBlockTmp->get_value(x));

							delete pBlockTmp;
							pBlockTmp = pBlockData;
						}

						m_lines[y]->at(v).reset(pBlockTmp);

					}

					//readTime.Stop();
					//m_stats[0] += readTime.Elapsed();
				}
			}
		}
		//	m_mutex.unlock();
		ASSERT(is_cached(y));
	}


	void CSfcDatasetCached::get_weather(const CGeoPoint& pt, CHourlyData& data)const
	{
		static const double POWER = 1;

		//always coimpute from the 4 nearest points
		CHourlyData4 data4;
		get_4nearest(pt, data4);

		//compute weight
		array<CStatistic, NB_VAR_EX> sumV;
		array<CStatistic, NB_VAR_EX> sumP;
		//array<array<array<double, NB_VAR_EX>, 2>, 2> weight = { 0 };

		const CGeoExtents& extents = GetExtents();
		ASSERT(extents.IsInside(pt));

		CGeoPointIndex xy1 = get_ul(pt);

		double nearestD = DBL_MAX;

		CGeoPointIndex nearest;
		for (size_t y = 0; y < 2; y++)
		{
			for (size_t x = 0; x < 2; x++)
			{
				CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
				CGeoPoint pt2 = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

				if (pt2.IsInit())
				{
					double d_xy = max(1.0, pt2.GetDistance(pt));//limit to 1 meters to avoid division by zero
					double p1 = 1 / pow(d_xy, POWER);

					if (d_xy < nearestD)
					{
						nearestD = d_xy; nearest = xy2;
					}

					for (size_t v = 0; v < NB_VAR_EX; v++)
					{
						if (data4[y][x][v] > -999)
						{
							sumV[v] += data4[y][x][v] * p1;
							sumP[v] += p1;
							//weight[x][y][v] = p1;
						}
					}
				}//is init
			}//x
		}//y


		if (nearest.m_x >= 0 && nearest.m_y >= 0)
		{
			//if (!bSpaceInterpol)
			//{
			//	//take the nearest point
			//	for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
			//	{
			//		if (me[nearest.m_z][nearest.m_y][nearest.m_x][v] > -999)
			//		{
			//			sumV[v] = me[nearest.m_z][nearest.m_y][nearest.m_x][v];
			//			sumP[v] = 1;
			//		}
			//	}
			//}



			//mean of 
			for (size_t v = 0; v < NB_VAR_EX; v++)
			{
				if (sumP[v].IsInit())
				{
					data[v] = sumV[v][SUM] / sumP[v][SUM];
					ASSERT(!_isnan(data[v]) && _finite(data[v]));
				}
			}
		}

	}


	//bool Verif(sumP)
	//{
	//	for (size_t v = 0; v < NB_VAR_EX; v++)
	//	{
	//		if (sumP[v].IsInit())
	//		{
	//			for (size_t y = 0; y < 2; y++)
	//			{
	//				for (size_t x = 0; x < 2; x++)
	//				{
	//					//					weight[x][y][v] =/ sumP[v][SUM];
	//					sumverif += weight[x][y][v] / sumP[v][SUM]
	//				}
	//			}

	//			ASSERT(fabs(sumverif - 1) < 0.0001);
	//		}
	//	}
	//}


	CGeoPointIndex CSfcDatasetCached::get_ul(const CGeoPoint& ptIn)const
	{
		CGeoExtents extents = GetExtents();
		CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

		return extents.CoordToXYPos(pt);//take the lower

	}


	void CSfcDatasetCached::get_nearest(const CGeoPoint& pt, CHourlyData& data)const
	{
		ASSERT(IsOpen());


		const CGeoExtents& extents = GetExtents();
		CGeoPointIndex xy = extents.CoordToXYPos(pt);
		if (xy.m_x < extents.m_xSize && xy.m_y < extents.m_ySize)
		{
			get_weather(xy, data);
		}//if valid position

	}


	void CSfcDatasetCached::get_4nearest(const CGeoPoint& pt, CHourlyData4& data4)const
	{
		ASSERT(IsOpen());


		const CGeoExtents& extents = GetExtents();
		CGeoPointIndex xy1 = get_ul(pt);

		for (size_t y = 0; y < 2; y++)
		{
			for (size_t x = 0; x < 2; x++)
			{
				CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
				if (xy2.m_x < extents.m_xSize && xy2.m_y < extents.m_ySize)
				{

					CGeoPoint pt2 = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell
					get_weather(xy2, data4[y][x]);
				}//if valid position
			}//x
		}//y
	}



	//*********************************************************************************************************
	//CSfcDatasetMap

	//CSfcDatasetMap::CSfcDatasetMap()
	//{
	//}

	//ERMsg CSfcDatasetMap::load(CTRef TRef, const string& filePath, CCallback& callback)const
	//{
	//	ERMsg msg;
	//	CSfcDatasetMap& me = const_cast<CSfcDatasetMap&>(*this);

	//	//me[TRef].reset(new CSfcDatasetCached);
	//	msg = me.OpenInputImage(filePath, true);

	//	return msg;
	//}

	//ERMsg CSfcDatasetMap::discard(CCallback& callback)
	//{
	//	ERMsg msg;

	////	if (!empty())
	//	//{
	//		//CTRef TRef = begin()->first;
	//		//callback.PushTask("Discard weather for " + TRef.GetFormatedString("%Y-%m-%d") + " (nbImages=" + ToString(size()) + ")", size());

	//		//for (iterator it = begin(); it != end() && msg;)
	//		//{
	//			Close();
	//			//it->second->Close();
	//			//it = erase(it);
	//			//msg += callback.StepIt();
	//		//}

	//		//callback.PopTask();
	//	//}

	//	return msg;
	//}

	//void CSfcDatasetMap::get_weather(CTRef TRef, const CGeoPointIndex& xy, CHourlyData& data)const
	//{
	//	ASSERT(at(TRef));

	//	at(TRef)->get_weather(xy, data);
	//}

	//const CGeoExtents& CSfcDatasetMap::get_extents(CTRef TRef)const
	//{
	//	ASSERT(at(TRef));
	//	ASSERT(at(TRef)->IsOpen());

	//	const CGeoExtents& extents = at(TRef)->GetExtents();
	//	return extents;
	//}

	//bool CSfcDatasetMap::is_loaded(CTRef TRef)const
	//{

	//	bool bRep = false;
	//	if (find(TRef) != end())
	//		bRep = at(TRef)->IsOpen();

	//	return bRep;
	//}

	//size_t CSfcDatasetMap::get_band(CTRef TRef, size_t v)const
	//{
	//	ASSERT(at(TRef));
	//	size_t b = at(TRef)->get_band(v);
	//	return b;
	//}



	//**************************************************************************************************************
	//CSfcWeather

	//void CSfcWeather::get_weather(const CGeoPoint& pt, CHourlyData& data)const
	//{
	//	static const double POWER = 1;

	//	CHourlyData4 data4;
	//	get_weather(pt, TRef, data4);

	//	//const CSfcWeather& me = *this;
	//	array<CStatistic, NB_VAR_EX> sumV;
	//	array<CStatistic, NB_VAR_EX> sumP;
	//	array<array<array<double, NB_VAR_EX>, 2>, 2> weight = { 0 };

	//	const CGeoExtents& extents = m_p_weather_DS.get_extents(TRef);
	//	CGeoPointIndex xy1 = get_ul(pt, TRef);

	//	double nearestD = DBL_MAX;
	//	//double nearestZ = DBL_MAX;
	//	CGeoPointIndex nearest;
	//	for (size_t y = 0; y < 2; y++)
	//	{
	//		for (size_t x = 0; x < 2; x++)
	//		{
	//			CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
	//			CGeoPoint pt2 = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

	//			if (pt2.IsInit())
	//			{
	//				//to avoid lost weight of elevation with distance, we compute 2 weight, one for distance and one for delta elevation
	//				double d_xy = max(1.0, pt2.GetDistance(pt));//limit to 1 meters to avoid division by zero

	//				double p1 = 1 / pow(d_xy, POWER);

	//				if (d_xy < nearestD)
	//				{
	//					nearestD = d_xy;
	//				}

	//				for (size_t v = 0; v < NB_VAR_EX; v++)
	//				{
	//					if (data4[y][x][v] > -999)
	//					{
	//						sumV[v] += data4[y][x][v] * p1;
	//						sumP[v] += p1;
	//						weight[x][y][v] = p1;
	//					}
	//				}
	//			}//is init
	//		}//x
	//	}//y


	//	if (nearest.m_x >= 0 && nearest.m_y >= 0)
	//	{
	//		//if (!bSpaceInterpol)
	//		//{
	//		//	//take the nearest point
	//		//	for (size_t v = 0; v < NB_ATM_VARIABLES; v++)
	//		//	{
	//		//		if (me[nearest.m_z][nearest.m_y][nearest.m_x][v] > -999)
	//		//		{
	//		//			sumV[v] = me[nearest.m_z][nearest.m_y][nearest.m_x][v];
	//		//			sumP[v] = 1;
	//		//		}
	//		//	}
	//		//}

	//		double sumverif = 0;

	//		for (size_t y = 0; y < 2; y++)
	//		{
	//			for (size_t x = 0; x < 2; x++)
	//			{
	//				for (size_t v = 0; v < NB_VAR_EX; v++)
	//				{
	//					weight[x][y][v] /= sumP[v][SUM];
	//				}

	//				sumverif += weight[x][y][H_TAIR];
	//			}
	//		}


	//		ASSERT(fabs(sumverif - 1) < 0.0001);

	//		//mean of 
	//		for (size_t v = 0; v < NB_VAR_EX; v++)
	//		{
	//			if (sumP[v].IsInit())
	//			{
	//				data[v] = sumV[v][SUM] / sumP[v][SUM];
	//				ASSERT(!_isnan(data[v]) && _finite(data[v]));
	//			}
	//		}
	//	}

	//}




	//CGeoPointIndex CSfcWeather::get_ul(const CGeoPoint& ptIn, CTRef TRef)const
	//{
	//	CGeoExtents extents = m_p_weather_DS.get_extents(TRef);
	//	CGeoPoint pt = ptIn - CGeoDistance(extents.XRes() / 2, extents.YRes() / 2, extents.GetPrjID());

	//	return extents.CoordToXYPos(pt);//take the lower

	//}


	//void CSfcWeather::get_weather(const CGeoPoint& pt, CTRef TRef, CHourlyData4& data4)const
	//{
	//	ASSERT(is_loaded(TRef));


	//	const CGeoExtents& extents = m_p_weather_DS.get_extents(TRef);
	//	CGeoPointIndex xy1 = get_ul(pt, TRef);

	//	for (size_t y = 0; y < 2; y++)
	//	{
	//		for (size_t x = 0; x < 2; x++)
	//		{
	//			CGeoPointIndex xy2 = xy1 + CGeoPointIndex((int)x, (int)y);
	//			if (xy2.m_x < extents.m_xSize &&
	//				xy2.m_y < extents.m_ySize)
	//			{

	//				CGeoPoint pt2 = extents.GetPixelExtents(xy2).GetCentroid();//Get the center of the cell

	//				//size_t bGph = m_p_weather_DS.get_band(TRef, ATM_HGT, L);
	//				//double gph = m_p_weather_DS.GetPixel(TRef, bGph, xy2); //geopotential height [m]
	//				//(*cuboids)[i].m_pt[z][y][x].m_z = gph - groundAlt;//groundAlt is equal 0 when is over ground
	//				//ASSERT(z == 1 || pt.m_z >= (*cuboids)[i].m_pt[0][y][x].m_z);
	//				//ASSERT(z == 0 || pt.m_z <= (*cuboids)[i].m_pt[1][y][x].m_z);

	//				//size_t xy = y * 2 + x;
	//				m_p_weather_DS.get_weather(TRef, xy2, data4[y][x]);
	//				//if (v == ATM_PRCP)
	//				//	data[xy][v] *= 3600; //convert mm/s into mm/h
	//			}//if valid position
	//		}//x
	//	}//y
	//}


	//string CSfcWeather::get_image_filepath(CTRef TRef)const
	//{
	//	TTRefFilePathMap::const_iterator it = m_filepath_map.find(TRef);
	//	ASSERT(it != m_filepath_map.end());

	//	string path = ".";
	//	if (m_file_path_gribs.find('/') != string::npos || m_file_path_gribs.find('\\') != string::npos)
	//		path = GetPath(m_file_path_gribs);

	//	return GetAbsolutePath(path, it->second);
	//}

	//ERMsg CSfcWeather::load(const std::string& filepath, CCallback& callback)
	//{
	//	ERMsg msg;

	//	ifStream file;
	//	msg = file.open(filepath);
	//	if (msg)
	//	{

	//		
	//		//init max image to load at the sime time
	//		//m_p_weather_DS.m_max_hour_load = m_world.m_world_param.m_max_hour_load;
	//		//m_p_weather_DS.m_clipRect = CGeoRect(-84, 40, -56, 56, PRJ_WGS_84);// m_world.m_moths_param.m_clipRect;


	//		std::ios::pos_type length = file.length();
	//		callback.PushTask("Load Gribs", length);

	//		for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
	//		{
	//			if ((*loop).size() == 2)
	//			{
	//				StringVector tmp((*loop)[0], "/- :");
	//				ASSERT(tmp.size() >= 4 && tmp.size() <= 6);
	//				if (tmp.size() >= 4)
	//				{
	//					tm timeinfo = { 0 };

	//					if (tmp.size() == 6)
	//						timeinfo.tm_sec = ToInt(tmp[5]);     // seconds after the minute - [0,59] 

	//					if (tmp.size() >= 5)
	//						timeinfo.tm_min = ToInt(tmp[4]);     // minutes after the hour - [0,59] 

	//					timeinfo.tm_hour = ToInt(tmp[3]);    // hours since midnight - [0,23] 
	//					timeinfo.tm_mday = ToInt(tmp[2]);    // day of the month - [1,31] 
	//					timeinfo.tm_mon = ToInt(tmp[1]) - 1;     // months since January - [0,11] 
	//					timeinfo.tm_year = ToInt(tmp[0]) - 1900;

	//					__int64 UTCTime = _mkgmtime(&timeinfo);
	//					m_filepath_map[UTCTime] = (*loop)[1];
	//				}
	//				else
	//				{
	//					callback.AddMessage("Bad time format " + (*loop)[1]);
	//				}
	//			}

	//			msg += callback.SetCurrentStepPos((double)file.tellg());
	//		}

	//		if (msg)
	//			m_file_path_gribs = filepath;


	//		callback.PopTask();
	//	}

	//	return msg;

	//}

	//ERMsg CSfcWeather::discard(CCallback& callback)
	//{
	//	return m_p_weather_DS.discard(callback);
	//}


	//bool CSfcWeather::is_loaded(CTRef TRef)const
	//{
	//	bool bIsLoaded = false;
	//	if (!m_file_path_gribs.empty())
	//		bIsLoaded = m_p_weather_DS.is_loaded(TRef);

	//	return bIsLoaded;
	//}

	//ERMsg CSfcWeather::load_weather(CTRef TRef, CCallback& callback)
	//{
	//	ERMsg msg;


	//	if (!m_p_weather_DS.is_loaded(TRef))
	//	{
	//		
	//		string filePath = get_image_filepath(TRef);
	//		ASSERT(!filePath.empty());
	//		msg = m_p_weather_DS.load(TRef, get_image_filepath(TRef), callback);
	//		if(msg)
	//			m_p_weather_DS[TRef]->set_variables(m_variables);

	//		//if (msg && !m_bHgtOverSeaTested)
	//		//{
	//		//	size_t b = m_p_weather_DS.get_band(TRef, ATM_HGT, 0);
	//		//	if (b != -999)
	//		//	{
	//		//		m_bHgtOverSeaTested = true;
	//		//		//test to see if we have special WRF file with HGT over ground and not over sea level
	//		//		for (int x = 0; x < m_p_weather_DS.at(UTCWeatherTime)->GetRasterXSize() && !m_bHgtOverSea; x++)
	//		//		{
	//		//			//for (int y = 0; y < m_p_weather_DS.Get(UTCTRef)->GetRasterYSize() && !m_bHgtOverSea; y++)
	//		//			if (x < m_p_weather_DS.at(UTCWeatherTime)->GetRasterYSize())
	//		//			{
	//		//				double elev = m_p_weather_DS.at(UTCWeatherTime)->GetPixel(b, CGeoPointIndex(x, x));
	//		//				if (elev > 0)
	//		//					m_bHgtOverSea = true;
	//		//			}
	//		//		}
	//		//	}
	//		//}
	//	}


	//	return msg;
	//}

	//CTPeriod CSfcWeather::GetEntireTPeriod()const
	//{
	//	CTPeriod p;

	//	ASSERT(!m_filepath_map.empty());
	//	if (!m_filepath_map.empty())
	//	{
	//		for (TTRefFilePathMap::const_iterator it = m_filepath_map.begin(); it != m_filepath_map.end(); it++)
	//		{
	//			p += it->first;
	//		}
	//	}

	//	return p;
	//}

	//size_t CSfcWeather::get_prj_ID(CTRef TRef)const
	//{
	//	size_t prjID = NOT_INIT;


	//	if (!m_p_weather_DS.is_loaded(TRef))
	//		m_p_weather_DS.load(TRef, get_image_filepath(TRef), CCallback());

	//	CSfcDatasetMap::const_iterator it = m_p_weather_DS.find(TRef);
	//	if (it != m_p_weather_DS.end())
	//	{
	//		ASSERT(it->second);
	//		prjID = it->second->GetPrjID();
	//	}


	//	return prjID;
	//}




	//******************************************************************************************************

	//ERMsg CATMWorld::LoadWeather(CTRef TRef, const vector<__int64>& weather_time, CCallback& callback)
	//{
	//	ERMsg msg;

	//	//MessageBox(NULL, to_wstring(weather_time.size()).c_str(), L"Load weather", MB_OK);

	//	ASSERT(!weather_time.empty());

	//	vector<__int64> weatherToLoad;
	//	for (size_t i = 0; i < weather_time.size() && msg; i++)
	//	{
	//		if (!m_weather.IsLoaded(weather_time[i]))
	//			weatherToLoad.push_back(weather_time[i]);
	//	}

	//	callback.PushTask("Load weather for " + TRef.GetFormatedString("%Y-%m-%d") + " (nb hours=" + ToString(weatherToLoad.size()) + ")", weatherToLoad.size());

	//	//pre-Load weather for the day
	//	for (size_t i = 0; i < weatherToLoad.size() && msg; i++)
	//	{
	//		CTRef TRef = weatherToLoad[i];
	//		ASSERT(!m_weather.IsLoaded(UTCWeatherTime));

	//		msg += m_weather.LoadWeather(UTCWeatherTime, callback);
	//		if (msg)
	//		{
	//			size_t prjID = m_weather.GetGribsPrjID(UTCWeatherTime); ASSERT(prjID != NOT_INIT);
	//			if (m_GEO2.find(prjID) == m_GEO2.end())
	//				m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
	//			if (m_2GEO.find(prjID) == m_2GEO.end())
	//				m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
	//		}
	//		msg += callback.StepIt();
	//	}


	//	callback.PopTask();

	//	return msg;
	//}

	//ERMsg CATMWorld::Init(CCallback& callback)
	//{
	//	ASSERT(m_DEM_DS.IsOpen());
	//	assert(m_weather.is_init());
	//	assert(!m_moths.empty());
	//	ERMsg msg;

	//	//register projection
	//	if (m_water_DS.IsOpen())
	//	{
	//		size_t prjID = m_water_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
	//		if (m_GEO2.find(prjID) == m_GEO2.end())
	//			const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
	//		if (m_2GEO.find(prjID) == m_2GEO.end())
	//			const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
	//	}

	//	if (m_defoliation_DS.IsOpen())
	//	{
	//		size_t prjID = m_defoliation_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
	//		if (m_GEO2.find(prjID) == m_GEO2.end())
	//			const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
	//		if (m_2GEO.find(prjID) == m_2GEO.end())
	//			const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);
	//	}

	//	size_t prjID = m_DEM_DS.GetPrjID(); ASSERT(prjID != NOT_INIT);
	//	if (m_GEO2.find(prjID) == m_GEO2.end())
	//		const_cast<CATMWorld&>(*this).m_GEO2[prjID] = GetReProjection(PRJ_WGS_84, prjID);
	//	if (m_2GEO.find(prjID) == m_2GEO.end())
	//		const_cast<CATMWorld&>(*this).m_2GEO[prjID] = GetReProjection(prjID, PRJ_WGS_84);

	//	random().Randomize(m_world_param.m_seed);


	//	return msg;
	//}
	////***************************************************************************************************************
	////class CWRFDatabase
	//
	//
	//
	//ERMsg CreateGribsFromText(CCallback& callback)
	//{
	//
	//	ERMsg msg;
	//
	//
	//	//Cells height [m]
	//	CGDALDatasetEx terrain_height;
	//	msg += terrain_height.OpenInputImage("E:\\Travaux\\Bureau\\WRF2013\\Terrain Height.tif");
	//
	//	array<array<float, 252>, 201> height = { 0 };
	//	terrain_height.GetRasterBand(0)->RasterIO(GF_Read, 0, 0, (int)height[0].size(), (int)height.size(), &(height[0][0]), (int)height[0].size(), (int)height.size(), GDT_Float32, 0, 0);
	//
	//
	//	static const char* THE_PRJ = "PROJCS[\"unnamed\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\", 6378137, 298.257223563,AUTHORITY[\"EPSG\", \"7030\"]],AUTHORITY[\"EPSG\", \"6326\"]],PRIMEM[\"Greenwich\", 0],UNIT[\"degree\", 0.0174532925199433],AUTHORITY[\"EPSG\", \"4326\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\", 30],PARAMETER[\"standard_parallel_2\", 60],PARAMETER[\"latitude_of_origin\", 48.000004],PARAMETER[\"central_meridian\", -69],PARAMETER[\"false_easting\", 0],PARAMETER[\"false_northing\", 0],UNIT[\"metre\", 1,AUTHORITY[\"EPSG\", \"9001\"]]]";
	//	msg += CProjectionManager::CreateProjection(THE_PRJ);
	//
	//	size_t prjID = CProjectionManager::GetPrjID(THE_PRJ);
	//	CProjectionPtr prj = CProjectionManager::GetPrj(prjID);
	//
	//	CProjectionTransformation Geo2LCC(PRJ_WGS_84, prjID);
	//
	//
	//	//std::string elevFilepath = "D:\\Travaux\\Brian Sturtevant\\MapInput\\DEMGrandLake4km.tif";
	//	//CGDALDatasetEx elevDS;
	//	//elevDS.OpenInputImage(elevFilepath);
	//
	//	//vector < float > elev(elevDS.GetRasterXSize()* elevDS.GetRasterYSize());
	//	//ASSERT(elev.size()==7257);
	//
	//	//GDALRasterBand* pBand = elevDS.GetRasterBand(0);
	//	//pBand->RasterIO(GF_Write, 0, 0, elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), &(elev[0]), elevDS.GetRasterXSize(), elevDS.GetRasterYSize(), GDT_Float32, 0, 0);
	//
	//	//1-pressure(hPa)
	//	//2-height(m above ground not above sea-level)
	//	//3-temperature(K)
	//	//4-water vapor mixing ratio(g/kg)
	//	//5-u - component wind speed(m/s, positive = eastward)
	//	//6-v - component wind speed(m/s, positive = northward)
	//	//7-w - component wind speed(m/s, positive = upward)
	//	//8-precipitation reaching the ground in the last hour(mm)
	//
	//	static const size_t NB_WRF_LEVEL = 37;
	//	enum TWRFVars { WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_WVMR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_PRCP, NB_WRF_VARS };
	//	static const char* VAR_NAME[NB_WRF_VARS] = { "Pres", "Hght", "Tair", "WVMR", "Uwnd", "Vwnd", "Wwnd", "Prcp" };
	//	CBaseOptions options;
	//	options.m_nbBands = NB_WRF_LEVEL * (NB_WRF_VARS - 1) + 1;//prcp have only one band
	//	options.m_outputType = GDT_Float32;
	//	options.m_createOptions.push_back("COMPRESS=LZW");
	//	options.m_dstNodata = -9999;
	//	//options.m_extents = CGeoExtents(-280642.1,1903575,133144.6,2205765,101,74,101,1,prjID);
	//	options.m_extents = CGeoExtents(-524000, 402000, 488000, -402000, 252, 201, 252, 1, prjID);//attention ici il y a 252 pixel, mais dans le .nc il y en a 253!!! donc le x size n'est pas 4000...
	//	options.m_prj = THE_PRJ;
	//	options.m_bOverwrite = true;
	//	array<array<float, 252>, 201> last_prcp = { 0 };
	//
	//	static const size_t NB_WRF_HOURS = 289;//289 hours
	//	callback.PushTask("Create gribs", NB_WRF_HOURS);
	//
	//	for (size_t h = 0; h < NB_WRF_HOURS&&msg; h++)
	//	{
	//		CTRef UTCRef(2013, JULY, DAY_13, 0);
	//		UTCRef += int(h);
	//
	//		callback.PushTask(UTCRef.GetFormatedString("%Y-%m-%d-%H"), 50652 * NB_WRF_LEVEL);
	//
	//		std::string filePathIn = FormatA("E:\\Travaux\\Bureau\\WRF2013\\WRF\\wrfbud_%03d.txt", h);
	//		CGDALDatasetEx geotif;
	//
	//
	//
	//
	//		string filePathOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
	//		msg += geotif.CreateImage(filePathOut, options);
	//		//create .inv file
	//		string filePathInvIn = "E:\\Travaux\\Bureau\\WRF2013\\WRF\\template.inv";
	//		string filePathInvOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
	//		CopyOneFile(filePathInvIn, filePathInvOut, false);
	//
	//
	//
	//
	//		ifStream file;
	//		msg += file.open(filePathIn);
	//		if (msg)
	//		{
	//			std::string csv_text[NB_WRF_VARS];
	//
	//			//write header
	//			for (size_t j = 0; j < NB_WRF_VARS; j++)
	//			{
	//				csv_text[j] = "KeyID,Latitude,Longitude,Elevation";
	//				for (size_t i = 0; i < NB_WRF_LEVEL; i++)
	//					csv_text[j] += "," + FormatA("sounding%02d", i + 1);
	//
	//				csv_text[j] += "\n";
	//			}
	//
	//			CGeoPointIndex xy;
	//			vector<array<array<array<float, 252>, 201>, NB_WRF_LEVEL>> data(NB_WRF_VARS);
	//			for (size_t j = 0; j < data.size(); j++)
	//				for (size_t s = 0; s < data[j].size(); s++)
	//					for (size_t y = 0; y < data[j][s].size(); y++)
	//						for (size_t x = 0; x < data[j][s][y].size(); x++)
	//							data[j][s][y][x] = -9999;
	//
	//			int i = NB_WRF_LEVEL + 1;
	//			while (!file.eof() && msg)
	//			{
	//				string line;
	//				getline(file, line);
	//				if (!line.empty())
	//				{
	//					//if (i % NB_WRF_LEVEL == 0)//+1 for coordinate
	//					if (i == 38)
	//					{
	//						StringVector str(line, " ");
	//						ASSERT(str.size() == 5);
	//
	//						for (size_t j = 0; j < NB_WRF_VARS; j++)
	//							csv_text[j] += str[2] + "," + str[0] + "," + str[1] + "," + str[4];
	//
	//						CGeoPoint lastPoint(ToDouble(str[1]), ToDouble(str[0]), PRJ_WGS_84);
	//						msg += lastPoint.Reproject(Geo2LCC);
	//
	//						ASSERT(geotif.GetExtents().IsInside(lastPoint));
	//						xy = geotif.GetExtents().CoordToXYPos(lastPoint);
	//
	//
	//						i = 0;//restart cycle
	//
	//					}
	//					else
	//					{
	//						//std::stringstream stream(line);
	//						//fixed field length
	//						StringVector str(line, " ");
	//						//StringVector str;
	//						//str.reserve(NB_WRF_VARS + 1);
	//						//str.resize(NB_WRF_VARS + 1);
	//						//static const size_t FIELD_LENGTH[NB_WRF_VARS + 1] = { 2, 8, 9, 9, 9, 9, 9, 9, 9 };
	//						/*for (size_t j = 0; j < NB_WRF_VARS+1; j++)
	//						{
	//						char buffer[10] = { 0 };
	//						stream.read(buffer, FIELD_LENGTH[j]);
	//						if (strlen(buffer)>0)
	//						str.push_back(buffer);
	//						}
	//						*/
	//						if (str.size() == NB_WRF_VARS)
	//						{
	//							StringVector tmp(str[3], "-");
	//							ASSERT(tmp.size() == 2);
	//
	//							str[3] = tmp[0];
	//							str.insert(str.begin() + 4, tmp[1]);
	//						}
	//						ASSERT(str.size() == NB_WRF_VARS + 1);
	//
	//						size_t sounding = ((i - 1) % NB_WRF_LEVEL);
	//						ASSERT(sounding < NB_WRF_LEVEL);
	//						ASSERT(sounding == ToInt(str[0]) - 1);
	//						for (size_t j = 0; j < NB_WRF_VARS; j++)
	//						{
	//							float v = ToFloat(str[j + 1]);
	//							if (j == WRF_HGHT)
	//							{
	//								//add terrain height when WRF
	//								v += height[xy.m_y][xy.m_x];
	//							}
	//							else if (j == WRF_TAIR)//tmp
	//							{
	//								v -= 273.15f;//convert K to ᵒC
	//							}
	//							else if (j == WRF_PRCP)
	//							{
	//								if (sounding == 0)//modify souding zero let all other the original value
	//									v = max(0.0, (v - last_prcp[xy.m_y][xy.m_x]) / 3600.0);//convert mm/h to mm/s (but, in 2013 version it's cumulative)
	//							}
	//
	//							//v = max(0.0, v / 3600.0 - last_prcp[xy.m_y][xy.m_x]);//convert mm/h to mm/s (but, in 2013 version it cumulative)
	//							//else if (j == WRF_WVMR)
	//							//v /= 3600;//convert mixing ratio in relative humidity
	//
	//							data[j][sounding][xy.m_y][xy.m_x] = v;
	//
	//							csv_text[j] += "," + str[j + 1];
	//						}
	//
	//						if (i == 37)//last record
	//							for (size_t j = 0; j < NB_WRF_VARS; j++)
	//								csv_text[j] += "\n";
	//
	//
	//
	//					}
	//
	//					i++;//next line
	//				}//for all line
	//
	//
	//				msg += callback.StepIt();
	//			}//
	//
	//			ASSERT(i == NB_WRF_LEVEL + 1);
	//
	//			for (size_t j = 0; j < data.size(); j++)
	//			{
	//				size_t size = j == WRF_PRCP ? 1 : data[j].size();//save only one layer of precipitation
	//				for (size_t s = 0; s < size; s++)
	//				{
	//					GDALRasterBand* pBand = geotif.GetRasterBand(j * NB_WRF_LEVEL + s);
	//					for (size_t y = 0; y < data[j][s].size(); y++)
	//						pBand->RasterIO(GF_Write, 0, (int)y, (int)data[j][s][y].size(), 1, &(data[j][s][y][0]), (int)data[j][s][y].size(), 1, GDT_Float32, 0, 0);
	//				}
	//
	//				string path = "E:\\Travaux\\Bureau\\WRF2013\\CSV\\";
	//				string fileTitle = FormatA("%4d_%02d_%02d_%02d_%s", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour(), VAR_NAME[j]);
	//
	//				ofStream fileOut;
	//				msg = fileOut.open(path + fileTitle + ".csv");
	//				if (msg)
	//				{
	//					fileOut.write(csv_text[j]);
	//					fileOut.close();
	//
	//					msg = fileOut.open(path + fileTitle + ".vrt");
	//					if (msg)
	//					{
	//						static const char* VRT_FORMAT =
	//							"<OGRVRTDataSource>\n    <OGRVRTLayer name=\"%s\">\n        <SrcDataSource relativeToVRT=\"1\">%s.csv</SrcDataSource>\n        <GeometryType>wkbPoint</GeometryType>\n        <LayerSRS>WGS84</LayerSRS>\n        <GeometryField encoding = \"PointFromColumns\" x=\"Longitude\" y=\"Latitude\"/>\n    </OGRVRTLayer>\n</OGRVRTDataSource>\n";
	//						fileOut.write(Format(VRT_FORMAT, fileTitle, fileTitle));
	//						fileOut.close();
	//
	//						msg = fileOut.open(path + fileTitle + ".csvt");
	//						if (msg)
	//						{
	//							fileOut.write("Integer,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real,Real");
	//							fileOut.close();
	//						}
	//					}
	//				}
	//			}//for all output file
	//
	//
	//			for (size_t y = 0; y < data[WRF_PRCP][0].size(); y++)
	//				for (size_t x = 0; x < data[WRF_PRCP][0][y].size(); x++)
	//					last_prcp[y][x] = data[WRF_PRCP][1][y][x];//take sounding 1 because sounding 0 was modified
	//
	//		}//if msg
	//
	//
	//
	//
	//		callback.PopTask();
	//
	//		msg += callback.StepIt();
	//	}//for all 193 hours
	//
	//	callback.PopTask();
	//
	//	return msg;
	//}
	//
	////using namespace netCDF;
	////typedef std::unique_ptr < NcFile > NcFilePtr;
	////typedef std::array<NcFilePtr, NB_VARIABLES> NcFilePtrArray;
	//
	//
	//ERMsg CreateGribsFromNetCDF(CCallback& callback)
	//{
	//	enum TWRFLEvels { NB_WRF_LEVEL = 38 };
	//	enum TWRFVars { WRF_PRES, WRF_HGHT, WRF_TAIR, WRF_UWND, WRF_VWND, WRF_WWND, WRF_RELH, WRF_PRCP, NB_WRF_VARS };
	//
	//
	//	GDALSetCacheMax64(2000000000);
	//
	//	ERMsg msg;
	//
	//	CGDALDatasetEx terrain_height;
	//	msg += terrain_height.OpenInputImage("E:\\Travaux\\Bureau\\WRF2013\\Terrain Height.tif");
	//
	//	if (!msg)
	//		return msg;
	//
	//	array<array<float, 252>, 201> height = { 0 };
	//	terrain_height.GetRasterBand(0)->RasterIO(GF_Read, 0, 0, (int)height[0].size(), (int)height.size(), &(height[0][0]), (int)height[0].size(), (int)height.size(), GDT_Float32, 0, 0);
	//
	//
	//
	//
	//	CBaseOptions options;
	//	terrain_height.UpdateOption(options);
	//	options.m_outputType = GDT_Float32;
	//	options.m_createOptions.push_back("COMPRESS=LZW");
	//	options.m_dstNodata = -9999;
	//	options.m_bOverwrite = true;
	//	options.m_nbBands = NB_WRF_LEVEL * (NB_WRF_VARS - 1) + 1;//prcp have only one band
	//
	//	enum { WRF_PHB, WRF_PH, WRF_PB, WRF_P, WRF_T, WRF_U, WRF_V, WRF_W, WRF_QVAPOR, WRF_RAINC, WRF_RAINNC, WRF_PSFC, WRF_T2, WRF_U10, WRF_V10, WRF_Q2, NB_VAR_BASE };
	//	static const char* VAR_NAME[NB_VAR_BASE] = { "PHB", "PH", "PB", "P", "T", "U", "V", "W", "QVAPOR", "RAINC", "RAINNC", "PSFC", "T2", "U10", "V10", "Q2" };
	//
	//
	//	CTRef begin = CTRef(2013, JULY, DAY_13, 0);
	//	CTRef end = CTRef(2013, JULY, DAY_25, 0);
	//	callback.PushTask("Create gribs", (end - begin + 1)*NB_WRF_LEVEL * NB_WRF_VARS);
	//
	//	array<array<float, 252>, 201> last_RAIN_C = { 0 };
	//	array<array<float, 252>, 201> last_RAIN_NC = { 0 };
	//
	//	for (CTRef UTCRef = begin; UTCRef <= end && msg; UTCRef++)
	//	{
	//		//create .inv file
	//		string filePathInvIn = "E:\\Travaux\\Bureau\\WRF2013\\NetCDF\\template.inv";
	//		string filePathInvOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\WRF_%4d_%02d_%02d_%02d.inv", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
	//		CopyOneFile(filePathInvIn, filePathInvOut, false);
	//
	//
	//		CGDALDatasetEx geotifOut;
	//		string filePathOut = FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\WRF_%4d_%02d_%02d_%02d.tif", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
	//		msg += geotifOut.CreateImage(filePathOut, options);
	//
	//		array<CGDALDatasetEx, NB_VAR_BASE> geotifIn;
	//		for (size_t i = 0; i < geotifIn.size() && msg; i++)
	//		{
	//			string filePathIn = WBSF::FormatA("E:\\Travaux\\Bureau\\WRF2013\\NETCDF\\wrfout_d02_%d-%02d-%02d_%02d.nc", UTCRef.GetYear(), UTCRef.GetMonth() + 1, UTCRef.GetDay() + 1, UTCRef.GetHour());
	//			string filePathTmp = WBSF::FormatA("E:\\Travaux\\Bureau\\WRF2013\\TIF2\\tmp\\%s.tif", VAR_NAME[i]);
	//			string command = WBSF::FormatA("\"C:\\Program Files\\QGIS\\bin\\GDAL_translate.exe\" -co \"COMPRESS=LZW\" -stats -ot Float32 -a_srs \"+proj=lcc +lat_1=30 +lat_2=60 +lat_0=48.000004 +lon_0=-69 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84\" -a_ullr -524000 402000 488000 -402000 NETCDF:\"%s\":%s \"%s\"", filePathIn.c_str(), VAR_NAME[i], filePathTmp.c_str());
	//
	//			DWORD exist = 0;
	//			msg = WinExecWait(command.c_str(), "", SW_HIDE, &exist);
	//
	//			if (msg)
	//				msg += geotifIn[i].OpenInputImage(filePathTmp);
	//		}
	//
	//
	//		if (msg)
	//		{
	//
	//			for (int l = 0; l < NB_WRF_LEVEL && msg; l++)
	//			{
	//				array<array<array<float, 252>, 201>, NB_WRF_VARS> data = { 0 };
	//				for (size_t s = 0; s < NB_WRF_VARS&&msg; s++)
	//				{
	//					switch (s)
	//					{
	//					case WRF_PRES:
	//					{
	//						if (l == 0)
	//						{
	//							GDALRasterBand* pBand = NULL;
	//							pBand = geotifIn[WRF_PSFC].GetRasterBand(0);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							array<array<float, 252>, 201> pb = { 0 };
	//							array<array<float, 252>, 201> p = { 0 };
	//							GDALRasterBand* pBand1 = geotifIn[WRF_PB].GetRasterBand(l - 1);
	//							GDALRasterBand* pBand2 = geotifIn[WRF_P].GetRasterBand(l - 1);
	//							pBand1->RasterIO(GF_Read, 0, 0, (int)pb[0].size(), (int)pb.size(), &(pb[0][0]), (int)pb[0].size(), (int)pb.size(), GDT_Float32, 0, 0);
	//							pBand2->RasterIO(GF_Read, 0, 0, (int)p[0].size(), (int)p.size(), &(p[0][0]), (int)p[0].size(), (int)p.size(), GDT_Float32, 0, 0);
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (pb[y][x] + p[y][x]);//add base and perturbation
	//						}
	//
	//						for (size_t y = 0; y < data[s].size(); y++)
	//							for (size_t x = 0; x < data[s][y].size(); x++)
	//								data[s][y][x] /= 100.0f;//convert Pa into mbar
	//
	//						break;
	//					}
	//
	//					case WRF_HGHT:
	//					{
	//						if (l == 0)
	//						{
	//							array<array<float, 252>, 201> phb = { 0 };
	//							array<array<float, 252>, 201> ph = { 0 };
	//							GDALRasterBand* pBand1 = geotifIn[WRF_PHB].GetRasterBand(0);
	//							GDALRasterBand* pBand2 = geotifIn[WRF_PH].GetRasterBand(0);
	//
	//							pBand1->RasterIO(GF_Read, 0, 0, (int)phb[0].size(), (int)phb.size(), &(phb[0][0]), (int)phb[0].size(), (int)phb.size(), GDT_Float32, 0, 0);
	//							pBand2->RasterIO(GF_Read, 0, 0, (int)ph[0].size(), (int)ph.size(), &(ph[0][0]), (int)ph[0].size(), (int)ph.size(), GDT_Float32, 0, 0);
	//
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (phb[y][x] + ph[y][x]) / 9.8;//convert m²/s² into m
	//						}
	//						else
	//						{
	//							array<array<float, 252>, 201> phb1 = { 0 };
	//							array<array<float, 252>, 201> ph1 = { 0 };
	//							array<array<float, 252>, 201> phb2 = { 0 };
	//							array<array<float, 252>, 201> ph2 = { 0 };
	//							GDALRasterBand* pBand1 = geotifIn[WRF_PHB].GetRasterBand(l - 1);
	//							GDALRasterBand* pBand2 = geotifIn[WRF_PH].GetRasterBand(l - 1);
	//							GDALRasterBand* pBand3 = geotifIn[WRF_PHB].GetRasterBand(l);
	//							GDALRasterBand* pBand4 = geotifIn[WRF_PH].GetRasterBand(l);
	//							pBand1->RasterIO(GF_Read, 0, 0, (int)phb1[0].size(), (int)phb1.size(), &(phb1[0][0]), (int)phb1[0].size(), (int)phb1.size(), GDT_Float32, 0, 0);
	//							pBand2->RasterIO(GF_Read, 0, 0, (int)ph1[0].size(), (int)ph1.size(), &(ph1[0][0]), (int)ph1[0].size(), (int)ph1.size(), GDT_Float32, 0, 0);
	//							pBand3->RasterIO(GF_Read, 0, 0, (int)phb2[0].size(), (int)phb2.size(), &(phb2[0][0]), (int)phb2[0].size(), (int)phb2.size(), GDT_Float32, 0, 0);
	//							pBand4->RasterIO(GF_Read, 0, 0, (int)ph2[0].size(), (int)ph2.size(), &(ph2[0][0]), (int)ph2[0].size(), (int)ph2.size(), GDT_Float32, 0, 0);
	//
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = ((phb1[y][x] + phb2[y][x]) / 2 + (ph1[y][x] + ph2[y][x]) / 2) / 9.8;//convert m²/s² into m
	//						}
	//
	//						break;
	//					}
	//					case WRF_TAIR:
	//					{
	//						if (l == 0)
	//						{
	//							GDALRasterBand* pBand = geotifIn[WRF_T2].GetRasterBand(0);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							array<array<float, 252>, 201> T = { 0 };
	//							GDALRasterBand* pBand = geotifIn[WRF_T].GetRasterBand(l - 1);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)T[0].size(), (int)T.size(), &(T[0][0]), (int)T[0].size(), (int)T.size(), GDT_Float32, 0, 0);
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (T[y][x] + 300)*pow(data[WRF_PRES][y][x] / 1000, 0.2854);//convert into Kelvin
	//						}
	//
	//						//convert Kelvin to ᵒC
	//						for (size_t y = 0; y < data[s].size(); y++)
	//							for (size_t x = 0; x < data[s][y].size(); x++)
	//								data[s][y][x] -= 273.15f;
	//
	//						break;
	//					}
	//
	//					case WRF_UWND:
	//					{
	//
	//						if (l == 0)
	//						{
	//							GDALRasterBand* pBand = geotifIn[WRF_U10].GetRasterBand(0);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							array<array<float, 253>, 201> U = { 0 };
	//							GDALRasterBand* pBand = geotifIn[WRF_U].GetRasterBand(l - 1);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)U[0].size(), (int)U.size(), &(U[0][0]), (int)U[0].size(), (int)U.size(), GDT_Float32, 0, 0);
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (U[y][x] + U[y][x + 1]) / 2;
	//
	//						}
	//
	//						break;
	//					}
	//					case WRF_VWND:
	//					{
	//						if (l == 0)
	//						{
	//							GDALRasterBand* pBand = geotifIn[WRF_V10].GetRasterBand(0);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							array<array<float, 252>, 202> V = { 0 };
	//							GDALRasterBand* pBand = geotifIn[WRF_V].GetRasterBand(l - 1);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)V[0].size(), (int)V.size(), &(V[0][0]), (int)V[0].size(), (int)V.size(), GDT_Float32, 0, 0);
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (V[y][x] + V[y + 1][x]) / 2;
	//						}
	//
	//						break;
	//					}
	//					case WRF_WWND:
	//					{
	//						if (l == 0)
	//						{
	//							GDALRasterBand* pBand = geotifIn[WRF_W].GetRasterBand(0);
	//							pBand->RasterIO(GF_Read, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//						}
	//						else
	//						{
	//							array<array<float, 252>, 201> W1 = { 0 };
	//							array<array<float, 252>, 201> W2 = { 0 };
	//							GDALRasterBand* pBand1 = geotifIn[WRF_W].GetRasterBand(l - 1);
	//							GDALRasterBand* pBand2 = geotifIn[WRF_W].GetRasterBand(l);
	//							pBand1->RasterIO(GF_Read, 0, 0, (int)W1[0].size(), (int)W1.size(), &(W1[0][0]), (int)W1[0].size(), (int)W1.size(), GDT_Float32, 0, 0);
	//							pBand2->RasterIO(GF_Read, 0, 0, (int)W2[0].size(), (int)W2.size(), &(W2[0][0]), (int)W2[0].size(), (int)W2.size(), GDT_Float32, 0, 0);
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//									data[s][y][x] = (W1[y][x] + W2[y][x]) / 2;
	//						}
	//
	//						break;
	//					}
	//
	//					case WRF_RELH:
	//					{
	//						GDALRasterBand* pBand = geotifIn[l == 0 ? WRF_Q2 : WRF_QVAPOR].GetRasterBand(l == 0 ? 0 : l - 1);
	//
	//						array<array<float, 252>, 201> W = { 0 };
	//						pBand->RasterIO(GF_Read, 0, 0, (int)W[0].size(), (int)W.size(), &(W[0][0]), (int)W[0].size(), (int)W.size(), GDT_Float32, 0, 0);
	//
	//						for (size_t y = 0; y < data[s].size(); y++)
	//						{
	//							for (size_t x = 0; x < data[s][y].size(); x++)
	//							{
	//								double T = data[WRF_TAIR][y][x];//ᵒC
	//								double p = data[WRF_PRES][y][x];//hPa
	//								double es = 6.108 * exp(17.27*T / (T + 237.3));//hPa
	//								double ws = 0.62197*(es / (p - es));
	//								double Hr = max(1.0, min(100.0, (W[y][x] / ws) * 100));
	//								data[s][y][x] = Hr;
	//							}
	//						}
	//
	//						break;
	//					}
	//					case WRF_PRCP:
	//					{
	//
	//						GDALRasterBand* pBand1 = NULL;
	//						GDALRasterBand* pBand2 = NULL;
	//						if (l == 0)
	//						{
	//							array<array<float, 252>, 201> RAIN_C = { 0 };
	//							array<array<float, 252>, 201> RAIN_NC = { 0 };
	//
	//							pBand1 = geotifIn[WRF_RAINC].GetRasterBand(0);
	//							pBand2 = geotifIn[WRF_RAINNC].GetRasterBand(0);
	//							pBand1->RasterIO(GF_Read, 0, 0, (int)RAIN_C[0].size(), (int)RAIN_C.size(), &(RAIN_C[0][0]), (int)RAIN_C[0].size(), (int)RAIN_C.size(), GDT_Float32, 0, 0);
	//							pBand2->RasterIO(GF_Read, 0, 0, (int)RAIN_NC[0].size(), (int)RAIN_NC.size(), &(RAIN_NC[0][0]), (int)RAIN_NC[0].size(), (int)RAIN_NC.size(), GDT_Float32, 0, 0);
	//
	//							for (size_t y = 0; y < data[s].size(); y++)
	//							{
	//								for (size_t x = 0; x < data[s][y].size(); x++)
	//								{
	//									//precipitation is cumulative over simulation hours. 
	//									double prcp = (RAIN_NC[y][x] - last_RAIN_NC[y][x]) + (RAIN_C[y][x] - last_RAIN_C[y][x]);
	//									//if (prcp < 0.05)
	//									//prcp = 0;
	//
	//									data[s][y][x] = prcp / 3600;//Convert from mm to mm/s
	//									last_RAIN_NC[y][x] = RAIN_NC[y][x];
	//									last_RAIN_C[y][x] = RAIN_C[y][x];
	//								}
	//							}
	//						}
	//
	//						break;
	//					}
	//					}//switch
	//
	//					if (l == 0 || s != WRF_PRCP)//save precipitation only once at surface
	//					{
	//						//size_t b = l == 0 ? s : (l - 1) * (NB_WRF_VARS - 1) + NB_WRF_VARS + s;
	//						size_t b = s * NB_WRF_LEVEL + l;
	//						GDALRasterBand* pBand = geotifOut.GetRasterBand(b);
	//						pBand->RasterIO(GF_Write, 0, 0, (int)data[s][0].size(), (int)data[s].size(), &(data[s][0][0]), (int)data[s][0].size(), (int)data[s].size(), GDT_Float32, 0, 0);
	//					}
	//
	//					msg += callback.StepIt();
	//				}//variables
	//			}//level
	//
	//
	//			geotifOut.Close();
	//
	//			for (size_t i = 0; i < geotifIn.size(); i++)
	//				geotifIn[i].Close();
	//		}//if msg
	//
	//	}//for all hours
	//
	//
	//	return msg;
	//}
	//
	//
}
//
//
//
//
//
//
//
