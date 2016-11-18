//***************************************************************************
// Fichier:     CWeatherDatabase.h
//
// Classe:      CWeatherDatabase
//
// Sommaire:    Opération sur les bases de données temps réel.
//
// Description: La class CWeatherDatabase permet d'obtenir et d'éditer (add, delete, 
//              modify) la base de données RT. Elle permet aussi de faire
//              des recherches(match station) et d'obtenir une liste de loc à 
//              partir des stations RT.
//
// Attributs:   bool m_bDBOpen: pour savoir si la BD est chargée en mémoire
//              CTime m_openDate: la date du dernier chargement
//              CRTStationHeadArray* m_pStationArrayTmp: liste des station RT
//              CRTStationArray m_cacheStation: cache pour GetRTStation()
//
// Note:        À chaque fois que l'on fait un GetRTStation ou matchStation
//              alors la BD est charger en mémoire jusqu'à ce qu'on
//              effectue une modification interne ou externe sur la BD.
//              Il y a un cas ou la cache peut devenir invalide: si l'usager
//              modifie directement les donnée RT (.wea) sans vider la cache.
//***************************************************************************
#pragma once

#include "Basic/WeatherDatabase.h"

namespace WBSF
{


	class CHourlyDatabase : public CDHDatabaseBase
	{
	public:

		static const int   VERSION;
		static const char* XML_FLAG;
		static const char* DATABASE_EXT;
		static const char* OPT_EXT;
		static const char* DATA_EXT;
		static const char* META_EXT;
		static const CTM   DATA_TM;
		virtual int GetVersion()const{ return VERSION; }
		virtual const char* GetXMLFlag()const{ return XML_FLAG; }
		virtual const char* GetDatabaseExtension()const{ return DATABASE_EXT; }
		virtual const char* GetOptimizationExtension()const{ return OPT_EXT; }
		virtual const char* GetDataExtension()const{ return DATA_EXT; }
		virtual const char* GetHeaderExtension()const{ return META_EXT; }
		virtual const CTM	GetDataTM()const{ return DATA_TM; }
		virtual const char	GetDBType()const{ return 'H'; }


		CHourlyDatabase(int cacheSize = 200) : CDHDatabaseBase(cacheSize)
		{}

		static int GetVersion(const std::string& filePath);
		static ERMsg CreateDatabase(const std::string& filePath);
		static ERMsg DeleteDatabase(const std::string& filePath, CCallback& callback = DEFAULT_CALLBACK);
		static ERMsg RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback = DEFAULT_CALLBACK);
		//static ERMsg AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback = DEFAULT_CALLBACK);

	};




	typedef std::shared_ptr<CHourlyDatabase> CHourlyDatabasePtr;


}