#pragma once


#include "UpdaterBase.h"
#include "UtilWWW.h"
#include "WeatherStation.h"
#include "GDALBasic.h"


class CUIEnvCanGribForecast : public CUpdaterBase
{
public:

	//force type
	enum{ TYPE_00HOURS, TYPE_12HOURS};
	enum TATTRIBUTE { TYPE, NB_ATTRIBUTE };
	enum TATTRIBUTE_I { I_TYPE = CUpdaterBase::I_NB_ATTRIBUTE, I_NB_ATTRIBUTE };


	CUIEnvCanGribForecast(void);
	virtual ~CUIEnvCanGribForecast(void);
	CUIEnvCanGribForecast(const CUIEnvCanGribForecast& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUIEnvCanGribForecast& operator =(const CUIEnvCanGribForecast& in);
	bool operator ==(const CUIEnvCanGribForecast& in)const;
	bool operator !=(const CUIEnvCanGribForecast& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);
	virtual std::string GetClassID()const{return CLASS_NAME;}
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);


	virtual ERMsg Execute(CFL::CCallback& callback = DEFAULT_CALLBACK);
	virtual ERMsg PreProcess(CFL::CCallback& callback = DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(StringVector& stationList, CFL::CCallback& callback = DEFAULT_CALLBACK);
	virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CFL::CCallback& callback = DEFAULT_CALLBACK);

protected:

	ERMsg DownloadGrib(CFL::CCallback& callback);
	std::string GetOutputFilePath(const std::string& fileName)const;
	bool NeedDownload(const CFileInfo& info, const std::string& filePath)const;

	std::string GetRemoteFilePath(int hhh, const std::string& filetitle);
	std::string GetOutputFilePath(const std::string& filetitle);

//	std::string m_outputPath;
	int m_type;

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }

	std::vector<CGDALDatasetEx> datasets;
	
};

