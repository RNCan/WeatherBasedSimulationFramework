#pragma once

#include <string>
#include "basic/ERMsg.h"

#ifdef QC_LIBRARY_EXPORTS
#define QC_LIBRARY_API __declspec(dllexport)
#else
#define QC_LIBRARY_API __declspec(dllimport)
#endif


namespace WBSF
{
	class CCallback;
}

//**************************************************************
namespace Mosa
{
	QC_LIBRARY_API ERMsg ExecuteFTP(const std::string& workingDir, size_t n, const std::string& userName, const std::string& password, WBSF::CCallback& callback);
}

