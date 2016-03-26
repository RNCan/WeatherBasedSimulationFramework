#pragma once

#include "UIWeather.h"

class CWUCountrySelection
{
public:

	enum TABRV{UNKNOW=-1, NB_COUNTRY=233};
	enum TNAMEE{ ALL=NB_COUNTRY, NB_TAG};

	
	//class CInfo
	//{
	//public:
		//enum { ABVR, NAME, NB_TAG };
		
//		long m_firstID;
//		long m_lastID;
		//std::string m_abrv;
		//std::string m_name;
	//};
	
//	typedef CArray<CInfo, CInfo&> CInfoArray;

	static const std::string COUNTRAY_NAME[NB_TAG];
//	static CInfoArray m_countryList;

	static short GetCountry(const std::string& in);//by abr
	//static short GetCountry(long in);//by number
	

	CWUCountrySelection();

	void Reset(bool bUse=false)
	{
		for(int i=0; i<NB_COUNTRY; i++)
			m_use[i] = bUse;
	}


	bool operator ==(const CWUCountrySelection& in)const
	{
		bool bEqual = true;
		for(int i=0; i<NB_COUNTRY; i++)
			if( m_use[i] != in.m_use[i])bEqual = false;

		return bEqual;
	}
	bool operator !=(const CWUCountrySelection& in)const
	{
		return !operator ==(in);
	}

//	ERMsg Load(const std::string filePath);

	std::string ToString()const;
	ERMsg FromString(const std::string&);
	
//	void GetXML(CMarkup& xml);
//	ERMsg SetXML(CMarkup& xml);

	static std::string GetName(short country);

	int GetNbSelection()const
	{
		int nbSel = 0;
		for(int i=0; i<NB_COUNTRY; i++)
			if( m_use[i])
				nbSel++;

		return nbSel;
	}

	bool IsUsedAll()const
	{ 
		bool bUsed = true;
		for(int i=0; i<NB_COUNTRY; i++)
			if( !m_use[i])
				bUsed = false;
		
		return bUsed;
	}
	bool IsUsed(short p)const{ASSERT(p>=0&&p<NB_COUNTRY); return m_use[p]; }
	bool IsUsed(const std::string& country)const
	{
		int p = GetCountry(country);
		return p>=0?IsUsed(p):false;
	}

	ERMsg SetUsed(const std::string& country)
	{
		ERMsg message;
		short p= GetCountry(country);
		if( p>=0 && p<=NB_COUNTRY)
			SetUsed(p, true);
		else
		{
			message.ajoute( "Le paye suivant est invalide : " + country);
		}

		return message;
	}
	void SetUsed(short p, bool bUse)
	{
		ASSERT(p>=0&&p<=NB_COUNTRY); 
		if( p>=0 && p<NB_COUNTRY)
			m_use[p]=bUse; 
		else Reset(true);
	}
		
private:
	bool m_use[NB_COUNTRY];//mettre dynamic

};

class CUIWunderground : public CUIWeather
{
public:

	//force type
	enum TATTRIBUTE {PROVINCE,  NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_PROVINCE=CUIWeather::I_NB_ATTRIBUTE,  I_NB_ATTRIBUTE};


	CUIWunderground(void);
	virtual ~CUIWunderground(void);
	CUIWunderground(const CUIWunderground& in);

	virtual void InitClass(const StringVector& option = EMPTY_OPTION);

	void Reset();
	CUIWunderground& operator =(const CUIWunderground& in);
	bool operator ==(const CUIWunderground& in)const;
	bool operator !=(const CUIWunderground& in)const;
	//virtual operator
	virtual bool Compare(const CParameterBase& in)const;
	virtual CParameterBase& Assign(const CParameterBase& in);

	virtual void GetSelection(short param, CSelectionDlgItemVector& items)const;
	virtual void SetSelection(short param, const CSelectionDlgItemVector& items);
	
	virtual std::string GetClassID()const{return CLASS_NAME;}
	
//proptree param
	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	
	
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	virtual ERMsg PreProcess(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStationList(StringVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual ERMsg GetStation(const std::string& stationName, CDailyStation& station, CFL::CCallback& callback=DEFAULT_CALLBACK);

	ERMsg GetStationList(CLocationVector& stationList, CFL::CCallback& callback=DEFAULT_CALLBACK)const;
protected:

	
	//Update station list part
	//CFL::CPeriod String2Period(const std::string& period)const;
	//int GetNbStation(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page)const;

	ERMsg GetStationListPage(UtilWWW::CHttpConnectionPtr& pConnection, const std::string& page, CLocationVector& stationList)const;
	ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
	



	//Update data part
	ERMsg ParseStationDataPage(const std::string& source, std::string& parsedText)const;
	bool IsValid(const CLocation& info, short y, short m, const std::string& filePath )const;
	static long GetNbDay(const CTime& t);
	static long GetNbDay(int y, int m, int d);

//	bool NeedDownload(const CECPackDB& packDB, const CLocation& info, short y, short m)const;
//	ERMsg CopyStationDataPage(CHttpConnectionPtr& pConnection, const std::string& page, short year, short month, CECPackDB& packDB)const;
//	ERMsg DonwloadStation( CHttpConnectionPtr& pConnection, const CLocation& info, CFL::CCallback& callback);

	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;
	static const char* SERVER_NAME;
	
//	long m_nbDays;
//	bool m_bUseForecast;
	CWUCountrySelection m_selection;
//	short m_forceDownload;


	//stat
//	void InitStat();
//	void AddToStat(short year, short m);
//	void ReportStat(CFL::CCallback& callback);
	
//	typedef C2DimArray<int, int> CIntMatrix;
//	int m_nbDownload;
//	CIntMatrix m_stat;

	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
};

