#pragma once


#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{


	enum TRadar
	{ 
		R_ATL, R_ONT, R_PNR, R_PYR, R_QUE, 
		R_WUJ, R_XBE, R_WBI, R_WHK, R_XNC, R_XDR, R_WSO, R_XFW, R_XFT, R_XGO, R_WTP, 
		R_WHN, R_WKR, R_WMB, R_XLA, R_XME, R_XMB, R_WMN, R_WGJ, R_XTI, R_XPG, R_XRA, 
		R_XBU, R_XSS, R_WWW, R_XSM, R_XNI, R_XAM, R_XSI, R_WVY, R_XWL, NB_RADARS
	};
		
	class CCanadianRadar : public std::bitset<NB_RADARS>
	{
	public:

		enum TInfo{ ABRV1, ABRV2, NAME, COORD, NB_INFO };

		static std::string GetAllPossibleValue(bool bAbvr = true, bool bName = true);
		static size_t GetRadar(const std::string& in, size_t t );
		static std::string GetName(size_t r, size_t t );
		

		CCanadianRadar(const std::string& sel = "")
		{
			FromString(sel);
		}

		std::string ToString()const;
		ERMsg FromString(const std::string&);

//		using std::bitset<NB_RADAR>::at;
		bool at(const std::string& in)const
		{
			if (none())
				return true;
			
			size_t p1 = GetRadar(in, ABRV1);
			size_t p2 = GetRadar(in, ABRV2);

			return p1 < size() ? test(p1) : p2 < size() ? test(p2) : false;
		}

		using std::bitset<NB_RADARS>::set;
		ERMsg set(const std::string& in);

	protected:

		static const char* DEFAULT_LIST[NB_RADARS][NB_INFO];
	};


	class CUIEnvCanRadar : public CTaskBase
	{
	public:

		enum TTemporal{ CURRENT_RADAR, HISTORICAL_RADAR, NB_TEMPORAL_TYPE };
		enum TPrcp{ T_SNOW, T_RAIN, NB_TYPE };
		enum TBackground { B_WHITE, B_BROWN};

		enum TAttributes { WORKING_DIR, TYPE, PRCP_TYPE, BACKGROUND, RADAR, FIRST_DATE, LAST_DATE, COMPOSITE, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanRadar); }

		CUIEnvCanRadar(void);
		virtual ~CUIEnvCanRadar(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:


		ERMsg ExecuteCurrent(CCallback& callback);
		ERMsg ExecuteHistorical(CCallback& callback);
		bool NeedDownload(const CFileInfo& info, const std::string& filePath)const;
		ERMsg GetRadarList(StringVector& stationList, CCallback& callback)const;
		ERMsg CleanRadarList(StringVector& stationList, CCallback& callback)const;
		std::string GetOutputFilePath(size_t t, const std::string& fileName)const;
		CTPeriod GetPeriod()const;
		static std::string GetRadarID(size_t t, const std::string& URL);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_TEMPORAL_TYPE];
		static const char* SERVER_PATH;
		static const char* TYPE_NAME_OLD[NB_TYPE];
		static const char* TYPE_NAME_NEW[NB_TYPE];
	};

}