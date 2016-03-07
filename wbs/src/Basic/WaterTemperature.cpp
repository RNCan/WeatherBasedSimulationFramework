//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-09-2014	Rémi Saint-Amant	Initial version
//****************************************************************************
#include "stdafx.h"
#include "WaterTemperature.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

//*******************************************************************************


namespace WBSF
{
	const double CWaterTemperature::α1 = 1.119;
	const double CWaterTemperature::α2 = 0.946;
	const double CWaterTemperature::θ1 = 0.052;
	const double CWaterTemperature::β1 = 0.137;
	const double CWaterTemperature::β2 = 0.119;
	const double CWaterTemperature::β3 = 0.128;
	const double CWaterTemperature::b1 = 1.295;
	const double CWaterTemperature::b2 = -0.394;
	const double CWaterTemperature::b3 = 0.608;

	CWaterTemperature::CWaterTemperature(const CWeatherYears& data) :
		A°(0), A¹(0), B¹(0)
	{
		if (!data.empty())
			Compute(data);
	}

	//after Use of Air-Water Relationships for Predicting Water Temperature 
	//by V.Kothandaraman and R.L.Evans
	double CWaterTemperature::GetTwI(const CTRef& TRef) const
	{
		double Twtr = 0;

		if (GetAnnualCycleTrend(TRef) >= 0)
		{
			double dr = (double)TRef.GetJDay() / TRef.GetNbDaysPerYear();
			assert(dr >= 0 && dr < 1);

			double Ra1 = Ra.IsInside(TRef - 0) ? Ra[TRef - 0][0] : 0;
			double Ra2 = Ra.IsInside(TRef - 1) ? Ra[TRef - 1][0] : 0;
			double Ra3 = Ra.IsInside(TRef - 2) ? Ra[TRef - 2][0] : 0;

			double angle = 2 * PI*dr - (θ + θ1);
			double cosA = cos(angle);
			Twtr = max(0.0, (α1*A° / 2) + (α2*sqrt(A¹*A¹ + B¹*B¹) * cosA) + β1*Ra1 + β2*Ra2 + β3*Ra3);
		}

		ASSERT(Twtr >= 0 && Twtr <= 30);
		return Twtr;
	}

	//after 
	double CWaterTemperature::GetTwII(const CTRef& TRef) const
	{
		double Twtr = 0;
		if (GetAnnualCycleTrend(TRef) >= 0)
		{
			Twtr = max(0.0, GetAnnualCycleTrend(TRef) + Rw[TRef][0]);
		}

		return Twtr;
	}


	void CWaterTemperature::ComputeRa(const CWeatherYears& data)
	{
		CTPeriod period(data.GetEntireTPeriod(CTM(CTM::DAILY)));
		Ra.Init(period, 1, 0);

		for (size_t y = 0; y != data.size(); y++)
		{
			for (size_t m = 0; m != data[y].size(); m++)
			{
				for (size_t d = 0; d != data[y][m].size(); d++)
				{
					const CDay& day = data[y][m][d];
					CTRef TRef = day.GetTRef(); TRef.Transform(CTM(CTM::DAILY));
					double dr = (double)TRef.GetJDay() / TRef.GetNbDaysPerYear();//d between [0, 1[
					const CStatistic& Tair = day[HOURLY_DATA::H_TAIR];
					ASSERT(!Tair.IsInit() || (Tair[MEAN] >= -60 && Tair[MEAN] <= 60));

					Ra[TRef][0] = Tair.IsInit() ? (Tair[MEAN] - GetAnnualCycleTrend(TRef)) : 0;
				}
			}
		}
	}

	void CWaterTemperature::ComputeRw(const CWeatherYears& data)
	{
		CTPeriod period(data.GetEntireTPeriod(CTM(CTM::DAILY)));
		Rw.Init(period, 1, 0);

		for (size_t y = 0; y != data.size(); y++)
		{
			for (size_t m = 0; m != data[y].size(); m++)
			{
				for (size_t d = 0; d != data[y][m].size(); d++)
				{
					const CDay& day = data[y][m][d];
					CTRef TRef = day.GetTRef(); TRef.Transform(CTM(CTM::DAILY));
					double dr = (double)TRef.GetJDay() / TRef.GetNbDaysPerYear();//d between [0, 1[

					double Rw1 = Rw.IsInside(TRef - 1) ? Rw[TRef - 0][0] : 0;
					double Rw2 = Rw.IsInside(TRef - 2) ? Rw[TRef - 1][0] : 0;
					double Ra0 = Ra[TRef][0];

					Rw[TRef][0] = b1*Rw1 + b2*Rw2 + b3*Ra0;
				}
			}
		}
	}

	void CWaterTemperature::Compute(const CWeatherYears& data)
	{
		ComputeParams(data);
		ComputeRa(data);
		ComputeRw(data);
	}

	void CWaterTemperature::ComputeParams(const CWeatherYears& data)
	{
		CStatistic A°Stat;
		CStatistic A¹Stat;
		CStatistic B¹Stat;

		for (CWeatherYears::const_iterator itA = data.begin(); itA != data.end(); itA++)
		{
			for (CWeatherYear::const_iterator itM = itA->second->begin(); itM != itA->second->end(); itM++)
			{
				for (CWeatherMonth::const_iterator itD = itM->begin(); itD != itM->end(); itD++)
				{
					for (CWeatherDay::const_iterator itH = itD->begin(); itH != itD->end(); itH++)
					{
						CTRef TRef = itH->GetTRef(); TRef.Transform(CTM(CTM::DAILY));
						double d = (double)TRef.GetJDay() / TRef.GetNbDaysPerYear();//d between [0, 1[

						double Tair = (*itH)[H_TAIR];
						if (!IsMissing(Tair))
						{
							A°Stat += 2 * Tair;
							A¹Stat += 2 * Tair*cos(2 * PI*d);
							B¹Stat += 2 * Tair*sin(2 * PI*d);
						}
					}
				}
			}
		}


		A° = A°Stat[MEAN];
		A¹ = A¹Stat[MEAN];
		B¹ = B¹Stat[MEAN];
		θ = atan2(B¹, A¹);
	}

	ERMsg CWaterTemperature::Export(const std::string& filePath)
	{
		ERMsg msg;
		ofStream file;


		msg = file.open(filePath);
		if (msg)
		{
			file << "Date,Ta,Ra,Rw,TwI,TwII" << endl;
			for (CTRef d = Ra.GetFirstTRef(); d <= Ra.GetLastTRef(); d++)
			{
				file << d.GetFormatedString() << "," << GetAnnualCycleTrend(d) << "," << ToString(Ra[d][0], 1) << "," << ToString(Rw[d][0], 1) << "," << ToString(GetTwI(d), 1) << "," << ToString(GetTwII(d), 1) << endl;
			}

			file.close();
		}

		return msg;
	}

}