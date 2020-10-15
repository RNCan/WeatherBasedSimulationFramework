//*********************************************************************
//13/10/2020	1.0.0	Rémi Saint-Amant    Creation from R code
//*********************************************************************

#include <string>
#include "Basic/WeatherDefine.h"
#include "Basic/GrowingSeason.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "FWI.h"
#include "FBP.h"
#include "FBP-Model.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CFBPModel::CreateObject);

	enum TOutput { O_CFB, O_CFC, O_FD, O_HFI, O_RAZ, O_ROS, O_SFC, O_TFC, NB_PRIMARY_OUTPUT, O_BE = NB_PRIMARY_OUTPUT, O_SF, O_ISI, O_FFMC, O_FMC, O_D0, O_RSO, O_CSI, O_FROS, O_BROS, O_HROSt, O_FROSt, O_BROSt, O_FCFB, O_BCFB, O_FFI, O_BFI, O_FTFC, O_BTFC, O_TI, O_FTI, O_BTI, O_LB, O_LBt, O_WSV, O_DH, O_DB, O_DF, O_TROS, O_TROSt, O_TCFB, O_TFI, O_TTFC, O_TTI, NB_OUTPUT_COL };

	//**************************************************************************************************
	//CFBPModel

	CFBPModel::CFBPModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 12;
		VERSION = "1.0.0 (2020)";

		m_bAutoSelect = true;
		m_firstDay = NOT_INIT;
		m_lastDay = NOT_INIT;
		m_FFMC = 85.0;
		m_DMC = 6.0;
		m_DC = 15.0;
		m_nbDaysStart = 3;
		m_TtypeStart = CGSInfo::TT_TNOON;
		m_thresholdStart = 12;
		m_nbDaysEnd = 3;
		m_TtypeEnd = CGSInfo::TT_TMAX;
		m_thresholdEnd = 5;
		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;
		size_t m_VanWagnerType = CFWI::VAN_WAGNER_1987;
		bool m_fbpMod = false;
		m_ignition.clear();
		m_fuel_type = FUEL_C2;
		
		m_method = CFWI::ALL_HOURS_CALCULATION;
	}

	CFBPModel::~CFBPModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CFBPModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		if (WBSF::Find(m_info.m_modelName, "fixed"))
		{
			m_bAutoSelect = false;

			//transfer your parameter here
			
			m_firstDay = CMonthDay(parameters[c++].GetString());
			m_lastDay = CMonthDay(parameters[c++].GetString());
			m_FFMC = parameters[c++].GetReal();
			m_DMC = parameters[c++].GetReal();
			m_DC = parameters[c++].GetReal();
			m_carryOverFraction = parameters[c++].GetReal();
			m_effectivenessOfWinterPrcp = parameters[c++].GetReal();
			m_VanWagnerType = parameters[c++].GetInt();
			m_fbpMod = parameters[c++].GetBool();


			if (!GetFileData(0).empty())
				msg = m_init_values.Load(GetFileData(0));

		}
		else //if (parameters.size() == 8)
		{
			m_bAutoSelect = true;
			
			m_nbDaysStart = parameters[c++].GetInt();
			m_TtypeStart = parameters[c++].GetInt();
			m_thresholdStart = parameters[c++].GetReal();
			m_nbDaysEnd = parameters[c++].GetInt();
			m_TtypeEnd = parameters[c++].GetInt();
			m_thresholdEnd = parameters[c++].GetReal();
			m_carryOverFraction = parameters[c++].GetReal();
			m_effectivenessOfWinterPrcp = parameters[c++].GetReal();
			m_VanWagnerType = parameters[c++].GetInt();
			m_fbpMod = parameters[c++].GetBool();
		}

		string str_igni = parameters[c++].GetString();
		if (str_igni.empty())
			m_ignition = CTRef::GetCurrentTRef(CTM::HOURLY);
		else 
			m_ignition.FromFormatedString(str_igni);

		m_fuel_type = parameters[c++].GetInt();


		return msg;
	}


	ERMsg CFBPModel::ExecuteFWIHourly(CModelStatVector& output)
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CFWI FWI;
		FWI.m_method = (CFWI::TMethod)m_method;
		FWI.m_bAutoSelect = m_bAutoSelect;
		FWI.m_nbDaysStart = m_nbDaysStart;
		FWI.m_TtypeStart = m_TtypeStart;
		FWI.m_thresholdStart = m_thresholdStart;
		FWI.m_nbDaysEnd = m_nbDaysEnd;
		FWI.m_TtypeEnd = m_TtypeEnd;
		FWI.m_thresholdEnd = m_thresholdEnd;
		//manual setting 
		FWI.m_firstDay = m_firstDay;
		FWI.m_lastDay = m_lastDay;
		FWI.m_FFMC = m_FFMC;
		FWI.m_DMC = m_DMC;
		FWI.m_DC = m_DC;
		FWI.m_init_values = m_init_values;

		//common setting
		FWI.m_carryOverFraction = m_carryOverFraction;
		FWI.m_effectivenessOfWinterPrcp = m_effectivenessOfWinterPrcp;
		FWI.m_VanWagnerType = CFWI::TVanWagner(m_VanWagnerType);
		FWI.m_fbpMod = m_fbpMod;

		msg = FWI.Execute(m_weather, output);


		return msg;
	}


	void ExecuteTest()
	{


		static const double NA = -999;

		enum { NB_TESTS = 20 };
		enum { I_FUELTYPE, I_LAT, I_LONG, I_ELV, I_FFMC, I_BUI, I_WS, I_WD, I_GS, I_DJ, I_D0, I_HR, I_PC, I_PDF, I_GFL, I_CC, I_THETA, I_ACCEL, I_ASPECT, I_BUIEFF, I_CBH, I_CFL, I_ISI, NB_INPUT_COL };

		static const double INPUT[NB_TESTS][NB_INPUT_COL] =
		{
			//FuelType LAT LONG  ELV  FFMC  BUI   WS    WD   GS  Dj   D0  hr          PC PDF  GFL cc theta Accel Aspect BUIEff CBH CFL ISI
			{  FUEL_C1, 55, 110,  NA, 90.0, 130, 20.0,   0, 15, 182,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT, 270, true, NA, NA, 0},
			{  FUEL_C2, 50,  90,  NA, 97.0, 119, 20.4,   0, 75, 121,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT, 315, true, NA, NA, 0},
			{  FUEL_C3, 55, 110,  NA, 95.0,  30, 50.0,   0,  0, 182,  NA, 0.08333333, NA, NA,  NA, NA, 0, AC_POINT, 180, true, NA, NA, 0},
			{  FUEL_C4, 55, 105, 200, 85.0,  82,  0.0,  NA, 75, 182,  NA, 0.50000000, NA, NA,  NA, NA, 0, AC_POINT, 315, true, NA, NA, 0},
			{  FUEL_C5, 55, 105,  NA, 88.0,  56,  3.4,   0, 23, 152, 145, 0.50000000, NA, NA,  NA, NA, 0, AC_POINT, 180, true, NA, NA, 0},
			{  FUEL_C6, 55, 105,  NA, 94.0,  56, 25.0,   0, 10, 152, 132, 1.00000000, NA, NA,  NA, NA, 0, AC_POINT, 180, true, NA, NA, 0},
			{  FUEL_C7, 50, 125,  NA, 88.8,  15, 22.1, 270, 15, 152,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT,  NA, true, NA, NA, 0},
			{  FUEL_D1, 45, 100,  NA, 98.0, 100, 50.0, 270, 35, 152,  NA, 1.00000000, NA, NA,  NA, NA, 0, AC_POINT,  NA, true, NA, NA, 0},
			{  FUEL_M1, 47,  85,  NA, 90.0,  40, 15.5, 180, 25, 182,  NA, 0.33333333, 55, NA,  NA, NA, 0, AC_POINT, 180, true, NA, NA, 0},
			{  FUEL_M2, 63, 120, 100, 97.0, 150, 41.0, 180, 50, 213,  NA, 0.33333333, 10, NA,  NA, NA, 0, AC_POINT, 200, true, NA, NA, 0},
			{  FUEL_M3, 56,  90,  10, 87.0,  25, 10.7, 180,  8, 130,  NA, 0.33333333, NA, 70,  NA, NA, 0, AC_POINT, 273, true, NA, NA, 0},
			{  FUEL_M4, 56,  90,  NA, 97.0,  80, 35.0, 180, 50, 258,  NA, 0.33333333, NA, 30,  NA, NA, 0, AC_POINT, 270, true, NA, NA, 0},
			{ FUEL_O1A, 56,  90,  NA, 95.0,  20, 35.0, 180, 50, 244,  NA, 0.08333333, NA, NA, 1.0, 90, 0, AC_POINT, 270, true, NA, NA, 0},
			{ FUEL_O1B, 50,  90,  NA, 85.0,  40,  0.5, 180,  0, 152,  NA, 0.16666667, NA, NA, 0.2, 55, 0, AC_POINT, 179, true, NA, NA, 0},
			{ FUEL_O1B, 50,  90,  NA, 95.0,  40, 35.0, 180, 10, 152,  NA, 0.33333333, NA, NA, 1.0, 45, 0, AC_POINT, 270, true, NA, NA, 0},
			{  FUEL_S1, 50,  90,  NA, 95.0, 130, 15.0, 180, 20, 152,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT, 225, true, NA, NA, 0},
			{  FUEL_S2, 50,  90,  NA, 87.0,  63,  0.0,   0,  0, 182,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT, 180, true, NA, NA, 0},
			{  FUEL_S3, 50,  90,  NA, 89.0,  20,  0.0,   0, 30, 213,  NA, 0.33333333, NA, NA,  NA, NA, 0, AC_POINT, 270, true, NA, NA, 0},
			{  FUEL_C6, 46,  77,  NA, 90.0,  80, 15.0,   0, 10, 171,  NA, 0.16666667, NA, NA,  NA, NA, 0, AC_POINT, 270, true, NA, NA, 0},
			{  FUEL_C6, 46,  77,  NA, 91.0, 100, 20.0,   0, 30, 213,  NA, 0.16666667, NA, NA,  NA, NA, 0, AC_POINT, 270, true, NA, NA, 0},
		};


		static const double OUTPUT[NB_TESTS][NB_OUTPUT_COL] =
		{
			{0.647104242578707,0.485328181934031,'I',3147.81699515641,174.695822642755,5.56890323027268,1.3988351592692,1.88416334120323,1.03318240193305,1.4370913349704,11.7962327820584,90,93.3349,0,1.04028335890129,436.555481410134,1.07914808539035,0.00246776958955448,5.01057143031719,1.03449194539257,0.00222035386336239,0.00889905401740365,0,455.025849519168,1.03560086005324,1.40550944978225,1.3988351592692,2.60935011273251,28.9358684261342,0,2.58137463953659,2.42282784631943,20.0860090757352,67.8078781434913,0.0300479668429307,13.9997412968049,0.62586595238303,0.639557700677759,0,262.644989754866,1.3988351592692,0},
			{0.999999999999025,0.79999999999922,'C',164491.280970482,141.486358211905,121.10375472564,3.72755796997276,4.52755796997198,1.13744995825129,10,136.138306549571,97,107.3872,0,0.857473661399939,958.88483417792,6.49285176413901,0.00145635968667872,107.329770587934,6.40471805536654,0.00129071762819298,0.726413370613696,0,8392.70304275241,1.62859954716789,4.30868866646372,3.72755796997276,0.0653736087515112,1.57919743931096,0,9.3260415826994,8.37906215212931,127.692365189986,1434.6198577286,0.01725233483667,85.608453787329,0.351265249970278,0.386191487308083,0,392.808474630355,3.72755796997276,0},
			{0.999999224457519,1.14999910812615,'C',42314.892843365,180,80.5171364879306,0.601797452999195,1.75179656112534,0.780774854321134,1,86.7070036615329,95,93.3349,0,19.344514414696,3492.44385128107,6.63695969341862,0.00242927349621415,33.7582809485455,5.40319591369762,0.00101851730902459,0,0,1198.23163174729,0.438579180798037,0.601797452999195,0.601797452999195,2.52794202731508,0,0,6.06599779724986,3.12401215919929,50,92.0027056367927,0.00277580331507386,14.72553190441,0.692729394338597,1.13355461052667,0,125.064835539193,0.601797452999195,0},
			{0.968401704144199,1.16208204497304,'C',18512.2516426181,135,16.6568192196659,2.54255715784298,3.70463920281602,1.03353504011969,10,11.11317685096,85,94.1476,0,1.63659274811217,1248.33918185595,1.99642689677318,0.142762060308646,15.9947298059639,1.97697660074723,0.137087432544359,0.0794296489121025,0,1579.89602444644,108.894209491848,2.6378727365375,2.54255715784298,0.962012835660672,17.611472073755,0,4.20741207883134,4.07992113624689,32.997385490689,350.924436231945,3.00769882103142,43.374874566639,0.242591108742525,0.248198895611271,0,185.040527988711,2.54255715784298,0},
			{0,0,'S',17.8770298180006,0,0.0489188298484945,1.21814237130959,1.21814237130959,1,1.83243094803766,3.21853883750534,88,85.9261,145,29.081967831855,10627.7931771135,0.0488816552055113,0.0488444839817769,0.0473658704640836,0.0473298760063493,0.047293884754217,0,0,17.8634446156738,17.8498606628865,1.21814237130959,1.21814237130959,0,0,0,1.00000003497476,1.00000003386447,0.00422950231922226,1.05568776098454,1.05408334767751,1.05488551860789,0.0489188298484945,0.0473658704640836,0,17.8770298180006,1.21814237130959,0},
			{0.911060651320368,1.63990917237666,'C',36704.8748953921,180,42.8087391408051,1.21814237130959,2.85805154368625,0.980903849652367,1.24971713012428,23.9629618259706,94,92.56,132,7.74038262323916,2828.66641305483,7.30888261647147,0.152347428793258,42.7292835512454,7.30426113878333,0.152064662820336,0,0,2670.9778806156,55.6742574519414,1.21814237130959,1.21814237130959,1.90268954498956,0,0,2.9389640540115,2.93536522033551,22.9392246876276,2160.886408237,7.69014679751236,369.3878601564,1.63447272461056,1.63566381523603,0,597.306144179388,1.21814237130959,0},
			{0,0,'S',1490.58859272483,99.8209732183045,2.35774365441997,2.10736592720825,2.10736592720825,0.628091204735719,1.4370913349704,11.1748495362977,88.8,89.8384,0,7.35869660704848,4652.23994950707,0.416124325018051,0.0344824100427163,2.12135900128936,0.400594692514259,0.0310252434751065,0,0,263.077867207671,21.8001168036131,2.10736592720825,2.10736592720825,0,0,0,2.87441267025055,2.68648622283963,22.42868049301,28.7082730300327,0.419863474294186,5.42123318122554,0.509615521460805,0.514783588726026,0,322.183915770889,2.10736592720825,0},
			{0,0,'S',11792.2542974621,107.495277144699,31.2118068421794,1.25937964833724,1.25937964833724,1.11845195691392,2.72471437989957,134.43168838928,98,0,0,0,0,2.48722206694109,0.0704086529724865,31.1803520380296,2.48682313581334,0.0703376961579418,0,0,939.707055600269,26.6013673861166,1.25937964833724,1.25937964833724,0,0,0,6.28858514704807,6.2832553879966,52.4250939131341,1601.5749145479,3.61288703784535,127.735361883608,1.7043507413947,1.70541793132556,0,643.927391202294,1.25937964833724,0},
			{0.780181093733677,0.343279681242818,'I',6817.05346251822,0,13.3071942878299,1.36433133353805,1.70761101478087,0.945741609003176,1.95302280490521,14.9237132165919,90,108.57,0,6.72044983156286,2750.67608420152,2.16292000908921,0.403106683824305,11.3295628539028,2.05001512383686,0.343199506404784,0,0,885.281862101044,164.991323850035,1.36433133353805,1.36433133353805,7.37771485440348,0,0,3.169396212999,2.84699420618432,24.7530305914556,147.286001041493,4.46164459384786,26.6505013086627,13.3071942878299,11.3295628539028,0.780181093733677,6817.05346251822,1.70761101478087,7.37771485440348},
			{0.935365378193126,0.0748292302554501,'C',9474.79871026767,7.66008226939565,18.0574768272737,1.67417826783847,1.74900749809392,1.16039720840319,4.65446777462586,129.899857702029,97,118.7048,0,6.14875912955223,3088.23567266092,1.23613346558213,0.0197755457326646,15.8921785498294,1.21352117324376,0.0174042313170324,0,0,620.85233526764,9.93233667008187,1.67417826783847,1.67417826783847,3.92540105053804,0,0,7.31201479303579,6.55513193009228,65.6158240646865,211.294130506199,0.23139759673167,16.1343456057606,0.209692582226352,0.229766101807125,0,105.318829227087,1.67417826783847,0},
			{0.317671551170782,0.177896068655638,'I',4010.23969340226,13.3066658976746,9.36613066549974,1.24931716614509,1.42721323480073,0.8,1.18595690083222,4.8216541834098,87,112.7252,0,7.7041996196507,2887.49665087143,3.74830918993646,2.2796315160426,4.30576074089343,2.13392052581257,1.04798323192685,0,0,1404.84810450211,854.394835643212,1.24931716614509,1.24931716614509,56.1712153898108,0,0,1.55346872301905,1.25443846386492,10.8605798502626,47.4480906465141,11.5484362408335,23.5151139680378,5.05388789893233,3.07818696953931,0,1894.17267237273,1.24931716614509,0},
			{0.999449204588101,0.239867809101144,'C',35166.4981587813,43.9799640656668,36.0984970576477,3.00740479457743,3.24727260367857,1.08727963802049,4.65446777462586,111.007162191726,97,120,0,3.47177044447195,3132.30572411315,3.0637650944687,0.276073804186881,31.9911890811774,2.99890505868044,0.244661966286249,0,0,2764.19455036924,249.079704710657,3.00740479457743,3.00740479457743,0.930482780204959,0,0,5.93625322768788,5.37460346638114,48.6393034284345,427.592823202644,3.27014105756387,40.0832328333907,36.0917217720138,31.9862906319271,0.999448345605857,35159.8955668003,3.24727239752283,0.930666848775629},
			{0,0,'S',40407.9783111616,40.7132850701426,134.693261037205,1,1,1,4.65446777462586,80.3963325527967,95,0,0,0,0,10.4127273318184,0.910809713923511,58.9007070817609,8.69452713250832,0.398292652163292,0,0,3123.81819954553,273.242914177053,1,1,0,0,0,6.51145787409415,3.41013368698389,46.1751710701503,161.286240911633,1.09063418476404,23.8079925892894,0.806956698281234,1.329068839687,0,242.08700948437,1,0},
			{0,0,'S',24.1425769997816,0,0.402376283329693,0.2,0.2,1,1,2.1610371166982,85,0,0,0,0,0.386436611018451,0.37049693870721,0.274969157194465,0.264076571217207,0.25318398523995,0,0,23.1861966611071,22.2298163224326,0.2,0.2,0,0,0,1,1,0.5,1.63272669182737,1.5033695228303,1.56804810732884,0.402376283329693,0.274969157194465,0,24.1425769997816,0.2,0},
			{0,0,'S',3989.59082562123,5.02801482782898,13.2986360854041,1,1,1,1.24971713012428,50.848550472649,95,0,0,0,0,1.16904248552329,0.112867550932242,11.9653302053245,1.14676519200472,0.101551580755924,0,0,350.712745656986,33.8602652796727,1,1,0,0,0,5.73610617339245,5.26126964360756,35.1352015158902,161.926371830592,1.37429379242294,15.5191309973463,0.293296832649672,0.313726324450666,0,87.9890497949017,1,0},
			{0,0,'S',105514.822621456,16.8888743482949,45.1105066638324,7.79676623953422,7.79676623953422,1.30719300209754,1.66884089459573,26.9153417497712,95,0,0,0,0,8.44950298875355,3.63896758961431,40.5877794155644,8.13525848818214,3.27412890589569,0,0,19763.6398930671,8511.65389483921,7.79676623953422,7.79676623953422,0,0,0,2.88475395051834,2.69579069830277,22.5105703025964,549.272920065182,44.3086655812834,110.094152713743,2.06197297070443,2.14033774111405,0,4823.01637344611,7.79676623953422,0},
			{0,0,'S',3478.57690417422,180,1.01230237190568,11.4543407869519,11.4543407869519,1,1,2.78947231102028,87,0,0,0,0,1.01230237190568,1.01230237190568,0.910810106367136,0.910810106367136,0.910810106367136,0,0,3478.57690417422,3478.57690417422,11.4543407869519,11.4543407869519,0,0,0,1,1,0,12.3259595364143,12.3259595364143,12.3259595364143,1.01230237190568,0.910810106367136,0,3478.57690417422,11.4543407869519,0},
			{0,0,'S',4301.88861667569,90,1.3990952186766,10.2492157294455,10.2492157294455,0.774760105787914,2.30040379466567,5.08253186841579,89,0,0,0,0,0.692760746362693,0.253073199590338,1.25882354947136,0.633556930017629,0.227700373163823,0,0,2130.07630150888,778.140545382722,10.2492157294455,10.2492157294455,0,0,0,1.19245239207156,1.1731573377264,6.22370344955802,17.0356126110166,3.08146074184249,8.57390253887275,0.511990815222564,0.486592891481046,0,1574.25129501323,10.2492157294455,0},
			{0,0,'S',2687.57483815575,173.255847737316,4.39541966501334,2.03816324187677,2.03816324187677,1.04132064370764,1.24971713012428,9.17757320813933,90,96.8125,0,4.89621616570476,2993.78634396668,1.1292647826069,0.101364361351529,3.00366818541939,0.91607574381354,0.06926867751683,0,0,690.487791076604,61.9791346029003,2.03816324187677,2.03816324187677,0,0,0,1.99102287418548,1.67722859364437,15.1045164885424,17.8353429517863,0.411307289309274,5.43952395941696,0.335628389800598,0.345060915556458,0,205.21963410656,2.03816324187677,0},
			{0.705935749718007,1.27068434949241,'I',20766.5248736041,160.575377237914,17.6370032495574,2.65411787451452,3.92480222400693,1.07077501696957,2.30040379466567,14.3992434735666,91,120,0,4.95727334952015,3947.15634174478,3.25319914831257,0.0659152785149905,10.2657633114515,2.57393010158467,0.0383665319027639,0,0,2590.31220266754,52.4840756730718,2.65411787451452,2.65411787451452,3.78254820395176,0,0,2.72084765195744,2.00163357913456,21.207112553966,58.699150425272,0.219378019844285,14.7176109202241,0.659298880421299,0.757480325647467,0,524.957082952075,2.65411787451452,0},
		};

		for (size_t no = 0; no < NB_TESTS; no++)
		{
			CFBPInput in;
			for (size_t i = 0; i < NB_INPUT_COL; i++)
			{
				switch (i)
				{
				case I_FUELTYPE:in.fuel_type = INPUT[no][i]; break;
				case I_LAT:		in.LAT = INPUT[no][i]; break;
				case I_LONG:	in.LONG = INPUT[no][i]; break;
				case I_ELV:		in.ELV = INPUT[no][i]; break;
				case I_FFMC:	in.FFMC = INPUT[no][i]; break;
				case I_BUI:		in.BUI = INPUT[no][i]; break;
				case I_WS:		in.WS = INPUT[no][i]; break;
				case I_WD:		in.WD = INPUT[no][i]; break;
				case I_GS:		in.GS = INPUT[no][i]; break;
				case I_DJ:		in.DJ = INPUT[no][i]; break;
				case I_D0:		in.D0 = INPUT[no][i]; break;
				case I_HR:		in.hr = INPUT[no][i]; break;
				case I_PC:		in.PC = INPUT[no][i]; break;
				case I_PDF:		in.PDF = INPUT[no][i]; break;
				case I_GFL:		in.GFL = INPUT[no][i]; break;
				case I_CC:		in.cc = INPUT[no][i]; break;
				case I_THETA:	in.theta = INPUT[no][i]; break;
				case I_ACCEL:	in.accel = TAcceleration((size_t)INPUT[no][i]); break;
				case I_ASPECT:	in.ASPECT = INPUT[no][i]; break;
				case I_BUIEFF:	in.BUIeff = INPUT[no][i] != 0; break;
				case I_CBH:		in.CBH = INPUT[no][i]; break;
				case I_CFL:		in.CFL = INPUT[no][i]; break;
				case I_ISI:		in.ISI = INPUT[no][i]; break;
				default: ASSERT(false);
				}
			}

			CFBPOutput out = { 0 };
			CFBP::Execute(in, out);

			for (size_t i = 0; i < NB_OUTPUT_COL; i++)
			{
				switch (i)
				{
				case O_CFB: assert(fabs(out.CFB - OUTPUT[no][i]) < 0.01); break;
				case O_CFC:	assert(fabs(out.CFC - OUTPUT[no][i]) < 0.01); break;
				case O_FD:	assert(fabs(out.FD - OUTPUT[no][i]) < 0.01); break;
				case O_HFI: assert(fabs(out.HFI - OUTPUT[no][i]) < 0.01); break;
				case O_RAZ: assert(fabs(out.RAZ - OUTPUT[no][i]) < 0.01); break;
				case O_ROS: assert(fabs(out.ROS - OUTPUT[no][i]) < 0.01); break;
				case O_SFC: assert(fabs(out.SFC - OUTPUT[no][i]) < 0.01); break;
				case O_TFC: assert(fabs(out.TFC - OUTPUT[no][i]) < 0.01); break;
				case O_BE:	assert(fabs(out.BE - OUTPUT[no][i]) < 0.01); break;
				case O_SF:	assert(fabs(out.SF - OUTPUT[no][i]) < 0.01); break;
				case O_ISI: assert(fabs(out.ISI - OUTPUT[no][i]) < 0.01); break;
				case O_FFMC:assert(fabs(out.FFMC - OUTPUT[no][i]) < 0.01); break;
				case O_FMC: assert(fabs(out.FMC - OUTPUT[no][i]) < 0.01); break;
				case O_D0:	assert(fabs(out.D0 - OUTPUT[no][i]) < 0.01); break;
				case O_RSO: assert(fabs(out.RSO - OUTPUT[no][i]) < 0.01); break;
				case O_CSI: assert(fabs(out.CSI - OUTPUT[no][i]) < 0.01); break;
				case O_FROS:assert(fabs(out.FROS - OUTPUT[no][i]) < 0.01); break;
				case O_BROS:assert(fabs(out.BROS - OUTPUT[no][i]) < 0.01); break;
				case O_HROSt: assert(fabs(out.HROSt - OUTPUT[no][i]) < 0.01); break;
				case O_FROSt: assert(fabs(out.FROSt - OUTPUT[no][i]) < 0.01); break;
				case O_BROSt: assert(fabs(out.BROSt - OUTPUT[no][i]) < 0.01); break;
				case O_FCFB:assert(fabs(out.FCFB - OUTPUT[no][i]) < 0.01); break;
				case O_BCFB:assert(fabs(out.BCFB - OUTPUT[no][i]) < 0.01); break;
				case O_FFI:	assert(fabs(out.FFI - OUTPUT[no][i]) < 0.01); break;
				case O_BFI:	assert(fabs(out.BFI - OUTPUT[no][i]) < 0.01); break;
				case O_FTFC:assert(fabs(out.FTFC - OUTPUT[no][i]) < 0.01); break;
				case O_BTFC:assert(fabs(out.BTFC - OUTPUT[no][i]) < 0.01); break;
				case O_TI:	assert(fabs(out.TI - OUTPUT[no][i]) < 0.01); break;
				case O_FTI:	assert(fabs(out.FTI - OUTPUT[no][i]) < 0.01); break;
				case O_BTI:	assert(fabs(out.BTI - OUTPUT[no][i]) < 0.01); break;
				case O_LB:	assert(fabs(out.LB - OUTPUT[no][i]) < 0.01); break;
				case O_LBt:	assert(fabs(out.LBt - OUTPUT[no][i]) < 0.01); break;
				case O_WSV:	assert(fabs(out.WSV - OUTPUT[no][i]) < 0.01); break;
				case O_DH:	assert(fabs(out.DH - OUTPUT[no][i]) < 0.01); break;
				case O_DB:	assert(fabs(out.DB - OUTPUT[no][i]) < 0.01); break;
				case O_DF:	assert(fabs(out.DF - OUTPUT[no][i]) < 0.01); break;
				case O_TROS:assert(fabs(out.TROS - OUTPUT[no][i]) < 0.01); break;
				case O_TROSt:assert(fabs(out.TROSt - OUTPUT[no][i]) < 0.01); break;
				case O_TCFB:assert(fabs(out.TCFB - OUTPUT[no][i]) < 0.01); break;
				case O_TFI:	assert(fabs(out.TFI - OUTPUT[no][i]) < 0.01); break;
				case O_TTFC:assert(fabs(out.TTFC - OUTPUT[no][i]) < 0.01); break;
				case O_TTI:	assert(fabs(out.TTI - OUTPUT[no][i]) < 0.01); break;
				default: ASSERT(false);
				}
			}

		}



	}

	//This method is call to compute solution
	ERMsg CFBPModel::OnExecuteHourly()
	{
		ERMsg msg;

		//Init class member

		CModelStatVector FWI;
		msg = ExecuteFWIHourly(FWI);

//		CJDayRef TRef(m_weather.GetFirstYear(), m_DJ);


		CFBPInput input;

		input.LAT = m_weather.m_lat;   //Latitude [decimal degrees]
		input.LONG = m_weather.m_lon;//Longitude [decimal degrees]
		input.ELV = m_weather.m_elev;//Elevation [meters above sea level]
		input.GS = m_weather.GetSlope();//GS	Ground Slope [percent]
		input.ASPECT = m_weather.GetAspect();//Aspect	Aspect of the slope [decimal degrees]
		input.accel = AC_POINT;//Acceleration: 1 = point, 0 = line
		input.D0 = -999;//Julian day of minimum Foliar Moisture Content
		input.BUIeff = true;//Buildup Index effect: 1=yes, 0=no
		input.theta = 0;//Elliptical direction of calculation [degrees]


		input.fuel_type = m_weather.GetSSI("FuelType").empty() ? m_fuel_type : ToSizeT(m_weather.GetSSI("FuelType"));
		input.PC = m_weather.GetSSI("PC").empty() ? 50 : ToDouble(m_weather.GetSSI("PC"));     //Percent Conifer for M1/M2 [percent]
		input.PDF = m_weather.GetSSI("PDF").empty() ? 35 : ToDouble(m_weather.GetSSI("PDF"));	//Percent Dead Fir for M3/M4 [percent]
		input.cc = m_weather.GetSSI("cc").empty() ? 80 : ToDouble(m_weather.GetSSI("cc"));		//Percent Cured for O1A/O1B [percent]
		input.GFL = m_weather.GetSSI("GFL").empty() ? 0.35 : ToDouble(m_weather.GetSSI("GFL"));	//Grass Fuel Load [kg/m^2]
		input.CBH = m_weather.GetSSI("CBH").empty() ? 3 : ToDouble(m_weather.GetSSI("CBH"));	//Crown to Base Height [m]
		input.CFL = m_weather.GetSSI("CFL").empty() ? 1 : ToDouble(m_weather.GetSSI("CFL"));	//Crown Fuel Load [kg/m^2]
		input.FMC = m_weather.GetSSI("FMC").empty() ? 0 : ToDouble(m_weather.GetSSI("FMC"));	//Foliar Moisture Content if known [percent]
		input.SD = m_weather.GetSSI("SD").empty() ? 0 : ToDouble(m_weather.GetSSI("SD"));		//Fuel Type Stand Density [stems/ha]
		input.SH = m_weather.GetSSI("SH").empty() ? 0 : ToDouble(m_weather.GetSSI("SH"));		//Fuel Type Stand Height [m]




		//CTRef begin = TRef.as(CTM::HOURLY) + m_h;
		CTRef end = m_ignition + 96;
		CTPeriod p(m_ignition, end);

		m_output.Init(p, NB_PRIMARY_OUTPUT, -999);



		for (CTRef h = m_ignition; h <= end; h++)
		{
			input.FFMC = FWI[h][CFWIStat::FFMC];//Fine fuel moisture code [FWI System component]
			input.ISI = FWI[h][CFWIStat::ISI];//Initial spread index
			input.BUI = FWI[h][CFWIStat::BUI];//Buildup index [FWI System component]

			const CHourlyData& weather = m_weather.GetHour(h);
			input.DJ = h.GetJDay() + 1;//Julian day
			input.hr = h- m_ignition;//Hours since ignition
			input.WS = weather[H_WNDS];//Wind speed [km/h]
			input.WD = weather[H_WNDD];//Wind direction [decimal degrees]

			CFBPOutput out;
			CFBP::Execute(input, out);
			toOutput(h, out, m_output);
		}

		//ExecuteTest();

		return msg;
	}

	void CFBPModel::toOutput(CTRef TRef, const CFBPOutput& in, CModelStatVector& out)
	{
		//CModelStatVector out(NB_OUTPUT_COL);

		for (size_t i = 0; i < NB_PRIMARY_OUTPUT; i++)
		{
			switch (i)
			{
			case O_CFB: out[TRef][i] = in.CFB; break;
			case O_CFC:	out[TRef][i] = in.CFC; break;
			case O_FD:	out[TRef][i] = in.FD; break;
			case O_HFI: out[TRef][i] = in.HFI; break;
			case O_RAZ: out[TRef][i] = in.RAZ; break;
			case O_ROS: out[TRef][i] = in.ROS; break;
			case O_SFC: out[TRef][i] = in.SFC; break;
			case O_TFC: out[TRef][i] = in.TFC; break;
			case O_BE:	out[TRef][i] = in.BE; break;
			case O_SF:	out[TRef][i] = in.SF; break;
			case O_ISI: out[TRef][i] = in.ISI; break;
			case O_FFMC:out[TRef][i] = in.FFMC; break;
			case O_FMC: out[TRef][i] = in.FMC; break;
			case O_D0:	out[TRef][i] = in.D0; break;
			case O_RSO: out[TRef][i] = in.RSO; break;
			case O_CSI: out[TRef][i] = in.CSI; break;
			case O_FROS:out[TRef][i] = in.FROS; break;
			case O_BROS:out[TRef][i] = in.BROS; break;
			case O_HROSt: out[TRef][i] = in.HROSt; break;
			case O_FROSt: out[TRef][i] = in.FROSt; break;
			case O_BROSt: out[TRef][i] = in.BROSt; break;
			case O_FCFB:out[TRef][i] = in.FCFB; break;
			case O_BCFB:out[TRef][i] = in.BCFB; break;
			case O_FFI:	out[TRef][i] = in.FFI; break;
			case O_BFI:	out[TRef][i] = in.BFI; break;
			case O_FTFC:out[TRef][i] = in.FTFC; break;
			case O_BTFC:out[TRef][i] = in.BTFC; break;
			case O_TI:	out[TRef][i] = in.TI; break;
			case O_FTI:	out[TRef][i] = in.FTI; break;
			case O_BTI:	out[TRef][i] = in.BTI; break;
			case O_LB:	out[TRef][i] = in.LB; break;
			case O_LBt:	out[TRef][i] = in.LBt; break;
			case O_WSV:	out[TRef][i] = in.WSV; break;
			case O_DH:	out[TRef][i] = in.DH; break;
			case O_DB:	out[TRef][i] = in.DB; break;
			case O_DF:	out[TRef][i] = in.DF; break;
			case O_TROS:out[TRef][i] = in.TROS; break;
			case O_TROSt:out[TRef][i] = in.TROSt; break;
			case O_TCFB:out[TRef][i] = in.TCFB; break;
			case O_TFI:	out[TRef][i] = in.TFI; break;
			case O_TTFC:out[TRef][i] = in.TTFC; break;
			case O_TTI:	out[TRef][i] = in.TTI; break;
			default: ASSERT(false);
			}
		}

	}
}