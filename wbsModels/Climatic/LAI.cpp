//**********************************************************************
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
// Create from :
//Leaf Area Index Review and Determination for the Greater Athabasca
//Oil Sands Region of  Northern Alberta, Canada
//February 19, 2009
//**********************************************************************

#include <iostream>
#include <random>
#include <boost/math/distributions/normal.hpp>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "LAI.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	namespace LeafAreaIndex
	{
		const CLAIProfile CLeafAreaIndex::LAI_PROFILE[NB_FOREST_COVER_TYPE * 12]
		{
			//		Forest Cover type,DOY, Mean LAI,Standard Deviation,Median LAI
			{HARDWOODS,17,0.22485,0.11946,0.2},
			{HARDWOODS,41,0.51284,0.47425,0.4},
			{HARDWOODS,73,0.47032,0.29487,0.5},
			{HARDWOODS,105,0.97302,0.2433,1},
			{HARDWOODS,137,1.64062,0.702,1.6},
			{HARDWOODS,169,3.42824,1.45429,3.2},
			{HARDWOODS,193,3.40666,1.60837,3.4},
			{HARDWOODS,225,0.9625,1.2018,0.5},
			{HARDWOODS,257,1.20393,0.3522,1.2},
			{HARDWOODS,289,0.7733,0.2118,0.8},
			{HARDWOODS,321,0.71982,0.36982,0.7},
			{HARDWOODS,353,0.57481,0.4354,0.5},
			{MIXED_WOODS,17,0.20587,0.11797,0.2},
			{MIXED_WOODS,41,0.56,0.60426,0.4},
			{MIXED_WOODS,73,0.46635,0.3216,0.4},
			{MIXED_WOODS,105,0.96867,0.26006,1},
			{MIXED_WOODS,137,1.35531,0.62001,1.3},
			{MIXED_WOODS,169,2.95821,1.26207,2.6},
			{MIXED_WOODS,193,3.00894,1.47936,2.8},
			{MIXED_WOODS,225,1.0239,1.20943,0.6},
			{MIXED_WOODS,257,1.16683,0.29716,1.2},
			{MIXED_WOODS,289,0.78678,0.24263,0.8},
			{MIXED_WOODS,321,0.86763,0.40657,0.8},
			{MIXED_WOODS,353,0.62685,0.57399,0.5},
			{WHITE_SPRUCE,17,0.18077,0.101,0.2},
			{WHITE_SPRUCE,41,0.58028,0.64155,0.5},
			{WHITE_SPRUCE,73,0.49761,0.31235,0.5},
			{WHITE_SPRUCE,105,0.96268,0.23638,1},
			{WHITE_SPRUCE,137,1.41912,0.47732,1.4},
			{WHITE_SPRUCE,169,2.58387,1.08702,2.3},
			{WHITE_SPRUCE,193,2.45464,1.28739,2.3},
			{WHITE_SPRUCE,225,0.96632,1.03962,0.6},
			{WHITE_SPRUCE,257,1.09435,0.25802,1.1},
			{WHITE_SPRUCE,289,0.75497,0.17561,0.8},
			{WHITE_SPRUCE,321,0.92689,0.3929,0.9},
			{WHITE_SPRUCE,353,0.73341,0.86881,0.5},
			{BLACK_SPRUCE_FORESTS,17,0.20055,0.11795,0.2},
			{BLACK_SPRUCE_FORESTS,41,0.4675,0.51939,0.4},
			{BLACK_SPRUCE_FORESTS,73,0.48477,0.31559,0.5},
			{BLACK_SPRUCE_FORESTS,105,1.09063,0.22045,1.1},
			{BLACK_SPRUCE_FORESTS,137,1.43064,0.44081,1.4},
			{BLACK_SPRUCE_FORESTS,169,2.48564,0.86004,2.3},
			{BLACK_SPRUCE_FORESTS,193,2.65199,1.04122,2.6},
			{BLACK_SPRUCE_FORESTS,225,0.56682,0.76469,0.3},
			{BLACK_SPRUCE_FORESTS,257,1.12821,0.22186,1.1},
			{BLACK_SPRUCE_FORESTS,289,0.82041,0.18462,0.8},
			{BLACK_SPRUCE_FORESTS,321,0.81151,0.45254,0.8},
			{BLACK_SPRUCE_FORESTS,353,0.7044,0.51452,0.6},
			{BLACK_SPRUCE_FENS,17,0.15218,0.10798,0.1},
			{BLACK_SPRUCE_FENS,41,0.41645,0.51501,0.3},
			{BLACK_SPRUCE_FENS,73,0.35447,0.27576,0.3},
			{BLACK_SPRUCE_FENS,105,1.03958,0.28633,1.1},
			{BLACK_SPRUCE_FENS,137,1.338,0.45017,1.3},
			{BLACK_SPRUCE_FENS,169,2.21388,0.75267,2.1},
			{BLACK_SPRUCE_FENS,193,2.32606,1.05589,2.3},
			{BLACK_SPRUCE_FENS,225,0.86316,0.93794,0.5},
			{BLACK_SPRUCE_FENS,257,1.13286,0.22643,1.1},
			{BLACK_SPRUCE_FENS,289,0.87824,0.20659,0.9},
			{BLACK_SPRUCE_FENS,321,0.97094,0.4141,1},
			{BLACK_SPRUCE_FENS,353,0.6162,0.55285,0.5},
			{JACK_PINE_FORESTS,17,0.18782,0.11634,0.2},
			{JACK_PINE_FORESTS,41,0.54375,0.62648,0.4},
			{JACK_PINE_FORESTS,73,0.33452,0.30471,0.2},
			{JACK_PINE_FORESTS,105,0.91404,0.28332,0.9},
			{JACK_PINE_FORESTS,137,1.06669,0.44384,1.1},
			{JACK_PINE_FORESTS,169,1.89249,0.7857,1.8},
			{JACK_PINE_FORESTS,193,1.98983,1.01162,1.8},
			{JACK_PINE_FORESTS,225,0.69143,0.72613,0.5},
			{JACK_PINE_FORESTS,257,1.06735,0.25467,1.1},
			{JACK_PINE_FORESTS,289,0.90988,0.29623,0.9},
			{JACK_PINE_FORESTS,321,1.02021,0.49199,1},
			{JACK_PINE_FORESTS,353,0.55455,0.47353,0.5}
		};

		//doy_in: day of year (zero based)
		double CLeafAreaIndex::ComputeLAI(size_t doy_in, size_t forestCoverType, double q)
		{
			ASSERT(doy_in < 366);

			int doy = (int)doy_in;
			if (doy >= 353)
				doy -= 365;

			static const int DOY[13] = { -12, 17, 41, 73, 105,137,169,193,225,257,289,321,353};
			//linear interpolation between month
			int i1= -999;
			int i2 = -999;
			for (int i = 0; i < 13&&i1==-999; i++)
			{
				if (doy < DOY[i])
				{
					i1 = i-1;
					i2 = i;
				}
			}

			ASSERT(i1 >= 0 && i1 <= 12);
			ASSERT(i2 >= 0 && i2 <= 12);

			const CLAIProfile& p1 = LAI_PROFILE[GetProfileIndex(forestCoverType, (i1+11)%12)];
			const CLAIProfile& p2 = LAI_PROFILE[GetProfileIndex(forestCoverType, (i2+11)%12)];

			double f2 = ((double)doy - DOY[i1]) / ((double)DOY[i2] - DOY[i1]);
			double f1 = 1.0 - f2;
			ASSERT(f1 >= 0 && f1 <= 1);
			ASSERT(f2 >= 0 && f2 <= 1);

			boost::math::normal uniformNormal1(p1.m_LAI_Mean, p1.m_LAI_SD);
			boost::math::normal uniformNormal2(p2.m_LAI_Mean, p2.m_LAI_SD);

			// % of distribution is below q:
			double LAI1 = quantile(uniformNormal1, q);
			double LAI2 = quantile(uniformNormal2, q);
			double LAI = f1 * LAI1 + f2 * LAI2;

			return LAI;

		}

	}


}