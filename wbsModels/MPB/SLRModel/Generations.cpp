//*********************************************************************
// File: Generations.cpp
//
// Class: CGenerationResult: hold result for one generation
//        CGenerationVector:  hold result for all generation
//        CGrowingChamber:   Simulate generation
//
//************** MODIFICATIONS  LOG ********************
// 01/09/2003   Jacques Regniere    Creation
// 27/10/2003   Rémi Saint-Amant    Reengineering
// 30/10/2007	Rémi Saint-Amant    Integration with all MPB models, removing NEWMAP
//									New Algo, take care of the new stage 
//*********************************************************************
#include <math.h>
#include <stdlib.h>
#include "Generations.h"
#include "../MPBDevRates.h"
#include "../DevelopementVector.h"


using namespace std;

namespace WBSF
{


	//**********************************************************************
	// CGenerationResult

	CGenerationResult::CGenerationResult()
	{
		for (size_t i = 0; i < NB_GENERATION_STAGES; i++)
			m_medianDate[i] = 0;
	}

	//**********************************************************************
	//CGenerationVector


	//*****************************************************
	// GetMeanGenerationTime: Compute the mean generation time
	// 
	// Output 
	//  return: mean generation time.
	//*****************************************************
	double CGenerationVector::GetMeanGenerationTime()const
	{
		double sum = 0;
		for (CGenerationVector::size_type i = 0; i < size(); i++)
		{
			sum += y_p_g(i);
		}

		return sum / size();
	}


	//*****************************************************
	// GetStabilityFlag : Evaluate criterious
	//
	// Input :
	//  minOvipDate and maxOvipDate: biologically feasible min and max ovip date
	//  
	// Output 
	//  return 0: unstable seasonality; 1: stable seasonality
	//*****************************************************
	bool CGenerationVector::GetStabilityFlag(int minOvipDate, int maxOvipDate)const
	{
		_ASSERTE(size() > 1);
		_ASSERTE(minOvipDate >= 0 && minOvipDate < 366);
		_ASSERTE(maxOvipDate >= 0 && maxOvipDate < 366);

		//apply stability criteria
		bool criteria[3] = { 0 }; //stability criteria

		int lastGen = size() - 1;


		//if(int(y_p_g(lastGen))==1) (there may be a problem with real-int accuracy here)
		//if(m_d(lastGen,PEAK_OVIPOSITION)-m_d(lastGen,EGG)==365) 
		if (int(GetNbDaysPerGeneration(lastGen)) == 365)
			criteria[0] = true; //univoltine 


		if (m_d_e(lastGen, OVIPOSITING_ADULT) == m_d_e(lastGen - 1, OVIPOSITING_ADULT))
			criteria[1] = true; //existence of an attractor emergence date (constant)


		//median oviposition date (temporary storage)
		int medOvipDate = (m_d_e(lastGen, OVIPOSITING_ADULT)) % 365;
		_ASSERTE(medOvipDate >= 0 && medOvipDate < 365);

		if (medOvipDate >= minOvipDate && medOvipDate <= maxOvipDate)
			criteria[2] = true; //oviposition date within user-defined bounds

		bool bStability = false;
		if (criteria[0] && criteria[1] && criteria[2])
			bStability = true;

		return bStability;
	}

	//*****************************************************
	// Save: Save the CGenerationVector to disk
	//
	// Input :
	//  filePath: file path
	//  
	// Output 
	//  return true: successful; false otherwise
	//*****************************************************
	bool CGenerationVector::Save(const char* filePath)
	{
		bool bRep = false;

		FILE* pFile = fopen(filePath, "w");
		if (pFile)
		{
			bRep = true;
			fprintf(pFile, "StartDay,EggEmerg,L1Emerg,L2Emerg,L3Emerg,L4Emerg,PUPEAEmerg,TeneralEmerg,BeginOviposition,PeakOviposition,Ovip (t-1),Ovip (t),Nb Year\n");

			for (CGenerationVector::size_type n = 0; n < size(); n++)
			{
				for (int s = 0; s < NB_GENERATION_STAGES; s++)
					fprintf(pFile, "%3d,", m_d(n, s) + 1);

				fprintf(pFile, "%3d,", m_d(n, EGG) + 1);
				fprintf(pFile, "%3d,", m_d_e2(n, OVIPOSITING_ADULT) + 1);
				fprintf(pFile, "%.5lf", y_p_g(n));

				fprintf(pFile, "\n");

			}
			fprintf(pFile, "meanGenerationTime = %15.7lf", GetMeanGenerationTime());

			fclose(pFile);
		}

		return bRep;
	}


	//**********************************************************************
	// CGrowingChamber


	//*****************************************************
	// SimulateGeneration: Simulate generations.
	//          
	// Input :
	//  devRates: developpement rates
	//  dayStart: initial oviposition date.
	//  nbGen: number of generations for the stability test. nbGen > 1.
	//                  
	// Output 
	//  generations: the object is initialised with the good number of unstable generation
	//  return: true if it's able to do nbGen geneations; false otherwise.
	//
	// Notes:
	//  these tree methods are use to simplify coding
	// m_d_e(gen, stade): it's the median day of emergence for a life stage and a generation, also
	// p_s(gen, t): phase space for a generation (0 = oviposition year t-1, 1 = year t)
	// y_p_g(gen): years per generation for a generation 
	//*****************************************************
	bool CGenerationVector::SimulateGeneration(int nbGen, int dayStart, const CMPBDevelopmentVector& devRates)
	{
		_ASSERTE(devRates.size() == 365 || devRates.size() == 366);
		_ASSERTE(dayStart >= 0 && dayStart < 366);
		_ASSERTE(nbGen > 1);

		reserve(nbGen);
		resize(1);

		for (int n = 0; n < nbGen; n++)
		{
			m_d(n, 0) = (n>0) ? (m_d(n - 1, PEAK_OVIPOSITION) % 365) : dayStart;

			for (int s = 0; s < NB_STAGES; s++)
			{
				double threshold = (s == OVIPOSITING_ADULT) ? 32 / 92.2 : 1;
				int nextStageDay = devRates.GetNextStageDay(m_d(n, s), s, threshold);
				if (nextStageDay == -1)
					return false;

				m_d(n, s + 1) = nextStageDay;
			}

			if (n > 0)
			{
				if (GetStabilityFlag(0, 365))//are we stable: we stop here
					break;
			}

			push_back(CGenerationResult());
		}

		return true;
	}

	//*****************************************************
	// GetNextStageDay : Find the day when the bug change of stage
	//          
	// Input :
	//  devRates : developement rate table
	//  firstDay : The first day in the current stage
	//  stage : The stage to evaluate
	//
	// Output 
	//  return: day of the next stage if sussesfukl. -1 otherwise.
	//*****************************************************
	/*int CGenerationVector::GetNextStageDay(const CMPBDevelopmentVector& devRates, int firstDay, int s)
	{
	_ASSERTE( devRates.size() == 365 || devRates.size() == 366);
	_ASSERTE( s>=0 || s<NB_GENERATION_STAGES);

	int index = -1;

	//Note: for the stability analysis, we need the MEDIAN egg, this happens at 32 cm of gallery. Thus,
	//Age of adult when median egg is laid is 32/92.2 = .347
	double threshold = (s==OVIPOSITING_ADULT)?32/92.2:1;

	//init the accumulation
	double sum=0;

	int maxDay = devRates.size()*2;//simulate on 2 years max

	//accumulate all values until reach the next stage
	for(int d=firstDay; d<maxDay; d++)
	{
	int jd = d%devRates.size();
	sum += devRates[jd][s];
	if( sum>=threshold)
	{
	index=d;
	break;
	}
	}

	_ASSERTE( index>=-1 && index<maxDay);

	return index;
	}
	*/

	//**************************************************************************
	//CAccumulatorData
	bool CAccumulatorData::EvaluateP(int model, double meanPpt)const
	{
		bool p = 0;

		bool pL = m_stabilityFlag;
		bool p1 = (m_DDHatch >= 305.56 && m_DDGen >= 833.33); //550 and 1500 F	
		bool p2 = m_lowestMinimum > -40;
		bool p3 = m_meanMaxAugust >= 18.3;  //[7] is August, 65 F
		bool p4 = m_precAMJ < meanPpt;


		switch (model)
		{
		case P_LOGAN:
		case P_LOGAN2b:			p = pL; break;
		case P_SAFRANYIK:		p = p1 && p2 && p3 && p4; break;
		case P_HYBRID:			p = pL && p2 && p3 && p4; break;
		case P_SAFRANYIK_P3P4:	p = p3 && p4; break;
		case P_HYBRID_CT:		p = pL && p3 && p4; break;
		default: _ASSERTE(false);
		}

		return p;
	}

	//**************************************************************************
	//CAccumulator


	CAccumulator::CAccumulator(int n)
	{
		m_n = n;
		Reset();
	}

	void CAccumulator::Reset()
	{
		m_X1 = 0;
		m_X2 = 0;

		m_Y1 = 0;
		m_Y2 = 0;
		m_meanP = 0;
	}

	void CAccumulator::ComputeMeanP_Y1_Y2()
	{
		m_meanP = ComputeMeanPrecipitation();
		m_X1 = ComputeCVPrecipitation();
		m_X2 = ComputeAverageWaterDefecit();
		m_Y1 = GetY1(m_X1);
		m_Y2 = GetY2(m_X2);
	}

	double CAccumulator::GetSqrtY1Y2()const
	{
		return sqrt(m_Y1*m_Y2);
	}

	bool CAccumulator::EvaluateP(int model, int y)const
	{
		ASSERT(y >= 0 && y < (int)size());

		bool p = false;

		switch (model)
		{
		case P_LOGAN: p = at(y).EvaluateP(model, m_meanP); break;
		case P_LOGAN2b:
		{
			p = at(y).EvaluateP(model, m_meanP);

			if (!p)//if p == 0 we need to count the length of the series
			{

				int nbZero = 1;


				//Get the length of zero
				for (int i = y + 1; i < (int)size() && nbZero < m_n; i++)
				{
					if (at(i).EvaluateP(model, m_meanP))
						break;
					else nbZero++;
				}
				for (int i = y - 1; i >= 0 && nbZero < m_n; i--)
				{
					if (at(i).EvaluateP(model, m_meanP))
						break;
					else nbZero++;
				}
				//p is true if the number of zero is lower then m_n
				p = nbZero < m_n;
			}
			break;
		}
		case P_SAFRANYIK:
		case P_HYBRID:
		case P_SAFRANYIK_P3P4:
		case P_HYBRID_CT:
		{
			bool Pm1 = (y>0) ? at(y - 1).EvaluateP(model, m_meanP) : false;
			bool P = at(y).EvaluateP(model, m_meanP);
			bool Pp1 = ((y + 1) < (short)size()) ? at(y + 1).EvaluateP(model, m_meanP) : false;

			p = ((Pm1&&P) || (P&&Pp1));

			break;
		}

		default: _ASSERTE(false);
		}

		return p;
	}

	double CAccumulator::MeanP(int model, short curY, short runLength)const
	{
		_ASSERTE(curY > 0);//never ask the first year

		CStatistic s;

		//if it's the first year, we skip it because we don't have CT
		short y0 = max(1, curY - runLength + 1);

		for (short y = y0; y <= curY; y++)
		{
			double p = EvaluateP(model, y) ? 1 : 0;

			if (model == P_HYBRID_CT)
				p *= at(y).m_S;

			s += p;
		}

		return s[MEAN];
	}



	double CAccumulator::GetPsurv(short curY, short runLength)const
	{
		short y0 = curY - runLength + 1;

		//if it's the first year, we skip it because we don't CT
		if (y0 == 0)
			y0++;

		CStatistic stat;
		//skip first year because we don't have Cold Tolerance (CT)
		for (short i = y0; i <= curY; i++)
		{
			stat += at(i).m_S;
		}

		return stat[MEAN];
	}

	double CAccumulator::ComputeMeanPrecipitation()
	{
		double mean = 0;
		for (short i = 0; i<short(size()); i++)
		{
			mean += at(i).m_precAMJ;
		}

		return mean / size();
	}


	double CAccumulator::ComputeCVPrecipitation()
	{
		double sum = 0;
		double sum2 = 0;
		for (short i = 0; i<short(size()); i++)
		{
			sum += at(i).m_precAMJ;
			sum2 += Square(at(i).m_precAMJ);
		}

		double var = (sum2 - Square(sum) / size()) / (size() - 1);
		double mean = sum / size();

		double CV = sqrt(var) / mean;

		return (CV);
	}

	double CAccumulator::ComputeAverageWaterDefecit()
	{
		double mean = 0;
		for (short i = 0; i < short(size()); i++)
			mean += at(i).m_waterDeficit;

		mean /= size();

		return mean;
	}

	double CAccumulator::GetY1(double X1)
	{
		double		Y1 = 0.2;
		if (X1 > 0.30) Y1 = 0.4;
		if (X1 > 0.35) Y1 = 0.7;
		if (X1 > 0.40) Y1 = 0.9;
		if (X1 > 0.45) Y1 = 1.0;

		return Y1;
	}

	double CAccumulator::GetY2(double X2)
	{
		double    Y2 = 0.2;
		if (X2 > 0) Y2 = 0.4;
		if (X2 > 4) Y2 = 0.7;
		if (X2 > 8) Y2 = 0.9;
		if (X2 > 12) Y2 = 1.0;

		return Y2;
	}

	double CAccumulator::GetProbability(int model, short y0, short runLength)
	{
		_ASSERTE(size() > 1);

		//Summarize the overall result:

		double p = 0;

		switch (model)
		{
		case P_LOGAN:		p = MeanP(P_LOGAN, y0, runLength); break;
		case P_LOGAN2b:		p = MeanP(P_LOGAN2b, y0, runLength); break;//2b call a new way to compute value
		case P_SAFRANYIK:		p = MeanP(P_SAFRANYIK, y0, runLength)*GetSqrtY1Y2(); break;
		case P_SAFRANYIK_P3P4:	p = MeanP(P_SAFRANYIK_P3P4, y0, runLength)*GetSqrtY1Y2(); break;
			//case P_COLD_TOLERANCE:	p=GetPsurv(y0, runLength); break;
		case P_HYBRID_CT:		p = MeanP(P_HYBRID_CT, y0, runLength)*GetSqrtY1Y2(); break;
		default: _ASSERTE(false);
		}


		return p;
	}

}