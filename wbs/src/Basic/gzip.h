
#pragma once

#include <string>
#include "ERMsg.h"

namespace WBSF
{
	class Gzip
	{
	public:
		
		static ERMsg compress(const std::string& data, std::string& compress);
		static ERMsg decompress(const std::string& compress, std::string& data);
	};
}