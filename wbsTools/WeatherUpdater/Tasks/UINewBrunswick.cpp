#include "StdAfx.h"
#include "UINewBrunswick.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
#include "Basic\UtilZen.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


#include "UI/Common/UtilWin.h"
#include "Basic/decode_html_entities_utf8.h"

using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;



namespace WBSF
{

	const char* CUINewBrunswick::SERVER_NAME[NB_NETWORKS] = { "www1.gnb.ca", "daafmaapextweb.gnb.ca"};


	//*********************************************************************
	const char* CUINewBrunswick::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", };
	const size_t CUINewBrunswick::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT };
	const UINT CUINewBrunswick::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NEWBRUNSWICK_P;
	const UINT CUINewBrunswick::DESCRIPTION_TITLE_ID = ID_TASK_NEWBRUNSWICK;

	const char* CUINewBrunswick::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NewBrunswick";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINewBrunswick::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINewBrunswick::CLASS_NAME(), (createF)CUINewBrunswick::create);



	CUINewBrunswick::CUINewBrunswick(void)
	{}

	CUINewBrunswick::~CUINewBrunswick(void)
	{}



	std::string CUINewBrunswick::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK:	str = "0=Fire|1=Agriculture"; break;
		};
		return str;
	}

	std::string CUINewBrunswick::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NB\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************
	ERMsg CUINewBrunswick::GetFileList(size_t n, StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

//		msg = GetHttpConnection(SERVER_NAME[n], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{

			if (n == FIRE)
			{
				//open a connection on the server
//				string str;
	//			msg = UtilWWW::GetPageText(pConnection, "content/gnb/en/departments/natural_resources/ForestsCrownLands/content/FireManagement/FireWeatherLatestObservations.html", str);
				if (msg)
				{
					static const char* STATIONS[25] = { "Alward", "Anfield", "Apohaqui", "Bayles+Lake", "Bantalor", "Big+Tracadie", "Boston+Brook", "Brockway", "Clearwater", "County+Line", "Crowe+Brook", "Cumberland+Bay", "Dingee+Brook", "Dungarvon", "Florenceville", "Heath+Steele", "Hornes+Gulch", "Kagout+Brook", "Lunts+Brook", "McRae+Lake", "Meadow+Brook", "Mulligan+Gulch", "Robinson+Tower", "Quarryville", "St.+Paul" };
					for (int i = 0; i < 25; i++)
						fileList.push_back(STATIONS[i]);
				}
			}
			else if (n == AGRI)
			{
				
				//				string str;
				//			msg = UtilWWW::GetPageText(pConnection, "content/gnb/en/departments/natural_resources/ForestsCrownLands/content/FireManagement/FireWeatherLatestObservations.html", str);
				if (msg)
				{
					static const char* STATIONS[19] = { "47", "49", "62", "42", "45", "51", "55", "36", "37", "59", "41", "67", "68", "70", "66", "65", "53", "73", "69" };
					for (int i = 0; i < 19; i++)
						fileList.push_back(STATIONS[i]);
				}

//"47" Aberdeen(47)
//"49" Andover(49)
//"62" Brighton(62)
//"42" Drummond(42)
//"45" Drummond(45)
//"51" Drummond(51)
//"55" Drummond(55)
//"36" Gordon(36)
//"37" Kent(37)
//"59" Richmond(59)
//"41" Saint - Andre(41)
//"67" Saint - Andre(67)
//"68" Saint - Leonard(68)
//"70" Simonds(70)
//"66" Wakefield(66)
//"65" Wicklow(65)
//"53" Wilmot(53)
//"73" Wilmot(73)
//"69" Woodstock(69)
			}
		}

		return msg;
	}



	ERMsg CUINewBrunswick::Execute(CCallback& callback)
	{
		ERMsg msg;

		std::bitset<NETWORK> network;

		StringVector str(Get(NETWORK), "|");
		if (str.empty())
		{
			network.set();
		}
		else
		{

			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = ToSizeT(str[i]);
				if (n < network.size())
					network.set(n);
			}
		}

		for (size_t n = 0; n < network.size()&&msg; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case FIRE: msg += ExecuteFire(callback); break;
				case AGRI: msg += ExecuteAgriculture(callback); break;
				}
			}
		}
		

		return msg;
	}

	ERMsg PostPageText(CHttpConnectionPtr& pConnection, const std::string& URLIn, const std::string& header, const std::string& data, std::string& text, bool bConvert = false, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE)
	{

		ERMsg msg;


		//CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded");
		//CString strFormData = _T("__EVENTTARGET=&__EVENTARGUMENT=&__VIEWSTATE=%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%2Fkx8tP9Qp93CvCxoYUfKz%2B%2F6A%3D%3D&__VIEWSTATEGENERATOR=7FE89812&__EVENTVALIDATION=%2FwEWhAEC6vCn2AwCjPGn3gECkN%2Fz0AcC7Of0OwL3wr3hAgLmha0BAtCk16ACAtCkn6MCAtak%2B6ACAtCk%2B6ACAtCkz6ACAtek%2F6ACAtekz6ACAtGky6ACAtGk16ACAtekn6MCAtCk%2F6ACAtak16ACAtakk6MCAtWk86ACAtaky6ACAtakz6ACAtekx6ACAtWkx6ACAtakn6MCAuLr9sIDAuPr9sIDAuDr9sIDAuHr9sIDAubr9sIDAufr9sIDAuTr9sIDAvXr9sIDAvrr9sIDAuLrtsEDAuLrusEDAuLrvsEDAuLrgsEDAuLrhsEDAuLrisEDAuLrjsEDAuLrksEDAuLr1sIDAuLr2sIDAuPrtsEDAuPrusEDAuPrvsEDAuPrgsEDAuPrhsEDAuPrisEDAuPrjsEDAuPrksEDAuPr1sIDAuPr2sIDAuDrtsEDAuDrusEDAtXQoYkBAtTQoYkBAtfQoYkBAtbQoYkBAtHQoYkBAtDQoYkBAtPQoYkBAsLQoYkBAs3QoYkBAtXQ4YoBAtXQ7YoBAtXQ6YoBApTRytAPApTR5rkJApTR0twBAv%2Fo4MUGAv%2Fo3JgNAv%2FoyL8EAv%2FopNIMAv%2FokOkLAv%2FojIwCAvKEiO4EAvS6t0wC9bq3TAL2urdMAve6t0wC8Lq3TALxurdMAvK6t0wC47q3TALsurdMAvS6908C9Lr7TwL0uv9PAvS6w08C9LrHTwL0ustPAvS6z08C9LrTTwL0updMAvS6m0wC9br3TwL1uvtPAvW6%2F08C9brDTwL1usdPAvW6y08C9brPTwL1utNPAvW6l0wC9bqbTAL2uvdPAva6%2B08C14iy%2BgQC1oiy%2BgQC1Yiy%2BgQC1Iiy%2BgQC04iy%2BgQC0oiy%2BgQC0Yiy%2BgQCwIiy%2BgQCz4iy%2BgQC14jy%2BQQC14j%2B%2BQQC14j6%2BQQCrte4hQQCrteU7AICrtegiQoCxe6SkA0Cxe6uzQYCxe666g8Cxe7WhwcCxe7iPALF7v7ZCQL0x6SlAQKA3uC2A0LNcpbYprj4QvUiSHNkEUzsHvnf&ctl00%24hfLang=fr-CA&ctl00%24Content1%24ddlWS=47&ctl00%24Content1%24ddlFromDay=1&ctl00%24Content1%24ddlFromMonth=1&ctl00%24Content1%24ddlFromYear=2015&ctl00%24Content1%24hdnFromDate=&ctl00%24Content1%24ddlToDay=2&ctl00%24Content1%24ddlToMonth=1&ctl00%24Content1%24ddlToYear=2015&ctl00%24Content1%24hdnToDate=&ctl00%24Content1%24btnGetData=Submit");

		CString URL = WBSF::convert(URLIn).c_str();
		CString strHeaders = WBSF::convert(header).c_str();
		CString strFormData = WBSF::convert(data).c_str();
		CHttpFile* pURLFile = pConnection->OpenRequest(_T("POST"), URL, NULL, 1, NULL, NULL, flags);

		bool bRep = false;

		if (pURLFile != NULL)
		{
			int nbTry = 0;
			while (!bRep && msg)
			{
				TRY
				{
					nbTry++;
					bRep = pURLFile->SendRequest(strHeaders, (LPVOID)(LPCTSTR)strFormData,strFormData.GetLength()) != 0;
				}
				CATCH_ALL(e)
				{
					DWORD errnum = GetLastError();
					if (errnum == 12002 || errnum == 12029)
					{
						if (nbTry >= 10)
						{
							msg = UtilWin::SYGetMessage(*e);
						}
						//try again
					}
					else if (errnum == 12031 || errnum == 12111)
					{
						//throw a exception: server reset
						THROW(new CInternetException(errnum));
					}
					else if (errnum == 12003)
					{
						msg = UtilWin::SYGetMessage(*e);

						DWORD size = 255;
						TCHAR cause[256] = { 0 };
						InternetGetLastResponseInfo(&errnum, cause, &size);
						if (_tcslen(cause) > 0)
							msg.ajoute(UtilWin::ToUTF8(cause));
					}
					else
					{
						CInternetException e(errnum);
						msg += UtilWin::SYGetMessage(e);
					}
				}
				END_CATCH_ALL
			}
		}


		if (bRep)
		{
			const short MAX_READ_SIZE = 4096;
			pURLFile->SetReadBufferSize(MAX_READ_SIZE);

			std::string tmp;
			tmp.resize(MAX_READ_SIZE);
			UINT charRead = 0;
			while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE))>0)
			{
				//tmp.ReleaseBuffer();
				text.append(tmp.c_str(), charRead);
			}


			//convert UTF8 to UTF16
			//size_t test1 = text.size();
			//size_t test2 = text.length();
			//int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);

			//std::wstring out;
			//out.resize(len);
			////WCHAR *ptr = textOut.GetBufferSetLength(len); ASSERT(ptr);
			//
			//	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &(out[0]), len);
			//
			////Convert UTF16 to ANSI
			//int newLen = WideCharToMultiByte(CP_THREAD_ACP, 0, out.c_str(), -1, NULL, 0, 0, 0);
			//text.resize(newLen);
			//WideCharwToMultiByte(CP_THREAD_ACP, 0, out.c_str(), -1, NULL, 0, 0, 0);

			//Convert UTF16 to ANSI
			//text = WBSF::UTF8(out);

			//decode HTML code
			if (bConvert)
			{
				decode_html_entities_utf8(&(text[0]), NULL);

				//Convert UTF8 to UTF16 
				int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);

				std::wstring w_text;
				w_text.resize(len);
				MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &(w_text[0]), len);

				//Convert UTF16 to Windows-1252 encoding
				int newLen = WideCharToMultiByte(CP_ACP, 0, w_text.c_str(), -1, NULL, 0, 0, 0);

				text.resize(newLen);
				WideCharToMultiByte(CP_ACP, 0, w_text.c_str(), -1, &(text[0]), newLen, 0, 0);
				text.resize(strlen(text.c_str()));
			}

			//textOut.ReleaseBuffer();

			//text.Replace(_T("&nbsp;"), _T(""));
			////text.Remove('\r');
			////text.Remove('\n');
			//if( replaceAccent )
			//{

			//	//lowercase
			//	text.Replace(_T("&egrave;"), _T("è"));
			//	text.Replace(_T("&eacute;"), _T("é"));
			//	text.Replace(_T("&ocirc;"), _T("ô"));
			//	text.Replace(_T("&icirc;"), _T("î"));
			//	text.Replace(_T("&ecirc;"), _T("ê"));
			//	text.Replace(_T("&euml;"), _T("ë"));
			//	text.Replace(_T("&ccedil;"), _T("ç"));
			//	

			//	//uppercase 
			//	text.Replace(_T("&Egrave;"), _T("È"));
			//	text.Replace(_T("&Eacute;"), _T("É"));
			//	text.Replace(_T("&Ocirc;"), _T("Ô"));
			//	text.Replace(_T("&Icirc;"), _T("Î"));
			//	text.Replace(_T("&Ecirc;"), _T("Ê"));
			//	text.Replace(_T("&Euml;"), _T("Ë"));
			//	text.Replace(_T("&Ccedil;"), _T("Ç"));
			//}

			pURLFile->Close();
		}
		else
		{
			CString tmp;
			tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
			msg.ajoute(UtilWin::ToUTF8(tmp));
		}

		delete pURLFile;
		return msg;
	}

	ERMsg CUINewBrunswick::ExecuteAgriculture(CCallback& callback)
	{
		ERMsg msg;

		//string workingDir = GetDir(WORKING_DIR);
		//msg = CreateMultipleDir(workingDir);

		//callback.AddMessage(GetString(IDS_UPDATE_DIR));
		//callback.AddMessage(workingDir, 1);
		//callback.AddMessage(GetString(IDS_UPDATE_FROM));
		//callback.AddMessage(string(SERVER_NAME[AGRI]), 1);
		//callback.AddMessage("");


		//StringVector fileList;
		//GetFileList(AGRI, fileList, callback);

		//callback.PushTask("Download New Brunswick agriculture data", fileList.size());


		//size_t curI = 0;
		//bool bDownloaded = false;

		//CInternetSessionPtr pSession;
		//CHttpConnectionPtr pConnection;

		//msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		//if (msg)
		//{
		//	TRY
		//	{
		//		for (size_t i = curI; i < fileList.size() && msg; i++)
		//		{
		//			//string URL = "/010-001/archive.aspx? HTTP/1.1";
		//			string URL = "010-001/archive.aspx?";
		//			string header = "Content-Type: application/x-www-form-urlencoded";
		//			//string data = "__EVENTTARGET=&__EVENTARGUMENT=&__VIEWSTATE=%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%2Fkx8tP9Qp93CvCxoYUfKz%2B%2F6A%3D%3D&__VIEWSTATEGENERATOR=7FE89812&__EVENTVALIDATION=%2FwEWhAEC6vCn2AwCjPGn3gECkN%2Fz0AcC7Of0OwL3wr3hAgLmha0BAtCk16ACAtCkn6MCAtak%2B6ACAtCk%2B6ACAtCkz6ACAtek%2F6ACAtekz6ACAtGky6ACAtGk16ACAtekn6MCAtCk%2F6ACAtak16ACAtakk6MCAtWk86ACAtaky6ACAtakz6ACAtekx6ACAtWkx6ACAtakn6MCAuLr9sIDAuPr9sIDAuDr9sIDAuHr9sIDAubr9sIDAufr9sIDAuTr9sIDAvXr9sIDAvrr9sIDAuLrtsEDAuLrusEDAuLrvsEDAuLrgsEDAuLrhsEDAuLrisEDAuLrjsEDAuLrksEDAuLr1sIDAuLr2sIDAuPrtsEDAuPrusEDAuPrvsEDAuPrgsEDAuPrhsEDAuPrisEDAuPrjsEDAuPrksEDAuPr1sIDAuPr2sIDAuDrtsEDAuDrusEDAtXQoYkBAtTQoYkBAtfQoYkBAtbQoYkBAtHQoYkBAtDQoYkBAtPQoYkBAsLQoYkBAs3QoYkBAtXQ4YoBAtXQ7YoBAtXQ6YoBApTRytAPApTR5rkJApTR0twBAv%2Fo4MUGAv%2Fo3JgNAv%2FoyL8EAv%2FopNIMAv%2FokOkLAv%2FojIwCAvKEiO4EAvS6t0wC9bq3TAL2urdMAve6t0wC8Lq3TALxurdMAvK6t0wC47q3TALsurdMAvS6908C9Lr7TwL0uv9PAvS6w08C9LrHTwL0ustPAvS6z08C9LrTTwL0updMAvS6m0wC9br3TwL1uvtPAvW6%2F08C9brDTwL1usdPAvW6y08C9brPTwL1utNPAvW6l0wC9bqbTAL2uvdPAva6%2B08C14iy%2BgQC1oiy%2BgQC1Yiy%2BgQC1Iiy%2BgQC04iy%2BgQC0oiy%2BgQC0Yiy%2BgQCwIiy%2BgQCz4iy%2BgQC14jy%2BQQC14j%2B%2BQQC14j6%2BQQCrte4hQQCrteU7AICrtegiQoCxe6SkA0Cxe6uzQYCxe666g8Cxe7WhwcCxe7iPALF7v7ZCQL0x6SlAQKA3uC2A0LNcpbYprj4QvUiSHNkEUzsHvnf&ctl00%24hfLang=fr-CA&ctl00%24Content1%24ddlWS=47&ctl00%24Content1%24ddlFromDay=1&ctl00%24Content1%24ddlFromMonth=1&ctl00%24Content1%24ddlFromYear=2015&ctl00%24Content1%24hdnFromDate=&ctl00%24Content1%24ddlToDay=2&ctl00%24Content1%24ddlToMonth=1&ctl00%24Content1%24ddlToYear=2015&ctl00%24Content1%24hdnToDate=&ctl00%24Content1%24btnGetData=Submit";
		//			string data =   "__EVENTTARGET=&__EVENTARGUMENT=&__VIEWSTATE=%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%2Fkx8tP9Qp93CvCxoYUfKz%2B%2F6A%3D%3D&__VIEWSTATEGENERATOR=7FE89812&__EVENTVALIDATION=%2FwEWhAEC6vCn2AwCjPGn3gECkN%2Fz0AcC7Of0OwL3wr3hAgLmha0BAtCk16ACAtCkn6MCAtak%2B6ACAtCk%2B6ACAtCkz6ACAtek%2F6ACAtekz6ACAtGky6ACAtGk16ACAtekn6MCAtCk%2F6ACAtak16ACAtakk6MCAtWk86ACAtaky6ACAtakz6ACAtekx6ACAtWkx6ACAtakn6MCAuLr9sIDAuPr9sIDAuDr9sIDAuHr9sIDAubr9sIDAufr9sIDAuTr9sIDAvXr9sIDAvrr9sIDAuLrtsEDAuLrusEDAuLrvsEDAuLrgsEDAuLrhsEDAuLrisEDAuLrjsEDAuLrksEDAuLr1sIDAuLr2sIDAuPrtsEDAuPrusEDAuPrvsEDAuPrgsEDAuPrhsEDAuPrisEDAuPrjsEDAuPrksEDAuPr1sIDAuPr2sIDAuDrtsEDAuDrusEDAtXQoYkBAtTQoYkBAtfQoYkBAtbQoYkBAtHQoYkBAtDQoYkBAtPQoYkBAsLQoYkBAs3QoYkBAtXQ4YoBAtXQ7YoBAtXQ6YoBApTRytAPApTR5rkJApTR0twBAv%2Fo4MUGAv%2Fo3JgNAv%2FoyL8EAv%2FopNIMAv%2FokOkLAv%2FojIwCAvKEiO4EAvS6t0wC9bq3TAL2urdMAve6t0wC8Lq3TALxurdMAvK6t0wC47q3TALsurdMAvS6908C9Lr7TwL0uv9PAvS6w08C9LrHTwL0ustPAvS6z08C9LrTTwL0updMAvS6m0wC9br3TwL1uvtPAvW6%2F08C9brDTwL1usdPAvW6y08C9brPTwL1utNPAvW6l0wC9bqbTAL2uvdPAva6%2B08C14iy%2BgQC1oiy%2BgQC1Yiy%2BgQC1Iiy%2BgQC04iy%2BgQC0oiy%2BgQC0Yiy%2BgQCwIiy%2BgQCz4iy%2BgQC14jy%2BQQC14j%2B%2BQQC14j6%2BQQCrte4hQQCrteU7AICrtegiQoCxe6SkA0Cxe6uzQYCxe666g8Cxe7WhwcCxe7iPALF7v7ZCQL0x6SlAQKA3uC2A0LNcpbYprj4QvUiSHNkEUzsHvnf&ctl00%24hfLang=fr-CA&ctl00%24Content1%24ddlWS=47&ctl00%24Content1%24ddlFromDay=1&ctl00%24Content1%24ddlFromMonth=1&ctl00%24Content1%24ddlFromYear=2015&ctl00%24Content1%24hdnFromDate=&ctl00%24Content1%24ddlToDay=2&ctl00%24Content1%24ddlToMonth=1&ctl00%24Content1%24ddlToYear=2015&ctl00%24Content1%24hdnToDate=&ctl00%24Content1%24btnGetData=Submit";
		//			//string data = "__EVENTTARGET:\r\n"
		//			//	"__EVENTARGUMENT:\r\n"
		//			//	"__VIEWSTATE:/wEPDwUKLTk1NTAxMjAxMA9kFgJmD2QWAgIED2QWAgIFD2QWBAIHDw9kDxAWA2YCAQICFgMWAh4OUGFyYW1ldGVyVmFsdWUFAjQ3FgIfAGUWAh8AZRYDZmZmZGQCCQ9kFhACBQ8QZA8WE2YCAQICAgMCBAIFAgYCBwIIAgkCCgILAgwCDQIOAg8CEAIRAhIWExAFDUFiZXJkZWVuICg0NykFAjQ3ZxAFDEFuZG92ZXIgKDQ5KQUCNDlnEAUNQnJpZ2h0b24gKDYyKQUCNjJnEAUNRHJ1bW1vbmQgKDQyKQUCNDJnEAUNRHJ1bW1vbmQgKDQ1KQUCNDVnEAUNRHJ1bW1vbmQgKDUxKQUCNTFnEAUNRHJ1bW1vbmQgKDU1KQUCNTVnEAULR29yZG9uICgzNikFAjM2ZxAFCUtlbnQgKDM3KQUCMzdnEAUNUmljaG1vbmQgKDU5KQUCNTlnEAUQU2FpbnQtQW5kcmUgKDQxKQUCNDFnEAUQU2FpbnQtQW5kcmUgKDY3KQUCNjdnEAUSU2FpbnQtTGVvbmFyZCAoNjgpBQI2OGcQBQxTaW1vbmRzICg3MCkFAjcwZxAFDldha2VmaWVsZCAoNjYpBQI2NmcQBQxXaWNrbG93ICg2NSkFAjY1ZxAFC1dpbG1vdCAoNTMpBQI1M2cQBQtXaWxtb3QgKDczKQUCNzNnEAUOV29vZHN0b2NrICg2OSkFAjY5Z2RkAgkPEGQPFh9mAgECAgIDAgQCBQIGAgcCCAIJAgoCCwIMAg0CDgIPAhACEQISAhMCFAIVAhYCFwIYAhkCGgIbAhwCHQIeFh8QBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmcQBQIxMwUCMTNnEAUCMTQFAjE0ZxAFAjE1BQIxNWcQBQIxNgUCMTZnEAUCMTcFAjE3ZxAFAjE4BQIxOGcQBQIxOQUCMTlnEAUCMjAFAjIwZxAFAjIxBQIyMWcQBQIyMgUCMjJnEAUCMjMFAjIzZxAFAjI0BQIyNGcQBQIyNQUCMjVnEAUCMjYFAjI2ZxAFAjI3BQIyN2cQBQIyOAUCMjhnEAUCMjkFAjI5ZxAFAjMwBQIzMGcQBQIzMQUCMzFnZGQCCw8QZA8WDGYCAQICAgMCBAIFAgYCBwIIAgkCCgILFgwQBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmdkZAINDxBkDxYJZgIBAgICAwIEAgUCBgIHAggWCRAFBDIwMDcFBDIwMDdnEAUEMjAwOAUEMjAwOGcQBQQyMDA5BQQyMDA5ZxAFBDIwMTAFBDIwMTBnEAUEMjAxMQUEMjAxMWcQBQQyMDEyBQQyMDEyZxAFBDIwMTMFBDIwMTNnEAUEMjAxNAUEMjAxNGcQBQQyMDE1BQQyMDE1Z2RkAhMPEGQPFh9mAgECAgIDAgQCBQIGAgcCCAIJAgoCCwIMAg0CDgIPAhACEQISAhMCFAIVAhYCFwIYAhkCGgIbAhwCHQIeFh8QBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmcQBQIxMwUCMTNnEAUCMTQFAjE0ZxAFAjE1BQIxNWcQBQIxNgUCMTZnEAUCMTcFAjE3ZxAFAjE4BQIxOGcQBQIxOQUCMTlnEAUCMjAFAjIwZxAFAjIxBQIyMWcQBQIyMgUCMjJnEAUCMjMFAjIzZxAFAjI0BQIyNGcQBQIyNQUCMjVnEAUCMjYFAjI2ZxAFAjI3BQIyN2cQBQIyOAUCMjhnEAUCMjkFAjI5ZxAFAjMwBQIzMGcQBQIzMQUCMzFnZGQCFQ8QZA8WDGYCAQICAgMCBAIFAgYCBwIIAgkCCgILFgwQBQExBQExZxAFATIFATJnEAUBMwUBM2cQBQE0BQE0ZxAFATUFATVnEAUBNgUBNmcQBQE3BQE3ZxAFATgFAThnEAUBOQUBOWcQBQIxMAUCMTBnEAUCMTEFAjExZxAFAjEyBQIxMmdkZAIXDxBkDxYJZgIBAgICAwIEAgUCBgIHAggWCRAFBDIwMDcFBDIwMDdnEAUEMjAwOAUEMjAwOGcQBQQyMDA5BQQyMDA5ZxAFBDIwMTAFBDIwMTBnEAUEMjAxMQUEMjAxMWcQBQQyMDEyBQQyMDEyZxAFBDIwMTMFBDIwMTNnEAUEMjAxNAUEMjAxNGcQBQQyMDE1BQQyMDE1Z2RkAh8PPCsADQBkGAEFHGN0bDAwJENvbnRlbnQxJGd2QXJjaGl2ZURhdGEPZ2ToC/kx8tP9Qp93CvCxoYUfKz+/6A==\r\n"
		//			//	"__VIEWSTATEGENERATOR:7FE89812\r\n"
		//			//	"__EVENTVALIDATION:/wEWhAEC6vCn2AwCjPGn3gECkN/z0AcC7Of0OwL3wr3hAgLmha0BAtCk16ACAtCkn6MCAtak+6ACAtCk+6ACAtCkz6ACAtek/6ACAtekz6ACAtGky6ACAtGk16ACAtekn6MCAtCk/6ACAtak16ACAtakk6MCAtWk86ACAtaky6ACAtakz6ACAtekx6ACAtWkx6ACAtakn6MCAuLr9sIDAuPr9sIDAuDr9sIDAuHr9sIDAubr9sIDAufr9sIDAuTr9sIDAvXr9sIDAvrr9sIDAuLrtsEDAuLrusEDAuLrvsEDAuLrgsEDAuLrhsEDAuLrisEDAuLrjsEDAuLrksEDAuLr1sIDAuLr2sIDAuPrtsEDAuPrusEDAuPrvsEDAuPrgsEDAuPrhsEDAuPrisEDAuPrjsEDAuPrksEDAuPr1sIDAuPr2sIDAuDrtsEDAuDrusEDAtXQoYkBAtTQoYkBAtfQoYkBAtbQoYkBAtHQoYkBAtDQoYkBAtPQoYkBAsLQoYkBAs3QoYkBAtXQ4YoBAtXQ7YoBAtXQ6YoBApTRytAPApTR5rkJApTR0twBAv/o4MUGAv/o3JgNAv/oyL8EAv/opNIMAv/okOkLAv/ojIwCAvKEiO4EAvS6t0wC9bq3TAL2urdMAve6t0wC8Lq3TALxurdMAvK6t0wC47q3TALsurdMAvS6908C9Lr7TwL0uv9PAvS6w08C9LrHTwL0ustPAvS6z08C9LrTTwL0updMAvS6m0wC9br3TwL1uvtPAvW6/08C9brDTwL1usdPAvW6y08C9brPTwL1utNPAvW6l0wC9bqbTAL2uvdPAva6+08C14iy+gQC1oiy+gQC1Yiy+gQC1Iiy+gQC04iy+gQC0oiy+gQC0Yiy+gQCwIiy+gQCz4iy+gQC14jy+QQC14j++QQC14j6+QQCrte4hQQCrteU7AICrtegiQoCxe6SkA0Cxe6uzQYCxe666g8Cxe7WhwcCxe7iPALF7v7ZCQL0x6SlAQKA3uC2A0LNcpbYprj4QvUiSHNkEUzsHvnf\r\n"
		//			//	"ctl00$hfLang:fr-CA\r\n"
		//			//	"ctl00$Content1$ddlWS:47\r\n"
		//			//	"ctl00$Content1$ddlFromDay:1\r\n"
		//			//	"ctl00$Content1$ddlFromMonth:1\r\n"
		//			//	"ctl00$Content1$ddlFromYear:2015\r\n"
		//			//	"ctl00$Content1$hdnFromDate:\r\n"
		//			//	"ctl00$Content1$ddlToDay:2\r\n"
		//			//	"ctl00$Content1$ddlToMonth:1\r\n"
		//			//	"ctl00$Content1$ddlToYear:2015\r\n"
		//			//	"ctl00$Content1$hdnToDate:\r\n"
		//			//	"ctl00$Content1$btnGetData:Submit\r\n";



		//			//string str;
		//			//msg = PostPageText(pConnection, URL, header, data, str);


		//			////split data in seperate files
		//			//if (msg)
		//			//{
		//			//	string::size_type pos1 = str.find("<TABLE");
		//			//	string::size_type pos2 = str.find("</TABLE>");

		//			//	if (pos1 != string::npos && pos2 != string::npos)
		//			//	{
		//			//		string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
		//			//		msg += SaveStation(fileList[i], tmp);
		//			//	}

		//			//	curI++;
		//			//	msg += callback.StepIt();
		//			//	bDownloaded = true;
		//			//}
		//		}
		//	}
		//		CATCH_ALL(e)
		//	{
		//		msg = UtilWin::SYGetMessage(*e);
		//	}
		//	END_CATCH_ALL

		//		//clean connection
		//		pConnection->Close();
		//	pSession->Close();
		//}
		//

		//callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		//callback.PopTask();

		return msg;
	}

	ERMsg CUINewBrunswick::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]), 1);
		callback.AddMessage("");




		StringVector fileList;
		GetFileList(FIRE, fileList, callback);

		callback.PushTask("Download New Brunswick data", fileList.size());


		size_t curI = 0;
		//size_t nbRun = 0;
		bool bDownloaded = false;

		//while (curI<fileList.size() && nbRun < 5 && msg)
		//{
		//nbRun++;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{
			TRY
			{
				for (size_t i = curI; i < fileList.size() && msg; i++)
				{
					string URL = "0079/FireWeather/FireWeatherHourly-e.asp?stn=" + fileList[i];
					string str;
					msg = UtilWWW::GetPageText(pConnection, URL, str);


					//split data in seperate files
					if (msg)
					{
						string::size_type pos1 = str.find("<TABLE");
						string::size_type pos2 = str.find("</TABLE>");

						if (pos1 != string::npos && pos2 != string::npos)
						{
							string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
							msg += SaveStation(fileList[i], tmp);
						}

						curI++;
						msg += callback.StepIt();
						bDownloaded = true;
					}
				}
			}
			CATCH_ALL(e)
			{
				msg = UtilWin::SYGetMessage(*e);
			}
			END_CATCH_ALL

			//clean connection
			pConnection->Close();
			pSession->Close();
		}
		//else
		//{
		//	if (nbRun > 1 && nbRun < 5)
		//	{
		//		callback.PushTask("Waiting 30 seconds for server...", 600);
		//		for (size_t i = 0; i < 600 && msg; i++)
		//		{
		//			Sleep(50);//wait 50 milisec
		//			msg += callback.StepIt();
		//		}
		//		callback.PopTask();
		//	}
		//}


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		callback.PopTask();

		return msg;
	}


	string CUINewBrunswick::GetOutputFilePath(size_t n, const string& ID, int year)const
	{
		static const char* NETWORK_NAME[NB_NETWORKS] {"Fire", "Agriculture"};
		return GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\Hourly\\" + ToString(year) + "\\" + ID + ".csv";
	}

	ERMsg CUINewBrunswick::SaveStation(const std::string& ID, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);

	
		enum THourlyColumns{ C_DATE, C_TIME, C_TEMP, C_RH, C_DIR, C_WSPD, C_MAXSPD, C_RAIN, C_RAIN24, C_FFMC, C_ISI, C_FWI, NB_COLUMNS };
		static const TVarH COL_POS[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR, H_RELH, H_WNDD, H_WNDS, H_SKIP, H_PRCP, H_SKIP, H_SKIP, H_SKIP, H_SKIP };
		
		bool bLoadData = true;

//		zen::XmlDoc doc;
		try
		{
			WBSF::ReplaceString(str, "\t", "");
			WBSF::ReplaceString(str, "<BR>", "");
			WBSF::ReplaceString(str, "</TR>", "</TR>\r\n");
			WBSF::ReplaceString(str, "\r\n\r\n", "\r\n");
			WBSF::ReplaceString(str, "\r\n\r\n", "\r\n");
			WBSF::ReplaceString(str, " nowrap align=center", "");
			WBSF::ReplaceString(str, " nowrap align=right", "");
			
			zen::XmlDoc doc = zen::parse(str);



			zen::XmlIn in(doc.root());
			for (zen::XmlIn itTR = in["TR"]; itTR; itTR.next())
			{
				StringVector tmp;
				for (zen::XmlIn itTD = itTR["TD"]; itTD; itTD.next())
				{
					string value;
					itTD(value);
					tmp.push_back(value);
				}//for all columns
				

				if (tmp.size() == NB_COLUMNS)
				{

					StringVector date(tmp[C_DATE], "-");
					StringVector time(tmp[C_TIME], ":");

					int year = ToInt(date[0]);
					size_t month = ToInt(date[1]) - 1;
					size_t day = ToInt(date[2]) - 1;
					size_t hour = ToInt(time[0]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef = CTRef(year, month, day, hour);

					if (bLoadData)
					{
						string filePath = GetOutputFilePath(FIRE, ID, year);
						data.LoadData(filePath, -999, false);//don't erase other years when multiple years
						bLoadData = false;
					}

					for (size_t i = 0; i < NB_COLUMNS; i++)
					{
						if (COL_POS[i] != H_SKIP)
						{
							double value = ToDouble(tmp[i]);
							if (value > -99)
							{
								data.GetHour(TRef).SetStat(COL_POS[i], value);
								if (COL_POS[i] == H_RELH)
								{
									double T = data[TRef][H_TAIR][MEAN];
									double Hr = value;
									if (T > -99 && Hr)
										data.GetHour(TRef).SetStat(H_TDEW, Hr2Td(T, Hr));
								}
							}
						}
					}
				}
			}


			if (msg)
			{
				//save data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					string filePath = GetOutputFilePath(FIRE, ID, it1->first);
					string outputPath = GetPath(filePath);
					CreateMultipleDir(outputPath);
					it1->second->SaveData(filePath);
				}
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}

	
		return msg;
	}

	std::string CUINewBrunswick::GetStationListFilePath()const
	{
		
		return WBSF::GetApplicationPath() + "Layers\\NBStations.csv";
	}

	ERMsg CUINewBrunswick::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		
		msg = m_stations.Load(GetStationListFilePath());

		if (msg)
			msg += m_stations.IsValid();

		if (msg)
		{
			for (size_t i = 0; i < m_stations.size(); i++)
				stationList.push_back(m_stations[i].m_ID);
		}

		return msg;
	}

	ERMsg CUINewBrunswick::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		size_t it = m_stations.FindByID(ID);
		if (it == NOT_INIT)
		{
			msg.ajoute(FormatMsg(IDS_NO_STATION_INFORMATION, ID));
			return msg;
		}
		
		((CLocation&)station) = m_stations[it];

		size_t n = FIRE;//station.GetSSI("NETWORK");
		//station.m_name = TraitFileName(station.m_name);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}
}


