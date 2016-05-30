//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/SearchResult.h"
#include "Basic/WeatherDefine.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherCorrection.h"

namespace WBSF
{
	enum TScaleG{ LOCAL_GRADIENT, REGIONAL_GRADIENT, CONTINENTAL_GRADIENT, NB_SCALE_GRADIENT };
	enum TDefaultG{ SOUTH_HEMISPHERE, NORTH_HEMISPHERE, NB_HEMISPHERE};//CENTRAL_GRADIENT, 
	extern const char* GetHemisphereName(size_t i);
	extern const char* GetGradientName(size_t i);
	extern const char* GetScaleName(size_t i);
	extern const char* GetSpaceName(size_t i);
	

	typedef std::array<double, GRADIENT::NB_SPACE_EX> CGradientSpace;
	typedef std::array<CGradientSpace, 12> CGradientYear;
	typedef std::array<CGradientYear, GRADIENT::NB_GRADIENT_EX> CGradientVariables;
	typedef std::array < CGradientVariables, NB_SCALE_GRADIENT> CScaledGradient;
	typedef std::array < std::array < CGradientSpace, GRADIENT::NB_GRADIENT_EX >, NB_SCALE_GRADIENT> CGradientFactor;
	typedef std::array < double, 12> CGradientR²;
	typedef std::array <CStatistic, 12> CGradientS°;

	//***************************************************************************************
	class CWeatherGradient : public CWeatherCorrections
	{
	public:


		static const int NB_STATION_REGRESSION_LOCAL;
		static const double REGIONAL_FACTOR;
		static const double PPT_FACTOR;
		static const double FACTOR_Z;


		//bool m_bForceComputeAllScale;

		CWeatherGradient();
		


		void reset();
		void reset_data();
		ERMsg Save(const std::string& filePath)const;

		ERMsg CreateGradient(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg ComputeGradient(size_t v, CSearchResultVector& results, CGradientYear& Gr, CGradientR²& R², CCallback& callBack);
		ERMsg CreateDefaultGradient(CCallback& callback = DEFAULT_CALLBACK);
		double GetFactor(size_t z, size_t v, size_t s, const CSearchResultVector& results)const;
		double GetDistance(size_t s, const CLocation& target, const CLocation& station)const;
		void GetS°(size_t g, const CSearchResultVector& results, CGradientS°& S°)const;

		const double& operator ()(size_t z, size_t g, size_t m, size_t s)const
		{
			ASSERT(z < NB_SCALE_GRADIENT && m < 12 && g < GRADIENT::NB_GRADIENT_EX && s < GRADIENT::NB_SPACE_EX);
			return m_gradient[z][g][m][s];
		}

		CNormalsDatabasePtr& GetNormalsDatabase(){ return m_pNormalDB; }
		void SetNormalsDatabase(CNormalsDatabasePtr& pNormalDB){ m_pNormalDB = pNormalDB; }
		ERMsg ExportInput(const std::string& filePath, size_t v, CSearchResultVector& results = CSearchResultVector());

		
		virtual double GetCorrection(const CLocation& pt, CTRef TRef, size_t v)const;
		double GetCorrectionII(const CLocation& station, size_t m, size_t g, size_t s)const;

		static ERMsg SetShore(const std::string& filePath);
		static void SetShore(CApproximateNearestNeighborPtr& pShore){ m_pShore = pShore; }
		static const CApproximateNearestNeighborPtr& GetShore(){ return m_pShore; }
		static ERMsg Shape2ANN(const std::string& filePathIn, const std::string& filePathOut);
		static ERMsg AddShape(const std::string& filePathIn1, const std::string& filePathIn2, const std::string& filePathOut);
		static double GetShoreDistance(const CLocation& location);
		static size_t GetNbSpaces();

		double GetS°(size_t z, size_t g, size_t m)const{ return m_S°[z][g][m][MEAN]; }
		double GetFactor(size_t z, size_t g, size_t s)const{ return m_factor[z][g][s]; }
		double GetR²(size_t s, size_t g, size_t m)const{ return m_R²[s][g][m]; }

	protected:
		

		CNormalsDatabasePtr m_pNormalDB;
		CScaledGradient m_gradient;
		CGradientFactor m_factor;
		std::array < std::array < CGradientR², GRADIENT::NB_GRADIENT_EX >, NB_SCALE_GRADIENT> m_R²;
		std::array <std::array <CGradientS°, GRADIENT::NB_GRADIENT_EX >, NB_SCALE_GRADIENT> m_S°;
		
		//for optimization
		//std::map<CGeoPoint3D, double> m_shoreCache;



		static CApproximateNearestNeighborPtr m_pShore;
		static const double DEFAULT_GRADIENTS[NB_HEMISPHERE][GRADIENT::NB_GRADIENT_EX][12][GRADIENT::NB_SPACE_EX];
		static const CGradientS° GLOBAL_S°[NB_HEMISPHERE][GRADIENT::NB_GRADIENT_EX];
		static const size_t NB_S_MAX[NB_HEMISPHERE];
		static const CGeoRect DEFAULT_RECT[NB_HEMISPHERE];


		static const double A[NB_SCALE_GRADIENT];
		static const double B[NB_SCALE_GRADIENT];
		static const double F1[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT_EX];
		static const double F2[NB_SCALE_GRADIENT][GRADIENT::NB_GRADIENT_EX];

	};



	typedef std::shared_ptr<CWeatherGradient> CWeatherGradientPtr;


}