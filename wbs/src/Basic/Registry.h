//******************************************************************************
//  project:		Weather-based simulation framework
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************

#pragma once

#include <string>
#include <wtypes.h>

namespace WBSF
{

	class CRegistry
	{
	public:
		CRegistry(std::string section = "Common", std::string application = "");
		~CRegistry();

		//application path

		static const char * BIOSIM;
		static const char * SHOWMAP;
		static const char * HOURLY_EDITOR;
		static const char * DAILY_EDITOR;
		static const char * NORMAL_EDITOR;
		static const char * MODEL_EDITOR;
		static const char * MATCH_STATION;
		static const char * WEATHER_UPDATER;
		static const char * FTP_TRANSFER;
		static const char * TDATE;
		static const char * MERGEFILE;

		static const char * TEXT_EDITOR;
		static const char * XML_EDITOR;
		static const char * SPREADSHEET1;
		static const char * SPREADSHEET2;
		static const char * GIS;
		static const char * R_SCRIPT;

		//common path
		static const char * WEATHER;
		static const char * MAPS;
		static const char * MAPS_EXTENSIONS;


		enum TLanguage { FRENCH, ENGLISH, NB_LANGUAGE };

		template <typename T>
		T GetValue(const std::string& itemName, const T& defaultValue, bool localMachine = false)const
		{
			std::string defaultStr = ToString(defaultValue);
			std::string str = GetProfileString(itemName, defaultStr, localMachine);
			return ToValue<T>(str);
		}

		template <class T>
		T GetObject(const std::string& itemName, const T& defaultValue, bool localMachine = false)const
		{
			std::string defaultStr = ToString(defaultValue);
			std::string str = GetProfileString(itemName, defaultStr, localMachine);
			return ToObject<T>(str);
		}

		template <typename T>
		void SetValue(const std::string& itemName, const T& value, bool localMachine = false)const
		{
			std::string str = ToString(value);
			WriteProfileString(itemName, str, localMachine);
		}

		std::string GetProfileString(const std::string& itemName, std::string defaultValue = "", bool localMachine = false)const;
		void WriteProfileString(const std::string& itemName, const std::string& value, bool localMachine = false)const;

		int GetProfileInt(const std::string& itemName, int defaultValue = 0)const;
		void WriteProfileInt(const std::string& itemName, int value);

		bool GetProfileBool(const std::string& itemName, bool defaultValue = false)const;
		void WriteProfileBool(const std::string& itemName, bool value);

		std::string GetAppFilePath(std::string item)const;
		void SetAppFilePath(std::string item, std::string appFilePath);

		std::string GetLocalInfo(std::string name, int type)const;
		int GetLanguage()const;
		void SetLanguage(int language);


		char GetListDelimiter()const;
		void SetListDelimiter(char sep);
		char GetDecimalDelimiter()const;
		void SetDecimalDelimiter(char sep);


	protected:

		std::wstring m_keyName;
		std::wstring m_commonKeyName;

		static const char* KEY_NAME;


		HKEY  m_hKey;
		HKEY  m_hCommonKey;
	};

}//namespace WBSF