//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-02-2013	Rémi Saint-Amant	Correction of a bug : leak of handle
// 20-01-2010	Rémi Saint-Amant	Add GetUserDataPath()
// 01-10-1998	Rémi Saint-Amant	Initial version 
//****************************************************************************

#include "stdafx.h"
#include "Basic/Registry.h"
#include "Basic/UtilStd.h"

using namespace std;

namespace WBSF
{

	const char* CRegistry::KEY_NAME = "Software\\NRCan\\";



	const char * CRegistry::BIOSIM = "BioSIM11";
	const char * CRegistry::SHOWMAP = "ShowMap";
	const char * CRegistry::GIS = "QGIS";
	//const char * CRegistry::PLT_WIDGET = "PLTWidget";
	//const char * CRegistry::PLT_WIN = "PLTWin";
	const char * CRegistry::HOURLY_EDITOR = "HourlyEditor";
	const char * CRegistry::DAILY_EDITOR = "DailyEditor";
	const char * CRegistry::NORMAL_EDITOR = "NormalsEditor";
	const char * CRegistry::MODEL_EDITOR = "ModelEditor";
	const char * CRegistry::MATCH_STATION = "MatchStation";
	const char * CRegistry::WEATHER_UPDATER = "WeatherUpdater";
	const char * CRegistry::FTP_TRANSFER = "FTPTransfer";
	const char * CRegistry::TDATE = "TDate";
	const char * CRegistry::MERGEFILE = "MergeFile";
	

	const char * CRegistry::TEXT_EDITOR = "TextEditor";
	const char * CRegistry::XML_EDITOR = "XMLEditor";
	const char * CRegistry::SPREADSHEET1 = "Spreadsheet1";
	const char * CRegistry::SPREADSHEET2 = "Spreadsheet2";
	
	const char * CRegistry::R_SCRIPT = "RScript";
	
	const char * CRegistry::WEATHER = "WeatherPath";
	const char * CRegistry::MAPS = "MapsPath";
	const char * CRegistry::MAPS_EXTENSIONS = "MapsExtensions";
	

	//****************************************************************************
	// Sommaire:     Constructeur par défaut.
	//
	// Description:  Crée la classe et ouvre une clé dans le régistre
	//
	// Entrée:
	//
	// Sortie:
	//
	// Note:
	//****************************************************************************
	CRegistry::CRegistry(std::string section, std::string application)
	{
		m_hKey = NULL;
		m_hCommonKey = NULL;

		string keyName;
		keyName = KEY_NAME;
		if (application.empty())
		{
			std::string appPath;
			appPath.resize(MAX_PATH);
			GetModuleFileNameA(NULL, &(appPath[0]), MAX_PATH);
			appPath.resize(strlen(appPath.c_str()));
			application = GetFileTitle(appPath);
		}

		keyName = "Software\\NRCan\\" + application + "\\" + section;

		DWORD  dwDisposition = 0;
		// try to create an App Name key 

		m_keyName = UTF16(keyName);
		LONG lRetCode = RegCreateKeyExW(HKEY_CURRENT_USER,
			m_keyName.c_str(),
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &m_hKey,
			&dwDisposition);


		ASSERT(lRetCode == ERROR_SUCCESS);
		ASSERT(m_hKey != NULL);

		keyName = "Software\\NRCan\\Common";
		m_commonKeyName = UTF16(keyName);
		lRetCode = RegCreateKeyExW(HKEY_CURRENT_USER,
			m_commonKeyName.c_str(),
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &m_hCommonKey,
			&dwDisposition);

		ASSERT(lRetCode == ERROR_SUCCESS);
		ASSERT(m_hCommonKey != NULL);

	}

	//****************************************************************************
	// Sommaire:     Destructeur de la classe
	//
	// Description:  Referme la clé dans le régistre.
	//
	// Entrée:
	//
	// Sortie:
	//
	// Note:
	//****************************************************************************
	CRegistry::~CRegistry()
	{
		if (m_hKey != NULL)
			RegCloseKey(m_hKey);

		if (m_hCommonKey != NULL)
			RegCloseKey(m_hCommonKey);

		m_hKey = NULL;
		m_hCommonKey = NULL;
	}

	//****************************************************************************
	// Sommaire:    Permet de modifier un item dans le régistre
	//
	// Description: Change dans le registre l'item pour la valeur de type chaine
	//
	// Entrée:		const string& itemName: Le nom de l'item
	//				const string& value: La valeur assigner à cet item
	//
	// Sortie:	
	//
	// Note:		On peut utiliser GetItemName() pour avoir le nom d'un item
	//				prédéfini.
	//****************************************************************************
	void CRegistry::WriteProfileString(const string& itemName, const string& value, bool bCommonKey)const
	{
		ASSERT(m_hKey != NULL);
		ASSERT(!itemName.empty());

		wstring wvalue = UTF16(value);

		HKEY  hKey = bCommonKey ? m_hCommonKey : m_hKey;
		if (hKey != NULL)
		{
			RegSetValueExW(hKey,
				UTF16(itemName).c_str(),
				0,
				REG_SZ,
				(const BYTE *)wvalue.data(),
				(DWORD)wvalue.length()*sizeof(wchar_t));
		}
	}

	void CRegistry::WriteProfileInt(const string& itemName, int value)
	{
		WriteProfileString(itemName, ToString(value));

		//ASSERT(m_hKey != NULL);
		//ASSERT(!itemName.empty());

		//char valueTmp[50] = { 0 };
		//_itoa_s(value, valueTmp, 50, 10);


		//RegSetValueExW(m_hKey,
		//	UTF16(itemName).c_str(),
		//	0,
		//	REG_DWORD,
		//	(LPBYTE)valueTmp,
		//	sizeof(DWORD));
	}

	bool CRegistry::GetProfileBool(const string& itemName, bool defaultValue)const
	{
		return GetProfileInt(itemName, defaultValue != 0) != 0;
	}

	void CRegistry::WriteProfileBool(const string& itemName, bool value)
	{
		WriteProfileInt(itemName, value != 0);
	}

	//****************************************************************************
	// Sommaire:    Permet d'obtenir la valeur d'un item particulier
	//
	// Description: En fournissant le nom de l'item à la fonction
	//				GetItemPath(const string& itemName, string& strNamePath)const
	//				On obtient sa valeur de type chaine.
	//
	// Entrée:		const string& itemName: Le nom de l'item
	//
	// Sortie:		string& value: La valeur de cet item dans le régistre
	//				bool : vrai si succes, faux autrement
	//
	// Note:		On peut utiliser GetItemName() pour avoir le nom d'un item
	//				prédéfini.
	//****************************************************************************
	string CRegistry::GetProfileString(const string& itemName, string defaultValue, bool bCommonKey)const
	{
		vector<BYTE> buffer;
		DWORD length = 0;
		DWORD type = 0;

		string value = defaultValue;

		HKEY  key = bCommonKey ? m_hCommonKey : m_hKey;
		if (key != NULL)
		{
			ASSERT(!itemName.empty());
			LONG lRetCode = RegQueryValueExW(key, UTF16(itemName).c_str(), 0, &type, NULL, &length);
			if (lRetCode == ERROR_SUCCESS && length > 0)
			{
				buffer.resize(length);
				lRetCode = RegQueryValueExW(key, UTF16(itemName).c_str(), 0, &type, &(buffer[0]), &length);
				if (lRetCode == ERROR_SUCCESS)
				{
					if (type == REG_SZ)
					{
						wstring wvalue(reinterpret_cast<wchar_t*>(&(buffer[0])), length / sizeof(wchar_t));
						wvalue.resize(wcslen(wvalue.c_str()));
						value = UTF8(wvalue);
					}
				}
			}
		}

		return value;
	}


	int CRegistry::GetProfileInt(const string& itemName, int defaultValue)const
	{
		string val = GetProfileString(itemName, ToString(defaultValue));
		return ToInt(val);
		
		//DWORD length = 4;
		//unsigned char valueTmp[sizeof(DWORD)] = { 0 };
		//DWORD type = REG_DWORD;

		//int value = defaultvalue;

		//wstring itemNameTmp = UTF16(itemName);
		//LONG lRetCode = RegQueryValueExW(m_hKey,
		//	itemNameTmp.data(),
		//	0,
		//	&type,
		//	valueTmp, &length);



		//if (lRetCode == ERROR_SUCCESS)
		//{
		//	ASSERT(length == sizeof(DWORD));
		//	value = atoi((char*)valueTmp);
		//}

		//return value;
	}


	string CRegistry::GetAppFilePath(string itemName)const
	{

		string filePath = GetProfileString(itemName + " FilePath");
		if (filePath.empty())
			filePath = GetProfileString(itemName + " FilePath", "", true);

		if (filePath.empty())
		{
			if (itemName == TEXT_EDITOR)
				filePath = "Notepad.exe";
			else if (itemName == XML_EDITOR)
				filePath = "Notepad.exe";
			else if (itemName == SPREADSHEET1)
				filePath = "Excel.exe";
			else if (itemName == SPREADSHEET2)
				filePath = "scalc.exe";
			else if (itemName == R_SCRIPT)
				filePath = "Rscript.exe";
			else 
				filePath = itemName + ".exe";
			
		}

		return filePath;
	}

	void CRegistry::SetAppFilePath(string itemName, string appFilePath)
	{
		WriteProfileString(itemName + " FilePath", appFilePath);
		WriteProfileString(itemName + " FilePath", appFilePath, true);
	}




	//****************************************************************************
	// Sommaire:    Permet d'obtenir la langue utitiliser par les application du CFL
	//
	// Description: 
	//
	// Entrée:
	//
	// Sortie:		retourne la langue: LTFRANCAIS / LTANGLAIS
	//
	// Note:
	//****************************************************************************
	string CRegistry::GetLocalInfo(string name, int type)const
	{
		string text = GetProfileString(name, "");

		if (text.empty())
		{
			wstring tmp;
			text.resize(1024);
			GetLocaleInfoW(LOCALE_USER_DEFAULT, type, &(tmp[0]), 1024);
			tmp.resize(wcslen(tmp.c_str()));
			tmp.shrink_to_fit();
			text = UTF8(tmp);
		}

		return text;
	}

	int CRegistry::GetLanguage()const
	{
		string lang = GetLocalInfo("Language", LOCALE_SISO639LANGNAME);

		int nLanguage = ENGLISH;
		if (lang == "fr")
			nLanguage = FRENCH;

		return nLanguage;
	}

	void CRegistry::SetLanguage(int language)
	{
		string lang = "En";
		if (language == FRENCH)
			lang = "fr";


		WriteProfileString("Language", lang);
	}

	char CRegistry::GetListDelimiter()const
	{
		string sepStr = GetLocalInfo("ListDelimiter", LOCALE_SLIST);
		char sep = ',';
		if (!sepStr.empty() && sepStr.length() == 1)
			sep = sepStr[0];

		return sep;

	}

	void CRegistry::SetListDelimiter(char sep)
	{
		string tmp;
		tmp.insert(tmp.begin(), sep);
		WriteProfileString("ListDelimiter", tmp);
	}

	char CRegistry::GetDecimalDelimiter()const
	{
		string sepStr = GetLocalInfo("DecimalDelimiter", LOCALE_SDECIMAL);
		char sep = '.';
		if (!sepStr.empty() && sepStr.length() == 1)
			sep = sepStr[0];

		return sep;
	}

	void CRegistry::SetDecimalDelimiter(char sep)
	{
		string tmp;
		tmp.insert(tmp.begin(), sep);
		WriteProfileString("DecimalDelimiter", tmp);
	}



}//namespace WNSF