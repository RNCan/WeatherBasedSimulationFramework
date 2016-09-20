//***************************************************************************
// File:	   TempGenHelp.h
//
// Class:      CTempGenHelp
//
// Summary:    Help class to use TemgGenLib.dll
//
// Description: This class is used to wrap TemgGenLib.dll fonctions into a c++ class
//
// Attributes:   void * m_hDll: handle on the dll
//
// Note:        
//***************************************************************************
#pragma once


#include <string.h>
#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"

namespace WBSF
{

	class CTempGenHelp
	{
	public:

		//Correction for exposition
		enum TAlbedo
		{
			NONE,           //no Albedo correction
			CONIFER_CANOPY  //Correction for conifer canopy
		};

		enum TSeed
		{
			RANDOM_SEED,	//Normal will be generate at random
			FIXE_SEED	//randomization of normal will always give the same values
		};
		//Simulated variables in TempGen
		enum TVariable
		{
			TMIN,	//Minimum Temperature (C°)
			TMAX,	//Maximum Temperature (C°)
			PRCP,	//Precipitation (mm)
			TDEW,	//Dew point  (C°)
			RELH,	//Relative humidity (%)
			WNDS	//Wind speed (km/h)
		};

		//type of statistic
		enum TStat
		{
			LOWEST,     //minimum value of the data
			MEAN,       //mean of the data
			SUM,        //sum of the data
			SUM2,       //sum of square of each data
			STD_DEV,    //standard deviation of the data
			STD_ERR,	//Standar error
			COEF_VAR,   //coeficiant of variation of the data
			VARIANCE,   //variance of the data
			HIGHEST,    //higest value of the data
			QUADRATIC_MEAN,
			TSS,        //total sum of squares
			NB_VALUE,   //number of values added in the statistic
			NB_STAT_TYPE
		};



		CTempGenHelp();
		virtual ~CTempGenHelp();

		bool IsInit()const;
		ERMsg Initialize(const char* DLLPath = "TempGenLib.dll");

		//initialisation of the simulator
		ERMsg SetNormalDBFilePath(const char* fileName);
		ERMsg SetDailyDBFilePath(const char* fileName);
		void SetTarget(const char* name, const char* ID, const double& lat, const double& lon, float elev, float slope = 0, float aspect = 0);
		void SetNbReplication(short nbRep);
		void SetTGInput(short firstYear, short lastYear,
			short nbNormalStation = 8, short nbDailyStation = 8,
			short albedoType = CONIFER_CANOPY, const char* category = "TN T TX P",
			short seedType = RANDOM_SEED, short Xval = 0);

		//execution of the simulator
		ERMsg Generate(void);
		ERMsg Save(const StringVector& outputFilePathVector)const;
		void WriteStream(short r, std::ostream& io)const;
		float GetValue(short r, short y, short jd, short v)const;
		double GetAllYearsStat(short var, short dailyStatType, short annualStatType)const;
		double GetGrowingSeasonStat(short var, short dailyStatType, short annualStatType)const;

	private:

		void * m_hDll;

		typedef bool(*SetNormalDBFilePathF)(const char* fileName, char messageOut[1024]);
		typedef bool(*SetDailyDBFilePathF)(const char* fileName, char messageOut[1024]);
		typedef void(*SetTargetF)(const char* name, const char* ID, const double& lat, const double& lon, float elev, float slope, float aspect);
		typedef void(*SetReplicationF)(short nbRep);
		typedef void(*SetTGInputF)(short year, short nbYear,
			short nbNormalStation, short nbDailyStation,
			short albedoType, const char* cat, short seedType, short Xval);
		typedef bool(*GenerateF)(char messageOut[1024]);
		typedef bool(*SaveF)(char** outputFilePathArray, char messageOut[1024]);
		typedef void(*WriteStreamF)(short r, std::ostream& io);
		typedef float(*GetValueF)(short r, short y, short jd, short v);
		typedef double(*GetAllYearsStatF)(short var, short dailyStatType, short annualStatType);
		typedef double(*GetGrowingSeasonStatF)(short var, short dailyStatType, short annualStatType);

		SetNormalDBFilePathF m_SetNormalDBFilePath;
		SetDailyDBFilePathF m_SetDailyDBFilePath;
		SetTargetF m_SetTarget;
		SetReplicationF m_SetReplication;
		SetTGInputF m_SetTGInput;
		GenerateF m_Generate;
		SaveF m_Save;
		WriteStreamF m_WriteStream;
		GetValueF m_GetValue;
		GetAllYearsStatF m_GetAllYearsStat;
		GetGrowingSeasonStatF m_GetGrowingSeasonStat;
	};

}