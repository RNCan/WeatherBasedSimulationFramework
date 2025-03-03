#include "pch.h"
#include "CppUnitTest.h"
#include "Basic/ARmodel/ARmodel.h"
#include "Basic/UtilMath.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace WBSF;



namespace WBSFUnitTest
{

	double mean(const std::vector<double>& v)
	{
		double sum = 0;

		for (auto& each : v)
			sum += each;

		return sum / v.size();
	}

	double sd(const std::vector<double>& v)
	{
		double square_sum_of_difference = 0;
		double mean_var = mean(v);
		auto len = v.size();

		double tmp;
		for (auto& each : v) {
			tmp = each - mean_var;
			square_sum_of_difference += tmp * tmp;
		}

		return std::sqrt(square_sum_of_difference / (len - 1));
	}


	TEST_CLASS(WBSFUnitTest)
	{
	public:

		TEST_METHOD(TestMethod1)
		{
			vector<double> data = { 988,985.9,978.8,1000.3,995.2,977.1,976.2,988.9,983.8,996.9,996,991.8,999.4,998.9,1018.6,1009.8,990.3,987,990.2,996.7,990,984.9,996,1009.9,1004.9,1006.2,1002.3,991.6,993.1,995.8,986,991.1,996,1004.3,1009.6,996.1,985.9,988.4,999.3,980.7,977,980.4,994.7,993.5,998.6,1003.7,1001.4,980,980.6,998,990.7,996.2,1007,1006.7,992.6,989.5,982.1,994.9,1002.7,1009,1005.9,1000.7,990.9,978.4,985,981.9,982.8,989.2,984.1,1009.3,1010.3,1002.3,993.8,989.8,992.4,990,994.6,1002.1,999,996.2,991.9,980,977.3,984.5,985.6,989.4,979,983.9,978.9,975.6 };


			double mean_pres = mean(data);
			Assert::AreEqual(992.7, Round(mean_pres, 1));


			CARModel AR1;
			AR1.fit(2, data, true);//substrat mean

			//printf(" %8s %8s %8s %8s %8s %8s %8s\n", "p0", "p1", "MCC", "BIC", "GIC", "AIC", "AICC");
			//for (size_t i = 0; i < AR.coeffs().size(); ++i)
				//printf(" %8.4lg", AR.coeffs()[i]);

			double a1 = AR1.coeffs()[0];
			double a2 = AR1.coeffs()[1];
			double mean = AR1.mean();
			double sigma = AR1.sigma();
			Assert::AreEqual(0.704 , Round(a1, 3) );
			Assert::AreEqual(-0.255, Round(a2, 3) );
			Assert::AreEqual(992.7, Round(mean, 1));
			Assert::AreEqual(7.74, Round(sigma, 2));
			
			
			//printf(" %8.4lg", AR.get_metrics<MCC>());
			//printf(" %8.4lg", AR.get_metrics<BIC>());
			//printf(" %8.4lg", AR.get_metrics<GIC<>>());
			//printf(" %8.4lg", AR.get_metrics<AIC>());
			//printf(" %8.4lg", AR.get_metrics<AICC>());
			//printf("\n");


			CARModel AR2(const vector<double> {a1,a2} , mean, sigma);
			

			//Generate data from this AR model
			//size_t N = 10000;
			vector<double> data2 = AR2.predict(10'000, data);

			//Evaluate the new parameters with this generation
			CARModel AR3;
			AR3.fit(2, data2, true);


			double test_a1 = Round(AR3.coeffs()[0],2);
			double test_a2 = Round(AR3.coeffs()[1],2);
			double test_mean = Round(AR3.mean(),1);
			double test_sigma = Round(AR3.sigma(),2);
			Assert::IsTrue(test_a1>=0.68&& test_a1 <= 0.72);
			Assert::IsTrue(test_a2 >= -0.27 && test_a2 <= -0.23);
			Assert::IsTrue(test_mean >= 990.7&& test_mean<= 994.7);
			Assert::IsTrue(test_sigma >= 7.54 && test_sigma <= 7.94);


		}
	};
}
