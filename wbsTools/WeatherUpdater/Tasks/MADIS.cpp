#include "StdAfx.h"
#include "MADIS.h"
#include "UtilTime.h"
//#include "Projection.h"
#include "UtilWin.h"
#include "CommonRes.h"
//#include "DailyYear.h"
#include "UtilMath.h"
//#include "ShapeFileBase.h"
//#include "DailyStation.h"
//#include "UtilTime.h"
//#include "NormalFile.h"
//#include "AdvancedNormalStation.h"
//#include "NewMat.h"

using namespace CFL;
using namespace UtilWin;
using namespace DAILY_DATA;

//CGeoGridWP CMADIS::DATA_GRID;
//const char* CMADIS::VAR_NAME[NB_VAR_BASE] = {"stmn","stmx","pcp","", "sq","swmx"};

CMADIS::CMADIS(const CString& path)
{
	SetPath(path);
}

CMADIS::~CMADIS(void)
{
}

//CString CMADIS::GetFilePath(short v, short year, short m)const
//{
//	ASSERT( !m_path.IsEmpty() );
//	ASSERT( v>=0&&v<NB_VAR_BASE);
//	ASSERT( m>=0&&m<12);
//	ASSERT( year>=1961&&year<=2100);
//
//	CString tmp;
//	tmp.Format("%sadj\\adj_p1%s\\adj_p1%s_%4d%02d.nc", m_path, VAR_NAME[v], VAR_NAME[v], year, m+1);
//	return tmp;
//}
//
//ERMsg CMADIS::GetDiffMap(const CString& filePath1, const CString& filePath2, const CString& filePath3, bool sub, CSCCallBack& callback)
//{
//	ERMsg msg;
//
//	CMapBinaryFile grid1;
//	CMapBinaryFile grid2;
//	
//
//	msg += grid1.Open(filePath1);
//	msg += grid2.Open(filePath2);
//	ASSERT(grid1.GetNbProfile()==grid2.GetNbProfile());
//	ASSERT(grid1.GetNbCellByProfile()==grid2.GetNbCellByProfile());
//
//	CMapBinaryFile gridDiff( grid1 );
//	if(msg)
//		msg += gridDiff.Open(filePath3, CGeoFileInterface::modeWrite);
//
//	if(!msg)
//		return msg;
//
//
//	callback.SetCurrentDescription("Create diff");
//	callback.SetCurrentStepRange(0, 172*180, 1);
//	callback.SetStartNewStep();
//
//	for(int y=0; y<grid1.GetNbProfile(); y++)
//	{
//		for(int x=0; x<grid1.GetNbCellByProfile(); x++)
//		{
//			CGeoMapPoint mapPt1;
//			CGeoMapPoint mapPt2;
//			grid1.ReadCell(mapPt1);
//			grid2.ReadCell(mapPt2);
//			
//			double diff = grid1.GetNoData();
//			if( mapPt1.m_z > grid1.GetNoData() &&
//				mapPt2.m_z > grid2.GetNoData() )
//				diff = sub?mapPt1.m_z-mapPt2.m_z:mapPt1.m_z/mapPt2.m_z;
//			
//
//			msg += gridDiff.WriteCell(float(diff));
//			msg += callback.StepIt();
//		}
//	}
//
//	grid1.Close();
//	grid2.Close();
//	gridDiff.Close(true);
//	
//	
//	return msg;
//}


ERMsg CMADIS::ExtractLOC(const CString& filePath, CSCCallBack& callback)
{
	ERMsg msg;

	callback.SetCurrentDescription("Extract location");
	callback.SetCurrentStepRange(0, 3, 1);
	callback.SetStartNewStep();

	CString outputFilePath(filePath);
	UtilWin::SetFileExtension( outputFilePath, ".loc");
	
//	CString outputFilePathGrid(filePath);
//	UtilWin::SetFileExtension( outputFilePathGrid, ".bil");

//	CShapeFileBase shapefile;
//	shapefile.Read("C:\\ouranos_data\\MapInput\\canada.shp");

	

	NcFile file(filePath);	

	if( !file.is_valid() )
	{
		CString err;
		err.FormatMessage(IDS_CMN_UNABLE_OPEN_READ, filePath);
		msg.ajoute(err);
	}

	//CMapBinaryFile grid;
 //   grid.SetProjection( GetDataGrid().GetPrj() );
 //   grid.SetNbCols(180);
 //   grid.SetNbRows(172);
 //   grid.SetBoundingBox(GetDataGrid());
 //   grid.SetNoData(-9999);
 //   grid.SetCellSizeX( 45000 );
 //   grid.SetCellSizeY( 45000 );
	CStdioFileEx fileOut;


	//msg += grid.Open(outputFilePathGrid, CGeoFileInterface::modeWrite);
	msg += fileOut.Open( outputFilePath, CFile::modeCreate|CFile::modeWrite);
	if(!msg)
		return msg;

	fileOut.WriteString("LOC_FILE 4 2,ID,Latitude,Longitude,Elevation,Slope(%),Orientation(°)\n");
	

	//char providerId(recNum, maxProviderIdLen) ;
	//	providerId:long_name = "Data Provider station Id" ;
	//	providerId:reference = "station table" ;
	//char stationId(recNum, maxStaIdLen) ;
	//	stationId:long_name = "alphanumeric station Id" ;
	//	stationId:reference = "station table" ;
	//char handbook5Id(recNum, maxStaIdLen) ;
	//	handbook5Id:long_name = "Handbook5 Id (AFOS or SHEF id)" ;
	//	handbook5Id:reference = "station table" ;
	//char stationName(recNum, maxNameLength) ;
	//	stationName:long_name = "alphanumeric station name" ;
	//	stationName:reference = "station table" ;

	//float latitude(recNum) ;
	//	latitude:long_name = "latitude" ;
	//	latitude:units = "degree_north" ;
	//	latitude:_FillValue = 3.4028235e+038f ;
	//	latitude:missing_value = -9999.f ;
	//	latitude:reference = "station table" ;
	//	latitude:standard_name = "latitude" ;
	//float longitude(recNum) ;
	//	longitude:long_name = "longitude" ;
	//	longitude:units = "degree_east" ;
	//	longitude:_FillValue = 3.4028235e+038f ;
	//	longitude:missing_value = -9999.f ;
	//	longitude:reference = "station table" ;
	//	longitude:standard_name = "longitude" ;
	//float elevation(recNum) ;
	//	elevation:long_name = "elevation" ;
	//	elevation:units = "meter" ;
	//	elevation:_FillValue = 3.4028235e+038f ;
	//	elevation:missing_value = -9999.f ;
	//	elevation:reference = "station table" ;
	//	elevation:standard_name = "elevation" ;

	//NcVar* pVarX = file.get_var("xc");
	//NcVar* pVarY = file.get_var("yc");
	
	NcVar* pVarLat = file.get_var("staLat");//file.get_var("latitude");
	NcVar* pVarLon = file.get_var("staLon");//file.get_var("longitude");
	NcVar* pVarElev = file.get_var("staElev");//file.get_var("elevation");
	NcDim* pDim = pVarLat->get_dim(0);
	//double offset = pVarElev->get_att("add_offset")->as_float(0);
	//double scaleFactor = pVarElev->get_att("scale_factor")->as_float(0);

	int nbRect = pDim->size();
	//delete pDim; pDim = NULL;

	//typedef boost::multi_array<char, 1> CDataCharArray;
	typedef boost::multi_array<float, 1> CDataFloatArray;
	//CDataFloatArray X(boost::extents[180]);
	//CDataFloatArray Y(boost::extents[172]);
	CDataFloatArray lat(boost::extents[nbRect]);
	CDataFloatArray lon(boost::extents[nbRect]);
	CDataFloatArray elev(boost::extents[nbRect]);

	ASSERT( pVarLat->get_dim(0)->size() == pVarLon->get_dim(0)->size() );
	ASSERT( pVarLat->get_dim(0)->size() == pVarElev->get_dim(0)->size() );
	
	//pVarX->get(&(X[0]), 180);
	//pVarY->get(&(Y[0]), 172);
	pVarLat->get(&(lat[0]), nbRect);callback.StepIt();
	pVarLon->get(&(lon[0]), nbRect);callback.StepIt();
	pVarElev->get(&(elev[0]), nbRect);callback.StepIt();

	
	//CProjection prj = GetDataGrid().GetPrj();

	for(int i=0; i<nbRect; i++)
	{
		double Lat=0, Lon=0; 
		
		CString line;
		line.Format("%d,%d,%f,%f,%.1f,0,0\n",i+1,i+1,lat[i],lon[i],elev[i]);
		fileOut.WriteString(line);
	}

	
	fileOut.Close();
	
	return msg;
}
/*
ERMsg CMADIS::ExtractMask(const CString& filePath, CSCCallBack& callback)
{
	ERMsg msg;

	CLocArray loc;

	
	callback.SetCurrentDescription("Create Mask");
	callback.SetCurrentStepRange(0, 172*180, 1);
	callback.SetStartNewStep();

	CMapBinaryFile grid;
    grid.SetProjection( GetDataGrid().GetPrj() );
    grid.SetNbCols(180);
    grid.SetNbRows(172);
    grid.SetBoundingBox(GetDataGrid());
    grid.SetNoData(-9999);
    grid.SetCellSizeX( 45000 );
    grid.SetCellSizeY( 45000 );

	msg += grid.Open(filePath, CGeoFileInterface::modeWrite);
	msg += loc.Load("c:\\temp\\grid\\mask.loc");
	if(!msg)
		return msg;

	
	CGeoGridWP rect = GetDataGrid();

	for(int y=0; y<172; y++)
	{
		for(int x=0; x<180; x++)
		{
			CPoint ptIn(x,y);
			CGeoPointWP ptOut;
			
			rect.ColRowToCartoCoord(ptIn, ptOut);
			ptOut.ReprojectToGeographic();

			//Find the nearest point in the loc
			int index = loc.Find(ptOut);
			ASSERT( index >=0 );
			
			msg += grid.WriteCell((float)loc[index].GetElev());
			msg += callback.StepIt(1.0);
		}
	}

	grid.Close(true);
	
	
	return msg;
}

CGeoGridWP CMADIS::GetDataGrid()
{
	if( DATA_GRID.IsRectNull() )
	{
		//I don't know why I have to adjust false easting and false northing
		CProjParam param;
		param.SetParam(CProjParam::CENTER_LON, 245-360);
		param.SetParam(CProjParam::CENTER_LAT, 60);
		param.SetParam(CProjParam::FALSE_EASTING, 2745000-45000);
		param.SetParam(CProjParam::FALSE_NORTHING, 8068500-45000);
		CSpheroids spheroids(CSpheroids::SPHERE_OF_RADIUS);

		CProjection prj(CProjection::POLAR_STEREO, param, spheroids, CProjectionBase::UT_METERS);
		CGeoRect rect( 0, 0, 8100000, 7740000, CProjection::POLAR_STEREO);
		DATA_GRID.Set(172, 180, rect, prj);
	}

	return DATA_GRID;
}

double ConvertData(short v, double in)
{
	double out=0;
	switch(v)
	{
	case TMIN:
	case TMAX: out=in-273.16; break;// °K -> °C
	case PRCP: out=in*3600*24; break;// mm/s -> mm/day
	case WNDS: out=in*3600/1000; break;//m/s -> km/h
	case TDEW:
	case RELH: out=in; break; //no conversion because T missing
	default: ASSERT(false);
	}

	return out;
}

ERMsg CMADIS::CreateCCGrid( short v, short firstccYear, short lastccYear, short m0, const CString& filePath, CSCCallBack& callback )
{
	ASSERT( v>=0 && v<NB_VAR_BASE);
	ASSERT( firstccYear>=1961 && firstccYear<=2071 );
	ASSERT( m0>=-1 && m0<12);

	ERMsg msg;

	int m1= m0==-1?0:m0;
	int m2= m0==-1?12:m0+1;

	int nbYear = lastccYear-firstccYear+1;	
	int nbMonth = m2-m1;
	callback.SetCurrentDescription("Create Bil");
	callback.SetCurrentStepRange(0, (nbYear*nbMonth)+1, 1);
	callback.SetStartNewStep();

	//take all cell
	CRect rect(0,0,180,172);
	CStatisticMultiArray cc(boost::extents[rect.Height()][rect.Width()]);
	
	
	for(int y=0; y<nbYear&&msg; y++)
	{
		short year=firstccYear+y;

		for(int m=m1; m<m2; m++)
		{
			if( v==RELH || v==TDEW )
			{
				int nbDays = CFL::GetNbDayPerMonth(m);
				COneMonthGrid data(boost::extents[nbDays][rect.Height()][rect.Width()]);
				COneMonthGrid Tmin(boost::extents[nbDays][rect.Height()][rect.Width()]);
				COneMonthGrid Tmax(boost::extents[nbDays][rect.Height()][rect.Width()]);
				msg += ReadData( RELH, year, m, rect, data);
				msg += ReadData( TMIN, year, m, rect, Tmin);
				msg += ReadData( TMAX, year, m, rect, Tmax);

				//oneMonthStat.resize( boost::extents[data[0].size()][data[0][0].size()]);
		
				for(int d=0; d<(int)data.size(); d++)
				{
					for(int y=0; y<(int)data[d].size(); y++)
					{
						for(int x=0; x<(int)data[d][y].size(); x++)
						{
							double TminC = ConvertData( TMIN, Tmin[d][y][x]);
							double TmaxC = ConvertData( TMAX, Tmax[d][y][x]);
							double Tmean = (TminC+TmaxC)/2;
							double Hr = CFL::Hs2Hr(Tmean, data[d][y][x]);
							double Td = CFL::Hr2Td(TminC, TmaxC, Hr);
							
							if( v==RELH)
								cc[y][x] += Hr;
							else cc[y][x] += Td;
						}
					}
				}
			}
			else 
			{
				//int nbDays = CFL::GetNbDayPerMonth(m);
				//COneMonthGrid data(boost::extents[nbDays][rect.Height()][rect.Width()]);
				msg += ReadData( v, year, m, rect, cc);
				//DailyToStat(data, cc);
			}

			msg += callback.StepIt();
		}
	}
	


	CMapBinaryFile grid;
    grid.SetProjection( GetDataGrid().GetPrj() );
    grid.SetNbCols(180);
    grid.SetNbRows(172);
    grid.SetBoundingBox(GetDataGrid());
    grid.SetNoData(-9999);
    grid.SetCellSizeX( 45000 );
    grid.SetCellSizeY( 45000 );


	msg += grid.Open(filePath, CGeoFileInterface::modeWrite);
	if( msg)
	{
		for(int y=0; y<172&&msg; y++)
		{
			for(int x=0; x<180&&msg; x++)
			{
				double value = 0;
				if( v == PRCP)
					value=ConvertData(v, cc[y][x][SUM])/nbYear;
				else value=ConvertData(v, cc[y][x][MEAN]);
				
				msg+=grid.WriteCell(float(value));
				msg += callback.StepIt(1.0/(172*180));
			}
		}
		
		grid.Close(true);
	}

	return msg;
}

ERMsg CMADIS::CreateCCGrid( short v, const CString& filePath, CSCCallBack& callback)
{
	ASSERT( v>=0 && v<NB_VAR_BASE);

	ERMsg msg;


	CMapBilFile grid;
	grid.SetCellType(CMapBilFile::FLOAT);
    grid.SetProjection( GetDataGrid().GetPrj() );
    grid.SetNbCols(180);
    grid.SetNbRows(172);
	grid.SetNbBand(140*12);//140 year * 12 month
    grid.SetBoundingBox(GetDataGrid());
    grid.SetNoData(-9999);
    grid.SetCellSizeX( 45000 );
    grid.SetCellSizeY( 45000 );
	msg += grid.Open(filePath, CGeoFileInterface::modeWrite|CGeoFileInterface::modeDirectAccess);
	if( !msg)
		return msg;

	//take all cell
	CRect rect(0,0,180,172);


	callback.SetCurrentDescription("Create Bil");
	callback.SetCurrentStepRange(0, 140*12*172*180, 1);
	callback.SetStartNewStep();

	
	for(int y=0; y<140&&msg; y++)
	{
		short year=1961+y;
		
		for(int m=0; m<12&&msg; m++)
		{
			CStatisticMultiArray cc(boost::extents[rect.Height()][rect.Width()]);
			msg += ReadData( v, year, m, rect, cc);

			for(int r=0; r<172&&msg; r++)
			{
				for(int c=0; c<180&&msg; c++)
				{
					float value = float(cc[r][c][MEAN]);
					msg += grid.WriteCell(value,c,r,y*12+m);
					msg += callback.StepIt();
				}
			}
		}
	}

	grid.Close(true);

	return msg;
}

ERMsg CMADIS::CreateNormal( short firstccYear, short lastccYear, const CString& filePath, CSCCallBack& callback )
{
	ASSERT( firstccYear>=1961 && firstccYear<=2071 );

	ERMsg msg;


	CString tmp;
	tmp.Format("%stopo.nc", m_path);
	NcFile file(tmp);
	
	NcVar* pVarLat = file.get_var("lat");
	NcVar* pVarLon = file.get_var("lon");
	NcVar* pVarElev = file.get_var("topo");
	NcAtt* pAttOffset = pVarElev->get_att("add_offset");
	NcAtt* pAttScaleFactor = pVarElev->get_att("scale_factor");
	double offset = pAttOffset->as_float(0);
	double scaleFactor = pAttScaleFactor->as_float(0);

	typedef boost::multi_array<short, 2> CDataArray;
	typedef boost::multi_array<float, 2> CDataFloatArray;
	CDataFloatArray lat(boost::extents[172][180]);
	CDataFloatArray lon(boost::extents[172][180]);
	CDataArray elev(boost::extents[172][180]);
	
	pVarLat->get(&(lat[0][0]), 172, 180);
	pVarLon->get(&(lon[0][0]), 172, 180);
	pVarElev->get(&(elev[0][0]), 1, 172, 180);

//	delete pVarLat;pVarLat=NULL;
//	delete pVarLon;pVarLon=NULL;
//	delete pVarElev;pVarElev=NULL;
	delete pAttOffset; pAttOffset = NULL;
	delete pAttScaleFactor; pAttScaleFactor = NULL;
	
	tmp.Format("%s\\MapInput\\mask.flt", m_path);
	CMapBinaryFile maskMap;

	msg += maskMap.Open(tmp, CGeoFileInterface::modeRead|CGeoFileInterface::modeDirectAccess);


	CNormalFile::DeleteDatabase(filePath);

	CNormalFile normalDB;
	normalDB.SetBeginYear(firstccYear);
	normalDB.SetEndYear(lastccYear);
	msg += normalDB.Open(filePath, CFile::modeWrite);

	int nbYear = lastccYear-firstccYear+1;	
	callback.SetCurrentDescription("Create normals");
	callback.SetCurrentStepRange(0, 180*172*(nbYear*12)+1, 1);
	callback.SetStartNewStep();

	for(int y=0; y<172&&msg; y++)
	{
		for(int x=0; x<180&&msg; x++)
		{
			int value = (int)maskMap.ReadCell(x, y);

			if( value == -1)
			{

				//take all cell
				CRect rect(x,y,x+1,y+1);
				//CStatisticMultiArray cc(boost::extents[rect.Height()][rect.Width()]);

				CDailyStation station;
				CYearPackage package;
				
				CString name;
				name.Format("%03d-%03d",x+1,y+1);
				station.SetName(name);
				station.SetID("MRCC " + name);
				station.SetLat(lat[y][x]);
				station.SetLon(lon[y][x]-360);
				station.SetElev(short(elev[y][x]*scaleFactor+offset));

				for(int yr=0; yr<nbYear&&msg; yr++)
				{
					CDailyData dailyData;

					short year=firstccYear+yr;

					for(int m=0; m<12&&msg; m++)
					{
						int nbDays = CFL::GetNbDayPerMonth(m);
						COneMonthGrid Tmin(boost::extents[nbDays][1][1]);
						COneMonthGrid Tmax(boost::extents[nbDays][1][1]);
						COneMonthGrid Prcp(boost::extents[nbDays][1][1]);
						COneMonthGrid Relh(boost::extents[nbDays][1][1]);
						COneMonthGrid Wnds(boost::extents[nbDays][1][1]);
						msg += ReadData( TMIN, year, m, rect, Tmin);
						msg += ReadData( TMAX, year, m, rect, Tmax);
						msg += ReadData( PRCP, year, m, rect, Prcp);
						msg += ReadData( RELH, year, m, rect, Relh);
						msg += ReadData( WNDS, year, m, rect, Wnds);
						
						for(int d=0; d<nbDays&&msg; d++)
						{
							double TminC = ConvertData( TMIN, Tmin[d][0][0]);
							double TmaxC = ConvertData( TMAX, Tmax[d][0][0]);
							double Tmean = (TminC+TmaxC)/2;
							double Hr = CFL::Hs2Hr(Tmean, Relh[d][0][0]);
							double Td = CFL::Hr2Td(TminC, TmaxC, Hr);
							
							int jd = CFL::GetJDay(year, m, d);
							dailyData[jd][TMIN] = (float)TminC;
							dailyData[jd][TMAX] = (float)TmaxC;
							dailyData[jd][PRCP] = (float)ConvertData( PRCP, Prcp[d][0][0]);
							dailyData[jd][RELH] = (float)Hr;
							dailyData[jd][TDEW] = (float)Td;
							dailyData[jd][WNDS] = (float)ConvertData( WNDS, Wnds[d][0][0]);
						}//day
						msg += callback.StepIt();
					}//month

					CDailyYear dailyYear(year);
					dailyYear.SetData(dailyData);
					package.SetYear(dailyYear);
					
				}//year

				station.AddPakage(package);

				CAdvancedNormalStation normalsStations;
				msg+=normalsStations.FromDaily(station, 1);
				msg+=normalDB.Add(normalsStations);
			}//if continental
		}//x
	}//y
	
	return msg;
}


ERMsg CMADIS::ReadData( short v, short year, short m, CRect rect, CStatisticMultiArray& oneMonthStat)const
{
	ERMsg msg;

	int nbDays = CFL::GetNbDayPerMonth(m);
	COneMonthGrid data(boost::extents[nbDays][rect.Height()][rect.Width()]);
	
	msg = ReadData( v, year, m, rect, data);

	if( msg )
	{
		DailyToStat(data, oneMonthStat);
	}

	return msg;
}

void CMADIS::DailyToStat(const COneMonthGrid& data, CStatisticMultiArray& oneMonthStat)
{
	oneMonthStat.resize( boost::extents[data[0].size()][data[0][0].size()]);
	//oneMonthStat.resize(boost::extents[1][1][1]
	for(int d=0; d<(int)data.size(); d++)
	{
		for(int y=0; y<(int)data[d].size(); y++)
		{
			//ATTENTION au 31 decembre 2100 surtout Tmax: a vérifier...
			for(int x=0; x<(int)data[d][y].size(); x++)
			{
				oneMonthStat[y][x] += data[d][y][x];
			}
		}
	}
}

double CMADIS::GetMonthlyRatio(const COneMonthGrid& dataRef, const COneMonthGrid& data)
{
	ASSERT( dataRef[0].size() == 1 && dataRef[0][0].size() == 1);
	ASSERT( data[0].size() == 1 && data[0][0].size() == 1);

	CStatisticMultiArray stat;
	CStatisticMultiArray statRef;
	DailyToStat(dataRef, statRef);
	DailyToStat(data, stat);

	ASSERT( statRef[0][0][MEAN] > 0);
	

	return (statRef[0][0][MEAN]==0)?1:stat[0][0][SUM]/statRef[0][0][SUM];
}


ERMsg CMADIS::ReadData( short v, short year, short m, CRect rect, COneMonthGrid& oneMonthGrid)const
{
	ASSERT( rect.left>=0&&rect.left<180);
	ASSERT( rect.right>0&&rect.right<=180);
	ASSERT( rect.top>=0&&rect.top<172);
	ASSERT( rect.bottom>0&&rect.bottom<=172);

	ERMsg msg;
	
	NcError::set_err( NcError::silent_nonfatal );

  
	CString filePath = GetFilePath(v, year, m);
	NcFile file(filePath);	//current year file

	if( !file.is_valid() )
	{
		CString err;
		err.FormatMessage(IDS_CMN_UNABLE_OPEN_READ, filePath);
		msg.ajoute(err);
	}

	if(!msg)
		return msg;

			
	int nbDays = CFL::GetNbDayPerMonth(m);

	NcVar* pVar = file.get_var(VAR_NAME[v]);
	NcAtt* pAttOffset = pVar->get_att("add_offset");
	NcAtt* pAttScaleFactor = pVar->get_att("scale_factor");
	float offset = pAttOffset->as_float(0);
	float scaleFactor = pAttScaleFactor->as_float(0);
	
	typedef boost::multi_array<short, 3> CDataArray;
	CDataArray data(boost::extents[nbDays][rect.Height()][rect.Width()]);
	//typedef short CDataArray[1][1][1];
	//CDataArray data;
	if( v == PRCP )
	{
		pVar->set_cur(0, rect.top, rect.left);
		if( !pVar->get(&(data[0][0][0]), nbDays, rect.Height(), rect.Width()) )
		{
			msg.ajoute( "Unable to get NetCDFData");
			return msg;
		}
	}
	else
	{
		pVar->set_cur(0, 0, rect.top, rect.left);
		if( !pVar->get(&(data[0][0][0]), nbDays, 1, rect.Height(), rect.Width()) )
		{
			msg.ajoute( "Unable to get NetCDFData");
			return msg;
		}
	}
	
	//adjust scale factor ans offset
	for(int y=0; y<rect.Height(); y++)
	{
		for(int x=0; x<rect.Width(); x++)
		{
			//ATTENTION au 31 decembre 2100 surtout Tmax: a vérifier...
			for(int d=0; d<nbDays; d++)
			{
				oneMonthGrid[d][y][x] = data[d][y][x]*scaleFactor+offset;
			}
		}
	}

	//delete pVar; pVar = NULL;
	delete pAttOffset; pAttOffset = NULL;
	delete pAttScaleFactor; pAttScaleFactor = NULL;

	return msg;
}

bool CMADIS::VerifyZero(short v)
{
	bool bRep = true;
	//take all cell
	CRect rect(0,0,180,172);
	CStatisticMultiArray cc(boost::extents[rect.Height()][rect.Width()]);
		
	for(int y=0; y<30&&bRep; y++)
	{
		short year=1971+y;
		for(int m=0; m<12&&bRep; m++)
		{
			ReadData( v, year, m, rect, cc);
			for(int y=0; y<172; y++)
				for(int x=0; x<180; x++)
					if(cc[y][x][SUM]<0.1)
						bRep = false;
		}
	}

	return bRep;
}

ERMsg CMADIS::ExportData( short year, short m0, CGeoPointWP pt, const CString& filePath, CSCCallBack& callback )
{
	ASSERT( year>=1961 && year<=2071 );

	ERMsg msg;

	int m1= m0==-1?0:m0;
	int m2= m0==-1?12:m0+1;

	CPoint index = CMADIS::GetColRow(pt);
	
	CStdioFileEx file;
	msg = file.Open(filePath, CFile::modeWrite|CFile::modeCreate|CFile::modeNoTruncate);
	file.SeekToEnd();
	if( file.GetPosition() == 0)
		file.WriteString("Year,Month,Day,Tmin(°K),Tmax(°K),Prcp(mm/s),SpeH(kg/kg),WindS(m/s),Tmin(°C),Tmax(°C),Prcp(mm),Tdew(°C),RelH(%),WndS(km/h)\n");
	
	if(!msg || index.x==-1 || index.y==-1)
		return msg;

	
	
	callback.SetCurrentDescription("Create normals");
	callback.SetCurrentStepRange(0, 12, 1);
	callback.SetStartNewStep();

	//take all cell
	CRect rect(index.x,index.y,index.x+1,index.y+1);
	

	//CDailyStation station;
	//CYearPackage package;
	//
	//CString name;
	//name.Format("%03d-%03d",x+1,y+1);
	//station.SetName(name);
	//station.SetID("MRCC " + name);
	//station.SetLat(lat[y][x]);
	//station.SetLon(lon[y][x]-360);
	//station.SetElev(short(elev[y][x]*scaleFactor+offset));

//	for(int yr=0; yr<nbYear&&msg; yr++)
//	{
//		CDailyData dailyData;
//
//		short year=firstccYear+yr;

	for(int m=m1; m<m2&&msg; m++)
	{
		int nbDays = CFL::GetNbDayPerMonth(m);
		COneMonthGrid Tmin(boost::extents[nbDays][1][1]);
		COneMonthGrid Tmax(boost::extents[nbDays][1][1]);
		COneMonthGrid Prcp(boost::extents[nbDays][1][1]);
		COneMonthGrid Relh(boost::extents[nbDays][1][1]);
		COneMonthGrid Wnds(boost::extents[nbDays][1][1]);
		msg += ReadData( TMIN, year, m, rect, Tmin);
		msg += ReadData( TMAX, year, m, rect, Tmax);
		msg += ReadData( PRCP, year, m, rect, Prcp);
		msg += ReadData( RELH, year, m, rect, Relh);
		msg += ReadData( WNDS, year, m, rect, Wnds);
		
		for(int d=0; d<nbDays&&msg; d++)
		{
			double TminC = ConvertData( TMIN, Tmin[d][0][0]);
			double TmaxC = ConvertData( TMAX, Tmax[d][0][0]);
			double Tmean = (TminC+TmaxC)/2;
			double Hr = CFL::Hs2Hr(Tmean, Relh[d][0][0]);
			double Td = CFL::Hr2Td(TminC, TmaxC, Hr);
			double prcp = ConvertData( PRCP, Prcp[d][0][0]);
			double wnds = ConvertData( WNDS, Wnds[d][0][0]);
			
			CString line;
			line.Format("%d,%d,%d,%.1lf,%.1lf,%.6lg,%.6lg,%.6lg,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf\n",year, m+1, d+1,Tmin[d][0][0],Tmax[d][0][0],Prcp[d][0][0],Relh[d][0][0],Wnds[d][0][0],TminC,TmaxC,prcp,Td,Hr,wnds);
			file.WriteString(line);
		}//day
		msg += callback.StepIt();
	}//month

//		CDailyYear dailyYear(year);
//		dailyYear.SetData(dailyData);
//		package.SetYear(dailyYear);
		
//	}//year

	
	return msg;
}

*/