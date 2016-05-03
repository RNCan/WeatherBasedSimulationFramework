#pragma once

#include <bitset>
#include <array>


#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
//#include "Geomatic/ShapeFileBase.h"

#include "TaskBase.h"

namespace WBSF
{

	class CShapeFileBase;

	enum { ATLANTIC, ONTARIO, PRAIRIES, PACIFIC, QUEBEC, NB_REGIONS };
	typedef std::array<std::pair<std::string, std::string>, NB_REGIONS> CRegionArray;

	class CRegionSelection : public std::bitset < NB_REGIONS >
	{
	public:

		class CFindRegion
		{
		public:

			CFindRegion(const std::string& name, bool bAbvr = false)
			{
				m_name = name;
				m_bAbvr = bAbvr;
			}

			inline bool operator() (const std::pair<std::string, std::string>& info)
			{
				return m_bAbvr ? info.second == m_name : info.first == m_name;
			}


		protected:

			std::string m_name;
			bool m_bAbvr;
		};


		//enum { ATLANTIC, ONTARIO, PRAIRIES, PACIFIC, QUEBEC, NB_REGIONS };
		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);

		CRegionSelection(const std::string& bits = "");

		const std::string& GetName(size_t i, bool bAbvr = false)const
		{
			ASSERT(i >= 0 && i < REGION_NAME.size());
			return !bAbvr ? REGION_NAME[i].first : REGION_NAME[i].second;
		}

		size_t GetRegion(std::string str, bool bAbvr = false)
		{
			Trim(str);
			CRegionArray::const_iterator it = std::find_if(REGION_NAME.begin(), REGION_NAME.end(), CFindRegion(str, bAbvr));
			return it == REGION_NAME.end() ? UNKNOWN_POS : int(it - REGION_NAME.begin());
		}


		std::string GetString()const;
		void SetString(std::string);

	protected:


		static CRegionArray REGION_NAME;

	};


	//**************************************************************
	class CUIEnvCanHourlyForecast : public CTaskBase
	{
	public:

		enum TAttributes { WORKING_DIR, REGION, NB_ATTRIBUTES };


		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanHourlyForecast); }

		CUIEnvCanHourlyForecast(void);
		virtual ~CUIEnvCanHourlyForecast(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsForecast()const{ return true; }
		virtual bool IsHourly()const{ return true; }
		

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Default(size_t i)const;
		std::string Option(size_t i)const;

	protected:

		ERMsg LoadStationInformation(const std::string& outputFilePath, CLocation& station);



		ERMsg ReadData(const std::string& filePath, CWeatherStationVector& stations, CCallback& callback)const;
		std::string GetShapefileFilePath()const;
		std::string GetStationListFilePath()const;
		std::string GetDatabaseFilePath()const;


		CRegionSelection m_selection;
		CLocationVector m_stations;

		CHourlyDatabase m_DB;
		CShapeFileBase* m_pShapefile;



		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};
}