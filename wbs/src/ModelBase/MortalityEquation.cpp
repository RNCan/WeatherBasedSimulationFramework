#include "MortalityEquation.h"
#include <functional>

using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

namespace WBSF
{

	//if(modelm==1)  form<-expression(a*xlinea^2+b*xlinea+c)
	//if(modelm==2)  form<-expression(a*sqrt(xlinea)+b*xlinea+c)
	//if(modelm==3)  form<-expression(a*(1/sqrt(xlinea))+b*xlinea+c)
	//if(modelm==4)  form<-expression(a*(1/xlinea^2)+b*xlinea+c)
	//if(modelm==5)  form<-expression(a*(1/xlinea)+b*xlinea+c)
	//if(modelm==6)  form<-expression(b*xlinea+a*log(xlinea)+c)
	//if(modelm==7)  form<-expression(1/(1+a*exp(-b*((xlinea-c)/d)^2))) # log normal, 4 parameters
	//if(modelm==8)  form<-expression((a*exp(-b*((xlinea-xo)/c)^2)))
	//if(modelm==9)  form<-expression(y0 + a * exp(-0.5 * ((xlinea-x0)/b)^2))
	//if(modelm==10) form<-expression(y0 + a * exp(-0.5 * (log(abs(xlinea/x0))/b)^2))
	//if(modelm==11) form<-expression(b1+b2*xlinea+b3*xlinea^d)
	//if(modelm==12) form<-expression(exp(b1+b2*xlinea+b3*xlinea^2))
	//if(modelm==13) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*xlinea^2))))
	//if(modelm==14) form<-expression(exp(b1+b2*xlinea+b3*sqrt(xlinea)))
	//if(modelm==15) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*sqrt(xlinea)))))
	//if(modelm==16) form<-expression(exp(b1+b2*xlinea+b3*(1/sqrt(xlinea))))
	//if(modelm==17) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*(1/sqrt(xlinea))))))
	//if(modelm==18) form<-expression(exp(b1+b2*xlinea+b3*(1/xlinea)))
	//if(modelm==19) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*(1/xlinea)))))
	//if(modelm==20) form<-expression(exp(b1+b2*xlinea+b3*xlinea^d))
	//if(modelm==21) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*xlinea^d))))
	//if(modelm==22) form<-expression(exp(b1+b2*xlinea+b3*log(xlinea)))
	//if(modelm==23) form<-expression(1-(b4/(1+b5*exp(b1+b2*xlinea+b3*log(xlinea)))))
	//if(modelm==24) form<-expression(1-rm*exp((-0.5)*(-(xlinea-Topt)/Troh)^2))
	//if(modelm==25) form<-expression(1-rm*exp((-0.5)*(-(log(xlinea)-log(Topt))/Troh)^2))
	//if(modelm==26) form<-expression(1 - 1/(exp((1+exp(-(xlinea-Topt)/B))*(1+exp(-(Topt-xlinea)/B))*H)))
	//if(modelm==27) form<-expression(1 - 1/(exp((1+exp(-(xlinea-Tl)/B))*(1+exp(-(Th-xlinea)/B))*H)))
	//if(modelm==28) form<-expression(1 - 1/(exp((1+exp(-(xlinea-Topt)/Bl))*(1+exp(-(Topt-xlinea)/Bh))*H)))
	//if(modelm==29) form<-expression(1 - 1/(exp((1+exp(-(xlinea-Tl)/Bl))*(1+exp(-(Th-xlinea)/Bh))*H)))
	//if(modelm==30) form<-expression(1 - H/(exp(1+exp(-(xlinea-Topt)/B))*(1+exp(-(Topt-xlinea)/B))))
	//if(modelm==31) form<-expression(1 - H/(exp(1+exp(-(xlinea-Tl)/B))*(1+exp(-(Th-xlinea)/B))))
	//if(modelm==32) form<-expression(1 - H/(exp(1+exp(-(xlinea-Topt)/Bl))*(1+exp(-(Topt-xlinea)/Bh))))
	//if(modelm==33) form<-expression(1 - H/(exp(1+exp(-(xlinea-Tl)/Bl))*(1+exp(-(Th-xlinea)/Bh))))
	//if(modelm==34) form<-expression(Bm + ((1-1/(1+exp((Hl/1.987)*((1/Tl)-(1/(xlinea+273.15))))+exp((Hh/1.987)*((1/Th)-(1/(xlinea+273.15))))))*(1-Bm))) #DM: se agrego 2 parentesis "Bm + (...)*"
	//if(modelm==35) form<-expression((1 - exp(-(exp(a1+b1*xlinea)))) + (1 - exp(-(exp(a2+b2*xlinea)))))
	//if(modelm==36) form<-expression((w-xlinea)^(-1))
	//if(modelm==37) form<-expression(a1*exp(b1*xlinea) + a2*exp(b2*xlinea))
	//if(modelm==38) form<-expression(a1*exp(b1*xlinea) + a2*exp(b2*xlinea)+c1)
	//if(modelm==39) form<-expression(a*(abs(xlinea-b))^nn)
	//if(modelm==40) form<-expression(a*xlinea*(xlinea-To)*(Tl-xlinea)^(1/d))
	//if(modelm==41) form<-expression(exp(a*xlinea*(xlinea-To)*(Tl-xlinea)^(1/d)))
	//if(modelm==42) form<-expression(a*((xlinea-Tmin)^n)*(Tmax-xlinea)^m)
	//if(modelm==43) form<-expression(1/((Dmin/2) * (exp(k*(xlinea-Tp)) + exp(-(xlinea-Tp)*lamb))))
	//if(modelm==44) form<-expression(a*(1-exp(-(xlinea-Tl)/B))*(1-exp(-(Th-xlinea)/B)))
	//if(modelm==45) form<-expression(exp(a*(1-exp(-(xlinea-Tl)/Bl))*(1-exp(-(Th-xlinea)/Bh))))

	const char* CMortalityEquation::EQUATION[NB_EQUATIONS][3] =
	{

		{"aT²+bT+c","a*T^2+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"a·sqrt(T)+bT+c","a*sqrt(T)+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"a/sqrt(T)+bT+c","a/sqrt(T)+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"a/T²+bT+c","a/T^2+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"a/T+bT+c","a/T+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"a·ln(T)+bT+c","a·ln(T)+b*T+c","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]"},
		{"1/(1+a·exp(-b((T-c)/d)²))","1/(1+a*exp(-b((T-c)/d)^2))","a=1[-100,100]|b=1[-100,100]|c=1[0,100]|d=1[-100,100]"},
		{"a·exp(-b((T-To)/c)²)","a*exp(-b((T-To)/c)^2)","a=1[-100,100]|b=1[-100,100]|c=1[-100,100]|To=1[0,50]"},
		{"y0+a·exp(-0.5((T-To)/b)²)","y0+a*exp(-0.5((T-To)/b)²)","y0=0[0,1]|a=1[-100,100]|b=1[-100,100]|To=1[0,50]"},
		{"y0+a·exp(-0.5(log(abs(T/To))/b)²)","y0+a·exp(-0.5(log(abs(T/To))/b)^2)", "y0=0[0,1]|a=1[-100,100]|b=1[-100,100]|To=1[0,50]"},
		{"b1+b2.T+b3.T^d ","b1+b2*T+b3*T^d", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|d=1[0,4]"},
		{"exp(b1+b2.T+b3.T²) ","exp(b1+b2*T+b3*T^2)", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.T²)))","1-(b4/(1+b5*exp(b1+b2*T+b3*T^2)))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|b4=1[-100,100]|b5=1[-100,100]"},
		{"exp(b1+b2.T+b3.(T)^0.5)","exp(b1+b2*T+b3*(T)^0.5)", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.(T)^0.5)))","1-(b4/(1+b5*exp(b1+b2*T+b3*T^0.5)))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"exp(b1+b2.T+b3.(1/(T)^0.5))","exp(b1+b2*T+b3*(1/(T)^0.5))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.(1/(T)^0.5))))","1-(b4/(1+b5*exp(b1+b2*T+b3*(1/(T)^0.5))))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|b4=1[-100,100]|b5=1[-100,100]"},
		{"exp(b1+b2.T+b3.(1/T))","exp(b1+b2.T+b3*(1/T))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.(1/T))))","1-(b4/(1+b5*exp(b1+b2*T+b3*(1/T))))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|b4=1[-100,100]|b5=1[-100,100]"},
		{"exp(b1+b2.T+b3.T^d)","exp(b1+b2*T+b3*T^d)", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|d=1[0,4]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.T^d)))","1-(b4/(1+b5*exp(b1+b2*T+b3*T^d)))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|b4=1[-100,100]|b5=1[-100,100]|d=1[0,4]"},
		{"exp(b1+b2.T+b3.ln(T))","exp(b1+b2*T+b3*ln(T))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]"},
		{"1-(b4/(1+b5.exp(b1+b2.T+b3.ln(T))))","1-(b4/(1+b5*exp(b1+b2*T+b3*ln(T))))", "b1=1[-100,100]|b2=1[-100,100]|b3=1[-100,100]|b4=1[-100,100]|b5=1[-100,100]"},
		{"1-rm.exp((-0.5).(-(T-Topt)/Troh)²)","1-rm*exp((-0.5)*(-(T-Topt)/Troh)^2)", "Topt=1[0,100]|Troh=1[-100,100]"},
		{"1-rm.exp((-0.5).(-(log(T)-log(Topt))/Troh)^2)","1-rm*exp((-0.5)*(-(log(T)-log(Topt))/Troh)^2)", "Topt=1[0,100]|Troh=1[-100,100]"},
		{"1-1/(exp((1+exp(-(T-Topt)/B)).(1+exp(-(Topt-T)/B)).H))","1 - 1/(exp((1+exp(-(T-Topt)/B))*(1+exp((Topt-T)/B))*H))", "Topt=1[0,100]|B=1[-100,100]|H=1[-100,100]"},
		{"1-1/(exp((1+exp(-(T-Tl)/B)).(1+exp(-(Th-T)/B)).H))","1 - 1/(exp((1+exp(-(T-Tl)/B))*(1+exp(-(Th-T)/B))*H))", "Tl=0[0,50|Th=50[0,50]|B=1[-100,100]|H=1[-100,100]"},
		{"1-1/(exp((1+exp(-(T-Topt)/Bl)).(1+exp(-(Topt-T)/Bh)).H))","1 - 1/(exp((1+exp(-(T-Topt)/Bl))*(1+exp(-(Topt-T)/Bh))*H))", "Topt=0[0,50|Bl=50[0,50]|Bh=1[-100,100]|H=1[-100,100]"},
		{"1-1/(exp((1+exp(-(T-Tl)/Bl)).(1+exp(-(Th-T)/Bh)).H))","1 - 1/(exp((1+exp(-(T-Tl)/Bl))*(1+exp(-(Th-T)/Bh))*H))", "Tl=0[0,50|Th=50[0,50]|Bl=1[-100,100]|Bh=1[-100,100]|H=1[-100,100]"},
		{"1-H/(exp(1+exp(-(T-Topt)/B)).(1+exp(-(Topt-T)/B)))","1 - H/(exp(1+exp(-(T-Topt)/B))*(1+exp(-(Topt-T)/B)))", "Topt=1[0,100]|B=1[-100,100]|H=1[-100,100]"},
		{"1-H/(exp(1+exp(-(T-Tl)/B))*(1+exp(-(Th-T)/B)))","1 - H/(exp(1+exp(-(T-Tl)/B))*(1+exp(-(Th-T)/B)))", "Tl=0[0,50|Th=50[0,50]|B=1[-100,100]|H=1[-100,100]"},
		{"1-H/(exp(1+exp(-(T-Topt)/Bl)).(1+exp(-(Topt-T)/Bh)))","1 - H/(exp(1+exp(-(T-Topt)/Bl))*(1+exp(-(Topt-T)/Bh))", "Topt=25[0,50|Bl=1[-100,100]|Bh=1[-100,100]|H=1[-100,100]"},
		{"1-H/(exp(1+exp(-(T-Tl)/Bl)).(1+exp(-(Th-T)/Bh)))","1 - H/(exp(1+exp(-(T-Tl)/Bl))*(1+exp(-(Th-T)/Bh)))", "Tl=0[0,50|Th=50[0,50]|Bl=1[-100,100]|Bh=1[-100,100]|H=1[-100,100]"},
		{"Bm + ((1-1/(1+exp((Hl/1.987).((1/Tl)-(1/T)))+exp((Hh/1.987)*((1/Th)-(1/T))))).(1-Bm))","Bm + ((1-1/(1+exp((Hl/1.987)*((1/Tl)-(1/T)))+exp((Hh/1.987)*((1/Th)-(1/T)))))*(1-Bm))", "Tl=0[0,50|Th=50[0,50]|Hl=1[-100,100]|Hh=1[-100,100]|Bm=1[-100,100]"},
		{"(1 - exp(-(exp(a1+b1.T)))) + (1 - exp(-(exp(a2+b2.T))))","(1 - exp(-(exp(a1+b1*T)))) + (1 - exp(-(exp(a2+b2*T))))","a1=1[-100,100]|b1=1[-100,100]|a2=1[-100,100]|b2=1[-100,100]"},
		{"(w-T)^(-1)","(w-T)^(-1)","w=25[-100,100]"},
		{"a1.exp(b1.T) + a2.exp(b2.T)","a1*exp(b1*T) + a2*exp(b2*T)","a1=1[-100,100]|b1=1[-100,100]|a2=1[-100,100]|b2=1[-100,100]"},
		{"a1.exp(b1.T) + a2.exp(b2.T) + c1","a1*exp(b1*T) + a2*exp(b2*T) + c1","a1=1[-100,100]|b1=1[-100,100]|a2=1[-100,100]|b2=1[-100,100]|c1=1[-100,100]"},
		{"a.(abs(T-b))^nn","a*(abs(T-b))^nn","a=1[-100,100]|b=1[-100,100]|nn=1[-100,100]"},
		{"a.T.(T-To).(Tl-T)^(1/d)","a*T*(T-To)*(Tl-T)^(1/d)","a=1[-100,100]|To=1[-100,100]|Tl=1[-100,100]|d=1[0.01,10]"},
		{"exp(a.T.(T-To).(Tl-T)^(1/d))","exp(a*T*(T-To)*(Tl-T)^(1/d))","a=1[-100,100]|To=1[-100,100]|Tl=1[-100,100]|d=1[0.01,10]"},
		{"a.((T-Tmin)^n).(Tmax-T)^m","a*((T-Tmin)^n)*(Tmax-T)^m","a=1[-100,100]|Tmin=1[-100,100]|Tmax=1[-100,100]|m=1[0.01,10]|n=1[0.01,10]"},
		{"1/((Dmin/2).(exp(k.(T-Tp))+exp(-(T-Tp)*lamb)))","1/((Dmin/2) * (exp(k*(T-Tp)) + exp(-(T-Tp)*lamb)))","k=1[-100,100]|Dmin=1[-100,100]|Tp=1[-100,100]|lamb=1[0.01,10]"},
		{"a.(1-exp(-(T-Tl)/B)).(1-exp(-(Th-T)/B))","a*(1-exp(-(T-Tl)/B))*(1-exp(-(Th-T)/B))","a=1[-100,100]|Tl=1[-100,100]|Tl=1[-100,100]|Th=1[0.01,10]|B=1[0.01,10]"},
		{"exp(a.(1-exp(-(T-Tl)/Bl)).(1-exp(-(Th-T)/Bh)))","exp(a*(1-exp(-(T-Tl)/Bl))*(1-exp(-(Th-T)/Bh)))","a=1[-100,100]|Tl=1[-100,100]|Tl=1[-100,100]|Th=1[0.01,10]|Bl=1[0.01,10]|Bh=1[0.01,10]"},
	};

	bool CMortalityEquation::IsParamValid(CMortalityEquation::TMortalityEquation model, const std::vector<double>& P)
	{
		bool bValid = true;
		/*switch (model)
		{
		case Allahyari:bValid = P[P1] < P[P2]; break;
		case Analytis_1977: bValid = P[P3] < P[P4]; break;
		case Bieri_1983:bValid = P[P2] < P[P3]; break;
		case Briere1_1999:bValid = P[P1] < P[P2]; break;
		case Briere2_1999:bValid = P[P2] < P[P3]; break;
		case HilbertLogan_1983:bValid = P[P2] < P[P3]; break;
		case Kontodimas_2004:bValid = P[P1] < P[P2]; break;
		case Ratkowsky_1983:bValid = P[P2] < P[P3]; break;
		case Regniere_1987:  bValid = P[P4] < P[P5]; break;
		case Regniere_2012:bValid = P[P2] < P[P4]; break;
		case Shi_2011: bValid = P[P3] < P[P4]; break;
		case HueyStevenson_1979: bValid = P[P2] < P[P3]; break;
		case Shi_beta_2016:bValid = (P[P2] < P[P1]) && (P[P1] < P[P3]); break;
		case Wang_1982:bValid = (P[P3] < P[P5]) && (P[P5] < P[P4]); break;
		case Wangengel_1998:bValid = P[P1] < P[P2]; break;
		case Yin_beta_1995:bValid = P[P3] < P[P4]; break;
		}
*/
		return bValid;
	}

	CSAParameterVector CMortalityEquation::GetParameters(TMortalityEquation model)
	{
		CSAParameterVector p;

		StringVector tmp(CMortalityEquation::EQUATION[model][EQ_PARAM], "=[,]|");
		ASSERT(tmp.size() % 4 == 0);
		for (size_t i = 0; i < tmp.size() / 4; i++)
		{
			p.push_back(CSAParameter(tmp[i * 4], stod(tmp[i * 4 + 1]), stod(tmp[i * 4 + 2]), stod(tmp[i * 4 + 3])));
		}

		return p;
	}

	CSAParameterVector CMortalityEquation::GetParameters(TMortalityEquation model, const vector<double>& X)
	{
		CSAParameterVector p = GetParameters(model);
		ASSERT(X.size() >= p.size());
		for (size_t i = 0; i < p.size(); i++)
		{
			p[i].m_initialValue = X[i];
		}

		return p;
	}

















	double CMortalityEquation::GetMortality(CMortalityEquation::TMortalityEquation modelm, const std::vector<double>& P, double T)
	{
		double m = 0;

		if (modelm == 1)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = a * T*T + b * T + c;
			//if (proc == "mortal") frm < -"m(T) = aT²+bT+c" else frm < -"f(T) = aT²+bT+c"
		}
		else if (modelm == 2)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = a * sqrt(T) + b * T + c;
			//	if (proc == "mortal") frm < -"m(T) = a·sqrt(T)+bT+c" else frm < -"f(T) = a·sqrt(T)+bT+c"
		}
		else if (modelm == 3)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = a * (1 / sqrt(T)) + b * T + c;
			//if (proc == "mortal") frm < -"m(T) = a/sqrt(T)+bT+c" else frm < -"f(T) = a/sqrt(T)+bT+c"
		}
		else if (modelm == 4)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = a * (1.0 / (T *T)) + b * T + c;
			//if (proc == "mortal") frm < -"m(T) = a/T²+bT+c" else frm < -"f(T) = a/T²+bT+c"
		}
		else if (modelm == 5)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = a * (1 / T) + b * T + c;
			//if (proc == "mortal") frm < -"m(T) = a/T+bT+c" else frm < -"f(T) = a/T+bT+c"
		}
		else if (modelm == 6)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			m = b * T + a * log(T) + c;
			//	if (proc == "mortal") frm < -"m(T) = a·ln(T)+bT+c" else frm < -"f(T) = a·ln(T)+bT+c"
		}
		else if (modelm == 7)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double d = P[P3];

			m = 1 / (1 + a * exp(-b * pow((T - c) / d, 2.0)));
			//	if (proc == "mortal") frm < -"m(T) = 1/(1+a·exp(-b((x-c)/d)²))" else frm < -"f(T) = 1/(1+a·exp(-b((x-c)/d)²))"
		}
		else if (modelm == 8)
		{
			double a = P[P0];
			double b = P[P1];
			double c = P[P2];
			double xo = P[P3];

			m = a * exp(-b * pow((T - xo) / c, 2.0));
			//if (proc == "mortal") frm < -"m(T) = a·exp(-b((x-xo)/c)²)" else frm < -"f(T) = a·exp(-b((x-xo)/c)²)"
		}
		else if (modelm == 9)
		{
			double a = P[P0];
			double b = P[P1];
			double y0 = P[P2];
			double x0 = P[P3];

			m = y0 + a * exp(-0.5 * pow((T - x0) / b, 2.0));
			//if (proc == "mortal") frm < -"m(T) = y0+a·exp(-0.5((x-x0)/b)²)" else frm < -"f(T) = y0+a·exp(-0.5((x-x0)/b)²)"
		}
		else if (modelm == 10)
		{
			double a = P[P0];
			double b = P[P1];
			double y0 = P[P2];
			double x0 = P[P3];

			m = y0 + a * exp(-0.5 * pow(log(abs(T / x0)) / b, 2.0));
			//if (proc == "mortal") frm < -"m(T) = y0+a·exp(-0.5(log(abs(x/x0))/b)²)" else frm < -"f(T) = y0+a·exp(-0.5(log(abs(x/x0))/b)²)"
		}
		else if (modelm == 11)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double d = P[P3];

			m = b1 + b2 * T + b3 * pow(T, d);
			//if (proc == "mortal") frm < -"m(T) = b1+b2.x+b3.x^d " else frm < -"f(T) = b1+b2.x+b3.x^d"
		}
		else if (modelm == 12)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			m = exp(b1 + b2 * T + b3 * T*T);
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.x²) " else frm < -"f(T) = exp(b1+b2.x+b3.x²)"
		}
		else if (modelm == 13)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];
			m = 1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * T*T)));
			//	if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.x²)))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.x²)))"
		}
		else if (modelm == 14)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			m = exp(b1 + b2 * T + b3 * sqrt(T));
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.(x)^0.5)" else frm < -"f(T) = exp(b1+b2.x+b3.(x)^0.5)"
		}
		else if (modelm == 15)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			m = exp(1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * sqrt(T)))));
			//if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(x)^0.5)))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(x)^0.5)))"
		}
		else if (modelm == 16)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];

			m = exp(b1 + b2 * T + b3 * (1 / sqrt(T)));
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.(1/(x)^0.5))" else frm < -"f(T) = exp(b1+b2.x+b3.(1/(x)^0.5))"
		}
		else if (modelm == 17)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			m = 1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * (1 / sqrt(T)))));
			//if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(1/(x)^0.5))))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(1/(x)^0.5))))"
		}
		else if (modelm == 18)
		{

			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];

			m = exp(b1 + b2 * T + b3 * (1 / T));
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.(1/x))" else frm < -"f(T) = exp(b1+b2.x+b3.(1/x))"
		}
		else if (modelm == 19)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			m = 1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * (1 / T))));
			//if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(1/x))))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.(1/x))))"
		}
		else if (modelm == 20)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double d = P[P3];

			m = exp(b1 + b2 * T + b3 * pow(T, d));
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.x^d)" else frm < -"f(T) = exp(b1+b2.x+b3.x^d)"
		}
		else if (modelm == 21)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];
			double d = P[P3];

			m = 1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * pow(T, d))));
			//if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.x^d)))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.x^d)))"
		}
		else if (modelm == 22)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			m = exp(b1 + b2 * T + b3 * log(T));
			//if (proc == "mortal") frm < -"m(T) = exp(b1+b2.x+b3.ln(x))" else frm < -"f(T) = exp(b1+b2.x+b3.ln(x))"
		}
		else if (modelm == 23)
		{
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			m = 1 - (b4 / (1 + b5 * exp(b1 + b2 * T + b3 * log(T))));
			//if (proc == "mortal") frm < -"m(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.ln(x))))" else frm < -"f(T) = 1-(b4/(1+b5.exp(b1+b2.x+b3.ln(x))))"
		}
		else if (modelm == 24)
		{
			double rm = P[P0];
			double Topt = P[P1];
			double Troh = P[P2];

			m = 1 - rm * exp((-0.5)*pow(-(T - Topt) / Troh, 2.0));
			//if (proc == "mortal") frm < -"m(T) = 1-rm.exp((-0.5).(-(x-Topt)/Troh)^2)" else frm < -"f(T) = 1-rm.exp((-0.5).(-(x-Topt)/Troh)^2)"
		}
		else if (modelm == 25)
		{
			double rm = P[P0];
			double Topt = P[P1];
			double Troh = P[P2];

			m = 1 - rm * exp((-0.5)*pow(-(log(T) - log(Topt)) / Troh, 2.0));
			//if (proc == "mortal") frm < -"m(T) = 1-rm.exp((-0.5).(-(log(x)-log(Topt))/Troh)^2)" else frm < -"f(T) = 1-rm.exp((-0.5).(-(log(x)-log(Topt))/Troh)^2)"
		}
		else if (modelm == 26)
		{
			double Topt = P[P0];
			double B = P[P1];
			double H = P[P2];

			m = 1 - 1 / (exp((1 + exp(-(T - Topt) / B))*(1 + exp(-(Topt - T) / B))*H));
			//if (proc == "mortal") frm < -"m(T) = 1 - 1/(exp((1+exp(-(x-Topt)/B)).(1+exp(-(Topt-x)/B)).H))" else frm < -"f(T) = 1 - 1/(exp((1+exp(-(x-Topt)/B)).(1+exp((Topt-x)/B)).H))"
		}
		else if (modelm == 27)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double B = P[P2];
			double H = P[P3];

			m = 1 - 1 / (exp((1 + exp(-(T - Tl) / B))*(1 + exp(-(Th - T) / B))*H));
			//if (proc == "mortal") frm < -"m(T) = 1 - 1/(exp((1+exp(-(x-Tl)/B)).(1+exp(-(Th-x)/B)).H))" else frm < -"f(T) = 1 - 1/(exp((1+exp(-(x-Tl)/B)).(1+exp(-(Th-x)/B)).H))"
		}
		else if (modelm == 28)
		{
			double Topt = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];
			m = 1 - 1 / (exp((1 + exp(-(T - Topt) / Bl))*(1 + exp(-(Topt - T) / Bh))*H));
			//if (proc == "mortal") frm < -"m(T) = 1 - 1/(exp((1+exp(-(x-Topt)/Bl)).(1+exp(-(Topt-x)/Bh)).H))" else frm < -"f(T) = 1 - 1/(exp((1+exp(-(x-Topt)/Bl)).(1+exp(-(Topt-x)/Bh)).H))"
		}
		else if (modelm == 29)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double Bl = P[P2];
			double Bh = P[P3];
			double H = P[P4];

			m = 1 - 1 / (exp((1 + exp(-(T - Tl) / Bl))*(1 + exp(-(Th - T) / Bh))*H));
			//if (proc == "mortal") frm < -"m(T) = 1 - 1/(exp((1+exp(-(x-Tl)/Bl)).(1+exp(-(Th-x)/Bh)).H))" else frm < -"f(T) = 1 - 1/(exp((1+exp(-(x-Tl)/Bl)).(1+exp(-(Th-x)/Bh)).H))"
		}
		else if (modelm == 30)
		{
			double Topt = P[P0];
			double B = P[P1];
			double H = P[P2];

			m = 1 - H / (exp(1 + exp(-(T - Topt) / B))*(1 + exp(-(Topt - T) / B)));
			//if (proc == "mortal") frm < -"m(T) = 1 - H/(exp(1+exp(-(x-Topt)/B)).(1+exp(-(Topt-x)/B)))" else frm < -"f(T) = 1 - H/(exp(1+exp(-(x-Topt)/B)).(1+exp(-(Topt-x)/B)))"
		}
		else if (modelm == 31)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double B = P[P2];
			double H = P[P3];

			m = 1 - H / (exp(1 + exp(-(T - Tl) / B))*(1 + exp(-(Th - T) / B)));
			//if (proc == "mortal") frm < -"m(T) = 1 - H/(exp(1+exp(-(x-Tl)/B))*(1+exp(-(Th-x)/B)))" else frm < -"f(T) = 1 - H/(exp(1+exp(-(x-Tl)/B))*(1+exp(-(Th-x)/B)))"
		}
		else if (modelm == 32)
		{
			double Topt = P[P0];
			double Bl = P[P1];
			double Bh = P[P2];
			double H = P[P3];

			m = 1 - H / (exp(1 + exp(-(T - Topt) / Bl))*(1 + exp(-(Topt - T) / Bh)));
			//if (proc == "mortal") frm < -"m(T) = 1 - H/(exp(1+exp(-(x-Topt)/Bl)).(1+exp(-(Topt-x)/Bh)))" else frm < -"f(T) = 1 - H/(exp(1+exp(-(x-Topt)/Bl)).(1+exp(-(Topt-x)/Bh))"
		}
		else if (modelm == 33)
		{
			double Tl = P[P0];
			double Th = P[P1];
			double Bl = P[P2];
			double Bh = P[P3];
			double H = P[P4];

			m = 1 - H / (exp(1 + exp(-(T - Tl) / Bl))*(1 + exp(-(Th - T) / Bh)));
			//if (proc == "mortal") frm < -"m(T) = 1 - H/(exp(1+exp(-(x-Tl)/Bl)).(1+exp(-(Th-x)/Bh)))" else frm < -"f(T) = 1 - H/(exp(1+exp(-(x-Tl)/Bl)).(1+exp(-(Th-x)/Bh)))"
		}
		else if (modelm == 34)
		{
			double Hl = P[P0];
			double Tl = P[P1];
			double Hh = P[P2];
			double Th = P[P3];
			double Bm = P[P4];

			m = Bm + ((1 - 1 / (1 + exp((Hl / 1.987)*((1 / Tl) - (1 / (T + 273.15)))) + exp((Hh / 1.987)*((1 / Th) - (1 / (T + 273.15))))))*(1 - Bm));// #DM: se agrego 2 parentesis "Bm + (...)*";
			//	if (proc == "mortal") frm < -"m(T) = Bm + ((1-1/(1+exp((Hl/1.987).((1/Tl)-(1/x)))+exp((Hh/1.987)*((1/Th)-(1/x))))).(1-Bm))" else frm < -"f(T) = Bm + ((1-1/(1+exp((Hl/1.987).((1/Tl)-(1/x)))+exp((Hh/1.987)*((1/Th)-(1/x))))).(1-Bm))" #DM : se agrego 2 parentesis "Bm + (...)*"
		}
		else if (modelm == 35)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];

			m = (1 - exp(-(exp(a1 + b1 * T)))) + (1 - exp(-(exp(a2 + b2 * T))));
			//if (proc == "mortal") frm < -"m(T) = (1 - exp(-(exp(a1+b1.T)))) + (1 - exp(-(exp(a2+b2.T))))" else frm < -"f(T) = (1 - exp(-(exp(a1+b1.T)))) + (1 - exp(-(exp(a2+b2.T))))"
		}
		else if (modelm == 36)
		{
			double w = P[P0];

			m = 1 / (w - T);
			//if (proc == "mortal") frm < -"m(T) = (w-x)^(-1)" else frm < -"f(T) = (w-x)^(-1)"
		}
		else if (modelm == 37)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];

			m = a1 * exp(b1*T) + a2 * exp(b2*T);
			//if (proc == "mortal") frm < -"m(T) = a1.exp(b1.x) + a2.exp(b2.x)" else frm < -"f(T) = a1.exp(b1.x) + a2.exp(b2.x)"
		}
		else if (modelm == 38)
		{
			double a1 = P[P0];
			double b1 = P[P1];
			double a2 = P[P2];
			double b2 = P[P3];
			double c1 = P[P4];

			m = a1 * exp(b1*T) + c1;
			//if (proc == "mortal") frm < -"m(T) = a1.exp(b1.x) + a2.exp(b2.x) + c1" else frm < -"f(T) = a1.exp(b1.x) + a2.exp(b2.x) + c1"
		}
		else if (modelm == 39)
		{
			double a = P[P0];
			double b = P[P1];
			double nn = P[P2];

			m = a * pow(abs(T - b), nn);
			//if (proc == "mortal") frm < -"m(T) = a.(abs(x-b))^nn" else frm < -"f(T) = a.(abs(x-b))^nn"
		}
		else if (modelm == 40)
		{
			double a = P[P0];
			double To = P[P1];
			double Tl = P[P2];
			double d = P[P3];

			m = a * T*(T - To)*pow(Tl - T, 1.0 / d);
			//if (proc == "mortal") frm < -"m(T) = a.x.(x-To).(Tl-x)^(1/d)" else frm < -"f(T) = a.x.(x-To).(Tl-x)^(1/d)"
		}
		else if (modelm == 41)
		{
			double a = P[P0];
			double To = P[P1];
			double Tl = P[P2];
			double d = P[P3];

			m = exp(a*T*(T - To)*pow(Tl - T, 1 / d));
			//if (proc == "mortal") frm < -"m(T) = exp(a.x.(x-To).(Tl-x)^(1/d))" else frm < -"f(T) = exp(a.x.(x-To).(Tl-x)^(1/d))"
		}
		else if (modelm == 42)
		{
			double a = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];
			double n = P[P3];
			m = a * (pow(T - Tmin, n)*pow(Tmax - T, m));
			//if (proc == "mortal") frm < -"m(T) = a.((x-Tmin)^n).(Tmax-x)^m" else frm < -"f(T) = a.((x-Tmin)^n).(Tmax-x)^m"
		}
		else if (modelm == 43)
		{
			double Dmin = P[P0];
			double k = P[P1];
			double Tp = P[P2];
			double lamb = P[P3];
			m = 1 / ((Dmin / 2) * (exp(k*(T - Tp)) + exp(-(T - Tp)*lamb)));
			//		if (proc == "mortal") frm < -"m(T) = 1/((Dmin/2) . (exp(k.(x-Tp)) + exp(-(x-Tp).lamb)))" else frm < -"f(T) = 1/((Dmin/2) . (exp(k.(x-Tp)) + exp(-(x-Tp).lamb)))"
		}
		else if (modelm == 44)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double B = P[P3];

			m = a * (1 - exp(-(T - Tl) / B))*(1 - exp(-(Th - T) / B));
			//if (proc == "mortal") frm < -"m(T) = a.(1-exp(-(x-Tl)/B)).(1-exp(-(Th-x)/B))" else frm < -"f(T) = a.(1-exp(-(x-Tl)/B)).(1-exp(-(Th-x)/B))"
		}
		else if (modelm == 45)
		{
			double a = P[P0];
			double Tl = P[P1];
			double Th = P[P2];
			double Bl = P[P3];
			double Bh = P[P4];
			
			m = exp(a*(1 - exp(-(T - Tl) / Bl))*(1 - exp(-(Th - T) / Bh)));
				//if (proc == "mortal") frm < -"m(T) = exp(a.(1-exp(-(x-Tl)/Bl)).(1-exp(-(Th-x)/Bh)))" else frm < -"f(T) = exp(a.(1-exp(-(x-Tl)/Bl)).(1-exp(-(Th-x)/Bh)))"
		}

		return m;
	}


}









