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


#include "Basic/OpenMP.h"
#include "Basic/UtilStd.h"
#include "hxGrid/interface\IAgent.h"
#include "Geomatic/UniversalKriging.h"
#include "Newmat/NewMat.h"


namespace WBSF
{

	CVariogramCache CUniversalKriging::VARIOGRAM_CACHE;

	void CUniversalKriging::FreeMemoryCache()
	{
		VARIOGRAM_CACHE.FreeMemoryCache();
	}


	static const double LIMIT = 0.001;//a revoir
	static const double EPSLON = 0.000000000000001;


	static const int TERM_DEFINE[15][4] =
	{
		{ 0, 0, 0 },
		{ 1, CTerm::LAT, 0 },
		{ 1, CTerm::LON, 0 },
		{ 1, CTerm::ELEV, 0 },
		{ 1, CTerm::EXPO, 0 },
		{ 3, CTerm::LAT, CTerm::LON, CTerm::LAT | CTerm::LON },
		{ 3, CTerm::LAT, CTerm::ELEV, CTerm::LAT | CTerm::ELEV },
		{ 3, CTerm::LAT, CTerm::EXPO, CTerm::LAT | CTerm::EXPO },
		{ 3, CTerm::LON, CTerm::ELEV, CTerm::LON | CTerm::ELEV },
		{ 3, CTerm::LON, CTerm::EXPO, CTerm::LON | CTerm::EXPO },
		{ 3, CTerm::ELEV, CTerm::EXPO, CTerm::ELEV | CTerm::EXPO },
		{ 1, CTerm::LAT², 0 },
		{ 1, CTerm::LON², 0 },
		{ 1, CTerm::ELEV², 0 },
		{ 1, CTerm::EXPO², 0 },
	};

	CParamUK::CParamUK()
	{
		Reset();
	}

	void CParamUK::Reset()
	{
		m_xsiz = 0;
		m_ysiz = 0;

		m_nxdis = 1;
		m_nydis = 1;
		m_nzdis = 1;

		m_radius = 100000;//100 km


		//bFiltnug=false;//old value
		//bFiltnug=true;//new value


		m_bTrend = false;
		m_bSK = false;
		m_SKmean = 0;

	}

	//**********************************************************************
	//CUniversalKriging
	CUniversalKriging::CUniversalKriging() :
		m_pVariogram(new CVariogram)
	{
		Reset();
	}

	CUniversalKriging::~CUniversalKriging()
	{
		delete m_pVariogram;
	}

	void CUniversalKriging::Reset()
	{
		m_p.Reset();
		
		m_lastCheckSum=0;
//		m_lastParam.Reset();
		//m_lastPrePostTransfo.Reset();

		if (m_pVariogram)
			m_pVariogram->Reset();
		m_externalDrift.Reset();
	}

	

	CVariogram* CUniversalKriging::GetVariogram()const
	{
		CVariogram* pVariogram = NULL;

		CVariogramInput input;
		input.m_model = m_param.m_variogramModel;
		input.m_nlags = m_param.m_nbLags;
		input.m_xlag = (float)m_param.m_lagDist;
		input.m_detrendingModel = m_param.m_detrendingModel;
		input.m_bRotmat = false;
		input.m_checkSum = m_pPts->GetCheckSum();


		VARIOGRAM_CACHE.Enter();


		CVariogramCache::const_iterator it = VARIOGRAM_CACHE.find(input);
		if (it == VARIOGRAM_CACHE.end())
		{

			//try to find a variogram with the same lag distance and lag varaince
			VARIOGRAM_CACHE[input].reset(new CVariogram);

			it = VARIOGRAM_CACHE.find(input);
			ASSERT(it != VARIOGRAM_CACHE.end());
		}

		pVariogram = it->second.get();

		VARIOGRAM_CACHE.Leave();

		return pVariogram;
	}

	bool CUniversalKriging::GetVariogram(CVariogram& variogram)const
	{
		if (m_pVariogram != NULL)
			variogram = *m_pVariogram;
		
		return m_pVariogram!=NULL;
	}

	ERMsg CUniversalKriging::Initialization()
	{
		ERMsg msg = CGridInterpolBase::Initialization();

		if (!m_bInit)
			m_pVariogram->Reset(); // force creation of the variogram
	
		//detrending and variogram
		//m_pVariogram = GetVariogram();
		//m_pVariogram = new CVariogram;

		int d = m_param.m_detrendingModel;
		CDetrending detrending(TERM_DEFINE[d][0]);
		for (int i = 0; i < TERM_DEFINE[d][0]; i++)
			detrending[i] = TERM_DEFINE[d][i + 1];

		

		msg = m_pVariogram->CreateVariogram(*m_pPts, m_prePostTransfo, m_param.m_variogramModel, m_param.m_nbLags, m_param.m_lagDist, detrending);


		if (msg)
		{
			int e = m_param.m_externalDrift;
			m_externalDrift.resize(TERM_DEFINE[e][0]);
			for (int i = 0; i < TERM_DEFINE[e][0]; i++)
				m_externalDrift[i] = TERM_DEFINE[e][i + 1];


			size_t checkSum = m_pPts->GetCheckSum();
			if (checkSum != m_lastCheckSum)
			{
				m_lastCheckSum = checkSum;
				m_pANNSearch = make_unique<CANNSearch>();
				m_pANNSearch->Init(m_pPts, m_param.m_bUseElevation);
			}

			//fill-in information
			m_p.m_nbPoint = m_param.m_nbPoints;
			m_p.m_xsiz = m_info.m_cellSizeX;
			m_p.m_ysiz = m_info.m_cellSizeY;
			
			m_bInit = true;
		}

		return msg;
	}

	void CUniversalKriging::Init_a(const CGridPoint& pt, CGridPointVector& va, NEWMAT::Matrix &a)const
	{
		ASSERT(a.Ncols() == a.Nrows());


		size_t neq = va.size() + GetMDT();
		// Initialize the main kriging matrix: 
		a.ReSize(int(neq), int(neq));
		a = 0;

		// Fill in the kriging matrix: 
		for (int i = 0; i < (int)va.size(); i++)
		{
			for (int j = i; j<(int)va.size(); j++)
			{
				double cov = m_pVariogram->cova3(va[i], va[j]);

				a[i][j] = cov;
				a[j][i] = cov;
			}
		}

		// Fill in the OK unbiasedness portion of the matrix (if not doing SK): 
		if (!m_p.m_bSK)
		{
			double unbias = m_pVariogram->GetUnbias();
			ASSERT(neq > va.size());
			for (int i = 0; i < (int)va.size(); ++i)
			{
				a[(int)va.size()][i] = unbias;
				a[i][(int)va.size()] = unbias;
			}
		}

		// Add the additional Unbiasedness constraints: 
		int im = int(va.size()) + 1;

		//const CDetrending& detrending = m_pVariogram->GetDetrending();
		for (int i = 0; i < (int)m_externalDrift.size(); i++)
		{
			double resce = m_pVariogram->GetMaxCov() / max(LIMIT, m_externalDrift[i].GetE(pt));
			//compute 
			for (int j = 0; j < (int)va.size(); j++)
			{
				a[im][j] = m_externalDrift[i].GetE(va[j])*resce;
				a[j][im] = a[im][j];
			}

			im++;
		}
	}

	void CUniversalKriging::Init_r(const CGridPoint& pt, CGridPointVector& va, NEWMAT::ColumnVector& r)const
	{

		double unbias = m_pVariogram->GetUnbias();
		size_t neq = va.size() + GetMDT();
		r.resize((int)neq);
		r = 0;

		CDiscretisation m_discr;
		m_discr.Initialization(pt, *m_pVariogram, m_p, m_param.m_bFillNugget);
		// Set up the right hand side: 
		for (int i = 0; i < (int)va.size(); i++)
		{
			//compute mean over dicretisation
			double cb = 0.0;
			for (int j = 0; j < (int)m_discr.size(); j++)
			{
				cb += m_pVariogram->cova3(va[i], m_discr[j]);

				double d = va[i].GetDistance(va[j]);
				if (d < EPSLON && (m_discr.size() > 1 || m_param.m_bFillNugget))
					cb -= m_pVariogram->GetNugget();
			}
			cb = cb / m_discr.size();
			r[i] = cb;
		}


		// Fill in the OK unbiasedness portion of the matrix (if not doing SK): 
		if (!m_p.m_bSK)
			r[(int)va.size()] = unbias;

		// Add the additional Unbiasedness constraints: 
		int im = int(va.size()) + 1;

		double resc = max(m_pVariogram->GetMaxCov(), LIMIT) / (2.0 * m_p.m_radius);
		const CDetrending& detrending = m_pVariogram->GetDetrending();


		for (int i = 0; i < m_externalDrift.size(); i++)
		{
			r[im] = m_pVariogram->GetMaxCov();
			im++;
		}

	}

	void CUniversalKriging::Init_va(const CGridPoint& pt, int iXval, CGridPointVector& va)const
	{

		CGridPointResultVector result;
		size_t nbPoint = m_p.m_nbPoint + (iXval >= 0 ? 1 : 0);
		m_pANNSearch->Search(pt, nbPoint, result);
		if (iXval >= 0)
		{
			for (size_t i = 0; i < result.size(); i++)
			{
				if (result[i].i == iXval)
				{
					result.erase(result.begin() + i);
					break;
				}
			}
		}

		va.resize(result.size());
		for (size_t i = 0; i < result.size(); i++)
		{
			const CGridPoint& pt = m_pPts->at(result[i].i);
			va[i] = pt;
		}
	}



	double CUniversalKriging::Evaluate(const CGridPoint& pt, int iXval)const
	{
		//if variogram is not initialised correckly, we retuern no data;
		if (!m_pVariogram->IsInit())
			return m_param.m_noData;

		// Find the nearest samples: 
		CGridPointVector va;

		Init_va(pt, iXval, va);

		int mdt = GetMDT();
		// Test number of samples found: 
		// Test if there are enough samples to estimate all drift terms: 
		if ((int)va.size() < m_p.m_nbPoint || (int)va.size() <= mdt)
			return m_param.m_noData;

		if ( /*iXval<0 &&*/ va[0].GetDistance(pt) > m_param.m_maxDistance)
			return m_param.m_noData;

		// There are enough samples - proceed with estimation. 
		// Go ahead and set up the OK portion of the kriging matrix: 

		NEWMAT::ColumnVector r;
		NEWMAT::Matrix a;

		Init_a(pt, va, a);
		Init_r(pt, va, r);

		// If estimating the trend then reset all the right hand side terms=0.0:
		if (m_p.m_bTrend)
			r = 0;

		//copy r to compute variance
		NEWMAT::ColumnVector rr(r);


		// Solve the kriging system: 
		int neq = mdt + int(va.size());

		//reduce the matrix until the 2/3 of the number of neightbor
		bool bOK = false;
		while (!bOK && neq >= mdt + m_p.m_nbPoint * 2 / 3)//by RSA 25/10/2010
		{
			Try
			{
				a = a.i();
				bOK = true;
			}
				Catch(NEWMAT::Exception)
			{
				OutputDebugStringW(L"a=a.i() failled; Reduce number of point");

				Init_a(pt, va, a);
				Init_r(pt, va, r);

				neq--;
				int firstRow = 1 + mdt + int(va.size()) - neq;
				a = a.SubMatrix(firstRow, a.Nrows(), firstRow, a.Ncols());
				r = r.SubMatrix(firstRow, r.Nrows(), 1, 1);
			}

			if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID) == S_FALSE)
				throw(CHxGridException());

		}

		//if we don't solve system, return missing
		if (!bOK)
			return m_param.m_noData;


		NEWMAT::ColumnVector s = a*r;


		CDiscretisation m_discr;
		m_discr.Initialization(pt, *m_pVariogram, m_p, m_param.m_bFillNugget);

		// Compute the solution: 
		double uuk = 0.0;
		double ukv = m_discr.cbb;
		for (int j = 0; j < neq; j++)
		{
			ukv -= s[j] * rr[j];
			if (j < (int)va.size())
				uuk += s[j] * (m_prePostTransfo.Transform(va[j].m_event) - m_p.m_SKmean);
		}

		uuk += m_p.m_SKmean;
		uuk = m_prePostTransfo.InvertTransform(uuk, m_param.m_noData);
		//
		if ( /*iXval<0 &&*/ m_param.m_bRegionalLimit  && uuk > m_param.m_noData)
		{
			CStatistic stat;
			for (size_t i = 0; i < va.size(); i++)
				stat += va[i].m_event;

			bool bOutside = uuk<stat[LOWEST] - m_param.m_regionalLimitSD*stat[STD_DEV] || uuk>stat[HIGHEST] + m_param.m_regionalLimitSD*stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bRegionalLimitToBound)
					uuk = min(stat[HIGHEST] + m_param.m_regionalLimitSD*stat[STD_DEV], max(stat[LOWEST] - m_param.m_regionalLimitSD*stat[STD_DEV], uuk));
				else
					uuk = m_param.m_noData;
			}
		}

		if ( /*iXval<0 &&*/ m_param.m_bGlobalLimit && uuk > m_param.m_noData)
		{
			bool bOutside = uuk<m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV] || uuk>m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bGlobalLimitToBound)
					uuk = min(m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV], max(m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV], uuk));
				else
					uuk = m_param.m_noData;
			}
		}

		if ( /*iXval<0 &&*/ m_param.m_bGlobalMinMaxLimit && uuk > m_param.m_noData)
		{
			bool bOutside = uuk<m_param.m_globalMinLimit || uuk>m_param.m_globalMaxLimit;
			if (bOutside)
			{
				if (m_param.m_bGlobalMinMaxLimitToBound)
					uuk = min(m_param.m_globalMaxLimit, max(m_param.m_globalMinLimit, uuk));
				else
					uuk = m_param.m_noData;
			}
		}


		return uuk;
	}



	void CUniversalKriging::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

		CSugestedLagOptionNew op;

		int nbModel = m_param.m_variogramModel == CGridInterpolParam::BEST_VARIOGRAM ? CVariogram::NB_MODELS : 1;
		int nbStepLag = m_param.m_nbLags == 0 ? (int((op.m_nbLagMax - op.m_nbLagMin) / op.m_nbLagStep) + 1) : 1;
		int nbStepLagDist = m_param.m_lagDist == 0 ? (int((op.m_lagDistMax - op.m_lagDistMin) / op.m_lagDistStep) + 1) : 1;
		int nbDetrending = m_param.m_detrendingModel == CGridInterpolParam::BEST_DETRENDING ? 15 : 1;
		int nbExternalDrift = m_param.m_externalDrift == CGridInterpolParam::BEST_EXTERNAL_DRIFT ? 15 : 1;

		int nbParamters = nbStepLag*nbStepLagDist*nbDetrending*nbModel*nbExternalDrift;
		if (nbParamters > 1) //if we have to do optimization
		{


			double defaultLagDist = 0;
			if (m_param.m_lagDist == 0)  //compute default lag distance only if we need it
				defaultLagDist = CVariogram::GetSuggestedLag(*m_pPts);

			parameterset.resize(nbParamters, m_param);

			for (int i = 0; i < nbStepLag; i++)
			{
				for (int j = 0; j < nbStepLagDist; j++)
				{
					for (int k = 0; k < nbDetrending; k++)
					{
						for (int l = 0; l < nbModel; l++)//put model at the and for optimization
						{
							for (int m = 0; m < nbExternalDrift; m++)
							{

								int index = i*(nbStepLagDist*nbDetrending*nbModel*nbExternalDrift) + j*(nbDetrending*nbModel*nbExternalDrift) + k*(nbModel*nbExternalDrift) + l*(nbExternalDrift)+m;

								if (m_param.m_nbLags == 0)
									parameterset[index].m_nbLags = op.m_nbLagMin + op.m_nbLagStep*i;

								if (m_param.m_lagDist == 0)
									parameterset[index].m_lagDist = defaultLagDist*(op.m_lagDistMin + j*op.m_lagDistStep);

								if (m_param.m_detrendingModel == CGridInterpolParam::BEST_DETRENDING)
									parameterset[index].m_detrendingModel = k;

								if (m_param.m_variogramModel == CGridInterpolParam::BEST_VARIOGRAM)
									parameterset[index].m_variogramModel = l;

								if (m_param.m_externalDrift == CGridInterpolParam::BEST_EXTERNAL_DRIFT)
									parameterset[index].m_externalDrift = m;
							}//for all external drift
						}//for all model
					}//for all detrending
				}//for all lag distance
			}// for all nb lag

			//remove detrending and extern drift with expo if no expo present
			bool bHaveExpo = m_pPts->HaveExposition();
			if (!bHaveExpo)// a verifier
			{
				for (int i = (int)parameterset.size() - 1; i >= 0; i--)
				{
					if (parameterset[i].m_detrendingModel == 4 ||
						parameterset[i].m_detrendingModel == 7 ||
						parameterset[i].m_detrendingModel == 9 ||
						parameterset[i].m_detrendingModel == 10 ||
						parameterset[i].m_detrendingModel == 14 ||
						parameterset[i].m_externalDrift == 4 ||
						parameterset[i].m_externalDrift == 7 ||
						parameterset[i].m_externalDrift == 9 ||
						parameterset[i].m_externalDrift == 10 ||
						parameterset[i].m_externalDrift == 14)
						parameterset.erase(parameterset.begin() + i);
				}
			}
		}//if nb param > 1

	}

	string CUniversalKriging::GetFeedbackBestParam()const
	{
		string str;
		int varPrint[5] = { CGridInterpolParam::VARIOGRAM_MODEL, CGridInterpolParam::NB_LAGS, CGridInterpolParam::LAG_DISTANCE, CGridInterpolParam::DETRENDING_MODEL, CGridInterpolParam::EXTERNAL_DRIFT };

		for (int i = 0; i < 5; i++)
		{
			string tmp = string(CGridInterpolParam::GetMemberName(varPrint[i])) + " = " + m_param.GetMember(varPrint[i]);
			if (i == 0)
				tmp += string(" \"") + m_pVariogram->GetModelName() + "\"\t(R² = " + ToString(m_pVariogram->GetR2()) + ")";
			if (i == 3)
				tmp += " (" + m_pVariogram->GetDetrending().GetEquation() + ")";
			if (i == 4)
				tmp += " (" + m_externalDrift.GetTerms() + ")";
			str += tmp + "\n";
		}

		str += FormatA("Nugget = \t%lf\nSill =\t%lf\nRange = \t%lf", m_pVariogram->GetNugget(), m_pVariogram->GetSill(), m_pVariogram->GetRange());

		return str;
	}

	string CUniversalKriging::GetFeedbackOnOptimisation(const CGridInterpolParamVector& parameterset, const std::vector<double>& optimisationR²)const
	{
		string str;

		return str;
	}

	int CUniversalKriging::GetMDT()const
	{
		if (m_p.m_bSK)
			return 0;

		//int mdt = 0;

		//if (m_param.m_detrendingModel != NO_DETRENDING)
			//mdt += m_pVariogram->GetDetrending().size();
			
		//if( m_p.bExternDrift )
			//NO_EXTERNAL_DRIFT

		double mdt = m_externalDrift.size();

		return  mdt + 1;//+1 for stabilization
	}


	double CUniversalKriging::GetOptimizedR²()const
	{
		double XValR² = CGridInterpolBase::GetOptimizedR²();
		double varioR² = m_pVariogram->GetR2();
		double R² = XValR² * 3 / 4 + varioR² / 4;
		return R²;

	}

	//************************************************************************************
	//CDiscretisation
	CDiscretisation::CDiscretisation()
	{
		Reset();
	}

	void CDiscretisation::Reset()
	{
		clear();
		cbb = 0;
	}

	void CDiscretisation::Initialization(const CGridPoint& pt, const CVariogram& variogram, const CParamUK& p, bool bFillNugget)
	{
		ASSERT(p.m_nxdis >= 1 && p.m_nydis >= 1);
		resize(p.m_nxdis * p.m_nydis);

		double xdis = p.m_xsiz / p.m_nxdis;
		double ydis = p.m_ysiz / p.m_nydis;

		int i = 0;
		for (int ix = 0; ix < p.m_nxdis; ix++)
		{
			for (int iy = 0; iy < p.m_nydis; iy++)
			{
				at(i) = pt;
				at(i).m_x += (ix / (p.m_nxdis + 1) - 0.5)*p.m_xsiz;
				at(i).m_y += (iy / (p.m_nydis + 1) - 0.5)*p.m_ysiz;
				at(i).m_event = -999;//?????

				i++;
			}
		}

		// Initialize accumulators: 
		cbb = 0;

		for (int i = 0; i < size(); i++)
		{
			for (int j = 0; j < size(); j++)
			{
				double cov = variogram.cova3(at(i), at(j));

				if ((i == j) && (size() > 1 || bFillNugget))
					cov -= variogram.GetNugget();

				cbb += cov;
			}
		}
		cbb /= (size()*size());


	}

}