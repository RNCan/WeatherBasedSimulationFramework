///////////////////////////////////////////////////////////////////////////// 
// version
// 1.0.0:19/01/2022	Rémi Saint-Amant	Final version

#include "SurvivalEquation.h"
#include <functional>


using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

namespace WBSF
{

	const char* CSurvivalEquation::EQUATION[NB_EQUATIONS][NB_INFO] =
	{
		{"Survival_01","1/(1+exp(k0+k1*T+k2*T^2))","k0=-1[-50,50]|k1=-0.5[-10,5]|k2=0.01[-1,1]","over(1, 1+e^{k[0]+k[1]*T+k[2]*T^{~2}})"},
		{"Survival_02","1/(1+exp(k0+k1*T+k2*T^kk))","k0=-1[-50,50]|k1=-0.5[-10,10]|k2=0.01[-1,1]|kk=2[-4,4]","over(1, 1+e^{k[0]+k[1]*T+k[2]*T^{~kk}})"},
		{"Survival_03","1-1/(1+k*exp(-kk*((T-To)/deltaT)^2))","k=20[0,1e3]|kk=2[0,100]|To=20[0,30]|deltaT=10[1,1e3]", "1 - over(1, 1+k~e^{-kk~bgroup('(',over(T~-~T[o],Delta[T]),')')^2})"},
		{"Survival_04","k*exp(-kk*((T-To)/deltaT)^2)","k=1[0,1]|kk=1[0,10]|To=15[0,30]|deltaT=10[1e-2,100]", "k~e^{~-~kk~bgroup('(',over(T~-~T[o],Delta[T]),')')^2}"},
		{"Survival_05","k0+k1*exp(-kk*((T-To)/deltaT)^2)","k0=0.8[0,1]|k1=0.1[0,1]|kk=0.5[0,10]|To=15[0,30]|deltaT=20[1e-2,100]", "k[0]+k[1]*e^{~-~kk~bgroup('(',over(T~-~T[o],Delta[T]),')')^2}"},
		{"Survival_06","k0+k1*exp(-kk*(log(abs(T/To))/deltaT)^2)", "k0=0.8[0,1]|k1=0.1[0,1]|kk=0.5[0,10]|To=15[0,30]|deltaT=1[1e-2,100]", "k[0]+k[1]*e^{~-kk~bgroup('(',over(ln~{ bgroup('|',over(T,T[o]),'|') },Delta[T]),')')^2}"},
		{"Survival_07","k0+k1*T+k2*T^kk", "k0=0.8[-1,1]|k1=-0.03[-1,1]|k2=0.14[-1,1]|kk=0.5[-4,4]","k[0]+k[1]*T+k[2]*T^{~kk}"},
		{"Survival_08","k0+k1*exp(kk1*T) + k2*exp(kk2*T)","k0=1.2[-2,2]|k1=-0.2[-10,-1e-4]|kk1=0.005[0,1]|k2=-0.25[-10,10]|kk2=-0.2[-10,0]", "k[0]+k[1]~e^{kk[1]~T} + k[2]~e^{kk[2]~T}"},
		{"Survival_09","1-exp(k0+k1*T+k2/T^kk)", "k0=-1[-100,0]|k1=0.005[0,100]|k2=0.1[0,1e3]|kk=2[0,4]", "1-e^{k[0]+k[1]*T+k[2]*T^{~-kk}}"},
		{"Survival_10","k0/(1+k1*exp(k2+k3*T+k4*T^kk))", "k0=1[0,10]|k1=1[0,10]|k2=-1[-100,100]|k3=-0.5[-10,10]|k4=0.01[0,1]|kk=2[-4,4]","over(k[0],1+k[1]~e^{k[2]+k[3]*T+k[4]*T^{~kk}})"},
		{"Survival_11","1/(exp(kk*(1+exp(-(T-To)/deltaTl))*(1+exp(-(To-T)/deltaTh))))", "kk=5e-4[1e-5,1]|To=20[0,100|deltaTl=10[1e-5,1e3]|deltaTh=1[1e-5,1e3]", "over(1, e^{kk~bgroup('(',1+~e^{-~~over(T~-~T[o],Delta[T[L]])},')')~bgroup('(',1+~e^{-~~over(T[o]~-~T,Delta[T[H]])},')')})"},
		{"Survival_12","k/(exp(1+exp(-(T-To)/deltaTl))*(1+exp(-(To-T)/deltaTh)))", "k=13[1,1e3]|To=15[0,50|deltaTl=10[1e-5,1e3]|deltaTh=10[1e-5,1e3]", "over(k, e^{bgroup('(',1+e^{~-~over(T~-~T[o],Delta[T[L]])},')')}~bgroup('(',1+e^{~-~over(T[o]~-~T,Delta[T[H]])},')'))"},
		{"Survival_13","ifelse(T>=Tl&T<=Th,k/(exp(1+exp(-(T-Tl)/deltaTl))*(1+exp(-(Th-T)/deltaTh))),0)", "k=7[1,1e3]|Tl=10[0,50|Th=25[0,50]|deltaTl=10[1e-5,1e3]|deltaTh=10[1e-5,1e3]", "over(k, e^{bgroup('(',1+e^{~-~over(T~-~T[L],Delta[T[L]])},')')}~bgroup('(',1+e^{~-~over(T[H]~-~T,Delta[T[H]])},')'))"},
		{"Survival_14","ifelse(T>=Tl&T<=Th,k*(1-exp(-pmax(0,(T-Tl))/deltaT))*(1-exp(-pmax(0,(Th-T))/deltaT)),0)","k=1[0,100]|Tl=10[0,50]|Th=30[10,100]|deltaT=1[0.1,1e4]","{k~bgroup('(',1-e^{~-~over(T~-~T[L],Delta[T])},')')~bgroup('(',{1-e^{~-~over(T[H]~-~T,Delta[T])}}, ')')}"},
		{"Survival_15","ifelse(T>=Tl&T<=Th,1-exp(k*(1-exp(-pmax(0,(T-Tl))/deltaTl))*(1-exp(-pmax(0,(Th-T))/deltaTh))),0)","k=-1[-100,-1e-5]|Tl=0[-100,100]|Th=50[-100,100]|deltaTl=1[1e-5,1e5]|deltaTh=1[1e-5,1e5]", "1-e^{~kk~bgroup('(',1~-~e^{~-~over(T~-~T[L],Delta[T[L]])},')')~bgroup('(',{1~-~e^{~-~over(T[H]~-~T,Delta[T[H]])}}, ')')}"},
		{"Survival_16","ifelse(T>=Tl&T<=Th,1/(exp(k*(1+exp(-(T-Tl)/deltaTl))*(1+exp(-(Th-T)/deltaTh)))),0)", "k=5e-5[1e-5,1]|Tl=10[-50,50|Th=30[0,100]|deltaTl=10[0.1,100]|deltaTh=2[0.1,100]", "over(1, e^{kk~bgroup('(',1+e^{~-~over(T~-~T[L],Delta[T[L]])},')')~bgroup('(',1+e^{~-~over(T[H]~-~T,Delta[T[H]])},')')})"},
	};

	bool CSurvivalEquation::IsParamValid(CSurvivalEquation::TSurvivalEquation model, const std::vector<double>& P)
	{
		bool bValid = true;

		switch (model)
		{
		case Survival_13:
		case Survival_14:
		case Survival_15:
		case Survival_16:bValid = P[P1] < P[P2]; break;
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
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];

			s = 1.0 / (1.0 + exp(k0 + k1 * T + k2 * T * T));
		}
		else if (model == Survival_02)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double kk = P[P3];

			s = 1.0 / (1.0 + exp(k0 + k1 * T + k2 * pow(T, kk)));
		}
		else if (model == Survival_03)
		{
			double k = P[P0];
			double kk = P[P1];
			double To = P[P2];
			double deltaT = P[P3];


			s = 1 - 1.0 / (1.0 + k * exp(-kk * pow((T - To) / deltaT, 2.0)));
		}
		else if (model == Survival_04)
		{
			double k = P[P0];
			double kk = P[P1];
			double To = P[P2];
			double deltaT = P[P3];


			s = k * exp(-kk * pow((T - To) / deltaT, 2.0));
		}
		else if (model == Survival_05)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double kk = P[P2];
			double To = P[P3];
			double deltaT = P[P4];


			s = k0 + k1 * exp(-kk * pow((T - To) / deltaT, 2.0));
		}
		else if (model == Survival_06)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double kk = P[P2];
			double To = P[P3];
			double deltaT = P[P4];

			s = k0 + k1 * exp(-kk * pow(log(abs(T / To)) / deltaT, 2.0));
		}
		else if (model == Survival_07)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double kk = P[P3];

			s = k0 + k1 * T + k2 * pow(T, kk);
		}
		else if (model == Survival_08)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double kk1 = P[P2];
			double k2 = P[P3];
			double kk2 = P[P4];

			s = k0 + k1 * exp(kk1 * T) + k2 * exp(kk2 * T);
		}
		else if (model == Survival_09)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double kk = P[P3];

			s = 1.0 - exp(k0 + k1 * T + k2 * pow(T, -kk));
		}
		else if (model == Survival_10)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double k3 = P[P3];
			double k4 = P[P4];
			double kk = P[P5];

			s = k0 / (1.0 + k1 * exp(k2 + k3 * T + k4 * pow(T, kk)));
		}
		else if (model == Survival_11)
		{
			double kk = P[P0];
			double To = P[P1];
			double deltaTl = P[P2];
			double deltaTh = P[P3];

			s = 1.0 / exp(kk*(1.0 + exp(-(T - To) / deltaTl))*(1.0 + exp(-(To - T) / deltaTh)));
		}
		else if (model == Survival_12)
		{
			double k = P[P0];
			double To = P[P1];
			double deltaTl = P[P2];
			double deltaTh = P[P3];


			s = k / (exp(1.0 + exp(-(T - To) / deltaTl))*(1.0 + exp(-(To - T) / deltaTh)));
		}
		else if (model == Survival_13)
		{
			double k = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaTl = P[P3];
			double deltaTh = P[P4];

			s = (T >= Tl && T <= Th) ? k / (exp(1.0 + exp(-(T - Tl) / deltaTl))*(1.0 + exp(-(Th - T) / deltaTh))) : 0;
		}
		else if (model == Survival_14)
		{
			double k = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaT = P[P3];

			s = (T >= Tl && T <= Th) ? k * (1.0 - exp(-(T - Tl) / deltaT))*(1.0 - exp(-(Th - T) / deltaT)) : 0;
		}
		else if (model == Survival_15)
		{
			double kk = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaTl = P[P3];
			double deltaTh = P[P4];

			s = (T >= Tl && T <= Th) ? 1.0 - exp(kk*(1.0 - exp(-(T - Tl) / deltaTl))*(1.0 - exp(-(Th - T) / deltaTh))) : 0;
		}
		else if (model == Survival_16)
		{
			double kk = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double deltaTl = P[P3];
			double deltaTh = P[P4];

			s = (T >= Tl && T <= Th) ? 1.0 / exp(kk*(1.0 + exp(-(T - Tl) / deltaTl))*(1.0 + exp(-(Th - T) / deltaTh))) : 0;
		}
		else if (model == Unknown)
		{
			s = 1.0;
		}

	//	ASSERT(s >= 0 && s <= 1);
		return s;
	}








}
