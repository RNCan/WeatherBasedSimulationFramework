#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	typedef std::map<std::string, CWeatherStation> CWeatherStationMap;
	//**************************************************************
	class CUISOPFEUHourly : public CTaskBase
	{
	public:

//FE	Émetteur Sopfeu
//Identifiant	Station
//Date
//Heure	Temps universel coordonné(ou UTC)

//00. TAi000H	Température horaire(°C)
//01. TAn000H	Température minimale horaire(°C)
//02. TAx000H	Température maximale horaire(°C)
//03. TAm000H	Température moyenne horaire(°C)
//04. PC040H	Précipitation liquide horaire cumulée(mm) : se remet à 0.0 au jour julien 90
//05. PT040H	Précipitation liquide horaire(mm)
//06. PC020H	Précipitation totale horaire cumulée(mm)
//07. PT041H	Poids total en mm
//08. HAi000H	Humidité relative horaire(%)
//09. HAn000H	Humidité relative minimale horaire(%)
//10. HAx000H	Humidité relative maximale horaire(%)
//11. NSi000H	Épaisseur de la neige au sol horaire(cm)
//12. VDi300H	Direction du vent instantanée horaire(°degrés)
//13. VVi300H	Vitesse du vent instantanée horaire(km/h)
//14. VVxi500H	Vitesse de la pointe de vent horaire(km/h)
//15. VDm025B	Direction du vent instantanée horaire à 2,5 mètres(°degrés)
//16. VVm025B	Vitesse du vent instantanée horaire à 2,5 mètres(km / h)
//17. TDi000H	Température du point de rosée horaire(°C)
//18. VB000B	Voltage batterie
//

		enum TField{ TAI000H, TAN000H, TAX000H, TAM000H, PC040H, PT040H, PC020H, PT041H, HAI000H, HAN000H, HAX000H, NSI000H, VDI300H, VVI300H, VVXI500H, VDM025B, VVM025B, TDI000H, VB000B, NB_FIELDS };
		static const char * FIELDS_NAME[NB_FIELDS];
		static const HOURLY_DATA::TVarH VARIABLE[NB_FIELDS];
		static HOURLY_DATA::TVarH GetVariable(std::string str);

		enum TAttributes { USER_NAME, PASSWORD, WORKING_DIR, FIRST_YEAR, LAST_YEAR, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUISOPFEUHourly); }

		CUISOPFEUHourly(void);
		virtual ~CUISOPFEUHourly(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationListFilePath()const;

		ERMsg LoadWeatherInMemory(CCallback& callback);
		ERMsg ReadData(const std::string& filePath, CWeatherStationMap& m_stations, CCallback& callback = DEFAULT_CALLBACK)const;
		std::string GetOutputFilePath(CTRef TRef)const;
		

		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback)const;
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback)const;

		CLocationVector m_stationsList;
		CWeatherStationMap m_stations;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;

		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};



}