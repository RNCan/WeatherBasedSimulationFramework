//**********************************************************************
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 06/04/2013			Rémi Saint-Amant	Create
//**********************************************************************

#include <math.h>
#include <crtdbg.h>
#include "Basic/Evapotranspiration.h"
#include "ModelBase/EntryPoint.h"
#include "..\Climatic\WetnessDuration.h"
#include "MagareyInfection.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	const char MAGAREY_HEADER[] = "INFECTION|CUMUL_INFECTION|RAIN_INFECTION|CUMUL_RAIN_INFECTION|INFECTION_EVENT";

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CMagareyInfectionModel::CreateObject);


	CMagareyInfectionModel::CMagareyInfectionModel()
	{
		NB_INPUT_PARAMETER = 4;
		VERSION = "1.1.1 (2018)";

		m_pathogenType = 0;
		m_prcpThreshold = 2;
		m_interruption = 1;

	}

	CMagareyInfectionModel::~CMagareyInfectionModel()
	{}


	ERMsg CMagareyInfectionModel::OnExecuteDaily()
	{
		ERMsg msg;


		CMagareyInfectionStatVector output;

		CMagareyInfection model;
		model.m_pathogenType = m_pathogenType;
		model.m_prcpThreshold = m_prcpThreshold;
		model.m_interruption = m_interruption;
		model.m_beginning = m_beginning;
		model.m_loc = m_info.m_loc;
		model.Execute(m_weather, output);

		SetOutput(output);

		return msg;
	}

	//this method is call to load your parameter in your variable
	ERMsg CMagareyInfectionModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_pathogenType = parameters[c++].GetInt();

		//Precipitation threshold for infection
		m_prcpThreshold = parameters[c++].GetReal();
		m_interruption = parameters[c++].GetReal();
		m_beginning = parameters[c++].GetTRef();

		if (!m_beginning.IsInit())
			msg.ajoute("Invalid beginning date");



		return msg;
	}
	//
	//
	//void CMagareyInfectionModel::AddDailyResult(const StringVector& header, const StringVector& data)
	//{
	//	ASSERT( header.size() == NB_PARAM*2+1 );
	//	ASSERT( header[0] == "ID");
	//	
	//
	//	std::vector<double> obs(16);
	//	for(int i=0; i<16; i++)
	//		obs[i] = data[i+1].ToDouble();
	//	
	//	m_SAResult.push_back( CSAResult(CTRef(), obs ) );
	//}
	//
	//void CMagareyInfectionModel::GetFValueDaily(CStatisticXY& stat)
	//{
	//	ERMsg msg;
	//
	//	if( m_SAResult.size() > 0)
	//	{
	//		//look to see if all ai are in growing order
	//		bool bValid=true;
	//		for(int s=1; s<NB_PARAM&&bValid; s++)
	//		{
	//			if( m_CR.m_a[s] < m_CR.m_a[s-1] )
	//				bValid = false;
	//		}
	//		
	//		CStatistic statDD[NB_PARAM];
	//		CStatistic::SetVMiss(-999);
	//		if( bValid )
	//		{
	//			CModelStatVector statSim;
	//			m_CR.Execute(m_weather, statSim);
	//			for(int d=0; d<statSim.size(); d++)
	//			{
	//				for(int s=0; s<NB_PARAM; s++)
	//				{
	//					if( statSim[d][CCanolaContinuingRatio::O_FIRST_STAGE+s+1] > 1 &&
	//						statSim[d][CCanolaContinuingRatio::O_FIRST_STAGE+s+1] < 99 )
	//					{
	//						double DD =statSim[d][CCanolaContinuingRatio::O_DD]; 
	//						statDD[s]+=DD;
	//					}
	//				}
	//			}
	//		}
	//
	//		
	//		for(int s=0; s<NB_PARAM; s++)
	//		{
	//			for(int t=0; t<2; t++)
	//			{
	//				double obs= m_SAResult[0].m_obs[s*2+t];
	//				double sim= statDD[s][t==0?LOWEST:HIGHEST];
	//				stat.Add(sim, obs); 
	//			}
	//		}
	//	}
	//}
	//
	//
	//
	//
	//****************************************************************************************************




	//Pathogen						Crop				Tmin	Tmax	Topt	Wmin	Wmax
	//Albugo occidentalis			Spinach				6		28		16		3		12
	//Alternaria brassicae			Oilseed rape		2.6		35		18		6		22
	//Alternaria cucumis			Muskmelon			12		25		19		8		24
	//Alternaria mali				Apple				1		35		23		5		40
	//Alternaria pori				Onion				1		35		23		8		24
	//Alternaria sp.				Mineola tangelo		9.4		35		25		8		16
	//Ascochyta rabiei				Chick pea			1		35		25		12		48
	//Bipolaris oryzae				Rice				8		35		27.5	10		24
	//Botryosphaeria dothidea		Apple				8		35		28		8		19
	//Botryosphaeria obtusa			Apple				1		35		26		5		40
	//Botrytis cinerea				Grapes				10		35		20		4		10
	//Botrytis cinerea				Strawberry			5		35		25		8		18
	//Botrytis cinerea				Geranium			15		30		25		6		21.8
	//Botrytis cinerea				Grape flower		1		34		25		1		12
	//Botrytis squamosa				Onion				1		28		18		15		24
	//Bremia lactucae				Lettuce				1		25		15		4		10
	//Cercospora arachidicola		Peanut				13.3	35		24		24		48
	//Cercospora carotae			Carrot				11		32		24		28		96
	//Cercospora kikuchii			Soybean				15		35		25		24		75.8
	//Coccomyces hiemalis			Prunus sp			4		30		18		5		30
	//Colletotrichum acutatum		Strawberry fruit	7		35		27.5	6		36
	//Colletotrichum coccodes		Tomato				15		35		25		14		45.8
	//Colletotrichum orbiculare		Watermelon			7		30		24		2		16
	//Didymella arachidicola		Peanut				13.3	35		18.5	24		210
	//Diplocarpon earlianum			Strawberry			2.9		35		22.5	12		18
	//Elsinoe ampelina				Grape				0		35		21		3		12.8
	//Guignardia bidewelli			Grape				7		35		27		6		24
	//Gymnoporangium j.v.			Apple				1		35		21		2		24
	//Leptosphaeria					Maculans rape		2.6		35		18.5	7		18
	//Melampsora medusae			Poplar				12		28		20.5	5		12
	//Microcylus ulei				Rubber				16		35		22.5	8		27.8
	//Monilinia fructicola			Prunus fruit		10		35		20		10		16
	//Mycopshaerella pinodes		Pea					1.4		35		20		6		72
	//Mycosphaerella fragariae		Strawberry			10		30		25		30		93.8
	//Mycosphaerella graminicola	Wheat				1		26		22		15		48.8
	//Phaeoisarpis personata		Peanut				1		35		20		16		51.8
	//Phakopsora pachyrhizi			Soybean				10		28		23		8		12
	//Phytophthora cactorum			Apple fruit			1		35		25		2		5
	//Phytophthora cactorum			Strawberry fruit	6		35		20.5	1		3
	//Phytophthora infestans		Potato				1		28		15		6		12
	//Plasmopara viticola			Grape				1		30		20		2		14
	//Psuedoperonospora cubensis	cucumber			1		28		20		2		12
	//Puccinia arachidis			Groundnut			5		35		25		5		25
	//Puccinia menthae				peppermint			5		35		15		6		12
	//Puccinia psidii				Eucalyptus			1		30		21.5	6		24
	//Puccinia recondita			Wheat				2.6		30		25		5		25
	//Puccinia striiformis			Wheat				2.6		18		8.5		5		8
	//Pyrenopezzia brassicae		Oilseed rape		2.6		24		16		6		24
	//Pyrenophora teres				Barley				2.5		35		20.5	4.5		27
	//Rhizoctonia solani			Rye grass			18		35		27		12		39.8
	//Rhynchosporium secalis		Barley				2.6		30		22.5	9		33.5
	//Sclerotinia sclerotiorum		Beans				1		30		25		48		144
	//Septoria apiicola				Celery				10		30		25		12		39.8
	//Septoria glycines				Soybean				10		35		25		6		18
	//Uromyces phaseoli				Bean				1		29		17		4		15.8
	//Venturia inequalis			Apple				1		35		20		6		40.5
	//Venturia pirina				Pear				1		35		21		10		27.5
	//Wilsonomyces carpophilus		Almond				5		35		25		12		48




	//Albugo occidentalis ( Spinach), Alternaria brassicae ( Oilseed rape )|Alternaria cucumis ( Muskmelon )|Alternaria mali ( Apple )|
	//Alternaria pori ( Onion )|Alternaria sp. ( Mineola tangelo )|Ascochyta rabiei ( Chick pea )|Bipolaris oryzae ( Rice )|Botryosphaeria dothidea ( Apple )|
	//Botryosphaeria obtusa ( Apple )|Botrytis cinerea ( Grapes )|Botrytis cinerea ( Strawberry )|Botrytis cinerea ( Geranium )|Botrytis cinerea ( Grape flower )|
	//Botrytis squamosa ( Onion )|Bremia lactucae ( Lettuce )|Cercospora arachidicola ( Peanut )|Cercospora carotae ( Carrot )|Cercospora kikuchii ( Soybean )|
	//Coccomyces hiemalis ( Prunus sp )|Colletotrichum acutatum ( Strawberry fruit )|Colletotrichum coccodes ( Tomato )|Colletotrichum orbiculare ( Watermelon )|
	//Didymella arachidicola ( Peanut )|Diplocarpon earlianum ( Strawberry )|Elsinoe ampelina ( Grape )|Guignardia bidewelli ( Grape )|Gymnoporangium juniperi-virginianae ( Apple )|
	//Leptosphaeria maculans ( rape )|Melampsora medusae ( Poplar )|Microcylus ulei ( Rubber )|Monilinia fructicola ( Prunus fruit )|Mycopshaerella pinodes ( Pea )|
	//Mycosphaerella fragariae ( Strawberry )|Mycosphaerella graminicola ( Wheat )|Phaeoisarpis personata ( Peanut )|Phakopsora pachyrhizi ( Soybean )|Phytophthora cactorum ( Apple fruit )|
	//Phytophthora cactorum ( Strawberry fruit )|Phytophthora infestans ( Potato )|Plasmopara viticola ( Grape )|Psuedoperonospora cubensis ( cucumber )|Puccinia arachidis ( Groundnut )|
	//Puccinia menthae ( peppermint )|Puccinia psidii ( Eucalyptus )|Puccinia recondita ( Wheat )|Puccinia striiformis ( Wheat )|Pyrenopezzia brassicae ( Oilseed rape )|
	//Pyrenophora teres ( Barley )|Rhizoctonia solani ( Rye grass )|Rhynchosporium secalis ( Barley )|Sclerotinia sclerotiorum ( Beans )|Septoria apiicola ( Celery )|
	//Septoria glycines ( Soybean )|Uromyces phaseoli ( Bean )|Venturia inequalis ( Apple )|Venturia pirina ( Pear )|Wilsonomyces carpophilus ( Almond )
	//



	const double CMagareyInfection::LIBRARY[NB_PATHOGEN][NB_PARAM] =
	{
		//	  Tmin     Tmax    Topt    Wmin    Wmax	
		{ 6, 28, 16, 3, 12 },
		{ 2.6, 35, 18, 6, 22 },
		{ 12, 25, 19, 8, 24 },
		{ 1, 35, 23, 5, 40 },
		{ 1, 35, 23, 8, 24 },
		{ 9.4, 35, 25, 8, 16 },
		{ 1, 35, 25, 12, 48 },
		{ 8, 35, 27.5, 10, 24 },
		{ 8, 35, 28, 8, 19 },
		{ 1, 35, 26, 5, 40 },
		{ 10, 35, 20, 4, 10 },
		{ 5, 35, 25, 8, 18 },
		{ 15, 30, 25, 6, 21.8 },
		{ 1, 34, 25, 1, 12 },
		{ 1, 28, 18, 15, 24 },
		{ 1, 25, 15, 4, 10 },
		{ 13.3, 35, 24, 24, 48 },
		{ 11, 32, 24, 28, 96 },
		{ 15, 35, 25, 24, 75.8 },
		{ 4, 30, 18, 5, 30 },
		{ 7, 35, 27.5, 6, 36 },
		{ 15, 35, 25, 14, 45.8 },
		{ 7, 30, 24, 2, 16 },
		{ 13.3, 35, 18.5, 24, 210 },
		{ 2.9, 35, 22.5, 12, 18 },
		{ 0, 35, 21, 3, 12.8 },
		{ 7, 35, 27, 6, 24 },
		{ 1, 35, 21, 2, 24 },
		{ 2.6, 35, 18.5, 7, 18 },
		{ 12, 28, 20.5, 5, 12 },
		{ 16, 35, 22.5, 8, 27.8 },
		{ 10, 35, 20, 10, 16 },
		{ 1.4, 35, 20, 6, 72 },
		{ 10, 30, 25, 30, 93.8 },
		{ 1, 26, 22, 15, 48.8 },
		{ 1, 35, 20, 16, 51.8 },
		{ 10, 28, 23, 8, 12 },
		{ 1, 35, 25, 2, 5 },
		{ 6, 35, 20.5, 1, 3 },
		{ 1, 28, 15, 6, 12 },
		{ 1, 30, 20, 2, 14 },
		{ 1, 28, 20, 2, 12 },
		{ 5, 35, 25, 5, 25 },
		{ 5, 35, 15, 6, 12 },
		{ 1, 30, 21.5, 6, 24 },
		{ 2.6, 30, 25, 5, 25 },
		{ 2.6, 18, 8.5, 5, 8 },
		{ 2.6, 24, 16, 6, 24 },
		{ 2.5, 35, 20.5, 4.5, 27 },
		{ 18, 35, 27, 12, 39.8 },
		{ 2.6, 30, 22.5, 9, 33.5 },
		{ 1, 30, 25, 48, 144 },
		{ 10, 30, 25, 12, 39.8 },
		{ 10, 35, 25, 6, 18 },
		{ 1, 29, 17, 4, 15.8 },
		{ 1, 35, 20, 6, 40.5 },
		{ 1, 35, 21, 10, 27.5 },
		{ 5, 35, 25, 12, 48 }
	};

	void CMagareyInfection::Execute(const CWeatherStation& weather, CMagareyInfectionStatVector& output)
	{

		//CWeather weather(weatherIn);
		output.Init(weather.GetEntireTPeriod());


		CSWEB sweb;
		//sweb.m_loc = m_loc;
		sweb.m_LAI = 1;	//Leaf Area Index
		sweb.m_Zc = 0.5; //in meters

		CWetnessDurationStat wetD;
		sweb.Execute(weather, wetD);

		/*
		static const double INPUT_DATA[111][3] =
		{
		{6.1, 24, 0},
		{6.1, 24, 0},
		{7.8, 22, 8},
		{10.3, 7, 0},
		{9.7, 0, 0},
		{16.4, 0, 0},
		{20.3, 0, 0},
		{21.7, 0, 0},
		{18.6, 0, 2},
		{17.8, 12, 5},
		{18.3, 15, 29},
		{16.9, 24, 26},
		{16.1, 11, 33},
		{12.5, 18, 0},
		{11.7, 24, 0},
		{9.2, 10, 0},
		{8.3, 0, 0},
		{16.1, 0, 0},
		{18.1, 0, 12},
		{20.6, 7, 83},
		{17.2, 24, 0},
		{10.6, 22, 9},
		{9.7, 5, 0},
		{13.3, 7, 9},
		{14.2, 0, 0},
		{19.4, 14, 0},
		{15.0, 8, 25},
		{18.6, 18, 0},
		{21.9, 13, 0},
		{22.2, 5, 0},
		{23.3, 11, 0},
		{23.3, 7, 24},
		{22.8, 21, 0},
		{22.2, 24, 0},
		{20.0, 24, 0},
		{20.8, 19, 0},
		{21.7, 24, 0},
		{18.9, 24, 0},
		{18.9, 24, 0},
		{22.5, 20, 0},
		{22.5, 13, 5},
		{25.0, 5, 0},
		{25.0, 0, 0},
		{21.9, 9, 0},
		{21.9, 21, 0},
		{21.1, 13, 7},
		{19.2, 9, 0},
		{15.3, 5, 0},
		{16.9, 0, 0},
		{17.2, 24, 0},
		{18.6, 21, 0},
		{18.3, 12, 16},
		{18.9, 12, 4},
		{16.1, 1, 0},
		{13.6, 24, 0},
		{15.8, 24, 0},
		{19.7, 13, 0},
		{22.2, 12, 0},
		{25.3, 10, 0},
		{23.1, 11, 0},
		{16.1, 24, 0},
		{17.2, 11, 2},
		{18.6, 6, 0},
		{23.3, 22, 0},
		{24.7, 21, 0},
		{24.2, 24, 0},
		{25.8, 24, 26},
		{25.6, 23, 4},
		{19.4, 2, 0},
		{16.1, 0, 0},
		{17.8, 1, 13},
		{23.1, 12, 17},
		{22.2, 10, 4},
		{22.5, 10, 30},
		{22.5, 15, 33},
		{19.7, 12, 3},
		{18.1, 0, 25},
		{18.6, 7, 25},
		{18.6, 11, 0},
		{20.3, 2, 30},
		{21.9, 18, 30},
		{23.1, 11, 0},
		{23.6, 0, 54},
		{23.6, 15, 54},
		{26.4, 13, 0},
		{24.2, 0, 63},
		{23.9, 15, 63},
		{23.6, 12, 0},
		{22.5, 0, 0},
		{21.1, 7, 14},
		{23.9, 18, 115},
		{21.9, 24, 101},
		{23.3, 20, 72},
		{23.1, 24, 72},
		{21.4, 9, 0},
		{21.9, 0, 0},
		{21.4, 5, 0},
		{19.7, 22, 0},
		{21.7, 14, 0},
		{21.9, 14, 0},
		{23.3, 12, 0},
		{24.4, 19, 0},
		{25.0, 24, 0},
		{21.9, 11, 0},
		{19.7, 0, 16},
		{21.4, 22, 0},
		{22.8, 24, 0},
		{22.8, 14, 0},
		{21.7, 5, 0},
		{24.7, 11, 0},
		{25.6, 21, 0}
		};

		for(int d=0; d<111; d++)
		{
		CTRef TRef = CTRef(2004,APRIL,12-1)+d;
		CWeatherDay day = weather.GetDay(TRef);
		day.SetTMin(INPUT_DATA[d][0]);
		day.SetTMax(INPUT_DATA[d][0]);
		day.SetPpt(INPUT_DATA[d][2]);
		weather.SetData(TRef, day);
		wetD[TRef][0] = INPUT_DATA[d][1];
		}
		*/


		double Wmin = LIBRARY[m_pathogenType][L_WMIN];
		double WDContinuation = 24 - m_interruption;//h


		//assuming no infection the first of january of the first year
		for (size_t y = 0; y < weather.size(); y++)
		{
			CTRef beginning = m_beginning;
			beginning.m_year = weather[y].GetTRef().GetYear();

			for (CTRef d = beginning; d <= weather[y].GetEntireTPeriod().End(); d++)
			{
				bool bFirstDay = d == weather[0].GetTRef();
				const CWeatherDay& wDay = weather.GetDay(d);

				double T = wDay[H_TAIR][MEAN];   //is T the mean temperature or the mean temperature of wetness period????
				double F1 = GetF1(T);

				double WD = wetD[d][0];
				double infection = GetInfection(WD, F1);

				output[d][O_INFECTION] = infection;
				output[d][O_CUMUL_INFECTION] = bFirstDay ? 0 : output[d - 1][O_CUMUL_INFECTION] + infection;

				double prcp = wDay[H_PRCP][SUM];
				bool bPrcp = prcp >= m_prcpThreshold;
				if (bPrcp)
					output[d][O_RAIN_INFECTION] = infection;

				output[d][O_CUMUL_RAIN_INFECTION] = bFirstDay ? 0 : output[d - 1][O_CUMUL_RAIN_INFECTION] + output[d][O_RAIN_INFECTION];

				bool bWDContinuation = !bFirstDay && output[d - 1][O_INFECTION] > 0 && WD > WDContinuation;
				bool bRainInfection = (!bFirstDay && output[d - 1][O_INFECTION_EVENT]) || output[d][O_RAIN_INFECTION] > 0;
				bool bInfectionEvent = output[d][O_INFECTION] > 0 && bRainInfection;//ici j'aurais pensé que ce resait un ou, mais dans Excel c'est un ET

				//if(d==CTRef(2004, JUNE, 26-1) )
				//{
				//	int i;
				//	i=0;
				//}

				//if( (!bFirstDay && output[d-1][O_INFECTION_EVENT]>0) && output[d][O_RAIN_INFECTION]>0)
				//{
				//	if(bWetness||prcp)
				//	{
				//		output[d][O_INFECTION_EVENT] = bFirstDay?0:output[d-1][O_INFECTION_EVENT] + output[d][O_INFECTION];
				//	}
				//}

				if (bInfectionEvent && (bWDContinuation || bPrcp))
					output[d][O_INFECTION_EVENT] = bFirstDay ? 0 : output[d - 1][O_INFECTION_EVENT] + output[d][O_INFECTION];

			}
		}
	}

	double CMagareyInfection::GetF1(double T)
	{

		double F1 = 0;

		double Tmin = LIBRARY[m_pathogenType][L_TMIN];
		double Tmax = LIBRARY[m_pathogenType][L_TMAX];
		double Topt = LIBRARY[m_pathogenType][L_TOPT];

		if (T >= Tmin && T <= Tmax)
		{
			double alpha = log(2.0) / (log((Tmax - Tmin) / (Topt - Tmin)));
			double a = 2 * pow(T - Tmin, alpha);
			double b = pow(Topt - Tmin, alpha);
			double c = pow(T - Tmin, 2 * alpha);
			double d = pow(Topt - Tmin, 2 * alpha);
			F1 = (a*b - c) / d;//Wang and Engel  1998 Eqn 6
		}

		return F1;
	}

	double CMagareyInfection::GetF2(double F1)
	{
		double Wmin = LIBRARY[m_pathogenType][L_WMIN];
		return Wmin / F1;
	}



	//Magarey et al 2005 Eqn 1
	double CMagareyInfection::GetInfection(double WD, double F1)
	{
		double Wmin = LIBRARY[m_pathogenType][L_WMIN];
		double Wmax = LIBRARY[m_pathogenType][L_WMAX];
		double infection = 0;

		if (Wmax > 1)//if Wmax is define
		{

			if (WD < Wmax)
				infection = WD*F1 / Wmin;
			else
				infection = WD / Wmax;
		}
		else
		{
			infection = WD*F1 / Wmin;
		}

		return infection;
	}
}