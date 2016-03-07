//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <boost/algorithm/string.hpp>

#include "Basic/CSV.h"

using namespace std;

namespace WBSF
{


	const std::string CSVRow::EMPTY_STRING;
	std::istream& operator>>(std::istream& str, CSVRow& data)
	{
		data.readNextRow(str);
		return str;
	}


}