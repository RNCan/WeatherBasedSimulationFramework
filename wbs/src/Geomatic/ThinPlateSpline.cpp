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
#include <math.h>

#define BOOST_UBLAS_TYPE_CHECK 0
#include <boost\crc.hpp>
#include "TPSInterpolate.hpp"

#include "Basic/UtilStd.h"
#include "Basic/openMP.h"
#include "Geomatic/ThinPlateSpline.h"
#include "KMeanLocal\KMlocal.h" 

using namespace std;

namespace WBSF
{



	typedef ThinPlateSpline<3, 1> Spline3;
	typedef ThinPlateSpline<4, 1> Spline4;
	typedef ThinPlateSpline<5, 1> Spline5;

	class MySpline : public Spline4
	{
	public:

		MySpline(std::vector<boost::array<double, 4 >>& p, std::vector<boost::array<double, 1> >& v) :Spline4(p, v)
		{}

	};

	//**********************************************************************
	//CThinPlateSpline
	CThinPlateSpline::CThinPlateSpline()
	{
		m_pObject = NULL;
		m_nbDim = 0;
		Reset();

		InitializeCriticalSection(&CS);
	}

	CThinPlateSpline::~CThinPlateSpline()
	{
		DeleteCriticalSection(&CS);
		Reset();
	}

	void CThinPlateSpline::Reset()
	{
		m_inc = 1;
		m_bClustered = false;

		if (m_pObject)
		{
			int t3 = sizeof(Spline3);
			int t4 = sizeof(Spline4);
			int t5 = sizeof(Spline5);
			switch (m_nbDim)
			{
			case 3: delete (Spline3*)m_pObject; break;
			case 4: delete (Spline4*)m_pObject; break;
			case 5: delete (Spline5*)m_pObject; break;
			default: ASSERT(false);
			}

			m_pObject = NULL;
			m_nbDim = 0;
		}

		for (key_to_value_type::iterator it = m_splinMap.begin(); it != m_splinMap.end(); it++)
		{
			delete it->second.first;
		}

		m_splinMap.clear();
		m_keyTracker.clear();
	}

	//template <int T>
	//void* NewSpline(int nbDim, std::vector<boost::array<double, T> >& positions, std::vector<boost::array<double, 1> >& values)
	//{
	//	//void* pObject=NULL;
	//
	//	//switch( T )
	//	//{
	//	//case 3: pObject = new Spline3(positions, values); break;
	//	//case 4: pObject = new Spline4(positions, values); break;
	//	//case 5: pObject = new Spline5(positions, values); break;
	//	//default: ASSERT(false);
	//	//}
	//
	//	//return new ThinPlateSpline<T,1>(positions, values);
	//	return NULL;
	////	return pObject;
	//}
	//static CRITICAL_SECTION CS = {NULL,0,0,NULL,NULL,0};

	void CThinPlateSpline::GetClusterNode(int nbCluster, const CPrePostTransfo& transfo, CGridPointVector& pointIn, CGridPointVector& pointOut, float noData)
	{

		KMterm	term(100, 0, 0, 0,		// run for 100 stages
			0.10,			// min consec RDL
			0.10,			// min accum RDL
			3,			// max run stages
			0.50,			// init. prob. of acceptance
			100,			// temp. run length
			0.95);			// temp. reduction factor

		//	term.setAbsMaxTotStage(stages);		// set number of stages
		bool bGeographic = IsGeographic(pointIn.GetPrjID());
		//bool bGeographic = pointIn.m_bGeographic;
		bool bHaveExposition = pointIn.HaveExposition();
		int nbDim = bHaveExposition ? bGeographic ? 6 : 5 : bGeographic ? 5 : 4;
		CStatisticVector stats(nbDim);
		KMdata dataPts(nbDim, (int)pointIn.size());		// allocate data storage
		for (int i = 0; i < pointIn.size(); i++)
		{
			const CGridPoint& ptTmp = pointIn[i];
			//dataPts[i][0] = ptTmp(0);
			//dataPts[i][1] = ptTmp(1);
			//dataPts[i][2] = ptTmp(2);
			//dataPts[i][3] = ptTmp(3);

			//if( bHaveExposition )
			//	dataPts[i][4] = ptTmp(4);

			for (int j = 0; j < nbDim - 1; j++)
			{
				dataPts[i][j] = bGeographic ? ptTmp(j) : ptTmp[j];
				stats[j] += dataPts[i][j];
			}

			dataPts[i][nbDim - 1] = transfo.Transform(ptTmp.m_event);
			stats[nbDim - 1] += dataPts[i][nbDim - 1];
		}

		//bool bStandardize= true;
		//if( bStandardize )
		//{
		for (int i = 0; i < dataPts.getNPts(); i++)
		{
			for (int j = 0; j < dataPts.getDim(); j++)
			{
				dataPts[i][j] = (dataPts[i][j] - stats[j][MEAN]) / stats[j][STD_DEV];
			}
		}
		//}

		dataPts.buildKcTree();			// build filtering structure

		KMfilterCenters ctrs(nbCluster, dataPts);		// allocate centers

		//CTimer timer(true);
		// run each of the algorithms
		//cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
		KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
		ctrs = kmLloyds.execute();			// execute
		//timer.Stop();   
		//printSummary("Lloyds",kmLloyds, dataPts, ctrs);	// print summary
		//cout << timer.Elapsed() << " s" << endl;

		/*	timer.Start(true);
			cout << "\nExecuting Clustering Algorithm: Swap\n";
			KMlocalSwap kmSwap(ctrs, term);		// Swap heuristic
			ctrs = kmSwap.execute();
			timer.Stop();
			printSummary("Swap", kmSwap, dataPts, ctrs);
			cout << timer.Elapsed() << " s" << endl;

			timer.Start(true);
			cout << "\nExecuting Clustering Algorithm: EZ-Hybrid\n";
			KMlocalEZ_Hybrid kmEZ_Hybrid(ctrs, term);	// EZ-Hybrid heuristic
			ctrs = kmEZ_Hybrid.execute();
			timer.Stop();
			printSummary("EZ-Hybrid", kmEZ_Hybrid, dataPts, ctrs);
			cout << timer.Elapsed() << " s" << endl;

			timer.Start(true);
			cout << "\nExecuting Clustering Algorithm: Hybrid\n";
			KMlocalHybrid kmHybrid(ctrs, term);		// Hybrid heuristic
			ctrs = kmHybrid.execute();
			timer.Stop();
			printSummary("Hybrid", kmHybrid, dataPts, ctrs);
			cout << timer.Elapsed() << " s" << endl;
			*/



		pointOut.resize(ctrs.getK());
		//pointOut.m_bGeographic = bGeographic;
		KMcenterArray clusterPts = ctrs.getCtrPts();

		for (int i = 0; i < ctrs.getK(); i++)
		{
			CGridPoint& ptTmp = pointOut[i];
			if (bGeographic)
			{
				ptTmp.SetXY(dataPts[i][0] * stats[0][STD_DEV] + stats[0][MEAN], dataPts[i][1] * stats[1][STD_DEV] + stats[1][MEAN], dataPts[i][2] * stats[2][STD_DEV] + stats[2][MEAN]);
				ptTmp.m_z = dataPts[i][3] * stats[3][STD_DEV] + stats[3][MEAN];
			}
			else
			{
				ptTmp.m_x = clusterPts[i][0] * stats[0][STD_DEV] + stats[0][MEAN];
				ptTmp.m_y = clusterPts[i][1] * stats[1][STD_DEV] + stats[1][MEAN];
				ptTmp.m_z = clusterPts[i][2] * stats[2][STD_DEV] + stats[2][MEAN];
			}

			if (bHaveExposition)
			{
				if (bGeographic)
					ptTmp.SetExposition(ptTmp.m_y, dataPts[i][nbDim - 2] * stats[nbDim - 2][STD_DEV] + stats[nbDim - 2][MEAN]);
				else//we don't know latitude, use 0 
					ptTmp.SetExposition(0, dataPts[i][nbDim - 2] * stats[nbDim - 2][STD_DEV] + stats[nbDim - 2][MEAN]);
			}

			ptTmp.m_event = transfo.InvertTransform(dataPts[i][nbDim - 1] * stats[nbDim - 1][STD_DEV] + stats[nbDim - 1][MEAN], noData);
		}
	}


	ERMsg CThinPlateSpline::Initialization()
	{
		ERMsg msg = CGridInterpolBase::Initialization();

		if (m_param.m_TPSType == CGridInterpolParam::TPS_REGIONAL)
		{
			//m_ANNSearch.Init(m_pPts);
			//double lastCheckSum = m_pPts->GetCheckSum();

			//ANN_SEARCH_CACHE.Enter();
			//CANNSearchCache::iterator it = ANN_SEARCH_CACHE.find(lastCheckSum);
			//if( it == ANN_SEARCH_CACHE.end() )
			//{
			if (!m_bInit)
			{
				m_pANNSearch = make_unique<CANNSearch>();
				m_pANNSearch->Init(m_pPts);
				m_bInit = true;
			}
			//ANN_SEARCH_CACHE[lastCheckSum].reset( pANNSearch );

			//it = ANN_SEARCH_CACHE.find(lastCheckSum);
			//}

			//m_pANNSearch = pANNSearch;
			//m_pANNSearch = it->second.get();
			//	ANN_SEARCH_CACHE.Leave();

		}
		else
		{
			size_t nbPoint = m_param.m_nbPoints;
			if (nbPoint == 0)
				nbPoint = m_pPts->size();

			CGridPointVector* pPts = m_pPts.get();

			//if there are more than 2x the number of point
			//regroup point with Kmean cluster
			CGridPointVector clusterPoint;

			//if( nbPoint>m_pPts->size()*3/4 )
			if (nbPoint == pPts->size() || m_param.m_TPSType == CGridInterpolParam::TPS_GLOBAL)
			{
				m_bClustered = false;
				//nbPoint = Max(1, Min( nbPoint, m_pPts->size()-100));//keep at least 100 pts to make x validation
				nbPoint = max(size_t(1), min(nbPoint, m_pPts->size()));
			}
			else
			{
				ASSERT(m_param.m_TPSType == CGridInterpolParam::TPS_GLOBAL_WITH_CLUSTER);
				m_bClustered = true;
				GetClusterNode((int)nbPoint, m_prePostTransfo, *m_pPts, clusterPoint, m_param.m_noData);
				pPts = &clusterPoint;
			}

			m_inc = max(1.0, (double)pPts->size() / nbPoint);

			bool bGeographic = IsGeographic(pPts->GetPrjID());
			bool bHaveExposition = pPts->HaveExposition();
			int nbDim = bHaveExposition ? bGeographic ? 5 : 4 : bGeographic ? 4 : 3;

			std::vector<boost::array<double, 3> > positions3(nbDim == 3 ? nbPoint : 0);
			std::vector<boost::array<double, 4> > positions4(nbDim == 4 ? nbPoint : 0);
			std::vector<boost::array<double, 5> > positions5(nbDim == 5 ? nbPoint : 0);
			std::vector<boost::array<double, 1> > values(nbPoint);



			for (int i = 0, ii = 0; ii < pPts->size(); ++i, ii = int(i*m_inc))
			{

				const CGridPoint& ptTmp = pPts->at(int(ii));
				for (int k = 0; k < nbDim; k++)
				{
					if (nbDim == 3)
						positions3[i][k] = bGeographic ? ptTmp(k) : ptTmp[k];
					else if (nbDim == 4)
						positions4[i][k] = bGeographic ? ptTmp(k) : ptTmp[k];
					else if (nbDim == 5)
						positions5[i][k] = bGeographic ? ptTmp(k) : ptTmp[k];
				}

				values[i][0] = m_prePostTransfo.Transform(ptTmp.m_event);
			}


			try
			{
				m_nbDim = nbDim;
				if (nbDim == 3)
					m_pObject = (void*) new Spline3(positions3, values);
				else if (nbDim == 4)
					m_pObject = (void*) new Spline4(positions4, values);
				else if (nbDim == 5)
					m_pObject = (void*) new Spline5(positions5, values);
			}
			catch (...)
			{
				msg.ajoute("Unable to create Thin Plate Splin");
			}
		}

		return msg;
	}

	//typedef boost::crc_optimal<32, 0x04C11DB7>  fast_crc_type;

	int GetCheckSum(const CGridPointResultVector& result)
	{
		boost::crc_32_type crc;
		for (size_t i = 0; i < result.size(); i++)
			crc.process_bytes(&result[i].i, sizeof(result[i].i));

		return crc.checksum();
	}

	double CThinPlateSpline::Evaluate(const CGridPoint& pt, int iXval)const
	{
		double value = m_param.m_noData;


		if (m_param.m_TPSType == CGridInterpolParam::TPS_REGIONAL)
		{
			CThinPlateSpline& me = const_cast<CThinPlateSpline&>(*this);

			int nbPoint = int(m_pANNSearch->GetSize() - (iXval >= 0 ? 1 : 0));
			if (m_param.m_nbPoints > 0)
				nbPoint = int(min(m_pANNSearch->GetSize(), m_param.m_nbPoints + (iXval >= 0 ? 1 : 0)));

			// Find the nearest samples: 
			CGridPointResultVector result;

			//int nbPoint = m_param.m_nbPoints + (iXval>=0?1:0);
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
			else
			{
				if (result[0].d > m_param.m_maxDistance)
					return m_param.m_noData;
			}


			int checkSum = GetCheckSum(result);
			//omp_get_max_threads();

			EnterCriticalSection(&me.CS);
			key_to_value_type::const_iterator it = m_splinMap.find(checkSum);
			if (it != m_splinMap.end())
			{
				// We do have it:

				// Update access record by moving 
				// accessed key to back of list
				me.m_keyTracker.splice(me.m_keyTracker.end(), me.m_keyTracker, (*it).second.second);

				boost::array<double, 4> curPos;
				curPos[0] = pt(0);
				curPos[1] = pt(1);
				curPos[2] = pt(2);
				curPos[3] = pt(3) * 100;

				value = it->second.first->interpolate(curPos)[0];
			}
			LeaveCriticalSection(&me.CS);

			if (it == m_splinMap.end())
			{

				//create a new splin
				std::vector<boost::array<double, 4> > positions(result.size());
				std::vector<boost::array<double, 1> > values(result.size());


				for (size_t i = 0; i < result.size(); i++)
				{
					const CGridPoint& ptTmp = m_pPts->at(i);
					positions[i][0] = ptTmp(0);
					positions[i][1] = ptTmp(1);
					positions[i][2] = ptTmp(2);
					positions[i][3] = ptTmp(3) * 100;

					values[i][0] = m_prePostTransfo.Transform(ptTmp.m_event);
				}



				try
				{

					MySpline* pSpline = new MySpline(positions, values);

					boost::array<double, 4> curPos;
					curPos[0] = pt(0);
					curPos[1] = pt(1);
					curPos[2] = pt(2);
					curPos[3] = pt(3) * 100;

					value = pSpline->interpolate(curPos)[0];

					EnterCriticalSection(&me.CS);

					if (m_splinMap.size() > 1000)
					{
						// Assert method is never called when cache is empty
						assert(!m_keyTracker.empty());

						// Identify least recently used key
						const key_to_value_type::iterator it = me.m_splinMap.find(m_keyTracker.front());
						assert(it != m_splinMap.end());

						// Erase both elements to completely purge record
						delete it->second.first;
						me.m_splinMap.erase(it);
						me.m_keyTracker.pop_front();

					}

					key_tracker_type::iterator it = me.m_keyTracker.insert(m_keyTracker.end(), checkSum);
					me.m_splinMap.insert(std::make_pair(checkSum, std::make_pair(pSpline, it)));

					LeaveCriticalSection(&me.CS);
				}
				catch (...)
				{
					//return value;
				}
			}
		}
		else
		{

			if (iXval >= 0 && !m_bClustered)
			{
				int l = (int)ceil((iXval) / m_inc);
				if (int(l*m_inc) == iXval)
					return value;
			}

			boost::array<double, 3> curPos3;
			boost::array<double, 4> curPos4;
			boost::array<double, 5> curPos5;
			bool bGeographic = IsGeographic(m_pPts->GetPrjID());

			for (int k = 0; k < m_nbDim; k++)
			{
				if (m_nbDim == 3)
					curPos3[k] = bGeographic ? pt(k) : pt[k];
				else if (m_nbDim == 4)
					curPos4[k] = bGeographic ? pt(k) : pt[k];
				else if (m_nbDim == 5)
					curPos5[k] = bGeographic ? pt(k) : pt[k];
			}


			if (m_nbDim == 3)
				value = m_prePostTransfo.InvertTransform( ((Spline3*)m_pObject)->interpolate(curPos3)[0], m_param.m_noData);
			else if (m_nbDim == 4)
				value = m_prePostTransfo.InvertTransform(((Spline4*)m_pObject)->interpolate(curPos4)[0], m_param.m_noData);
			else if (m_nbDim == 5)
				value = m_prePostTransfo.InvertTransform(((Spline5*)m_pObject)->interpolate(curPos5)[0], m_param.m_noData);
		}


		if ( /*iXval<0 &&*/ m_param.m_bGlobalLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV] || value>m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bGlobalLimitToBound)
					value = min(m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV], max(m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV], value));
				else
					value = m_param.m_noData;
			}
		}

		if ( /*iXval<0 &&*/ m_param.m_bGlobalMinMaxLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_param.m_globalMinLimit || value>m_param.m_globalMaxLimit;
			if (bOutside)
			{
				if (m_param.m_bGlobalMinMaxLimitToBound)
					value = min(m_param.m_globalMaxLimit, max(m_param.m_globalMinLimit, value));
				else
					value = m_param.m_noData;
			}
		}

		return value;

	}



	void CThinPlateSpline::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

		/*CSugestedLagOptionNew op;

		int nbModel = m_param.m_variogramModel==CGridInterpolParam::BEST_VARIOGRAM?CVariogramNew::NB_MODEL:1;
		int nbStepLag = m_param.m_nbLags==0?(int((op.m_nbLagMax-op.m_nbLagMin)/op.m_nbLagStep) + 1):1;
		int nbStepLagDist = m_param.m_lagDist==0?(int((op.m_lagDistMax-op.m_lagDistMin)/op.m_lagDistStep) + 1):1;
		int nbDetrending = m_param.m_detrendingModel==CGridInterpolParam::BEST_DETRENDING?15:1;
		int nbExternalDrift = m_param.m_externalDrift==CGridInterpolParam::BEST_EXTERNAL_DRIFT?15:1;

		int nbParamters = nbStepLag*nbStepLagDist*nbDetrending*nbModel*nbExternalDrift;
		if( nbParamters > 1) //if we have to do optimisation
		{


		double defaultLagDist=0;
		if( m_param.m_lagDist==0 )  //compute default lag distance only if we need it
		defaultLagDist=CVariogramNew::GetSuggestedLag(*m_pPts);

		parameterset.resize(nbParamters, m_param);

		for(int i=0; i<nbStepLag; i++)
		{
		for(int j=0; j<nbStepLagDist; j++)
		{
		for(int k=0; k<nbDetrending; k++)
		{
		for(int l=0; l<nbModel; l++)//put model at the and for optimization
		{
		for(int m=0; m<nbExternalDrift; m++)//put model at the and for optimization
		{

		int index = i*(nbStepLag*nbDetrending*nbModel*nbExternalDrift) + j*(nbDetrending*nbModel*nbExternalDrift) + k*(nbModel*nbExternalDrift) + l*(nbExternalDrift) + m;

		if( m_param.m_nbLags==0)
		parameterset[index].m_nbLags = op.m_nbLagMin + op.m_nbLagStep*i;

		if( m_param.m_lagDist==0 )
		parameterset[index].m_lagDist = defaultLagDist*(op.m_lagDistMin+j*op.m_lagDistStep);

		if( m_param.m_detrendingModel==CGridInterpolParam::BEST_DETRENDING )
		parameterset[index].m_detrendingModel = k;

		if( m_param.m_variogramModel==CGridInterpolParam::BEST_VARIOGRAM)
		parameterset[index].m_variogramModel = l;

		if( m_param.m_externalDrift==CGridInterpolParam::BEST_EXTERNAL_DRIFT)
		parameterset[index].m_externalDrift = m;
		}//for all external drift
		}//for all model
		}//for all detrending
		}//for all lag distance
		}// for all nb lag

		//remove detrend and extern drift with expo if no expo present
		bool bHaveExpo = m_pPts->HaveExposition();
		if( !bHaveExpo)// a vérifier ???
		{
		for(int i=parameterset.size()-1; i>=0; i--)
		{
		if( parameterset[i].m_detrendingModel == 4 ||
		parameterset[i].m_detrendingModel == 7 ||
		parameterset[i].m_detrendingModel == 9 ||
		parameterset[i].m_detrendingModel == 10 ||
		parameterset[i].m_detrendingModel == 14 ||
		parameterset[i].m_externalDrift == 4 ||
		parameterset[i].m_externalDrift == 7 ||
		parameterset[i].m_externalDrift == 9 ||
		parameterset[i].m_externalDrift == 10 ||
		parameterset[i].m_externalDrift == 14 )
		parameterset.erase(parameterset.begin()+i);
		}
		}
		}//if nb param > 1
		*/
	}

	string CThinPlateSpline::GetFeedbackBestParam()const
	{
		string str;
		/*int varPrint[5] = {CGridInterpolParam::VARIOGRAM_MODEL, CGridInterpolParam::NB_LAGS, CGridInterpolParam::LAG_DISTANCE, CGridInterpolParam::DETRENDING_MODEL, CGridInterpolParam::EXTERNAL_DRIFT};
		//str.Format( "%s = %s (R² = %.4lf)\n", CGridInterpolParam::GetMemberName(varPrint[0]),CVariogramNew::GetModelName(m_param.m_variogramModel), m_variogram.GetR2());
		for(int i=0; i<5; i++)
		{
		CStdString tmp;
		tmp.Format( "%s = %s", CGridInterpolParam::GetMemberName(varPrint[i]),m_param.GetMember(varPrint[i]));
		if( i==0 )
		tmp += CStdString(" \"") + m_variogram.GetModelName() + "\"\t(R² = " + ToString(m_variogram.GetR2())+ ")";
		if( i==3)
		tmp += " (" + m_variogram.GetDetrending().GetEquation() + ")";
		if( i==4)
		tmp += " (" + m_externalDrift.GetTerms() + ")";
		str += tmp+"\n";
		}*/

		return str;
	}

	double CThinPlateSpline::GetOptimizedR²()const
	{
		//	double XValR² = CGridInterpolBase::GetOptimizedR²();
		//  double varioR² = m_variogram.GetR2();
		//    double R² = XValR²*3/4 + varioR²/4; 
		return 0;

	}

}