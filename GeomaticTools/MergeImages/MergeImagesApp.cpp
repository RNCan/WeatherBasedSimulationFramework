//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MergeImages.h"

using namespace std;
using namespace WBSF;
//
//int test()
//{
//
//	string line;
//	WBSF::ifStream in1;
//
//	in1.open("D:\\Travaux\\MergeImage\\input\\old\\2015\\info.txt");
//
//	for (size_t i = 0; i < 81; i++)
//	{
//		getline(in1, line);
//		
//		string old_file_path = "D:\\Travaux\\MergeImage\\input\\old\\2015\\" + line;
//		string new_file_path = "D:\\Travaux\\MergeImage\\input\\2015\\" + line;
//
//
//		string title = WBSF::GetFileTitle(new_file_path);
//		size_t pos = title.find_last_of("_");
//		title = title.substr(0, pos) + "_" + Landsat1::GetSceneName(i % 9);
//		WBSF::SetFileTitle(new_file_path, title);
//		
//		ASSERT(WBSF::FileExists(old_file_path));
//		WBSF::CopyOneFile(old_file_path, new_file_path, false);
//	}
//
//	in1.close();
//
//	in1.open("D:\\Travaux\\MergeImage\\input\\old\\2016\\info.txt");
//
//	for (size_t i = 0; i < 252; i++)
//	{
//		getline(in1, line);
//		string old_file_path = "D:\\Travaux\\MergeImage\\input\\old\\2016\\" + line;
//		string new_file_path = "D:\\Travaux\\MergeImage\\input\\2016\\" + line;
//
//
//		string title = WBSF::GetFileTitle(new_file_path);
//		size_t pos = title.find_last_of("_");
//		title = title.substr(0, pos) + "_" + Landsat1::GetSceneName(i%9);
//		WBSF::SetFileTitle(new_file_path, title);
//
//		ASSERT(WBSF::FileExists(old_file_path));
//		WBSF::CopyOneFile(old_file_path, new_file_path, false);
//		
//	}
//
//	in1.close();
//	in1.open("D:\\Travaux\\MergeImage\\input\\old\\2017\\info.txt");
//
//	for (size_t i = 0; i < 135; i++)
//	{
//		getline(in1, line);
//		string old_file_path = "D:\\Travaux\\MergeImage\\input\\old\\2017\\" + line;
//		string new_file_path = "D:\\Travaux\\MergeImage\\input\\2017\\" + line;
//
//
//		string title = WBSF::GetFileTitle(new_file_path);
//		size_t pos = title.find_last_of("_");
//		title = title.substr(0, pos) + "_" + Landsat1::GetSceneName(i % 9);
//		WBSF::SetFileTitle(new_file_path, title);
//
//		ASSERT(WBSF::FileExists(old_file_path));
//		WBSF::CopyOneFile(old_file_path, new_file_path, false);
//	}
//
//	in1.close();
//	
//	
//	{
//		WBSF::ofStream file;
//		file.open("D:\\Travaux\\MergeImage\\input\\list.txt");
//
//
//
//		StringVector list[9];
//		for (size_t i = 0; i < 9; i++)
//			for (size_t y = 0; y < 3; y++)
//			{
//				StringVector list2 = WBSF::GetFilesList(string("D:\\Travaux\\MergeImage\\input\\" + to_string(2015 + y) + "\\*_") + Landsat1::GetSceneName(i) + ".tif");
//				list[i].insert(list[i].end(), list2.begin(), list2.end());
//			}
//				
//
//		for (size_t j = 0; j < list[0].size(); j++)
//		{
//			for (size_t i = 0; i < 9; i++)
//			{
//				string file_path = WBSF::GetRelativePath("D:\\Travaux\\MergeImage\\input\\", list[i][j]);
//				file << file_path << endl;
//			}
//		}
//
//		file.close();
//	}
//
//
//	return 0;
//}

//***********************************************************************
//									 
//	Main                                                             
//						 	 		 
//***********************************************************************
int _tmain(int argc, _TCHAR* argv[])
{
	//return test();

	CTimer timer(true);

	//Create a mergeImages object
	CMergeImages mergeImage;
	ERMsg msg = mergeImage.m_options.ParseOption(argc, argv);

	if (!msg || !mergeImage.m_options.m_bQuiet)
		cout << mergeImage.GetDescription() << endl;


	if (msg)
		msg = mergeImage.Execute();

	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!mergeImage.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
