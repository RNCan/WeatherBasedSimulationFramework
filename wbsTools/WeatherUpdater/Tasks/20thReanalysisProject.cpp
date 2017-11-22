#include "StdAfx.h"
#include "20thReanalysisProject.h"
//#include "oldUtilTime.h"

//#include "Projection.h"
#include "UtilWin.h"
#include "CommonRes.h"
//#include "DailyYear.h"
#include "UtilMath.h"
#include "UtilTime.h"

//#include "ShapeFileBase.h"
//#include "DailyStation.h"
#include "NormalFile.h"
#include "DailyFile.h"
//#include "AdvancedNormalStation.h"
#include "NewMat.h"
#include "UtilGDAL.h"

#include "SYShowMessage.h"
//#include "CommonRes.h"
//#include "mappingRes.h"
//#include "ShapefileBase.h" 
#include "Resource.h"
#include <math.h>
#include "2DimArray.h"
#include "GeomaticsTools\Source\GDALBasic.h"

//#include <shlwapi.h>
//#include <atlpath.h>


using namespace CFL;
using namespace UtilWin;
using namespace DAILY_DATA;





C20thReanalysisProject::C20thReanalysisProject(const CString& path)
{
	SetPath(path);
}

C20thReanalysisProject::~C20thReanalysisProject(void)
{
}

static const double rpd = 1.74532925199433E-02; //radians per degree = 

//Normal Gravity
double fgnSMT(double Latitude)
{
// Normal Gravity vs Latitude from Smithsonian Meteorological Tables (page 488)
	double Cos2L = cos(2 * Latitude * rpd);
	double fgnSMT = 9.80616 * (1 - 0.0026373 * Cos2L + 0.0000059 * Cos2L * Cos2L);

	return fgnSMT;
}

//Vertical Variation in Normal Gravity
double fdgdzSMT(double Latitude)
{
// Rate of change of gravity with altitude
// From the Smithsonian Meteorological Tables p.218 Equation 7
// rpd = radians per degree = 1.74532925199433E-02
	double Cos2L = cos(2 * rpd * Latitude);
	double Cos4L = cos(4 * rpd * Latitude);
	double fdgdzSMT = -(3.085462 * pow(10.0,-6) + 2.27 * pow(10.0, -9) * Cos2L - 2 * pow(10.0,-12) * Cos4L);
	return fdgdzSMT;
}

//Ad Hoc Radius
double fRsmt(double Latitude)
{
// Smithsonian Meteorological Tables p.218 Equation 6
// Ginned up earth radius to compensate for centrifugal force
// variation with latitide
// Note that this is not the Earth ellipsoid radius!
	double fRsmt = -2 * fgnSMT(Latitude) / fdgdzSMT(Latitude) / 1000;

	return fRsmt;
}


//Convert Geopotential Height to Geometric Altitude
double fZhtoZgSMT(double Zgh, double Latitude)
{
// Convert geopotential height (Zgh) to geometric altitude (Zg)
// Zgh ......... Geopotential height (km)
// Latitude .... Degrees
// Return ...... Geometric altitude (km)
//
// Verified against Smithsonian Meteorological Tables (page 222-223)
// if 9.80665 set to 9.8 as in tables
	double r = fRsmt(Latitude);                //Ad hoc radius
	double fZhtoZgSMT = r * Zgh / (fgnSMT(Latitude) * r / 9.80665 - Zgh);

	return fZhtoZgSMT;
}

//Convert Geometric Altitude to Geopotential Height
double fZgToZghSMT(double Zg, double Latitude)
{
// Convert geometric altitude to geopotential height
// Zg ........ Geometric altitude (km)
// Latitude .. Degrees
// Return .... Geopotential height
//
// Verified against Smithsonian Meteorological Tables (page 220-221)
// if 9.80665 set to 9.8 as in tables

	double r = fRsmt(Latitude);
	double fZgToZghSMT = r * (fgnSMT(Latitude) / 9.80665) * (Zg / (Zg + r));

	return fZgToZghSMT;
}

ERMsg C20thReanalysisProject::ExtractTopo(const CString& filePath, CSCCallBack& callback)
{
	ERMsg msg;

	

	CString outputFilePath(filePath);
	UtilWin::SetFileExtension( outputFilePath, ".csv");
	
	CString outputFilePathGrid(filePath);
	UtilWin::SetFileExtension( outputFilePathGrid, ".tif");
	
	CString filePathIn;
	if(m_type==_2X2)
		filePathIn.Format("%shgt.2x2.nc", m_path);
	else filePathIn.Format("%shgt.gauss.nc", m_path);

	int sizeX=0;
	int sizeY=0;
	int nbBand=0;
	CGeoRectWP rect;
	float noData=0;
	double cellSizeX=0;
	double cellSizeY=0;

	msg = GetGridInfo(filePathIn, sizeX, sizeY, nbBand, rect, noData, cellSizeX, cellSizeY);
	CMFCGDALDataset dataset;

	rect.m_xMax+=cellSizeX;
	dataset.Create(outputFilePathGrid, sizeX+1, sizeY, rect, "GTiff", GDT_Int16, 1, -9999);
	

	NcFile file(filePathIn);	
	
	int nbDim = file.num_dims();
	int nbAtt = file.num_atts();
	int nbVar = file.num_vars();

	//NcDim* dim0 = file.get_dim(0);
	//NcDim* dim1 = file.get_dim(1);
	//NcDim* dim2 = file.get_dim(2);

	if( !file.is_valid() )
	{
		CString err;
		err.FormatMessage(IDS_CMN_UNABLE_OPEN_READ, filePath);
		msg.ajoute(err);
	}

	callback.SetCurrentDescription("Create Topo");
	callback.SetCurrentStepRange(0, 17*28, 1);
	callback.SetStartNewStep();

	CStdioFileEx fileOut;


	msg += fileOut.Open( outputFilePath, CFile::modeCreate|CFile::modeWrite);
	if(!msg)
		return msg;

	fileOut.WriteString("Name,ID,Latitude,Longitude,Elevation\n");
	
//	NcVar* pVarX = file.get_var("xc");
	//NcVar* pVarY = file.get_var("yc");
//	NcDim* pDim = file.get_dim("lat");
	//int size = pDim->size();
	NcDim* pDimX = file.get_dim("lon");
	NcDim* pDimY = file.get_dim("lat");
	NcDim* pDimTime = file.get_dim("time");
	

		
	NcVar* pVarLat = file.get_var("lat");
	NcVar* pVarLon = file.get_var("lon");
	NcVar* pVarElev = file.get_var("hgt");
	float offset = pVarElev->get_att("add_offset")->as_float(0);
	float scaleFactor = pVarElev->get_att("scale_factor")->as_float(0);
	//int sizeX2 = pDimX->size();
	//int sizeY2 = pDimY->size();
	
	//typedef boost::multi_array<short, 3> CData3Array;
	//typedef boost::multi_array<float, 2> CDataFloatArray;
	vector<float> X(sizeX);
	vector<float> Y(sizeY);
//	CDataFloatArray X(boost::extents[180]);
//	CDataFloatArray Y(boost::extents[172]);
	//CData2Array lat(boost::extents[28][17]);
	//CData2Array lon(boost::extents[28][17]);
	boost::multi_array<short, 2> elev(boost::extents[sizeY][sizeX]);
	
	
//	pVarX->get(&(X[0]), 180);
//	pVarY->get(&(Y[0]), 172);
	pVarLon->get(&(X[0]), sizeX);
	pVarLat->get(&(Y[0]), sizeY);
	pVarElev->get(&(elev[0][0]), 1, sizeY, sizeX);

//	CProjection prj = GetDataGrid().GetPrj();

	CGridLine profile(sizeX+1);
	for(int i=0; i<sizeY; i++)
	{
		for(int jj=0; jj<sizeX; jj++)
		{
			int j=((sizeX+1)/2+jj)%sizeX;
			//int j=jj;
			//elev[y][x] = short(elev[y][x]*scaleFactor+offset);

			//double Lat=0, Lon=0; 
			//VERIFY( prj.InvertTransform(X[x],Y[y], &Lat, &Lon) );
			//
			//double x2=0, y2=0; 
			//VERIFY( prj.Transform(lat[y*180+x],lon[y*180+x], &x2, &y2) );
			//
			CString line;
			
			float lon = X[j];
			if( lon >= 180)
				lon-=360;

			
			double elev2 = -9999;
			if( elev[i][j]!= 32766 && elev[i][j]!=-32767 )
			{
				elev2=elev[i][j]+offset;
				//elev2 = fZhtoZgSMT(elev2/1000, Y[i])*1000;
			}
			
			//<(sizeX/2)?j+sizeX/2:j-sizeX/2
			//profile[j] = elev[i][(j+sizeX/2)%sizeX];
			profile[jj] = float(elev2);
			if( jj==0)
				profile[sizeX] = float(elev2);

			line.Format("%03d-%03d,%03d-%03d,%f,%f,%d\n",i,jj,i,jj,Y[i],lon,int(elev2));

			fileOut.WriteString(line);

			
			//CGeoPoint point(X[x], Y[y], prj.GetPrjType() );
//			int bContinental = (int)maskMap.ReadCell(x, y)==-1;

	//		if( bContinental )
//			if( shapefile.IsInside(point) )
		//	{
			//}
			//else
			//{
				//msg += grid.WriteCell(grid.GetNoData());
			//}
			msg += callback.StepIt();
		}

		dataset.WriteLine(profile);
	}

	file.close();
	fileOut.Close();
	dataset.Close();
	
	return msg;
}

	
ERMsg C20thReanalysisProject::GetGridInfo(const CString& filePath, int& sizeX, int& sizeY, int& nbBand, CGeoRectWP& rect, float& noData, double& cellSizeX, double& cellSizeY)
{
	ERMsg msg;
	
//	CString filePathIn;
	//filePathIn.Format("%scccma_cgcm3_1-20c3m-run1-pr-1961-2000_monthly.nc", m_path);
	NcError error( NcError::verbose_nonfatal );
	NcFile file(filePath);

	if( !file.is_valid() )
	{
		CString err;
		err.FormatMessage(IDS_CMN_UNABLE_OPEN_READ, filePath);
		msg.ajoute(err);
		return msg;
	}

	int nbDim = file.num_dims();
	int nbAtt = file.num_atts();
	int nbVar = file.num_vars();

	//NcDim* dim0 = file.get_dim(0);
	//NcDim* dim1 = file.get_dim(1);
	//NcDim* dim2 = file.get_dim(2);


	//NcVar* pVarX = file.get_var("xc");
	//NcVar* pVarY = file.get_var("yc");
	NcDim* pDimX = file.get_dim("lon");
	NcDim* pDimY = file.get_dim("lat");
	NcDim* pDimTime = file.get_dim("time");
	if( pDimX && pDimY && pDimTime)
	{
		sizeX = pDimX->size();
		sizeY = pDimY->size();
		nbBand = pDimTime->size();


	
		//double offset = pVarElev->get_att("add_offset")->as_float(0);
		//double scaleFactor = pVarElev->get_att("scale_factor")->as_float(0);

		typedef vector<float> CDataArray;
		CDataArray Y(sizeY);
		CDataArray X(sizeX);
		NcVar* pVarX = file.get_var("lon");
		
	
	
		NcVar* pVarY = file.get_var("lat");
	
		pVarX->get(&(X[0]), sizeX);
		pVarY->get(&(Y[0]), sizeY);

		CStatistic Xstat;
		CStatistic Ystat;

		Xstat+=X[0];Xstat+=X[sizeX-1];
		Ystat+=Y[0];Ystat+=Y[sizeY-1];

		cellSizeX = (Xstat[HIGHEST]-Xstat[LOWEST])/(sizeX-1);
		cellSizeY = (Ystat[HIGHEST]-Ystat[LOWEST])/(sizeY-1);

		noData=-999;
		rect.SetPrj( CProjection( CProjection::GEO ) );
		rect.m_xMin = Xstat[LOWEST]-cellSizeX/2-180;
		rect.m_yMin = Ystat[LOWEST]-cellSizeY/2;
		rect.m_xMax = rect.m_xMin+cellSizeX*sizeX;
		rect.m_yMax = rect.m_yMin+cellSizeY*sizeY;
		
	}
	else
	{
		msg.ajoute("Unable to find one of dimention \"lat, lon or time\"");
	}

	file.close();

	return msg;
}
		


ERMsg C20thReanalysisProject::ReadData( CString filePath, CString varName, CData3Array& data)
{
	//typedef boost::multi_array<int, 2> array_type;


	ERMsg msg;
	
	NcError::set_err( NcError::silent_nonfatal );

  
	//CString filePath = m_path + "cccma_cgcm3_1-20c3m-run1-pr-1961-2000_monthly.nc";//GetFilePath(v, year, m);
	NcFile file(filePath);	//current year file

	if( !file.is_valid() )
	{
		CString err;
		err.FormatMessage(IDS_CMN_UNABLE_OPEN_READ, filePath);
		msg.ajoute(err);
		return msg;
	}

	NcVar* pVarData = file.get_var((LPCTSTR)varName);//is the varaible always at ffirst???
	
	//CString varName = pVarData->name();
	
	size_t sizeTime = pVarData->get_dim(0)->size();
	size_t sizeY = pVarData->get_dim(1)->size();
	size_t sizeX = pVarData->get_dim(2)->size();
	float offset = pVarData->get_att("add_offset")->as_float(0);
	float scaleFactor = pVarData->get_att("scale_factor")->as_float(0);
	
	boost::multi_array<short, 3> tmp(boost::extents[sizeTime][sizeY][sizeX]);

	ENSURE( pVarData->num_dims() == 3);
	
	if( pVarData->get(&(tmp[0][0][0]), sizeTime, sizeY, sizeX) )
	{
		//tmp.extens()
		//data.resize(sizeTime);
		//data.resize(boost::extents[sizeTime][sizeY][sizeX]);
		//apply offset and scale factor
		for(size_t i=0; i<tmp.size(); i++)
			for(size_t j=0; j<tmp[i].size(); j++)
				for(size_t k=0; k<tmp[i][j].size(); k++)
					data[i][j][k] = tmp[i][j][k]*scaleFactor+offset;

		file.close();
	}
	else
	{
		msg.ajoute( "Unable to get NetCDFData");
	}

	return msg;
}

ERMsg C20thReanalysisProject::ReadData( NcVar* pVarData, int x, int y, vector<float>& data)
{
	//typedef boost::multi_array<int, 2> array_type;


	ERMsg msg;
	
	
	size_t sizeTime = pVarData->get_dim(0)->size();
//	size_t sizeY = pVarData->get_dim(1)->size();
	//size_t sizeX = pVarData->get_dim(2)->size();
	float offset = pVarData->get_att("add_offset")->as_float(0);
	float scaleFactor = pVarData->get_att("scale_factor")->as_float(0);
	
	//boost::multi_array<short, 3> tmp(boost::extents[sizeTime][sizeY][sizeX]);
	vector <short > tmp(sizeTime);
	ENSURE( pVarData->num_dims() == 3);
	
	pVarData->set_cur(0,y, x);
	if( pVarData->get(&(tmp[0]), sizeTime, 1, 1) )
	{
		//tmp.extens()
		//data.resize(sizeTime);
		//data.resize(boost::extents[sizeTime][sizeY][sizeX]);
		//apply offset and scale factor
		for(size_t i=0; i<data.size(); i++)
			data[i] = tmp[i]*scaleFactor+offset;

		//file.close();
	}
	//else
	//{
		//msg.ajoute( "Unable to get NetCDFData");
	//}

	return msg;
}

ERMsg C20thReanalysisProject::ReadData( CGDALDatasetEx& dataset, int x, int y, vector<float>& data)
{
	ERMsg msg;

	int nbDays = dataset->GetRasterCount();
	data.resize(nbDays);
	for(int jd=0; jd<nbDays; jd++)
	{
		GDALRasterBand* pBand = dataset->GetRasterBand(jd+1);
		pBand->RasterIO(GF_Read,x,y,1, 1, &(data[jd]),1, 1, GDT_Float32,0,0);
	}

	return msg;
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


ERMsg C20thReanalysisProject::ExtractData(const CString& DBFilePath, CSCCallBack& callback)
{
	ERMsg msg;


	CGeoRect2 test;
	test.SetProjectionRef("+proj=latlon");
	

	callback.SetNbTask(138+2);

	//delete old database
	CDailyFile::DeleteDatabase(DBFilePath);


	//open new one
	CDailyFile dailyDB;
	msg += dailyDB.Open(DBFilePath, CDailyFile::modeEdit, callback);
	if(!msg)
		return msg;

	callback.SetCurrentDescription("Open NetCDF file...");
	callback.SetNbStep(138);

	CGDALDatasetEx fileTmin[138];
	//NcVar* pVarTmin[138] = {0};
	CGDALDatasetEx fileTmax[138];
	//NcVar* pVarTmax[138] = {0};
	CGDALDatasetEx filePrcp[138];
	//NcVar* pVarPrcp[138] = {0};
	CGDALDatasetEx fileShum[138];
	//NcVar* pVarShum[138] = {0};
	CGDALDatasetEx fileUwnd[138];
	//NcVar* pVarUwnd[138] = {0};
	CGDALDatasetEx fileVwnd[138];
	//NcVar* pVarVwnd[138] = {0};

//	CData3Array elev(boost::extents[1][sizeY][sizeX]);
	//msg += ReadData( filePathIn, "hgt", elev);
	for(int yr=0; yr<138&&msg; yr++)
	{ 
		int year = 1871+yr;
		CString filePath;
		
		//read data
		filePath = m_path + "GaussTif\\tmin.2m."+ToString(year)+".tif";
		msg += fileTmin[yr].OpenInputImage((LPCTSTR)filePath);

		filePath = m_path + "GaussTif\\tmax.2m."+ToString(year)+".tif";
		msg += fileTmax[yr].OpenInputImage((LPCTSTR)filePath);

		filePath = m_path + "GaussTif\\prate."+ToString(year)+".tif";
		msg += filePrcp[yr].OpenInputImage((LPCTSTR)filePath);

		filePath = m_path + "GaussTif\\shum.2m."+ToString(year)+".tif";
		msg += fileShum[yr].OpenInputImage((LPCTSTR)filePath);

		filePath = m_path + "GaussTif\\uwnd.10m."+ToString(year)+".tif";
		msg += fileUwnd[yr].OpenInputImage((LPCTSTR)filePath);

		filePath = m_path + "GaussTif\\vwnd.10m."+ToString(year)+".tif";
		msg += fileVwnd[yr].OpenInputImage((LPCTSTR)filePath);
		

		msg += callback.StepIt();
	}


	

	//int sizeX=0;
	//int sizeY=0;
	//int nbBand=0;
	//CGeoRectWP rect;
	//float noData=0;
	//double cellSizeX=0;
	//double cellSizeY=0;

	
	//msg += GetGridInfo(filePathIn, sizeX, sizeY, nbBand, rect, noData, cellSizeX, cellSizeY);
	
//	NcFile fileElev(filePathIn);	//current year file
	//ASSERT( fileElev.is_valid() );
	
	CString filePath = m_path + "gaussTif\\hgt.gauss.tif";

	CGDALDatasetEx fileElev;
	msg += fileElev.OpenInputImage((LPCTSTR)filePath);

	if( !msg)
		return msg;

	int sizeX = fileElev->GetRasterXSize();
	int sizeY = fileElev->GetRasterYSize();

	CGeoExtents rect = fileElev.GetExtents();

	//copy data to weather station
	for(int y=0; y<sizeY&&msg; y++)
	{
		for(int x=0; x<sizeX&&msg; x++)
		{
			CYearPackage package;

			//CPoint xy = rect.CoordToXYPos(CGeoPoint2(-71,45));
			//x = xy.x;
			//y = xy.y;

			callback.SetCurrentDescription( "process x=" + ToString(x+1) + ", y="+ToString(y+1) );
			callback.SetNbStep(138);

			//#pragma omp parallel for
			for(int yr=0; yr<138&&msg; yr++)
			{
				int year = 1871 + yr;
				int nbDays = CFL::GetNbDay(year);

				CString filePath;
		
				//read data
				//#pragma omp critical
				vector<float> Tmin(nbDays);
				msg += ReadData( fileTmin[yr], x, y, Tmin);

				vector<float> Tmax(nbDays);
				msg += ReadData( fileTmax[yr] , x, y, Tmax);

				vector<float> prcp(nbDays);
				msg += ReadData( filePrcp[yr], x, y, prcp);

				vector<float> shum(nbDays);
				msg += ReadData( fileShum[yr] , x, y, shum);

				vector<float> uwnd(nbDays);
				msg += ReadData( fileUwnd[yr] , x, y, uwnd);

				vector<float> vwnd(nbDays);
				msg += ReadData( fileVwnd[yr] , x, y, vwnd);

				
				CDailyData dailyData;
				
				for(int jd=0; jd<nbDays&&msg; jd++)
				{
					double TminC = ConvertData( TMIN, Tmin[jd]);
					double TmaxC = ConvertData( TMAX, Tmax[jd]);
					double Tmean = (TminC+TmaxC)/2;
					double ppt = ConvertData( PRCP, prcp[jd]);
					if( ppt < 0.1) ppt=0;
					double Hr = CFL::Hs2Hr(Tmean, shum[jd]);
					double Td = CFL::RH2Td(TminC, TmaxC, Hr);
					double wspd = ConvertData( WNDS, sqrt((double)Square(uwnd[jd]) + Square(vwnd[jd])) );
							
					dailyData[jd][TMIN] = (float)TminC;
					dailyData[jd][TMAX] = (float)TmaxC;
					dailyData[jd][PRCP] = (float)ppt;
					dailyData[jd][RELH] = (float)Hr;
					dailyData[jd][TDEW] = (float)Td;
					dailyData[jd][WNDS] = (float)wspd;
				}//day

				CDailyYear dailyYear(year);
				dailyYear.SetData(dailyData);
				package.SetYear(dailyYear);

				msg += callback.StepIt();
			}//year

			vector<float> elev(1);
			msg += ReadData( fileElev, x, y, elev);

			
			CGeoPoint2 pt = rect.XYPosToCoord(CPoint2(x,y));

			CDailyStation station;

			CString name;
			name.Format("%03d-%03d",x+1,y+1);
			station.SetName(name);
			station.SetID("20th " + name);
			station.SetLat(pt.m_y);
			station.SetLon(pt.m_x);
			station.SetElev(int(elev[0]));

			station.AddPakage(package);

			//CAdvancedNormalStation normalsStations;
			//msg+=normalsStations.FromDaily(station, 1);
			msg+=dailyDB.Add(station);

		}//x
	}//y


	dailyDB.Close();

	for(int yr=0; yr<138&&msg; yr++)
	{
		fileTmin[yr].Close();
		fileTmax[yr].Close();
		filePrcp[yr].Close();
		fileShum[yr].Close();
		fileUwnd[yr].Close();
		fileVwnd[yr].Close();
	}
	
	return msg;
	
}

ERMsg C20thReanalysisProject::CreateDailyGrid( CString filePathIn, CString varName, const CString& filePathOut, CSCCallBack& callback)
{
	ERMsg msg;


	int sizeX=0;
	int sizeY=0;
	int nbBand=0;
	CGeoRectWP rect;
	float noData=0;
	double cellSizeX=0;
	double cellSizeY=0;
	//CString filePathIn = m_path + "gauss\\hgt.gauss.nc";
	CString filePath = filePathIn+".1871.nc";
	msg = GetGridInfo(filePath, sizeX, sizeY, nbBand, rect, noData, cellSizeX, cellSizeY);
	if( !msg)
		return msg;

	int nbDayTotal=0;
	for(int yr=0; yr<138&&msg; yr++)
		nbDayTotal+=CFL::GetNbDay(1871+yr);

//	if( !msg)
	//	return msg;

//	callback.SetCurrentDescription("Init "+GetFileTitle(filePathOut));
	//callback.SetCurrentStepRange(0, sizeX*sizeY*138, 1);
	

	//init with no data
	//for(int z=0; z<138&&msg; z++)
	//{
	//	for(int x=0; x<sizeX&&msg; x++)
	//	{
	//		for(int y=0; y<sizeY&&msg; y++)
	//		{
	//			msg += grid.WriteCell(noData,x,y,z);
	//			msg += callback.StepIt();
	//		}
	//	}
	//}

	//take all cell
	//CRect rect(0,0,180,172);


	callback.SetCurrentDescription("Create "+GetFileTitle(filePathOut));
	callback.SetCurrentStepRange(0, nbDayTotal, 1);
	callback.SetStartNewStep();

	int jdy=0;
	for(int yr=0; yr<1/*38*/&&msg; yr++)
	{
		int year = 1871 + yr;
		int nbDays = CFL::GetNbDay(year);

		//CMapBilFile grid;
		//grid.SetCellType(CMapBilFile::FLOAT);
		//grid.SetProjection( CProjection(CProjection::GEO) );//GetDataGrid().GetPrj() );
		//grid.SetNbCols(sizeX);
		//grid.SetNbRows(sizeY);
		//grid.SetNbBand(nbDays);//40 year * ~365days
		//grid.SetBoundingBox(rect);
		//grid.SetNoData(noData);
		//grid.SetCellSizeX( cellSizeX );
		//grid.SetCellSizeY( cellSizeY );

		CString filePathOut = filePathIn+"."+ToString(year)+".tif";
		CStdStringVector createOptions;
		createOptions.push_back("COMPRESS=LZW");

		
		CGeoRect2 boundingBox;
		boundingBox.m_xMin = rect.m_xMin; 
		boundingBox.m_yMin =  rect.m_yMin;
		boundingBox.m_xMax = rect.m_xMax+cellSizeX; //add one cell
		boundingBox.m_yMax =  rect.m_yMax;
		boundingBox.SetProjectionRef("+proj=latlon +datum=NAD83");
		

		CGDALDatasetEx dataset;
		
		msg += dataset.CreateImage((LPCTSTR)filePathOut, sizeX+1, sizeY, boundingBox, "GTIFF", GDT_Float32, nbDays, -9999, createOptions, true);

		
		
		//read data
		CData3Array var(boost::extents[nbDays][sizeY][sizeX+1]);
		CString filePath = filePathIn+"."+ToString(year)+".nc";
		msg += ReadData( filePath, varName, var);

		//copy data to weather station
		for(int jd=0; jd<nbDays; jd++)
		{
			//boost::multi_array<float, 2>  varXY(boost::extents[sizeX+1][sizeY]);
			for(int y=0; y<sizeY; y++)
			{
				for(int xx=0; xx<(sizeX+1)/2; xx++)
				{
					//work only if the number of cell is even
					int x=((sizeX+1)/2+xx)%sizeX;
					
					//varXY[xx][y] = var[jd][y][x];
					float a = var[jd][y][xx];
					var[jd][y][xx] = var[jd][y][x];
					var[jd][y][x]=a;
				}
				//the last cell is a copy of the first cell
				var[jd][y][sizeX]=var[jd][y][0];
				//varXY[sizeX][y]=varXY[0][y];
			}

			GDALRasterBand* pBand = dataset->GetRasterBand(jd+1);
			pBand->RasterIO(GF_Write,0,0,sizeX+1, sizeY, &(var[jd][0][0]),sizeX+1, sizeY, GDT_Float32,0,0);//, 1, NULL,0,0,0);
			msg += callback.StepIt();
		}
		
		
		
		//dataset->GDALDataset::RasterIO(GF_Write,0,0,sizeX, sizeY, &(var[0][0][0]),sizeX, sizeY, GDT_Float32, nbDays, NULL,0,0,0);

			

		////copy data to weather station
		//for(int y=0; y<sizeY&&msg; y++)
		//{
		//	for(int xx=0; xx<sizeX&&msg; xx++)
		//	{
		//		int x=((sizeX+1)/2+xx)%sizeX;
		//		for(int jd=0; jd<nbDays&&msg; jd++)
		//		{
		//			//float value = (float)MMG[x][sizeY-y-1][z];
		//			//int bandNo = bandBase+z;
		//			msg += grid.WriteCell(var[jd][x][y],xx,y,jdy);
		//			jdy++;
		//		}
		//		
		//		msg += callback.StepIt();
		//	}
		//}
		//	
		dataset.Close();
	}




	return msg;
}

/*
ERMsg C20thReanalysisProject::ExtractData(const CString& filePath, CSCCallBack& callback)
{
	ERMsg msg;

	//delete old database
	CDailyFile::DeleteDatabase(filePath);

	//open new one
	CDailyFile dailyDB;
	msg += dailyDB.Open(filePath, CDailyFile::modeEdit);
	if(!msg)
		return msg;

	CString filePathIn = m_path + "gauss\\hgt.gauss.nc";
	//if(m_type==_2X2)
		//filePathIn.Format("%shgt.2x2.nc", m_path);
	//else 
	//filePathIn.Format("%shgt.gauss.nc", m_path);

	int sizeX=0;
	int sizeY=0;
	int nbBand=0;
	CGeoRectWP rect;
	float noData=0;
	double cellSizeX=0;
	double cellSizeY=0;

	msg += GetGridInfo(filePathIn, sizeX, sizeY, nbBand, rect, noData, cellSizeX, cellSizeY);
	CData3Array elev(boost::extents[1][sizeY][sizeX]);
	msg += ReadData( filePathIn, "hgt", elev);

	if( !msg)
		return msg;

	callback.SetNbStep(138*sizeX*sizeY);
//	C2DimArray<CDailyStation> stations;
	//stations.SetSize(sizeX, sizeY);
	
	C2DimArray<CYearPackage> packages;
	packages.SetSize(sizeX, sizeY);


	for(int yr=0; yr<138&&msg; yr++)
	{
		int year = 1871 + yr;
		int nbDays = CFL::GetNbDay(year);

		CString filePath;
		
		//read data
		CData3Array Tmin(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\tmin.2m."+ToString(year)+".nc";
		msg += ReadData( filePath, "tmin", Tmin);

		CData3Array Tmax(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\tmax.2m."+ToString(year)+".nc";
		msg += ReadData( filePath, "tmax", Tmax);

		CData3Array prcp(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\prate."+ToString(year)+".nc";
		msg += ReadData( filePath, "prate", prcp);

		CData3Array shum(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\shum.2m."+ToString(year)+".nc";
		msg += ReadData( filePath, "shum", shum);

		CData3Array uwnd(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\uwnd.10m."+ToString(year)+".nc";
		msg += ReadData( filePath, "uwnd", uwnd);

		CData3Array vwnd(boost::extents[nbDays][sizeY][sizeX]);
		filePath = m_path + "Gauss\\vwnd.10m."+ToString(year)+".nc";
		msg += ReadData( filePath, "vwnd", vwnd);

		//copy data to weather station
		for(int y=0; y<sizeY&&msg; y++)
		{
			for(int xx=0; xx<sizeX&&msg; xx++)
			{
				int x=((sizeX+1)/2+xx)%sizeX;

				CDailyData dailyData;

				
				for(int jd=0; jd<nbDays&&msg; jd++)
				{
					double TminC = ConvertData( TMIN, Tmin[jd][y][x]);
					double TmaxC = ConvertData( TMAX, Tmax[jd][y][x]);
					double Tmean = (TminC+TmaxC)/2;
					double ppt = ConvertData( PRCP, prcp[jd][y][x]);
					double Hr = CFL::Hs2Hr(Tmean, shum[jd][y][x]);
					double Td = CFL::RH2Td(TminC, TmaxC, Hr);
					double wspd = ConvertData( WNDS, sqrt((double)Square(uwnd[jd][y][x]) + Square(vwnd[jd][y][x])) );
							
					dailyData[jd][TMIN] = (float)TminC;
					dailyData[jd][TMAX] = (float)TmaxC;
					dailyData[jd][PRCP] = (float)ppt;
					dailyData[jd][RELH] = (float)Hr;
					dailyData[jd][TDEW] = (float)Td;
					dailyData[jd][WNDS] = (float)wspd;
				}//day

				CDailyYear dailyYear(year);
				dailyYear.SetData(dailyData);
				packages[x][y].SetYear(dailyYear);

				msg += callback.StepIt();
			}//year
		}//x
	}//y

	//write weather station
	for(int y=0; y<sizeY&&msg; y++)
	{
		for(int x=0; x<sizeX&&msg; x++)
		{
			
			CDailyStation station;

			CString name;
			name.Format("%03d-%03d",x+1,y+1);
			station.SetName(name);
			station.SetID("20th " + name);
			station.SetLat(rect.m_yMax-(y+0.5)*cellSizeY);
			station.SetLon(rect.m_xMin+(x+0.5)*cellSizeX);
			station.SetElev(int(elev[0][y][x]));

			station.AddPakage(packages[x][y]);

			//CAdvancedNormalStation normalsStations;
			//msg+=normalsStations.FromDaily(station, 1);
			msg+=dailyDB.Add(station);
		}//x
	}//y

	return msg;
	
}
*/

//*******************************************************************************************
const char* C20thRPTask::ATTRIBUTE_NAME[] = { "WORKING_DIR", "NORMAL_FILEPATH", "OUTPUT_FILEPATH", "TYPE"};
const char* C20thRPTask::CLASS_NAME = "20th_Reanalysis_Project";

C20thRPTask::C20thRPTask(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

}

void C20thRPTask::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_20THRP );

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);
	
	CStringArrayEx title(IDS_PROPERTIES_20THRP);
	ASSERT( title.GetSize() == NB_ATTRIBUTE);

	CString filter1 = "*.DailyStations|*.DailyStations||";
	//CString filter2 = GetString( IDS_MAP_FILTER_SHAPEFILE);
	GetParameters().Add( CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], title[0], "NetCDF files (*.nc)|*.nc" ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], title[1], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], title[2], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[3], title[3], CStringArrayEx("2x2;gauss;"), "0" ) );
	
}

C20thRPTask::~C20thRPTask(void)
{
}


C20thRPTask::C20thRPTask(const C20thRPTask& in)
{
	operator=(in);
}

void C20thRPTask::Reset()
{
	m_path.Empty();
	m_inputNormalFilePath.Empty();
	m_outputFilePath.Empty();
	m_type=C20thReanalysisProject::_2X2;
}

C20thRPTask& C20thRPTask::operator =(const C20thRPTask& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_path = in.m_path;
		m_inputNormalFilePath = in.m_inputNormalFilePath;
		m_outputFilePath = in.m_outputFilePath;
		m_type=in.m_type;
	}

	return *this;
}

bool C20thRPTask::operator ==(const C20thRPTask& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_path != in.m_path)bEqual = false;
	if( m_inputNormalFilePath != in.m_inputNormalFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_type!=in.m_type)bEqual = false;

	return bEqual;
}

bool C20thRPTask::operator !=(const C20thRPTask& in)const
{
	return !operator ==(in);
}


CString C20thRPTask::GetValue(short type)const
{
	CString str;
	

	ASSERT( NB_ATTRIBUTE == 4); 
	switch(type)
	{
	case I_WORKING_DIR: str = m_path;break;
	case I_NORMAL_DB: str = m_inputNormalFilePath;break;
	case I_OUT_FILEPATH: str = m_outputFilePath; break;
	case I_TYPE: str = ToString(m_type); break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void C20thRPTask::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 4); 
	switch(type)
	{
	case I_WORKING_DIR: m_path = str;break;
	case I_NORMAL_DB: m_inputNormalFilePath = str;break;
	case I_OUT_FILEPATH: m_outputFilePath=str; break;
	case I_TYPE: m_type= ToInt(str); break;
	default: CToolsBase::SetValue(type, str); break;
	}

}


bool C20thRPTask::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const C20thRPTask& info = dynamic_cast<const C20thRPTask&>(in);
	return operator==(info);
}

CParameterBase& C20thRPTask::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const C20thRPTask& info = dynamic_cast<const C20thRPTask&>(in);
	return operator=(info);
}

ERMsg C20thRPTask::Execute(CSCCallBack& callback)
{
	ERMsg msg;

	GDALAllRegister();
	C20thReanalysisProject project(m_path);
	project.m_type=m_type;
	
	//msg = project.ExtractTopo(m_outputFilePath, callback);
	msg = project.ExtractData(m_outputFilePath, callback);

	//callback.SetNbTask(6);
	//char * varName[6] = {"tmin","tmax","prate","shum","vwnd","uwnd"};
	//char * varExt[6] = {".2m",".2m","",".2m",".10m",".10m"};
	//for(int v=0; v<6; v++)
	//{
	//	CString filePathIn = m_path+"gauss\\"+varName[v]+varExt[v];
	//	CString filePathOut = m_path+varName[v]+".bil";

	//	project.CreateDailyGrid( filePathIn, varName[v], filePathOut, callback);
	//}

	


	return msg;
}

