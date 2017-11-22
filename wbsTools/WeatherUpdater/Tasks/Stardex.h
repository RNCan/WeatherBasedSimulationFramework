#pragma once

#include "ErMsg.h"
#include "DailyStation.h"
#include "FloatMatrix.h"

class CStardex
{
public:

	ERMsg ExportFile(const CDailyStation& dailyStation);
	void Execute();
	ERMsg ImportResult(float result1[4][32][55], float result2[32][58]);
	void CleanFiles();

	void SetFilePath(const CString& filePath)
	{	m_filePath = filePath;
	}

	CString GetOutputFilePath()
	{	return m_filePath+".ind.csv";
	}

private:

	CString m_filePath;
};
