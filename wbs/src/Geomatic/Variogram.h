//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <memory>
#include <unordered_map>
#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Basic/GeoBasic.h"
#include "Geomatic/SpatialRegression.h"

namespace WBSF
{


	class CSugestedLagOptionNew
	{
	public:

		CSugestedLagOptionNew(int nbLagMin = 10, int nbLagMax = 40, int nbLagStep = 5, float lagDistMin = 0.5, float lagDistMax = 10.0, float lagDistStep = .5)
		{
			m_nbLagMin = nbLagMin;
			m_nbLagMax = nbLagMax;
			m_nbLagStep = nbLagStep;

			m_lagDistMin = lagDistMin;
			m_lagDistMax = lagDistMax;
			m_lagDistStep = lagDistStep;
		}
		void LoadDefaultCtrl();
			

		int m_nbLagMin;
		int m_nbLagMax;
		int m_nbLagStep;

		float m_lagDistMin;
		float m_lagDistMax;
		float m_lagDistStep;
	};


	//********************************************************************
	class CSugestedLagResultNew
	{
	public:

		CSugestedLagResultNew()
		{
			m_nbLags = 0;
			m_lagDist = 0;
			m_R2Variogram = 0;
			m_R2XValidation = 0;
			m_R2 = 0;
		}

		int m_nbLags;
		double m_lagDist;
		double m_R2Variogram;
		double m_R2XValidation;
		double m_R2;
		std::string m_method;
	};


	typedef std::vector<CSugestedLagResultNew> CSugestedLagResultVector;

	class CRotationMatrix
	{
	public:
		CRotationMatrix();
		void Reset();

		void Init(double ang1, double ang2, double ang3, double anis1, double anis2);
		double  sqdist(double x1, double y1, double z1, double x2, double y2, double z2)const;

		bool operator == (const CRotationMatrix& in)const;
		bool operator != (const CRotationMatrix& in)const{ return !operator==(in); }


		std::istream& operator << (std::istream& s) { for(size_t i=0; i<3; i++)for (size_t j = 0; j < 3; j++)s >> m_rotmat[i][j]; return s; }
		std::ostream& operator >> (std::ostream& s)const { for (size_t i = 0; i < 3; i++)for (size_t j = 0; j < 3; j++)s << m_rotmat[i][j]; return s; }
		friend std::istream& operator >>(std::istream& s, CRotationMatrix& p) { p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CRotationMatrix& p) { p >> s; return s; }

	protected:
		double m_rotmat[3][3];
	};

	//
	class CVariogramPredictor
	{
	public:

		bool operator == (const CVariogramPredictor& in)const
		{
			bool bEqual = true;
			if (m_np != in.m_np)bEqual = false;
			if (abs(m_lagDistance - in.m_lagDistance) > 0.00001) bEqual = false;
			if (abs(m_lagVariance - in.m_lagVariance) > 0.00001) bEqual = false;
			//edVariance - in.m_predictedVariance) > 0.00001) bEqual = false;

			return bEqual;
		}

		bool operator != (const CVariogramPredictor& in)const{ return !operator==(in); }

		
		std::istream& operator << (std::istream& s) { s >> m_np >> m_lagDistance >> m_lagVariance; return s; }
		std::ostream& operator >> (std::ostream& s)const { s << m_np << m_lagDistance << m_lagVariance; return s; }
		friend std::istream& operator >>(std::istream& s, CVariogramPredictor& p) { p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CVariogramPredictor& p) { p >> s; return s; }



		size_t	m_np;
		double	m_lagDistance;
		double	m_lagVariance;
		//double	m_predictedVariance;
	};

	class CVariogramPredictorVector : public std::vector<CVariogramPredictor>
	{
	public:
		
		std::istream& operator << (std::istream& s);
		std::ostream& operator >> (std::ostream& s)const;
		friend std::istream& operator >>(std::istream& s, CVariogramPredictorVector& p) { p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CVariogramPredictorVector& p) { p >> s; return s; }

	};

	typedef CGeoRegression CDetrending;



	class CVariogram
	{
	public:

		enum TModel { SPERICAL, EXPONENTIAL, GAUSSIAN, POWER, CUBIC, PENTASPHERICAL, SINE_HOLE_EFFECT, NB_MODELS };

		static double GetSuggestedLag(const CGridPointVector& pts);
		static const char* GetModelName(size_t model)	{ASSERT( model < NB_MODELS);return model < NB_MODELS?MODEL_NAME[model]:"";}
		static double   expfn(double x);
		static double   powfn(double x, double p);


		CVariogram();
		CVariogram(const CVariogram& in);
		~CVariogram();

		void Reset();
		CVariogram& operator =(const CVariogram& in);
		bool operator == (const CVariogram& in)const;
		bool operator != (const CVariogram& in)const{ return !operator==(in); }

		ERMsg CreateVariogram(const CGridPointVector& pts, const CPrePostTransfo& transfo, size_t model, int nLags, float dLag,
			const CDetrending& detrending = CDetrending(), const CRotationMatrix& rotmat = CRotationMatrix(), CCallback& callback = DEFAULT_CALLBACK);

		ERMsg Save(std::string filePath)const;

		size_t GetModel()const	{ ASSERT(m_model < NB_MODELS);	return m_model; }
		const char* GetModelName()const{ return GetModelName(m_model); }

		bool IsInit()const{ return m_nugget != 0 || m_sill != 0 || m_range != 0 || m_R2 != 0; }

		double GetNugget()const	{ return m_nugget; }
		double GetSill()const	{ return m_sill; }
		double GetRange()const	{ return m_range; }
		double GetR2()const		{ return m_R2; }

		double  cova3(const CGridPoint& pt1, const CGridPoint& pt2)const;
		//double  cova3(double x1, double y1, double z1, double x2, double y2, double z2)const;
		double GetMaxCov()const{ return m_nugget + ((m_model == POWER) ? m_PMX : m_sill); }
		double GetUnbias()const{ return GetMaxCov(); }

		const CDetrending& GetDetrending()const{ return m_detrending; }
		float GetDistLag()const {return m_dLag;}
		
		std::istream& operator << (std::istream& s);
		std::ostream& operator >> (std::ostream& s)const;

		friend std::istream& operator >>(std::istream& s, CVariogram& p) { p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CVariogram& p) { p >> s; return s; }

	protected:

		ERMsg FitVariogramModels(size_t model, const CVariogramPredictorVector& lagVar, double& final_nugget, double& final_sill, double& final_range, double& final_r2, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ComputePredictor(const CGridPointVector& pts, const CPrePostTransfo& transfo, int nLags, float xLag, const CDetrending& detrending, const CRotationMatrix& rotmat, CVariogramPredictorVector& lagVar);


		size_t	m_model;
		int		m_nLags;
		float	m_dLag;
		
		CDetrending m_detrending;
		CRotationMatrix m_rotmat;

		//variogram 
		double	m_nugget;
		double	m_sill;
		double	m_range;
		double	m_R2;
		double	m_PMX;

		CVariogramPredictorVector m_predictorVector;
		//size_t m_lastCheckSum;
		IAgent* m_pAgent;
		DWORD m_hxGridSessionID;

		static const char* MODEL_NAME[NB_MODELS];

	};



	class CVariogramInput
	{
	public:
		size_t	m_model;
		int		m_nlags;
		float	m_xlag;
		int		m_detrendingModel;
		bool	m_bRotmat;
		size_t	m_checkSum;
		CPrePostTransfo m_prePostTransfo;


		bool operator==(const CVariogramInput& in)const
		{
			bool bEqual = true;
			if (m_model != in.m_model)bEqual = false;
			if (m_nlags != in.m_nlags)bEqual = false;
			if (m_xlag != in.m_xlag)bEqual = false;
			if (m_detrendingModel != in.m_detrendingModel)bEqual = false;
			if (m_bRotmat != in.m_bRotmat)bEqual = false;
			if (m_checkSum != in.m_checkSum)bEqual = false;
			if (m_prePostTransfo != in.m_prePostTransfo)bEqual = false;

			return bEqual;
		}

		bool operator!=(const CVariogramInput& in)const
		{
			return !operator==(in);
		}


	};



	template<class T> class CVariogramHash;

	template<>
	class CVariogramHash < CVariogramInput >
	{
	public:
		size_t operator()(const CVariogramInput &s) const
		{
			size_t h1 = std::hash<size_t>()(s.m_model);
			size_t h2 = std::hash<int>()(s.m_nlags);
			size_t h3 = std::hash<float>()(s.m_xlag);
			size_t h4 = std::hash<int>()(s.m_detrendingModel);
			size_t h5 = std::hash<bool>()(s.m_bRotmat);
			size_t h6 = std::hash<size_t>()(s.m_checkSum);
			//size_t h7 = std::hash<CPrePostTransfo>()(s.m_prePostTransfo);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);// ^ (h7 << 6);
		}
	};

	typedef std::unordered_map<CVariogramInput, std::unique_ptr<CVariogram>, CVariogramHash<CVariogramInput> > _CVariogramCache;

	class CVariogramCache : public _CVariogramCache
	{
	public:

		CVariogramCache()
		{}


		~CVariogramCache()
		{
			FreeMemoryCache();
		}

		void FreeMemoryCache()
		{
			clear();
		}
		void Enter()
		{
			m_CS.Enter();
		}
		void Leave()
		{
			m_CS.Leave();
		}

	protected:

		CCriticalSection m_CS;

	private:


		CVariogramCache(const CVariogramCache& in)
		{/*operator=(in);*/
		}

		CVariogramCache& operator=(const CVariogramCache& in)
		{
			/*if(&in != this)
			{
			_CVariogramCache::operator=(in);
			}*/

			return *this;
		}

	};

}