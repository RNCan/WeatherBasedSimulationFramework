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
#include <iostream>
#include <cassert>
#include <istream>
#include <fstream>
#include <float.h>

#define _USE_MATH_DEFINES
#include <math.h>


#include "Basic/UtilStd.h"
#include "Basic/Statistic.h"
#include "Basic/OpenMP.h"
#include "hxGrid/interface/IAgent.h"
#include "Newmat/Regression.h"
#include "Geomatic/Variogram.h" 

#define BOOST_NONCOPYABLE_HPP_INCLUDED
#include "dlib/svm.h"



namespace WBSF
{

static const double EPSILON = 0.000000000000001;
//static const double EPSILON = 0.0001;
// The svm functions use column vectors to contain a lot of the data on which they 
// operate. So the first thing we do here is declare a convenient typedef.  

// This typedef declares a matrix with 2 rows and 1 column.  It will be the
// object that contains each of our 2 dimensional samples.   
//typedef dlib::matrix<double, 2, 1> sample_type;



class cross_validation_objective
{
	/*!
	WHAT THIS OBJECT REPRESENTS
	This object is a simple function object that takes a set of model
	parameters and returns a number indicating how "good" they are.  It
	does this by performing 10 fold cross validation on our dataset
	and reporting the accuracy.
	!*/
public:

	cross_validation_objective(int model, const CVariogramPredictorVector& samples) : 
		m_model(model),
		m_samples(samples)
	{}

	double TSS()const
	{
		CStatistic stat;

		for (CVariogramPredictorVector::const_iterator it = m_samples.begin(); it != m_samples.end(); it++)
			stat += it->m_lagVariance;

		return stat[WBSF::TSS];
	}
		
	double calc(const dlib::matrix<double>& params, double xx)const
	{
		


		double nugget = params(0);
		double sill = params(1);
		double range = params(2);

		double yh = 0;
		switch (m_model)
		{
		case CVariogram::SPERICAL:
		{
			if (range == 0)
				range = EPSILON;

			if (xx <= range)
				yh = nugget + sill * (1.5*xx / range - 0.5*pow(xx / range, 3));
			else
				yh = nugget + sill;

			break;
		}
		
		case  CVariogram::EXPONENTIAL:
		{
			if (range == 0)
				range = EPSILON;

			//double tmp = ;
			//if (tmp < 99)
			yh = nugget + sill*(1.0 - exp(-xx / range));

			break;
		}

		case CVariogram::GAUSSIAN:
		{
			if (range == 0)
				range = EPSILON;

			yh = nugget + sill*(1.0 - exp(-Square(xx / range)));
			break;
		}
		case CVariogram::POWER:
		{
			yh = nugget + sill*pow(xx, range);
			break;
		}
		case CVariogram::CUBIC:
		{
			if (range == 0)
				range = EPSILON;

			if (xx <= range)
				yh = nugget + sill * (7 * pow(xx / range, 2) - 35 / 4 * pow(xx / range, 3) + 7 / 2 * pow(xx / range, 5) - 3 / 4 * pow(xx / range, 7));
			else
				yh = nugget + sill;

			break;
		}
		case CVariogram::PENTASPHERICAL:
		{
			if (range == 0)
				range = EPSILON;

			if (xx <= range)
				yh = nugget + sill * (15 / 8 * pow(xx / range, 1) - 5 / 4 * pow(xx / range, 3) + 3 / 8 * pow(xx / range, 5));
			else
				yh = nugget + sill;

			break;
		}
		case CVariogram::SINE_HOLE_EFFECT:
		{
			if (range == 0)
				range = EPSILON;

			if (xx <= range)
				yh = nugget + sill * (1.0 - sin(PI*xx / range) / (PI*xx / range));

			break;
		}
		default: assert(false);
		}

		return yh;
	}

	double RSS(const dlib::matrix<double>& params, bool bWeighted)const
	{
		
		double f = 0;
		

		for (CVariogramPredictorVector::const_iterator it = m_samples.begin(); it != m_samples.end(); it++)
		{
			double xx = it->m_lagDistance;
			double yy = it->m_lagVariance;
			double nn = it->m_np;

			double yh = calc(params, xx);

			double 	weight = 1;
			if (bWeighted && (yy > 0) )
				weight = nn / Square(yy);

			f += Square(yh - yy)*weight;

		}

		if (!isfinite(f))
			f = 99999999;

		return f;
	}

	


	double operator() (const dlib::matrix<double>& params) const
	{
		return RSS(params, true);
	}

	void ComputeParams(dlib::matrix<double>& params, dlib::matrix<double>& lower_bound, dlib::matrix<double>& upper_bound)const
	{
	
		//guess at the sill
		double sill = 0;
		double ma = 0;

		int last = (int)(m_samples.size()*0.75 + 0.5);
		for (size_t i = last; i < m_samples.size(); i++)
		{
			sill += m_samples[i].m_lagVariance;
			ma += m_samples[i].m_lagDistance;
		}
		sill /= (double)(m_samples.size() - last + 1);
		ma /= (double)(m_samples.size() - last + 1);


		//guess at nugget
		double nugget = min(sill, (m_samples[0].m_lagVariance + m_samples[1].m_lagVariance) / 2.);

		//guess at the range
		double range08 = 5 * (nugget + 0.9*(sill - nugget));
		double range = m_samples[int(m_samples.size() / 2)].m_lagDistance;//take middle lag distance

		for (size_t i = 2; i < m_samples.size() - 2; i++)
		{
			double ra = 0;
			for (int j = -2; j <= 2; j++)
				ra += m_samples[i + j].m_lagVariance;

			if (ra > range08)
			{
				range = m_samples[i].m_lagDistance;
				break;
			}
		}
		ASSERT(range > 0);
		ASSERT(sill-nugget >= 0);

		
		switch (m_model)
		{
		case CVariogram::SPERICAL:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range;
			break;
		}
		case  CVariogram::EXPONENTIAL:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range / 3;
			break;
		}
		case CVariogram::GAUSSIAN:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range / sqrt(3.0);
			
			break;
		}
		case CVariogram::POWER:
		{
			params(0) = nugget;
			params(1) = (sill - nugget) / pow(ma, 0.25);
			params(2) = 0.25;
			break;
		}
		case CVariogram::CUBIC:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range;
			break;
		}
		case CVariogram::PENTASPHERICAL:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range;
			break;
		}
		case CVariogram::SINE_HOLE_EFFECT:
		{
			params(0) = nugget;
			params(1) = sill - nugget;
			params(2) = range;
			break;
		}
		default: ASSERT(false);
		}//switch model

		//static const double LO_L = 1.0E-15;
		//static const double HI_L = 1.0E+15;//10000000.0

		CStatistic statV;
		CStatistic statD;
		for (CVariogramPredictorVector::const_iterator it = m_samples.begin(); it != m_samples.end(); it++)
		{
			//ASSERT(it->m_lagVariance >= LO_L && it->m_lagVariance<=HI_L);
			//ASSERT(it->m_lagDistance >= LO_L && it->m_lagDistance <= HI_L);

			statV += it->m_lagVariance;
			statD += it->m_lagDistance;
		}
		
		
		//lower_bound = max(LO_L, statV[LOWEST]), max(LO_L, statV[LOWEST]), (m_model == CVariogram::POWER) ? 0.0 : max(LO_L, statD[LOWEST]);
		//upper_bound = min(HI_L, statV[HIGHEST]), min(HI_L, statV[HIGHEST]), (m_model == CVariogram::POWER) ? 2 : min(HI_L, statD[HIGHEST]);
		lower_bound = statV[LOWEST], statV[LOWEST], (m_model == CVariogram::POWER) ? 0.0 : statD[LOWEST];
		upper_bound = statV[HIGHEST], statV[HIGHEST], (m_model == CVariogram::POWER) ? 2 : statD[HIGHEST];

		//param is limited between lower and upper bounds
		double p[3] = { min(upper_bound(0), max(lower_bound(0), params(0))), min(upper_bound(1), max(lower_bound(1), params(1))), min(upper_bound(2), max(lower_bound(2), params(2))) };
		params = p[0], p[1], p[2];

		ASSERT(min(params) >= 0);
		
		//rho_begin : 0.0605543
		//rho_end : 0.001
		double rho_begin = min(upper_bound - lower_bound) / 10;
		ASSERT(min(upper_bound - lower_bound) > 2 * rho_begin);
		ASSERT(min(params - lower_bound) >= 0);
		ASSERT(min(upper_bound - params) >= 0);
	}


protected:

	int m_model;
	const CVariogramPredictorVector& m_samples;
};







//**********************************************************************
CRotationMatrix::CRotationMatrix()
{
	Reset();
}

void CRotationMatrix::Reset()
{
	for(int i=0; i<3; i++)
		for(int j=0; j<3; j++)
			m_rotmat[i][j] = i==j?1:0;

//	ZeroMemory(rotmat, sizeof( double)*TOTNST*3*3);
}

bool CRotationMatrix::operator == (const CRotationMatrix& in)const
{
	bool bEqual = true;

	for(int i=0; i<3&&bEqual; i++)
		for(int j=0; j<3&&bEqual; j++)
			if (fabs(m_rotmat[i][j] - in.m_rotmat[i][j]) > EPSILON)
				bEqual = false;

	return bEqual;
}

void CRotationMatrix::Init(double ang1, double ang2, double ang3, double anis1, double anis2)
/*-----------------------------------------------------------------------
*              Sets up an Anisotropic Rotation Matrix
*              **************************************
* Sets up the matrix to transform cartesian coordinates to coordinates
* accounting for angles and anisotropy (see manual for a detailed
* definition):
* INPUT PARAMETERS:
*   ang1             Azimuth angle for principal direction
*   ang2             Dip angle for principal direction
*   ang3             Third rotation angle
*   anis1            First anisotropy ratio
*   anis2            Second anisotropy ratio
*   MAXROT           The maximum number of rotation matrices dimensioned
*   rotmat           The rotation matrices
* Converts the input angles to three angles which make more
*  mathematical sense:
*         alpha   angle between the major axis of anisotropy and the
*                 E-W axis. Note: Counter clockwise is positive.
*         beta    angle between major axis and the horizontal plane.
*                 (The dip of the ellipsoid measured positive down)
*         theta   Angle of rotation of minor axis about the major axis
*                 of the ellipsoid.
*/
{
	double alpha, beta, theta, sina, sinb, sint, cosa, cosb, cost;
	double afac1, afac2;

	if(ang1 >= 0.0 && ang1 < 270.0)
		alpha = (double)((90.0 - ang1) * DEG2RAD);
	else
		alpha = (double)((450.0 - ang1) * DEG2RAD);
	
	beta  =(double)(-1.0 * ang2 * DEG2RAD);
	theta =(double)(        ang3 * DEG2RAD);

//* Get the required sines and cosines: 

	sina  = (double)sin((double)alpha);
	sinb  = (double)sin((double)beta);
	sint  = (double)sin((double)theta);
	cosa  = (double)cos((double)alpha);
	cosb  = (double)cos((double)beta);
	cost  = (double)cos((double)theta);

//* Construct the rotation matrix in the required memory: 

	afac1 = (double)(1.0 / max(anis1, EPSILON));
	afac2 = (double)(1.0 / max(anis2, EPSILON));

	m_rotmat[0][0] =       (cosb * cosa);
	m_rotmat[0][1] =       (cosb * sina);
	m_rotmat[0][2] =       (-sinb);
	m_rotmat[1][0] = afac1*(-cost*sina + sint*sinb*cosa);
	m_rotmat[1][1] = afac1*(cost*cosa + sint*sinb*sina);
	m_rotmat[1][2] = afac1*( sint * cosb);
	m_rotmat[2][0] = afac2*(sint*sina + cost*sinb*cosa);
	m_rotmat[2][1] = afac2*(-sint*cosa + cost*sinb*sina);
	m_rotmat[2][2] = afac2*(cost * cosb);

}

double  CRotationMatrix::sqdist(double x1,double y1,double z1,double x2,double y2,double z2)const
/*-----------------------------------------------------------------------
*    Squared Anisotropic Distance Calculation Given Matrix Indicator
*    ***************************************************************
* This routine calculates the anisotropic distance between two points
*  given the coordinates of each point and a definition of the
*  anisotropy.
* INPUT VARIABLES:
*   x1,y1,z1         Coordinates of first point
*   x2,y2,z2         Coordinates of second point
*   ind              The matrix indicator to initialize
*   MAXROT           The maximum number of rotation matrices dimensioned
*   rotmat           The rotation matrices
* OUTPUT VARIABLES:
*   sqdist           The squared distance accounting for the anisotropy
*                      and the rotation of coordinates (if any).
*/
{
	//double dx,dy,dz, sqd, cont;
	//int i;

	// Compute component distance vectors and the squared distance: 

	double dx = x1 - x2;
	double dy = y1 - y2;
	double dz = z1 - z2;
	double sqd = 0.0;
	for (int i=0; i<3; ++i)
	{
		double cont = m_rotmat[i][0] * dx +
		  m_rotmat[i][1] * dy + m_rotmat[i][2] * dz;
		sqd += cont*cont;
	}

	return(sqd);
}

const char* CVariogram::MODEL_NAME[NB_MODELS] = {"Sperical", "Exponential", "Gaussian", "Power", "Cubic","Pentaspherical","Sine Hole Effect"};

		
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVariogram::CVariogram()
{
    Reset();
}

CVariogram::CVariogram(const CVariogram& in)
{
    operator =(in);
}

CVariogram::~CVariogram()
{
}

void CVariogram::Reset()
{
	m_pAgent=NULL;
	m_hxGridSessionID=0;
    m_model = SPERICAL;
    m_nLags=40;
	m_dLag=0.0f;
	m_detrending.Reset();
	m_rotmat.Reset();

	m_nugget = 0;
    m_sill = 0;
    m_range = 0;
    m_R2 = 0;

	m_PMX = 999;
}

CVariogram& CVariogram::operator =(const CVariogram& in)
{
	if( &in != this)
	{
		m_pAgent=in.m_pAgent;
		m_hxGridSessionID=in.m_hxGridSessionID;
		m_model = in.m_model;
		m_nLags = in.m_nLags;
		m_dLag = in.m_dLag;
		m_detrending = in.m_detrending;
		m_rotmat = in.m_rotmat;

		m_nugget = in.m_nugget;
		m_sill = in.m_sill;
		m_range = in.m_range;
		m_R2 = in.m_R2;

		m_predictorVector = in.m_predictorVector;
		m_PMX = in.m_PMX;
	}

	ASSERT( *this == in);

	return *this;
}

bool CVariogram::operator == (const CVariogram& in)const
{
	bool bEqual = true;

	
	if(m_model != in.m_model)bEqual = false;
	if(m_nugget != in.m_nugget)bEqual = false;
	if(m_sill != in.m_sill)bEqual = false;
	if(m_range != in.m_range)bEqual = false;
	if(m_R2 != in.m_R2)bEqual = false;
	if( m_detrending != in.m_detrending)bEqual = false;
	if (m_dLag != in.m_dLag)bEqual = false;
	if (m_nLags != in.m_nLags)bEqual = false;
	if( m_rotmat != in.m_rotmat)bEqual = false;
	
	if( !std::equal( m_predictorVector.begin(), m_predictorVector.end(), in.m_predictorVector.begin() ) )
		bEqual=false;
	
	
	return bEqual;
}

ERMsg CVariogram::ComputePredictor(const CGridPointVector& pts, const CPrePostTransfo& transfo, int nLags, float dLag, const CDetrending& detrending, const CRotationMatrix& rotmat, CVariogramPredictorVector& lagVar)
{
	ERMsg msg;

	if (detrending != m_detrending)
	{
		m_detrending = detrending;

		if (m_detrending.Compute(pts, transfo) == -999)
		{
			m_R2 = -99999;
			m_sill = m_range = m_nugget = 0;
			msg.ajoute("Unable to create detrending");
			return msg;
		}
	}

	if (nLags != m_nLags ||
		dLag != m_dLag ||
		rotmat != m_rotmat)
	{

		m_nLags = nLags;
		m_dLag = dLag;
		m_rotmat = rotmat;

		int nsiz = nLags + 1;//+1 class at the begging 
		vector<int> np(nsiz);
		vector<double> dis(nsiz);
		vector<double> gam(nsiz);

		// Define maximum distance
		const double D_HIGHT = (nLags + 0.5)*dLag;

		// main loop over all pairs:
#pragma omp parallel for 
		for (int i = 0; i < pts.size(); i++)
		{
			ASSERT(pts[i].m_event > -FLT_MAX);
			for (int j = i + 1; j < pts.size() && msg; j++)//By RSA 27/10/2010, don't compute distance twice
			{
				if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID))
					throw(CHxGridException());

				// Definition of the lag corresponding to the current pair:
				double d = pts[i].GetDistance(pts[j]);


				if (d <= D_HIGHT)
				{
					// Determine which lag this is and skip if outside the defined distance tolerance:
					int il = (int)(d / m_dLag + 0.5);
					ASSERT(il >= 0 && il < nsiz);

					//compute residual value of f for i and j
					double ei = m_detrending.GetF(pts[i]);
					double vri = transfo.Transform(pts[i].m_event) - ei;
					double ej = m_detrending.GetF(pts[j]);
					double vrj = transfo.Transform(pts[j].m_event) - ej;

					np[il]++;
					dis[il] += d;
					gam[il] += ((vri - vrj)*(vri - vrj));
				}
			}
		}


		// Get average values for gam, then compute
		// the semivariogram:
		//Transfer lags with np>0 to output arrays
		lagVar.clear();
		for (int i = 0; i < nsiz; i++)
		{
			if (np[i]>0 && (lagVar.empty() || gam[i] > 0))
			{
				dis[i] = dis[i] / np[i];
				gam[i] = 0.5*gam[i] / np[i];

				CVariogramPredictor vp;
				vp.m_np = np[i] * 2;  //*2 by RSA to obtain same result without computing distance twice
				vp.m_lagDistance = dis[i];
				vp.m_lagVariance = gam[i];
				lagVar.push_back(vp);
			}
		}
	}
	return msg;
}


//approximation of suggested lag
double CVariogram::GetSuggestedLag(const CGridPointVector& pts)
{
	//LAG_DISTANCE_CAHE.Enter();

	double maxDist = 0;
	
	int inc = max(1, int(pts.size() / 1000) );
	#pragma omp parallel for 
	for(int i=0; i<pts.size(); i+=inc) 
	{
		for(int j=i+1; j<pts.size(); j+=inc) 
		{
			double d = pts.GetDistance(i, j);
			if( d > maxDist)
				maxDist = d;
		}
	}

	return 2 * maxDist / pts.size();
}


ERMsg CVariogram::FitVariogramModels(int model, const CVariogramPredictorVector& lagVar, double& final_nugget,double& final_sill,double& final_range,double& final_r2)
{
	ASSERT(model>= SPERICAL && model <NB_MODELS);
    ERMsg msg;
	

	if (lagVar.size() < 8)//this variogram doesn't have the minimum number of lags
	{
		m_R2 = -999;
		msg.ajoute("Not enough lags");
		return msg;
	}
		

	
	try
	{


		// We need to supply a starting point for the optimization.
		dlib::matrix<double> x(3, 1);
		// We also need to supply lower and upper bounds for the search.  
		dlib::matrix<double> x_lower(3, 1), x_upper(3, 1);

		cross_validation_objective CVO(model, lagVar);
		CVO.ComputeParams(x, x_lower, x_upper);

		ASSERT(is_col_vector(x));
		ASSERT(is_col_vector(x_lower));
		ASSERT(is_col_vector(x_upper));
		ASSERT(x.size() == x_lower.size());
		ASSERT(x_lower.size() == x_upper.size());
		ASSERT(x.size() > 1);

		long npt = x.size() * 2 + 1;
		double rho_begin = min(x_upper - x_lower) / 10;
		double rho_end = min(0.001, rho_begin/2);
		ASSERT(x.size() + 2 <= npt);
		ASSERT(npt <= (x.size() + 1)*(x.size() + 2) / 2);
		ASSERT(0 < rho_end);
		ASSERT(rho_end < rho_begin);
		ASSERT(min(x_upper - x_lower) > 2 * rho_begin);
		ASSERT(min(x - x_lower) >= 0 && min(x_upper - x) >= 0);
			

		// Finally, ask BOBYQA to look for the best set of parameters.  Note that we are using the
		// cross validation function object defined at the top of the file.
		double best_score =
			find_min_bobyqa(
			CVO,		// Function to maximize
			x,			// starting point
			npt,        // See BOBYQA docs, generally size*2+1 is a good setting for this
			x_lower,    // lower bound 
			x_upper,    // upper bound
			rho_begin,  // search radius
			rho_end,    // desired accuracy
			1000        // max number of allowable calls to cross_validation_objective()
			);

		m_nugget = x(0);
		m_sill = x(1);
		m_range = x(2);

		double RSS = CVO.RSS(x, false);
		double TSS = CVO.TSS();

		m_R2 = 1 - RSS / TSS;
	}
	catch (exception& e)
	{
		msg.ajoute( e.what() );
		m_R2 = -999;
	}


	return msg;
}


ERMsg CVariogram::CreateVariogram(const CGridPointVector& pts, const CPrePostTransfo& transfo, int model, int nLags, float dLag,
		const CDetrending& detrending, const CRotationMatrix& rotmat )
{
    ASSERT(model>=SPERICAL && model < NB_MODELS);
    ASSERT(nLags > 0);
    ASSERT(dLag > 0);

    ERMsg msg;

	
	//size_t checkSum = pts.GetCheckSum();

	//bypass variogram if all parameters is the same
	//note that prePostTransfo must be constant
	if (model != m_model ||
		nLags != m_nLags ||
		dLag != m_dLag ||
		detrending != m_detrending ||
		rotmat != m_rotmat)
	{
		m_model = model;
		//compute la varience only if it's not the same

		msg = ComputePredictor(pts, transfo, nLags, dLag, detrending, rotmat, m_predictorVector);
		if (msg)
			msg = FitVariogramModels(m_model, m_predictorVector, m_nugget,m_sill,m_range,m_R2);
	}

	//m_CS.Leave();
	
    return msg;
}

ERMsg CVariogram::Save(std::string filePath)const
{
	ERMsg msg;

	ofStream file;
	msg = file.open(filePath);
	if (msg)
	{
		for (size_t i = 0; i < m_predictorVector.size(); i++)
		{
			file << m_predictorVector[i].m_lagDistance << "," << m_predictorVector[i].m_np << "," << m_predictorVector[i].m_lagVariance << "," << m_predictorVector[i].m_predictedVariance << endl;
		}
	}

	return msg;
}

/*
void CVariogram::WriteLog(CStdString& m_logBook)const
{
    CStdString tmp;

    //*****************************
    m_logBook += "Variogram Input Log:\r\n\r\n";

    tmp.Format("Nb Lags: %d\r\nxLag: %f\r\nxlTol: %f\r\nbandw: %f\r\n\r\n", 
        m_nlags, m_xlag, m_xltol, m_bandw);

    m_logBook += tmp;


    //*****************************
    m_logBook += "Variogram Output Log:\r\n\r\n";

    
    tmp.Format("Best Model: %s\r\nNugget: %lf\r\nSill: %lf\r\nRange: %lf\r\nR²: %lf\r\n\r\n",
        GetModelName(m_model), m_nugget, m_sill, m_range, m_R2);

    m_logBook += tmp;
    

    //*****************************
    m_logBook += "Detrending Output Log:\r\n\r\n";

    
    tmp.Format("Lat Check: %1d\r\nLon Check: %1d\r\nLat² Check: %1d\r\nLon² Check: %1d\r\nLat Lon Check: %1d\r\nDrift Check: %1d\r\n\r\n",
        m_latcheck,	m_loncheck,	m_lat2check, m_lon2check, m_latloncheck, m_driftcheck);

    m_logBook += tmp;
    
    //*****************************
    m_logBook += "Detrending Parameter Output Log: \r\n\r\n";

	tmp.Format("Intercept: %f\r\nLat Param: %f\r\nLon Param: %f\r\nLat² Param: %f\r\nLon² Param: %f\r\nLat Lon Param: %f\r\nDrift Param: %f\r\nDetrend R²: %f\r\n\r\n", 
        m_intercept, m_latparm, m_lonparm, m_lat2parm, m_lon2parm, m_latlonparm, m_driftparm, m_detrendr2);
	
    m_logBook += tmp;
	
    m_logBook += "\r\n-----------------------------------\r\n";
}

ERMsg CVariogram::CreateDataFile(const CStdString& filePathIn)const
{
    ERMsg msg; 

    CStdString path = UtilWin::GetPath(filePathIn);
    CStdString title = UtilWin::GetFileTitle(filePathIn);
    
    CStdString filePath;
    filePath = path + title + ".Variogram";

	CStdioFile file;
	CFileException e;
    if( file.Open(filePath, CFile::modeWrite|CFile::modeCreate, &e) )
    {
        CStdString tmp;
        for(int i=0; i<m_varioArray.GetSize(); i++)
        {
            tmp.Format("%lf %lf %lf\n",
                m_varioArray[i].m_lagDistance,
		        m_varioArray[i].m_lagVariance,
		        m_varioArray[i].m_predictedVariance);
            file.WriteString(tmp);
        }
    
        file.Close();
    }
    else
    {
		msg = SYGetMessage(e);
        return msg;
    }
    return msg;
}



ERMsg CVariogram::CreateCommandFile(const CStdString& filePathIn )const
{

    CStdString path = UtilWin::GetPath(filePathIn);
    CStdString title = UtilWin::GetFileTitle(filePathIn);
    CStdString filePath = path + title + "Variogram.cmd";
    //CStringArray ressourceLableArray;
    //LoadRessourceLable( IDS_COM_VARIOGRAMRESULT_HEADER, 3, ressourceLableArray);
	//ASSERT( ressourceLableArray.GetSize() == 3);


    ERMsg msg;

	CStdioFile file;
	CFileException e;
    if( file.Open(filePath, CFile::modeWrite|CFile::modeCreate, &e) )
    {
	
        CStdString dataFilePath = path + title + ".Variogram";

	    CStdString tmp;
	    tmp.Format("missing %f\n", VMISS);	
	    file.WriteString(tmp);
	    tmp.Format("file %s %d\n", dataFilePath , 3);
	    file.WriteString(tmp);
	    tmp.Format("pal 1 9 9 9\n");
	    file.WriteString(tmp);
	    tmp.Format("pal 8 0 0 0\n");
	    file.WriteString(tmp);
        tmp.Format("title 1 Variogram ( R² = %.3lf )\n", m_R2 );
	    file.WriteString(tmp);
        tmp.Format("title x1 %s\n", "Lag Distance" );
	    file.WriteString(tmp);
	    tmp.Format("title y1 %s\n", "Lag Variance" );
	    file.WriteString(tmp);
        tmp.Format("font %d\n", 7 );
        file.WriteString(tmp);
        tmp.Format("GRO %d L\n", 3 );
        file.WriteString(tmp);

		    
    
	    tmp.Format("sort\n");
	    file.WriteString(tmp);

	    tmp.Format("variable x1 1\n");
	    file.WriteString(tmp);
	    tmp.Format("variable y1 2\n");
	    file.WriteString(tmp);	
	    
	    file.WriteString("line 1 0\n");
	    file.WriteString("sym 1 .1\n");
	    
	    tmp.Format("variable x1 1\n");
	    file.WriteString(tmp);
	    tmp.Format("variable y1 3\n");
	    file.WriteString(tmp);	


	    
	    file.Close();
    }
    else
    {
		msg = SYGetMessage(e);
        return msg;
    }


	return msg;	
}
*/



//********************************/
double    CVariogram::expfn    (double x)
//********************************/
{
	if (x > DBL_MAX_10_EXP)
		x = DBL_MAX_10_EXP;
	else if (x < -DBL_MAX_10_EXP)
		return 0;
	
	return exp(x);
}

//********************************
double   CVariogram::powfn    (double x, double p)
//********************************
{

	double t = (p * log(x));
	return(expfn(t));
}

double  CVariogram::cova3 (const CGridPoint& pt1, const CGridPoint& pt2)const
{
	double h = pt1.GetDistance(pt2);
	if (h < EPSILON)
		return GetMaxCov();


	double cova3 = 0.0;


	double aa=GetRange();
	double cc=GetSill();
	double hr = h/aa;
	double hh = (h*h)/(aa*aa);


	ASSERT( h>= 0);
	switch (m_model) 
	{
	case SPERICAL:			cova3 = (hr < 1.0)?cc*(1.0 - hr*(1.5-0.5*hr*hr)):0; break;
	case EXPONENTIAL:		cova3 = cc*expfn(-hr); break;
	case GAUSSIAN:			cova3 = cc*expfn(-hh); break;
	case POWER:				cova3 = m_PMX - cc*powfn(h,aa);break;
	case CUBIC:				cova3 = (hr < 1.0) ? cc * (7 * pow(hr, 2) - 35 / 4 * pow(hr, 3) + 7 / 2 * pow(hr, 5) - 3 / 4 * pow(hr, 7)) : 0; break;
	case PENTASPHERICAL:	cova3 = (hr < 1.0) ? cc * (15 / 8 * pow(hr, 1) - 5 / 4 * pow(hr, 3) + 3 / 8 * pow(hr, 5)) : 0; break;
	case SINE_HOLE_EFFECT:	cova3 = (hr < 1.0) ? cc * (1.0 - sin(PI*hr) / (PI*hr)) : 0; break;
	default: ASSERT(false);
	}

	return cova3;
}

double  CVariogram::cova3 (double x1, double y1, double z1, double x2, double y2, double z2)const
/*-----------------------------------------------------------------------
*              Covariance Between Two Points (3-D Version)
*              *******************************************
* This function returns the covariance associated with a variogram model
* that is specified by a nugget effect and possibly four different
* nested varigoram structures.  The anisotropy definition can be
* different for each of the nested structures (spherical, exponential,
* gaussian, or power).
* INPUT VARIABLES:
*   x1,y1,z1         Coordinates of first point
*   x2,y2,z2         Coordinates of second point
*   nst              Number of nested structures (max. 4).
*   c0               Nugget constant (isotropic).
*   PMX              Maximum variogram value needed for kriging when
*                      using power model.  A unique value of m_p.PMX is
*                      used for all nested structures which use the
*                      power model.  therefore, m_p.PMX should be chosen
*                      large enough to account for the largest single
*                      structure which uses the power model.
*   cc(nst)          Multiplicative factor of each nested structure.
*                      slope for linear model.
*   aa(nst)          Parameter "a" of each nested structure.
*   it(nst)          Type of each nested structure:
*                      1. spherical model of range a;
*                      2. exponential model of parameter a;
*                           i.e. practical range is 3a
*                      3. gaussian model of parameter a;
*                           i.e. practical range is a*sqrt(3)
*                      4. power model of power a (a must be gt. 0  and
*                           lt. 2).  if linear model, a=1,c=slope.
*   ang1(nst)        Azimuth angle for the principal direction of
*                      continuity (measured clockwise in degrees from Y)
*   ang2(nst)        Dip angle for the principal direction of continuity
*                      (measured in negative degrees down)
*   ang3(nst)        Third rotation angle to rotate the two minor
*                      directions around the principal direction.  A
*                      positive angle acts clockwise while looking
*                      in the principal direction.
*   anis1(nst)       Anisotropy (radius in minor direction at 90
*                      degrees from ang1 divided by the principal radius
*                      in direction ang1)
*   anis2(nst)       Anisotropy (radius in minor direction at 90 degrees
*                      vertical from "ang1" divided by the principal
*                      radius in direction "ang1")
*   first            A logical variable which is set to true if the
*                      direction specifications have changed - causes
*                      the rotation matrices to be recomputed.
* OUTPUT VARIABLES: returns "cova3" the covariance obtained from the
*                   variogram model.
*/
{
	// Check for very small distance: 

	double hsqd = m_rotmat.sqdist(x1,y1,z1,x2,y2,z2);
	if (hsqd < EPSILON)
		return GetMaxCov();

	// Compute the appropriate structural distance: 
	double cova3 = 0.0;


	double aa=GetRange();
	double cc=GetSill();
	


	ASSERT( hsqd>= 0);
	double h = sqrt(hsqd);
	switch (m_model) 
	{
		case SPERICAL:
		{
			double hr = h/aa;
			if(hr < 1.0)
				cova3 = cc*(1.0 - hr*(1.5-.5*hr*hr));
			break;
		}
		case EXPONENTIAL:
		{
			cova3 = cc*expfn(-h/aa);
			break;
		}
		case GAUSSIAN:
		{
			double hh = -(h*h)/(aa*aa);
			cova3 = cc*expfn(hh);
			break;
		}

		case POWER: 
		{
			double cov1  = m_PMX - cc*powfn(h,aa);
			cova3 = cov1;
			break;
		}
		case CUBIC:				cova3 = (h / aa < 1.0) ? cc * (7 * pow(h / aa, 2) - 35 / 4 * pow(h / aa, 3) + 7 / 2 * pow(h / aa, 5) - 3 / 4 * pow(h / aa, 7)) : 0; break;
		case PENTASPHERICAL:	cova3 = (h / aa < 1.0) ? cc * (15 / 8 * pow(h / aa, 1) - 5 / 4 * pow(h / aa, 3) + 3 / 8 * pow(h / aa, 5)) : 0; break;
		case SINE_HOLE_EFFECT:	cova3 = (h / aa < 1.0) ? cc * (1.0 - sin(PI*h / aa) / (PI*h / aa)) : 0; break;


	}

	return (cova3);
}



//***********************************************************************************************************************************
// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

This is an example that shows some reasonable ways you can perform
model selection with the dlib C++ Library.

It will create a simple set of data and then show you how to use
the cross validation and optimization routines to determine good model
parameters for the purpose of training an svm to classify the sample data.

The data used in this example will be 2 dimensional data and will
come from a distribution where points with a distance less than 10
from the origin are labeled +1 and all other points are labeled
as -1.


As an side, you should probably read the svm_ex.cpp and matrix_ex.cpp example
programs before you read this one.
*/
//
//class CFourierSeries
//{
//public:
//
//	CFourierSeries(double A°, double A¹, double B¹) :
//		m_A°(A°), m_A¹(A¹), m_B¹(B¹)
//	{}
//
//	//d: julian day (0..365)
//	//Tair: air temperature [°C]
//	double operator() (double d) const
//	{
//		assert(d >= 0 && d < 365);
//		return m_A° / 2 + m_A¹*cos(2 * M_PI*d / 365) + m_B¹*sin(2 * M_PI*d / 365);
//	}
//
//
//protected:
//
//	double m_A°;
//	double m_A¹;
//	double m_B¹;
//};

//
//
//void LoadSample(const std::string& filePath, std::vector<sample_type>& samples)
//{
//	std::ifstream file;
//
//	file.open(filePath);
//	if (file.is_open())
//	{
//
//		string line;
//		std::getline(file, line); //read header
//		while (std::getline(file, line))
//		{
//			stringstream iss(line);
//			std::vector <string> data;
//
//			std::string token;
//			while (std::getline(iss, token, ','))
//				data.push_back(token);
//
//
//			int m = stoi(data[1]) - 1;
//			int d = stoi(data[2]) - 1;
//			int h = stoi(data[3]);
//			double Tair = stod(data[4]);
//			if (Tair > -999)
//			{
//
//				sample_type M;
//				M = (double)((m * 31 * 24 + d * 24 + h) * 365) / (12 * 31 * 24), Tair;
//				samples.push_back(M);
//			}
//		}
//	}
//}
//
//matrix<double> ComputeParams(const std::vector<sample_type>& samples)
//{
//	matrix<double> params;
//	params.set_size(4, 1);
//	params = 0, 0, 0, 0;
//
//	for (int d = 0; d != samples.size(); ++d)
//	{
//		double D = samples[d](0);
//		double Tair = samples[d](1);
//		double a = 2 * Tair / samples.size();
//		params(0) += a;
//		params(1) += a*cos(2 * M_PI*D / 365);
//		params(2) += a*sin(2 * M_PI*D / 365);
//	}
//
//	//params(3) = atan2(params(2), params(1));
//	params(3) = atan(params(2) / params(1));
//
//	return params;
//}
//
//int Powell()
//{
//	try
//	{
//
//		// Now we make objects to contain our samples and their respective labels.
//		//std::vector<sample_type> samples;
//		//LoadSample("D:\\project\\Dlib\\Powell\\meteo.csv", samples);
//
//		//cout << "Load " << samples.size() << " points" << endl;
//
//		dlib::matrix<double> result = ComputeParams(samples);
//		//cout << " Computed result: " << endl;
//		//cout << " Ao: " << result(0) << "   A1: " << result(1) << " B1: " << result(2) << " angle : " << result(3) * 180 / M_PI << endl;
//		//cout << "\n\n Try the BOBYQA algorithm" << endl;
//
//		// We need to supply a starting point for the optimization.
//		dlib::matrix<double> params(3, 1);
//		//params.set_size(3, 1);
//		params = result(0), result(1), result(2);
//
//		// We also need to supply lower and upper bounds for the search.  
//		dlib::matrix<double> lower_bound(3, 1), upper_bound(3, 1);
//		lower_bound = -60, -30, -30;
//		upper_bound = 60, 30, 30;
//
//		// Finally, ask BOBYQA to look for the best set of parameters.  Note that we are using the
//		// cross validation function object defined at the top of the file.
//		double best_score = find_min_bobyqa(
//			cross_validation_objective(samples),		 // Function to maximize
//			params,                                      // starting point
//			params.size() * 2 + 1,                       // See BOBYQA docs, generally size*2+1 is a good setting for this
//			lower_bound,                                 // lower bound 
//			upper_bound,                                 // upper bound
//			min(upper_bound - lower_bound) / 10,         // search radius
//			0.001,                                       // desired accuracy
//			1000                                         // max number of allowable calls to cross_validation_objective()
//			);
//
//
//		cout << " best result of BOBYQA: " << best_score << endl;
//		cout << " best Ao: " << params(0) << "   best A1: " << params(1) << " best B1: " << params(2) << " angle : " << atan(params(2) / params(1)) * 180 / M_PI << endl;
//
//		// Also note that the find_max_bobyqa() function only works for optimization problems
//		// with 2 variables or more.  If you only have a single variable then you should use
//		// the find_max_single_variable() function.
//
//
//	}
//	catch (exception& e)
//	{
//		cout << e.what() << endl;
//	}
//}
//

}