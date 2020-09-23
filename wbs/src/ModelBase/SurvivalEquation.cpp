#include "SurvivalEquation.h"
#include <functional>

using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

namespace WBSF
{

	const char* CSurvivalEquation::EQUATION[NB_EQUATIONS][NB_INFO] =
	{
		{"Survival_01","1/(1+exp(a+b*T+c*T^2))","a=-1[-100,0]|b=-0.5[-10,0]|c=0.01[0,1]","over(1, 1+e^{k[0]+k[1]*T+k[2]*T^{~2}})"},
		{"Survival_02","1/(1+exp(a+b*T+c*T^d))","a=-1[-100,0]|b=-0.5[-10,0]|c=0.01[0,1]|d=2[-4,4]","over(1, 1+e^{k[0]+k[1]*T+k[2]*T^{~kk}})"},
		{"Survival_03","1-1/(1+a*exp(-b*((T-To)/deltaT)^2))","a=1[0,1e3]|b=5[0,100]|deltaT=50[1,1e3]|To=15[0,30]", "1 - over(1, 1+k~e^{-kk~bgroup('(',over(T~-~T[o],Delta[T]),')')^2})"},
		{"Survival_04","a*exp(-b*((T-To)/deltaT)^2)","a=1[0,1]|b=1[0,10]|deltaT=10[1e-2,100]|To=15[0,30]", "k~e^{~-~bgroup('(',over(T~-~T[o],Delta[T]),')')^2}"},
		{"Survival_05","a*exp(-b*((T-To)/deltaT)^2)+c","a=0.1[0,1]|b=0.5[0,10]|deltaT=20[1e-2,100]|c=0.8[0,1]|To=15[0,30]", "k[0]+k[1]*e^{~-~bgroup('(',over(T~-~T[o],Delta[T]),')')^2}"},
		{"Survival_06","a*exp(-b*(log(abs(T/To))/deltaT)^2)+c", "a=0.1[0,1]|b=0.5[0,10]|deltaT=1[1e-2,100]|c=0.8[0,1]|To=15[0,30]", "k[0]+k[1]*e^{~-kk~over(ln( bgroup('|',over(T,T[o]),'|') ),Delta[T])^2}"},
		{"Survival_07","a+b*T+c*T^d", "a=0.8[-1,1]|b=-0.03[-1,1]|c=0.14[-1,1]|d=0.5[-4,4]","k[0]+k[1]*T+k[2]*T^{~kk}"},
		{"Survival_08","1-exp(a+b*T+c/T^d)", "a=-1[-100,0]|b=0.005[0,100]|c=0.1[0,1e3]|d=2[0,4]", "1-e^{k[0]+k[1]*T+k[2]*T^{~-kk}}"},
		{"Survival_09","k1/(1+k2*exp(a+b*T+c*T^d))", "k1=1[0,10]|k2=1[0,10]|a=-1[-100,100]|b=-0.5[-10,10]|c=0.01[0,1]|d=2[-4,4]","over(kk[1],1+kk[2]~e^{k[0]+k[1]*T+k[2]*T^{~kk}})"},
		{"Survival_10","1/(exp(a*(1+exp(-(T-To)/deltaTl))*(1+exp(-(To-T)/deltaTh))))", "To=15[0,100|deltaTl=10[1e-5,1e3]|deltaTh=2[1e-5,1e3]|a=5e-4[1e-5,1]", "over(1, e^{kk~bgroup('(',1+e^{~-~over(T~-~T[L],Delta[T[L]])},')')~bgroup('(',1+e^{~-~over(T[H]~-~T,Delta[T[H]])},')')})"},
		{"Survival_11","a/(exp(1+exp(-(T-To)/deltaTl))*(1+exp(-(To-T)/deltaTh)))", "To=25[0,50|deltaTl=10[1e-5,1e3]|deltaTh=10[1e-5,1e3]|a=2[1,1e3]", "over(k, e^{bgroup('(',1+e^{~-~over(T~-~T[o],Delta[T[L]])},')')}~bgroup('(',1+e^{~-~over(T[o]~-~T,Delta[T[H]])},')'))"},
		{"Survival_12","a/(exp(1+exp(-(T-Tl)/deltaTl))*(1+exp(-(Th-T)/deltaTh)))", "Tl=10[0,50|Th=30[0,50]|deltaTl=10[1e-5,1e3]|deltaTh=10[1e-5,1e3]|a=2[1,1e3]", "over(k, e^{bgroup('(',1+e^{~-~over(T~-~T[L],Delta[T[L]])},')')}~bgroup('(',1+e^{~-~over(T[H]~-~T,Delta[T[H]])},')'))"},
		//{"Survival_14","exp(-(exp(a1+b1*T))) + exp(-(exp(a2+b2*T)))","a1=1[-100,100]|b1=1[-100,100]|a2=1[-100,100]|b2=1[-100,100]", "e^{-e^{k[1]+b[1]*T}} + e^{-e^{k[2]+b[2]*T} }"},
//		{"Survival_16","1-exp(a*T*pmax(0,T-Tl)*pmax(0,Th-T)^d)","a=-1[-100,-1e-5]|Tl=15[0,100]|Th=50[10,100]|d=-2[-4,4]", "1-e^{a~T~(T-T[L])~(T[H]-T)^d}"},
		{"Survival_13","1-a*(1-exp(-pmax(0,(T-Tl))/deltaT))*(1-exp(-pmax(0,(Th-T))/deltaT))","a=5[0,100]|Tl=0[0,50]|Th=35[10,100]|deltaT=30[0.1,1e4]","{1-k~bgroup('(',1-e^{~-~over(T~-~T[L],Delta[T])},')')~bgroup('(',{1-e^{~-~over(T[H]~-~T,Delta[T])}}, ')')}"},
		{"Survival_14","1-exp(a*(1-exp(-pmax(0,(T-Tl))/deltaTl))*(1-exp(-pmax(0,(Th-T))/deltaTh)))","a=-1[-100,-1e-5]|Tl=0[-100,100]|Th=50[-100,100]|deltaTl=1[1e-5,1e5]|deltaTh=1[1e-5,1e5]", "1-e^{~kk~bgroup('(',1~-~e^{~-~over(T~-~T[L],Delta[T[L]])},')')~bgroup('(',{1~-~e^{~-~over(T[H]~-~T,Delta[T[H]])}}, ')')}"},
		{"Gompertz&Makeham","a1*exp(b1*T) + a2*exp(b2*T) + c","a1=-0.2[-10,-1e-4]|b1=0.005[0,1]|a2=-0.25[-10,10]|b2=-0.2[-10,0]|c=1.2[-2,2]", "k[0]+k[1]~e^{kk[1]~T} + k[2]~e^{kk[2]~T}"},
		{"Wang2","1/(exp(a*(1+exp(-(T-Tl)/deltaTl))*(1+exp(-(Th-T)/deltaTh))))", "Tl=10[-50,50|Th=30[0,100]|deltaTl=10[0.1,100]|deltaTh=2[0.1,100]|a=5e-5[1e-5,1]", "over(1, e^{kk~bgroup('(',1+~e^{-~~over(T~-~T[o],Delta[T[L]])},')')~bgroup('(',1+~e^{-~~over(T[o]~-~T,Delta[T[H]])},')')})"},
	};

	bool CSurvivalEquation::IsParamValid(CSurvivalEquation::TSurvivalEquation model, const std::vector<double>& P)
	{
		bool bValid = true;

		switch (model)
		{
		case Survival_12:bValid = P[P0] < P[P1]; break;
		case Survival_13:
		case Survival_14:bValid = P[P1] < P[P2]; break;
		case Wang2:bValid = P[P0] < P[P1]; break;
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

		if (model == Survival_01)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];

			s = 1.0 / (1.0 + exp(a + b * T + c * T * T));
		}
		else if (model == Survival_02)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = 1.0 / (1.0 + exp(a + b * T + c * pow(T, d)));
		}
		else if (model == Survival_03)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double To = P[P3];

			s = 1 - 1.0 / (1.0 + a * exp(-b * pow((T - To) / c, 2.0)));
		}
		else if (model == Survival_04)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double To = P[P3];

			s = a * exp(-b * pow((T - To) / c, 2.0));
		}
		else if (model == Survival_05)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];
			double To = P[P4];

			s = a * exp(-b * pow((T - To) / c, 2.0)) + d;
		}
		else if (model == Survival_06)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];
			double To = P[P4];

			s = a * exp(-b * pow(log(abs(T / To)) / c, 2.0)) + d;
		}
		else if (model == Survival_07)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = a + b * T + c * pow(T, d);
		}
		else if (model == Survival_08)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			s = 1.0 - exp(a + b * T + c / pow(T, d));
		}
		else if (model == Survival_09)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double a = P[P2];
			double b = P[P3];
			double c = P[P4];
			double d = P[P5];

			s = b1 / (1.0 + b2 * exp(a + b * T + c * pow(T, d)));
		}
		else if (model == Survival_10)
		{
			double To = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];

			s = 1.0 / exp((1.0 + exp(-(T - To) / Bl))*(1.0 + exp(-(To - T) / Bh))*H);
		}
		else if (model == Survival_11)
		{
			double To = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];

			s = H / (exp(1.0 + exp(-(T - To) / Bl))*(1.0 + exp(-(To - T) / Bh)));
		}
		else if (model == Survival_12)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double Bl = P[P2];
			double Bh = P[P3];
			double H = P[P4];

			//T = max(Tl, min(Th, T));
			s = H / (exp(1.0 + exp(-(T - Tl) / Bl))*(1.0 + exp(-(Th - T) / Bh)));
		}
		//else if (model == Survival_14)
		//{
		//	double a1 = P[P0];
		//	double b1 = P[P1];
		//	double a2 = P[P2];
		//	double b2 = P[P3];

		//	s = exp(-(exp(a1 + b1 * T))) + exp(-(exp(a2 + b2 * T)));
		//}
		//else if (model == Survival_16)
		//{
		//	double a = P[P0];
		//	double Tl = P[P1];
		//	double Th = P[P2];
		//	double d = P[P3];

		//	T = max(Tl, min(Th, T));
		//	s = 1.0 - exp(a*T*(T - Tl)*pow(Th - T, d));
		//}
		else if (model == Survival_13)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaT = P[P3];

			T = max(Tl, min(Th, T));
			s = 1.0 - a * (1.0 - exp(-(T - Tl) / deltaT))*(1.0 - exp(-(Th - T) / deltaT));
		}
		else if (model == Survival_14)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaTl = P[P3];
			double deltaTh = P[P4];

			T = max(Tl, min(Th, T));
			s = 1.0 - exp(a*(1.0 - exp(-(T - Tl) / deltaTl))*(1.0 - exp(-(Th - T) / deltaTh)));
		}
		else if (model == GompertzMakeham)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];
			double c = P[P4];

			s = a1 * exp(b1*T) + a2 * exp(b2*T) + c;
		}
		else if (model == Wang2)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double deltaTl = P[P2];
			double deltaTh = P[P3];
			double H = P[P4];

			//T = max(Tl, min(Th, T));
			s = 1.0 / exp((1.0 + exp(-(T - Tl) / deltaTl))*(1.0 + exp(-(Th - T) / deltaTh))*H);
		}


		return s;
	}








}
