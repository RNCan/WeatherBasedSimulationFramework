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

		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		//virtual ERMsg OnExecuteAnnual()override;
		//virtual ERMsg OnExecuteMonthly()override;
		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteHourly()override;

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;

		static CBioSIMModelBase* CreateObject(){ return new CSoilTemperatureModel; }
		
		static double GetDeltaSoilTemperature(double z, double LAI, double Litter, double F);


		//static CStatistic GetSoilTemperature(const CWeatherYear& weather, double z, double& Tsoil, const std::valarray<double>& A, double LAI, double& Litter, double F, double Fo);
		//static CStatistic GetSoilTemperature(const CWeatherMonth& weather, double z, double& Tsoil, const std::valarray<double>& A, double LAI, double& Litter, double F, double Fo);
		CStatistic GetSoilTemperature(const CWeatherDay& weather, double z, double& Tsoil, double LAI, double& Litter);
		static double GetTairAtSurface(const CWeatherDay& weather, size_t h, double Fo);

		double m_LAI;
		double m_Litter;
		double m_z;
		double m_F;
		double m_Fo;
		double m_Cs;
		double m_Kt;
		double m_Cice;
		double m_Fs;
	};
}