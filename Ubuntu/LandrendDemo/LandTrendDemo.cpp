//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************

#include <iostream>
#include <fstream>
#include <iomanip>
#include "LandTrendCore.h"



using namespace std;
using namespace LTR;


//***********************************************************************
//
//	Main
//
//***********************************************************************
//
int main(int argc, char* argv[])
{

	CRealArray x = { 1984, 1985,1986,1987,1988,1989,1990,1991,1992,1993,1994,1995,1996,1997,1998,1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015,2016,2017,2018,2019,2020,2021 };
	CRealArray y = { 672.987,699.626,719.188,689.667,647.972,704.698,735.36,733.929,718.447,715.716,735.475,673.822,738.697,758.855,730.845,708.566,713.21,724.845,718.656,723.697,724.386,747.256,715.891,771.447,735.972,743.666,749.56,694.596,757.707,759.097,772.607,620.987,602.313,634.684,635.128,624.309,659.521,701.292 };
	CRealArray yfit = { 697.699,699.566,701.432,703.299,705.165,707.031,708.898,710.764,712.631,714.497,716.364,718.23,720.096,721.963,723.829,725.696,727.562,729.428,731.295,733.161,735.028,736.894,738.761,740.627,742.493,744.36,746.226,748.093,749.959,751.826,753.692,620.987,629.107,637.227,645.347,653.468,661.588,669.708 };

	CBoolArray goods(true, x.size());
//	goods[0] = false;
//	goods[1] = false;
//	goods[2] = false;
//	goods[3] = false;
//	goods[12] = false;
//	goods[13] = false;
//	goods[14] = false;
//	goods[goods.size() - 1] = false;
//	goods[goods.size() - 2] = false;
//	goods[goods.size() - 3] = false;


	REAL_TYPE pval = 0.05;
	REAL_TYPE recovery_threshold = 0.25;
	REAL_TYPE distweightfactor = 2;
	size_t vertexcountovershoot = 3;
	REAL_TYPE bestmodelproportion = 0.75;
	size_t minneeded = 6;
	size_t max_segments = 6;
	int background = 0;
	int modifier = -1;//-1 for NBR
	REAL_TYPE  desawtooth_val = 0.9;
	TFitMethod fit_method = FIT_EARLY_TO_LATE;

	CBestModelInfo result = fit_trajectory_v2(x, y, goods,
		minneeded, background, modifier,
		desawtooth_val, pval, max_segments, recovery_threshold,
		distweightfactor, vertexcountovershoot, bestmodelproportion, fit_method);

    assert(result.ok);
	bool bEqal = true;
	for (size_t i = 0; i < yfit.size(); i++)
		bEqal &= fabs(result.yfit[i] - yfit[i]) < 0.001;

	//assert(bEqal);
	if(bEqal)
    {
        cout << "Test work correctly"<<endl;
    }
    else
    {
        cout << "Error. Test was not past correctly"<<endl;
    }


//	ofstream file;
//	file.open("/home/remi/Project/LandTrendDemo/test_landTrend.csv");
//	file << "x,y,obs_yfit,my_yfit" << endl;
//
//	for (size_t i = 0; i < result.yfit.size(); i++)
//
//	{
//		file << x[i] << "," << std::setprecision (3) << std::fixed << y[i] << "," << yfit[i] << "," << result.yfit[i] << endl;
//	}
//
//	file.close();



	return 0;
}
