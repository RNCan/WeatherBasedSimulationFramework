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

	//const char* CDevRateEquation::NAME[NB_EQUATIONS] =
	//{ /*"Sharpe & DeMichele 1", "Sharpe & DeMichele 2",*/ "Sharpe&DeMichele3", "Sharpe&DeMichele4", "Sharpe&DeMichele5",
	//  "Sharpe&DeMichele6", /*"Sharpe & DeMichele 7", "Sharpe & DeMichele 8", "Sharpe & DeMichele 9",*/ "Sharpe&DeMichele10",
	//  "Sharpe&DeMichele11", /*"Sharpe & DeMichele 12",*/ "Sharpe&DeMichele13", "Sharpe&DeMichele14",
	//  "Deva&Higgis", "Logan6 (1976)", "Logan10 (1976)", "Briere1 (1999)", "Briere2 (1999)", "Hilber&logan1", "Hilber&Logan2 (1983)",
	//  "Latin2", "Linear", "Poly2", "Poly3", "Poly4","Exponential", "Tb Model (Logan)", "Exponential Model (Logan)",
	//  "Ratkowsky1 (Square root model)", "Ratkowsky2 (1983)", "Davidson (1944)", "Pradham", "Allahyari",
	//  "Kontodimas (2004)", "Janish1 ", "Janish2", "Stinner1", "Stinner2", "Stinner3 (1974)", "Stinner4",
	//  "Taylor (1981)", "Lactin1 (1995)", "Lactin2 (1995)", "Logistic", "Regniere1 (1987)",
	//  "Analytis (1977)"/*,"Bayoh 2003"*/,"Shi beta (2016)","Yin 1995 (beta)","Bieri (1983)","Damos (2008)","Damos (2011)",
	//  "Lamb (1992)","Shi2 (2001) perf","Rootsq (1982)","Schoolfield (1981)","SchoolfieldHigh (1981)","SchoolfieldLow (1981)","Shi 2011",
	//  "Wagner (1988)","Wang (1982)","Wangengel (1998)"
	//};
			//"Deva&Higgis", "Logan6 (1976)", "Logan10 (1976)", "Briere1 (1999)", "Briere2 (1999)", "Hilber&logan1", "Hilber&Logan2 (1983)",
		//"Latin2", "Linear", "Poly2", "Poly3", "Poly4","Exponential", "Tb Model (Logan)", "Exponential Model (Logan)",
		//"Ratkowsky1 (Square root model)", "Ratkowsky2 (1983)", "Davidson (1944)", "Pradham", "Allahyari",
		//"Kontodimas (2004)", "Janish1 ", "Janish2", "Stinner1", "Stinner2", "Stinner3 (1974)", "Stinner4",
		//"Taylor (1981)", "Lactin1 (1995)", "Lactin2 (1995)", "Logistic", "Regniere1 (1987)",
		//"Analytis (1977)"/*,"Bayoh 2003"*/,"Shi beta (2016)","Yin 1995 (beta)","Bieri (1983)","Damos (2008)","Damos (2011)",
		//"Lamb (1992)","Shi2 (2001) perf","Rootsq (1982)","Schoolfield (1981)","SchoolfieldHigh (1981)","SchoolfieldLow (1981)","Shi 2011",
		//"Wagner (1988)","Wang (1982)","Wangengel (1998)"

	const char* CDevRateEquation::EQUATION[NB_EQUATIONS][3] =
	{
		{"Allahyari","p*(((T-Tmin)/(Tmax-Tmin))^n)*(1-((T-Tmin)/(Tmax-Tmin))^m)","p=0.2[0.001,10]|Tmin=12[1,50]|Tmax=25[1,50]|n=1[0.001,10]|m=1[0.001,10]"},
		{"Analytis_1977","aa*(T-Tmin)^bb*(Tmax-T)^cc","aa=5E-4[1.0E-10,0.1]|bb=2.5[0.001,100]|cc=0.35[0.001,100]|Tmin=2.3[-50,50]|Tmax=35[0,50]"},
		{"Bieri1_1983","aa*(T-Tmin)-(bb*exp(T-Tmax))","aa=0.015[0.00001,0.1]|bb=3.6[0.0001,100]|Tmin=4[0,50]|Tmax=42[0,50]"},
		{"Briere1_1999","aa*T*(T-Tmin)*(Tmax-T)^(1/2)","aa=0.8[0.00001,100]|Tmin=9.1[0,50]|Tmax=37.3[0,50]"},
		{"Briere2_1999","aa*T*(T-Tmin)*(Tmax-T)^(1/bb)","aa=1.2[1E-5,1E5]|bb=2.8[1E-5,1E5]|Tmin=7.0[-50,50]|Tmax=35[-50,50]"},
		{"Damos_2008","aa*(bb-T/10)*(T/10)^cc","aa=7.5E-4[1E-10,0.1]|bb=3.8[0.01,100]|cc=5.0[0.01,100]"},
		{"Damos_2011","aa/(1+bb*T+cc*T^2)","aa=0.01[-10,10]|bb=-0.1[-10,10]|cc=0.01[-10,10]"},
		{"Davidson_1944(Logistic)","k/(1+exp(a - b*T))","k=0.4[0.0001,100]|a=4.5[-1E3,1E3]|b=-0.21[-1E3,1E3]"},
		{"Deva&Higgis","b1*10^(-((((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))+exp(b4*((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))))/((1+b4)/(1+1.5*b4+0.39*b4^2)))^2)*(1-b5+b5*((((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))+exp(b4*((T-b3)/(b3-b2)-(1/(1+0.28*b4+0.72*log(1+b4))))))/((1+b4)/(1+1.5*b4+0.39*b4^2)))^2)","b1=0.2[0.0001,1000]|b2=10[0,50]|b3=25[0,50]|b4=0.2[0.0001,1000]|b5=0.2[0.0001,1000]"},
		{"Exponencial","b1*exp(b2*T)","b1=0.2[0.0001,1000]|b2=0.2[0.0001,1000]"},
		{"Hilber&logan_1983","phi*(((T-Tb)^2/((T-Tb)^2+aa^2))-exp(-(Tmax-(T-Tb))/deltaT))","phi=0.5[0.001,100]|aa=312[0.001,1000]|Tb=5,0.01,50]|Tmax=35[1,50]|deltaT=6[0.001,100]"},
		{"Hilber&loganIII","phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))","phi=0.3[0.0001,1000]|d=10[0.001,1000]|Tmax=35[0,50]|deltaT=6[0.0001,1000]"},
		{"Janisch1_1932","2/(Dmin*(exp(K*(T-Topt))+exp((-K)*(T-Topt))))","Dmin=28[-50,50]|Topt=33[-50,50]|K=0.2[0.0001,10]"},
		{"Janisch2","2*c/(a^(T-Tm)+b^(Tm-T))","c=0.2[1E-5,1E5]|a=0.2[1E-5,1E5]|b=0.2[1E-5,1E5]|Tm=15[-50,50]"},
		{"Kontodimas_2004","aa*((T-Tmin)^2)*(Tmax-T)","aa=1.6e-5,1e-7,1]|Tmin=8[1,50]|Tmax=42[1,50]"},
		{"Lactin1_1995","exp(aa*T)-exp(aa*Tmax-(Tmax-T)/deltaT)","aa=0.016[0.0001,100]|Tmax=37[1,50]|deltaT=8[0.0001,100]"},
		{"Lactin2_1995","exp(aa*T)-exp(aa*Tmax-(Tmax-T)/deltaT)+bb","aa=0.03[0.0001,10]|bb=-1.0[-10,10]|Tmax=41[1,60]|deltaT=4[0.0001,100]"},
		{"Lamb_1992","Rm*exp(-1/2*((T-Tmax)/To)^2)","Rm=0.004[0.0001,10]|Tmax=12[0,50]|To=4[0.1,50]"},
		{"Logan10_1976","phi*(1/(1+cc*exp(-bb*T))-exp(-((Tmax-T)/deltaT)))","phi=0.3[0.0001,100]|bb=0.0,-100,100]|cc=110[0.0001,1000]|Tmax=36.7[0,50]|deltaT=3.7,0.01,100]"},
		{"Logan6_1976","phi*(exp(bb*T)-exp(bb*Tmax-(Tmax-T)/deltaT))","phi=0.12[0.0001,100]|bb=0.14[0.0001,100]|Tmax=34.6[0,50]|deltaT=4.6[0.01,100]"},
		{"LoganExponential","sy*exp(b*(T-Tb))","sy=0.2[0.0001,1000]|b=0.2[0.0001,1000]|Tb=15[0,50]"},
		{"LoganTb","sy*exp(b*(T-Tb)-exp(b*(T-Tb)/DTb))","sy=0.2[0.0001,1000]|b=0.2[0.0001,1000]|Tb=15[0,50]|DTb=0.2[0.0001,1000]"},
		{"Poly1","a0+a1*T","a0=-0.05[-1E3,1E3]|a1=0.1[1E-4,1E4]"},
		{"Poly2","a0+a1*T+a2*T^2","a0=0[-1E5,1E5]|a1=-0.003[-1E4,1E4]|a2=0.0004[-1E3,1E3]"},
		{"Poly3","a0+a1*T+a2*T^2+a3*T^3","a0=0.2[-1E5,1E5]|a1=-0.1[-1E4,1E4]|a2=0.007[-1E3,1E3]|a3=-0.0001[-1E2,1E2]"},
		{"Poly4","a0+a1*T+a2*T^2+a3*T^3+a4*T^4","a0=0.2[-1E5,1E5]|a1=-0.02[-1E4,1E4]|a2=0.002[-1E3,1E3]|a3=-0.0002[-1E2,1E2]|a4=0.00002[-10,10]"},
		{"Pradham","R*exp((-1/2)*((T-Tm)/To))^2","R=0.5[0.0001,1000]|Tm=10[1,50]|To=8[1,50]"},
		{"Ratkowsky_1982(Square)","b*(T-Tb)^2","b=0.2[0.0001,1000]|Tb=30[0,50]"},
		{"Ratkowsky_1983","(a*(T-Tmin)*(1-exp((b*(Tmax-T)))))^2","a=0.2[0.0001,1000]|b=0.2[0.0001,1000]|Tmin=10[1,50]|Tmax=30[1,50]"},
		{"Regniere_1987","b1*((1/(1+exp(b2-b3*(T-Tb)/(Tm-Tb))))-(exp(((T-Tb)/(Tm-Tb)-1)/b4)))","b1=0.2[0.001,1000]|b2=2[0.001,1000]|b3=5[0.001,1000]|b4=0.15[0.001,1000]|Tb=10[0,50]|Tm=30[0,50]"},
		{"Regniere_2012","phi*(exp(bb*(T-Tb))-((Tm-T)/(Tm-Tb))*exp(-bb*(T-Tb)/deltab)-((T-Tb)/(Tm-Tb))*exp(bb*(Tm-Tb)-(Tm-T)/deltam))","phi=0.1[0.0001|100]|bb=0[-1E3,1E3]|Tb=10[0,50]|Tm=30[0,50]|deltab=4[1E-4,1E4]|deltam=4[1E-4,1E4]"},
		{"SchoolfieldHigh_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(dd/1.987*(1/ee-1/(T+273.16))))","p25=0.1[-1000,1000]|aa=0.2[0,1E5]|dd=-0.2[-1E5,0]|ee=280[250,350]"},
		{"SchoolfieldLow_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(bb/1.987*(1/cc-1/(T+273.16))))","p25=0.2[0.00001,1000]|aa=0.2[0,1E5]|bb=-0.2[-1E5,0]|cc=280[250,350]"},
		{"Schoolfield_1981","(p25*(T+273.16)/298*exp(aa/1.987*(1/298-1/(T+273.16))))/(1+exp(bb/1.987*(1/cc-1/(T+273.16)))+exp(dd/1.987*(1/ee-1/(T+273.16))))","p25=0.2[0.0001,100]|aa=0.2[0,1E5]|bb=-0.2,-1E5,0]|cc=300[250,350]|dd=0.2[0,1E5]|ee=300[200,400]"},
		{"Sharpe&DeMichele3","(p*((T+273.16)/(To))*exp((Ha/1.987)*((1/To)-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/Tl)-(1/(T+273.16))))+exp((Hh/1.987)*((1/Th)-(1/(T+273.16)))))","p=0.01[0.001,1000]|To=0.2[0.001,50]|Ha=0.2[-1000,100000]|Hl=2[0.001,50]|Tl=5[0.001,50]|Hh=0.15[0.001,10000]|Th=4[1,50]"},
		{"Sharpe&DeMichele4","(p*((T+273.16)/(To))*exp((Ha/1.987)*((1/To)-(1/(T+273.16)))))","p=0.01[0.001,1000]|To=0.2[0.001,50]|Ha=0.2[-1000,1000000]"},
		{"Sharpe&DeMichele5","(p*((T+273.16)/(To))*exp((Ha/1.987)*((1/To)-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/Tl)-(1/(T+273.16)))))","p=0.01[0.001,1000]|To=0.2[0.001,50]|Ha=0.2[-1000,100000]|Hl=2[0.001,50]|Tl=5[0.001,50]"},
		{"Sharpe&DeMichele6","(p*((T+273.16)/(To))*exp((Ha/1.987)*((1/To)-(1/(T+273.16)))))/(1+exp((Hh/1.987)*((1/Th)-(1/(T+273.16)))))","p=0.01[0.001,1000]|To=0.2[0.001,50]|Ha=0.2[-1000,100000]|Hh=0.15[0.001,10000]|Th=4[1,50]"},
		{"Sharpe&DeMichele10","(p*((T+273.16)/(298.16))*exp((Ha/1.987)*((1/298.16)-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/Tl)-(1/(T+273.16))))+exp((1/1.987)*((1/1)-(1/(T+273.16)))))","p=0.01[0.001,1000]|Ha=0.2[-1000,100000]|Hl=2[0.001,50]|Tl=5[0.001,50]"},
		{"Sharpe&DeMichele11","(p*((T+273.16)/298.16)*exp((Ha/1.987)*((1/298.16)-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/Tl)-(1/(T+273.16))))+exp((Hh/1.987)*((1/Th)-(1/(T+273.16)))))","p=0.01[0.001,1000]|Ha=0.2[-1000,100000]|Hl=2[0.001,50]|Tl=5[0.001,50]|Hh=0.15[0.001,10000]|Th=4[1,50]"},
		{"Sharpe&DeMichele13","(p*((T+273.16)/298.16)*exp((Ha/1.987)*((1/298.16)-(1/(T+273.16)))))/(1+exp((Hl/1.987)*((1/Tl)-(1/(T+273.16)))))","p=0.01[0.001,1000]|Ha=0.2[-1000,100000]|Hl=2[0.001,50]|Tl=5[0.001,50]"},
		{"Sharpe&DeMichele14","(p*((T+273.16)/298.16)*exp((Ha/1.987)*((1/298.16)-(1/(T+273.16)))))/(1+exp((Hh/1.987)*((1/Th)-(1/(T+273.16)))))","p=0.01[0.001,1000]|Ha=0.2[-1000,100000]|Hh=0.15[0.001,10000]|Th=4[1,50]"},
		{"SharpeDeMichele_1977","((T+273.16)*exp((aa-bb/(T+273.16))/1.987))/(1+exp((cc-dd/(T+273.16))/1.987)+exp((ff-gg/(T+273.16))/1.987))","aa=1[-1E3,1E3]|bb=1[-1E3,1E3]|cc=1[-1E3,1E3]|dd=1[-1E3,1E3]|ff=1[-1E3,1E3]|gg=1[-1E3,1E3]"},
		{"Shi_2011","cc*(1-exp(-k1*(T-T1)))*(1-exp(k2*(T-T2)))","cc=0.2[0.001,1000]|k1=0.2[0.001,1000]|k2=0.2[0.001,1000]|T1=10[0,40]|T2=30[0,40]"},
		{"Shi_Perf2_2011","cc*(T-T1)*(1-exp(k*(T-T2)))","cc=0.2[0.001,1000]|k=0.2[0.001,1000]|T1=15[0,50]|T2=30[0,50]"},
		{"Shi_beta_2016","rm*(T2-T)/(T2-Tm)*((T-T1)/(Tm-T1))^((Tm-T1)/(T2-Tm))","rm=0.02[0.001,10]|Tm=15,10,20]|T1=05[0,15]|T2=30,20,40]"},
		{"Stinner1","Rmax*(1+exp(k1+k2*(Topc)))/(1+exp(k1+k2*T))","Rmax=0.2[1E-5,1E5]|Topc=15[-50,50]|k1=0.2[-10,10]|k2=0.2[-10,10]"},
		{"Stinner2","Rmax*(exp(k1+k2*Topc))/(1+exp(k1+k2*T))","Rmax=0.2[0.0001,1000]|Topc=15[1,50]|k1=1[-1000,1000]|k2=1[-1000,1000]"},
		{"Stinner_1974","c1/(1+exp(k1+k2*T))","c1=0.02[0.0001,1000]|k1=0.2[-1000,1000]|k2=0.2[-1000,1000]"},
		{"Stinner4","c1/(1+exp(k1+k2*T))+c2/(1+exp(k1+k2*(2*To-T)))","c1=0.2[0.0001,1000]|c2=0.2[0.0001,1000]|k1=0.2[-1000,1000]|k2=0.2[-1000,1000]|To=15[1,50]"},
		{"Taylor_1981","Rm*exp(-1/2*((T-Topt)/Troh)^2)","Rm=0.08[0.0001,1.0]|Topt=32[1,50]|Troh=9.4[1,50]"},
		{"Wagner_1988","1/((1+exp((cc/1.987)*((1/dd)-(1/(T+273.16)))))/(aa*(T+273.16)/298.15*exp((bb/1.987)*((1/298.15)-1/(T+273.16)))))","aa=0.1[0.0001,10]|bb=28500[1,1E6]|cc=130000[1,2E6]|dd=305[250,350]"},
		{"Wang_1982","(K/(1+exp(-r*(T-To))))*(1-exp(-(T-Tl)/aa))*(1-exp(-(Th-T)/aa))","K=0.04[0.0001,10]|r=0.23[0.0001,10]|aa=2.3[1,10]|Tl=284[273,323]|Th=312[273,323]|To=283[273,323]"},
		{"Wangengel_1998","(2*(T-Tmin)^aa*(Topt-Tmin)^aa-(T-Tmin)^(2*aa))/((Topt-Tmin)^(2*aa))","aa=1,0.01,100]|Tmin=10[0,50]|Topt=30[0,50]"},
		{"Yin_beta_1995","exp(mu)*(T-Tb)^aa*(Tc-T)^bb","aa=1[1,4]|bb=1[1,4]|mu=-10.0[-100,0]|Tb=05[0,10]|Tc=35[30,50]"},
	};

	double CDevRateEquation::GetFValue(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P, double T)
	{
		//		enum TParameters { P0, P1, P2, P3, P4, P5, P6, P7 };

				//	x < -datao[, 1] + 273.15
				//	y < -datao[, 2]
				//dataa < -data.frame(x = datlinea[punt, 1] + 273.15, y = c((datlinea[punt, 2]), (datlinea[punt, 4]), (datlinea[punt, 3])))
				//coefi < -as.numeric(coef(lm(dataa[, 2]~dataa[, 1])))

		double rT = 0;


		//  Sharpe&DeMichele sin p y To
		//if (model == SharpeDeMichele1)
		//{

		//	double Ha = P[P0];
		//	double Hl = P[P1];
		//	double Tl = P[P2];
		//	double Hh = P[P3];
		//	double Th = P[P4];

		//	double x = T + 273.15;
		//	//			rT = ((coefi1 + coefi2 * ((Hl - Hh) / (1.987*log(-Hl / Hh) + (Hl / Tl) - (Hh / Th)))) * (x / (((Hl - Hh) / (1.987*log(-Hl / Hh) + (Hl / Tl) - (Hh / Th))))) * exp((Ha / 1.987) * ((1 / ((Hl - Hh) / (1.987*log(-Hl / Hh) + (Hl / Tl) - (Hh / Th)))) - (1 / x)))) /
		//			//		(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));



		//}
		////  Sharpe&DeMichele con To
		//else if (model == SharpeDeMichele2)
		//{
		//	//f < -function(x, To, Ha, Hl, Tl, Hh, Th)

		//	double To = P[P0];
		//	double Ha = P[P1];
		//	double Hl = P[P2];
		//	double Tl = P[P3];
		//	double Hh = P[P4];
		//	double Th = P[P5];

		//	double x = T + 273.15;
		//	//rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
		//	//	(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));

		//}
		//  Sharpe&DeMichele con To y p
		if (model == SharpeDeMichele3)
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

			rT = (p * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
				(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
		}
		//  Sharpe&DeMichele solo con To, p y Ha
		else if (model == SharpeDeMichele4)
		{
			//f < -function(x, p, To, Ha)
			double p = P[P0];
			double To = P[P1];
			double Ha = P[P2];
			double x = T + 273.15;
			rT = (p * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x))));
		}
		//  Sharpe&DeMichele con To, p, Ha, Hl, Tl
		else if (model == SharpeDeMichele5)
		{
			//f < -function(x, p, To, Ha, Hl, Tl)
			double p = P[P0];
			double To = P[P1];
			double Ha = P[P2];
			double Hl = P[P3];
			double Tl = P[P4];

			double x = T + 273.15;
			rT = (p * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
				(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))));
		}
		//  Sharpe&DeMichele con To, p, Ha, Hh, Th
		else if (model == SharpeDeMichele6)
		{
			//			f < -function(x, p, To, Ha, Hh, Th)
			double p = P[P0];
			double To = P[P1];
			double Ha = P[P2];
			double Hh = P[P3];
			double Th = P[P4];

			double x = T + 273.15;
			rT = (p * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
				(1 + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
		}
		//  Sharpe&DeMichele con To, Ha
		//else if (model == SharpeDeMichele7)
		//{
		//	//f < -function(x, To, Ha)
		//	double To = P[P0];
		//	double Ha = P[P1];
		//	double x = T + 273.15;
		//	//			rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x))));
		//}

		//  Sharpe&DeMichele con To, Ha, Hl, Tl
		//else if (model == SharpeDeMichele8)
		//{
		//	//f < -function(x, To, Ha, Hl, Tl)
		//	double To = P[P0];
		//	double Ha = P[P1];
		//	double Hl = P[P2];
		//	double Tl = P[P3];
		//	double x = T + 273.15;

		//	//rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
		//		//(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))));
		//}

		////  Sharpe&DeMichele con To, Ha, Hh, Th
		//else if (model == SharpeDeMichele9)
		//{
		//	//f < -function(x, To, Ha, Hh, Th)

		//	double To = P[P0];
		//	double Ha = P[P1];
		//	double Hh = P[P2];
		//	double Th = P[P3];

		//	double x = T + 273.15;
		//	//rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
		//		//(1 + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));

		//}

		//  Sharpe&DeMichele con To, Hh y Th, constantes
		else if (model == SharpeDeMichele10)
		{
			//f < -function(x, p, Ha, Hl, Tl)
			double p = P[P0];
			double Ha = P[P1];
			double Hl = P[P2];
			double Tl = P[P3];

			double x = T + 273.15;
			rT = (p * (x / (298.16)) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) / (1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((1 / 1.987) * ((1 / 1) - (1 / x))));
		}
		else if (model == SharpeDeMichele11)
		{
			//f < -function(x, p, Ha, Hl, Tl, Hh, Th)
			double p = P[P0];
			double Ha = P[P1];
			double Hl = P[P2];
			double Tl = P[P3];
			double Hh = P[P4];
			double Th = P[P5];

			double x = T + 273.15;

			rT = (p * (x / 298.16) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) /
				(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
		}
		//else if (model == SharpeDeMichele12)
		//{
		//	//f < -function(x, Ha, Hl, Tl, Hh, Th)
		//	double Ha = P[P0];
		//	double Hl = P[P1];
		//	double Tl = P[P2];
		//	double Hh = P[P3];
		//	double Th = P[P4];

		//	double x = T + 273.15;
		//	//rT = ((coefi1 + coefi2 * 298.16) * (x / 298.16) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) /
		//		//(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
		//}
		else if (model == SharpeDeMichele13)
		{
			//			f < -function(x, p, Ha, Hl, Tl)
			double p = P[P0];
			double Ha = P[P1];
			double Hl = P[P2];
			double Tl = P[P3];

			double x = T + 273.15;
			rT = (p * (x / 298.16) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) /
				(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))));
		}
		else if (model == SharpeDeMichele14)
		{
			//		f < -function(x, p, Ha, Hh, Th)
			double p = P[P0];
			double Ha = P[P1];
			double Hh = P[P2];
			double Th = P[P3];

			double x = T + 273.15;
			rT = (p * (x / 298.16) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) /
				(1 + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
		}
		//  deva no lineal o Higgis
		else if (model == DevaHiggis)
		{
			//		f < -function(x, b1, b2, b3, b4, b5)
			double b1 = P[P0];
			double b2 = P[P1];
			double b3 = P[P2];
			double b4 = P[P3];
			double b5 = P[P4];

			double x = T;
			double tau = (x - b3) / (b3 - b2);
			double mu = (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)));
			double phi = 1 + 1.5*b4 + pow(0.39*b4, 2.0);
			rT = b1 * pow(10, -pow(((tau - mu) + exp(b4*(tau - mu))) / ((1 + b4) / phi), 2))*(1 - b5 + b5 * pow(((tau - mu) + exp(b4*(tau - mu))) / ((1 + b4) / phi), 2.0));
		}
		//  Logan 1
		else if (model == Logan6_1976)//Logan6 1976
		{
			//f < -function(x, phi, bb, Tmax, deltaT)
			double phi = P[P0];
			double bb = P[P1];
			double Tmax = P[P2];
			double deltaT = P[P3];

			rT = phi * (exp(bb * T) - exp(bb * Tmax - (Tmax - T) / deltaT));
		}
		else if (model == Logan10_1976)//Logan10 1976
		{
			double alpha = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double Tmax = P[P3];
			double deltaT = P[P4];

			rT = alpha * (1 / (1 + cc * exp(-bb * T)) - exp(-((Tmax - T) / deltaT)));
		}
		//  Briere 1
		else if (model == Briere1_1999)//
		{
			//f < -function(x, aa, To, Tmax)
			double aa = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];

			rT = aa * T*(T - Tmin)*pow((Tmax - T), 0.5);
		}
		//  Briere 2
		else if (model == Briere2_1999)//1999
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			rT = aa * T*(T - Tmin)*pow((Tmax - T), 1 / bb);
		}
		else if (model == HilberLogan_1983)
		{
			//f < -function(x, trid, D, Tmax, Dt) 
			double phi = P[P0];
			double aa = P[P1];
			double Tb = P[P2];
			double Tmax = P[P3];
			double deltaT = P[P4];

			rT = phi * ((pow(T - Tb, 2.0) / (pow(T - Tb, 2.0) + aa * aa)) - exp(-(Tmax - (T - Tb)) / deltaT));
		}
		//  Hilber y logan    ---  Logan typo III
		else if (model == HilberLoganIII)
		{
			//f < -function(x, d, Y, Tmax, deltaT)
			double phi = P[P0];
			double d = P[P1];
			double Tmax = P[P2];
			double deltaT = P[P3];

			rT = phi * ((T*T) / (T*T + d*d) - exp(-(Tmax - T) / deltaT));
			//phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))
			//phi*(T^2/(T^2+d^2)-exp(-(Tmax-T)/deltaT))
		}
		else if (model == Poly1)
		{
			//f < -function(x, a, b)
			double a0 = P[P0];
			double a1 = P[P1];

			rT = a0 * a1*T;
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

			double x = T;
			rT = a0 + a1 * x + a2 * x * x + a3 * x * x* x;
		}
		else if (model == Poly4)
		{
			double a0 = P[P0];
			double a1 = P[P1];
			double a2 = P[P2];
			double a3 = P[P3];
			double a4 = P[P4];

			rT = a0 + a1 * T + a2 * T *T + a3 * T *T*T + a4 * T *T*T*T;
		}
		//  exponential simple
		else if (model == Exponential)
		{
			//f < -function(x, b1, b2)
			double b1 = P[P0];
			double b2 = P[P1];

			double x = T;
			rT = b1 * exp(b2*x);
		}
		else if (model == LoganTb)//  Tb Model (Logan)
		{
			//f < -function(x, sy, b, Tb, DTb)
			double sy = P[P0];
			double b = P[P1];
			double Tb = P[P2];
			double DTb = P[P3];

			double x = T;
			rT = sy * exp(b*(x - Tb) - exp(b*(x - Tb) / DTb));
		}
		//  Exponential Model (Logan)
		else if (model == LoganExponential)
		{
			//f < -function(x, sy, b, Tb)
			double sy = P[P0];
			double b = P[P1];
			double Tb = P[P2];

			double x = T;
			rT = sy * exp(b*(x - Tb));

		}
		//  Square root model of Ratkowsky
		else if (model == RatkowskySquare)
		{
			//f < -function(x, b, Tb)
			double b = P[P0];
			double Tb = P[P1];

			double x = T;
			rT = b * pow((x - Tb), 2.0);
		}
		//Ratkowsky2
		else if (model == Ratkowsky_1983)
		{
			//f < -function(x, aa, b, Tmin, Tmax)
			double aa = P[P0];
			double b = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			double x = T;
			rT = pow(aa*(x - Tmin)*(1 - exp((b*(Tmax - x)))), 2.0); //Agregar 1-exp
		}
		//Davidson
		else if (model == Davidson_1944)
		{
			//f < -function(x, k, a, b)
			double k = P[P0];
			double a = P[P1];
			double b = P[P2];

			rT = k / (1 + exp(a - b * T)); //DM: Se cambio "+b*x" por "-b*x"
			//k/(1+exp(a-b*T))
		}
		//Pradham1
		else if (model == Pradham)
		{
			//f < -function(x, R, Tm, To)
			double R = P[P0];
			double Tm = P[P1];
			double To = P[P2];

			double x = T;
			rT = R * pow(exp((-1 / 2)*((x - Tm) / To)), 2.0); //DM: Se agrego "^2"
		}
		//Allahyari
		else if (model == Allahyari)
		{
			//f < -function(x, P, Tmax, Tmin, n, m)
			double p = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];
			double n = P[P3];
			double m = P[P4];


			rT = p * pow((T - Tmin) / (Tmax - Tmin), n)*(1 - pow((T - Tmin) / (Tmax - Tmin), m)); //CAMBIO
			//p*(((T-Tmin)/(Tmax-Tmin))^n)*(1-((T-Tmin)/(Tmax-Tmin))^m)
		}
		//Kontodimas
		else if (model == Kontodimas_2004)
		{
			//f < -function(x, aa, Tmin, Tmax)
			double aa = P[P0];
			double Tmin = P[P1];
			double Tmax = P[P2];

			double x = T;
			rT = aa * (pow(x - Tmin, 2.0))*(Tmax - x);
		}
		//Janish1
		else if (model == Janisch1_1932)
		{
			//f < -function(x, Dmin, Topt, K) 
			double Dmin = P[P0];
			double Topt = P[P1];
			double K = P[P2];

			double x = T;
			rT = 2 / (Dmin*(exp(K*(x - Topt)) + exp((-K)*(x - Topt))));
			//rT = 2 / (Dmin*(exp(aa*(T - Topt)) + exp(-bb * (T – Topt)))) ^ (-1)

		}
		//Janish-2
		else if (model == Janisch2)
		{
			//f < -function(x, c, a, b, Tm)
			double c = P[P0];
			double a = P[P1];
			double b = P[P2];
			double Tm = P[P3];

			double x = T;
			rT = 2 * c / (pow(a, (x - Tm)) + pow(b, (Tm - x)));
		}
		//  Stinner
		else if (model == Stinner1)
		{
			//f < -function(xp, Rmax, Topc, k1, k2) 
			double Rmax = P[P0];
			double Topc = P[P1];
			double k1 = P[P2];
			double k2 = P[P3];

			double x = T;
			rT = (Rmax*(1 + exp(k1 + k2 * (Topc)))) / (1 + exp(k1 + k2 * x));
		}
		//Stinner2
		else if (model == Stinner2)
		{
			//f < -function(x, Rmax, k1, k2, Topc)
			double Rmax = P[P0];
			double Topc = P[P3];
			double k1 = P[P1];
			double k2 = P[P2];

			double x = T;
			rT = Rmax * (exp(k1 + k2 * Topc)) / (1 + exp(k1 + k2 * x));
		}
		// modelos adaptados para senescencia
		//Stinner3
		else if (model == Stinner_1974)
		{
			//f < -function(x, c1, k1, k2)
			double c1 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];

			double x = T;
			rT = c1 / (1 + exp(k1 + k2 * x));
		}
		//Stinner4
		else if (model == Stinner4)
		{
			//f < -function(x, c1, c2, k1, k2, To)
			double c1 = P[P0];
			double c2 = P[P1];
			double k1 = P[P2];
			double k2 = P[P3];
			double To = P[P4];

			double x = T;
			rT = c1 / (1 + exp(k1 + k2 * x)) + c2 / (1 + exp(k1 + k2 * (2 * To - x)));
		}
		//Taylor
		else if (model == Taylor_1981)
		{
			//f < -function(x, rm, Topt, Troh)
			double rm = P[P0];
			double Topt = P[P1];
			double Troh = P[P2];

			double x = T;
			rT = rm * exp(-(0.5)*pow(-(x - Topt) / Troh, 2.0));
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
			double Tau = (T - P[P4]) / (P[P5] - P[P4]);
			double p1 = 1 / (1 + exp(P[P1] - P[P2] * Tau));
			double p2 = exp((Tau - 1) / P[P3]);
			rT = P[P0] * (p1 - p2);
		}
		else if (model == Regniere_2012)
		{
			double phi = P[P0];
			double bb = P[P4];
			double Tb = P[P4];
			double Tm = P[P4];
			double deltab = P[P4];
			double deltam = P[P4];
			rT = phi * (exp(bb * (T - Tb)) - ((Tm - T) / (Tm - Tb)) * exp(-bb * (T - Tb) / deltab) - ((T - Tb) / (Tm - Tb)) * exp(bb * (Tm - Tb) - (Tm - T) / deltam));
		}
		else if (model == Analytis_1977)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double Tmin = P[P3];
			double Tmax = P[P4];
			rT = aa * pow((T - Tmin), bb) * pow((Tmax - T), cc);
		}
		//else if (model == Bayoh_2003)
		//{
		//	double aa = P[P0];
		//	double bb = P[P1];
		//	double cc = P[P2];
		//	double dd = P[P3];
		//	rT = aa + bb * T + cc * exp(T) + dd * exp(-T);
		//}
		else if (model == Shi_beta_2016)
		{
			double rm = P[P0];
			double Tm = P[P1];
			double T1 = P[P2];
			double T2 = P[P3];
			rT = rm * (T2 - T) / (T2 - Tm) * pow(((T - T1) / (Tm - T1)), ((Tm - T1) / (T2 - Tm)));
		}
		else if (model == Yin_beta_1995)
		{
			double aa = P[P0];
			double bb = P[P1];
			double mu = P[P2];
			double Tb = P[P3];
			double Tc = P[P4];

			rT = exp(mu) * pow(T - Tb, aa) * pow(Tc - T, bb);
		}
		else if (model == Bieri_1983)
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			rT = aa * (T - Tmin) - (bb * exp(T - Tmax));
		}
		else if (model == Bieri_1983)
		{
			double aa = P[P0];
			double bb = P[P1];
			double Tmin = P[P2];
			double Tmax = P[P3];

			rT = aa * (T - Tmin) - (bb * exp(T - Tmax));
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

			rT = aa / (1 + bb * T + cc * T*T);
		}
		else if (model == Lamb_1992)
		{
			double Rm = P[P0];
			double Tmax = P[P1];
			double To = P[P2];

			rT = Rm * exp(-1 / 2 * pow((T - Tmax) / To, 2.0));
		}
		else if (model == Shi_Perf2_2011)
		{
			double cc = P[P0];
			double k = P[P1];
			double T1 = P[P2];
			double T2 = P[P3];

			rT = cc * (T - T1) * (1 - exp(k * (T - T2)));
		}
		//else if (model == Rootsq_1982)
		//{
		//	double bb = P[P0];
		//	double Tb = P[P1];
		//	rT = pow(bb * (T - Tb), 2.0);
		//}
		else if (model == Schoolfield_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double bb = P[P2];
			double cc = P[P3];
			double dd = P[P4];
			double ee = P[P5];

			rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(bb / 1.987 * (1 / cc - 1 / (T + 273.16))) + exp(dd / 1.987 * (1 / ee - 1 / (T + 273.16))));
		}
		else if (model == SchoolfieldHigh_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double dd = P[P2];
			double ee = P[P3];

			rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(dd / 1.987 * (1 / ee - 1 / (T + 273.16))));
		}
		else if (model == SchoolfieldLow_1981)
		{
			double p25 = P[P0];
			double aa = P[P1];
			double bb = P[P2];
			double cc = P[P3];

			rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(bb / 1.987 * (1 / cc - 1 / (T + 273.16))));
		}
		else if (model == Shi_2011)
		{
			double cc = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double T1 = P[P3];
			double T2 = P[P4];

			rT = cc * (1 - exp(-k1 * (T - T1))) * (1 - exp(k2 * (T - T2)));
		}
		else if (model == Wagner_1988)
		{
			double aa = P[P0];
			double bb = P[P1];
			double cc = P[P2];
			double dd = P[P3];

			rT = 1 / ((1 + exp((cc / 1.987) * ((1 / dd) - (1 / (T + 273.16))))) / (aa * (T + 273.16) / 298.15 * exp((bb / 1.987) * ((1 / 298.15) - 1 / (T + 273.16)))));
		}
		else if (model == Wang_1982)
		{
			double K = P[P0];
			double r = P[P1];
			double aa = P[P2];
			double Tl = P[P3];
			double Th = P[P4];
			double To = P[P5];

			T += 273.16;
			rT = (K / (1 + exp(-r * (T - To)))) * (1 - exp(-(T - Tl) / aa)) * (1 - exp(-(Th - T) / aa));
		}
		else if (model == Wangengel_1998)
		{
			double aa = P[P0];
			double Tmin = P[P1];
			double Topt = P[P2];
			rT = (2 * pow(T - Tmin, aa) * pow(Topt - Tmin, aa) - pow(T - Tmin, 2 * aa)) / pow(Topt - Tmin, 2 * aa);
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

	//CSAParameterVector CDevRateEquation::GetParameters_old(TDevRateEquation model)
	//{
	//	CSAParameterVector p;

	//	//  Sharpe&DeMichele sin p y To
	//	//if (model == SharpeDeMichele1)
	//	//{
	//	//	p.push_back(CSAParameter("Ha", 0.2, 0.001, 1000));
	//	//	p.push_back(CSAParameter("Hl", 2, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Tl", 5, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Th", 4, 0, 40));
	//	//}
	//	////  Sharpe&DeMichele con To
	//	//else if (model == SharpeDeMichele2)
	//	//{
	//	//	//f < -function(x, To, Ha, Hl, Tl, Hh, Th)
	//	//	p.push_back(CSAParameter("To", 0.2, 0.001, 1000));
	//	//	p.push_back(CSAParameter("Ha", 0.2, 0.001, 1000));
	//	//	p.push_back(CSAParameter("Hl", 2, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Tl", 5, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//	//	p.push_back(CSAParameter("Th", 4, 0, 40));
	//	//}
	//	////  Sharpe&DeMichele con To y p
	//	//else 
	//	if (model == SharpeDeMichele3)
	//	{
	//		//f < -function(x, p, To, Ha, Hl, Tl, Hh, Th)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("To", 0.2, 0.001, 50));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hl", 2, 0.001, 50));
	//		p.push_back(CSAParameter("Tl", 5, 0.001, 50));
	//		p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//		p.push_back(CSAParameter("Th", 4, 1, 50));
	//		//rT = (p * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
	//			//(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
	//	}
	//	//  Sharpe&DeMichele solo con To, p y Ha
	//	else if (model == SharpeDeMichele4)
	//	{
	//		//f < -function(x, p, To, Ha)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("To", 0.2, 0.001, 50));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 1000000));
	//	}
	//	//  Sharpe&DeMichele con To, p, Ha, Hl, Tl
	//	else if (model == SharpeDeMichele5)
	//	{
	//		//f < -function(x, p, To, Ha, Hl, Tl)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("To", 0.2, 0.001, 50));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hl", 2, 0.001, 50));
	//		p.push_back(CSAParameter("Tl", 5, 0.001, 50));
	//	}
	//	//  Sharpe&DeMichele con To, p, Ha, Hh, Th
	//	else if (model == SharpeDeMichele6)
	//	{
	//		//			f < -function(x, p, To, Ha, Hh, Th)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("To", 0.2, 0.001, 50));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//		p.push_back(CSAParameter("Th", 4, 1, 50));
	//	}
	//	//  Sharpe&DeMichele con To, Ha
	//	//else if (model == SharpeDeMichele7)
	//	//{
	//	//	//f < -function(x, To, Ha)
	//	//}
	//	////  Sharpe&DeMichele con To, Ha, Hl, Tl
	//	//else if (model == SharpeDeMichele8)
	//	//{
	//	//	//f < -function(x, To, Ha, Hl, Tl)
	//	//	//rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
	//	//		//(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))));
	//	//}
	//	////  Sharpe&DeMichele con To, Ha, Hh, Th
	//	//else if (model == SharpeDeMichele9)
	//	//{
	//	//	//f < -function(x, To, Ha, Hh, Th)
	//	//	//rT = ((coefi1 + coefi2 * To) * (x / (To)) * exp((Ha / 1.987) * ((1 / To) - (1 / x)))) /
	//	//		//(1 + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
	//	//}
	//	//  Sharpe&DeMichele con To, Hh y Th, constantes
	//	else if (model == SharpeDeMichele10)
	//	{
	//		//f < -function(x, p, Ha, Hl, Tl)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hl", 2, 0.001, 50));
	//		p.push_back(CSAParameter("Tl", 5, 0.001, 50));
	//	}
	//	else if (model == SharpeDeMichele11)
	//	{
	//		//f < -function(x, p, Ha, Hl, Tl, Hh, Th)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hl", 2, 0.001, 50));
	//		p.push_back(CSAParameter("Tl", 5, 0.001, 50));
	//		p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//		p.push_back(CSAParameter("Th", 4, 1, 50));
	//	}
	//	//else if (model == SharpeDeMichele12)
	//	//{
	//	//	//f < -function(x, Ha, Hl, Tl, Hh, Th)
	//	//	//rT = ((coefi1 + coefi2 * 298.16) * (x / 298.16) * exp((Ha / 1.987) * ((1 / 298.16) - (1 / x)))) /
	//	//		//(1 + exp((Hl / 1.987) * ((1 / Tl) - (1 / x))) + exp((Hh / 1.987) * ((1 / Th) - (1 / x))));
	//	//}
	//	else if (model == SharpeDeMichele13)
	//	{
	//		//			f < -function(x, p, Ha, Hl, Tl)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hl", 2, 0.001, 50));
	//		p.push_back(CSAParameter("Tl", 5, 0.001, 50));
	//	}
	//	else if (model == SharpeDeMichele14)
	//	{
	//		//		f < -function(x, p, Ha, Hh, Th)
	//		p.push_back(CSAParameter("p", 0.01, 0.001, 1000));
	//		p.push_back(CSAParameter("Ha", 0.2, -1000, 100000));
	//		p.push_back(CSAParameter("Hh", 0.15, 0.001, 10000));
	//		p.push_back(CSAParameter("Th", 4, 1, 50));
	//	}
	//	//  deva no lineal o Higgis
	//	else if (model == DevaHiggis)
	//	{
	//		//		f < -function(x, b1, b2, b3, b4, b5)
	//		p.push_back(CSAParameter("b1", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b2", 10, 0, 50));
	//		p.push_back(CSAParameter("b3", 25, 0, 50));
	//		p.push_back(CSAParameter("b4", 0.2, 0.0001, 1000));//need be positive
	//		p.push_back(CSAParameter("b5", 0.2, 0.0001, 1000));
	//		//			rT = b1 * pow(10, (-((((x - b3) / (b3 - b2) - (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)))) + exp(b4*((x - b3) / (b3 - b2) - (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)))))) / ((1 + b4) / (1 + 1.5*b4 + 0.39*b4 ^ 2))) ^ 2)*(1 - b5 + b5 * ((((x - b3) / (b3 - b2) - (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)))) + exp(b4*((x - b3) / (b3 - b2) - (1 / (1 + 0.28*b4 + 0.72*log(1 + b4)))))) / ((1 + b4) / (1 + 1.5*b4 + 0.39*b4 ^ 2))) ^ 2));
	//	}
	//	//  Logan 1
	//	else if (model == Logan6_1976)
	//	{
	//		//f < -function(x, phi, bb, Tmax, deltaT)
	//		p.push_back(CSAParameter("phi", 0.12, 0.0001, 100));
	//		p.push_back(CSAParameter("bb", 0.14, 0.0001, 100));
	//		p.push_back(CSAParameter("Tmax", 34.6, 0, 50));
	//		p.push_back(CSAParameter("deltaT", 4.6, 0.01, 100));
	//		//rT = Y * (exp(p*x) - exp(p*Tmax - (Tmax - x) / v));
	//	}
	//	//  Logan 2
	//	else if (model == Logan10_1976)
	//	{
	//		//f < -function(x, phi, bb, Tmax, deltaT)
	//		p.push_back(CSAParameter("alpha", 0.3, 0.0001, 100));
	//		p.push_back(CSAParameter("bb", 0.0, -100, 100));
	//		p.push_back(CSAParameter("cc", 110, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tmax", 36.7, 0, 50));
	//		p.push_back(CSAParameter("deltaT", 3.7, 0.01, 100));
	//		//rT = alpha * (1 / (1 + cc * exp(-bb * T)) - exp(-((Tmax - T) / deltaT)))
	//	}
	//	//  Briere 1
	//	else if (model == Briere1_1999)
	//	{
	//		//f < -function(x, aa, To, Tmax)
	//		p.push_back(CSAParameter("aa", 0.8, 0.00001, 100));
	//		p.push_back(CSAParameter("To", 9.1, 0, 50));
	//		p.push_back(CSAParameter("Tmax", 37.3, 0, 50));
	//		//rT = aa * x*(x - To)*pow((Tmax - x), 0.5);
	//	}
	//	else if (model == Briere2_1999)
	//	{
	//		p.push_back(CSAParameter("aa", 1.2, 0.00001, 100));
	//		p.push_back(CSAParameter("bb", 2.8, 0.0001, 100));
	//		p.push_back(CSAParameter("To", 7.0, -20, 50));
	//		p.push_back(CSAParameter("Tmax", 35, 0, 50));
	//		//rT = aa * x*(x - To)*pow((Tmax - x), 1 / bb);
	//	}
	//	else if (model == HilberLogan_1983)
	//	{
	//		//f < -function(x, d, Y, Tmax, v)
	//		p.push_back(CSAParameter("phi", 0.3, 0.0001, 1000));
	//		p.push_back(CSAParameter("d", 10, 0.001, 1000));
	//		p.push_back(CSAParameter("Tmax", 35, 0, 50));
	//		p.push_back(CSAParameter("deltaT", 6, 0.0001, 1000));
	//		//rT = phi * ((x*x) / ((x*x) + (d*d)) - exp(-(Tmax - (x)) / deltaT));
	//	}
	//	//Hilber y logan 2
	//	//same as logan1 except D²
	//	//else if (model == HilberLogan2)
	//	//{
	//	//	//f < -function(x, trid, D, Tmax, Dt) 
	//	//	p.push_back(CSAParameter("phi", 10, 0.0001, 1000));
	//	//	p.push_back(CSAParameter("D", 0.2, 0.001, 1000));
	//	//	p.push_back(CSAParameter("Tmax", 35, 1, 50));
	//	//	p.push_back(CSAParameter("deltaT", 6, 0.001, 1000));
	//	//	//rT = phi * ((x * x) / (x * x + D) - exp(-(Tmax - x) / deltaT)); //Cambio de "x/Dt" por "x)/Dt"
	//	//}
	//	else if (model == HilberLoganIII)//1983
	//	{
	//		p.push_back(CSAParameter("phi", 0.5, 0.001, 100));
	//		p.push_back(CSAParameter("aa", 312, 0.001, 1000));
	//		p.push_back(CSAParameter("Tb", 5, 0.01, 50));
	//		p.push_back(CSAParameter("Tmax", 35, 1, 50));
	//		p.push_back(CSAParameter("deltaT", 6, 0.001, 100));
	//		//		rT = phi * ((pow(T - Tb, 2.0) / (pow(T - Tb, 2.0) + aa * aa)) - exp(-(Tmax - (T - Tb)) / deltaT));
	//	}
	//	else if (model == Poly1)
	//	{
	//		//f < -function(x, a, b)
	//		p.push_back(CSAParameter("a", -0.05, -10, 10));
	//		p.push_back(CSAParameter("b", 1, 0.0001, 10));
	//		//rT = a * x + b;
	//	}
	//	else if (model == Poly2)
	//	{
	//		p.push_back(CSAParameter("a0", 0, -100000, 100000));
	//		p.push_back(CSAParameter("a1", -0.003, -10000, 10000));
	//		p.push_back(CSAParameter("a2", 0.0004, -1000, 1000));
	//		//rT = a0 + a1 * T + a2 * T *T;
	//	}
	//	else if (model == Poly3)//Tanigoshi/HarcourtYee
	//	{
	//		//f < -function(x, a0, a1, a2, a3)
	//		p.push_back(CSAParameter("a0", 0.6, -100000, 100000));
	//		p.push_back(CSAParameter("a1", -0.1, -10000, 10000));
	//		p.push_back(CSAParameter("a2", 0.007, -1000, 1000));
	//		p.push_back(CSAParameter("a3", -0.00013, -100, 100));
	//		//rT = a0 + a1 * x + a2 * x * x + a3 * x * x* x;
	//	}
	//	else if (model == Poly4)
	//	{
	//		p.push_back(CSAParameter("a0", 0.2, -100000, 100000));
	//		p.push_back(CSAParameter("a1", -0.02, -10000, 10000));
	//		p.push_back(CSAParameter("a2", 0.002, -1000, 1000));
	//		p.push_back(CSAParameter("a3", -0.0002, -100, 100));
	//		p.push_back(CSAParameter("a4", 0.00002, -10, 10));

	//		//rT = a0 + a1 * T + a2 * T *T + a3 * T *T*T + a4 * T *T*T*T;
	//	}
	//	//  exponential simple
	//	else if (model == Exponential)
	//	{
	//		//f < -function(x, b1, b2)
	//		p.push_back(CSAParameter("b1", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b2", 0.2, 0.0001, 1000));
	//		//rT = b1 * exp(b2*x);
	//	}
	//	//  Tb Model (Logan)
	//	else if (model == LoganTb)
	//	{
	//		//f < -function(x, sy, b, Tb, DTb)
	//		p.push_back(CSAParameter("sy", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tb", 15, 0, 50));
	//		p.push_back(CSAParameter("DTb", 0.2, 0.0001, 1000));
	//		//rT = sy * exp(b*(x - Tb) - exp(b*(x - Tb) / DTb));
	//	}
	//	//  Exponential Model (Logan)
	//	else if (model == LoganExponential)
	//	{
	//		//f < -function(x, sy, b, Tb)
	//		p.push_back(CSAParameter("sy", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tb", 15, 0, 50));
	//		//rT = sy * exp(b*(x - Tb));

	//	}
	//	//  Square root model of Ratkowsky
	//	else if (model == RatkowskySquare)
	//	{
	//		//f < -function(x, b, Tb)
	//		p.push_back(CSAParameter("b", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tb", 30, 0, 50));
	//		//rT = b * pow((x - Tb), 2.0);
	//	}
	//	//Ratkowsky2
	//	else if (model == Ratkowsky_1983)
	//	{
	//		//f < -function(x, aa, Tmin, Tmax, b)
	//		p.push_back(CSAParameter("aa", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tmin", 10, 1, 50));
	//		p.push_back(CSAParameter("Tmax", 30, 1, 50));
	//		//rT = pow(aa*(x - Tmin)*(1 - exp((b*(Tmax - x)))), 2.0); //Agregar 1-exp
	//	}
	//	//Davidson
	//	else if (model == Davidson_1944)
	//	{
	//		//f < -function(x, k, a, b)
	//		p.push_back(CSAParameter("k", 0.4, 0.0001, 100));
	//		p.push_back(CSAParameter("a", 4.5, 0.0001, 100));
	//		p.push_back(CSAParameter("b", -0.21, -100, -0.0001));
	//		//rT = k / (1 + exp(a - b * x)); //DM: Se cambio "+b*x" por "-b*x"
	//	}
	//	//Pradham1
	//	else if (model == Pradham)
	//	{
	//		//f < -function(x, R, Tm, To)
	//		p.push_back(CSAParameter("R", 0.5, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tm", 10, 1, 50));
	//		p.push_back(CSAParameter("To", 8, 1, 50));
	//		//rT = R * pow(exp((-1 / 2)*((x - Tm) / To)), 2.0); //DM: Se agrego "^2"
	//	}
	//	//Allahyari
	//	else if (model == Allahyari)
	//	{
	//		//f < -function(x, p, Tmax, Tmin, n, m)
	//		p.push_back(CSAParameter("p", 0.2, 0.001, 10));
	//		p.push_back(CSAParameter("Tmin", 12, 1, 50));
	//		p.push_back(CSAParameter("Tmax", 25, 1, 50));
	//		p.push_back(CSAParameter("n", 1, 0.001, 10));
	//		p.push_back(CSAParameter("m", 1, 0.001, 10));
	//		//rT = p * (pow((x - Tmin) / (Tmax - Tmin), n))*(1 - pow((x - Tmin) / (Tmax - Tmin), m)); //CAMBIO
	//	}
	//	//Kontodimas
	//	else if (model == Kontodimas_2004)
	//	{
	//		//f < -function(x, aa, Tmin, Tmax)
	//		p.push_back(CSAParameter("aa", 1.6e-5, 1e-7, 1));
	//		p.push_back(CSAParameter("Tmin", 8, 1, 50));
	//		p.push_back(CSAParameter("Tmax", 42, 1, 50));
	//		//rT = aa * (pow(x - Tmin, 2.0))*(Tmax - x);
	//	}
	//	//Janish1
	//	else if (model == Janisch1_1932)
	//	{
	//		//f < -function(x, Dmin, Topt, K) 
	//		p.push_back(CSAParameter("Dmin", 28, 1, 50));
	//		p.push_back(CSAParameter("Topt", 33, 1, 50));
	//		p.push_back(CSAParameter("K", 0.2, 0.0001, 10));
	//		//rT = 2 / (Dmin*(exp(K*(x - Topt)) + exp((-K)*(x - Topt))));
	//	}
	//	//Janish2
	//	else if (model == Janisch2)
	//	{
	//		//f < -function(x, c, a, b, Tm)
	//		p.push_back(CSAParameter("c", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("a", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("b", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("Tm", 15, 1, 50));
	//		//rT = 2 * c / (pow(a, (x - Tm)) + pow(b, (Tm - x)));
	//	}
	//	//  Stinner 1
	//	else if (model == Stinner1)
	//	{
	//		//f < -function(xp, Rmax, Topc, k1, k2) 
	//		p.push_back(CSAParameter("Rmax", 0.2, 0.001, 10));
	//		p.push_back(CSAParameter("Topc", 15, 0, 50));
	//		p.push_back(CSAParameter("k1", 0.2, -10, 10));
	//		p.push_back(CSAParameter("k2", 0.2, -10, 10));
	//		//rT = (Rmax*(1 + exp(k1 + k2 * Topc))) / (1 + exp(k1 + k2 * x));
	//	}
	//	//Stinner2
	//	else if (model == Stinner2)
	//	{
	//		//f < -function(x, Rmax, k1, k2, Topc)
	//		p.push_back(CSAParameter("Rmax", 1, 0.0001, 1000));
	//		p.push_back(CSAParameter("k1", 1, -1000, 1000));
	//		p.push_back(CSAParameter("k2", 1, -1000, 1000));
	//		p.push_back(CSAParameter("Topc", 15, 1, 50));
	//		//rT = Rmax * (exp(k1 + k2 * Topc)) / (1 + exp(k1 + k2 * x));
	//	}
	//	//Stinner 3
	//	else if (model == Stinner_1974)//1974
	//	{
	//		//f < -function(x, c1, k1, k2)
	//		p.push_back(CSAParameter("c1", 0.02, 0.0001, 1000));
	//		p.push_back(CSAParameter("k1", 0.2, -1000, 1000));
	//		p.push_back(CSAParameter("k2", 0.2, -1000, 1000));
	//		//rT = c1 / (1 + exp(k1 + k2 * x));
	//	}
	//	//Stinner-4
	//	else if (model == Stinner4)
	//	{
	//		//f < -function(x, c1, c2, k1, k2, To)
	//		p.push_back(CSAParameter("c1", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("c2", 0.2, 0.0001, 1000));
	//		p.push_back(CSAParameter("k1", 0.2, -1000, 1000));
	//		p.push_back(CSAParameter("k2", 0.2, -1000, 1000));
	//		p.push_back(CSAParameter("To", 15, 1, 50));
	//		//rT = c1 / (1 + exp(k1 + k2 * x)) + c2 / (1 + exp(k1 + k2 * (2 * To - x)));
	//	}
	//	//Taylor
	//	//DM: se quito Smin
	//	else if (model == Taylor_1981)
	//	{
	//		//f < -function(x, rm, Topt, Troh)
	//		p.push_back(CSAParameter("rm", 0.08, 0.0001, 1.0));
	//		p.push_back(CSAParameter("Topt", 32, 1, 50));
	//		p.push_back(CSAParameter("Troh", 9.4, 1, 50));
	//		//rT = rm * exp(-(0.5)*pow(-(x - Topt) / Troh, 2.0));
	//	}
	//	else if (model == Lactin1_1995)
	//	{
	//		p.push_back(CSAParameter("aa", 0.016, 0.0001, 100));
	//		p.push_back(CSAParameter("Tmax", 37, 1, 50));
	//		p.push_back(CSAParameter("deltaT", 8, 0.0001, 100));
	//		//rT = exp(aa * T) - exp(aa * Tmax - (Tmax - T) / deltaT);
	//	}
	//	//lactin2 1995
	//	else if (model == Lactin2_1995)
	//	{
	//		p.push_back(CSAParameter("aa", 0.03, 0.0001, 10));
	//		p.push_back(CSAParameter("bb", -1.0, -10, 10));
	//		p.push_back(CSAParameter("Tmax", 41, 1, 60));
	//		p.push_back(CSAParameter("deltaT", 4, 0.0001, 100));
	//		//rT = exp(aa * T) - exp(aa * Tmax - (Tmax - T) / deltaT) + bb;
	//	}
	//	else if (model == Regniere_1987)
	//	{
	//		p.push_back(CSAParameter("b1", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("b2", 2, 0.001, 1000));
	//		p.push_back(CSAParameter("b3", 5, 0.001, 1000));
	//		p.push_back(CSAParameter("b4", 0.15, 0.001, 1000));
	//		p.push_back(CSAParameter("Tb", 10, 0, 50));
	//		p.push_back(CSAParameter("Tm", 30, 0, 50));
	//	}
	//	else if (model == Analytis_1977)
	//	{
	//		p.push_back(CSAParameter("aa", 5E-4, 1.0E-10, 0.1));
	//		p.push_back(CSAParameter("bb", 2.5, 0.001, 100));
	//		p.push_back(CSAParameter("cc", 0.35, 0.001, 100));
	//		p.push_back(CSAParameter("Tmin", 2.3, -50, 50));
	//		p.push_back(CSAParameter("Tmax", 35, 0, 50));
	//		//rT = aa * pow((T - Tmin), bb) * pow((Tmax - T), cc);
	//	}
	//	//else if (model == Bayoh_2003)
	//	//{
	//	//	p.push_back(CSAParameter("aa", -0.05, -100, 100));
	//	//	p.push_back(CSAParameter("bb", 0.05, 0.0001, 100));
	//	//	p.push_back(CSAParameter("cc", 1E-10, 1E-16, 1));
	//	//	p.push_back(CSAParameter("dd", 3E5, 0, 1E10));
	//	//	//	rT = aa + bb * T + cc * exp(T) + dd * exp(-T);
	//	//}
	//	else if (model == Shi_beta_2016)
	//	{
	//		p.push_back(CSAParameter("rm", 0.02, 0.001, 10));
	//		p.push_back(CSAParameter("Tm", 15, 10, 20));
	//		p.push_back(CSAParameter("T1", 05, 0, 15));
	//		p.push_back(CSAParameter("T2", 30, 20, 40));
	//		//rT = rm * (T2 - T) / (T2 - Tm) * pow(((T - T1) / (Tm - T1)), ((Tm - T1) / (T2 - Tm)));
	//	}
	//	else if (model == Yin_beta_1995)
	//	{
	//		p.push_back(CSAParameter("aa", 1, 1, 4));
	//		p.push_back(CSAParameter("bb", 1, 1, 4));
	//		p.push_back(CSAParameter("mu", -10.0, -100, 0));
	//		p.push_back(CSAParameter("Tb", 05, 0, 10));
	//		p.push_back(CSAParameter("Tc", 35, 30, 50));
	//		//rT = exp(mu) * pow(T - Tb, aa) * pow(Tc - T, bb);
	//	}
	//	else if (model == Bieri_1983)
	//	{
	//		p.push_back(CSAParameter("aa", 0.015, 0.00001, 0.1));
	//		p.push_back(CSAParameter("bb", 3.6, 0.0001, 100));
	//		p.push_back(CSAParameter("Tmin", 4, 0, 50));
	//		p.push_back(CSAParameter("Tmax", 42, 0, 50));
	//		//rT = aa * (T - Tmin) - (bb * exp(T - Tmax));
	//	}
	//	else if (model == Damos_2008)
	//	{
	//		p.push_back(CSAParameter("aa", 7.5E-4, 1E-10, 0.1));
	//		p.push_back(CSAParameter("bb", 3.8, 0.01, 100));
	//		p.push_back(CSAParameter("cc", 5.0, 0.01, 100));

	//		//rT = aa * (bb - T / 10) * pow(T / 10, cc);
	//	}
	//	else if (model == Damos_2011)
	//	{
	//		p.push_back(CSAParameter("aa", 0.01, -10, 10));
	//		p.push_back(CSAParameter("bb", -0.1, -10, 10));
	//		p.push_back(CSAParameter("cc", 0.01, -10, 10));
	//		//rT = aa / (1 + bb * T + cc * T*T);
	//	}
	//	else if (model == Lamb_1992)
	//	{
	//		p.push_back(CSAParameter("Rm", 0.004, 0.0001, 10));
	//		p.push_back(CSAParameter("Tmax", 12, 0, 50));
	//		p.push_back(CSAParameter("To", 4, 0.1, 50));

	//		//rT = Rm * exp(-1 / 2 * pow((T - Tmax) / To, 2.0));
	//	}
	//	else if (model == Shi_Perf2_2011)
	//	{
	//		p.push_back(CSAParameter("cc", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("k", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("T1", 15, 0, 50));
	//		p.push_back(CSAParameter("T2", 30, 0, 50));

	//		//rT = cc * (T - T1) * (1 - exp(k * (T - T2)));
	//	}
	//	else if (model == Rootsq_1982)
	//	{
	//		p.push_back(CSAParameter("bb", -0.01, -100, 100));
	//		p.push_back(CSAParameter("Tn", -2.8, -50, 50));

	//		//rT = pow(bb * (T - Tb), 2.0);
	//	}
	//	else if (model == Schoolfield_1981)
	//	{
	//		p.push_back(CSAParameter("p25", 0.2, 0.0001, 100));
	//		p.push_back(CSAParameter("aa", 0.2, 0, 100000));
	//		p.push_back(CSAParameter("bb", -0.2, -100000, 0));
	//		p.push_back(CSAParameter("cc", 300, 250, 350));
	//		p.push_back(CSAParameter("dd", 0.2, 0, 1000000));
	//		p.push_back(CSAParameter("ee", 300, 200, 400));

	//		//rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(bb / 1.987 * (1 / cc - 1 / (T + 273.16))) + exp(dd / 1.987 * (1 / ee - 1 / (T + 273.16))));
	//	}
	//	else if (model == SchoolfieldHigh_1981)
	//	{
	//		p.push_back(CSAParameter("p25", 0.1, -1000, 1000));
	//		p.push_back(CSAParameter("aa", 0.2, 0, 100000));
	//		p.push_back(CSAParameter("dd", -0.2, -100000, 0));
	//		p.push_back(CSAParameter("ee", 280, 250, 350));



	//		//rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(dd / 1.987 * (1 / ee - 1 / (T + 273.16))));
	//	}
	//	else if (model == SchoolfieldLow_1981)
	//	{
	//		p.push_back(CSAParameter("p25", 0.2, 0.00001, 1000));
	//		p.push_back(CSAParameter("aa", 0.2, 0, 100000));
	//		p.push_back(CSAParameter("bb", -0.2, -100000, 0));
	//		p.push_back(CSAParameter("cc", 280, 250, 350));
	//		//rT = (p25 * (T + 273.16) / 298 * exp(aa / 1.987 * (1 / 298 - 1 / (T + 273.16)))) / (1 + exp(bb / 1.987 * (1 / cc - 1 / (T + 273.16))));
	//	}
	//	else if (model == Shi_2011)
	//	{
	//		p.push_back(CSAParameter("cc", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("k1", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("k2", 0.2, 0.001, 1000));
	//		p.push_back(CSAParameter("T1", 10, 0, 40));
	//		p.push_back(CSAParameter("T2", 30, 0, 40));

	//		//rT = cc * (1 - exp(-k1 * (T - T1))) * (1 - exp(k2 * (T - T2)));
	//	}
	//	else if (model == Wagner_1988)
	//	{
	//		p.push_back(CSAParameter("aa", 0.1, 0.0001, 10));
	//		p.push_back(CSAParameter("bb", 28500, 1, 1E6));
	//		p.push_back(CSAParameter("cc", 130000, 1, 2E6));
	//		p.push_back(CSAParameter("dd", 305, 250, 350));

	//		//rT = 1 / ((1 + exp((cc / 1.987) * ((1 / dd) - (1 / (T + 273.16))))) / (aa * (T + 273.16) / 298.15 * exp((bb / 1.987) * ((1 / 298.15) - 1 / (T + 273.16)))));
	//	}
	//	else if (model == Wang_1982)
	//	{
	//		p.push_back(CSAParameter("K", 0.04, 0.0001, 10));
	//		p.push_back(CSAParameter("r", 0.23, 0.0001, 10));
	//		p.push_back(CSAParameter("aa", 2.3, 1, 10));
	//		p.push_back(CSAParameter("Tl", 273 + 11, 273, 273 + 50));
	//		p.push_back(CSAParameter("Th", 273 + 39, 273, 273 + 50));
	//		p.push_back(CSAParameter("To", 273 + 10, 273, 273 + 50));
	//		//rT = (K / (1 + exp(-r * (T - To)))) * (1 - exp(-(T - Tl) / aa)) * (1 - exp(-(Th - T) / aa));
	//	}
	//	else if (model == Wangengel_1998)
	//	{
	//		p.push_back(CSAParameter("aa", 1, 0.01, 100));
	//		p.push_back(CSAParameter("Tmin", 10, 0, 50));
	//		p.push_back(CSAParameter("Topt", 30, 0, 50));
	//		//rT = (2 * pow(T - Tmin, aa) * pow(Topt - Tmin, aa) - pow(T - Tmin, 2 * aa)) / pow(Topt - Tmin, 2 * aa);
	//	}

	//	return p;
	//}

}






