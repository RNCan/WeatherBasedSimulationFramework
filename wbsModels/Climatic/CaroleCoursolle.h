#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CHamonPET;
	class CCCModel : public CBioSIMModelBase
	{
	public:


		CCCModel();
		virtual ~CCCModel();

		//virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		static CBioSIMModelBase* CreateObject(){ return new CCCModel; }

	private:

		double GetTsea(const CWeatherMonth& weather);
		double GetTsea(const CWeatherYear& weather);
		double GetPsea(const CWeatherMonth& weather);
		double GetPsea(const CWeatherYear& weather);
		void GetPwv(std::vector<double>& Pwv, std::vector<size_t>& Pwm);
		void GetPdv(std::vector<double>& Pdv, std::vector<size_t>& Pdm);

		double GetWBALsea(const CWeatherYear& weather, const CModelStatVector& HamonPET);
		double GetMonthlyDifference(const CWeatherYear& weather);
		//double GetPETsea(const CModelStatVector& HamonPET);
		//double GetPETsum(const CModelStatVector& HamonPET);
		double GetCoolingDD(const CWeatherYear& weather, double threshold);
		double GetCoolingDD(const CWeatherMonth& weather, double threshold);


		int GetFrostDay(int year, const double& th);
		int GetDaysBelow(int year, const double& th);
		
	};
}