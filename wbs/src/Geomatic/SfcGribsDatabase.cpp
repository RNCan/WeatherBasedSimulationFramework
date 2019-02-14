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

#include <unordered_set>
#include "Basic/Statistic.h"
#include "Basic/Callback.h"
#include "Basic/UtilStd.h"
#include "Basic/CSV.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/GDAL.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "Geomatic/TimeZones.h"

#include "WeatherBasedSimulationString.h"


using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	//*********************************************************************
	ERMsg CGribsMap::load(const std::string& file_path)
	{
		ERMsg msg;

		CGribsMap& me = *this;

		clear();

		ifStream file;
		msg = file.open(file_path);

		if (msg)
		{
			m_file_path = file_path;
			string path = GetPath(file_path);
			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					CTRef TRef;
					TRef.FromFormatedString((*loop)[0], "", "-", 1);
					assert(TRef.IsValid());

					string abs_file_path = GetAbsolutePath(path, (*loop)[1]);
					me[TRef].push_back(abs_file_path);
					//insert(make_pair(TRef, GetAbsolutePath(path, (*loop)[1])));
				}
			}

			file.close();
		}


		return msg;
	}

	ERMsg CGribsMap::save(const std::string& file_path)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(file_path);
		if (msg)
		{
			const_cast<CGribsMap*>(this)->m_file_path = file_path;
			string path = GetPath(file_path);
			file << "TRef,FilePath" << endl;
			for (const_iterator it = begin(); it != end(); it++)
			{
				for (vector<string>::const_iterator iit = it->second.begin(); iit != it->second.end(); iit++)
					file << it->first.GetFormatedString("%Y-%m-%d-%H") << "," << GetRelativePath(path, *iit) << endl;
			}

			file.close();
		}

		return msg;
	}


	CTPeriod CGribsMap::GetEntireTPeriod()const
	{
		CTPeriod p;
		std::set<CTRef> TRefs = GetAllTRef();
		if (!TRefs.empty())
			p = CTPeriod(*TRefs.begin(), *TRefs.rbegin());

		return p;
	}

	std::set<CTRef> CGribsMap::GetAllTRef()const
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

		CIncementalDB& me = *this;

		clear();

		if (FileExists(file_path + ".inc.csv"))
		{
			ifStream file;
			msg = file.open(file_path + ".inc.csv");
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

						me[TRef].push_back(stamp);
						//insert(make_pair(TRef, stamp));
					}
				}

				file.close();

				if (FileExists(file_path + ".inc.txt"))
				{
					msg = file.open(file_path + ".inc.txt");

					if (msg)
					{
						string line;
						while (std::getline(file, line))
						{
							StringVector tmp(line, "=");
							if (tmp.size() == 2)
							{
								std::stringstream s(tmp[1]);
								if (tmp[0] == "Gribs")
								{
									m_grib_file_path = tmp[1];
								}
								if (tmp[0] == "Locations")
								{
									m_loc_file_path = tmp[1];
								}
								if (tmp[0] == "NbPoints")
								{
									s >> m_nb_points;
								}
								else if (tmp[0] == "Variables")
								{
									s >> m_variables;
								}
							}
						}

						file.close();
					}
				}
			}
		}


		return msg;
	}

	ERMsg CIncementalDB::save(const std::string& file_path)const
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(file_path + ".inc.csv");
		if (msg)
		{
			file << "Date,Name,LastUpdate,Size,Attribute" << endl;
			for (const_iterator it = begin(); it != end(); it++)
			{
				file << it->first.GetFormatedString("%Y-%m-%d-%H") << ",";
				for (vector<CFileStamp>::const_iterator iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					file << iit->m_filePath << ",";
					file << iit->m_time << ",";
					file << iit->m_size << ",";
					file << iit->m_attribute << endl;
				}
			}
			file.close();

			msg = file.open(file_path + ".inc.txt");

			if (msg)
			{
				file << "LastRun=" << WBSF::GetCurrentTimeString("%FT%T") << endl;
				file << "Gribs=" << m_grib_file_path << endl;
				file << "Locations=" << m_loc_file_path << endl;
				file << "NbPoints=" << m_nb_points << endl;
				file << "Variables=" << m_variables << endl;

				file.close();
			}
		}

		return msg;
	}

	ERMsg CIncementalDB::Delete(const std::string& file_path)
	{
		ERMsg msg;

		msg += WBSF::RemoveFile(file_path + ".inc.csv");
		msg += WBSF::RemoveFile(file_path + ".inc.txt");

		return msg;
	}

	ERMsg CIncementalDB::GetInvalidPeriod(const CGribsMap& gribs, CTPeriod& p_invalid)const
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

	ERMsg CIncementalDB::GetInvalidTRef(const CGribsMap& gribs, std::set<CTRef>& invalid)const
	{
		ERMsg msg;

		invalid.clear();
		for (CGribsMap::const_iterator it = gribs.begin(); it != gribs.end(); it++)
		{
			const_iterator it_find = find(it->first);
			if (it_find != end())
			{
				for (vector<string>::const_iterator iit = it->second.begin(); iit != it->second.end(); iit++)
				{
					CFileStamp info_new;
					msg += info_new.SetFileStamp(*iit);
					if (msg)
					{
						vector<CFileStamp>::const_iterator iit_find = std::find_if(it_find->second.begin(), it_find->second.end(), [info_new](const CFileStamp& m)->bool { return m.m_filePath == info_new.m_filePath; });
						if (iit_find != it_find->second.end())
						{
							const CFileStamp& info_old = *iit_find;
							if (info_new.m_filePath != info_old.m_filePath ||
								info_new.m_time != info_old.m_time)
							{
								invalid.insert(it->first);
							}
						}
						else
						{
							invalid.insert(it->first);
						}
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

	ERMsg CIncementalDB::Update(const CGribsMap& gribs)
	{
		ERMsg msg;

		CIncementalDB& me = *this;
		for (auto it = gribs.begin(); it != gribs.end() && msg; it++)
		{
			for (auto iit = it->second.begin(); iit != it->second.end() && msg; iit++)
			{
				CFileStamp info_new;
				msg += info_new.SetFileStamp(*iit);
				if (msg)
				{
					me[it->first].push_back(info_new);
				}
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

					if (v < NB_VAR_H)
					{
						float value = m_lines[xy.m_y]->at(v)->get_value(xy.m_x);
						float noData = (float)GetNoData(m_bands[v]);
						if (fabs(value - m_noData[v]) > 0.1)
						{
							switch (v)
							{
								//case H_PRCP:	ASSERT(m_units[v]=="kg/(m^2)"); data[v] = float(data[v]*3600.0); break; //[kg/(m²s)] --> [mm/hour]
							case H_WNDS:	ASSERT(m_units[v] == "[m/s]"); value = float(value * 3600.0 / 1000.0); break; //[m/s] --> [km/h]
							case H_PRES:	ASSERT(m_units[v] == "[Pa]"); value = float(value / 100.0); break; //[Pa] --> [hPa]
							case H_SNDH:	ASSERT(m_units[v] == "[m]"); value = float(value / 100.0); break; //[m] --> [cm]	
							}
							data[v] = value;
						}
					}
					else if (v == H_UWND)
					{
						ASSERT(m_bands[H_UWND] != NOT_INIT && m_bands[H_VWND] != NOT_INIT);

						double U = m_lines[xy.m_y]->at(H_UWND)->get_value(xy.m_x);
						double V = m_lines[xy.m_y]->at(H_VWND)->get_value(xy.m_x);

						if (fabs(U - m_noData[v]) > 0.1 && fabs(V - m_noData[v]) > 0.1)
						{
							data[H_WNDS] = (float)sqrt(U*U + V * V) * 3600.0 / 1000.0;
							data[H_WNDD] = (float)GetWindDirection(U, V, true);
						}

					}
					else if (v == H_VWND)
					{
						//do nothing
					}
					//else if (v == H_DSWR)
					//{
					//	double s = m_lines[xy.m_y]->at(H_DSWR)->get_value(xy.m_x);
					//	if (fabs(s - m_noData[v]) > 0.1)
					//		data[H_SRAD] = float(s);
					//}


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


		if (strVar == "HGT")//geopotentiel height [m]
			var = H_GHGT;
		else if (strVar == "TMP")//temperature [C]
			var = H_TAIR;
		//else if (strVar == "PRATE")//Actual precipitation rate [kg/(m^2 s)]
		else if (strVar == "APCP" || strVar == "APCP01")//Total precipitation [kg/(m^2)]
			var = H_PRCP;
		else if (strVar == "DPT")//dew point [C]
			var = H_TDEW;
		else if (strVar == "RH")//relative humidity [%]
			var = H_RELH;
		else if (strVar == "UGRD")//u wind [m/s]
			var = H_UWND;
		else if (strVar == "VGRD")//u wind [m/s]
			var = H_VWND;
	//	else if (strVar == "WIND")//wind speed [m/s]
		//	var = H_WNDS;
		//else if (strVar == "WDIR")//wind direction [deg true]
			//var = H_WNDD;
		///* 232 */ {"DTRF", "Downward total radiation flux [W/m^2]"},
		///* 115 */ {"LWAVR", "Long wave [W/m^2]"},
		///* 116 */{ "SWAVR", "Short wave [W/m^2]" },
		//else if (strVar == "GRAD")//Global radiation [W/m^2]
			//var = H_SRAD;
		else if (strVar == "DSWRF")//Downward short-wave radiation flux [W/(m^2)]
			var = H_SRAD;//it only to get a place: to be revised
		//else if (strVar == "DSWRF")//Downward short-wave radiation flux [W/(m^2)]
			//var = H_DSWR;//it only to get a place: to be revised
		//else if (strVar == "DLWRF")//Downward long-wave radiation flux [W/(m^2)]
			//var = H_DLWR;//it only to get a place: to be revised
		//else if (strVar == "VBDSF")//Downward short-wave radiation flux [W/(m^2)]
			//var = H_DSWR;//it only to get a place: to be revised
		//else if (strVar == "VDDSF")//Downward long-wave radiation flux [W/(m^2)]
			//var = H_DLWR;//it only to get a place: to be revised
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
		case H_UWND:
		case H_VWND:
		case H_WNDS:
		case H_WND2: unit = "[m/s]";  break;
		case H_WNDD: unit = "[deg true]";  break;
		case H_SRAD: unit = "[W/(m^2)]";  break;
		case H_PRES: unit = "[Pa]";  break;
		case H_SNDH: unit = "[m]";  break;
		case H_GHGT: unit = "[m]";  break;
		}

		return unit;
	}

	ERMsg CSfcDatasetCached::open(const std::string& filePath, bool bOpenInv)
	{
		ERMsg msg;

		m_bands.fill(NOT_INIT);
		m_units.fill("");
		m_noData.fill(FLT_MIN);
		CBaseOptions options;
		options.m_bMulti = true;
		options.m_IOCPU = 8;// options.m_CPU;
		
		msg = CGDALDatasetEx::OpenInputImage(filePath, options);
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

						if (var < NB_VAR_GRIBS  && m_variables.test(var))
						{
							bool bScf = false;
							if (strType == "SFC" || strType == "TGL" || strType == "HTGL")
							{
								if (strLevel == "0" || strLevel == "2" || strLevel == "10")
									bScf = true;
							}

							if (bScf)
							{
								m_bands[var] = i;
								m_units[var] = GetDefaultUnit(var);
								m_noData[var] = GetNoData(i);
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
							if (var < NB_VAR_GRIBS  && m_variables.test(var) && !description.empty())
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
									m_noData[var] = GetNoData(i);
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
			m_bands.fill(NOT_INIT);
			m_units.fill("");
			m_noData.fill(FLT_MIN);
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
#pragma omp critical(READ_BLOCK)
						{
							poBand->ReadBlock(0, int(y), pBlockTmp->m_ptr);
							poBand->FlushBlock(0, int(y));
							poBand->FlushCache();
						}

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

		//always compute from the 4 nearest points
		CHourlyData4 data4;
		get_4nearest(pt, data4); 

		//compute weight
		array<CStatistic, NB_VAR_H> sumV;
		array<CStatistic, NB_VAR_H> sumP;
		//array<array<array<double, NB_VAR_H>, 2>, 2> weight = { 0 };

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

					for (size_t v = 0; v < NB_VAR_H; v++)
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
			for (size_t v = 0; v < NB_VAR_H; v++)
			{
				if (IsMissing(data[v]))//keep the value of the first grib
				{
					if (sumP[v].IsInit())
					{
						data[v] = sumV[v][SUM] / sumP[v][SUM];
						ASSERT(!_isnan(data[v]) && _finite(data[v]));
					}
				}
			}
		}

	}


	//bool Verif(sumP)
	//{
	//	for (size_t v = 0; v < NB_VAR_H; v++)
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

	CLocationVector CSfcDatasetCached::get_nearest(const CLocationVector& locations, size_t nb_points)const
	{
		CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);

		map<size_t, CLocation> tmp;
		const CGeoExtents& extents = GetExtents();

		CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, GetPrjID());
		CProjectionTransformation WEA_2_GEO(GetPrjID(), PRJ_WGS_84);
		for (size_t i = 0; i < locations.size(); i++)
		{
			CGeoPoint pt1 = locations[i];
			pt1.Reproject(GEO_2_WEA);
			
			if (extents.IsInside(pt1))
			{
				CGeoPointIndexVector ptArray;
				extents.GetNearestCellPosition(pt1, (int)nb_points, ptArray);

				for (size_t j = 0; j < ptArray.size(); j++)
				{
					float z = -999;

					if (m_bands[H_GHGT] != NOT_INIT)
					{
						if (!is_cached(ptArray[j].m_y))
							me.load_block(ptArray[j].m_y);

						assert(m_lines[ptArray[j].m_y] != nullptr);
						assert(is_block_inside(ptArray[j].m_y));
						float zz = m_lines[ptArray[j].m_y]->at(H_GHGT)->get_value(ptArray[j].m_x);
						if (abs(zz - m_noData[H_GHGT]) > 0.1)
							z = zz;
					}

					size_t pos = ptArray[j].m_y*extents.m_xSize + ptArray[j].m_x;
					CGeoPoint pt2 = extents.XYPosToCoord(ptArray[j]);
					pt2.Reproject(WEA_2_GEO);
					tmp[pos] = CLocation(locations[i].m_name, to_string(ptArray[j].m_y) + "_" + to_string(ptArray[j].m_x), pt2.m_lat, pt2.m_lon, z);
				}//for all points
			}//is inside
			else
			{
				//msg.ajoute();
			}
		}

		CLocationVector out;
		out.reserve(tmp.size());
		for (auto it = tmp.begin(); it != tmp.end(); it++)
			out.push_back(it->second);

		return out;
	}

	//*********************************************************************************************************
	//CSfcGribDatabase

	const int CSfcGribDatabase::VERSION = 1;
	const char* CSfcGribDatabase::XML_FLAG = "HourlyDatabase";
	const char* CSfcGribDatabase::DATABASE_EXT = ".HourlyDB";
	const char* CSfcGribDatabase::OPT_EXT = ".Hzop";
	const char* CSfcGribDatabase::DATA_EXT = ".csv";
	const char* CSfcGribDatabase::META_EXT = ".HourlyHdr.csv";
	const CTM CSfcGribDatabase::DATA_TM = CTM(CTM::HOURLY);

	ERMsg CSfcGribDatabase::CreateDatabase(const std::string& filePath)
	{
		ERMsg msg;

		CSfcGribDatabase DB;
		msg = DB.Open(filePath, modeWrite);
		if (msg)
		{
			DB.Close();
			ASSERT(FileExists(filePath));
		}

		return msg;
	}


	//strategy to get static method
	int CSfcGribDatabase::GetVersion(const std::string& filePath) { return ((CDHDatabaseBase&)CSfcGribDatabase()).GetVersion(filePath); }
	ERMsg CSfcGribDatabase::DeleteDatabase(const std::string&  outputFilePath, CCallback& callback) { return ((CDHDatabaseBase&)CSfcGribDatabase()).DeleteDatabase(outputFilePath, callback); }
	ERMsg CSfcGribDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback) { return ((CDHDatabaseBase&)CSfcGribDatabase()).RenameDatabase(inputFilePath, outputFilePath, callback); }



	ERMsg CSfcGribDatabase::Update(const CGribsMap& gribs, const CLocationVector& locationsIn, CCallback& callback)
	{
		ASSERT(IsOpen());

		ERMsg msg;

		//callback.AddMessage("Create/Update Grib database");
		//callback.AddMessage(m_filePath, 1);

		CLocationVector locations;

		if (m_nb_points == 0)
		{
			//extract at location
			locations = locationsIn;
		}
		else
		{
			GribVariables variables;
			variables.set(H_GHGT);

			CSfcDatasetCached sfcDS;
			sfcDS.set_variables(variables);
			vector<CGeoExtents> extents;
			


			for (CGribsMap::const_iterator it = gribs.begin(); it != gribs.end() && locations.empty() && msg; it++)
			{
				for (vector<string>::const_iterator iit = it->second.begin(); iit != it->second.end() && locations.empty() && msg; iit++)
				{
					string file_path = *iit;
					msg = sfcDS.open(file_path, true);
					if (msg)
					{
						if (sfcDS.get_band(H_GHGT) != NOT_INIT)
						{
							locations = sfcDS.get_nearest(locationsIn, m_nb_points);

							if (std::find( extents.begin(), extents.end(), sfcDS.GetExtents()) == extents.end())
								extents.push_back(sfcDS.GetExtents());
						}
							

						sfcDS.close();
					}

					msg += callback.StepIt(0);
				}
			}

			if (msg && locations.empty())
				msg.ajoute("Unable to find nearest points from locations because there is no image with geopotentiel height");

			
			for (vector<CGeoExtents>::iterator it = extents.begin(); it != extents.end(); it++)
			{
				CGeoSize size = it->GetSize();
				
				CProjectionPtr pPrj = CProjectionManager::GetPrj(it->GetPrjID());
				string prjName = pPrj ? pPrj->GetName() : "Unknown";
				callback.AddMessage("Grid spacing: " + to_string(it->XRes()) + " x " + to_string(it->YRes()) + " ("+ prjName+")", 1 );
			}
		}


		if (!msg)
			return msg;


		CIncementalDB incremental;
		CWeatherStationVector stations;

		
		if (m_bIncremental)
		{
			if (FileExists(m_filePath))
				msg = incremental.load(m_filePath);

			if (!empty() && size() != locations.size())
			{
				msg.ajoute("The number of location to extract ("+to_string(locations.size())+") from gribs is not the same as the previous execution ("+to_string(size())+"). Do not use incremental.");
				return msg;
			}

			if (!empty() && incremental.m_variables != m_variables)
			{
				msg.ajoute("The variable to extract from gribs is not the same as the previous execution. Do not use incremental.");
				return msg;
			}

			stations.resize(locations.size());
			for (size_t i = 0; i < locations.size(); i++)
			{
				if (size() == locations.size())
				{
					msg += Get(stations[i], i);
				}
				else
				{
					((CLocation&)stations[i]) = locations[i];
					stations[i].SetHourly(true);
				}
			}
		}
		else
		{
			stations.resize(locations.size());
			for (size_t i = 0; i < locations.size(); i++)
			{
				((CLocation&)stations[i]) = locations[i];
				stations[i].SetHourly(true);
			}
		}

		std::set<CTRef> invalid;
		msg = incremental.GetInvalidTRef(gribs, invalid);

		callback.AddMessage("Nb input locations: " + to_string(locationsIn.size()));
		if(m_nb_points>0)
			callback.AddMessage("Nb grid locations to extract with " + to_string(m_nb_points)+ " nearest: " + to_string(locations.size()));
		callback.AddMessage("Nb variables: " + to_string(m_variables.count()));
		callback.AddMessage("Nb input hours: " + to_string(gribs.size()) + " ("+ to_string(int(gribs.size()/24))+" days)");
		callback.AddMessage("Nb hours to update: " + to_string(invalid.size()) + " (" + to_string(int(invalid.size() / 24)) + " days)");
		callback.AddMessage("Incremental: " + string(m_bIncremental ? "yes" : "no"));


		if (msg && !invalid.empty())//there is an invalid period, up-to-date otherwise
		{
			
			//create all data for multi-thread
			CTPeriod p(*invalid.begin(), *invalid.rbegin());
			for (size_t i = 0; i < stations.size(); i++)
				stations[i].CreateYears(p);


			size_t nbGribs = 0;
			for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end() && msg; it++)
				nbGribs += gribs.at(*it).size();

			size_t nbStationAdded = 0;
			string feed = "Create/Update Grib database \"" + GetFileName(m_filePath) + "\" (extracting " + to_string(invalid.size()) + " hours from " + to_string(nbGribs) + " gribs)";
			callback.PushTask(feed, nbGribs);
			callback.AddMessage(feed);

			//convert set into vector for multi-thread
			vector<CTRef> tmp; 
			for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end() && msg; it++)
				tmp.push_back(*it); 
			
#pragma omp parallel for shared(msg) num_threads(min(2,m_nbMaxThreads))
			for (__int64 i = 0; i < (__int64)tmp.size(); i++)
			{
#pragma omp flush(msg)
				if (msg)
				{
					CTRef TRef = tmp[i];
					for (std::vector<string>::const_iterator iit = gribs.at(TRef).begin(); iit != gribs.at(TRef).end() && msg; iit++)
					{
						msg += ExtractStation(TRef, *iit, stations, callback);
						msg += callback.StepIt();
#pragma omp flush(msg)
					}
				}
			}

			//for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end() && msg; it++)
			//{

			//	for (std::vector<string>::const_iterator iit = gribs.at(*it).begin(); iit != gribs.at(*it).end() && msg; iit++)
			//	{
			//		msg += ExtractStation(*it, *iit, stations, callback);
			//		msg += callback.StepIt();
			//	}
			//}


			callback.PopTask();
			callback.PushTask("Save weather to disk", stations.size());
			for (CWeatherStationVector::iterator it = stations.begin(); it != stations.end() && msg; it++)
			{
				if (msg)
				{
					if (it->HaveData())
					{
						//Force write file name in the file
						it->SetDataFileName(it->GetDataFileName());
						it->UseIt(true);

						msg = Set(std::distance(stations.begin(), it), *it);

						if (msg)
							nbStationAdded++;
					}
				}

				msg += callback.StepIt();
			}

			if (msg)
			{
				//update incremental even if incremental is not activate yet
				incremental.m_grib_file_path = gribs.get_file_path();
				incremental.m_loc_file_path = locationsIn.GetFilePath();
				incremental.m_nb_points = m_nb_points;
				incremental.m_variables = m_variables;
				incremental.Update(gribs);
				msg = incremental.save(m_filePath);
			}

			callback.PopTask();

		}

		return msg;
	}


	ERMsg CSfcGribDatabase::ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;

		CSfcDatasetCached sfcDS;
		sfcDS.set_variables(m_variables);

#pragma omp critical(OPEN_GDAL)
		msg = sfcDS.open(file_path, true);

		if (msg)
		{
			CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, sfcDS.GetPrjID());
			for (size_t i = 0; i < stations.size() && msg; i++)
			{
				CGeoPoint pt = stations[i];
				pt.Reproject(GEO_2_WEA);
				if (sfcDS.GetExtents().IsInside(pt))
				{
					CTRef localTRef = CTimeZones::UTCTRef2LocalTRef(TRef, stations[i]);
					CHourlyData& data = stations[i].GetHour(localTRef);
					if (m_nb_points == 0)
						sfcDS.get_weather(pt, data);//estimate weather at location
					else
						sfcDS.get_nearest(pt, data);//get weather for this grid point


					msg += callback.StepIt(0);
				}
			}

			sfcDS.close();
		}

		return msg;
	}

	GribVariables CSfcGribDatabase::get_var(CWVariables variables)
	{
		GribVariables out;

		ASSERT( variables.size()<= out.size());
		for (size_t i = 0; i < variables.size(); i++)
			out.set(i, variables.test(i));

		if (variables.test(H_WNDS) || variables.test(H_WNDD))
		{
			out.set(H_UWND);
			out.set(H_VWND);
		}
		//if (variables.test(H_SRAD))
		//{
			//out.set(H_DSWR);
			//out.set(H_DLWR);
		//}


		return out;
	}
}
