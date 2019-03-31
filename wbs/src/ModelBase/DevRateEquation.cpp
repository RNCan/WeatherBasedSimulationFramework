#include "DevRateEquation.h"
#include <functional>

using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

//Analytis	Analytis, S. (1977) Uber die Relation zwischen biologischer Entwicklung und Temperatur bei phytopathogenen Pilzen. Journal of Phytopathology 90(1): 64-76.
//Bayoh	Bayoh, M.N., Lindsay, S.W. (2003) Effect of temperature on the development of the aquatic stages of Anopheles gambiae sensu stricto (Diptera: Culicidae). Bulletin of entomological research 93(5): 375-81.
//Beta2	Shi, P. J., Chen, L., Hui, C., and Grissino-Mayer, H. D. (2016). Capture the time when plants reach their maximum body size by using the beta sigmoid growth equation. Ecological Modelling, 320, 177-181.
//Beta	Yin, X., Kropff, M.J., McLaren, G., and Visperas, R.M. (1995) A nonlinear model for crop development as a function of temperature. Agricultural and Forest Meteorology 77(1): 1-16.
//Bieri1_83	Bieri, M., Baumgartner, J., Bianchi, G., Delucchi, V., Arx, R. von. (1983) Development and fecundity of pea aphid (Acyrthosiphon pisum Harris) as affected by constant temperatures and by pea varieties. Mitteilungen der Schweizerischen Entomologischen Gesellschaft, 56, 163-171.
//Briere-1	Briere, J.F., Pracros, P., le Roux, A.Y. and Pierre, S. (1999) A novel rate model of temperature-dependent development for arthropods. Environmental Entomology, 28, 22-29.
//Briere-2	Briere, J.F., Pracros, P., le Roux, A.Y. and Pierre, S. (1999) A novel rate model of temperature-dependent development for arthropods. Environmental Entomology, 28, 22-29.
//Linear	Campbell, A., B. Frazer, N. Gilbert, A. Gutierrez, and M. Mackauer. (1974). Temperature requirements of some aphids and their parasites. Journal of applied ecology, 431-438.
//Simplified beta type	Damos, P.T., and Savopoulou-Soultani, M. (2008). Temperature-dependent bionomics and modeling of Anarsia lineatella (Lepidoptera: Gelechiidae) in the laboratory. Journal of economic entomology, 101(5), 1557-1567.
//Inverse second-order polynomial	Damos, P., and Savopoulou-Soultani, M. (2011) Temperature-driven models for insect development and vital thermal requirements. Psyche: A Journal of Entomology, 2012.
//Logistic	Davidson, J. (1944). On the relationship between temperature and rate of development of insects at constant temperatures. The Journal of Animal Ecology:26-38.
//Third-order polynomial	Harcourt, D. and Yee, J. (1982) Polynomial algorithm for predicting the duration of insect life stages. Environmental Entomology, 11, 581-584.
//Holling type III	Hilbert, DW, y JA Logan (1983) Empirical model of nymphal development for the migratory grasshopper, Melanoplus sanguinipes (Orthoptera: Acrididae). Environmental Entomology 12(1): 1-5.
//Janisch (Analytis modification)	Janisch, E. (1932) The influence of temperature on the life-history of insects. Transactions of the Royal Entomological Society of London 80(2): 137-68.\nAnalytis, S. (1977) Uber die Relation zwischen biologischer Entwicklung und Temperatur bei phytopathogenen Pilzen. Journal of Phytopathology 90(1): 64-76.
//Equation 16	Kontodimas, D.C., Eliopoulos, P.A., Stathas, G.J. and Economou, L.P. (2004) Comparative temperature-dependent development of Nephus includens (Kirsch) and Nephus bisignatus (Boheman)(Coleoptera: Coccinellidae) preying on Planococcus citri (Risso)(Homoptera: Pseudococcidae): evaluation of a linear and various nonlinear models using specific criteria. Environmental Entomology 33(1): 1-11.
//Lactin-1	Lactin, Derek J, NJ Holliday, DL Johnson, y R Craigen (1995) Improved rate model of temperature-dependent development by arthropods. Environmental Entomology 24(1): 68-75.
//Lactin-2	Lactin, Derek J, NJ Holliday, DL Johnson, y R Craigen (1995) Improved rate model of temperature-dependent development by arthropods. Environmental Entomology 24(1): 68-75.
//Lamb	Lamb, R. J., Gerber, G. H., & Atkinson, G. F. (1984). Comparison of developmental rate curves applied to egg hatching data of Entomoscelis americana Brown (Coleoptera: Chrysomelidae). Environmental entomology, 13(3), 868-872. \nLamb, RJ. (1992) Developmental rate of Acyrthosiphon pisum (Homoptera: Aphididae) at low temperatures: implications for estimating rate parameters for insects. Environmental Entomology 21(1): 10-19.
//Logan-10	Logan, J. A., Wollkind, D. J., Hoyt, S. C., and Tanigoshi, L. K. (1976). An analytic model for description of temperature dependent rate phenomena in arthropods. Environmental Entomology, 5(6), 1133-1140.
//Logan-6	Logan, J. A., Wollkind, D. J., Hoyt, S. C., and Tanigoshi, L. K. (1976). An analytic model for description of temperature dependent rate phenomena in arthropods. Environmental Entomology, 5(6), 1133-1140.
//Performance-2	Shi, P., Ge, F., Sun, Y., and Chen, C. (2011) A simple model for describing the effect of temperature on insect developmental rate. Journal of Asia-Pacific Entomology 14(1): 15-20.
//Second-order polynomial	-
//Forth-order polynomial	-
//Ratkowsky	Ratkowsky, D.A., Olley, J., McMeekin, T.A., and Ball, A. (1982) Relationship between temperature and growth rate of bacterial cultures. Journal of Bacteriology 149(1): 1-5.
//SR-Ratkowsky	Ratkowsky, D. A., Lowry, R. K., McMeekin, T. A., Stokes, A. N., & Chandler, R. E. (1983). Model for bacterial culture growth rate throughout the entire biokinetic temperature range. Journal of bacteriology, 154(3), 1222-1226.
//Regniere	Regniere, J., Powell, J., Bentz, B., and Nealis, V. (2012) Effects of temperature on development, survival and reproduction of insects: experimental design, data analysis and modeling. Journal of Insect Physiology 58(5): 634-47.
//Root square	Ratkowsky, D.A., Olley, J., McMeekin, T.A., and Ball, A. (1982) Relationship between temperature and growth rate of bacterial cultures. Journal of Bacteriology 149(1): 1-5.
//Schoolfield	Schoolfield, R., Sharpe, P. & Magnuson, C. (1981) Non-linear regression of biological temperature-dependent rate models based on absolute reaction-rate theory. Journal of theoretical biology, 88, 719-731.
//Schoolfield High	Schoolfield, R., Sharpe, P. & Magnuson, C. (1981) Non-linear regression of biological temperature-dependent rate models based on absolute reaction-rate theory. Journal of theoretical biology, 88, 719-731.
//Schoolfield Low	Schoolfield, R., Sharpe, P. & Magnuson, C. (1981) Non-linear regression of biological temperature-dependent rate models based on absolute reaction-rate theory. Journal of theoretical biology, 88, 719-731.
//Sharpe and DeMichele	Sharpe, P.J. & DeMichele, D.W. (1977) Reaction kinetics of poikilotherm development. Journal of Theoretical Biology, 64, 649-670.
//Shi	Shi, P., Ge, F., Sun, Y., and Chen, C. (2011) A simple model for describing the effect of temperature on insect developmental rate. Journal of Asia-Pacific Entomology 14(1): 15-20. doi:10.1016/j.aspen.2010.11.008.
//Logistic	Stinner, R., Gutierrez, A. & Butler, G. (1974) An algorithm for temperature-dependent growth rate simulation. The Canadian Entomologist, 106, 519-524.
//Gauss	Taylor, F. (1981) Ecology and evolution of physiological time in insects. American Naturalist, 1-23. \nLamb, RJ. (1992) Developmental rate of Acyrthosiphon pisum (Homoptera: Aphididae) at low temperatures: implications for estimating rate parameters for insects. Environmental Entomology 21(1): 10-19.
//wagner_88	Hagstrum, D.W., Milliken, G.A. (1988) Quantitative analysis of temperature, moisture, and diet factors affecting insect development. Annals of the Entomological Society of America 81(4): 539-46.
//Wang	Wang, R., Lan, Z. and Ding, Y. (1982) Studies on mathematical models of the relationship between insect development and temperature. Acta Ecol. Sin, 2, 47-57.
//Wang Engel	Wang, E., and Engel, T. (1998) Simulation of phenological development of wheat crops. Agricultural systems 58(1): 1-24.


namespace WBSF
{
	const char* CDevRateEquation::EQUATION[NB_EQUATIONS][3] =
	{
		{"Allahyari","p*((pmax(0,T-Tmin)/(Tmax-Tmin))^n)*(1-(pmax(0,T-Tmin)/(Tmax-Tmin))^m)","p=0.02[1E-5,1E4]|Tmin=1[0,50]|Tmax=40[0,100]|n=1[0.1,4]|m=1[0.1,4]"},
		{"Analytis_1977","aa*pmax(0,T-Tmin)^bb*pmax(0,Tmax-T)^cc","aa=5E-4[1.0E-10,0.1]|bb=0.5[1E-5,4]|cc=0.35[0.001,4]|Tmin=2.3[-50,50]|Tmax=35[0,50]"},
		{"Angilletta_2006","k1*exp(-0.5*(abs(T-To)/k2)^k3)","k1=1[-1E5,1E5]|k2=1[-1E5,1E5]|k3=3.8[1,4]|To=27[0,50]"},
		{"Bieri_1983","aa*(T-Tmin)-(bb*exp(T-Tmax))","aa=0.015[1E-5,10]|bb=3.6[1E-6,100]|Tmin=10[0,15]|Tmax=42[0,50]"},
		{"Briere1_1999","aa*T*pmax(0,T-Tmin)*pmax(0,Tmax-T)^(1/2)","aa=1E-4[1E-7,1]|Tmin=9.1[0,50]|Tmax=37.3[0,50]"},
		{"Briere2_1999","aa*T*pmax(0,T-Tmin)*pmax(0,Tmax-T)^(1/bb)","aa=1E-4[1E-7,1]|bb=2.8[1,10]|Tmin=7.0[0,50]|Tmax=35[0,50]"},
		{"Damos_2008","aa*(bb-T/10)*(T/10)^cc","aa=7.5E-4[1E-10,0.1]|bb=3.8[0.01,100]|cc=5.0[0.01,100]"},
		{"Damos_2011","aa/(1+bb*T+cc*T^2)","aa=0.01[-1E4,1E4]|bb=-0.1[-1E4,1E4]|cc=0.01[-1E4,1E4]"},
		{"Deva&Higgis","b1*10^(-((((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))+exp(b4*((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))))/((1+b4)/(1+1.5*b4+0.39*b4^2)))^2)*(1-b5+b5*((((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))+exp(b4*((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))))/((1+b4)/(1+1.5*b4+0.39*b4^2)))^2)","b1=0.3[0.0001,10]|b2=0[-50,50]|b3=25[0,50]|b4=2[0.0001,10]|b5=0.8[0.0001,10]"},
		{"Exponential","b1*exp(b2*T)","b1=0.2[0.0001,1000]|b2=-5[-1000,1000]"},
		{"Hilber&logan_1983","phi*((pmax(0,T-Tb)^2/(pmax(0.001,T-Tb)^2+aa^2))-exp(-(Tmax-(T-Tb))/deltaT))","phi=0.005[1E-5,10]|aa=10[0.001,1000]|Tb=5[0.01,50]|Tmax=35[1,50]|deltaT=6[0.001,100]"},
		{"Hilber&loganIII","phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))","phi=6.6[0.0001,1000]|d=144[0.001,1000]|Tmax=46[0,50]|deltaT=3[1,100]"},
		{"Huey&Stevenson_1979","cc*(T-Tmin)*(1-exp(k*(T-Tmax)))","cc=0.002[1E-10,1]|k=42423[0,1E5]|Tmin=8[0,30]|Tmax=43[0,100]"},
		{"Janisch1_1932","2/(Dmin*(exp(K*(T-Topt))+exp((-K)*(T-Topt))))","Dmin=28[-50,50]|Topt=33[-50,50]|K=0.2[0.0001,10]"},
		{"Janisch2","2*cc/(aa^(T-Tm)+bb^(Tm-T))","cc=0.2[1E-5,1E5]|aa=0.5[1E-5,10]|bb=1.0[1E-5,10]|Tm=36[-50,50]"},
		{"Kontodimas_2004","aa*(pmax(0,T-Tmin)^2)*(Tmax-T)","aa=1.6e-5,1e-7,1]|Tmin=8[1,50]|Tmax=42[1,50]"},
		{"Lactin1_1995","exp(aa*T)-exp(aa*Tmax-(Tmax-T)/deltaT)","aa=-0.016[-1,1]|Tmax=40[0,50]|deltaT=8[1,100]"},
		{"Lactin2_1995","exp(aa*T)-exp(aa*Tmax-(Tmax-T)/deltaT)+bb","aa=-0.03[-10,10]|bb=0[-100,100]|Tmax=1[-100,100]|deltaT=100[1,1000]"},
		{"Lamb_1992","c(Rm*exp(-1/2*((T[T<Tmax]-Tmax)/T0)^2),Rm*exp(-1/2*((T[T>=Tmax]-Tmax)/T1)^2))","Rm=0.004[0.0001,10]|Tmax=35[0,50]|T0=11[0.1,50]|T1=4[0.1,50]"},
		{"Logan6_1976","phi*(exp(bb*T)-exp(bb*Tmax-(Tmax-T)/deltaT))","phi=0.012[0.0001,1]|bb=0.14[0.0001,1]|Tmax=33[20,50]|deltaT=5[1,10]"},
		{"Logan10_1976","phi*(1/(1+cc*exp(-bb*T))-exp(-((Tmax-T)/deltaT)))","phi=0.03[0.0001,1]|bb=0.23[0,100]|cc=110[10,1000]|Tmax=33[20,50]|deltaT=5[1,10]"},
		{"LoganExponential","phi*exp(b*(T-Tb))","phi=0.02[0.0001,1]|b=0.2[0.0001,1]|Tb=40[0,100]"},
		{"LoganTb","phi*exp(b*(T-Tb)-exp(b*(T-Tb)/deltaT))","phi=0.02[1E-4,1]|b=0.2[-1E4,1]|Tb=30[0,100]|deltaT=2[1,100]"},
		{"Poly1","a0+a1*T","a0=0[-1,0.1]|a1=0.02[1E-5,0.1]"},
		{"Poly2","a0+a1*T+a2*T^2","a0=0[-1E5,1E5]|a1=-0.003[-1E4,1E4]|a2=0.0004[-1E3,1E3]"},
		{"Poly3","a0+a1*T+a2*T^2+a3*T^3","a0=0.2[-1E5,1E5]|a1=0.01[-1E4,1E4]|a2=0[-1E3,1E3]|a3=0[-1E2,1E2]"},
		{"Poly4","a0+a1*T+a2*T^2+a3*T^3+a4*T^4","a0=0.2[-1E5,1E5]|a1=0.02[-1E4,1E4]|a2=0[-1E3,1E3]|a3=0[-1E2,1E2]|a4=0[-10,10]"},
		{"Pradham","R*exp(-1/2*((T-Tm)/To))^2","R=4.2[1E-5,10]|Tm=84[1,200]|To=-18[-100,0]"},
		{"Ratkowsky_1982(Square)","b*(T-Tb)^2","b=0.0002[1E-5,1000]|Tb=10[0,50]"},
		{"Ratkowsky_1983","(a*(T-Tmin)*(1-exp((b*(T-Tmax)))))^2","a=0.002[1E-5,10]|b=0.02[1E-5,1]|Tmin=5[0,50]|Tmax=35[0,50]"},
		{"Regniere_1987","b1*((1/(1+exp(b2-b3*(T-Tb)/(Tm-Tb))))-(exp(((T-Tb)/(Tm-Tb)-1)/b4)))","b1=0.2[1E-6,1]|b2=2[1E-6,10]|b3=6[1E-6,10]|b4=0.15[1E-6,1]|Tb=15[-10,50]|Tm=35[0,50]"},
		{"Regniere_2012","phi*(exp(bb*pmax(0,T-Tb))-(pmax(0,Tm-T)/(Tm-Tb))*exp(-bb*pmax(0,T-Tb)/deltab)-(pmax(0,T-Tb)/(Tm-Tb))*exp(bb*(Tm-Tb)-pmax(0,Tm-T)/deltam))","phi=0.01[1E-5,1]|bb=-0.01[-10,10]|Tb=7[0,50]|Tm=35[0,50]|deltab=4[1,10]|deltam=5[1,10]"},
		{"SchoolfieldHigh_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(dd/1.987*(1/ee-1/(T+273.16))))","p25=0.01[1E-4,10]|aa=0.2[-1E5,1E5]|dd=-0.1[-1E5,1E5]|ee=280[250,350]"},
		{"SchoolfieldLow_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(bb/1.987*(1/cc-1/(T+273.16))))","p25=0.01[1E-4,10]|aa=0.2[-1E5,1E5]|bb=-0.1[-1E5,1E5]|cc=280[250,350]"},
		{"Schoolfield_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(bb/1.987*(1/cc-1/(T+273.16)))+exp(dd/1.987*(1/ee-1/(T+273.16))))","p25=0.01[1E-4,1E4]|aa=0.02[-1E5,1E5]|bb=-0.1[-1E5,1E5]|cc=300[250,350]|dd=0.2[-1E5,1E5]|ee=300[200,400]"},
		{"Sharpe&DeMichele3","(p*((T+273.16)/(To+273.16))*exp((Ha/1.987)*((1/(To+273.16))-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/(Tl+273.16))-(1/(T+273.16))))+exp((Hh/1.987)*((1/(Th+273.16))-(1/(T+273.16)))))","p=1E-4[1E-5,1E5]|To=0.2[0.001,50]|Ha=0.2[-1000,1E5]|Hl=2[0.001,50]|Tl=5[0.001,50]|Hh=0.15[0.001,1E5]|Th=4[1,50]"},
		{"Sharpe&DeMichele_1977","((T+273.16)*exp((aa-bb/(T+273.16))/1.987))/(1+exp((cc-dd/(T+273.16))/1.987)+exp((ff-gg/(T+273.16))/1.987))","aa=-11[-1E3,0]|bb=1E4[1E-5,1E5]|cc=-243[-1E3,0]|dd= 69[1E-5,1E5]|ff=-678[-1E3,0]|gg=894[1E-5,1E5]"},
		{"Shi_2011","cc*(1-exp(-k1*(T-T1)))*(1-exp(k2*(T-T2)))","cc=0.2[0.001,1000]|k1=0.2[0.001,1000]|k2=0.2[0.001,1000]|T1=10[0,50]|T2=30[0,50]"},
		{"Shi_beta_2016","rm*(T2-T)/(T2-Tm)*((T-T1)/(Tm-T1))^((Tm-T1)/(T2-Tm))","rm=0.02[0.001,10]|Tm=15[0,50]|T1=5[0,50]|T2=30[0,50]"},
		{"St-Amant_2019","aa*exp(bb*(T-T1)+1/(cc*pmin(-0.001,T-T2)))","aa=0.5[1E-4,1]|bb=0.5[1E-4,1]|cc=0.5[1E-4,1]|T1=35[0,50]|T2=35[0,100]"},
		//{"Stinner1","Rm*(1+exp(k1+k2*To))/(1+exp(k1+k2*T))","Rm=0.2[1E-5,1]|k1=3.7[-10,10]|k2=-0.08[-10,10]|To=25[0,50]"},
		//{"Stinner2","Rm*(exp(k1+k2*To))/(1+exp(k1+k2*T))","Rm=0.7[1E-5,1]|k1=3.7[-10,10]|k2=-0.08[-10,10]|To=39[0,50]"},
		{"Stinner_1974","c(Rm/(1+exp(k1 + k2*T[T<To])),Rm/(1+exp(k1 + k2*(2*To-T[T>=To]))))","Rm=0.2101[0.0001,1]|k1=4.0102[-10,10]|k2=-0.2227[-10,10]|To=26[0,50]"},
		//{"Stinner4","c1/(1+exp(k1+k2*T))+c2/(1+exp(k1+k2*(2*To-T)))","c1=0.2[-1E5,1]|c2=0.2[-1E5,1]|k1=0.2[-10,10]|k2=0.2[-10,10]|To=35[0,50]"},
		{"Taylor_1981","Rm*exp(-1/2*((T-Topt)/Troh)^2)","Rm=0.08[0.0001,1.0]|Topt=32[1,50]|Troh=9.4[1,50]"},
		{"Wagner_1988","1/((1+exp((cc/1.987)*((1/dd)-(1/(T+273.16)))))/(aa*(T+273.16)/298.15*exp((bb/1.987)*((1/298.15)-1/(T+273.16)))))","aa=0.1[0.0001,10]|bb=28500[1,1E6]|cc=130000[1,2E6]|dd=305[250,350]"},
		{"Wang_1982","(K/(1+exp(-r*(T-To))))*(1-exp(-pmax(0,T-Tl)/aa))*(1-exp(-pmax(0,Th-T)/aa))","K=0.04[0.0001,10]|r=0.23[0.0001,10]|aa=2.3[1,10]|Tl=10[0,50]|Th=35[0,50]|To=15[0,50]"},
		{"Wangengel_1998","(2*(T-Tmin)^aa*(Topt-Tmin)^aa-(T-Tmin)^(2*aa))/((Topt-Tmin)^(2*aa))","aa=2.95[1,4]|Tmin=-16[-1000,0]|Topt=100[0,1000]"},
		{"Yin_beta_1995","exp(mu)*(T-Tb)^aa*(Tc-T)^bb","aa=1[1,4]|bb=1[0.25,4]|mu=-10.0[-1E3,0]|Tb=5[0,30]|Tc=35[0,200]"},
	};

	bool CDevRateEquation::IsParamValid(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P)
	{
		bool bValid = true;
		switch (model)
		{
		case Allahyari:bValid = P[P1] < P[P2]; break;
		case Analytis_1977: bValid = P[P3] < P[P4]; break;
		case Bieri_1983:bValid = P[P2] < P[P3]; break;
		case Briere1_1999:bValid = P[P1] < P[P2]; break;
		case Briere2_1999:bValid = P[P2] < P[P3]; break;
		case HilberLogan_1983:bValid = P[P2] < P[P3]; break;
		case Kontodimas_2004:bValid = P[P1] < P[P2]; break;
		case Ratkowsky_1983:bValid = P[P2] < P[P3]; break;
		case Regniere_1987:  bValid = P[P4] < P[P5]; break;
		case Regniere_2012:bValid = P[P2] < P[P3]; break;
		case Shi_2011: bValid = P[P3] < P[P4]; break;
		case HueyStevenson_1979: bValid = P[P2] < P[P3]; break;
		case Shi_beta_2016:bValid = (P[P2] < P[P1]) && (P[P1] < P[P3]); break;
		case Wang_1982:bValid = (P[P3] < P[P5]) && (P[P5] < P[P4]); break;
		case Wangengel_1998:bValid = P[P1] < P[P2]; break;
		case Yin_beta_1995:bValid = P[P3] < P[P4]; break;
		}

		return bValid;
	}

	double CDevRateEquation::GetFValue(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P, double T)
	{

		double rT = 0;

		if (model == Allahyari)
		{
			//f < -function(x, P, Tmax, Tmin, n, m)
			double p = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];
			double n = P[P3];
			double m = P[P4];

			T = max(Tmin, min(Tmax, T));
			rT = p * pow((T - Tmin) / (Tmax - Tmin), n)*(1.0 - pow((T - Tmin) / (Tmax - Tmin), m)); //CAMBIO
		}
		else if (model == Analytis_1977)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double Tmin = P[P3];
			double Tmax = P[P4];

			T = max(Tmin, min(Tmax, T));
			rT = aa * pow(T - Tmin, bb) * pow(Tmax - T, cc);
		}
		if (model == Angilletta_2006)
		{
			double k1 = P[P0];
			double k2 = P[P1];
			double k3 = P[P2];
			double To = P[P3];

			rT = k1 * exp(-0.5*pow(abs(T - To) / k2, k3));
		}
		else if (model == Bieri_1983)
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			rT = aa * (T - Tmin) - (bb * exp(T - Tmax));
		}
		else if (model == Briere1_1999)
		{
			//f < -function(x, aa, To, Tmax)
			double aa = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];

			T = max(Tmin, min(Tmax, T));
			rT = aa * T*(T - Tmin)*pow(Tmax - T, 0.5);
		}
		else if (model == Briere2_1999)//1999
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			T = min(Tmax, T);
			rT = aa * T*(T - Tmin)*pow(Tmax - T, 1.0 / bb);
		}
		else if (model == Damos_2008)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			rT = aa * (bb - T / 10) * pow(T / 10, cc);
		}
		else if (model == Damos_2011)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];

			rT = aa / (1.0 + bb * T + cc * T*T);
		}
		else if (model == DevaHiggis)
		{
			//		f < -function(x, b1, b2, b3, b4, b5)
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			double tau = (T - b3) / (b3 - b2) - (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)));
			double mu = (1 + b4) / (1 + 1.5*b4 + 0.39*b4*b4);
			double phi = pow((tau + exp(b4*tau)) / mu, 2.0);

			T = max(b2, min(b3, T));
			rT = b1 * pow(10.0, -phi)*(1 - b5 + b5 * phi);
		}
		else if (model == Exponential)
		{
			//f < -function(x, b1, b2)
			double b1 = P[P0];
			double b2 = P[P1];

			rT = b1 * exp(b2*T);
		}
		else if (model == HilberLogan_1983)
		{
			//f < -function(x, trid, D, Tmax, Dt) 
			double phi = P[P0];
			double aa = P[P1];
			double Tb = P[P2];
			double Tmax = P[P3];
			double deltaT = P[P4];

			T = max(Tb, T);
			rT = phi * ((pow(T - Tb, 2.0) / (pow(T - Tb, 2.0) + aa * aa)) - exp(-(Tmax - (T - Tb)) / deltaT));
		}
		else if (model == HilberLoganIII)
		{
			//f < -function(x, d, Y, Tmax, deltaT)
			double phi = P[P0];
			double d = P[P1];
			double Tmax = P[P2];
			double deltaT = P[P3];

			rT = phi * ((T*T) / (T*T + d * d) - exp(-(Tmax - T) / deltaT));
			//phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))
			//phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))
		}
		else if (model == HueyStevenson_1979)
		{
			double cc = P[P0];
			double k = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			T = max(Tmin, min(Tmax, T));
			rT = cc * (T - Tmin) * (1.0 - exp(k * (T - Tmax)));
		}
		else if (model == Janisch1_1932)
		{
			//f < -function(x, Dmin, Topt, K) 
			double Dmin = P[P0];
			double Topt = P[P1];
			double K = P[P2];

			rT = 2.0 / (Dmin*(exp(K*(T - Topt)) + exp((-K)*(T - Topt))));
		}
		else if (model == Janisch2)
		{
			//f < -function(x, c, a, b, Tm)
			double cc = P[P0];
			double aa = P[P1];
			double bb = P[P2];
			double Tm = P[P3];

			rT = 2 * cc / (pow(aa, T - Tm) + pow(bb, Tm - T));
		}
		else if (model == Kontodimas_2004)
		{
			//f < -function(x, aa, Tmin, Tmax)
			double aa = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];

			T = max(Tmin, min(Tmax, T));
			rT = aa * (pow(T - Tmin, 2.0))*(Tmax - T);
		}
		else if (model == Logan6_1976)//Logan6 1976
		{
			//f < -function(x, phi, bb, Tmax, deltaT)
			double phi = P[P0];
			double bb = P[P1];
			double Tmax = P[P2];
			double deltaT = P[P3];

			//T = min(Tmax, T);
			rT = phi * (exp(bb * T) - exp(bb * Tmax - (Tmax - T) / deltaT));
		}
		else if (model == Logan10_1976)//Logan10 1976
		{
			double phi = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double Tmax = P[P3];
			double deltaT = P[P4];

			//T = min(Tmax, T);
			rT = phi * (1.0 / (1.0 + cc * exp(-bb * T)) - exp(-((Tmax - T) / deltaT)));
		}
		else if (model == LoganTb)//  Tb Model (Logan)
		{
			//f < -function(x, sy, b, Tb, DTb)
			double phi = P[P0];
			double b = P[P1];
			double Tb = P[P2];
			double DeltaT = P[P3];

			rT = phi * exp(b*(T - Tb) - exp(b*(T - Tb) / DeltaT));
		}
		//  Exponential Model (Logan)
		else if (model == LoganExponential)
		{
			//f < -function(x, sy, b, Tb)
			double phi = P[P0];
			double b = P[P1];
			double Tb = P[P2];

			rT = phi * exp(b*(T - Tb));
		}
		else if (model == Poly1)
		{
			//f < -function(x, a, b)
			double a0 = P[P0];
			double a1 = P[P1];

			rT = a0 + a1 * T;
		}
		else if (model == Poly2)
		{
			double a0 = P[P0];
			double a1 = P[P1];
			double a2 = P[P2];

			rT = a0 + a1 * T + a2 * T *T;
		}
		else if (model == Poly3)//Tanigoshi
		{
			//f < -function(x, a0, a1, a2, a3)
			double a0 = P[P0];
			double a1 = P[P1];
			double a2 = P[P2];
			double a3 = P[P3];

			rT = a0 + a1 * T + a2 * T*T + a3 * T*T*T;
		}
		else if (model == Poly4)
		{
			double a0 = P[P0];
			double a1 = P[P1];
			double a2 = P[P2];
			double a3 = P[P3];
			double a4 = P[P4];

			rT = a0 + a1 * T + a2 * T*T + a3 * T*T*T + a4 * T*T*T*T;
		}
		else if (model == RatkowskySquare)
		{
			//f < -function(x, b, Tb)
			double b = P[P0];
			double Tb = P[P1];

			rT = b * pow((T - Tb), 2.0);
		}
		else if (model == Ratkowsky_1983)
		{
			//f < -function(x, aa, b, Tmin, Tmax)
			double aa = P[P0];
			double b = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			//T = max(Tmin, min(Tmax, T));
			rT = pow(aa*(T - Tmin)*(1.0 - exp((b*(Tmax - T)))), 2.0);
		}
		else if (model == Pradham)
		{
			//f < -function(x, R, Tm, To)
			double R = P[P0];
			double Tm = P[P1];
			double To = P[P2];

			rT = R * pow(exp((-1.0 / 2.0)*((T - Tm) / To)), 2.0);
		}
		//else if (model == Stinner1)
		//{
		//	//f < -function(xp, Rm, To, k1, k2) 
		//	double Rm = P[P0];
		//	double k1 = P[P1];
		//	double k2 = P[P2];
		//	double To = P[P3];

		//	rT = Rm * (1.0 + exp(k1 + k2 * To)) / (1.0 + exp(k1 + k2 * T));
		//}
		//else if (model == Stinner2)
		//{
		//	//f < -function(x, Rm, k1, k2, To)
		//	double Rm = P[P0];
		//	double k1 = P[P1];
		//	double k2 = P[P2];
		//	double To = P[P3];

		//	rT = Rm * exp(k1 + k2 * To) / (1.0 + exp(k1 + k2 * T));
		//}
		else if (model == Stinner_1974)
		{
			double Rm = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double To = P[P3];

			T = T < To ? T : (2 * To - T);
			rT = Rm / (1.0 + exp(k1 + k2 * T));
		}
		//else if (model == Stinner4)
		//{
		//	//f < -function(x, c1, c2, k1, k2, To)
		//	double c1 = P[P0];
		//	double c2 = P[P1];
		//	double k1 = P[P2];
		//	double k2 = P[P3];
		//	double To = P[P4];

		//	rT = c1 / (1.0 + exp(k1 + k2 * T)) + c2 / (1.0 + exp(k1 + k2 * (2 * To - T)));
		//}
		//Taylor
		else if (model == Taylor_1981)
		{
			//f < -function(x, rm, Topt, Troh)
			double rm = P[P0];
			double Topt = P[P1];
			double Troh = P[P2];

			rT = rm * exp(-(0.5)*pow(-(T - Topt) / Troh, 2.0));
		}
		//lactin1 1995
		else if (model == Lactin1_1995)
		{
			double aa = P[P0];
			double Tmax = P[P1];
			double deltaT = P[P2];


			rT = exp(aa * T) - exp(aa * Tmax - (Tmax - T) / deltaT);
		}
		//lactin2 1995
		else if (model == Lactin2_1995)
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmax = P[P2];
			double deltaT = P[P3];

			rT = exp(aa * T) - exp(aa * Tmax - (Tmax - T) / deltaT) + bb;
		}
		else if (model == Regniere_1987)
		{
			T = max(P[P4], min(P[P5], T));
			double Tau = (T - P[P4]) / (P[P5] - P[P4]);
			double p1 = 1.0 / (1.0 + exp(P[P1] - P[P2] * Tau));
			double p2 = exp((Tau - 1) / P[P3]);
			rT = P[P0] * (p1 - p2);
		}
		else if (model == Regniere_2012)
		{
			double phi = P[P0];
			double bb = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];
			double deltab = P[P4];
			double deltam = P[P5];

			T = max(Tb, min(Tm, T));
			rT = phi * (exp(bb * (T - Tb)) - ((Tm - T) / (Tm - Tb)) * exp(-bb * (T - Tb) / deltab) - ((T - Tb) / (Tm - Tb)) * exp(bb * (Tm - Tb) - (Tm - T) / deltam));
		}
		else if (model == Shi_beta_2016)
		{
			double rm = P[P0];
			double Tm = P[P1];
			double T1 = P[P2];
			double T2 = P[P3];

			T = max(T1, min(T2, T));
			rT = rm * (T2 - T) / (T2 - Tm) * pow(((T - T1) / (Tm - T1)), ((Tm - T1) / (T2 - Tm)));
		}

		else if (model == Lamb_1992)
		{
			double Rm = P[P0];
			double Tmax = P[P1];
			double T0 = P[P2];
			double T1 = P[P3];

			double To = T < Tmax ? T0 : T1;
			rT = Rm * exp(-1.0 / 2.0 * pow((T - Tmax) / To, 2.0));
		}
		else if (model == Schoolfield_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double bb = P[P2];
			double cc = P[P3];
			double dd = P[P4];
			double ee = P[P5];

			rT = (p25 * (T + 273.16) / 298.0 * exp(aa / 1.987 * (1.0 / 298.0 - 1.0 / (T + 273.16)))) / (1.0 + exp(bb / 1.987 * (1.0 / cc - 1.0 / (T + 273.16))) + exp(dd / 1.987 * (1.0 / ee - 1.0 / (T + 273.16))));
		}
		else if (model == SchoolfieldHigh_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double dd = P[P2];
			double ee = P[P3];

			rT = (p25 * (T + 273.16) / 298.0 * exp(aa / 1.987 * (1.0 / 298.0 - 1.0 / (T + 273.16)))) / (1.0 + exp(dd / 1.987 * (1.0 / ee - 1.0 / (T + 273.16))));
		}
		else if (model == SchoolfieldLow_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double bb = P[P2];
			double cc = P[P3];

			rT = (p25 * (T + 273.16) / 298.0 * exp(aa / 1.987 * (1.0 / 298.0 - 1.0 / (T + 273.16)))) / (1.0 + exp(bb / 1.987 * (1.0 / cc - 1.0 / (T + 273.16))));
		}
		else if (model == SharpeDeMichele3)
		{
			//f < -function(x, p, To, Ha, Hl, Tl, Hh, Th)
			double p = P[P0];
			double To = P[P1];
			double Ha = P[P2];
			double Hl = P[P3];
			double Tl = P[P4];
			double Hh = P[P5];
			double Th = P[P6];

			double x = T + 273.15;
			rT = (p * (x / (To + 273.16)) * exp((Ha / 1.987) * ((1.0 / (To + 273.16)) - (1.0 / x)))) /
				(1.0 + exp((Hl / 1.987) * ((1.0 / (Tl + 273.16)) - (1.0 / x))) + exp((Hh / 1.987) * ((1.0 / (Th + 273.16)) - (1.0 / x))));
		}
		else if (model == SharpeDeMichele_1977)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double dd = P[P3];
			double ff = P[P4];
			double gg = P[P5];

			rT = ((T + 273.16)*exp((aa - bb / (T + 273.16)) / 1.987)) / (1.0 + exp((cc - dd / (T + 273.16)) / 1.987) + exp((ff - gg / (T + 273.16)) / 1.987));
		}

		else if (model == Shi_2011)
		{
			double cc = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double T1 = P[P3];
			double T2 = P[P4];

			rT = cc * (1.0 - exp(-k1 * (T - T1))) * (1.0 - exp(k2 * (T - T2)));
		}
		else if (model == StAmant_2019)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double T1 = P[P3];
			double T2 = P[P4];

			T = min(T2 - 0.001, T);
			rT = aa * exp(bb*(T - T1) + 1.0 / (cc*(T - T2)));
		}
		else if (model == Wagner_1988)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double dd = P[P3];

			rT = 1.0 / ((1.0 + exp((cc / 1.987) * ((1.0 / dd) - (1.0 / (T + 273.16))))) / (aa * (T + 273.16) / 298.15 * exp((bb / 1.987) * ((1.0 / 298.15) - 1.0 / (T + 273.16)))));
		}
		else if (model == Wang_1982)
		{
			double K = P[P0];
			double r = P[P1];
			double aa = P[P2];
			double Tl = P[P3];
			double Th = P[P4];
			double To = P[P5];


			rT = (K / (1.0 + exp(-r * (T - To)))) * (1.0 - exp(-(T - Tl) / aa)) * (1.0 - exp(-(Th - T) / aa));
		}
		else if (model == Wangengel_1998)
		{
			double aa = P[P0];
			double Tmin = P[P1];
			double Topt = P[P2];

			T = max(Tmin, T);
			rT = (2.0 * pow(T - Tmin, aa) * pow(Topt - Tmin, aa) - pow(T - Tmin, 2.0 * aa)) / pow(Topt - Tmin, 2.0 * aa);
		}
		else if (model == Yin_beta_1995)
		{
			double aa = P[P0];
			double bb = P[P1];
			double mu = P[P2];
			double Tb = P[P3];
			double Tc = P[P4];

			T = max(Tb, min(Tc, T));
			rT = exp(mu) * pow(T - Tb, aa) * pow(Tc - T, bb);
		}

		return rT;
	}

	CSAParameterVector CDevRateEquation::GetParameters(TDevRateEquation model)
	{
		CSAParameterVector p;

		StringVector tmp(CDevRateEquation::EQUATION[model][EQ_PARAM], "=[,]|");
		ASSERT(tmp.size() % 4 == 0);
		for (size_t i = 0; i < tmp.size() / 4; i++)
		{
			p.push_back(CSAParameter(tmp[i * 4], stod(tmp[i * 4 + 1]), stod(tmp[i * 4 + 2]), stod(tmp[i * 4 + 3])));
		}

		return p;
	}

	CSAParameterVector CDevRateEquation::GetParameters(TDevRateEquation model, const vector<double>& X)
	{
		CSAParameterVector p = GetParameters(model);
		ASSERT(X.size() == p.size());
		for (size_t i = 0; i < p.size(); i++)
		{
			p[i].m_initialValue = X[i];
		}

		return p;
	}

}






