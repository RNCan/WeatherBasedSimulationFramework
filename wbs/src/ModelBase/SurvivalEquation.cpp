#include "SurvivalEquation.h"
#include <functional>

using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

namespace WBSF
{

	const char* CSurvivalEquation::EQUATION[NB_EQUATIONS][3] =
	{
		{"1/(1+exp(a+b·T+c·T²))","1/(1+exp(a+b*T+c*T^2))","a=-1[-100,0]|b=-0.5[-10,0]|c=0.01[0,1]"},
		{"1/(1+exp(a+b·T+c·T^d))","1/(1+exp(a+b*T+c*T^d))","a=-1[-100,0]|b=-0.5[-10,0]|c=0.01[0,1]|d=2[-4,4]"},
		{"1-1/(1+a·exp(-b((T-To)/c)²))","1-1/(1+a*exp(-b*((T-To)/c)^2))","a=1[0,1e3]|b=5[0,100]|c=50[1,1e3]|To=15[0,30]"},
		{"a·exp(-b((T-To)/c)²)","a*exp(-b*((T-To)/c)^2)","a=1[0,1]|b=1[0,10]|c=10[1e-2,100]|To=15[0,30]"},
		{"a·exp(-b((T-To)/c)²)+d","a*exp(-b*((T-To)/c)^2)+d","a=0.1[0,1]|b=0.5[0,10]|c=20[1e-2,100]|d=0.8[0,1]|To=15[0,30]"},
		{"a·exp(-b(log(abs(T/To))/c)²)+d","a*exp(-b*(log(abs(T/To))/c)^2)+d", "a=0.1[0,1]|b=0.5[0,10]|c=1[1e-2,100]|d=0.8[0,1]|To=15[0,30]"},
		{"a+b·T+c·T^d ","a+b*T+c*T^d", "a=0.8[-1,1]|b=-0.03[-1,1]|c=0.14[-1,1]|d=0.5[-4,4]"},
		{"1-exp(a+b·T+c/T^d)","1-exp(a+b*T+c/T^d)", "a=-1[-100,0]|b=0.005[0,100]|c=0.1[0,1e3]|d=2[0,4]"},
		{"b1/(1+b2·exp(a+b·T+c·T^d))","b1/(1+b2*exp(a+b*T+c*T^d))", "b1=1[0,10]|b2=1[0,10]|a=-1[-100,100]|b=-0.5[-10,10]|c=0.01[0,1]|d=2[-4,4]"},
		{"1/(exp((1+exp(-(T-To)/Bl))·(1+exp(-(To-T)/Bh))·H))","1/(exp((1+exp(-(T-To)/Bl))*(1+exp(-(To-T)/Bh))*H))", "To=15[0,100|Bl=10[1e-5,1e3]|Bh=2[1e-5,1e3]|H=5e-4[1e-5,1]"},
		{"1/(exp((1+exp(-(T-Tl)/Bl))·(1+exp(-(Th-T)/Bh))·H))","1/(exp((1+exp(-(T-Tl)/Bl))*(1+exp(-(Th-T)/Bh))*H))", "Tl=10[0,100|Th=30[0,100]|Bl=10[1e-5,1e3]|Bh=2[1e-5,1e3]|H=5e-4[1e-5,1]"},
		{"H/(exp(1+exp(-(T-To)/Bl))·(1+exp(-(To-T)/Bh)))","H/(exp(1+exp(-(T-To)/Bl))*(1+exp(-(To-T)/Bh)))", "To=25[0,50|Bl=10[1e-5,1e3]|Bh=10[1e-5,1e3]|H=2[1,1e3]"},
		{"H/(exp(1+exp(-(T-Tl)/Bl))·(1+exp(-(Th-T)/Bh)))","H/(exp(1+exp(-(T-Tl)/Bl))*(1+exp(-(Th-T)/Bh)))", "Tl=10[0,50|Th=30[0,50]|Bl=10[1e-5,1e3]|Bh=10[1e-5,1e3]|H=2[1,1e3]"},
		{"exp(-(exp(a1+b1·T))) + exp(-(exp(a2+b2·T)))","exp(-(exp(a1+b1*T))) + exp(-(exp(a2+b2*T)))","a1=1[-100,100]|b1=1[-100,100]|a2=1[-100,100]|b2=1[-100,100]"},
		{"a1·exp(b1·T) + a2·exp(b2·T) + c","a1*exp(b1*T) + a2*exp(b2*T) + c","a1=-0.2[-10,10]|b1=0.01[0,1]|a2=-0.1[-10,10]|b2=-0.1[-1,0]|c=1.2[-2,2]"},
		{"1-exp(a·T·(T-Tl)·(Th-T)^d)","1-exp(a*T*pmax(0,T-Tl)*pmax(0,Th-T)^d)","a=-1[-100,-1e-5]|Tl=15[0,100]|Th=50[10,100]|d=-2[-4,4]"},
		{"1-a·(1-exp(-(T-Tl)/B))·(1-exp(-(Th-T)/B))","1-a*(1-exp(-pmax(0,(T-Tl))/B))*(1-exp(-pmax(0,(Th-T))/B))","a=1[1e-5,1e5]|Tl=0[-100,100]|Th=50[10,100]|B=1[1e-5,1e5]"},
		{"1-exp(a·(1-exp(-(T-Tl)/Bl))·(1-exp(-(Th-T)/Bh)))","1-exp(a*(1-exp(-pmax(0,(T-Tl))/Bl))*(1-exp(-pmax(0,(Th-T))/Bh)))","a=-1[-100,-1e-5]|Tl=0[-100,100]|Th=50[-100,100]|Bl=1[1e-5,1e5]|Bh=1[1e-5,1e5]"},
	};

	bool CSurvivalEquation::IsParamValid(CSurvivalEquation::TSurvivalEquation model, const std::vector<double>& P)
	{
		bool bValid = true;

		switch (model)
		{
		case 10:
		case 12:bValid = P[P0] < P[P1]; break;
		case 15:
		case 16:
		case 17:bValid = P[P1] < P[P2]; break;
		}

		return bValid;
	}

	CSAParameterVector CSurvivalEquation::GetParameters(TSurvivalEquation model)
	{
		CSAParameterVector p;

		StringVector tmp(CSurvivalEquation::EQUATION[model][EQ_PARAM], "=[,]|");
		ASSERT(tmp.size() % 4 == 0);
		for (size_t i = 0; i < tmp.size() / 4; i++)
		{
			p.push_back(CSAParameter(tmp[i * 4], stod(tmp[i * 4 + 1]), stod(tmp[i * 4 + 2]), stod(tmp[i * 4 + 3])));
		}

		return p;
	}

	CSAParameterVector CSurvivalEquation::GetParameters(TSurvivalEquation model, const vector<double>& X)
	{
		CSAParameterVector p = GetParameters(model);
		ASSERT(X.size() >= p.size());
		for (size_t i = 0; i < p.size(); i++)
		{
			p[i].m_initialValue = X[i];
		}

		return p;
	}










	double CSurvivalEquation::GetSurvival(CSurvivalEquation::TSurvivalEquation model, const std::vector<double>& P, double T)
	{
		double s = 0;

		if (model == 0)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];

			s = 1.0 / (1.0 + exp(a + b * T + c * T * T));
		}
		else if (model == 1)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = 1.0 / (1.0 + exp(a + b * T + c * pow(T, d)));
		}
		else if (model == 2)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double To = P[P3];

			s = 1 - 1.0 / (1.0 + a * exp(-b * pow((T - To) / c, 2.0)));
		}
		else if (model == 3)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double To = P[P3];

			s = a * exp(-b * pow((T - To) / c, 2.0));
		}
		else if (model == 4)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];
			double To = P[P4];

			s = a * exp(-b * pow((T - To) / c, 2.0)) + d;
		}
		else if (model == 5)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];
			double To = P[P4];

			s = a * exp(-b * pow(log(abs(T / To)) / c, 2.0)) + d;
		}
		else if (model == 6)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = a + b * T + c * pow(T, d);
		}
		else if (model == 7)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = 1.0 - exp(a + b * T + c / pow(T, d));
		}
		else if (model == 8)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double a = P[P2];
			double b = P[P3];
			double c = P[P4];
			double d = P[P5];

			s = b1 / (1.0 + b2 * exp(a + b * T + c * pow(T, d)));
		}
		else if (model == 9)
		{
			double To = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];

			s = 1.0 / exp((1.0 + exp(-(T - To) / Bl))*(1.0 + exp(-(To - T) / Bh))*H);
		}
		else if (model == 10)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double Bl = P[P2];
			double Bh = P[P3];
			double H = P[P4];

			//T = max(Tl, min(Th, T));
			s = 1.0 / exp((1.0 + exp(-(T - Tl) / Bl))*(1.0 + exp(-(Th - T) / Bh))*H);
		}
		else if (model == 11)
		{
			double To = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];

			s = H / (exp(1.0 + exp(-(T - To) / Bl))*(1.0 + exp(-(To - T) / Bh)));
		}
		else if (model == 12)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double Bl = P[P2];
			double Bh = P[P3];
			double H = P[P4];

			//T = max(Tl, min(Th, T));
			s = H / (exp(1.0 + exp(-(T - Tl) / Bl))*(1.0 + exp(-(Th - T) / Bh)));
		}
		else if (model == 13)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];

			s = exp(-(exp(a1 + b1 * T))) + exp(-(exp(a2 + b2 * T)));
		}
		else if (model == 14)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];
			double c = P[P4];

			s = a1 * exp(b1*T) + a2 * exp(b2*T) + c;
		}
		else if (model == 15)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double d = P[P3];

			T = max(Tl, min(Th, T));
			s = 1.0 - exp(a*T*(T - Tl)*pow(Th - T, d));
		}

		else if (model == 16)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double B = P[P3];

			T = max(Tl, min(Th, T));
			s = 1.0 - a * (1.0 - exp(-(T - Tl) / B))*(1.0 - exp(-(Th - T) / B));
		}
		else if (model == 17)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double Bl = P[P3];
			double Bh = P[P4];

			T = max(Tl, min(Th, T));
			s = 1.0 - exp(a*(1.0 - exp(-(T - Tl) / Bl))*(1.0 - exp(-(Th - T) / Bh)));
		}



		return s;
	}








}
