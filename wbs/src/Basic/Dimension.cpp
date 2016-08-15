#include "stdafx.h"
#include "Basic/Dimension.h"

#include "WeatherBasedSimulationString.h"

namespace WBSF
{

	const char* CDimension::DIMENSION_NAME[NB_DIMENSION] = { "Location", "Parameter", "Replication", "Temporal", "Variable" };


	StringVector CDimension::DIMENSION_TITLE;
	const char * CDimension::GetDimensionTitle(size_t d)
	{
		ASSERT(d >= 0 && d < NB_DIMENSION);

		if (DIMENSION_TITLE.empty())
			DIMENSION_TITLE = Tokenize(GetString(IDS_SIM_RESULT_HEAD), ";|");



		return DIMENSION_TITLE.empty() ? GetDimensionName(d) : DIMENSION_TITLE[d].c_str();
	}



}