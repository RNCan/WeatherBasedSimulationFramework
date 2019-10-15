#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CSoilTemperatureModel : public CBioSIMModelBase
	{
	public:


		CSoilTemperatureModel();
		virtual ~CSoilTemperatureModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		

		static CBioSIMModelBase* CreateObject(){ return new CSoilTemperatureModel; }
		static double GetSoilTemperature(double z, double Tair, double Tsoil, double LAI, double Litter);


		static CStatistic GetSoilTemperature(const CWeatherYear& weather, double z, double& Tsoil, double LAI, double& Litter);
		static CStatistic GetSoilTemperature(const CWeatherMonth& weather, double z, double& Tsoil, double LAI, double& Litter);
		static CStatistic GetSoilTemperature(const CWeatherDay& weather, double z, double& Tsoil, double LAI, double& Litter);


		double m_LAI;
		double m_LAImax;
		double m_z;
	};
}