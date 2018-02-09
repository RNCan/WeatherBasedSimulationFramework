
// BioSIMDoc.cpp : implementation of the CBioSIMDoc class
//

#include "stdafx.h"


#include "Basic/UtilTime.h"
#include "Basic/callback.h"

#include "Basic/Shore.h"
#include "FileManager/FileManager.h"
#include "Simulation/WeatherGradient.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/ProgressStepDlg.h"	
#include "Geomatic/TimeZones.h"
#include "Geomatic/ShoreCreator.h"

#include "BioSIM.h"
#include "BioSIMDoc.h"
#include "MainFrm.h"
#include "OutputView.h"


using namespace std;
using namespace WBSF;
using namespace UtilWin;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//****************************************************************
// CBioSIMDoc

IMPLEMENT_DYNCREATE(CBioSIMDoc, CDocument)

BEGIN_MESSAGE_MAP(CBioSIMDoc, CDocument)
END_MESSAGE_MAP()


// CBioSIMDoc construction/destruction

CBioSIMDoc::CBioSIMDoc()
{
	m_bInit = false;
	m_bExecute = false;
	CBioSIMProject* pProject = new CBioSIMProject;
	pProject->SetMyself(&m_projectPtr);
	m_projectPtr.reset(pProject);
	m_projectState.reset(new CProjectState);
}

CBioSIMDoc::~CBioSIMDoc()
{
}

BOOL CBioSIMDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	GetProject().Reset();
	m_lastSaveProject = GetProject();

	m_projectState->clear();
	m_curSel = "Project";//select project at the beginning 

	m_bInit = false;

	//UpdateAllViews(NULL, INIT);

	/*CStdioFile file1(_T("H:\\Travaux\\Dispersal2007\\Weather\\WRFdata\\wrfbud2_000.txt"), CFile::modeRead);
	CStdioFile file2(_T("H:\\Travaux\\Dispersal2007\\Input\\wrfbud2_000.csv"), CFile::modeWrite | CFile::modeCreate);
	file2.WriteString(_T("ID,Latitude,Longitude"));
	CString line;
	int i = 0; 
	while (file1.ReadString(line))
	{
		if (i % 38 == 0)
		{
			int pos = 0;
			CString lat = line.Tokenize(_T(" "), pos);
			CString lon = line.Tokenize(_T(" "), pos);
			CString ID = line.Tokenize(_T(" "), pos);
			CString line2;
			line2.Format(_T("%s,%s,%s\n"), ID, lat, lon);
			file2.WriteString(line2);
		}
		i++;
	}

	file1.Close();
	file2.Close();*/
	/*	CStdioFile file1("D:\\project\\models\\MPB\\Hourly\\Data\\snran.txt", CFile::modeRead);
		CStdioFile file2("D:\\project\\models\\MPB\\Hourly\\Data\\snras.txt", CFile::modeRead);
		CStdioFile file3("D:\\project\\models\\MPB\\Hourly\\Data\\snra1992-2004.txt", CFile::modeWrite|CFile::modeCreate);
		file3.WriteString("Year\tJD\tTime\tNorthBl\tSouthBl\tAverBl\n");
		CString line1;
		CString line2;
		VERIFY( file1.ReadString(line1) );
		VERIFY( file2.ReadString(line2) );
		int pos1 = 0;
		int pos2 = 0;

		int y = 1992;
		int jd = 199;
		int h = 0;
		while( pos1!=-1 && pos2!=-1 )
		{
		CString t1 = line1.Tokenize(" ", pos1);
		CString t2 = line2.Tokenize(" ", pos2);
		if( !t1.IsEmpty() && !t2.IsEmpty() )
		{
		CString line3;
		line3.Format("%d\t%d\t%d\t%.1lf\t%.1lf\t%.1lf\n", y, jd+1, h*100,atof(t1), atof(t2), (atof(t1)+atof(t2))/2);
		file3.WriteString(line3);
		}

		h = (++h)%24;
		if( h==0)
		jd++;

		if( jd == WBSF::GetNbDay(y) )
		{
		jd=0;
		y++;
		}
		}
		*/

	return TRUE;
}




// CBioSIMDoc serialization
BOOL CBioSIMDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	ENSURE(lpszPathName);

	string filePath = UtilWin::ToUTF8(lpszPathName);

	ERMsg msg = GetProject().Load(filePath);
	if (msg)
	{
		m_lastSaveProject = GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));

		//here : try to conver old loc file to new csv file
		GetFileManager().Loc().ConvertLoc2CSV();

		m_bInit = true;
		m_curSel = "Project";

		LoadProjectState(lpszPathName);
		//UpdateAllViews(NULL, INIT);
	}
	else
	{
		SYShowMessage(msg, ::AfxGetMainWnd());
	}




	return msg != 0;
}


BOOL CBioSIMDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	string filePath = UtilWin::ToUTF8(lpszPathName);
	bool newProject = !FileExists(filePath);
	ERMsg msg = GetProject().Save(filePath);
	if (msg)
	{
		m_lastSaveProject = GetProject();
		GetFileManager().SetProjectPath(GetPath(filePath));
		GetFileManager().CreateDefaultDirectories();

		if (newProject)
		{
			filePath = GetFileManager().WeatherUpdate().GetLocalPath() + "Download Current Normals Database.Update";
			if (!FileExists(filePath))
			{
				ofStream file;
				if (file.open(filePath))
				{
					file << "<?xml version=\"1.0\" encoding=\"Windows - 1252\"?>" << endl;
					file << "<WeatherUpdater version=\"2\">" << endl;
					file << "<Tasks type=\"Tools\">" << endl;
					file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
					file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
					file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
					file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
					file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
					file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
					file << "<Parameters name=\"Local\">tmp\\Canada-USA_1981-2010.zip</Parameters>" << endl;
					file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
					file << "<Parameters name=\"Password\"/>" << endl;
					file << "<Parameters name=\"Proxy\"/>" << endl;
					file << "<Parameters name=\"Remote\">regniere/Data11/Weather/Normals/Canada-USA_1981-2010.zip</Parameters>" << endl;
					file << "<Parameters name=\"Server\">ftp.cfl.scf.rncan.gc.ca</Parameters>" << endl;
					file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
					file << "<Parameters name=\"UserName\"/>" << endl;
					file << "</Task>" << endl;
					file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
					file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
					file << "<Parameters name=\"Command\">1</Parameters>" << endl;
					file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
					file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
					file << "<Parameters name=\"ZipFilepath\">tmp\\Canada-USA_1981-2010.zip</Parameters>" << endl;
					file << "</Task>" << endl;
					file << "</Tasks>" << endl;
					file << "</WeatherUpdater>" << endl;

					file.close();
				}
			}

			filePath = GetFileManager().WeatherUpdate().GetLocalPath() + "Download Current Daily Database.Update";
			if (!FileExists(filePath))
			{
				ofStream file;
				if (file.open(filePath))
				{
					//
					file << "<?xml version=\"1.0\" encoding=\"Windows - 1252\"?>" << endl;
					file << "<WeatherUpdater version=\"2\">" << endl;
					file << "<Tasks type=\"Tools\">" << endl;
					file << "<Task execute=\"true\" name=\"DownloadFile\" type=\"FTPTransfer\">" << endl;
					file << "<Parameters name=\"Ascii\">0</Parameters>" << endl;
					file << "<Parameters name=\"Connection\">0</Parameters>" << endl;
					file << "<Parameters name=\"ConnectionTimeout\">15000</Parameters>" << endl;
					file << "<Parameters name=\"Direction\">0</Parameters>" << endl;
					file << "<Parameters name=\"Limit\">0</Parameters>" << endl;
					file << "<Parameters name=\"Local\">tmp\\Canada_2016-2017.zip</Parameters>" << endl;
					file << "<Parameters name=\"Passive\">1</Parameters>" << endl;
					file << "<Parameters name=\"Password\"/>" << endl;
					file << "<Parameters name=\"Proxy\"/>" << endl;
					file << "<Parameters name=\"Remote\">regniere/Data11/Weather/Daily/Canada_2016-2017.zip</Parameters>" << endl;
					file << "<Parameters name=\"Server\">ftp.cfl.scf.rncan.gc.ca</Parameters>" << endl;
					file << "<Parameters name=\"ShowProgress\">0</Parameters>" << endl;
					file << "<Parameters name=\"UserName\"/>" << endl;
					file << "</Task>" << endl;
					file << "<Task execute=\"true\" name=\"UnzipFile\" type=\"ZipUnzip\">" << endl;
					file << "<Parameters name=\"AddSubDirectory\">0</Parameters>" << endl;
					file << "<Parameters name=\"Command\">1</Parameters>" << endl;
					file << "<Parameters name=\"Directory\">..\\Weather\\</Parameters>" << endl;
					file << "<Parameters name=\"Filter\">*.*</Parameters>" << endl;
					file << "<Parameters name=\"ZipFilepath\">tmp\\Canada_2016-2017.zip</Parameters>" << endl;
					file << "</Task>" << endl;
					file << "</Tasks>" << endl;
					file << "</WeatherUpdater>" << endl;
					file.close();
				}
			}
		}

		m_bInit = true;

		SaveProjectState(lpszPathName);
	}

	return msg != 0;
}

void CBioSIMDoc::OnCloseDocument()
{
	//UpdateAllViews(NULL, CLOSE, NULL);

	CDocument::OnCloseDocument();
}

void CBioSIMDoc::LoadProjectState(const CString& filePath)
{
	//save state of the document
	string stateFilePath((CStringA)filePath);
	SetFileExtension(stateFilePath, ".states");
	m_projectState->Load(stateFilePath);
	m_curSel = "Project";

}

void CBioSIMDoc::SaveProjectState(const CString& filePath)
{
	//save state of the document
	string stateFilePath((CStringA)filePath);
	SetFileExtension(stateFilePath, ".states");
	m_projectState->Save(stateFilePath);

}
BOOL CBioSIMDoc::SaveModified() // return TRUE if ok to continue
{
	//UpdateAllViews(NULL, SEL_CHANGE);//reset all view in execution mode

	CString filePath = GetPathName();
	if (!filePath.IsEmpty())
		SaveProjectState(filePath);

	const CBioSIMProject& project = GetProject();
	bool bModified = project != m_lastSaveProject;
	SetModifiedFlag(bModified);

	return CDocument::SaveModified();
}

void CBioSIMDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CBioSIMDoc diagnostics

#ifdef _DEBUG
void CBioSIMDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBioSIMDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CBioSIMDoc commands
UINT CBioSIMDoc::ExecuteTask(void* pParam)
{
	CProgressStepDlgParam* pMyParam = (CProgressStepDlgParam*)pParam;
	CBioSIMProject* pProject = (CBioSIMProject*)pMyParam->m_pThis;
	CFileManager* pFM = (CFileManager*)pMyParam->m_pExtra;

	ERMsg* pMsg = pMyParam->m_pMsg;
	CCallback* pCallback = pMyParam->m_pCallback;

	VERIFY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK);
	TRY
	{
		*pMsg = pProject->Execute(*pFM, *pCallback);
	}
		CATCH_ALL(e)
	{
		*pMsg = ::SYGetMessage(*e);
	}
	END_CATCH_ALL

		CoUninitialize();

	if (*pMsg)
		return 0;

	return -1;
}

ERMsg CBioSIMDoc::Execute(CComPtr<ITaskbarList3>& pTaskbarList)
{
	ERMsg msg;

	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	COutputView* pView = (COutputView*)pMainFrm->GetActiveView();



	SetIsExecute(true);
	pView->AdjustLayout();
	UpdateAllViews(NULL, PROJECT_CHANGE, NULL);


	GetProject().LoadDefaultCtrl();


	CProgressWnd& progressWnd = pView->GetProgressWnd();
	progressWnd.SetTaskbarList(pTaskbarList);

	CProgressStepDlgParam param(m_projectPtr.get(), (void*)&m_strPathName, &GetFM());

	TRY
	{
		//try to save 
		CAppOption option;
		if (option.GetProfileBool(_T("SaveAtRun"), false))
			GetProject().Save(UtilWin::ToUTF8(m_strPathName));

		//progressWnd.ShowPane(TRUE, FALSE, TRUE);
		msg = progressWnd.Execute(CBioSIMDoc::ExecuteTask, &param);
		m_lastLog = SYGetOutputCString(msg, progressWnd.GetCallback());
	}
		CATCH_ALL(e)
	{
		msg = SYGetMessage(*e);
		m_lastLog = SYGetOutputCString(msg);
	}
	END_CATCH_ALL


		SetIsExecute(false);
	pView->AdjustLayout();
	UpdateAllViews(NULL, PROJECT_CHANGE, NULL);


	return msg;
}

void CBioSIMDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU);
	SetTitle(GetPathName());
}

void CBioSIMDoc::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CMainFrame* pMainFrm = (CMainFrame*)AfxGetMainWnd();
	pMainFrm->UpdateAllViews(pSender, lHint, pHint);

	CDocument::UpdateAllViews(pSender, lHint, pHint);

}

#include "UI/Common/UtilWWW.h"
#include "json\json11.hpp"
using namespace UtilWWW;
using namespace json11;

void UpdateQuebec();
void CBioSIMDoc::OnInitialUpdate() // called first time after construct
{
	ERMsg msg;
	
	
	//UpdateQuebec();
	//
	//CShoreCreator::ComputeDistance( "D:\\Layers\\coastlines-generalized-3857\\coastlines_z8.shp", "D:\\Travaux\\Weather\\Normals\\Canada-USA 1981-2010.NormalsHdr.csv", "D:\\Travaux\\Weather\\Normals\\Canada-USA 1981-2010.WithShoreDistanceVector.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\gshhg-shp-2.3.6\\GSHHS_shp\\GSHHS10km.shp", "D:\\Travaux\\Weather\\Normals\\Canada-USA 1981-2010.NormalsHdr.csv", "D:\\Travaux\\Weather\\Normals\\Canada-USA 1981-2010.NormalsHdr2.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\gshhg-shp-2.3.6\\GSHHS_shp\\GSHHS10km.shp", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsHdr.csv", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsHdr.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\coastlines-generalized-3857\\coastlines_z8.shp", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsHdr.csv", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsHdr.csv");
	
	//CShoreCreator::ComputeDistance("D:\\Layers\\land-polygons-complete-4326\\land_polygons_lo.shp", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsT100.csv", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsT100.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\land-polygons-complete-4326\\land_polygons_lo.shp", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsP100.csv", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsP100.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\land-polygons-complete-4326\\land_polygons_lo.shp", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsT100.csv", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsT100.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\land-polygons-complete-4326\\land_polygons_lo.shp", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsP100.csv", "D:\\Travaux\\AnalyseGradient\\loc\\NormalsP100.csv");

	//CShoreCreator::Shape2ANN("D:\\Layers\\ne_10m_land\\ne_10m_land_2.shp", GetApplicationPath() + "Layers\\ne_10m.ann");
	//CShoreCreator::AddPoints(GetApplicationPath() + "Layers\\ne_10m.ann", "D:\\Layers\\ne_10m_land\\StationAdded.csv");
	//CShoreCreator::ComputeDistance("D:\\Layers\\ne_10m_land\\ne_10m_land.shp", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsHdr.csv", "D:\\Travaux\\Weather\\Normals\\World++ 1981-2010_ne10.NormalsHdr.csv");
	
	
	//msg = CShore::SetShore(GetApplicationPath() + "Layers\\ne_10m.ann");

	
	
	//CLocation loc1("Vancouver", "Vancouver", 49.229896, -123.234097, 25);
	//CLocation loc2("Tampa", "Tampa", 27.978402, -82.534988, 25);
	//double test = loc1.GetDistance(loc2,false,false);
	//CShoreCreator::Shape2ANN("D:\\Layers\\gshhg-shp-2.3.6\\GSHHS_shp\\GSHHS10km.shp", GetApplicationPath() + "Layers\\GSHHS10km.ann");
	//msg = CShore::SetShore(GetApplicationPath() + "Layers\\GSHHS10km.ann");
	
	
	msg = CShore::SetShore(GetApplicationPath() + "Layers/Shore.ann");


	//CNormalsDatabasePtr normalDB(new CNormalsDatabase);
	//msg = normalDB->Open("D:\\Travaux\\Weather\\Normals\\World++ 1981-2010.NormalsDB", CNormalsDatabase::modeRead, CCallback());
	//////msg = normalDB->Open("D:\\Travaux\\Weather\\Normals\\Canada-USA 1981-2010.NormalsDB", CNormalsDatabase::modeRead, CCallback());
	//CWeatherGradient gradient;
	//gradient.m_variables = "TN TX P TD";
	//gradient.m_bUseShore = true;
	//gradient.SetNormalsDatabase(normalDB);
	//msg = gradient.CreateDefaultGradient("D:\\Travaux\\AnalyseGradient\\default", CCallback());


	//CLocationVector loc0;
	//loc0.Load(GetApplicationPath() + "Layers/SOPFEUStnDesc.csv");

	//CLocationVector loc1;
	//loc1.Load(GetApplicationPath() + "Layers/QuebecStations.csv");

	//CLocationVector loc2;
	//loc2.Load(GetApplicationPath() + "Layers/reseau_agro.csv");

	//
	//size_t miss = 0;
	//CLocationVector loc3;
	//for (size_t i = 0; i < loc0.size(); i++)
	//{
	//	auto pos1 = loc1.FindBySSI("NumID", loc0[i].m_ID, false);
	//	if (pos1 == loc1.end())
	//	//auto pos1 = loc1.FindByID(loc2[i].m_ID);
	//	//if (pos1 == -1)
	//	{
	//		loc3.push_back(loc0[i]);
	//		//double dist_min = 9999999;
	//		//size_t nearest=-1;
	//		//for (size_t j = 0; j < loc2.size(); j++)
	//		//{
	//		//	if (loc2[j].GetDistance(loc0[i], false, false) < dist_min)
	//		//	{
	//		//		dist_min = loc2[j].GetDistance(loc0[i], false, false);
	//		//		nearest = j;
	//		//	}
	//		//}
	//		//
	//		//if (dist_min < 1000)
	//		//{
	//		//	CLocation loc = loc0[i];
	//		//	loc.SetSSI("NumID", loc0[i].m_ID);
	//		//	loc.m_ID = loc2[nearest].m_ID;
	//		//	//loc0[i].m_name = loc2[nearest].m_name;
	//		//	loc3.push_back(loc);
	//		//}
	//		//else
	//		//{
	//		//	miss++;
	//		//}
	//		

	//	}
	//}

	//loc3.Save(GetApplicationPath() + "Layers/missingStations2.csv");



	msg += CTimeZones::Load(GetApplicationPath() + "zoneinfo/time_zones.shp");

	if (!msg)
		SYShowMessage(msg, ::AfxGetMainWnd());

	UpdateAllViews(NULL, INIT, NULL);
}


void UpdateQuebec()
{
	ERMsg msg;
	//CInternetSession GoogleSession;
	//CHttpConnection* pGoogleConnection = GoogleSession.GetHttpConnection(_T("maps.googleapis.com"), INTERNET_DEFAULT_HTTPS_PORT, _T(""), _T(""));

	CInternetSessionPtr pGoogleSession;
	CHttpConnectionPtr pGoogleConnection;
	msg += GetHttpConnection("maps.googleapis.com", pGoogleConnection, pGoogleSession);

	if (msg)
	{

		CLocationVector loc0;
		loc0.Load(GetApplicationPath() + "Layers/missingStations.csv");

		CLocationVector loc1;
		loc1.Load(GetApplicationPath() + "Layers/QuebecStations.csv");

		size_t miss = 0;
		CLocationVector loc3;
		for (size_t i = 0; i < loc0.size(); i++)
		{
			auto pos1 = loc1.FindByID(loc0[i].m_ID);
			if (pos1 == -1)
			{
				CLocation loc = loc0[i];

				string strGeo;
				string URL = "/maps/api/geocode/json?latlng=" + ToString(loc.m_lat) + "," + ToString(loc.m_lon) + "&key=AIzaSyAz8Mr7mO8BkkugAHe7ds65mCEeSUVknZw";
				msg = UtilWWW::GetPageText(pGoogleConnection, URL, strGeo, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
				if (msg)
				{
					//extract elevation from google
					string error;
					Json jsonGeo = Json::parse(strGeo, error);
					ASSERT(jsonGeo.is_object());

					if (error.empty() && jsonGeo["status"] == "OK")
					{
						ASSERT(jsonGeo["results"].is_array());
						Json::array result1 = jsonGeo["results"].array_items();
						if (!result1.empty())
						{
							string name2;
							Json::array result2 = result1[0]["address_components"].array_items();
							for (int j = 0; j < result2.size(); j++)
							{
								Json::array result3 = result2[j]["types"].array_items();
								if (result3.size() == 2 && result3[0] == "locality")
								{
									string str = ANSI_2_ASCII(result2[j]["short_name"].string_value());
									WBSF::ReplaceString(str, ",", " ");
									loc.m_name = WBSF::TrimConst(str);
								}
								if (result3.size() == 2 && result3[0] == "administrative_area_level_3")
								{
									string str = ANSI_2_ASCII(result2[j]["short_name"].string_value());
									WBSF::ReplaceString(str, ",", " ");
									name2 = str;
								}
							}

							if (loc.m_name.empty())
								loc.m_name = name2;
						}
						else
						{
							miss++;
						}
					}
				}//if msg

				ASSERT(!loc.m_name.empty());
				loc3.push_back(loc);
			}
		}

		loc3.Save(GetApplicationPath() + "Layers/missingStations2.csv");
	}


	pGoogleConnection->Close();
	pGoogleSession->Close();

}
