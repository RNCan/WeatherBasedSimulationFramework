#pragma once

#include "TaskBase.h"
#include "basic/weatherStation.h"
#include "basic/FileStamp.h"
#include "Geomatic/SfcGribsDatabase.h"

namespace WBSF
{
	

	//**************************************************************
	class CQualityControl
	{
	public:

		enum TOutput{ OT_HOURLY, OT_DAILY, NB_OUTPUT_TYPES };
		enum TATTRIBUTE { HRDPS_PATH, HRRR_PATH, OUTPUT_PATH, UPDATE_LAST_N_DAYS, NB_ATTRIBUTES };


		CQualityControl(void);
		virtual ~CQualityControl(void);


		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		ERMsg ExecuteHourly(CCallback& callback);
		ERMsg ExecuteDaily(CCallback& callback);

	};

}