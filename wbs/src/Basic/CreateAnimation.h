
#pragma once
//#include <windows.h>
//#include <commctrl.h>
//#include <stdio.h>
#include "ERMsg.h"
#include <string>
#include <vector>




namespace WBSF
{
	ERMsg MakeGIF(std::string output_file_path, const std::vector<std::string>& file_list, unsigned short delay=20, bool bLoop=true);
}





