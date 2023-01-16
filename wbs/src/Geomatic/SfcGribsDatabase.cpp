3//******************************************************************************
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
#include "Basic/DailyDatabase.h"
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

	const char* CSfcGribDatabase::META_DATA[NB_VAR_GRIBS][NB_META] =
	{
		{"2[m] HTGL \"Minimum Temperature [C]\"", "Minimum Temperature [C]", "TMIN", "2-HTGL", "[C]"},
		{"2[m] HTGL \"Temperature [C]\"", "Temperature [C]", "TMP", "2-HTGL", "[C]"},
		{"2[m] HTGL \"Maximum Temperature [C]\"", "Maximum Temperature [C]", "TMAX", "2-HTGL", "[C]"},
		{ "0[-] SFC \"01 hr Total precipitation [kg/(m^2)]\"","01 hr Total precipitation [kg/(m^2)]","APCP01","0-SFC","[kg/(m^2)]" },
		{ "2[m] HTGL \"Dew point temperature [C]\"","Dew point temperature [C]","DPT","2-HTGL","[C]" },
		{ "2[m] HTGL \"Relative humidity [%]\"","Relative humidity [%]","RH","2-HTGL","[%]" },
		{ "10[m] HTGL \"Wind speed [m/s]\"","Wind speed [m/s]","WIND","10-HTGL","[m/s]" },
		{ "10[m] HTGL \"Wind direction (from which blowing) [deg true]\"","Wind direction (from which blowing) [deg true]","WDIR","10-HTGL","[deg true]" },
		{ "0[-] SFC \"Downward short-wave radiation flux [W/(m^2)]\"","Downward short-wave radiation flux [W/(m^2)]","DSWRF","0-SFC","[W/(m^2)]" },
		{ "0[-] SFC \"Pressure [Pa]\"","Pressure [Pa]","PRES","0-SFC","[Pa]" },
		{ "0[-] SFC \"Total snowfall [m]\"","Total snowfall [m]","ASNOW","0-SFC","[m]" },
		{ "0[-] SFC \"Snow depth [m]\"","Snow depth [m]","SNOD","0-SFC","[m]" },
		{ "0[-] SFC \"Water equivalent of accumulated snow depth [kg/(m^2)]\"","Water equivalent of accumulated snow depth [kg/(m^2)]","WEASD","0-SFC","[kg/(m^2)]" },
		{ "2[m] HTGL \"Wind speed [m/s]\"","Wind speed [m/s]","WIND","2-HTGL","[m/s]" },
		{"", "", "", "", ""},
		{"", "", "", "", ""},
		{ "0[-] SFC \"Geopotential height [gpm]\"","Geopotential height [gpm]","HGT","0-SFC","[gpm]" },
		{ "10[m] HTGL \"u-component of wind [m/s]\"","u-component of wind [m/s]","UGRD","10-HTGL","[m/s]" },
		{ "10[m] HTGL \"v-component of wind [m/s]\"","v-component of wind [m/s]","VGRD","10-HTGL","[m/s]" },
		{"", "", "", "", ""},
		{"", "", "", "", ""},
		{ "0[-] SFC \"Precipitation rate [kg/(m^2)]\"","Precipitation rate [kg/(m^2)]","PRATE","0-SFC","[kg/(m^2)]" },
		{ "2[m] HTGL \"Specific humidity [kg/kg]\"","Specific humidity [kg/kg]","SPFH","2-HTGL","[kg/kg]" },

		//{"2[m] HTGL=\"Specified height level above ground\"", "Minimum Temperature [C]", "TMIN", "2-HTGL", "[C]"},
		//{"2[m] HTGL=\"Specified height level above ground\"", "Temperature [C]", "TMP", "2-HTGL", "[C]"},
		//{"2[m] HTGL=\"Specified height level above ground\"", "Maximum Temperature [C]", "TMAX", "2-HTGL", "[C]"},
		//{ "0[-] SFC=\"Ground or water surface\"","01 hr Total precipitation [kg/(m^2)]","APCP01","0-SFC","[kg/(m^2)]" },
		//{ "2[m] HTGL=\"Specified height level above ground\"","Dew point temperature [C]","DPT","2-HTGL","[C]" },
		//{ "2[m] HTGL=\"Specified height level above ground\"","Relative humidity [%]","RH","2-HTGL","[%]" },
		//{ "10[m] HTGL=\"Specified height level above ground\"","Wind speed [m/s]","WIND","10-HTGL","[m/s]" },
		//{ "10[m] HTGL=\"Specified height level above ground\"","Wind direction (from which blowing) [deg true]","WDIR","10-HTGL","[deg true]" },
		//{ "0[-] SFC=\"Ground or water surface\"","Downward short-wave radiation flux [W/(m^2)]","DSWRF","0-SFC","[W/(m^2)]" },
		//{ "0[-] SFC=\"Ground or water surface\"","Pressure [Pa]","PRES","0-SFC","[Pa]" },
		//{ "0[-] SFC=\"Ground or water surface\"","Total snowfall [m]","ASNOW","0-SFC","[m]" },
		//{ "0[-] SFC=\"Ground or water surface\"","Snow depth [m]","SNOD","0-SFC","[m]" },
		//{ "0[-] SFC=\"Ground or water surface\"","Water equivalent of accumulated snow depth [kg/(m^2)]","WEASD","0-SFC","[kg/(m^2)]" },
		//{ "2[m] HTGL=\"Specified height level above ground\"","Wind speed [m/s]","WIND","2-HTGL","[m/s]" },
		//{"", "", "", "", ""},
		//{"", "", "", "", ""},
		//{ "0[-] SFC=\"Ground or water surface\"","Geopotential height [gpm]","HGT","0-SFC","[gpm]" },
		//{ "10[m] HTGL=\"Specified height level above ground\"","u-component of wind [m/s]","UGRD","10-HTGL","[m/s]" },
		//{ "10[m] HTGL=\"Specified height level above ground\"","v-component of wind [m/s]","VGRD","10-HTGL","[m/s]" },
		//{"", "", "", "", ""},
		//{"", "", "", "", ""},
		//{ "0[-] SFC=\"Ground or water surface\"","Precipitation rate [kg/(m^2)]","PRATE","0-SFC","[kg/(m^2)]" },
		//{ "2[m] HTGL=\"Specified height level above ground\"","Specific humidity [kg/kg]","SPFH","2-HTGL","[kg/kg]" },
	};


	class CDataBlock
	{
	public:

		CDataBlock(size_t size = 0, int dataType = GDT_Float32)
		{
			m_size = size;
			m_ptr = nullptr;
			m_dataType = dataType;

			if (size > 0)
			{
				int dataSize = GDALGetDataTypeSize((GDALDataType)dataType) / sizeof(char);
				m_ptr = new char[size * dataSize];
			}

		}

		~CDataBlock()
		{
			delete[]m_ptr;
		}

		size_t size()const { return m_size; }

		double get_value(size_t x)const
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

		void set_value(size_t x, double value)
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

		void* data() { return m_ptr; }
		const void* data()const { return m_ptr; }

	protected:

		void* m_ptr;
		size_t m_size;
		int m_dataType;
	};

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
					me[TRef] = abs_file_path;

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
				string format = it->first.GetType() == CTM::HOURLY ? "%Y-%m-%d-%H" : "%Y-%m-%d";
				string date = it->first.GetFormatedString(format);
				file << date << "," << GetRelativePath(path, it->second) << endl;
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
						TRef.FromFormatedString((*loop)[0], "", "-", 1);
						assert(TRef.IsValid());
						//TRef.FromFormatedString((*loop)[0], "%Y-%m-%d-%H");
						CFileStamp stamp;
						stamp.m_filePath = (*loop)[1];
						stamp.m_time = stoi((*loop)[2]);
						stamp.m_size = stoi((*loop)[3]);
						stamp.m_attribute = stoi((*loop)[4]);

						me[TRef] = stamp;
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
								else if (tmp[0] == "Period")
								{
									s >> m_period;
								}
							}
						}

						file.close();
					}
				}

				if (FileExists(file_path + ".inc.loc"))
				{
					msg += m_locations.Load(file_path + ".inc.loc");
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
			string format = m_period.GetTM().IsHourly() ? "%Y-%m-%d-%H" : "%Y-%m-%d";

			file << "Date,Name,LastUpdate,Size,Attribute" << endl;
			for (const_iterator it = begin(); it != end(); it++)
			{
				file << it->first.GetFormatedString(format) << ",";
				{
					file << it->second.m_filePath << ",";
					file << it->second.m_time << ",";
					file << it->second.m_size << ",";
					file << it->second.m_attribute << endl;
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
				file << "Period=" << m_period << endl;
				file.close();
			}



			msg += m_locations.Save(file_path + ".inc.loc");

		}

		return msg;
	}


	ERMsg CIncementalDB::Delete(const std::string& file_path)
	{
		ERMsg msg;

		msg += WBSF::RemoveFile(file_path + ".inc.csv");
		msg += WBSF::RemoveFile(file_path + ".inc.txt");
		msg += WBSF::RemoveFile(file_path + ".inc.loc");

		return msg;
	}

	ERMsg CIncementalDB::GetInvalidPeriod(const CGribsMap& gribs, const CTPeriod& period, CTPeriod& p_invalid)const
	{
		ERMsg msg;

		p_invalid.Reset();

		std::set<CTRef> TRefs;
		msg = GetInvalidTRef(gribs, period, TRefs);
		if (msg)
		{
			if (!TRefs.empty())
				p_invalid = CTPeriod(*TRefs.begin(), *TRefs.rbegin());
		}

		return msg;
	}

	ERMsg CIncementalDB::GetInvalidTRef(const CGribsMap& gribs, const CTPeriod& period, std::set<CTRef>& invalid)const
	{
		ERMsg msg;

		invalid.clear();
		for (CGribsMap::const_iterator it = gribs.begin(); it != gribs.end(); it++)
		{
			if (!period.IsInit() || period.IsInside(it->first))
			{
				const_iterator it_find = find(it->first);
				if (it_find != end())
				{
					const CFileStamp& info_old = it_find->second;
					CFileStamp info_new;
					msg += info_new.SetFileStamp(it->second);
					if (msg)
					{

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
				else
				{
					invalid.insert(it->first);
				}
			}
		}


		return msg;
	}

	ERMsg CIncementalDB::Update(const CGribsMap& gribs)
	{
		ASSERT(gribs.GetEntireTPeriod().GetTM() == m_period.GetTM());

		ERMsg msg;

		CIncementalDB& me = *this;
		for (auto it = gribs.begin(); it != gribs.end() && msg; it++)
		{


			if (m_period.IsInside(it->first))
			{
				CFileStamp info_new;
				msg += info_new.SetFileStamp(it->second);
				if (msg)
				{
					me[it->first] = info_new;
				}
			}
		}


		return msg;
	}


	//**************************************************************************************************************
	//CSfcVariableLine




	//**************************************************************************************************************
	//CSfcDatasetCached

	CSfcDatasetCached::CSfcDatasetCached()
	{
		m_bands.fill(UNKNOWN_POS);
		m_variables_to_load.set();//load all variables by default
	}


	double CovertUnit(size_t v, const string& units, double value)
	{
		switch (v)
		{
		case H_TMIN:
		case H_TAIR:
		case H_TMAX:
		case H_TDEW: ASSERT(units == "[C]"); break;
		case H_PRCP:
		{
			ASSERT(units == "[mm]" || units == "[kg/(m^2)]");
			//data[v] = float(data[v] * 3600.0);
			break; //[kg/(m²s)] --> [mm/hour]
		}
		case H_RELH: ASSERT(units == "[%]"); break;
		case H_WNDS:
		{
			ASSERT(units == "[m/s]" || units == "[km/h]");
			if (units == "[m/s]")
				value = float(value * 3600.0 / 1000.0);

			break; //[m/s] --> [km/h]
		}
		case H_SRAD: ASSERT(units == "[W/(m^2)]"); break;
		case H_PRES:
		{
			ASSERT(units == "[hPa]" || units == "[Pa]");
			if (units == "[Pa]")
				value = float(value / 100.0);
			break; //[Pa] --> [hPa]
		}
		case H_SNOW: ASSERT(units == "[mm]"); break;
		case H_SNDH:
		{
			ASSERT(units == "[cm]" || units == "[m]");
			if (units == "[m]")
				value = float(value / 100.0);
			break; //[m] --> [cm]	
		}
		case H_SWE: ASSERT(units == "[mm]"); break;
		}

		return value;
	}


	void CSfcDatasetCached::get_weather(const CGeoPointIndex& in_xy, CHourlyData& data)const
	{
		if (m_extents.IsInside(in_xy))
		{
			CGeoSize block_size = m_extents.GetBlockSize();
			CGeoBlockIndex block_ij = m_extents.GetBlockIndex(in_xy);
			CGeoPointIndex xy = in_xy - m_extents.GetBlockRect(block_ij).UpperLeft();

			CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
			if (!is_cached(block_ij.m_x, block_ij.m_y))
				me.load_block(block_ij.m_x, block_ij.m_y);

			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (m_bands[v] != NOT_INIT && m_variables_to_load.test(v))
				{
					assert(block(block_ij.m_x, block_ij.m_y) != nullptr);
					assert(is_block_inside(block_ij.m_x, block_ij.m_y));

					if (v < NB_VAR_H)
					{
						float value = block(block_ij.m_x, block_ij.m_y)->at(v)->get_value(xy.m_x, xy.m_y);
						float noData = (float)GetNoData(m_bands[v]);
						if (fabs(value - m_noData[v]) > 0.1)
						{
							value = CovertUnit(v, m_units[v], value);

							data[v] = value;
						}
					}
					else if (v == H_UWND)
					{
						ASSERT(m_bands[H_UWND] != NOT_INIT && m_bands[H_VWND] != NOT_INIT);

						double U = block(block_ij.m_x, block_ij.m_y)->at(H_UWND)->get_value(xy.m_x, xy.m_y);
						double V = block(block_ij.m_x, block_ij.m_y)->at(H_VWND)->get_value(xy.m_x, xy.m_y);
						//double U = m_lines[xy.m_y]->at(H_UWND)->get_value(xy.m_x);
						//double V = m_lines[xy.m_y]->at(H_VWND)->get_value(xy.m_x);

						if (fabs(U - m_noData[v]) > 0.1 && fabs(V - m_noData[v]) > 0.1)
						{
							data[H_WNDS] = (float)sqrt(U * U + V * V) * 3600.0 / 1000.0;
							data[H_WNDD] = (float)GetWindDirection(U, V, true);
						}

					}
					else if (v == H_VWND)
					{
						//do nothing
					}
					else if (v == H_SPFH)
					{
						ASSERT(m_units[v] == "[kg/kg]");
						ASSERT(m_bands[H_TAIR] != NOT_INIT && m_bands[H_SPFH] != NOT_INIT);

						double Tair = block(block_ij.m_x, block_ij.m_y)->at(H_TAIR)->get_value(xy.m_x, xy.m_y);
						double Hs = block(block_ij.m_x, block_ij.m_y)->at(H_SPFH)->get_value(xy.m_x, xy.m_y);

						if (fabs(Tair - m_noData[v]) > 0.1 && fabs(Hs - m_noData[v]) > 0.1)
						{
							data[H_TDEW] = (float)Hs2Td(Tair, 1000 * Hs); //kg/kg -> g/kg
							data[H_RELH] = (float)Hs2Hr(Tair, 1000 * Hs); //kg/kg -> g/kg
						}

					}
					else if (v == H_PRATE)
					{
						ASSERT(m_units[v] == "[kg/(m^2 s)]");
						double prcp = block(block_ij.m_x, block_ij.m_y)->at(H_PRATE)->get_value(xy.m_x, xy.m_y);
						if (fabs(prcp - m_noData[v]) > 0.1)
							data[H_PRCP] = float(3600 * prcp);
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

			}
		}
	}

	void CSfcDatasetCached::get_weather(const CGeoPointIndex& in_xy, CWeatherDay& data)const
	{
		if (m_extents.IsInside(in_xy))
		{
			CGeoSize block_size = m_extents.GetBlockSize();
			CGeoBlockIndex block_ij = m_extents.GetBlockIndex(in_xy);
			CGeoPointIndex xy = in_xy - m_extents.GetBlockRect(block_ij).UpperLeft();

			CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
			if (!is_cached(block_ij.m_x, block_ij.m_y))
				me.load_block(block_ij.m_x, block_ij.m_y);

			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (m_bands[v] != NOT_INIT && m_variables_to_load.test(v))
				{
					assert(block(block_ij.m_x, block_ij.m_y) != nullptr);
					assert(is_block_inside(block_ij.m_x, block_ij.m_y));

					if (v < NB_VAR_H)
					{
						float value = block(block_ij.m_x, block_ij.m_y)->at(v)->get_value(xy.m_x, xy.m_y);
						float noData = (float)GetNoData(m_bands[v]);
						if (fabs(value - m_noData[v]) > 0.1)
						{
							value = CovertUnit(v, m_units[v], value);
							data.SetStat((TVarH)v, value);
						}
					}
					else if (v == H_UWND)
					{
						ASSERT(m_bands[H_UWND] != NOT_INIT && m_bands[H_VWND] != NOT_INIT);

						double U = block(block_ij.m_x, block_ij.m_y)->at(H_UWND)->get_value(xy.m_x, xy.m_y);
						double V = block(block_ij.m_x, block_ij.m_y)->at(H_VWND)->get_value(xy.m_x, xy.m_y);

						if (fabs(U - m_noData[v]) > 0.1 && fabs(V - m_noData[v]) > 0.1)
						{
							//data[H_WNDS] = (float)sqrt(U*U + V * V) * 3600.0 / 1000.0;
							//data[H_WNDD] = (float)GetWindDirection(U, V, true);
							data.SetStat(H_WNDS, (float)sqrt(U * U + V * V) * 3600.0 / 1000.0);
							data.SetStat(H_WNDD, (float)GetWindDirection(U, V, true));
						}

					}
					else if (v == H_VWND)
					{
						//do nothing
					}
				}
			}
		}
	}

	float CSfcDatasetCached::get_variable(const CGeoPointIndex& in_xy, size_t v)const
	{
		ASSERT(m_extents.m_yBlockSize == 1);
		ASSERT(v < m_bands.size());

		size_t b = get_band(v);
		if (!m_extents.IsInside(in_xy) || b == UNKNOWN_POS)
			return WEATHER::MISSING;

		CGeoBlockIndex block_ij = m_extents.GetBlockIndex(in_xy);
		assert(is_block_inside(block_ij.m_x, block_ij.m_y));
		//ASSERT(m_extents.GetBlockIndex(xy).m_y == xy.m_y);//block y index is data y index

		CGeoPointIndex xy = in_xy - m_extents.GetBlockRect(block_ij).UpperLeft();
		ASSERT(m_extents.GetBlockExtents(block_ij).GetPosRect().IsInside(xy));

		CSfcDatasetCached& me = const_cast<CSfcDatasetCached&>(*this);
		if (!is_cached(block_ij.m_x, block_ij.m_y))
			me.load_block(block_ij.m_x, block_ij.m_y);

		assert(block(block_ij.m_x, block_ij.m_y));

		return block(block_ij.m_x, block_ij.m_y)->at(v)->get_value(xy.m_x, xy.m_y);

	}



	size_t CSfcDatasetCached::get_var(const string& strVar)
	{
		size_t var = UNKNOWN_POS;


		if (strVar == "HGT")//geopotentiel height [m]
			var = H_GHGT;
		else if (strVar == "TMIN")//temperature [C]
			var = H_TMIN;
		else if (strVar == "TMP")//temperature [C]
			var = H_TAIR;
		else if (strVar == "TMAX")//temperature [C]
			var = H_TMAX;
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
		else if (strVar == "WIND")//wind speed [m/s]
			var = H_WNDS;
		else if (strVar == "WDIR")//wind direction [deg true]
			var = H_WNDD;
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

		else if (strVar == "SPFH")//Specific humidity [kg/kg]
			var = H_SPFH;
		else if (strVar == "PRATE")//Precipitation rate [kg/(m^2 s)]
			var = H_PRATE;
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
		case H_SPFH: unit = "[kg/kg]";  break;
		case H_PRATE: unit = "[kg/(m^2)]";  break;
		}

		return unit;
	}

	ERMsg CSfcDatasetCached::open(const std::string& filePath, bool bOpenInv)
	{
		ERMsg msg;

		m_variables.reset();
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
				ASSERT(m_variables_to_load.any());//at least one variable to load
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

						if (var < NB_VAR_GRIBS && m_variables_to_load.test(var))
						{
							bool bScf = false;
							if (strType == "SFC" || strType == "TGL" || strType == "HTGL")
							{
								if (strLevel == "0" || strLevel == "2" || strLevel == "10")
									bScf = true;
							}

							if (bScf)
							{
								m_variables.set(var);
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
						if (description.empty())
						{
							StringVector tmp(strName, "-");
							if (tmp.size() == 2)
							{
								description.push_back(tmp[0]);
								description.push_back(strUnit);
								description.push_back(tmp[1]);
								description.push_back("");
							}
						}

						if (description.size() > 3 && (description[2] == "ISBL" || description[2] == "SFC" || description[2] == "HTGL" || description[2] == "HYBL"))
						{
							if (!description.empty() && description[0].find('-') != NOT_INIT)
								description.clear();

							//description.empty();

							size_t var = get_var(strVar);
							if (var < NB_VAR_GRIBS && m_variables_to_load.test(var) && !description.empty())
							{
								bool bSfc = false;

								if (description[2] == "SFC" || description[2] == "HTGL")
								{
									size_t level = as<size_t>(description[0]);
									if (level == 0 || level == 2 || level == 10)
									{
										bSfc = true;
									}
								}

								if (bSfc)
								{
									m_variables.set(var);
									m_bands[var] = i;
									m_units[var] = strUnit;
									m_noData[var] = GetNoData(i);
								}
							}
						}
					}
				}
			}

			//Update available variable with needed variable
			//set_variables(m_variables);
			/*ASSERT(m_bands.size() == m_variables.size());
			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (!m_variables.test(v))
				{
					m_bands[v] = NOT_INIT;
				}
			}*/
		}


		return msg;
	}

	void CSfcDatasetCached::close()
	{
		if (CGDALDatasetEx::IsOpen())
		{
			//m_blocks.resize(boost::extents[0][0]);
			m_blocks.clear();
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
			//ASSERT(m_extents.XNbBlocks() == 1);
			//me.m_blocks.resize(boost::extents[m_extents.YNbBlocks()][m_extents.XNbBlocks()]);
			me.m_blocks.resize(m_extents.YNbBlocks());
			for (size_t i = 0; i < me.m_blocks.size(); i++)
				me.m_blocks[i].resize(m_extents.XNbBlocks());
		}
	}

	void CSfcDatasetCached::load_block(size_t i, size_t j)
	{
		//		m_mutex.lock();
		if (!is_cached(i, j))
		{
			block(i, j).reset(new CSfcWeatherBlock);
			for (size_t v = 0; v < m_bands.size(); v++)
			{
				if (m_bands[v] != NOT_INIT && m_variables_to_load.test(v))
				{
					size_t b = m_bands[v];
					assert(block(i, j)->at(v) == nullptr);

					//CTimer readTime(TRUE);

					//copy part of the date
					CGeoExtents ext = GetExtents().GetBlockExtents(int(i), int(j));
					//CGeoRectIndex ind = GetExtents().CoordToXYPos(ext);

					GDALRasterBand* poBand = m_poDataset->GetRasterBand(int(b) + 1);//one base in direct load

					/*if (m_bVRT)
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
					{*/
					int nXBlockSize, nYBlockSize;
					poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);

					ASSERT(nXBlockSize == m_extents.m_xBlockSize);
					ASSERT(nYBlockSize == m_extents.m_yBlockSize);
					GDALDataType type = poBand->GetRasterDataType();

#pragma omp critical(READ_BLOCK)
					{
						//CSfcBlock* pBlockData = ;
						block(i, j)->at(v).reset(new CSfcBlock(nXBlockSize, nYBlockSize));
						if (type == GDT_Float32)
						{
							poBand->ReadBlock(int(i), int(j), block(i, j)->at(v)->data());
							poBand->FlushBlock(int(i), int(j));
							poBand->FlushCache();
						}
						else
						{
							CDataBlock block_tmp(nXBlockSize * nYBlockSize, type);
							poBand->ReadBlock(int(i), int(j), block_tmp.data());
							poBand->FlushBlock(int(i), int(j));
							poBand->FlushCache();
							CSfcBlock* pBlockData = new CSfcBlock(nXBlockSize, nYBlockSize);
							for (size_t y = 0; y < nYBlockSize; y++)
								for (size_t x = 0; x < nXBlockSize; x++)
									block(i, j)->at(v)->set_value(x, y, block_tmp.get_value(y * nXBlockSize + x));
						}
					}

					//if (type == GDT_Float64)
					//{

				//}

				//readTime.Stop();
				//m_stats[0] += readTime.Elapsed();
				}
			}
		}
		//	m_mutex.unlock();
		ASSERT(is_cached(i, j));
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



	void CSfcDatasetCached::get_weather(const CGeoPoint& pt, CWeatherDay& data)const
	{
		static const double POWER = 1;

		//always compute from the 4 nearest points
		CDailyData4 data4;
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
						if (data4[y][x][TVarH(v)].IsInit())
						{
							sumV[v] += data4[y][x][TVarH(v)][MEAN] * p1;
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
				if (!data[TVarH(v)].IsInit())//keep the value of the first grib
				{
					if (sumP[v].IsInit())
					{
						double value = sumV[v][SUM] / sumP[v][SUM];
						ASSERT(!_isnan(value) && _finite(value));
						data.SetStat(TVarH(v), value);
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

	void CSfcDatasetCached::get_nearest(const CGeoPoint& pt, CWeatherDay& data)const
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

	void CSfcDatasetCached::get_4nearest(const CGeoPoint& pt, CDailyData4& data4)const
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
						CGeoBlockIndex block_ij = m_extents.GetBlockIndex(ptArray[j]);
						assert(is_block_inside(block_ij.m_x, block_ij.m_y));

						if (!is_cached(block_ij.m_x, block_ij.m_y))
							me.load_block(block_ij.m_x, block_ij.m_y);

						assert(block(block_ij.m_x, block_ij.m_y) != nullptr);

						CGeoPointIndex xy = ptArray[j] - m_extents.GetBlockRect(block_ij).UpperLeft();
						float zz = block(block_ij.m_x, block_ij.m_y)->at(H_GHGT)->get_value(xy.m_x, xy.m_y);
						if (abs(zz - m_noData[H_GHGT]) > 0.1)
							z = zz;
					}

					size_t pos = ptArray[j].m_y * extents.m_xSize + ptArray[j].m_x;
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

	//const int CSfcGribDatabase::VERSION = 1;
	//const char* CSfcGribDatabase::XML_FLAG = "HourlyDatabase";
	//const char* CSfcGribDatabase::DATABASE_EXT = ".HourlyDB";
	//const char* CSfcGribDatabase::OPT_EXT = ".Hzop";
	//const char* CSfcGribDatabase::DATA_EXT = ".csv";
	//const char* CSfcGribDatabase::META_EXT = ".HourlyHdr.csv";
	//const CTM CSfcGribDatabase::DATA_TM = CTM(CTM::HOURLY);

	std::shared_ptr < CDHDatabaseBase> CSfcGribDatabase::GetDatabase(const std::string& filePath)
	{
		string ext = GetFileExtension(filePath);

		std::shared_ptr < CDHDatabaseBase> pDB = nullptr;
		if (IsEqualNoCase(ext, ".HourlyDB"))
		{
			pDB.reset(new CHourlyDatabase);
		}
		else if (IsEqualNoCase(ext, ".DailyDB"))
		{
			pDB.reset(new CDailyDatabase);
		}
		else
		{
			ASSERT(false);
		}

		return pDB;
	}

	ERMsg CSfcGribDatabase::CreateDatabase(const std::string& filePath)
	{
		ERMsg msg;


		std::shared_ptr < CDHDatabaseBase> pDB = GetDatabase(filePath);

		msg = pDB->Open(filePath, CDailyDatabase::modeWrite);
		if (msg)
		{
			pDB->Close();
			ASSERT(FileExists(filePath));
		}

		/*CSfcGribDatabase DB;
		msg = DB.Open(filePath, modeWrite);
		if (msg)
		{
			DB.Close();
			ASSERT(FileExists(filePath));
		}*/

		return msg;
	}


	//strategy to get static method
	int CSfcGribDatabase::GetVersion(const std::string& filePath) { return ((CDHDatabaseBase&)CSfcGribDatabase()).GetVersion(filePath); }
	ERMsg CSfcGribDatabase::DeleteDatabase(const std::string& filePath, CCallback& callback)
	{
		std::shared_ptr < CDHDatabaseBase> pDB = GetDatabase(filePath);
		return pDB->DeleteDatabase(filePath, callback);
		//return ((CDHDatabaseBase&)CSfcGribDatabase()).DeleteDatabase(outputFilePath, callback); 
	}
	ERMsg CSfcGribDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ASSERT(IsEqualNoCase(GetFileExtension(inputFilePath), GetFileExtension(outputFilePath)));


		std::shared_ptr < CDHDatabaseBase> pDB = GetDatabase(inputFilePath);
		return pDB->RenameDatabase(inputFilePath, outputFilePath, callback);
		//return ((CDHDatabaseBase&)CSfcGribDatabase()).RenameDatabase(inputFilePath, outputFilePath, callback); 
	}

	ERMsg CSfcGribDatabase::Open(const std::string& filePath, UINT flag, CCallback& callback, bool bSkipVerify)
	{
		ERMsg msg;

		string ext = GetFileExtension(filePath);


		if (IsEqualNoCase(ext, ".HourlyDB"))
		{
			m_pDB.reset(new CHourlyDatabase);
		}
		else if (IsEqualNoCase(ext, ".DailyDB"))
		{
			m_pDB.reset(new CDailyDatabase);
		}

		msg = m_pDB->Open(filePath, flag, callback, bSkipVerify);

		return msg;
	}

	ERMsg CSfcGribDatabase::Close(bool bSave, CCallback& callback)
	{
		return m_pDB->Close(bSave, callback);
	}


	template <class T>
	class CGeoBlockHash;

	template<>
	class CGeoBlockHash<CGeoBlockIndex>
	{
	public:
		std::size_t operator()(CGeoBlockIndex const& s) const
		{
			return s.m_x ^ (s.m_y << 1);
		}
	};


	ERMsg CSfcGribDatabase::Update(const CGribsMap& gribs, const CLocationVector& locationsIn, CCallback& callback)
	{
		ASSERT(m_pDB->IsOpen());

		ERMsg msg;

		CTPeriod Gribs_period = gribs.GetEntireTPeriod();
		if (!m_period.IsInit())
			m_period = Gribs_period;

		if (Gribs_period.GetTM() != m_period.GetTM())
		{
			msg.ajoute("Gribs mismatch Hourly/Daily");
			return msg;
		}

		if (!Gribs_period.IsIntersect(m_period))
		{
			msg.ajoute("Period to simulate doesn't intersect Gribs period");
			return msg;
		}


		m_bIsHourly = Gribs_period.GetTM() == CTM::HOURLY;
		CLocationVector cells_points;

		if (m_nb_points == 0)
		{
			//extract at location
			cells_points = locationsIn;
		}
		else
		{
			CSfcDatasetCached sfcDS;
			sfcDS.m_variables_to_load.set(H_GHGT);
			vector<CGeoExtents> extents;


			for (CGribsMap::const_iterator it = gribs.begin(); it != gribs.end() && cells_points.empty() && msg; it++)
			{
				string file_path = it->second;
				msg = sfcDS.open(file_path, true);
				if (msg)
				{
					if (sfcDS.get_band(H_GHGT) != NOT_INIT)
					{
						cells_points = sfcDS.get_nearest(locationsIn, m_nb_points);

						if (std::find(extents.begin(), extents.end(), sfcDS.GetExtents()) == extents.end())
							extents.push_back(sfcDS.GetExtents());
					}


					sfcDS.close();
				}

				msg += callback.StepIt(0);

			}

			if (msg && cells_points.empty())
				msg.ajoute("Unable to find nearest points from locations because there is no image with geopotentiel height");


			for (vector<CGeoExtents>::iterator it = extents.begin(); it != extents.end(); it++)
			{
				CGeoSize size = it->GetSize();

				CProjectionPtr pPrj = CProjectionManager::GetPrj(it->GetPrjID());
				string prjName = pPrj ? pPrj->GetName() : "Unknown";
				callback.AddMessage("Grid spacing: " + to_string(it->XRes()) + " x " + to_string(it->YRes()) + " (" + prjName + ")", 1);
			}
		}



		/*if (memory_required > memory_available)
		{
			msg.ajoute("Not enough memory to execute " + to_string(m_period.GetNbYears() )+ " years");
		}*/

		if (!msg)
			return msg;


		CIncementalDB incremental;


		//CWeatherStationVector stations;
		bool bIncremental = m_bIncremental;
		std::set<CTRef> invalid;




		if (bIncremental)
		{
			if (!m_pDB->empty())
			{
				msg = incremental.load(m_pDB->GetFilePath());



				if (!m_pDB->empty() && incremental.m_locations != locationsIn)
				{
					callback.AddMessage("Location' points to extract (" + to_string(locationsIn.size()) + ") from gribs is not the same as the previous execution (" + to_string(incremental.m_locations.size()) + "). Incremental was reset.");
					bIncremental = false;
				}

				if (!m_pDB->empty() && incremental.m_nb_points != m_nb_points)
				{
					callback.AddMessage("Number of nearest cells to used has change (" + to_string(incremental.m_nb_points) + " to " + to_string(m_nb_points) + "). Incremental was reset.");
					bIncremental = false;
				}

				if (!m_pDB->empty() && incremental.m_variables != m_variables)
				{
					callback.AddMessage("The variables to extract from gribs is not the same as the previous execution. Incremental was reset.");
					bIncremental = false;
				}

				if (bIncremental == false)
				{
					string file_path = m_pDB->GetFilePath();
					//remove old database;
					msg += m_pDB->Close(false);
					msg += m_pDB->DeleteDatabase(file_path, callback);
					msg += m_pDB->Open(file_path, CHourlyDatabase::modeEdit, callback, true);

					if (!msg)
						return msg;

					ASSERT(m_pDB->IsOpen());
				}

			}
			else
			{
				bIncremental = false;
			}
		}

		if (bIncremental)
		{
			/*stations.resize(cells_points.size());
			for (size_t i = 0; i < cells_points.size(); i++)
			{
				ASSERT(m_pDB->size() == cells_points.size());

				msg += m_pDB->Get(stations[i], i);
			}*/

			msg = incremental.GetInvalidTRef(gribs, m_period, invalid);
		}
		else
		{
			incremental.clear();
			//stations.resize(cells_points.size());
			for (size_t i = 0; i < cells_points.size(); i++)
			{
				//((CLocation&)stations[i]) = cells_points[i];
				//stations[i].SetHourly(m_bIsHourly);
				CWeatherStation station;
				((CLocation&)station) = cells_points[i];
				station.SetHourly(m_bIsHourly);
				m_pDB->Add(station);
			}

			for (CTRef TRef = m_period.Begin(); TRef <= m_period.End(); TRef++)
			{
				invalid.insert(TRef);
			}
		}




		callback.AddMessage("Input gribs: " + gribs.get_file_path());
		callback.AddMessage("Gribs period: " + ReplaceString(Gribs_period.GetFormatedString(), "|", ","));
		callback.AddMessage("Nb input locations: " + to_string(locationsIn.size()));
		if (m_nb_points > 0)
			callback.AddMessage("Nb grid locations to extract (using " + to_string(m_nb_points) + " nearest cells): " + to_string(cells_points.size()));

		size_t HD_factor = m_bIsHourly ? 24 : 1;
		callback.AddMessage("Nb variables: " + to_string(m_variables.count()));

		callback.AddMessage("Period: " + m_period.GetFormatedString("%1 to %2") + " (" + to_string(m_period.size()) + (m_bIsHourly ? " hours)" : " days)"));
		callback.AddMessage("Nb inputs: " + to_string(gribs.size()) + " (" + to_string(int(gribs.size() / HD_factor)) + " days)");
		callback.AddMessage("Nb elements to update: " + to_string(invalid.size()) + " (" + to_string(int(invalid.size() / HD_factor)) + " days)");
		callback.AddMessage("Incremental: " + string(bIncremental ? "yes" : "no"));


		if (msg && !invalid.empty())//there is an invalid period, up-to-date otherwise
		{

			//create all data for multi-thread
			//CTPeriod p(*invalid.begin(), *invalid.rbegin());
			//for (size_t i = 0; i < stations.size(); i++)
				//stations[i].CreateYears(p);


			//convert set into vector for multi-thread
			vector<CTRef> tmp;
			for (std::set<CTRef>::const_iterator it = invalid.begin(); it != invalid.end() && msg; it++)
			{
				if (gribs.find(*it) != gribs.end())
					tmp.push_back(*it);
			}

			if (false)
			{
				//
				//				//#pragma omp parallel for shared(msg) num_threads(min(2,m_nbMaxThreads))
				//				for (__int64 i = 0; i < (__int64)tmp.size(); i++)
				//				{
				//#pragma omp flush(msg)
				//					if (msg)
				//					{
				//						CTRef TRef = tmp[i];
				//						msg += ExtractStation(TRef, gribs.at(TRef), stations, callback);
				//						msg += callback.StepIt();
				//#pragma omp flush(msg)
				//
				//					}
				//				}
				//
				//				callback.PopTask();
				//				callback.PushTask("Save weather to disk", stations.size());
				//				for (CWeatherStationVector::iterator it = stations.begin(); it != stations.end() && msg; it++)
				//				{
				//					if (msg)
				//					{
				//						//Force write file name in the file
				//						it->SetDataFileName(it->GetDataFileName());
				//						it->UseIt(true);
				//
				//						msg = m_pDB->Set(std::distance(stations.begin(), it), *it);
				//
				//						if (msg)
				//							nbStationAdded++;
				//					}
				//
				//					msg += callback.StepIt();
				//				}
			}
			else
			{
				if (!tmp.empty())
				{
					//open th first image and assume all image have the same structure.

					CSfcDatasetCached sfcDS;
					sfcDS.m_variables_to_load = m_variables;
					msg = sfcDS.open(gribs.at(tmp[0]), true);
					if (msg)
					{
						CGeoExtents extents = sfcDS.GetExtents();
						sfcDS.Close();

						//std::vector<std::pair<int, int>> block_list = extents.GetBlockList();
						//
						
						double memory_required = 2*sizeof(CWeatherDay) * m_period.GetNbYears() * 366.0* (m_bIsHourly ? 24 : 1)  / 1000000;//Mb
						double memory_available = GetTotalSystemMemory() / 1000000 * 9 / 10;//Mb
						size_t max_points = memory_available / memory_required;


						unordered_map<CGeoBlockIndex, CLocationVector, CGeoBlockHash<CGeoBlockIndex>> blocks_tmp;
						deque<CLocationVector> blocks;

						for (size_t i = 0; i < cells_points.size(); i++)
						{
							CGeoPointIndex pt = extents.CoordToXYPos(cells_points[i]);
							CGeoBlockIndex bi = extents.GetBlockIndex(pt);
							cells_points[i].SetSSI("StationPos", to_string(i));
							blocks_tmp[bi].push_back(cells_points[i]);
						}

						//re-split block too big
						size_t cur_i=0;
						for (auto it = blocks_tmp.begin(); it != blocks_tmp.end(); it++)
						{
							for (size_t i = 0; i < it->second.size(); i++)
							{
								if (!blocks.empty() && blocks[cur_i].size() >= max_points)
									cur_i++;

								if (cur_i == blocks.size())
									blocks.resize(blocks.size() + 1);

								blocks[cur_i].push_back(it->second[i]);
							}
						}
						

						size_t nbStationAdded = 0;
						string feed = "Create/Update Grib database \"" + GetFileName(m_pDB->GetFilePath()) + "\" (extracting " + to_string(blocks.size()) + " block)";
						callback.PushTask(feed, blocks.size());
						callback.AddMessage(feed);

						for (size_t b = 0; b != blocks.size()&&msg; b++)
						{
							string feed = "extracting " + to_string(tmp.size()) + (m_bIsHourly ? " hours" : " days");
							callback.PushTask(feed, tmp.size());
							//callback.AddMessage(feed);

							CWeatherStationVector stations;
							if (bIncremental)
							{
								for (size_t i = 0; i < blocks[b].size(); i++)
								{
									size_t station_pos = ToSizeT(blocks[b][i].GetSSI("StationPos"));
									msg += m_pDB->Get(stations[i], station_pos);
								}
							}
							else
							{

								stations.resize(blocks[b].size());
								for (size_t i = 0; i < blocks[b].size(); i++)
								{
									((CLocation&)stations[i]) = blocks[b][i];
									stations[i].SetHourly(m_bIsHourly);
								}

							}


							for (__int64 i = 0; i < (__int64)tmp.size(); i++)
							{
								if (msg)
								{
									CTRef TRef = tmp[i];


									msg += ExtractStation(TRef, gribs.at(TRef), stations, callback);
									msg += callback.StepIt();

								}
							}

							callback.PopTask();
							callback.PushTask("Save weather block to disk", stations.size());
							for (CWeatherStationVector::iterator it = stations.begin(); it != stations.end() && msg; it++)
							{
								if (msg)
								{
									//Force write file name in the file
									it->SetDataFileName(it->GetDataFileName());
									it->UseIt(true);

									size_t station_pos = ToSizeT((*it).GetSSI("StationPos"));
									msg = m_pDB->Set(station_pos, *it);
									//std::distance(stations.begin(), it)

									if (msg)
										nbStationAdded++;
								}

								msg += callback.StepIt();
							}

							callback.PopTask();
							msg += callback.StepIt();
						}
					}
				}


				if (msg)
				{
					//update incremental even if incremental is not activate yet
					incremental.m_grib_file_path = gribs.get_file_path();
					incremental.m_loc_file_path = locationsIn.GetFilePath();
					incremental.m_nb_points = m_nb_points;
					incremental.m_variables = m_variables;
					incremental.m_period = m_period;
					incremental.m_locations = locationsIn;

					incremental.Update(gribs);
					msg = incremental.save(m_pDB->GetFilePath());
				}

				callback.PopTask();
			}
		}

		return msg;
	}


	ERMsg CSfcGribDatabase::ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback)
	{
		ERMsg msg;

		CSfcDatasetCached sfcDS;
		sfcDS.m_variables_to_load = m_variables;

#pragma omp critical(OPEN_GDAL)
		msg = sfcDS.open(file_path, true);

		if (msg)
		{

			//for optimization, we select only 2 wind variables
			GribVariables var = sfcDS.get_variables();
			if (var.test(H_WNDS) && var.test(H_WNDD))
			{
				var.reset(H_UWND);
				var.reset(H_VWND);
			}
			else if (var.test(H_UWND) && var.test(H_VWND))
			{
				var.reset(H_WNDS);
				var.reset(H_WNDD);
			}
			else
			{
				var.reset(H_UWND);
				var.reset(H_VWND);
				var.reset(H_WNDS);
				var.reset(H_WNDD);
			}
			//if(var.test(H_TAIR)&&)

			sfcDS.m_variables_to_load = var;

			CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, sfcDS.GetPrjID());
			for (size_t i = 0; i < stations.size() && msg; i++)
			{
				CGeoPoint pt = stations[i];
				pt.Reproject(GEO_2_WEA);
				if (sfcDS.GetExtents().IsInside(pt))
				{
					if (TRef.GetTM() == CTM::HOURLY)
					{
						CTRef localTRef = CTimeZones::UTCTRef2LocalTRef(TRef, stations[i]);
						CHourlyData& data = stations[i].GetHour(localTRef);
						if (m_nb_points == 0)
							sfcDS.get_weather(pt, data);//estimate weather at location
						else
							sfcDS.get_nearest(pt, data);//get weather for this grid point

						if (data[H_TMIN] == -999 && data[H_TAIR] != -999 && m_variables.test(H_TMIN))
							data[H_TMIN] = data[H_TAIR];
						if (data[H_TMAX] == -999 && data[H_TAIR] != -999 && m_variables.test(H_TMAX))
							data[H_TMAX] = data[H_TAIR];
					}
					else
					{
						//CWeatherDay data(;
						CWeatherDay& data = stations[i].GetDay(TRef);
						if (m_nb_points == 0)
							sfcDS.get_weather(pt, data);//estimate weather at location
						else
							sfcDS.get_nearest(pt, data);//get weather for this grid point

						if (data[H_TMIN].IsInit() && data[H_TMAX].IsInit() && !data[H_TAIR].IsInit() && m_variables.test(H_TAIR))
							data[H_TAIR] = data[H_TNTX];
					}

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

		ASSERT(variables.size() <= out.size());
		for (size_t i = 0; i < variables.size(); i++)
			out.set(i, variables.test(i));

		if (variables.test(H_WNDS) || variables.test(H_WNDD))
		{
			out.set(H_UWND);
			out.set(H_VWND);
		}


		return out;
	}

	ERMsg CSfcGribDatabase::Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double searchRadius, CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation, bool bUseShoreDistance)const
	{
		return m_pDB->Search(searchResultArray, station, nbStation, searchRadius, filter, year, bExcludeUnused, bUseElevation, bUseShoreDistance);
	}

	ERMsg CSfcGribDatabase::GetStations(CWeatherStationVector& stationArray, const CSearchResultVector& results, int year)const
	{
		return m_pDB->GetStations(stationArray, results, year);
	}

}
