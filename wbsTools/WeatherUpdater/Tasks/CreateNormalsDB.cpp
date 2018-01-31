#include "StdAfx.h"
#include "CreateNormalsDB.h"
#include "TaskFactory.h"
#include "Basic/NormalsStation.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDatabase.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"
#include "Simulation/AdvancedNormalStation.h"


#include "../resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std; 

namespace WBSF
{
	//*********************************************************************

	const char* CCreateNormalsDB::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "InputFilePath", "OutputFilePath", "FirstYear", "LastYear", "NbYears", "ApplyClimaticChange", "MMGFilepath", "ReferencePeriod", "FuturPeriod", "NbNeighbor", "MaxDistance", "Power" };
	const size_t CCreateNormalsDB::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_FILEPATH, T_FILEPATH, T_STRING, T_STRING, T_STRING, T_BOOL, T_FILEPATH, T_COMBO_STRING, T_STRING_SELECT, T_STRING, T_STRING, T_STRING };
	const UINT CCreateNormalsDB::ATTRIBUTE_TITLE_ID = IDS_TOOL_CREATE_NORMALS_P;
	const UINT CCreateNormalsDB::DESCRIPTION_TITLE_ID = ID_TASK_CREATE_NORMALS;

	const char* CCreateNormalsDB::CLASS_NAME(){ static const char* THE_CLASS_NAME = "CreateNormals";  return THE_CLASS_NAME; }
	CTaskBase::TType CCreateNormalsDB::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCreateNormalsDB::CLASS_NAME(), (createF)CCreateNormalsDB::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("NormalDBFromDailyDB", (createF)CCreateNormalsDB::create);


	CCreateNormalsDB::CCreateNormalsDB(void)
	{}


	CCreateNormalsDB::~CCreateNormalsDB(void)
	{}


	std::string CCreateNormalsDB::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case INPUT:			str = GetString(IDS_STR_FILTER_DAILY); break;
		case OUTPUT:		str = GetString(IDS_STR_FILTER_NORMALS); break;
		case MMG_FILEPATH:	str = GetString(IDS_STR_FILTER_MMG); break;
		case NORMAL_PERIOD: 
		{
			//str = " ";
			for (int i = 0; i < 3; i++)
			{
				if (!str.empty())
					str += "|";

				str += FormatA("%d-%d", 1961 + 10 * i, 1990 + 10 * i);
			}
				

			str += FormatA("|2003-2017");//special case for Quebec database
			break;
		}
		case FUTUR_PERIOD:
		{
			for (size_t i = 0; i < CNormalFromDaily::NB_CC_PERIODS; i++)
			{
				str += i > 0 ? "|" : "";
				str += FormatA("%d=%d-%d", i, CNormalFromDaily::FIRST_YEAR_OF_FIRST_PERIOD + 10 * i, (CNormalFromDaily::FIRST_YEAR_OF_FIRST_PERIOD+29) + 10 * i);
			}
				

			break;
		}
		};

		return str;
	}

	std::string CCreateNormalsDB::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case FIRST_YEAR:	str = "1981"; break;
		case LAST_YEAR:		str = "2010"; break;
		case NB_YEARS_MIN:	str = "10"; break;
		case APPLY_CLIMATIC_CHANGE:	str = "0"; break;
		case NORMAL_PERIOD:	str = "1981-2010"; break;
		case FUTUR_PERIOD:	str = "3|4|5|6|7|8|9|10|11"; break;
		case NB_NEIGHBOR:	str = "3"; break;
		case MAX_DISTANCE:	str = "500"; break;
		case POWER:			str = "2"; break;
		};

		return str;
	}

	ERMsg CCreateNormalsDB::Execute(CCallback& callback)
	{
		ASSERT(m_pProject);//parent must be set for creator
		
		ERMsg msg;

		//CPLSetConfigOption("GDAL_CACHEMAX", "128");
		//for(int y=0; y<12&&msg; y++)
		//{
		//	short firstYear=1961+10*y;
		//	short lastYear=firstYear+30-1;

		//	string filePath;
		//	filePath.Format("d:\\weather\\OuranosGrid%d-%d.Normals", firstYear, lastYear);
		//	CNormalsDatabase normalFile;
		//	msg+=normalFile.Open(filePath, CNormalsDatabase::modeEdit);
		//	if( msg)
		//	{
		//		for(int i=0; i<normalFile.size(); i++)
		//		{
		//			CNormalsStation station;
		//			normalFile.GetAt(i, station);
		//			if( station.GetElev() == 0)
		//			{
		//				station.SetElev( Rand(0,5) );
		//				msg += normalFile.SetAt(i, station);
		//			}
		//		}
		//	}
		//}

		//	COuranosDatabase ouranosDatabase(m_MMGFilePath);
		//ouranosDatabase.ExtractMask("c:\\temp\\grid\\mask.flt", callback);

		/*for(int y=0; y<12&&msg; y++)
		{
		short firstYear=1961+10*y;
		short lastYear=firstYear+30-1;

		string filePath;
		filePath.Format("C:\\ouranos_data\\Normals\\OuranosGrid%d-%d.Normals", firstYear, lastYear);
		msg += ouranosDatabase.CreateNormal( firstYear, lastYear, filePath, callback);
		}

		return msg;*/


		//CGeoPointWP pt(-70.37, 46.8, CProjection(CProjection::GEO) );
		//CClimaticChange cc;
		//msg += cc.Open(m_MMGFilePath);
		//msg += cc.ExportMonthlyValue(1971, 2100, pt, "C:\\temp\\testCC.csv", callback );
		//return msg;

		//msg+=ouranosDatabase.ExtractTopo("D:\\Travail\\OURANOS\\MapInput\\Topo.flt", callback);
		//msg+=ouranosDatabase.ExtractRealTopo("D:\\Travail\\OURANOS\\MapInput\\RealTopoHight.flt", true, callback);
		//	msg += ouranosDatabase.CreateCCGridFromNormals( filePathIn, pathOut, callback );
		//	return msg;

		/*	COuranosDatabase ouranosDatabase(m_MMGFilePath);
			for(int m=0;m<12;m++)
			{
			string tmp1;
			string tmp2;
			string tmp3;
			tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testtmin%d.flt", m+1);
			tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testtmin%ddelta.flt", m+1);
			tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffTmin%d.flt", m+1);
			ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, true, callback);

			tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testTmax%d.flt", m+1);
			tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testTmax%ddelta.flt", m+1);
			tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffTmax%d.flt", m+1);
			ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, true, callback);

			tmp1.Format( "D:\\Travail\\OURANOS\\MapOutput\\testPrcp%d.flt", m+1);
			tmp2.Format( "D:\\Travail\\OURANOS\\MapOutput\\testPrcp%ddelta.flt", m+1);
			tmp3.Format( "D:\\Travail\\OURANOS\\MapOutput\\DiffPrcp%d.flt", m+1);
			ouranosDatabase.GetDiffMap(tmp1, tmp2, tmp3, false, callback);
			}

			return msg;
			*/


		//msg+=ouranosDatabase.GetDiffMap("C:\\temp\\Maps\\Precip2001-2030.flt","C:\\temp\\Maps\\Precip1971-2000.flt","C:\\temp\\Maps\\DiffPrecip2001-2030.flt",false,callback);
		//msg+=ouranosDatabase.GetDiffMap("C:\\ouranos_data\\MapOutput\\prcp 2001-2030.flt", "C:\\ouranos_data\\MapOutput\\prcp 1971-2000.flt", "C:\\ouranos_data\\MapOutput\\Diffprcp 2001-2030.flt",false,callback);
		//msg+=ouranosDatabase.GetDiffMap("C:\\ouranos_data\\MapOutput\\Diffprcp 2001-2030.flt", "C:\\temp\\Maps\\DiffPrecip2001-2030.flt", "C:\\temp\\MapsDiff\\DiffPrecip2001-2030.flt", true, callback);

		/*
			for(int y=1; y<4&&msg; y++)
			{
			short firstYear=1971+30*y;
			short lastYear=firstYear+30-1;

			string filePathIn;
			filePathIn.Format("d:\\weather\\OuranosGrid%d-%d.Normals",firstYear, lastYear);
			string pathOut;
			pathOut.Format("%sadj\\Maps\\%d-%d\\", m_MMGFilePath, firstYear, lastYear);
			CreateMultipleDir( pathOut );

			msg += ouranosDatabase.CreateCCGridFromNormals( filePathIn, pathOut, callback );

			for(int m=0; m<12&&msg; m++)
			{
			for(int v=0; v<NB_VAR_BASE&&msg; v++)
			{
			string filePath;
			filePath.Format("%sadj\\Maps\\%d-%d\\%s%s%d-%d.bil", m_MMGFilePath, firstYear, lastYear,CWeatherDefine::GetFormatNameD(v), GetMonthName(m, false), firstYear, lastYear);
			msg += ouranosDatabase.CreateCCGrid( v, firstYear, lastYear, m, filePath, callback);

			//string filePath2;
			//filePath2.Format("C:\\temp\\MapsMonthly\\%s%s%d-%d.csv", CWeatherDefine::GetFormatNameD(v), GetMonthName(m, false), firstYear, lastYear);
			//for(int y2=0; y2<30; y2++)
			//	ouranosDatabase.ExportData(firstYear+y2,m,CGeoPointWP(-71.3799999873, 46.8,CProjection(CProjection::GEO)), filePath2, callback);

			//filePath.Format("C:\\temp\\Maps\\%s%d-%d.bil", CWeatherDefine::GetFormatNameD(v), firstYear, lastYear);
			//msg += ouranosDatabase.CreateCCGrid( v, firstYear, lastYear, -1, filePath, callback);
			}
			}
			}

			return msg;
			*/

		//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, JULY, "C:\\temp\\prcpJuly1971-2000.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, AUGUST, "C:\\temp\\prcpAugust1971-2000.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, 1971, SEPTEMBER, "C:\\temp\\prcpSeptember1971-2000.bil", callback);

		//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, JUNE, "C:\\temp\\prcpJune2001-2030.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, JULY, "C:\\temp\\prcpJuly2001-2030.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, AUGUST, "C:\\temp\\prcpAugust2001-2030.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, 2001, SEPTEMBER, "C:\\temp\\prcpSeptember2001-2030.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( TMIN, "C:\\temp\\adjTmin.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( TMAX, "C:\\temp\\adjTmax.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( PRCP, "C:\\temp\\adjPrcp.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( WNDS, "C:\\temp\\adjWnds.bil", callback);
		//msg += ouranosDatabase.CreateCCGrid( RELH, "C:\\temp\\adjrelh.bil", callback);


		//bool bRep = ouranosDatabase.VerifyZero(PRCP);
		//if( bRep )
		//	msg.ajoute("PRCP ok");
		//else msg.ajoute("PRCP pas ok");

		//ouranosDatabase.ExtractTopo("c:\\temp\\topo.nc");
		//return msg;
		//ouranosDatabase.CreateCCGrid(TMIN, 2071, JANUARY, "c:\\temp\\grid\\2071-2100\\tmin2071-2100jan.flt");
		//ouranosDatabase.CreateCCGrid(TMIN, 2071, JULY, "c:\\temp\\grid\\2071-2100\\tmin2071-2100jul.flt");
		//ouranosDatabase.CreateCCGrid(TMAX, 2071, JANUARY, "c:\\temp\\grid\\2071-2100\\tmax2071-2100jan.flt");
		//ouranosDatabase.CreateCCGrid(TMAX, 2071, JULY, "c:\\temp\\grid\\2071-2100\\tmax2071-2100jul.flt");
		//ouranosDatabase.CreateCCGrid(PRCP, 2071, JANUARY, "c:\\temp\\grid\\2071-2100\\prcp2071-2100jan.flt");
		//ouranosDatabase.CreateCCGrid(PRCP, 2071, JULY, "c:\\temp\\grid\\2071-2100\\prcp2071-2100jul.flt");
		//ouranosDatabase.CreateCCGrid(RELH, 2071, JANUARY, "c:\\temp\\grid\\2071-2100\\relh2071-2100jan.flt");
		//ouranosDatabase.CreateCCGrid(RELH, 2071, JULY, "c:\\temp\\grid\\2071-2100\\relh2071-2100jul.flt");
		//ouranosDatabase.CreateCCGrid(WNDS, 2071, JANUARY, "c:\\temp\\grid\\2071-2100\\wnds2071-2100jan.flt");
		//ouranosDatabase.CreateCCGrid(WNDS, 2071, JULY, "c:\\temp\\grid\\2071-2100\\wnds2071-2100jul.flt");


		//msg = CTYNClimaticChange::CreateBillFromATEAM(m_MMGFilePath, callback);
		//return msg;

		//msg = CCerfacsMMG::CreateMMG(m_MMGFilePath, m_outputFilePath, callback);
		//return msg;

		//msg=CCerfacsClimaticChange::CreateGrid(m_MMGFilePath+"Cerfacs_WHC.loc",m_MMGFilePath+"Cerfacs_WHC.flt", callback);
		//msg+=CCerfacsClimaticChange::CreateGrid(m_MMGFilePath+"Cerfacs_Altitude.loc",m_MMGFilePath+"Cerfacs_Altitude.flt", callback);

		//msg = CCerfacsMMG::CompareWHC(m_MMGFilePath+"Cerfacs_WHC.loc", m_MMGFilePath+"AWC_HWSD_30as(2m).flt",  "C:\\Travail\\IsabelChuine\\water holding capacity\\Webb\\1deg-WHC.flt","C:\\Travail\\IsabelChuine\\water holding capacity\\IGBP\\pawc.flt",m_MMGFilePath+"ComparisonWHC.csv");
		//return msg;




		//msg=CCerfacsClimaticChange::CreateDailyFile(m_MMGFilePath,1971,2000,false,callback);
		//return msg;

		//COuranosDatabase data(m_MMGFilePath);
		//int t = sizeof( CNormalsStation );
		//data.CreateMMGNew(callback);

		//return msg;


		if (as<int>(NB_YEARS_MIN)<= 1)
		{
			msg.ajoute("The number of year must be greater then 1");
			return msg;
		}

		string outputFilePath = Get(OUTPUT);
		if (outputFilePath.empty())
		{
			msg.ajoute(GetString(IDS_BSC_NAME_EMPTY));
			return msg;
		}

		SetFileExtension(outputFilePath, CNormalsDatabase::DATABASE_EXT);

		callback.AddMessage(GetString(IDS_CREATE_DB));
		callback.AddMessage(outputFilePath, 1);

		msg = CNormalsDatabase::DeleteDatabase(outputFilePath, callback);

		if (msg)
		{ 
			CNormalFromDaily normalFromDaily;
			normalFromDaily.m_firstYear = as<int>(FIRST_YEAR);
			normalFromDaily.m_lastYear = as<int>(LAST_YEAR);
			normalFromDaily.m_nbYearMin = as<int>(NB_YEARS_MIN);
			normalFromDaily.m_bApplyCC = as<bool>(APPLY_CLIMATIC_CHANGE);
			normalFromDaily.m_inputMMGFilePath = Get(MMG_FILEPATH);
			//normalFromDaily.m_bCreateAll = false;
			StringVector ref ( Get(NORMAL_PERIOD), "-");
			ASSERT(ref.size() == 2);
			//normalFromDaily.m_firstYear = .m_refPeriodIndex = as<int>(NORMAL_PERIOD)-1;
			normalFromDaily.m_firstRefYear = WBSF::as<int>(ref[0]);
			normalFromDaily.m_nbRefYears = WBSF::as<int>(ref[1]) - WBSF::as<int>(ref[0]) + 1;
			normalFromDaily.m_CCPeriodIndex = GetCCPeriod();
			normalFromDaily.m_inputDBFilePath = Get(INPUT);
			normalFromDaily.m_outputDBFilePath = outputFilePath;
			normalFromDaily.m_nbNeighbor = as<double>(NB_NEIGHBOR);
			normalFromDaily.m_maxDistance = as<double>(MAX_DISTANCE)*1000;//[km] to [m]
			normalFromDaily.m_power = as<double>(POWER);

			msg = normalFromDaily.CreateNormalDatabase(callback);
		}

		return msg;
	}

	CNormalFromDaily::CCPeriodBitset CCreateNormalsDB::GetCCPeriod()const
	{
		CNormalFromDaily::CCPeriodBitset CCPeriod;
		StringVector index(Get(FUTUR_PERIOD), "|");
		for (size_t i = 0; i < index.size(); i++)
			if (ToInt(index[i]) < CCPeriod.size())
				CCPeriod.set(ToInt(index[i]));

		return CCPeriod;
	}
	

}