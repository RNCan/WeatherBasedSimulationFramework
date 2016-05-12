//*********************************************************************
// File: Generations.h
//
// Class: CGenerationResult: hold result for one generation
//        CGenerationVector:  hold result for all generation
//
// Descrition: CGenerationVector implement simulator able to 
//             create a vector of generations 
//
//************** MODIFICATIONS  LOG ********************
// 01/09/2003   Jacques Regniere    Creation
// 27/10/2003   Rémi Saint-Amant    Reengineering
// 30/10/2007	Rémi Saint-Amant    Integration with all MPB models
//*********************************************************************
#pragma once



#include <crtdbg.h>
#include <vector>
#include "../MPBDevRates.h"


namespace WBSF
{

	class CMPBDevelopmentVector;

	//**********************************************************
	//CGenerationResult

	enum { PEAK_OVIPOSITION = OVIPOSITING_ADULT + 1, NB_GENERATION_STAGES };

	class CGenerationResult
	{
	public:

		CGenerationResult();

		size_t m_medianDate[NB_GENERATION_STAGES]; //Warning: creation; not emergence
	};


	//**********************************************************
	//CGenerationVector

	class CGenerationVector : public std::vector<CGenerationResult>
	{
	public:

		bool SimulateGeneration(int nbGen, int dayStartIn, const CMPBDevelopmentVector& devRates);
		bool GetStabilityFlag(int minOvipDate, int maxOvipDate)const;
		double GetMeanGenerationTime()const;
		bool Save(const char* name);

		//median emergence date (ie s+1)
		size_t m_d_e(size_t n, size_t s)const
		{
			_ASSERTE(n < size());
			_ASSERTE(s < NB_GENERATION_STAGES);
			return at(n).m_medianDate[s + 1];
		}
		//median emergence date in the first year (ie s+1, modulo 365)
		size_t m_d_e2(size_t n, size_t s)const
		{
			return m_d_e(n, s) % 365;
		}

		//median date
		size_t& m_d(size_t n, size_t s)
		{
			_ASSERTE(n < size());
			_ASSERTE(s < NB_GENERATION_STAGES);
			return at(n).m_medianDate[s];
		}
		//median date
		size_t m_d(size_t n, size_t s)const
		{
			_ASSERTE(n < size());
			_ASSERTE(s < NB_GENERATION_STAGES);
			return at(n).m_medianDate[s];
		}

		size_t p_s(size_t n, size_t t)const
		{
			_ASSERTE(n < size());
			_ASSERTE(t >= 0 && t < 2);
			return t == 0 ? m_d(n, EGG) : m_d_e2(n, OVIPOSITING_ADULT);
		}

		double y_p_g(size_t n)const
		{
			_ASSERTE(n < size());
			return (m_d_e(n, OVIPOSITING_ADULT) - m_d(n, EGG)) / 365.0;
		}
		size_t GetNbDaysPerGeneration(size_t n)const
		{
			_ASSERTE(n < size());
			return m_d(n, PEAK_OVIPOSITION) - m_d(n, EGG);
		}
	};


	//*******************************************************
	//CAccumulator
	class CAccumulatorData
	{
	public:

		enum TP { P_LOGAN, P_LOGAN2b, P_SAFRANYIK, P_HYBRID, P_SAFRANYIK_P3P4, P_HYBRID_CT, NB_P };


		CAccumulatorData()
		{
			m_DDHatch = 0;
			m_DDGen = 0;
			m_lowestMinimum = 0;
			m_meanMaxAugust = 0;
			m_totalPrecip = 0;
			m_waterDeficit = 0;
			m_precAMJ = 0;
			m_stabilityFlag = false;
			m_S = 0; //Cold Tolerance
		}

		bool EvaluateP(int model, double meanPpt)const;

		double m_DDHatch;
		double m_DDGen;
		double m_lowestMinimum;
		double m_meanMaxAugust;
		double m_totalPrecip;
		double m_waterDeficit;
		double m_precAMJ;
		bool   m_stabilityFlag;
		double m_S;

	};


	typedef std::vector<CAccumulatorData> CAccumulatorVector;

	class CAccumulator : public CAccumulatorVector
	{
	public:
		enum TP {
			P_LOGAN = CAccumulatorData::P_LOGAN,
			P_LOGAN2b = CAccumulatorData::P_LOGAN2b,
			P_SAFRANYIK = CAccumulatorData::P_SAFRANYIK,
			P_HYBRID = CAccumulatorData::P_HYBRID,
			P_SAFRANYIK_P3P4 = CAccumulatorData::P_SAFRANYIK_P3P4,
			P_HYBRID_CT = CAccumulatorData::P_HYBRID_CT,
			NB_P
		};

		CAccumulator(size_t n = 3);
		void Reset();

		void ComputeMeanP_Y1_Y2();
		double GetProbability(int model, short y0, short runLength);
		double GetPsurv(short curY, short runLength)const;


		//	short GetNbYear(){return m_nbYears;}
		//	double GetProbability(){return m_F;}

		double GetMeanPrecipitation(){ return m_meanP; }
		double GetAverageWaterDefecit(){ return m_X2; }
		double GetCVPrecipitation(){ return m_X1; }
		double GetSqrtY1Y2()const;

	private:

		double MeanP(int model, short y, short runLength)const;
		bool EvaluateP(int model, int y)const;

		double ComputeMeanPrecipitation();
		double ComputeCVPrecipitation();
		double ComputeAverageWaterDefecit();

		double m_meanP;
		double m_X1;
		double m_X2;

		double m_Y1;
		double m_Y2;

		//parameter
		size_t m_n;

		static double GetY1(double X1);
		static double GetY2(double X2);

	};



}