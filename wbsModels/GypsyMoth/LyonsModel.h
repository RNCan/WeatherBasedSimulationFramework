#pragma once


#include "EggModel.h"

namespace WBSF
{

	class CLyonsModel : public CEggModel
	{
	public:

		CLyonsModel(const CGMEggParam& param);
		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p);

	private:


		static const double  a;
		static const double  b;
		static const double  c;
		static const double  p0;
		static const double  p1;
		static const double  p2;

	};

}