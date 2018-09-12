//**********************************************************************
// 24/04/2018	1.5.1	Rémi Saint-Amant    Compile with VS 2017
// 20/09/2016	1.5.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	1.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 14/02/2013			Rémi Saint-Amant	Creation
//**********************************************************************


#include "ModelBase/EntryPoint.h"
#include "Basic/Evapotranspiration.h"
#include "WetnessDuration.h"



//LAI = varied between 1 and 4 resulting
//Crop				Max.root depth(m)					Max.LAI(m²/m²)
//Barley (Spring)		1.2-1.6								4 - 6
//Beans (Dry)			0.9-1.3								3 - 4
//Lentils				0.9-1.3								3 - 4
//Maize					1.5-2.0								4 - 7
//Oats					1.2-1.6								4 - 6
//Peas (Dry)			0.9-1.3								3 - 4
//Sorghum				1.4-1.8								6 - 10
//Soybean				1.4-1.8								4 - 7
//Sunflower				1.7-2.2								4 - 5
//Wheat (spring)		1.2-1.6								4 - 6
//Wheat (winter)		1.5-2.0								5 - 8
//Grass (cropped)		0.8									4.0

//Kc ={1 + (Kc' - 1) LAI / 3 if Kc' > 1 and LAI < 3
//Kc' otherwise
//Kc' is the ET crop coefficient input parameter.
//If Kc' < 1 then Kc = Kc'.
//3.0 Is maximum LAI (LAImax) of the reference crop.


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	extern const char WETNESS_DURATION_HEADER[] = "WetnessDuration";

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWetnessDurationModel::CreateObject);

	//Contructor
	CWetnessDurationModel::CWetnessDurationModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.5.1 (2018)";

		m_model = DAILY_SINUS;
		//	m_DPType = DP_MN;
		m_RHThreshold = 80;
		m_canopy = CDewDuration::SOYBEAN;
		m_exposure = CDewDuration::EXPOSED_LEAF;
		m_LAI = 1;
		m_Zc = 0.1;

		//tmp var
		m_x0 = 0;
		m_x1 = 0;
		m_x2 = 0;
		m_x3 = 0;
		m_x4 = 0;

		m_previousRH = -1;
		m_bWetPrevious = true;
	}


	CWetnessDurationModel::~CWetnessDurationModel()
	{}

	//void CWetnessDurationModel::GetHourlyDat(CDay& hourlyDay, CTRef ref)
	//{
	//	//const CWeatherDay& wDay = m_weather[ref];
	//	//CSun sun(m_info.weather.GetLocation().m_lat, m_info.m_loc.GetLon());
	//	//wDay.GetHourlyVar(hourlyDay, sun);
	//}

	double CWetnessDurationModel::GetWetnessDuration(CWeatherStation& weather, CTRef ref)
	{
		double WD = 0;

		CWeatherDay& wDay = weather.GetDay(ref);
		//ASSERT(ref.GetType()==CTRef::DAILY);
		//const CDay& hDay = (const CDay& )weather.Get(ref);


		switch (m_model)
		{
		case DAILY_SINUS:		WD = GetWetnessDurationSinus(wDay); break;
		case EXTENDED_TRESHOLD: WD = GetWetnessDurationExtT(wDay); break;
		case FIXED_THRESHOLD:	WD = GetWetnessDurationFixT(wDay); break;
		case DPD:				WD = GetWetnessDurationDPD(wDay); break;
		case CART:				WD = GetWetnessDurationCART(wDay); break;
		case PM:				WD = GetWetnessDurationPM(weather, ref); break;
		case SWEB:				WD = GetWetnessDurationSWEB(weather, ref); break;
		default: ASSERT(false);
		}

		return WD;
	}


	double CWetnessDurationModel::GetWetnessDurationSinus(CWeatherDay& wDay)const
	{


		//NbVal=   720	Bias=-0.40044	MAE= 3.85938	RMSE= 4.96637	CD= 0.37430	R²= 0.39480
		//x0                  	=  -0.80650  
		//x1                  	=   0.54394  
		//x2                  	=   1.00000  
		//x3                  	=   0.49111  
		//x4                  	=   1.08240  
		double K[5] = { -0.8, 0.55, 1, 0.5, 1 };
		//double K[5] = {m_x0,m_x1,m_x2,m_x3,m_x4};
		//static const double K[4] = {-1.1, 1.1, -2.8, 0.2};
		//static const double K[5] = {0, 0.95, -1.3, 0.22, 0};



		static const double a = 17.27;
		static const double b = 237.7;

		double Tmin = wDay[H_TMIN2][MEAN];
		double Tmax = wDay[H_TMAX2][MEAN];
		double Trng = Tmax - Tmin;

		double RH = max(1.0, min(100.0, wDay[H_RELH][MEAN] + K[0] * Trng));
		double c = (a*Tmax) / (b + Tmax) + log(RH / 100);
		double Tdew = (b*c) / (a - c);

		double DDP = 0;
		if (Tdew <= Tmin)
			DDP = 0;
		else if (Tdew >= (Tmax - K[4]))//-0.9
			DDP = 24;
		//else  DDP = (asin(2*(Tdew-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12;
		else DDP = K[1] * (asin(2 * (Tdew - Tmin) / (Tmax - Tmin) - 1) * 2 / PI + 1) * 12 + K[2] + K[3] * wDay[H_PRCP][SUM];

		//double WD = max(0.0, Min(24, DDP*K[1]+K[2] + K[3]*wDay.GetPpt()));
		double WD = max(0.0, min(24.0, DDP));
		//double WD = max(0.0, Min(24, (DDP1+DDP2)/2));
		//double WD = max(0.0, Min(24, (DDP1+DDP2)/2));

		/*


		double Tmin = wDay[TMIN];
		double Tmax = wDay[TMAX];

		double RH1 = Min(100,Max(1,wDay[RELH]+K1[0]*wDay.GetTRange()));
		double c1 = (a*Tmax)/(b+Tmax)+log(RH1/100);
		double Tdew1 = (b*c1)/(a-c1);

		double DDP1=0;
		if(Tdew1<Tmin)
			DDP1 = 0;
		else if(Tdew1>(Tmax-K1[4]))//-0.9
			DDP1 = 24;
		//else  DDP = (asin(2*(Tdew-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12;
		else DDP1 = max(0.0, Min(24, K1[1]*(asin(2*(Tdew1-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12+K1[2] + K1[3]*wDay.GetPpt()));

		double RH2 = Min(100,Max(1,wDay[RELH]+K2[0]*wDay.GetTRange()));
		double c2 = (a*Tmax)/(b+Tmax)+log(RH2/100);
		double Tdew2 = (b*c2)/(a-c2);

		double DDP2=0;
		if(Tdew2<Tmin)
			DDP2 = 0;
		else if(Tdew2>(Tmax-K2[4]))//-0.9
			DDP2 = 24;
		else DDP2 = (asin(2*(Tdew2-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12;

		DDP2 = max(0.0, Min(24,DDP2*K2[1]+K2[2] + K2[3]*wDay.GetPpt()));
		//K2[1]*(asin(2*(Tdew2-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12+K2[2] + K2[3]*wDay.GetPpt();

		//double WD = max(0.0, Min(24, (DDP1+DDP2)/2));
		double WD = DDP2;//(DDP1+DDP2)/2;
		//
	*/

		return WD;
	}

	/*
	double CWetnessDurationModel::GetWetnessDurationSinus(const CWeatherDay& wDay)const
	{
		double WD=0;
		double Tmin = wDay[TMIN];
		double Tmax = wDay[TMAX];

		double RH = wDay[RELH];
		static const double a = 17.27;
		static const double b = 237.7;
		//NbVal=212119	Bias= 0.00002	MAE= 2.96144	RMSE= 4.05465	CD= 0.59856	R²= 0.59856
		//NbVal=213409	Bias=-0.14737	MAE= 2.02378	RMSE= 3.28933	CD= 0.51327	R²= 0.51494
		//NbVal=212119	Bias=-0.05943	MAE= 0.98406	RMSE= 1.73374	CD= 0.68022	R²= 0.68080
		//NbVal=212119	Bias= 0.00002	MAE= 2.96144	RMSE= 4.05465	CD= 0.59856	R²= 0.59856
		static const double X0_SIN[4] = {-10.13105,-11.47112,-13.75512,-9.00000};
		static const double X1_SIN[4] = { 0.80058,   1.37961,  1.20949, 1.62299};
		static const double K_SIN[4] =  { 0.82206,  -0.63089, -0.01642,-0.71516};

		//double X0_SIN[4] = {m_x0,m_x0,m_x0,m_x0};
		//double X1_SIN[4] = {m_x1,m_x1,m_x1,m_x1};
		//double K_SIN[4] = {m_k,m_k,m_k,m_k};

		//here we use Tmax instead of Tmean because it give much better result...
		double T = wDay.GetTMax();
		//RH = RH*X1_SIN[m_DPType]+X0_SIN[m_DPType]+K_SIN[m_DPType]*wDay.GetTRange();
		RH = RH+K_SIN[DP_HI]*wDay.GetTRange();
		//RH = RH*K_SIN[m_DPType];
		double c = (a*T)/(b+T)+log(Min(100,Max(1,RH))/100);
		double Tdew = (b*c)/(a-c);



		double DDP=0;
		if(Tdew<Tmin)
			DDP = 0;
		else if(Tdew>Tmax)
			DDP = 24;
		else DDP = (asin(2*(Tdew-Tmin)/(Tmax-Tmin)-1)*2/PI+1)*12;


		WD = max(0.0, Min(24, DDP*X1_SIN[DP_HI]+X0_SIN[DP_HI]));
		return WD;
	}
	*/

	//2.  For the extended threshold leaf wetness model the RH is classified as follows 
	double CWetnessDurationModel::GetWetnessDurationExtT(CDay& hourlyDay)const
	{
		//if( m_x0>m_x1)
			//return -999;



		CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
		//if( hourlyDay[h][H_PRCP]>=2.8 )//2.8
			//{
			//	bWet=true;
			//}
			//else
			//{
				//x0                  	=  27.67887  
				//x1                  	=  96.60543  
				//x2                  	=  -1.52337  
				//x3                  	=  21.48100 
				//If RH<70% then hour is DRY (0)
				//if( RH<m_x0)//70
				//{
				//	bWet=false;
				//}
				//else  If RH>87% then hour is wet (1)
				//else 
				//if( RH>m_x1)//87
				//{
				//	bWet=true;
				//}
				//else //if( RH>=70 && RH<=87 )
				//if( RH>=m_x0 && RH<=m_x1)


		double WD = 0;
		for (int h = 0; h < 24; h++)
		{
			double RH = hourlyDay[h][H_RELH];
			bool bWet = false;

			if (m_previousRH == -1)
				me.m_previousRH = RH;


			double deltaRH = RH - m_previousRH;
			if (deltaRH < -1.5)
			{
				bWet = false;
			}
			else if (RH >= 87)
			{
				bWet = true;
			}
			else
			{
				bWet = m_bWetPrevious;
			}

			me.m_previousRH = RH;
			me.m_bWetPrevious = bWet;
			WD += bWet ? 1 : 0;
		}

		double prcp = hourlyDay[H_PRCP][SUM];
		return max(0.0, min(24.0, WD + 0.16*prcp));

		//		NbVal=   734	Bias= 0.99541	MAE= 3.50158	RMSE= 4.67131	CD= 0.30291	R²= 0.42217
		//x0                  	=  -1.53774  
		//x1                  	=  93.99692  
		//x4                  	=   0.08814  
//NbVal=   720	Bias= 0.07164	MAE= 3.45338	RMSE= 4.45647	CD= 0.49619	R²= 0.51406
//x0                  	=  -1.73950  
//x1                  	=  86.21532  
//x4                  	=   0.16381  
//NbVal=   355	Bias=-0.25221	MAE= 2.93518	RMSE= 3.74560	CD= 0.10009	R²= 0.20804
//x0                  	=  -3.15589  
//x1                  	=  84.29987  
//x4                  	=   0.03816  

			//else 
			//{
			//	if( RH<m_x0)//70
			//	{
			//		bWet=false;
			//	}
			//	//else  If RH>87% then hour is wet (1)
			//	//else 
			//	if( RH>m_x1)//87
			//	{
			//		bWet=true;
			//	}
			//	bWet=m_bWetPrevious;
			//}

	//If RH is between 70 -87% then  Hour is WET (1) if the RH is 3% higher than the previous hour OR it is DRY (0) if it is 2% lower than the previous hour 
	//ELSE it is considered the same as the previous hour. 



//Tdew = TdewStat[MEAN];

//return WD;


	}

	//3.       For the fixed threshold  model (FixT)
	double CWetnessDurationModel::GetWetnessDurationFixT(CDay& hourlyDay)const
	{
		double WD = 0;

		for (int h = 0; h < 24; h++)
		{
			if (hourlyDay[h][H_RELH] > m_RHThreshold)
				WD++;
		}

		double prcp = hourlyDay[H_PRCP][SUM];
		return max(0.0, min(24.0, WD + 0.14*prcp));//0.13

	}

	//4.       difference between T and the dew point temperature (DPD)
	double CWetnessDurationModel::GetWetnessDurationDPD(CDay& hourlyDay)const
	{


		//	N=     65401	T=  0.00020	F=14823.92487
		//NbVal=   432	Bias= 0.34594	MAE= 4.00190	RMSE= 5.50883	CD= 0.03973	R²= 0.25505
		//x0                  	=   0.17023  
		//x1                  	=   0.24141  
		//x2                  	=   2.03876  
		//x4                  	=   0.20339  
		//
		//NbVal=   720	Bias= 0.49882	MAE= 4.24834	RMSE= 5.46293	CD= 0.24292	R²= 0.31821
		//x0                  	=   2.55440  
		//x1                  	=   0.75859  
		//x2                  	=  18.28395  
		//x3                  	=   0.00965  
		//x4                  	=   0.18136  

			//if( h1==-1 && (DPD<0.17023 && sRad<0.24141) ) 
		const CDay& nextHourlyDay = hourlyDay.GetNext();



		double WD = 0;
		double prcp = 0;

		int h1 = -1;
		int h2 = -1;
		for (int hh = 0; hh < 24; hh++)
		{
			int h = (hh + 12) % 24;
			const CDay& hDay = hh < 12 ? hourlyDay : nextHourlyDay;
			double DPD = max(0.0f, hDay[h][H_TAIR2] - hDay[h][H_TDEW]);
			double RH = hDay[h][H_RELH];
			double sRad = hDay[h][H_SRAD];

			if (h1 == -1 && (DPD < 2.5 && sRad < 0.75))
				h1 = hh;

			if (h1 != -1 && h2 == -1 && hh >= 18 && DPD > 0.01)
				h2 = hh;

			prcp += hDay[h][H_PRCP];
		}

		if (h1 != -1 && h2 == -1)
			h1 = -1;

		WD = h2 - h1;


		return max(0.0, min(24.0, WD + 0.18*prcp));

	}

	static double CARTEq1(double T, double RH, double U, double DPD)
	{
		double Eq1 = 0;
		if (T >= 0)
			Eq1 = 1.6064*sqrt(T) + 0.0036*Square(T) + 0.1531*RH - 0.4599*U*DPD - 0.0035*T*RH;
		return Eq1;
	}

	static double CARTEq2(double T, double RH, double U, double DPD)
	{

		double Eq2 = 0;
		if (T >= 0)
			Eq2 = 0.7921*sqrt(T) + 0.0046*RH - 2.3889*U - 0.039*T*U + 1.0613*U*DPD;
		return Eq2;
	}

	//5.		non-parametric procedure of classification tree
	double CWetnessDurationModel::GetWetnessDurationCART(CDay& hourlyDay)const
	{
		double WD = 0;
		double prcp = 0;
		const CDay& nextHourlyDay = hourlyDay.GetNext();

		for (int hh = 0; hh < 24; hh++)
		{
			int h = (hh + 12) % 24;
			const CDay& hDay = hh < 12 ? hourlyDay : nextHourlyDay;

			double T = hDay[h][H_TAIR2];
			double RH = hDay[h][H_RELH];
			double DPD = max(0.0f, hDay[h][H_TAIR2] - hDay[h][H_TDEW]);
			double U = hDay[h][H_WND2] * 1000 / 3600;// (m/s)
			//double ws = hDay[h][H_WND2]*1000/3600;// (m/s)
			//double U = CASCE_ETsz::GetWindProfileRelationship(ws,10);//convert wind speed at 10 meters to 2 meters
			prcp += hDay[h][H_PRCP];

			if (DPD < 3.7)//3.7
			{

				if (U < 2.5)//2.5
				{

					if (CARTEq1(T, RH, U, DPD) > 14.45)
						WD++;
				}
				else
				{
					if (RH > 87.8)//87.8
					{
						if (CARTEq2(T, RH, U, DPD) > -9)//37
							WD++;
					}
				}
			}
		}

		return max(0.0, min(24.0, WD + 0.1*prcp));


		//N=     25117	T=  0.32946	F=22713.29868
		//NbVal=   720	Bias= 0.08309	MAE= 4.31113	RMSE= 5.61660	CD= 0.19973	R²= 0.30455
		//x0                  	=  15.00099  
		//x1                  	=  -9.08771  
		//x4                  	=   0.03112  

		//double prcp = hourlyDay.GetDailyStat(H_PRCP)[SUM];

		//return WD;
	}

	//6.		Sentelhas et al. (2006): a faire
	//double CWetnessDurationModel::GetWetnessDurationPM(const CYearsVector& weather, CTRef TRef)const

	//6.		M.J. Pedro Jr 1982
	double CWetnessDurationModel::GetWetnessDurationPM(CWeatherStation& weather, CTRef TRef)const
	{
		if (m_PMStat.empty())
		{
			CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
			//Init PM
			CDewDuration PM;
			PM.m_loc = m_info.m_loc;
			PM.m_canopy = m_canopy;
			PM.m_exposure = m_exposure;
			PM.m_x0 = m_x0;
			PM.m_x1 = m_x1;
			PM.m_x2 = m_x2;
			PM.m_x3 = m_x3;
			PM.m_x4 = m_x4;

			CWetnessDurationStat hStat;
			PM.Execute(weather, hStat);
			Hourly2Daily(hStat, me.m_PMStat);
		}

		double WD = m_PMStat[TRef][WETNESS_DURATION];

		return WD;


	}


	//7.		Surface Wetness Energy Balance (SWEB)
	double CWetnessDurationModel::GetWetnessDurationSWEB(CWeatherStation& weather, CTRef TRef)const
	{
		if (m_SWEBStat.empty())
		{
			CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
			//Init PM
			CSWEB SWEB;
			//SWEB.m_loc=m_info.m_loc;

			SWEB.m_LAI = m_LAI;
			SWEB.m_Zc = m_Zc;
			SWEB.m_x0 = m_x0;
			SWEB.m_x1 = m_x1;
			SWEB.m_x2 = m_x2;
			SWEB.m_x3 = m_x3;
			SWEB.m_x4 = m_x4;

			CWetnessDurationStat hStat;
			SWEB.Execute(weather, hStat);
			Hourly2Daily(hStat, me.m_SWEBStat);

		}

		//compute daily value
		//CTRef TRef2(TRef);
		//TRef2.Transform(CTRef::HOURLY);

		//double WD = 0;
		//for(int h=0; h<24; h++, TRef2++)
			//WD += m_SWEBStat[TRef2][WETNESS_DURATION]>0.1?1:0;//we cosidere dew if 10% of the leaf is wet

		double WD = m_SWEBStat[TRef][WETNESS_DURATION];

		return WD;
	}


	/*
	//system
	double Ca = 340; //Ambient CO2 concentration ( µmol/mol )

	//Canopy
	double ƒg = (1+0.1)/2;//fraction of gree vegetation
	double m= 4.0;//Ball & Berry slope
	double b = 0.04e6; //Ball & Berry offset
	double β = 0.029; // LUE value of Agriculture - C3 type from table 1 in mol CO2/ mol APAR
	double βn = 0.03; //nominal LUE
	double ɤn = 0.6; //nominal Ci/Ca
	double ɤo = 0.0; //Ci/Ca at β=0

	//soil
	double dr = 2;//rooting depth (m)
	double ȶ = 2.4; //Root density coeficient
	double bs = 6.3; //Moisture release paramters
	double ϴfc = 2; //water content at field capacity
	double ϴpwp = 1; //water content at permanet wilting point


	//z in meter
	double GetFroot(double z)
	{
		double Δz = 1;//?
		double Froot = (exp(-z*ȶ)-exp(-ȶ*(z+Δz)))/(1-exp(-ȶ*dr));
		return Froot;
	}

	//ϴ: actual volumetric soil water
	double GetAw(double ϴ)
	{
		double Aw = (ϴ - ϴpwp)/(ϴfc-ϴpwp);
		return Aw;
	}

	//ϴ: actual volumetric soil water
	double Getfaw(double ϴ)
	{
		//reduction in stranpiration due to stomatal closure
		double Aw = GetAw(ϴ);
		double faw = 1-2/3*pow(Aw*(pow(0.03,-1/bs)-pow(1.5,-1/bs))+pow(1.5,-1/bs), -bs);
		return faw;
	}

	double GetRb()
	{
		enum {AMPHISTOMATOUS=1, HYPOSTOMATOUS=2};
		double ƒs = AMPHISTOMATOUS;

		double Rb = ƒs/(ƒg*ƒdry)*Rx;
	}

	double Get∞()
	{
		double ∞ = Ca*(ɤn-ɤo)/(βn*APAR) + ɤo*(1.3*Rb+Ra);
		return ∞;
	}
	double GetC1()
	{
		double ∞ = Get∞();
		double C1 = (∞*Bc-1.6*(1-Bc*Rb) + (1-ɤo)*m*eas/ei)/(1.6*Bc);
		return C1;
	}
	double GetC2()
	{
		double ∞ = Get∞();
		double C2 = (-∞*(1-Bc*Rb)-1.6*Rb+(1-ɤo)*m*Rb)/(1.6*Bc);
		return C2;
	}
	double GetC3()
	{
		double ∞ = Get∞();
		double C3 = (-∞*Rb)/(1.6*Bc);
		return C3;
	}

	///?????????
	//Press et al. 1992
	//Numnerical recipe in C.
	double GetRc()
	{
		double Rc=0;
		double a = GetC1();
		double b = GetC2();
		double c = GetC3();
		double Q = (Square(a)-3*b)/9;
		double R = (2*Cube(a)-9*a*b+27*c)/54;
		if( Square(R) < Cube(Q) )
		{
			double ϴ = acos(R/sqrt(Cube(Q)) );
			double x1 = -2*sqrt(Q)*cos(ϴ/3)-a/3;
			double x2 = -2*sqrt(Q)*cos((ϴ+2*PI)/3)-a/3;
			double x3 = -2*sqrt(Q)*cos((ϴ-2*PI)/3)-a/3;
			if(x1>0)
				Rc=x1;
			if(x2>0)
				Rc=x2;
			if(x3>0)
				Rc=x3;
		}
		//return Rc = 1.3*Rb + Ra;
		return Rc;
	}


	//P: atmosphereic pressure kPa
	//ei: kPa
	//Rc: resistance of canopie m²s/µmol
	double GetLEct(double P, double ei, double RHb)
	{
		static const double λ = 2260/55508.4350618; //latent heat of vaporization J/µmol
		double Rc = GetRc();
		double LEct = λ*ei*(1-RHb)/(P*(Rc));//equation (3)
	}

	double U(double z)
	{
		double LEct = GetLEct();
		double faw = Getfaw();
		double LEctp = faw * LEct;
		double U = (LEctp * Froot(z))/();
	}

	//http://en.wikipedia.org/wiki/Psychrometric_constant
	//The psychrometric constant  relates the partial pressure of water in air to the air temperature. This lets one interpolate actual vapor pressure from paired dry and wet thermometer bulb temperature readings.[1]


	// psychrometric constant [kPa °C-1],
	//P = atmospheric pressure [kPa],
	// latent heat of water vaporization, 2.45 [MJ kg-1],
	// specific heat of air at constant pressure, [MJ kg-1 °C-1],
	// ratio molecular weight of water vapor/dry air = 0.622.
	//Both  and  are constants.
	//Since atmospheric pressure, P, depends upon altitude, so does .
	//At higher altitude water evaporates and boils at lower temperature.
	//Although  is constant, varied air composition results in varied .
	//Thus on average, at a given location or altitude, the psychrometric constant is approximately constant. Still, it is worth remembering that weather impacts both atmospheric pressure and composition.
	//[edit]vapor pressure estimation
	//
	//Saturated vapor pressure,
	//Actual vapor pressure,
	//here e[T] is vapor pressure as a function of temperature, T.
	//Tdew = the dewpoint temperature at which water condenses.
	//Twet = the temperature of a wet thermometer bulb from which water can evaporate to air.
	//Tdry = the temperature of a dry thermometer bulb in air.

	double GetLEce()
	{
		double LEce =;
		return LEce;
	}

	double GetLEc()
	{
		double LEc = GetLEce() + GetLEct();
		return LEc;

	}

	//http://www.public.iastate.edu/~bkh/teaching/505/rowlandson_alex.pdf
	double Getdew(double LEc)
	{
		//3600 number of second in hour
		static const double M = 0.018;//kg
		static const double λ = 44000;// joule
		double dew = -1*LEc*M*3600/λ;
		return dew;
	}
	*/


	//ERMsg CWetnessDurationModel::OnExecuteAnnual()
	//{
	//	ERMsg msg;
	//
	//	CWetnessDurationStat stat(m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)));
	//
	//
	//	//CWeatherStation weather(m_weather);
	//	//weather.AjusteMeanForHourlyRequest();
	//	//
	//	//CYearsVector hourlyData;
	//	//weather.GetHourlyVar(hourlyData, m_info.m_loc);
	//
	//	CWetnessDurationStat dailyStat;
	//	OnExecuteDaily(dailyStat);
	//
	//
	//
	//	for(size_t y=0; y<m_weather.size(); y++)
	//    {		
	//		int year = m_weather.GetFirstYear() + int(y);
	//		
	//		CStatistic statWD;
	//		for (size_t m = 0; m<m_weather[y].size(); m++)
	//		{
	//			for (size_t d = 0; d<m_weather[y][m].size(); d++)
	//			{
	//				statWD+=dailyStat[CTRef(year,m,d)][WETNESS_DURATION];
	//				HxGridTestConnection();
	//			}//all day
	//		}//all month
	//
	//		stat[y][WETNESS_DURATION] = statWD[MEAN];
	//	}//all years
	//
	//	SetOutput(stat);
	//
	//
	//	return msg;
	//}
	//
	//
	//ERMsg CWetnessDurationModel::OnExecuteMonthly()
	//{
	//	ERMsg msg;
	//
	//	CWetnessDurationStat stat(m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY)));
	//
	//	//CWeatherStation weather(m_weather);
	//	//weather.AjusteMeanForHourlyRequest();
	//	
	//	//CYearsVector hourlyData;
	//	//weather.GetHourlyVar(hourlyData, m_info.m_loc);
	//	CWetnessDurationStat dailyStat;
	//	OnExecuteDaily(dailyStat);
	//
	//
	//	for (size_t y = 0; y<m_weather.size(); y++)
	//	{
	//		int year = m_weather.GetFirstYear() + int(y);
	//		for (size_t m = 0; m<m_weather[y].size(); m++)
	//		{
	//			CStatistic statWD;
	//
	//			for (size_t d = 0; d<m_weather[y][m].size(); d++)
	//			{
	//				statWD+=dailyStat[m_weather[y][m][d].GetTRef()][WETNESS_DURATION];
	//				HxGridTestConnection();
	//			}//all day
	//
	//			stat[y*12+m][WETNESS_DURATION] = statWD[MEAN];
	//		}//all month
	//	}//all years
	//
	//	SetOutput(stat);
	//
	//
	//	return msg;
	//}
	//
	//
	//ERMsg CWetnessDurationModel::OnExecuteDaily()
	//{
	//	ERMsg msg;
	//
	//	CWetnessDurationStat dailyStat;
	//	OnExecuteDaily(dailyStat);
	//	SetOutput(dailyStat);
	//
	//
	//	return msg;
	//}
	//
	//
	 
	ERMsg CWetnessDurationModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
		{
			msg.ajoute("This model need hourly weather input");
			return msg;
		}


		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), 1);
		
		/*if( m_obs.empty() )
		{
			m_dailyData = m_weather;
			m_dailyData.AjusteMeanForHourlyRequest(m_info.m_loc);
			m_dailyData.GetHourlyVar(m_obs, m_info.m_loc);
		}*/
	
		for (size_t y = 0; y<m_weather.size(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					CTRef TRef(year, m, d);
					double WD = GetWetnessDuration(m_weather, TRef);
					m_output[TRef][WETNESS_DURATION] = WD;
					HxGridTestConnection();
				}
			}
		}

		return msg;

	}
	

	ERMsg CWetnessDurationModel::OnExecuteHourly()
	{
		ERMsg msg;

		CSWEB SWEB;

		SWEB.m_LAI = m_LAI;
		SWEB.m_Zc = m_Zc;
		/*SWEB.m_x0 = m_x0;
		SWEB.m_x1 = m_x1;
		SWEB.m_x2 = m_x2;
		SWEB.m_x3 = m_x3;
		SWEB.m_x4 = m_x4;*/

		SWEB.Execute(m_weather, m_output);

		return msg;
	}


	void CWetnessDurationModel::ExecuteHourly(CWetnessDurationStat& stat)
	{
		ASSERT(m_weather.IsHourly());
		stat.Init(m_weather.GetEntireTPeriod());

		/*if( m_obs.empty() )
		{
			CWeatherStation weather(m_weather);
			weather.AjusteMeanForHourlyRequest(m_info.m_loc);
			weather.GetHourlyVar(m_obs, m_info.m_loc);
		}*/

		CSWEB SWEB;
		//SWEB.m_loc=m_info.m_loc;

		SWEB.m_LAI = m_LAI;
		SWEB.m_Zc = m_Zc;
		SWEB.m_x0 = m_x0;
		SWEB.m_x1 = m_x1;
		SWEB.m_x2 = m_x2;
		SWEB.m_x3 = m_x3;
		SWEB.m_x4 = m_x4;

		SWEB.Execute(m_weather, stat);
	}



	//********************************************************************************************************************
	//simulated annaling 
	void CWetnessDurationModel::AddHourlyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == NB_INPUT_HOURLY + 6)
		{
			//Name	ID	Year	Month	Day	Hour	T	Pressure	Prcp	Tdew	RH	WindSpeed	SRad	NRad1	NRad2	Wetness	VPD

			std::vector<double> obs(NB_INPUT_HOURLY);

			CTRef TRef(ToInt(data[2]), ToSizeT(data[3]) - 1, ToSizeT(data[4]) - 1, ToSizeT(data[5]));
			for (int c = 0; c < NB_INPUT_HOURLY; c++)
				obs[c] = ToDouble(data[6 + c]);


			ASSERT(obs.size() == NB_INPUT_HOURLY);
			m_SAResult.push_back(CSAResult(TRef, obs));

		}


	}
	void CWetnessDurationModel::GetObsStatH(CObsStatVector& stat)const
	{

		//if (m_obsWeather.empty())
		//{
		//	CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
		//	me.m_obsWeather = m_weather;
		//	//Create a copy of weather ad adjust it
		//	//CWeatherStation weather(m_weather);
		//	//weather.AjusteMeanForHourlyRequest(m_info.m_loc);
		//	//weather.GetHourlyVar(me.m_obs, m_info.m_loc);

		//	for(size_t i=0; i<m_SAResult.size(); i++)
		//	{
		//		CTRef ref = m_SAResult[i].m_ref;
		//		if (m_weather.GetEntireTPeriod().IsInside(m_SAResult[i].m_ref))
		//		{
		//			if( m_SAResult[i].m_obs[H_T]>-999)
		//				me.m_obsWeather[ref][H_TAIR] = (float)m_SAResult[i].m_obs[H_T];
		//			if(m_SAResult[i].m_obs[H_PRCP]>-999)
		//				me.m_obsWeather[ref][H_PRCP] = (float)m_SAResult[i].m_obs[H_PRCP];
		//			if(m_SAResult[i].m_obs[H_TDEW]>-999)
		//				me.m_obsWeather[ref][H_TDEW] = (float)(m_SAResult[i].m_obs[H_TDEW]);
		//			if(m_SAResult[i].m_obs[H_RH]>-999)
		//				me.m_obsWeather[ref][H_RELH] = (float)max(0.0, Min(100, m_SAResult[i].m_obs[H_RH]));
		//			if(m_SAResult[i].m_obs[H_WIND_SPEED]>-999)
		//				me.m_obsWeather[ref][H_WNDS] = (float)(m_SAResult[i].m_obs[H_WIND_SPEED]);
		//			if(m_SAResult[i].m_obs[H_SRAD]>-999)
		//				me.m_obsWeather[ref][H_SRAD] = (float)(m_SAResult[i].m_obs[H_SRAD]);
		//			if(m_SAResult[i].m_obs[H_NRAD1]>-999)
		//				me.m_obsWeather[ref][H_ADD1] = (float)(m_SAResult[i].m_obs[H_NRAD1]);
		//			if(m_SAResult[i].m_obs[H_NRAD2]>-999)
		//				me.m_obsWeather[ref][H_ADD2] = (float)(m_SAResult[i].m_obs[H_NRAD2]);
		//		}
		//	}

		//	for(size_t i=0; i<m_SAResult.size(); i++)
		//	{
		//		CTRef ref = m_SAResult[i].m_ref;
		//		//if( me.m_obs.GetTPeriod().IsInside(m_SAResult[i].m_ref) )
		//		{

		//			//double Fcd = me.m_obs[ref].GetCloudiness(m_info.m_loc);
		//			//double Rln = me.m_obs[ref].GetNetLongWaveRadiationH(Fcd);
		//			//me.m_obs[ref][H_ADD3] = (float)Fcd;
		//			//me.m_obs[ref][H_ADD4] = (float)Rln;
		//			//me.m_obs[ref][H_ADD5] = (float)me.m_obs[ref].GetNetLongWaveRadiationH(1);
		//			
		//		}
		//	}
		//}

		//switch(m_model)
		//{
		//case DAILY_SINUS: 		//WD = GetWetnessDurationSinus(m_dailyData[y][m][d]); break;
		//case EXTENDED_TRESHOLD:	//WD = GetWetnessDurationExtT(hourlyDay); 			break;
		//case FIXED_THRESHOLD:	//WD = GetWetnessDurationFixT(hourlyDay); break;
		//case DPD:				//WD = GetWetnessDurationDPD(hourlyDay); break;
		//case CART:				break;//WD = GetWetnessDurationCART(hourlyDay); break;
		//case PM:
		//{
		//	CDewDuration PM;
		//	PM.m_loc=m_info.m_loc;
		//	PM.m_canopy=m_canopy;
		//	PM.m_exposure=m_exposure;
		//	PM.m_x0=m_x0;
		//	PM.m_x1=m_x1;
		//	PM.m_x2=m_x2;
		//	PM.m_x3=m_x3;
		//	PM.m_x4=m_x4;

		//	PM.Execute(m_obsWeather, stat);
		//	break;
		//}
		//case SWEB:
		//{
		//	CSWEB SWEB;
		//	SWEB.m_loc=m_info.m_loc;

		//	SWEB.m_LAI=m_LAI;
		//	SWEB.m_Zc=m_Zc;
		//	SWEB.m_x0 = m_x0;
		//	SWEB.m_x1 = m_x1;
		//	SWEB.m_x2 = m_x2;
		//	SWEB.m_x3 = m_x3;
		//	SWEB.m_x4 = m_x4;

		//	SWEB.Execute(m_obsWeather, stat);
		//	break;
		//}
		//default: ASSERT(false);
		//}//switch

		//

	}



	void CWetnessDurationModel::GetFValueHourly(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{
			//compute observation
			//if( m_obsH.empty() )
			//{
			//	CObsStatVector simStat;
			//	GetObsStatH(simStat);
			//}

			////CWetnessDurationStat hourlyStat;
			////OnExecuteHourly(hourlyStat);

			//for(size_t h=0; h<m_SAResult.size(); h++) 
			//{ 
			//
			//	CTRef TRef = m_SAResult[h].m_ref;
			//	if (simStat.IsInside(TRef) && m_SAResult[h].m_obs[H_WETNESS]>-999)
			//	{
			//		double obsDP = m_SAResult[h].m_obs[H_WETNESS];
			//		double simDP = simStat[TRef][WETNESS_DURATION];
			//		stat.Add(simDP,obsDP);
			//	}

			//	HxGridTestConnection();
			//		
			//}
		}
	}



	//********************************************************************************************************************
	void CWetnessDurationModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == NB_INPUT_DAILY + 5)
		{
			std::vector<double> obs(NB_INPUT_DAILY);

			CTRef TRef(ToInt(data[2]), ToSizeT(data[3]) - 1, ToSizeT(data[4]) - 1);
			for (int c = 0; c < NB_INPUT_DAILY; c++)
				obs[c] = ToDouble(data[5 + c]);

			ASSERT(obs.size() == NB_INPUT_DAILY);

			m_SAResult.push_back(CSAResult(TRef, obs));
		}

	}


	void CWetnessDurationModel::GetObsStat(CObsStatVector& stat)const
	{
		if (m_SAResult.empty())
			return;


		//stat.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)) );

		////Create sun information
		////CSun sun(m_info.weather.GetLocation().m_lat, m_info.m_loc.GetLon());
		//
		//if (m_obsWeather.empty())
		//{
		//	CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);

		//	//Create a copy of weather ad adjust it
		//	me.m_obsWeather = m_weather;

		//	if( m_SAResult[0].m_obs.size() == NB_INPUT_DAILY )
		//	{
		//		for(size_t i=0; i<m_SAResult.size(); i++)
		//		{
		//			//CTRef dailyRef = m_SAResult[h].m_ref;
		//			CTRef ref = m_SAResult[i].m_ref;
		//			if (m_obsWeather.GetEntireTPeriod().IsInside(m_SAResult[i].m_ref))
		//			{
		//				CWeatherDay& wDay = me.m_obsWeather.GetDay(ref);
		//				if (m_SAResult[i].m_obs[D_TMIN] > -999 && m_SAResult[i].m_obs[D_TMAX] > -999)
		//				{
		//					wDay[H_TAIR] = m_SAResult[i].m_obs[D_TMIN];
		//					wDay[H_TAIR] += m_SAResult[i].m_obs[D_TMAX];
		//				}

		//				if( m_SAResult[i].m_obs[D_PRCP]>-999)
		//					wDay.SetStat( H_PRCP, m_SAResult[i].m_obs[D_PRCP] );
		//				if( m_SAResult[i].m_obs[D_TDEW]>-999)
		//					wDay[H_TDEW] = m_SAResult[i].m_obs[D_TDEW];
		//				if( m_SAResult[i].m_obs[D_RH]>-999)
		//					wDay[H_RELH] = m_SAResult[i].m_obs[D_RH];
		//				if( m_SAResult[i].m_obs[D_WIND_SPEED]>-999)
		//					wDay[H_WNDS] = m_SAResult[i].m_obs[D_WIND_SPEED];
		//				if( m_SAResult[i].m_obs[D_SRAD]>-999)
		//					wDay[H_SRAD] = m_SAResult[i].m_obs[D_SRAD];
		//				
		//				//me.m_dailyData.SetData(ref, wDay);
		//			}
		//		}
		//	}

		//	//me.m_dailyData.AjusteMeanForHourlyRequest();
		//	//me.m_dailyData.GetHourlyVar(me.m_obs, m_info.m_loc);
		//	//CWeatherStation tmp(m_dailyData);
		//	//tmp.AjusteMeanForHourlyRequest(m_info.m_loc);
		//	//tmp.GetHourlyVar(me.m_obs, m_info.m_loc);
		//}


		//for(size_t y=0; y<m_obs.size(); y++)
		//{
		//	for(size_t  m=0; m<m_obs[y].size(); m++)
		//	{
		//		for(size_t  d=0; d<m_obs[y][m].size(); d++)
		//		{
		//			const CDay& hourlyDay = m_obs[y][m][d];

		//			double WD=0;
		//			double Tdew=0;
		//			switch(m_model)
		//			{
		//			case DAILY_SINUS: 		WD = GetWetnessDurationSinus(m_dailyData[y][m][d]); break;
		//			case EXTENDED_TRESHOLD:	WD = GetWetnessDurationExtT(hourlyDay); 			break;
		//			case FIXED_THRESHOLD:	WD = GetWetnessDurationFixT(hourlyDay); break;
		//			case DPD:				WD = GetWetnessDurationDPD(hourlyDay); break;
		//			case CART:				WD = GetWetnessDurationCART(hourlyDay); break;
		//			case PM:
		//				{
		//					if( m_PMStat.empty() )
		//					{
		//						CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
		//						CDewDuration PM;
		//						PM.m_loc=m_info.m_loc;
		//						PM.m_canopy=m_canopy;
		//						PM.m_exposure=m_exposure;
		//						PM.m_x0=m_x0;
		//						PM.m_x1=m_x1;
		//						PM.m_x2=m_x2;
		//						PM.m_x3=m_x3;
		//						PM.m_x4=m_x4;

		//						CWetnessDurationStat hStat;
		//						PM.Execute(m_obs, hStat);
		//						Hourly2Daily(hStat, me.m_PMStat);
		//						
		//					}
		//					
		//					WD = m_PMStat[m_dailyData[y][m][d].GetTRef()][WETNESS_DURATION];
		//					break;
		//				}
		//			case SWEB:
		//				{
		//					if( m_SWEBStat.empty() )
		//					{
		//						CWetnessDurationModel& me = const_cast<CWetnessDurationModel&>(*this);
		//						//Init PM
		//						CSWEB SWEB;
		//						SWEB.m_loc=m_info.m_loc;

		//						SWEB.m_LAI=m_LAI;
		//						SWEB.m_Zc=m_Zc;
		//						SWEB.m_x0 = m_x0;
		//						SWEB.m_x1 = m_x1;
		//						SWEB.m_x2 = m_x2;
		//						SWEB.m_x3 = m_x3;
		//						SWEB.m_x4 = m_x4;


		//						CWetnessDurationStat hStat;
		//						SWEB.Execute(m_obs, hStat);
		//						Hourly2Daily(hStat, me.m_SWEBStat);
		//					}
		//					
		//					WD = m_SWEBStat[m_dailyData[y][m][d].GetTRef()][WETNESS_DURATION];
		//					break;
		//				}
		//			default: ASSERT(false);
		//			}//switch

		//			stat[m_obs[y][m][d].GetTRef()][0] = WD;
		//		}//all days
		//	}
		//}
	}


	void CWetnessDurationModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{
			m_SWEBStat.clear();
			m_PMStat.clear();
			CWetnessDurationStat sim;
			ExecuteHourly(sim);

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{

				CTRef TRef = m_SAResult[i].m_ref;
				if (m_SAResult[i].m_obs[D_WETNESS_DURATION] > -999 && sim.IsInside(TRef) && sim[TRef][WETNESS_DURATION] > -999)
				{
					double obsWD = m_SAResult[i].m_obs[D_WETNESS_DURATION];
					double simWD = sim[TRef][WETNESS_DURATION];

					stat.Add(simWD, obsWD);
				}

				HxGridTestConnection();
			}

			//m_SWEBStat.clear();
			//m_PMStat.clear();
			//GetObsStat(m_obs);

			//for(size_t i=0; i<m_SAResult.size(); i++)
			//{
			//	
			//	CTRef TRef = m_SAResult[i].m_ref;
			//	if(m_SAResult[i].m_obs[D_WETNESS]>-999 && m_obs.IsInside(TRef) && m_obs[TRef][0] >-999)
			//	{
			//		double obsWD = m_SAResult[i].m_obs[D_WETNESS];
			//		double simWD = m_obs[TRef][0];
			//		
			//		stat.Add(simWD,obsWD);
			//	}

			//	HxGridTestConnection();
			//}
		}
	}

	//********************************************************************************************************************
	void CWetnessDurationModel::AddMonthlyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == NB_INPUT_MONTHY + 4)
		{
			std::vector<double> obs(NB_INPUT_MONTHY);

			if (ToDouble(data[M_WETNESS_DURATION]) > -999)
			{
				CTRef TRef(ToInt(data[2]), ToSizeT(data[3]) - 1);
				for (int c = 0; c < NB_INPUT_MONTHY; c++)
					obs[c] = ToDouble(data[4 + c]);

				ASSERT(obs.size() == NB_INPUT_MONTHY);
				m_SAResult.push_back(CSAResult(TRef, obs));
			}
		}

	}


	void CWetnessDurationModel::GetFValueMonthly(CStatisticXY& stat)
	{
		if (m_SAResult.size() > 0)
		{
			OnExecuteMonthly();
			const CWetnessDurationStat& data = (const CWetnessDurationStat&)GetOutput();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_obs[M_WETNESS_DURATION] > -999 && data.IsInside(m_SAResult[i].m_ref))
				{
					double obs = m_SAResult[i].m_obs[M_WETNESS_DURATION];
					double sim = data[m_SAResult[i].m_ref][WETNESS_DURATION];
					stat.Add(sim, obs);
				}

				HxGridTestConnection();

			}
		}
	}


	//********************************************************************************************************************

	//This method is call to load your parameter in your variable
	ERMsg CWetnessDurationModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_model = parameters[c++].GetInt();
		m_RHThreshold = parameters[c++].GetInt();

		//PM
		m_canopy = parameters[c++].GetInt();
		m_exposure = parameters[c++].GetInt();

		//SWEB
		m_LAI = parameters[c++].GetReal();
		m_Zc = parameters[c++].GetReal();

		if (parameters.size() == 11)
		{
			m_x0 = parameters[c++].GetReal();
			m_x1 = parameters[c++].GetReal();
			m_x2 = parameters[c++].GetReal();
			m_x3 = parameters[c++].GetReal();
			m_x4 = parameters[c++].GetReal();
			m_PMStat.clear();
			m_SWEBStat.clear();
		}


		return msg;
	}

	//********************************************************************************************************************
	//
	//Agricultural Meteorology, 25 (1982) 297--310 297
	//Elsevier Scientific Publishing Company, Amsterdam -- Printed in The Netherlands
	//ESTIMATING DEW DURATION UTILIZING STANDARD WEATHER STATION DATA
	//M.J. PEDRO, Jr.
	//Instituto Agronomico, CP28, 13100 Campinas, Sao Paulo (Brasil)
	//T.J. GILLESPIE
	//Department of Land Resource Science, University of Guelph, Guelph, Ont. (Canada)
	//(Received May 4, 1981; accepted October 19, 1981)
	const double CDewDuration::σ = 5.67e-08; //Stephan--Boltzman constant W/(m²·K4)];

	CDewDuration::CDewDuration()
	{
		m_canopy = CORN;
		m_exposure = EXPOSED_LEAF;
		m_cropHeight = 0.5;
	}

	//Rs: incoming solar radiation (Wm -2 )
	//RL: incoming long wave radiation (Wm -z)
	//Ta: air temperature (°C)
	//hc: heat and water vapour transfer coefficients for one side of the leaf (Wm -2 °C-1) 
	//hw: = water vapour transfer coefficients for one side of the leaf (W m -2 )
	//P = atmospheric pressure (mb)
	//esa = ambient saturated water vapour pressure (mb)
	//e = ambient vapour pressure (mb) 
	//ΔT[out]: difference between leaf and air temperature 
	double CDewDuration::GetΔT(double Rs, double RL, double Ta, double hc, double hw, double P, double esa, double e)
	{

		static const double a = 0.5;//short wave absorptivity of the leaf
		static const double oo[NB_CANOPY] = { 0.17,0.23,0.24 }; //albedo of the surface underneath the leaf 
		static const double ϵ = 0.95;//emissivity 

		//slope of the saturation vapour pressure curve at Ta [mbar/°C]
		double s = CASCE_ETsz::GetSlopeOfSaturationVaporPressure(Ta) * 10;


		//Equation [1]
		//The energy balance of an individual leaf can be solved for the difference between leaf and air temperature
		//as suggested by Kreith and Sellers (1975) and Norman (1979)
		double num = a * Rs*(1 + oo[m_canopy]) + ϵ * RL - ϵ * σ*Quart(Ta + 273) - (0.622 / P) * 2 * hw*(esa - e);
		double denom = 4 * ϵ*σ*Cube(Ta + 273) + 2 * hc + (0.622 / P) * 2 * hw*s;
		double ΔT = num / denom;

		return ΔT;
	}


	//Rs[in]: incoming solar radiation [W/m²]
	//RL[in]: incoming long wave radiation [W/m²]
	//Ta[in]: air temperature [°C]
	//hc[in]: heat and water vapour transfer coefficients for one side of the leaf [W/(m²°C)]
	//hw[in]: water vapour transfer coefficients for one side of the leaf  [W/m²]
	//P[in]: atmospheric pressure [mb]
	//esa[in]: ambient saturated water vapour pressure [mb]
	//e[in]: ambient vapour pressure [mb]
	//Tl[out]: leaf temperature (°C)

	double CDewDuration::GetTl(double Ta, double ΔT)
	{

		//Equation [2a]
		double Tl = Ta + ΔT;
		return Tl;
	}

	//hw[in]: water vapour transfer coefficients for one side of the leaf [W/m²]
	//P[in]: atmospheric pressure [mb]
	//esl[in]: saturated vapour pressure at Tl [mb]
	//e[in]: ambient vapour pressure [mb]
	//LE[OUT]: latent heat flux [W/m²]
	double CDewDuration::GetLE(double hw, double P, double esl, double e)
	{

		//Equation [2b]
		//latent heat flux (W/m² )
		double LE = -(0.622 / P) * 2 * hw*(esl - e);
		return LE;
	}

	//jDay[in]: Julian day (1..366)
	//δ[OUT]: solar declination [Radian]
	double CDewDuration::Getδ(double jDay)
	{
		double a = sin(Deg2Rad(360 * (jDay - 2) / 365.24));
		double b = sin(Deg2Rad(-24.44));
		double c = cos(Deg2Rad(360 * (jDay + 10) / 365.24 + 360 * a*0.0167 / PI));
		double δ = asin(b*c);
		return δ;
	}

	//λ[in]: latitude [Radian]
	//δ[in]: solar declination [Radian]
	//t[in]: hour (0..23)
	//tn[in]: solar noon (0..23)
	//Rst[out]: total estimated clear sky solar radiation [W/m²]
	double CDewDuration::GetRst(double λ, double δ, double t, double tn)
	{
		ASSERT(t >= 0 && t < 24);
		ASSERT(tn >= 0 && tn < 24);

		static const double A = 0.84;//clear sky atmospheric transmission coefficient
		static const double Rspo = 1361.0; //Solar constant (W/m²)

		double sinɸ = sin(λ)*sin(δ) + cos(λ)*cos(δ)*cos(Deg2Rad(15 * (t - tn))); //ɸ: the solar elevation angle
		double m = 1 / sinɸ;
		double Aᵐ = pow(A, m);

		//Equation [3]
		//The diffuse radiation was considered to be half the difference between 
		//the radiation on a horizontal surface below and above the atmosphere (List, 1971). 
		//the total estimated clear sky solar radiation (W/m²)
		double Rst = Aᵐ * Rspo*sinɸ + 0.5*Rspo*(1 - Aᵐ)*sinɸ;

		return max(0.0, Rst);//??
	}

	//Rst[in]: total estimated clear sky solar radiation [W/m²]
	//c[in]: fraction of sky covered by clouds
	//Rsc[out]: solar radiation for days with some cloud cover [W/m²]
	double CDewDuration::GetRsc(double Rst, double c)
	{
		//Equation [4]
		//The global solar radiation for days with some cloud cover (Rsc)
		//was obtained using the correction suggested by Mateer (1963) 
		double Rsc = Rst * (1.02 - (0.1831 / (1.27 - c)));
		return max(0.0, Rsc);
	}

	//λ[in]: latitude [Radian]
	//δ[in]: solar declination [Radian]
	//t: hour (0..23)
	//tn: solar noon (0..23)
	//Rsd[out]: shadded leaf diffuse solar radiation from clear sky [W/m²]
	double CDewDuration::GetRsd(double λ, double δ, double t, double tn)
	{
		const static double A = 0.84;
		const static double Rspo = 1380;//solar constant [w/m2]
		double sinɸ = sin(λ)*sin(δ) + cos(λ)*cos(δ)*cos(Deg2Rad(15 * (t - tn))); //ɸ: the solar elevation angle
		double m = 1 / sinɸ;

		double Aᵐ = pow(A, m);
		double Rst = 0.5*Rspo*(1 - Aᵐ)*sinɸ;

		return max(0.0, Rst);
	}


	//λ[in]: latitude [Radian]
	//δ[in]: solar declination [Radian]
	//t: hour (0..23)
	//tn: solar noon (0..23)
	//Rcd[out]: shadded leaf diffuse solar radiation with cloud [W/m²]
	double CDewDuration::GetRcd(double λ, double δ, double t, double tn)
	{
		double Rst = GetRst(λ, δ, t, tn);
		double Rcd = GetRsc(Rst, 1);
		return Rcd;
	}

	//For the shaded leaf it was necessary to estimate the diffuse solar radiation
	//(Rd) alone. Rd was obtained as the weighted sum of diffuse radiation from
	//the fraction of clear sky (Rsd), and diffuse radiation from the clouds (Rcd)
	//300
	//Rd = (1-- c)Rsd + c Rcd (5)
	//Rsd is the second term of eq. 3 and Rcd was obtained from eq. 4 with the
	//sky cover fraction set to unity. Comparing this method of estimation to data
	//from an Eppley pyranometer with a shading ring, yielded a 22 W m -2 S.E.E.
	//and an r 2 of 0.89 (Pedro, Jr., 1980).
	//Rd[out]: shadded leaf solar radiation  [W/m²]
	double CDewDuration::GetRd(double Rsd, double Rcd, double c)
	{
		//Equation [5]
		double Rd = (1 - c)*Rsd + c * Rcd;
		return Rd;
	}
	//Solar radiation on a sunlit leaf was estimated from eq. 3 when c = 0 and from eq. 4 when c>0. 
	//A comparison between measured and estimated values showed a standard error of estimate
	//(S.E.E.) of 23 W/m² and an r² value of 0.99 (Pedro, Jr., 1980).


	//Tws[in]: ambient temperature at the weather station [°C]
	//ews[in]: vapour pressure at the weather station [mb]
	//RL[OUT] incoming long wave radiation  [W/m²]
	double CDewDuration::GetRL(double Tws, double ews, double c)
	{
		//Equation [6]
		//The incoming long wave radiation was estimated according to Angstrom (1924) 
		//using empirical constants from Morgan et al. (1971)
		enum TCloud { CIRUS, CIRROTRATUS, ALTOCUMULUS, ALTOSTATUS, CUMULONIMBUS, CUMULUS, STATOCUMULUS, NIMBOSTRATUS, FOG, NB_CLOUD_TYPE };
		double K[NB_CLOUD_TYPE] = { 0.04, 0.08, 0.16, 0.20, 0.20, 0.20, 0.22, 0.25, 0.25 };

		//parameter dependent on cloud type.
		double k = K[ALTOCUMULUS]; //arbitrary choice??


		double RL = σ * Quart(Tws + 273)*(0.82 - 0.25*exp(-2.3*0.094*ews))*(1 + k * Square(c));
		return RL;
	}



	//In the apple tree the surrounding foliage was emitting longwave radiation
	//to the leaf since the DD measurements were made at 3/4 canopy height. A
	//correction was made to the incoming longwave radiation based on the observed
	//fraction of sky seen from that position (0.58) following Cain (1972)
	//RLt[out] = total incoming longwave radiation at the measurement site in the tree [W/m²]  
	double CDewDuration::GetRLt(double RL, double Ta)
	{
		//Equation [7]
		//The Angstrom formula gave better agreement with measured values of RL
		//at the experimental sites (S.E.E. = 12/m² , r² = 0.75) than other empirical
		//equations for RL available in the literature (Pedro, Jr., 1980).

		//RLt = total incoming longwave radiation (Wm -2) at the measurement site in the tree. 
		//No such correction was used for the corn and soybean canopies since DD measurements were made near the canopy tops.

		double RLt = 0.58*RL + 0.42*σ*Quart(Ta + 273);
		return RLt;
	}

	//For those periods of time when dew was forming or evaporating it was found that only very slight
	//	differences existed between vapour pressures in the crop (e) and at the weather station (ews).
	//	These differences did not significantly affect the ability of the model to estimate DD 
	//	and therefore it was taken that e = ews.




	//Tws[in]: ambient temperature at the weather station [°C]
	//Rn[in]: net radiation calculated from weather station data [W/m²]
	//Ta[OUT]: air temperatures in the canopies [°C]
	double CDewDuration::GetTa(double Tws, double Rn)
	{
		//The air temperatures in the canopies were obtained by using multiple
		//regression equations of the form
		//Crop	    A     B      C     r²   S.E.E.
		//Apple   0.446 0.953 0.00499 0.95   0.61
		//Corn    1.044 0.897 0.00572 0.98   0.49
		//Soybean 2.503 0.772 0.01143 0.97   0.62
		static const double A[NB_CANOPY] = { 1.04400, 2.50300, 0.44600 };
		static const double B[NB_CANOPY] = { 0.89700, 0.77200, 0.95300 };
		static const double C[NB_CANOPY] = { 0.00572, 0.01143, 0.00499 };

		//Equation [8]
		double Ta = A[m_canopy] + B[m_canopy] * Tws + C[m_canopy] * Rn;
		return Ta;
	}

	//Tws[in]: ambient temperature at the weather station [°C]
	//Rn[OUT]: net radiation calculated from weather station data [W/m²]
	double CDewDuration::GetRn(double Rs, double RL, double Tws)
	{
		//Equation [9]
		//The first term on the right side of eq. 9 assumes a shortwave absorptivity of 0.75 
		//while the second and third terms assume a longwave absorptivity or emissivity of 0.95.
		//Rs and RL were estimated from astronomical parameters and cloud cover fraction as described above.

		double Rn = 0.75*Rs + 0.95*RL - 0.95*σ*Quart(Tws + 273);

		return max(0.0, Rn);//???
	}


	//Uws: wind speed at the weather station 
	//Zws: (Zws = 10m)
	//Zc: crop height (m)
	double CDewDuration::GetUc(double Zc, double Zws, double Uws, double Rn)
	{
		//zero plane displacement (m) . The value of d was estimated (Campbell, 1977) as d = 0.64*Zc.
		double d = 0.64*Zc;

		//p is an exponent which depends on the roughness parameter and on the stability. 
		//Average values of the power law exponent (p) for corn, soybean, and apple as a function
		//of net radiation (Rn) and wind speed at the weather station (Uws)
		//Crop        Uws (m/s) at 10m          Rn (W/m²)
		//                                     
		//
		static const double p[3][2][3] =
		{
			// <-40  -40:100  >100
			{//Corn
				{0.80, 0.60, 0.45},	// < 3 m/s
				{0.45, 0.45, 0.45}	// > 3 m/s
			},
			{//Soybean
				{0.5, 0.4, 0.3},	// < 3 m/s
				{0.3, 0.3, 0.3}		// > 3 m/s
			},
			{//Apple
				{0.90, 0.70, 0.55},	// < 3 m/s
				{0.55, 0.55, 0.55},	// > 3 m/s
			}
		};

		int u = Uws < 3 ? 0 : 1;
		int r = Rn < -40 ? 0 : Rn <= 100 ? 1 : 2;

		//The power law profile was used to estimate the wind speed at crop height (Martin, 1971)
		//Euqation [10]
		// Uc: wind speed at crop height (m/s)
		double Uc = pow((Zc - d) / (Zws - d), p[m_canopy][u][r])*Uws;

		return Uc;
	}



	double CDewDuration::GetUh(double Uc, double Zh, double Zc)
	{

		static const double j = 1.2; //for an apple orchard (Landsberg et al., 1973).
		//Equation [11]
		//For apples, the wind estimates were complicated further because the
		//measurement site was at 3/4 canopy height rather than near the crop top.
		//Accordingly, wind at the measurement height (Uh) was determined from
		double Uh = Uc / Square(1 + j * (1 - Zh / Zc));
		return Uh;
	}

	//hc[OUT]: heat transfer coefficient [W/(m²·°C)]
	double CDewDuration::Gethc(double U)
	{
		//The role of convection was evaluated by adapting engineering heat transfer theory to plant foliage.
		//The heat transfer coefficient, hc(W m -2 °C-1 ) for one side of the leaf was found to be (Pedro, Jr. and Gillespie, 1982)

		const double static D[NB_CANOPY] = { 10,7,7 };//effective leaf dimension (cm) in the direction of the wind (cm)

		//Equation [12]
		double hc = 40 * sqrt(U / D[m_canopy]);
		return hc;
	}

	//L[IN]: is the latent heat of vaporization of water [J/kg]
	//Cp[IN]: specific heat of air [J/(kg·°C)]
	//hw: transfer coefficient for water vapour [W/m²]
	double CDewDuration::Gethw(double L, double Cp, double hc)
	{
		//Equation [13]
		//The transfer coefficient for water vapour, hw (W/m²) was calculated from
		//hc and the ratio of the molecular diffusivities for water vapour and heat (Ede, 1967)
		double hw = 1.07*(L / Cp)*hc;
		return hw;
	}






	//Two radiation environments were considered for horizontal leaves:
	//(1) An exposed leaf, which received the full rays of the sun during the morning and was open to the cold sky during the night.
	//(2) A shaded leaf, which was exposed to the cold sky during the night but was shaded from the sun during the morning. 
	//The total, global, solar radiation was assumed to fall on the exposed leaf while only diffuse solar radiation was assumed
	//for the shaded leaf. Both leaves were assumed to receive the total RL from the sky.

	//	
	//void CDewDuration::Execute(const CWeatherStation& weather, CWetnessDurationStat& stat)
	//{
	//	//CWeatherStation weather(weatherIn);
	//	ASSERT( weatherIn.IsHourlyAdjusted() == weather.IsHourlyAdjusted() );
	//	//weather.AjusteMeanForHourlyRequest(m_loc);
	//
	//
	//	//CYearsVector hourlyData;
	//	//weather.GetHourlyVar(hourlyData, m_loc);
	//
	//	CWetnessDurationStat hStat;
	//	Execute(m_weather, hStat);
	//
	//	CWetnessDurationModel::Hourly2Daily(hStat, stat);
	//}


	void CDewDuration::Execute(const CWeatherStation& weather, CWetnessDurationStat& stat)
	{
		ASSERT(weather.GetEntireTPeriod().GetNbRef() % 24 == 0);

		stat.Init(weather.GetEntireTPeriod());

		CSun sun(weather.GetLocation().m_lat, weather.GetLocation().m_lon);

		double lastWD = 0;
		for (size_t y = 0; y < weather.size(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < weather[y][m].size(); d++)
				{
					const CDay& hDay1 = weather[y][m][d];
					const CDay& hDay2 = weather[y][m][d].GetNext();

					//Julian day
					double jDay = (double)hDay2.GetTRef().GetJDay();

					//latitude [radians]:
					double λ = Deg2Rad(weather.GetLocation().m_lat);
					double δ = Getδ(jDay);
					double tn = sun.GetSolarNoon(hDay2.GetTRef());


					double test = 0;
					//int onset=-1;

					//double WD=0;
					for (int h = 0; h < 24; h++)
					{
						//int h = (hh+12)%24;
						//const CDay& hDay = (hh<12)?hDay1:hDay2;
						//int h = hh;
						const CDay& hDay = hDay2;

						//In the model, the onset of dew occurred when LE>0 and the ending occurred when the condensate which
						//had accumulated during the night evaporated during the morning (Mintah,1977).
						double c = hDay[h].GetVarEx(H_FNCD);
						double Tws = hDay[h][H_TAIR2];		//[°C]
						double ews = hDay[h][H_EA] * 10;	//[mb]
						double esaws = hDay[h][H_ES] * 10;	//[mb]   
						double Zws = 10;					//[m]
						double Uws = hDay[h][H_WNDS] * 1000 / 3600;		//km/h --> m/s
						double Rs1 = hDay[h][H_SRAD2]/**1000000/3600*/;	//MJ/(m²·h) --> W/m²;

						double Rs = Rs1;
						if (m_exposure == EXPOSED_LEAF)
						{
							//from eq. 3 when c = 0 and from eq. 4 when c > 0.
							double Rst = GetRst(λ, δ, h, tn);	//[W/m²]

							Rs = (c == 0) ? Rst : GetRsc(Rst, c);		//[W/m²]
						}
						else
						{
							double Rsd = GetRsd(λ, δ, h, tn);	//[W/m²]
							double Rcd = GetRcd(λ, δ, h, tn);	//[W/m²]
							Rs = GetRd(Rsd, Rcd, c);			//[W/m²]
						}

						double RL = GetRL(Tws, ews, c);	//[W/m²]
						double Rn = GetRn(Rs, RL, Tws);	//[W/m²]
						double Ta = GetTa(Tws, Rn);		//[°C]

						if (m_canopy == APPLE)//a correction for apple tree
						{
							RL = GetRLt(RL, Ta);			//[W/m²]
							Rn = GetRn(Rs, RL, Tws);	//[W/m²]
							Ta = GetTa(Tws, Rn);		//[°C]
							RL = GetRLt(RL, Ta);			//[W/m²]
						}

						double Zc = m_cropHeight;		//[m]
						double Uc = GetUc(Zc, Zws, Uws, Rn);	//[m/s]

						if (m_canopy == APPLE)
						{
							double Zh = 3.0 / 4.0*m_cropHeight;		//[m]
							double Uh = GetUh(Uc, Zh, Zc);			//[m/s]
							Uc = Uh;
						}

						////(2.5023e6 - 2430.54 * Tdl);
						//latent heat of vaporization 
						double L = 2260000;				//[J/kg]
						//Cp = specific heat of air 
						double Cp = 1004.67;			//[J/(kg·°C)]

						double hc = Gethc(Uc);			//[W/(m²·°C)]
						double hw = Gethw(L, Cp, hc);	//[W/m²]


						double P = hDay[h][H_PRES] * 10;	//[mb]
						double e = ews;					//[mb]   
						double esa = esaws;				//[mb]   //if e = ews, we assume that esa = esaws

						double ΔT = GetΔT(Rs, RL, Ta, hc, hw, P, esa, e);
						double Tl = GetTl(Ta, ΔT);
						double esl = eᵒ(Tl) * 10;	//[mb]

						//latent heat flux 
						double LE = GetLE(hw, P, esl, e);	//[W/m²]



						double WD = 0;
						//if(  hourlyData[y][m][d][h][H_PRCP]> m_x4)
						//{
							//WD=1;
						//NbVal=   720	Bias= 0.50639	MAE= 4.54889	RMSE= 5.77953	CD= 0.15263	R²= 0.25004
						//x0                  	=   0.01280  
						//x1                  	=   0.64924  
						//x2                  	=   0.04360  
						//x3                  	=   4.84347  
						//x4                  	=  99.20118  

						//test+=hourlyData[y][m][d][h][H_PRCP]*m_x4;
						//}

						test = max(0.0, test + LE);

						if (weather[y][m][d][h][H_PRCP] > 0.3)//0.15
						{
							WD = 1;
						}
						else
						{

							if (LE > 3.4)
							{
								WD = 1;
							}
							else if (LE < -8.4 && test < 710)
							{
								WD = 0;
							}
							else
							{
								WD = lastWD;
							}


							//if( LE>0 )
							//{
							//		test += LE*2;
							//		if(test>-250)//8
							//			WD=1;

							//}
							//else
							//{
							//		test += LE*2;
							//		if(test>0)///-8
							//		{
							//			WD=1;
							//		}
							//}
						}

						lastWD = WD;
						CTRef ref = weather[y][m][d][h].GetTRef();
						stat[ref][WETNESS_DURATION] = WD;//(float)
					}

					//double prcp = hourlyData[y][m][d].GetDailyStat(H_PRCP)[SUM];
					//WD = max(0.0, Min(24, WD+m_x4*prcp));

					//CTRef ref = hourlyData[y][m][d][h].GetTRef(); 
					//stat[ref][WETNESS_DURATION] = WD;


					//HxGridTestConnection();
				}
			}
		}
	}



	//*******************************************************************************************************
	//CSWEB

	//4 modifications is need to work like the Excel file
	//1- Dz = 2/3 of Zc and not Z
	//2- remove c form Uc calculation
	//3- Remove *0.5 from the Dp equation
	//4- Multiply Ep by *0.5 
	//5- we adjuste solar radiation. 

	//void CSWEB::Execute(const CWeatherStation& weatherIn, CWetnessDurationStat& stat)
	//{
	//	CWeatherStation weather(weatherIn);
	//	ASSERT( weatherIn.IsHourlyAdjusted() == weather.IsHourlyAdjusted() );
	//	//weather.AjusteMeanForHourlyRequest(m_loc);
	//
	//	//CYearsVector hourlyData;
	//	//weather.GetHourlyVar(hourlyData, m_loc);
	//
	//	CWetnessDurationStat hStat;
	//	Execute(hourlyData, hStat);
	//
	//	CWetnessDurationModel::Hourly2Daily(hStat, stat);
	//	//ASSERT( hStat.size() == weatherIn.GetNbDay()*24);
	//	//stat.Init(weatherIn.GetNbDay(), weatherIn.GetFirstTRef() );
	//	//
	//	//int WD=0;
	//	//for(int h=0; h<hStat.size(); h++)
	//	//{
	//	//	//int SW = GetSW(hStat[h][WETNESS_DURATION]);
	//	//	WD += hStat[h][WETNESS_DURATION]>0.1?1:0;//we cosidere dew if 10% of the leaf is wet
	//
	//	//	if( (h+1)%24 == 0)
	//	//	{
	//	//		CTRef TRef = stat.GetFirstTRef() + int(h/24);
	//	//		stat[TRef][WETNESS_DURATION] = WD;
	//	//		WD=0;
	//	//	}
	//	//}
	//}
	void CSWEB::Execute(const CWeatherStation& weather, CModelStatVector& stat)
	{
		ASSERT(weather.IsHourly());


		//specific gas constant for dry air 
		static const double Rspecific = 287.058; // J/(kg·K) 
		//The maximum water storage for a leaf is a function of age and species (Hall et al., 1997), however, a commonly accepted value for Cl is 0.02 cm (Noilhan and Planton, 1989).
		static const double Cl = 0.02;

		stat.Init(weather.GetEntireTPeriod(), NB_WETNESS_DURATION_STATS);

		//latitude [radians]:
		double ϕ = Deg2Rad(weather.GetLocation().m_lat);
		//maximum water storage for a canopy [cm]
		double C = GetC(m_LAI, Cl);
		//average fraction of wet area to total area of non-wettable leaves.
		double Wmax = GetWmax();
		//water storage [cm]
		double S = 0;

		//Wind speed reference
		double Z = 200;							//2 meters  --> 200 cm

		//Height of canopy [cm]
		double Zc = m_Zc * 100;						//m --> cm

		bool bReset = false;
		CStatistic LWRadDaySum;

		//CTRef firstDate(1997,JUNE,29,1);
		for (size_t y = 0; y < weather.size(); y++)
		{
			int year = weather[y].GetTRef().GetYear();
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < weather[y][m].size(); d++)
				{
					const CDay& hDay = weather[y][m][d];

					for (size_t hh = 0; hh < 24; hh++)
					{
						//if( CTRef(year,m,d,hh) >= firstDate)
						{
							//hourly prcp [cm]
							double P = hDay[hh][H_PRCP] / 10;
							//I = intercepted rain (cm for the hour)
							double I = GetI(m_LAI, P);

							//Atmopheric pressure [kPa]
							double ap = hDay[hh][H_PRES];
							//Psychrometric Constant [mbar/°C]
							double δ = CASCE_ETsz::GetPsychrometricConstant(ap) * 10;//kPa/°C  --> mbar/°C

							//hourly temperature [°C]
							double T = hDay[hh][H_TAIR2];
							//slope of the saturation vapor pressure-temperature curve [mbar/°C]
							double Δ = CASCE_ETsz::GetSlopeOfSaturationVaporPressure(T) * 10;
							//latent heat of vaporization [J/g]
							double λ = (2.5023e6 - 2430.54 * T) / 1000;//2260; //
							//double Rn =	max(0.0, -(hDay[hh][H_ADD1] - hDay[hh][H_ADD2]));


							double Fcd = hDay[hh].GetVarEx(H_FNCD);
							double Rln = CASCE_ETsz::GetNetLongWaveRadiationH(hDay[hh][H_TAIR2], hDay[hh][H_EA], Fcd);
							double Rn = 0;
							if (hDay[hh][H_SRAD2] > 0)
							{
								if (bReset)
								{
									LWRadDaySum.Reset();
									bReset = false;
								}

								LWRadDaySum += Rln;
							}
							else if (LWRadDaySum[NB_VALUE] > 0)
							{
								bReset = true;


								//with hourly values
								//NbVal=  8974	Bias= 0.00016	MAE= 0.08764	RMSE= 0.19461	CD= 0.69271	R²= 0.69664
								//x0                  	=   0.22373 {   0.22254,   0.22595}	VM={   0.00025,   0.00077}
								//x1                  	=  -0.85909 {  -0.87238,  -0.85324}	VM={   0.00160,   0.00481}
								//x2                  	=  -0.42102 {  -0.42705,  -0.41845}	VM={   0.00061,   0.00166}
								//x3                  	=   2.42150 {   2.40635,   2.45109}	VM={   0.00488,   0.00855}

								//with daily values 1996-1997
								//NbVal=   427	Bias= 0.53396	MAE= 3.35363	RMSE= 4.50189	CD= 0.52130	R²= 0.56656
								//x0                  	=  -0.08786  
								//x1                  	=  -1.55769  
								//x2                  	=   1.91481  
								//x3                  	=  -2.59569  
								//with daily values 1996-1998
								//NbVal=   720	Bias= 0.36889	MAE= 3.53639	RMSE= 4.79616	CD= 0.41645	R²= 0.48481
								//x0                  	=  -0.03150  
								//x1                  	=  -0.48987  
								//x2                  	=   0.58576  
								//x3                  	=   0.45235  
								Rn = max(0.0, 0 - 0.5*Rln + 0.5*sqrt(LWRadDaySum[MEAN]) + 0.5*Rln*sqrt(LWRadDaySum[MEAN]));
								//Rn = max(0.0, m_x0 + m_x1*Rln + m_x2*sqrt(LWRadDaySum[MEAN])+m_x3*Rln*sqrt(LWRadDaySum[MEAN]));


								//Rn = max(0.0, 0.22373 -0.85909*Rln -0.42102*sqrt(LWRadDaySum[MEAN])+ 2.42150*Rln*sqrt(LWRadDaySum[MEAN]));
								//Rn = max(0.0, 0.21902 - 1.44633*Rln - 0.01555*sqrt(LWRadDaySum[MEAN]) + 3.24653*Rln*sqrt(LWRadDaySum[MEAN]));
								//Rn = max(0.0, -0.08786  - 1.55769*Rln + 1.91481*sqrt(LWRadDaySum[MEAN])- 2.59569*Rln*sqrt(LWRadDaySum[MEAN]));
							}



							if (hDay[hh][H_ADD1] > -999 && hDay[hh][H_ADD2] > -999)
							{
								double Rad1 = hDay[hh][H_ADD1];
								double Rad2 = hDay[hh][H_ADD2];
								Rn = max(0.0, -(Rad1 - Rad2));
							}


							double Rnc = Rn * 1000000 / (60 * 10000);		//MJ/(m²·h) --> J/(cm²·min)
							//Potential condensation [cm³/(cm²·min)]
							double Dp = GetDp(Δ, λ, δ, Rnc);
							//Potential condensation [cm/h]
							double D = Dp * 60.0;			//cm/min  --> cm (for the hour)


							double Si = max(0.0, min(C, S + I + D));

							//Initial canopy water budget 
							double Wind = GetWind(Si, C);
							//actual fraction of wet area to total canopy surface area
							double W = GetW(Wind, Wmax);
							//c = shape scale variable (cm0.5/min0.5)
							double c = Getc(W);

							//Wind speed at 10 meters[m/s]

							//Wind speed at 2 meters[m/s]
							double U = hDay[hh][H_WND2] * 1000.0 / 3600.0;	//km/h  --> m/s

							//U = CASCE_ETsz::GetWindProfileRelationship(U,10);
							//Wind speed at 2 meters[cm/min]
							double Uz = U * 60.0*100.0;	//m/s  --> cm/min
							//wind speed at average height of the canopy [cm/min]
							double Uc = GetUc(Uz, Z, Zc);

							//hourly temperature [Kelvin]
							double Tk = hDay[hh][H_TAIR2] + 273.15;
							//p = density of air [g/cm³]
							double p = ap / (Rspecific*Tk);//Kpa·Kg/J = g/cm³
							//Cp = specific heat of air [J/(g·°C)]
							double Cp = 1.0045; //specific heat of air at constant pressure [J/(g·°C)]
							//h = transfer coefficient for heat and vapor from the surface to the atmosphere [cm/min]
							double h = Geth(c, Uc);
							//water vapor pressure of the atmosphere [mbar]
							double Ea = hDay[hh][H_EA] * 10.0;			//kPa  --> mbar
							//saturated water vapor pressure of the atmosphere [mbar]
							double Es = hDay[hh][H_ES] * 10.0;			//kPa  --> mbar

							//potential latent heat flux density (evaporation) [cm³/(cm²·min)]. (potential volume of water loss from a wet surface area)
							double Ep = GetEp(Δ, λ, δ, p, Cp, h, Ea, Es)*0.5;//add a 0.5 factor to get same result as the Excel file
							//E: Evaporation (cm/h)
							double E = GetE(Ep, W) * 60;   //cm/min  --> cm/h

							//update water storage [cm]
							S = max(0.0, min(C, Si - E));

							CTRef Tref(year, m, d, hh);
							W = GetWind(S, C);
							stat[Tref][WETNESS_DURATION] = W;
						}
					}
				}
			}
		}
	}

	//Table 2
	//Main inputs, variables and constants used in the SWEB model
	//Symbol Variable/constant
	//C Max water storage, C (cm)
	//Cl Max water storage per leaf area (cm)
	//c Shape scale variable (cm0.5 min0.5)
	//cd Shape scale constant for drops (cm0.5 min0.5)
	//cf Shape scale constant for film (cm0.5 min0.5)
	//Dp Potential condensation (cm min1)
	//E Evaporation (cm min1)
	//h Transfer coefficient (cm min1)
	//I Interception (cm)
	//LAI Leaf area index
	//P Precipitation (cm)
	//p Fraction of wettable leaves to total leaves in canopy
	//RHc Canopy relative humidity (%)
	//Rnc Canopy net radiant flux (J/(min·cm²) )
	//S Canopy water budget (cm)
	//Si Initial canopy water budget (cm)
	//SW Surface wetness
	//Tc Canopy air temperature (8C)
	//Uc Canopy wind speed (cm min1)
	//UZ Wind speed at reference height (cm min1)
	//W Actual fraction of wet area to total canopy surface area
	//Wd Average fraction of wet area to total area of
	//non-wettable leaves.
	//Wf Average fraction of wet area to total area of wettable
	//leaves.
	//Wind Index of fraction of canopy wet surface area
	//Wmax Max wet area (Wmax)
	//Wth Wetness threshold
	//Zc Height of canopy (cm)


	//The surface wetness (SW) is estimated from the index of the fraction of canopy wet surface area, 
	//Wind during a given time interval as shown in Eqs. (1) and (2):

	//SW = surface wetness, 
	//Wind = index of fraction of canopy wet surface area
	//Wth = surface wetness threshold (0.1).
	int CSWEB::GetSW(double Wind)
	{
		static const double Wth = 0.1;
		//Equation [1] and Equation [2]
		int SW = (Wind >= Wth) ? 1 : 0;
		return SW;
	}

	//S[IN] = canopy water storage (cm) 
	//C[IN] = maximum canopy water storage (cm).
	//Wind[OUT] = index of fraction of canopy wet surface area
	double CSWEB::GetWind(double S, double C)
	{
		//Wind is estimated from: (i) the relative volume of water stored in the canopy and (ii) the change in surface area to volume ratio during drying. 
		//Wet surface area decreases in a non-linear fashion as the water stored in a canopy decreases due to the volume-to-area argument. 
		//hat is, the water volume changes as a cubic power, while the wet area changes as a square (Deardorff, 1978). 
		//Equation [3]
		double Wind = pow(S / C, 0.6667);

		return Wind;
	}

	//Wind[IN] = index of fraction of canopy wet surface area
	//Wmax[IN] = maximum fraction of canopy allowed as wet surface area.
	//W[OUT] = actual fraction of wet area to total canopy surface area
	double CSWEB::GetW(double Wind, double Wmax)
	{

		//W is dependant on Wind and a factor that accounts for the relative wettability of the leaf
		//Equation [4]
		double W = Wind * Wmax; ASSERT(W >= 0);
		return W;
	}




	//p = fraction of wettable leaves to total leaves in canopy
	//Wf = average fraction of wet area to total area of wettable leaves, (1-p) = fraction of non-wettable leaves to total leaves in canopy
	//Wd = average fraction of wet area to total area of non-wettable leaves.
	double CSWEB::GetWmax()//double p, double Wf, double Wd)
	{
		//The factor Wmax is assumed to be the same for both dew and rain. 
		//When a leaf surface is wettable, it favors water distribution as a film. When a leaf surface is non-wettable, it  water distribution as drops. 
		//A simple technique for determining Wmax in terms of leaf wettability is to partition the canopy leaf area into wettable and nonwettable
		//fractions and then assign a proportion of maximum wet area to each fraction
		//Equation [5]
			//double Wmax = p*Wf+ (1-p)*Wd;
			//double Wmax = 0.5;

			//The proportion of wettable and non-wettable leaves could be based on observed or estimated leaf age. Values of Wd and Wf could be determined 
			//by selecting representative wettable and non-wettable leaves and determining the fraction of wet surface area in the laboratory after simulated rain or dew. 
			//In our studies, we used a value of 0.5 for Wmax since tests in the lab showed immature grape leaves are highly wettable and mature leaves are non-wettable.
		static const double Wmax = 0.5;

		return Wmax;
	}

	//LAI = leaf area index
	//Cl = maximum water storage for an average leaf (cm). The LAI for a grape canopy can be either measured (Grantz and Williams, 1993) or estimated from a crop model. 
	//C = maximum water storage for a canopy (cm)	
	double CSWEB::GetC(double LAI, double Cl)
	{
		//In the construction of the canopy water budget, the maximum water storage is computed by multiplying the leaf area index (LAI) and the maximum water storage
		//for a single leaf (Cl) having average properties of all leaves. 
		//Equation [6]
		double C = LAI * Cl;
		return C;

	}


	//I[in] = intercepted rain (cm),
	//D[in]p = potential condensation of dew (cm)
	//E[in] = Evaporation (cm)
	//S[OUT] = water storage (cm)
	double CSWEB::GetS(double I, double Dp, double E)
	{
		//With the maximum fraction of canopy allowed as wet surface area and maximum water storage determined for a canopy, the water budget can be computed
		//in terms of intercepted precipitation, condensation and evaporation from a vegetative surface. 


		//Equation [7]
		double S = I + Dp - E;


		return S;
	}


	//LAI[IN] = leaf area index 
	//P[IN] = precipitation as rain (cm)
	//I[OUT] = intercepted rain (cm),
	double CSWEB::GetI(double LAI, double P)
	{
		//The canopy water budget changes over time. In the model time steps, interception and condensation add to the water budget. 
		//If the balance is positive then evaporation may subtract from the budget. The equation for the interception of precipitation, which is assumed
		//to be rain, was derived from the work of Norman and Campbell (1983)
		//Equation [8]
		double I = (1 - exp(-0.5*LAI))*P;

		return I;
	}


	//Δ = slope of the saturation vapor pressure curve (mbar/°C)
	//λ = latent heat of vaporization (J/g)
	//δ = psychrometric constant (mbar/°C),
	//p = density of air (g/cm³)
	//Cp = specific heat of air (J/(g°C) )
	//h = transfer coefficient for heat and vapor from the surface to the atmosphere (cm/min),
	//Ea = water vapor pressure of the atmosphere (mbar),
	//Es = saturated water vapor pressure of the atmosphere (mbar).
	//Ep[OUT] = potential latent heat flux density (evaporation) (cm/min). (potential volume of water loss from a wet surface area)
	double CSWEB::GetEp(double Δ, double λ, double δ, double p, double Cp, double h, double Ea, double Es)
	{

		//The condensation and evaporation processes in the canopy water budget are based on a combination formulation developed by Tanner and Fuchs (1968).
		//Equation [9] is only for explanation
		//According to the original assumptions of the model, daytime net radiation is assumed not to contribute to evaporation in a shaded grape canopy. 
		//Equation 10
		double Ep = Δ / (λ*(Δ + δ))*(p*Cp*(h / Δ)*(Es - Ea));
		ASSERT(Ep >= 0);
		return Ep;
	}

	//Ep[IN] = potential latent heat flux density (evaporation) (cm min1). (potential volume of water loss from a wet surface area)
	//W[IN] = actual fraction of wet area to total canopy surface area
	//E: Evaporation (cm/min)
	double CSWEB::GetE(double Ep, double W)
	{

		//In order to compute the actual total moisture lost from the entire canopy, the evaporation, E must be multiplied by the actual fraction of canopy wet surface area. 
		//Equation [11]
		double E = Ep * W;
		return E;
	}


	//Δ[IN] = slope of the saturation vapor pressure curve (mbar/°C)
	//λ[IN] = latent heat of vaporization (J/g)
	//δ[IN] = psychrometric constant (mbar/°C),
	//Rnc[IN] = Canopy net radiant flux (J/(min·cm²) )
	//Dp[OUT] = the potential condensation of dew (cm/min) 
	double CSWEB::GetDp(double Δ, double λ, double δ, double Rnc)
	{
		//In addition to evaporation, at night the contribution of condensation must also be considered. Since the aerodynamic term cannot be negative, 
		//only the net radiation term makes a contribution to the potential condensation of dew 
		//Equation [12]
		double Dp = Δ / (λ*(Δ + δ))*Rnc*0.5;//remove the 0.5 factor to get same result than Excel file
		return Dp;
	}


	//The radiant flux at the average height of the canopy is assumed to be 50% of the total canopy flux. This is based upon the original assumption of the model that
	//SWEB represents the canopy as a big leaf at the average height of the canopy. In summary, the energy and water balance is calculated from the aerodynamic term
	//(Eqs. (10) and (11)) during the day and at night from both aerodynamic term and the potential condensation.




	//c = shape scale constant of an object (cm0.5 min0.5)
	//Uc = canopy wind speed (cm/min).
	//h = transfer coefficient (cm/min)
	double CSWEB::Geth(double c, double Uc)
	{
		//The transfer coefficient in Eqs. (9) and (10), h, was based on a generic transfer coefficient that accounted for the influence of wind speed and an object’s shape
		//and size by Bird et al. (1960; Magarey et al., 2005a).

		//Equation [23]
		double h = c * sqrt(Uc);///LL = 4.5 in the Excel code
		return h;
	}




	//W = actual fraction of canopy wet surface area in previous time step
	//c = shape scale variable (cm0.5 min0.5)
	double CSWEB::Getc(double W)
	{
		//The values of cd and cf were determined from controlled laboratory tests of the drying of drops and films on an artificial leaf surface representing a typical
		//hydrophobic leaf surface to be 8.7 and 2.0, respectively (Magarey et al., 2005a).

		//dans l'article
		//static const double cd = 8.7;//shape scale constant for drops (cm0.5 min0.5).
		//static const double cf = 2.0;//shape scale constant for film (cm0.5 min0.5)

		//dans le fichier Excel
		static const double cd = 18.2;//shape scale constant for drops (cm0.5 min0.5).
		static const double cf = 4.1;//shape scale constant for film (cm0.5 min0.5)

		//In the present study, the shape scale constant, c, becomes a variable in the SWEB model. As a leaf surface becomes wetter, the moisture transfer increasingly
		//behaves as if the water shape is a film. Conversely, as a leaf surface becomes drier, the moisture transfer behaves as if the water shape is a drop. 
		//To account for this change in behavior due to water shape, the shape scale variable, c, is weighted between constants for drops, cd and film,
		//cf according to the fraction of the leaf surface that is wet.
		//Equation [14]
		double c = W * cf + (1 - W)*cd;
		return c;
	}




	//Uz = wind speed (cm/min) 
	//Z =  reference height of Uz (cm)
	//Zc = height of canopy (cm)
	//Uc = wind speed at average height of the canopy (cm/min)
	double CSWEB::GetUc(double Uz, double Z, double Zc)
	{
		//Uz = Uz*1000;

		//static const double α = 1.3;
		double Dz = 2.0 / 3 * Zc; //zero plane displacement (cm), Use Zc instead of Z
		double Zo = 1.0 / 10 * Zc; //Zo = roughness length (cm)	
		//The canopy wind speed, Uc is affected by the height and density of a canopy. The canopy wind speed is computed from the logarithmic wind profile equation
		//(Monteith and Unsworth, 1990) and an analytical wind speed profile (Landsberg and James, 1971). 
		//Equation [15]
		double a = log((Zc - Dz) / Zo);
		double b = log((Z - Dz) / Zo);
		double Uc = Uz * a / b;//don't use c factor
		return Uc;
	}



	void CWetnessDurationModel::Hourly2Daily(const CWetnessDurationStat& hStat, CWetnessDurationStat& stat)
	{
		ASSERT(hStat.size() % 24 == 0);
		stat.Init(hStat.GetTPeriod().Transform(CTRef::DAILY));

		int WD = 0;
		for (size_t h = 0; h < hStat.size(); h++)
		{
			WD += (int)(hStat[h][WETNESS_DURATION] > 0.1 ? 1 : 0);//the hour is wet if 10% of the leaf is wet

			if ((h + 1) % 24 == 0)
			{
				CTRef TRef = stat.GetFirstTRef() + int(h / 24);
				stat[TRef][WETNESS_DURATION] = WD;
				WD = 0;
			}
		}
	}

}







/*void CSWEB::Execute(const CYearsVector& hourlyData, CWetnessDurationStat& stat)
{

	//specific gas constant for dry air
	//static const double Rspecific = 287.058; // J/(kg·K)
	//The maximum water storage for a leaf is a function of age and species (Hall et al., 1997), however, a commonly accepted value for Cl is 0.02 cm (Noilhan and Planton, 1989).
	static const double Cl = 0.02;

	stat.Init(hourlyData.GetTPeriod().GetNbRef(), hourlyData.GetFirstTRef() );

	//latitude [radians]:
	double ϕ = Deg2Rad(weather.GetLocation().m_lat);
	//maximum water storage for a canopy [cm]
	double C = GetC(m_LAI, Cl);
	//average fraction of wet area to total area of non-wettable leaves.
	double Wmax = 0.5;//GetWmax();
	//water storage [cm]
	double Si = 0;
	double S = 0;

	//Wind speed reference
	double Z=3;							//2 meters  --> 200 cm
	//Height of canopy [cm]
	double Zc=m_Zc;						//m --> cm

	CTRef firstDate(1997,JUNE,29,0);
	for(size_t y=0; y<hourlyData.size(); y++)
	{
		short year = hourlyData[y].GetTRef().GetYear();
		for(size_t m=0; m<12; m++)
		{
			for(size_t d=0; d<hourlyData[y][m].size(); d++)
			{
				const CDay& hDay = hourlyData[y][m][d];
				//hourlyData[y][m][d].CompileDailyStat();

				//Atmopheric pressure [kPa]
				//double test = GetAtmosphericPressure(m_elev);//kPa
				//double ap = hDay.GetDailyStat(H_PRES)[MEAN];
				//Psychrometric Constant [mbar/°C]
				//double δ = GetPsychrometricConstant(ap)*10;//kPa/°C  --> mbar/°C


				//daylight average air temperature [°C]
				//double Tdl = hourlyData[y][m][d].GetTdaylight(m_loc);




				//int WetnessDuration=0;
				for(int hh=0; hh<24; hh++)
				{
					if( CTRef(year,m,d,hh) >= firstDate)
					{
						//Get net radiation [MJ/(m²·h)]: get only net outgoing radiation
						//double Fcd = hDay.GetCloudiness(hh, m_loc);
						//double Rn =	hDay.GetNetLongWaveRadiationH(hh, Fcd);//
						double CNR = hDay[hh][H_ADD1] - hDay[hh][H_ADD2];   //net canopy in W/m²
						double RH = hDay[hh][H_RELH];

						//It use 1 calorie = 4.1876 joule
						//CNR = CNR*0.00002388*60*0.5;		//W/m²   --> cal/(min·cm²)  dans mathlab il y a un 0.5?????

						//Net canopy in calorie/(min·cm²)
						CNR = CNR*60/(4.1876*10000);		//W/m²   --> cal/(min·cm²)

						//ASSERT(Rn>=0);
						//double Rn =	max(0.0, -hDay.GetNetRadiation(weather.GetLocation().m_lat,m_loc.GetLon(),m_loc.GetElev(),hh));
						//Canopy net radiant flux [J/(cm²·min)]
						//double Rnc = Rn*1000/600;				//MJ/(m²·h) --> J/(cm²·min)
						//hourly temperature [°C]
						double T = hDay[hh][H_TAIR];
						//hourly temperature [Kelvin]
						//double Tk = hDay[hh](H_TK);
						//hourly prcp [cm]
						double P = hDay[hh][PRCP]/10;
						//water vapor pressure of the atmosphere [mbar]
						//double Ea=hDay[hh](H_EA)*10;			//kPa  --> mbar
						double Ea=6.11*exp(5327.0*(1.0/273.0-1.0/(273.0+T)));

						//saturated water vapor pressure of the atmosphere [mbar]
						//double Es=hDay[hh](H_ES)*10;			//kPa  --> mbar
						//Wind speed [cm/min]
						//double Uz=hDay[hh](H_WNDS)*100000/60;	//km/h  --> cm/min
						double Uz=hDay[hh](H_WNDS);//*100*60;		//m/s  --> cm/min

						//latent heat of vaporization [J/g]
						//double λ = 2260; //(2.5023e6 - 2430.54 * Tdl)/1000;
						//slope of the saturation vapor pressure-temperature curve [mbar/°C]
						//double Δ = CASCE_ETsz::GetSlopeOfSaturationVaporPressure(T)*10;
						double Δ = Ea*(6790.4985/Square(T+273) - 5.02808/(T+273));
						double ΔP = Δ + 0.66;
						double Rn = CNR*Δ/ΔP;

						// They used 583 calorie/cm³
						double Dp = max(0.0, -Rn*60/583);				//cal/(min·cm²)  -->  cm/h

						//I = intercepted rain (cm for the hour)
						double I = GetI(m_LAI, P);


						Si = max(0.0, Min(C, S+I+Dp));


						//Initial canopy water budget
						double Wind = GetWind(Si, C);
						//actual fraction of wet area to total canopy surface area
						double W = GetW(Wind, Wmax);



						//Cp = specific heat of air
						double Cp = 0.286; //specific heat of air at constant pressure cal/(g·°C)

						//p = density of air [g/cm³]
						double p=0.0012;//g/cm³
						double coef1 = Cp*p/ΔP;
						double Rh2 = (1-RH/100);

						//Potential condensation [cm/min)
						//double Dp = GetDp(Δ, λ, δ, Rnc)*60;  //cm/min  --> cm (for the hour)
						//double Dp = 60.0/583.0*Rnc*Δ/(Δ+δ);



						//c = shape scale variable (cm0.5/min0.5)
						double c = Getc(W);
						//Uc = wind speed at average height of the canopy [cm/min]
						double Dz = 1.3; //2.0/3*Zc; //zero plane displacement (cm)
						double Zo = 0.2; //1.0/10*Zc; //Zo = roughness length (cm),
						double Uc = Uz*log((Zc-Dz)/Zo)/log((Z-Dz)/Zo);
						//double Ucl = Uc*pow(1+1.3*(1-1.05/1.8), -2);
						//h = transfer coefficient for heat and vapor from the surface to the atmosphere [cm/min]
						double LL=4.5;
						double h=c*sqrt(Uc*6000/LL);


						//potential latent heat flux density (evaporation) [cm/min]. (potential volume of water loss from a wet surface area)
						//double Ep = GetEp(Δ, λ, δ, p, Cp, h, Ea, Es);
						//E: Evaporation (cm/min)
						//double E = GetE(Ep, W)*60;   //cm/min  --> cm (for the hour)
						double E = (Rh2*coef1*Ea)/583 * h*W;
						double volE=E*60;
						double minusWB = 0 + volE;

						CTRef Tref(year,m,d,hh);
						if( Tref == CTRef(1997, AUGUST, 11, 20 ) )
						{
							int y;
							y=0;

						}
						//update water storage [cm]
						//Si += GetS(I, Dp, E);
						S = max(0.0, Min(C, Si-minusWB));

						double Wr = GetWind(S, C);
						//actual fraction of wet area to total canopy surface area
						//double Wr = GetW(relWb, Wmax);

						//Canopy wet surface area (Wind)
						//int SW = GetSW(Wind);


						stat[Tref][HOURLY_WETNESS_PERIOD] = Wr;


					}
				}
			}
		}
	}
}
*/
