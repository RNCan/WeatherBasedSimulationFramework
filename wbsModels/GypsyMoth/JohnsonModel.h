#pragma once


#include "EggModel.h"

namespace WBSF
{

	class CJohnsonModel : public CEggModel
	{
	public:

		CJohnsonModel(const CGMEggParam& param);

		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p);
	
		
	protected:

		static short GetCol(short num_under_5);
		static const double EMERGE_TABLE[28][11];
	};

}