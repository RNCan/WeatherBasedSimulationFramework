#include "pch.h"
#include "CppUnitTest.h"
#include "Basic/WeatherDefine.h"
#include "Basic/WeatherStation.h"
#include "Basic/GrowingSeason.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace WBSF;
using namespace WBSF::HOURLY_DATA;


namespace UnitTestModels
{

	int YEAR = 2022;
	CWeatherStation WEATHER_N_D;
	CWeatherStation WEATHER_N_H;
	CWeatherStation WEATHER_S_D;
	CWeatherStation WEATHER_S_H;

	TEST_MODULE_INITIALIZE(ModuleInitialize)
	{
		Logger::WriteMessage("In Module Initialize");
		
		((CLocation&)WEATHER_N_D) = CLocation("Name", "ID", 45, -70, 100);
		((CLocation&)WEATHER_S_D) = CLocation("Name", "ID", -45, -70, 100);
		
		WEATHER_N_D.CreateYear(YEAR);
		WEATHER_S_D.CreateYears(YEAR-1, 2);
		
		
		for (CTRef TRef = CTRef(YEAR, JANUARY, DAY_01); TRef <= CTRef(YEAR, DECEMBER, DAY_31); TRef++)
		{
			size_t d = TRef.GetJDay();
			if (TRef.GetYear() == YEAR)
			{
				double Tmin = 20.0 * cos(2.0 * PI * (d + 150.0) / 365.0) + 2.0 * sin(2.0 * PI * (d % 7) / 7.0);
				double Tmax = Tmin + 10.0;
				WEATHER_N_D.GetDay(TRef).SetStat(H_TMIN, Tmin);
				WEATHER_N_D.GetDay(TRef).SetStat(H_TMAX, Tmax);
			}
		}
		ASSERT(WEATHER_N_D.IsDaily());
		ASSERT(fabs(WEATHER_N_D[2022][JANUARY][DAY_01][H_TMIN][MEAN] - -16.95) < 0.01);

		size_t d = 0;
		for (CTRef TRef = CTRef(YEAR-1, JANUARY, DAY_01); TRef <= CTRef(YEAR, DECEMBER, DAY_31); TRef++, d++)
		{
			double Tmin = 20.0 * cos(2.0 * PI * (d + 150.0-184.0) / 365.0) + 2 * sin(2.0 * PI * (d % 7) / 7.0);
			double Tmax = Tmin + 10.0;
			WEATHER_S_D.GetDay(TRef).SetStat(H_TMIN, Tmin);
			WEATHER_S_D.GetDay(TRef).SetStat(H_TMAX, Tmax);
		}
		
		ASSERT(fabs(WEATHER_S_D[2022][JANUARY][DAY_01][H_TMIN][MEAN] - 18.23) < 0.01);

		WEATHER_N_H = WEATHER_N_D;
		WEATHER_N_H.ComputeHourlyVariables();
		ASSERT(WEATHER_N_H.IsHourly());

		WEATHER_S_H = WEATHER_S_D;
		WEATHER_S_H.ComputeHourlyVariables();
		ASSERT(WEATHER_S_H.IsHourly());
	}

	TEST_MODULE_CLEANUP(ModuleCleanup)
	{
		Logger::WriteMessage("In Module Cleanup");

		WEATHER_N_D.clear();
	}


	TEST_CLASS(GrowingSeason)
	{
	public:
		
		
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In Class Initialize");
		}

		TEST_CLASS_CLEANUP(ClassCleanup)
		{
			Logger::WriteMessage("In Class Cleanup");
		}

		GrowingSeason()
		{
			Logger::WriteMessage("In UnitTestGrowingSeason");
		}

		~GrowingSeason()
		{
			Logger::WriteMessage("In ~UnitTestGrowingSeason");
		}

		
		
		TEST_METHOD(Test_1_1_0_GetFirst_greater_D_N)
		{
			CGSInfo info;

			info.m_d = CGSInfo::GET_FIRST;
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '>';
			info.m_threshold = 0;
			info.m_nbDays = 3;

			//CTRef a = info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0);
			//CTRef b = info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0);

			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, MAY, DAY_03));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, -2) == CTRef(2022, MAY, DAY_01));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, JULY, DAY_03));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, -2) == CTRef(2022, JULY, DAY_01));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, -3) == CTRef(2022, JUNE, DAY_30));
		}


		TEST_METHOD(Test_1_1_1_GetLast_greater_D_N)
		{
			CGSInfo info;

			info.m_d = CGSInfo::GET_LAST;
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '>';
			info.m_threshold = 0;
			info.m_nbDays = 3;


			//CTRef a = info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0);
			//CTRef b = info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0);


			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, JUNE, DAY_30));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 1) == CTRef(2022, JULY, DAY_01));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, NOVEMBER, DAY_01));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 1) == CTRef(2022, NOVEMBER, DAY_02));
			
		}

		
		TEST_METHOD(Test_1_1_2_GetFirst_smaller_D_N)
		{
			CGSInfo info;
			
			info.m_d = CGSInfo::GET_FIRST;
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = 0;
			info.m_nbDays = 3;
			
			//CTRef a = info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0);
			//CTRef b = info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0);
			
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, JANUARY, DAY_03));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, -2) == CTRef(2022, JANUARY, DAY_01));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, -3) == CTRef(2021, DECEMBER, DAY_31));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, NOVEMBER, DAY_04));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, -2) == CTRef(2022, NOVEMBER, DAY_02));

		}

		TEST_METHOD(Test_1_1_3_GetLast_smaller_D_N)
		{
			
			CGSInfo info;

			info.m_d = CGSInfo::GET_LAST;
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = 0;
			info.m_nbDays = 3;



			//CTRef a = info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0 );
			//CTRef b = info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0 );

			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, MAY, DAY_06));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 1) == CTRef(2022, MAY, DAY_07));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, DECEMBER, DAY_31));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 1) == CTRef(2023, JANUARY, DAY_01));

			
		}

		

		TEST_METHOD(Test_1_1_4_GSInfo_custom_D_N)
		{
			CGSInfo info;

			static const int NB_DAYS = 15;
			info.m_TT = CGSInfo::TT_TMAX;
			info.m_op = '>';
			info.m_threshold = 10;
			info.m_nbDays = NB_DAYS;

			//CTRef a = info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0);
			//CTRef b = info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0);

			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, -(NB_DAYS-1)) == CTRef(2022, MAY, DAY_07));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, MAY, DAY_21));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, JUNE, DAY_30));
			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, -(NB_DAYS - 1)) == CTRef(2022, JULY, DAY_01));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, NOVEMBER, DAY_01));
			
		}

		TEST_METHOD(Test_1_1_5_GetEvent_D_N)
		{
			CGSInfo info;

			info.m_d = CGSInfo::GET_FIRST;
			info.m_op = '>';

			Assert::IsTrue(info.GetEvent(WEATHER_N_D[YEAR], JANUARY, DECEMBER, 0) == info.GetFirst(WEATHER_N_D[YEAR], JANUARY, DECEMBER, 0));
			Assert::IsTrue(info.GetEvent(WEATHER_N_D[YEAR], JANUARY, DECEMBER, 0) != info.GetLast(WEATHER_N_D[YEAR], JANUARY, DECEMBER, 0));

			info.m_d = CGSInfo::GET_LAST;
			info.m_op = '>';

			Assert::IsTrue(info.GetEvent(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0));
			Assert::IsTrue(info.GetEvent(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) != info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0));
		}

		TEST_METHOD(Test_1_1_6_GSInfo_Extreme_D_N)
		{
			CGSInfo info;

			
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = -100;
			info.m_nbDays = 3;

			CTRef a = info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0);
			CTRef b = info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0);

			Assert::IsTrue(!info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, 0).IsInit());
			Assert::IsTrue(!info.GetLast(WEATHER_N_D[YEAR], JANUARY, JUNE, 0).IsInit());
			Assert::IsTrue(!info.GetFirst(WEATHER_N_D[YEAR], JULY, DECEMBER, 0).IsInit());
			Assert::IsTrue(!info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0).IsInit());


			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = 100;
			info.m_nbDays = 3;


			Assert::IsTrue(info.GetFirst(WEATHER_N_D[YEAR], JANUARY, JUNE, -2) == CTRef(2022, JANUARY, DAY_01));
			Assert::IsTrue(info.GetLast(WEATHER_N_D[YEAR], JULY, DECEMBER, 0) == CTRef(2022, DECEMBER, DAY_31));
			

		}
		TEST_METHOD(Test_1_2_0_GetGrowingSeasonDefault_D_N)
		{
			//default SG
			CGrowingSeason GS;

			//CTPeriod p1 = GS.GetGrowingSeason(WEATHER_N_D[YEAR]);
			CTPeriod p1 = GS.GetPeriod(WEATHER_N_D[YEAR]);
	
			//Assert::IsTrue(p1 == CTPeriod(2022, MAY, DAY_07, 2022, NOVEMBER, DAY_01));
			Assert::IsTrue(p1 == CTPeriod(2022, MAY, DAY_07, 2022, NOVEMBER, DAY_01));
		}

		TEST_METHOD(Test_1_2_1_GetGrowingSeason_D_N)
		{
			CGrowingSeason GS;
			
			GS.m_begin.m_d = CGSInfo::GET_FIRST;
			GS.m_begin.m_TT = CGSInfo::TT_TMIN;
			GS.m_begin.m_op = '>';
			GS.m_begin.m_threshold = 0;
			GS.m_begin.m_nbDays = 5;
			
			GS.m_end.m_d = CGSInfo::GET_LAST;
			GS.m_end.m_TT = CGSInfo::TT_TMIN;
			GS.m_end.m_op = '>';
			GS.m_end.m_threshold = 0;
			GS.m_end.m_nbDays = 5;

			CTPeriod p1 = GS.GetPeriod(WEATHER_N_D[YEAR]);
			
			Assert::IsTrue(p1 == CTPeriod(2022, MAY, DAY_07, 2022, NOVEMBER, DAY_01));
		}



		
		TEST_METHOD(Test_1_3_0_GetFrostFreePeriod_D_N)
		{

			CFrostFreePeriod FF;
			CTPeriod p1 = FF.GetPeriod(WEATHER_N_D[YEAR]);
			//CTPeriod p2 = FF.GetFrostFreePeriod(WEATHER_N_D[YEAR]);
			
			Assert::IsTrue(p1 == CTPeriod(2022, MAY, DAY_07, 2022, NOVEMBER, DAY_01));
		}

		TEST_METHOD(Test_1_4_0_Execute_D_N)
		{
			CGrowingSeason GS;

			CModelStatVector output;
			GS.Execute(WEATHER_N_D, output);

			Assert::IsTrue(output[0][O_GS_BEGIN] == CTRef(2022, MAY, DAY_07).GetRef());
			Assert::IsTrue(output[0][O_GS_END] == CTRef(2022, NOVEMBER, DAY_01).GetRef());
			Assert::IsTrue(output[0][O_GS_LENGTH] == 179);
		}



		TEST_METHOD(Test_1_5_0_GetFirst_greater_D_S)
		{
			CGSInfo info;

			info.m_d = CGSInfo::GET_FIRST;
			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '>';
			info.m_threshold = 0;
			info.m_nbDays = 3;

			CTRef a = info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0);
			CTRef b = info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, 0);

			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0) == CTRef(2021, NOVEMBER, DAY_07));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, -2) == CTRef(2021, NOVEMBER, DAY_05));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, JANUARY, DAY_03));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, -2) == CTRef(2022, JANUARY, DAY_01));
			
		}


		TEST_METHOD(Test_1_5_1_GetLast_greater_D_S)
		{
			CGSInfo info;

			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '>';
			info.m_threshold = 0;
			info.m_nbDays = 3;


			CTRef a = info.GetLast(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0);
			CTRef b = info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 0);
			


			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0) == CTRef(2021, DECEMBER, DAY_31));
			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, MAY, DAY_08));
			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 1) == CTRef(2022, MAY, DAY_09));
			

		}


		TEST_METHOD(Test_1_5_2_GetFirst_smaller_D_S)
		{
			CGSInfo info;

			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = 0;
			info.m_nbDays = 3;

			CTRef a = info.GetFirst(WEATHER_S_D[YEAR - 1], JULY, DECEMBER, 0);
			CTRef b = info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, 0);
			

			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0) == CTRef(2021, JULY, DAY_03));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, -2) == CTRef(2021, JULY, DAY_01));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR-1], JULY, DECEMBER, -3) == CTRef(2021, JUNE, DAY_30));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, MAY, DAY_11));
			Assert::IsTrue(info.GetFirst(WEATHER_S_D[YEAR], JANUARY, JUNE, -2) == CTRef(2022, MAY, DAY_09));
			
			

		}

		TEST_METHOD(Test_1_5_3_GetLast_smaller_D_S)
		{

			CGSInfo info;

			info.m_TT = CGSInfo::TT_TMIN;
			info.m_op = '<';
			info.m_threshold = 0;
			info.m_nbDays = 3;



			CTRef a = info.GetLast(WEATHER_S_D[YEAR - 1], JULY, DECEMBER, 0);
			CTRef b = info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 0 );
			

			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR-1], JULY, DECEMBER, 0) == CTRef(2021, NOVEMBER, DAY_04));
			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR-1], JULY, DECEMBER, -2) == CTRef(2021, NOVEMBER, DAY_02));
			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 0) == CTRef(2022, JUNE, DAY_30));
			Assert::IsTrue(info.GetLast(WEATHER_S_D[YEAR], JANUARY, JUNE, 1) == CTRef(2022, JULY, DAY_01));
			


		}
		//	void Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output);
	};
}
