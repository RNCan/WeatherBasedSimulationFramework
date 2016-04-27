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

//FE	�metteur Sopfeu
//Identifiant	Station
//Date
//Heure	Temps universel coordonn�(ou UTC)

//00. TAi000H	Temp�rature horaire(�C)
//01. TAn000H	Temp�rature minimale horaire(�C)
//02. TAx000H	Temp�rature maximale horaire(�C)
//03. TAm000H	Temp�rature moyenne horaire(�C)
//04. PC040H	Pr�cipitation liquide horaire cumul�e(mm) : se remet � 0.0 au jour julien 90
//05. PT040H	Pr�cipitation liquide horaire(mm)
//06. PC020H	Pr�cipitation totale horaire cumul�e(mm)
//07. PT041H	Poids total en mm
//08. HAi000H	Humidit� relative horaire(%)
//09. HAn000H	Humidit� relative minimale horaire(%)
//10. HAx000H	Humidit� relative maximale horaire(%)
//11. NSi000H	�paisseur de la neige au sol horaire(cm)
//12. VDi300H	Direction du vent instantan�e horaire(�degr�s)
//13. VVi300H	Vitesse du vent instantan�e horaire(km/h)
//14. VVxi500H	Vitesse de la pointe de vent horaire(km/h)
//15. VDm025B	Direction du vent instantan�e horaire � 2,5 m�tres(�degr�s)
//16. VVm025B	Vitesse du vent instantan�e horaire � 2,5 m�tres(km / h)
//17. TDi000H	Temp�rature du point de ros�e horaire(�C)
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