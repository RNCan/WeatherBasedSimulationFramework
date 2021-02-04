#include "DevRateEquation.h"
#include <functional>

using namespace std;
//**************************************************************************************************
//Equation translate to C++ from ILCYM: https://research.cip.cgiar.org/confluence/display/ilcym/Downloads

//Allahyari H. Decision making with degree-day in control program of Colorado potato beetle. PhD dissertation, University of Tehran, Tehran, Iran. 2005.
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

//Gompertz–Makeham 
//Lobry–Rosso–Flandrois (LRF) model (Lobry et al. 1991; Rosso et al. 1993): 


namespace WBSF
{
	const char* CDevRateEquation::EQUATION[NB_EQUATIONS][NB_INFO] =
	{
		{ "Allahyari_2005","beta=pmax(0,T-Tb)/(Tm-Tb);psi*(beta^k1)*(1-beta^k2)","psi=0.02[1E-5,1E4]|k1=1[0.1,4]|k2=1[0.1,4]|Tb=1[-50,50]|Tm=40[0,100]", "list(psi~bgroup('(',beta^k[1],')')~bgroup('(',1-beta^k[2],')'),~~~scriptstyle(beta~' = '~over(T~-~T[b],T[m]~-~T[b])))"},
		{ "Analytis_1977","psi*pmax(0,T-Tb)^k1*pmax(0,Tm-T)^k2","psi=5E-4[1.0E-10,0.1]|k1=0.5[1E-5,4]|k2=0.35[0.001,4]|Tb=2.3[-50,50]|Tm=35[0,50]", "psi~bgroup('(',T-T[b],')')^k[1]~bgroup('(',T[m]-T,')')^k[2]"},
		{ "Angilletta_2006","psi*exp(-0.5*(abs(T-To)/deltaT)^k)","psi=1[-1E5,1E5]|deltaT=1[-1E5,1E5]|k=3.8[1,4]|To=27[0,50]", "psi~e^{~-~over(1,2)~bgroup('|',over(T~-~T[o],Delta[T]),'|')^~k}"},
		{ "Bieri_1983","k1*(T-Tb)-(k2*exp(T-Tm))","k1=0.015[1E-5,10]|k2=3.6[1E-6,100]|Tb=10[-50,15]|Tm=42[0,50]", "bgroup('[',k[1]~bgroup('(',T-T[b],')'),']')~-~bgroup('[',k[2]~e^{T-T[m]},']')"},
		{ "Briere1_1999","psi*T*pmax(0,T-Tb)*pmax(0,Tm-T)^(1/2)","psi=1E-4[1E-7,1]|Tb=9.1[-50,50]|Tm=37.3[0,50]", "psi~T~bgroup('(',T-T[b],')')~bgroup('(',T[m]-T,')')^~over(1,2)"},
		{ "Briere2_1999","psi*T*pmax(0,T-Tb)*pmax(0,Tm-T)^(1/k)","psi=1E-4[1E-7,1]|k=2.8[1,10]|Tb=7.0[-50,50]|Tm=35[0,50]", "psi~T~bgroup('(',T-T[b],')')~bgroup('(',T[m]-T,')')^~over(1,k)"},
		{ "Damos_2008","psi*(k1-T/10)*(T/10)^k2","psi=0.01[1E-5,0.5]|k1=5[0.001,10]|k2=2.0[0,4]", "psi~bgroup('(',k-over(T,10),')')~bgroup('(',over(T,10),')')^k"},
		{ "Damos_2011","psi*(1/(1+k1*T+k2*T^2))","psi=0.01[0,1E4]|k1=-0.08[-0.5,0]|k2=0.006[1e-5,1E-2]", "psi~bgroup('(',over(1,1+k[1]~T+k[2]~T^~2), ')')"},
		{ "Deutsch_2008","ifelse(T<=To, psi*exp(-k*(T-To)^2),psi*(1-((T-To)/(To-Tm))^2))","psi=0.08[0.0001,1.0]|k=1[0,100]|To=25[0,50]|Tm=35[0,50]", "bgroup('{',atop(psi~bgroup('[',e^{-~k~(T~-~T[o])^{~2}},']'),psi~bgroup('[',1-bgroup('(',over(T~-~T[o],T[o]~-~T[m]),')')^2,']')),'')~~~~atop(T<=T[o],T>T[o])" },
		{ "Deva&Higgis", "beta1=(pmax(Tb,pmin(Tm,T))-Tm)/(Tm-Tb)-(1/(1+0.28*k1+0.72*log(1+k1)));beta2=(1+k1)/(1+1.5*k1+0.39*k1^2);Omega=((beta1+exp(k1*beta1))/beta2)^2;psi*10^-Omega*(1-k2+k2*Omega)", "psi=0.3[0.0001,10]|k1=2[0.0001,10]|k2=0.8[0.0001,10]|Tb=0[-50,50]|Tm=25[0,50]", "list(psi~bgroup('[',10^{~-~Omega}~bgroup('(',1-k[2]+k[2]~Omega,')'),']'),~Omega==bgroup('(',over(beta[1]+e^{~k[1]~beta[1]},beta[2]),')')^2, atop(~~beta[1]==bgroup('(',over(T~-~T[m],T[m]~-~T[b]),')')~-~bgroup('(',over(1,1~+~0.28~k[1]~+~0.72~ln~(1+k[1])),')'),beta[2]==over(1+k[1],1~+~1.5~k[1]~+~0.39~k[1]^~2)))"},
		{ "Hansen_2011","psi*((exp(k*(T-Tb))-1)-(exp(k*(Tm-Tb))-1)*exp((T-Tm)/deltaT))","psi=0.5[1E-5,10]|k=0.01[1E-5,10]|Tb=5[0,50]|Tm=35[0,50]|deltaT=2[1E-5,10]", "psi~bgroup('{',bgroup('[',e^{k~(T-T[m])}-1,']')~-~bgroup('[',e^{k~(T[m]-T[b])}-1,']')~e^{bgroup('(',over(T~-~T[m], Delta[T]) ,')')}, '}')" },
		{ "Hilbert&Logan_1983","psi*((pmax(0,T-Tb)^2/(pmax(0.001,T-Tb)^2+k^2))-exp(-(Tm-(T-Tb))/deltaT))","psi=0.005[1E-5,10]|k=10[0.001,1000]|Tb=5[0.01,50]|Tm=35[1,50]|deltaT=6[0.001,100]", "psi~bgroup('[',over(bgroup('(',T-T[b],')')^{~2},bgroup('(',T-T[b],')')^{~2+k^~2})~-~e^{~-~over(T[m]~-~(T~-~T[b]),Delta[T])},']')" },
		{ "Hilbert&LoganIII","ifelse(T>=0,psi*(T^2/(T^2+k^2)-exp(-(Tm-T)/deltaT)),0)","psi=6.6[0.0001,1000]|k=144[0.001,1000]|Tm=46[0,50]|deltaT=3[0.5,100]", "psi~bgroup('[',over(T^~2,T^{~2}+k^{~2})~-~e^{~-~over(T[m]-T,Delta[T])},']')" },
		{ "Huey&Stevenson_1979","psi*pmax(0,T-Tb)*(1-exp(k*pmin(0,T-Tm)))","psi=0.002[1E-10,1]|k=10[0,1E5]|Tb=10[-50,50]|Tm=30[0,100]", "psi~bgroup('(',T-T[b],')')~bgroup('(',1-e^{k~(T-T[m])},')')" },
		{ "Janisch1_1932","psi=1/Dm;psi*(2/(exp(k*(T-To))+exp(-k*(T-To))))","Dm=28[-50,50]|k=0.2[0.0001,10]|To=33[-50,50]", "over(1,psi)~bgroup('(',over(2,e^{~k~(T-T[o])}+e^{~-~k~(T-T[o])}),')')" },
		{ "Janisch2_1932","psi=1/Dm;psi*(2/(k1^(T-Tm)+k2^(Tm-T)))","Dm=5[1E-5,1E5]|k1=0.5[1E-5,10]|k2=1.0[1E-5,10]|Tm=36[-50,50]", "over(1,psi)~bgroup('(',over(2,k[1]^{~~bgroup('(',T~-~T[m],')')}~+~k[2]^{~~bgroup('(',T[m]~-~T,')')}), ')')" },
		{ "Johnson_1974","Tk=T+273.15;Tko=To+273.15;beta1=k2/((k2-k1)*Tko*exp(-k1/Tko));beta2=k2/Tko-log(k2/k1-1);psi*(beta1*Tk*exp(-k1/Tk))/(1+exp(beta2-(k2/Tk)))","psi=0.08[0.0001,1.0]|k1=1e4[0,1e6]|k2=5e4[0,1e6]|To=25[0,50]", "list(psi~bgroup('[',over(beta[1]~T[k]~e^{~-~over(k[1],T[k])},1+e^bgroup('(',beta[2]~-~over(k[2],T[k]),')')),']'),~beta[1]==over(k[2],(k[2]~-~k[1])~T[k[o]]~e^~-~over(k[1],T[k[o]])),~beta[2]==over(k[2],T[k[o]])~-~ln~bgroup('(',over(k[2],k[1])-1,')'))" },//,scriptstyle(atop(~~T[k]==T+273.15,~~~T[k[o]]==T[o]+273.15))
		{ "Kontodimas_2004","psi*(pmax(0,T-Tb)^2)*(Tm-T)","psi=1.6e-5,1e-7,1]|Tb=0[-50,50]|Tm=42[0,100]", "psi~bgroup('(',T-T[b],')')^~2~bgroup('(',T[m]-T,')')" },
		{ "Lactin1_1995","ifelse(T<=Tm,exp(k*T)-exp(k*Tm-(Tm-T)/deltaT),0)","k=0.148[0.1,0.2]|Tm=35[0,100]|deltaT=6.75[2.5,10]", "e^{k~T}~-~e^~bgroup('(',k~T[m]~-~over(T[m]~-~T,Delta[T]), ')')" },
		{ "Lactin2_1995","ifelse(T<=Tm,k1 + exp(k2*T)-exp(k2*Tm-(Tm-T)/deltaT),0)","k1=0[-1,1],k2=0.148[0.1,0.2]|Tm=35[0,100]|deltaT=6.75[2.5,10]", "k[1]~+~e^{k[2]~T}~-~e^~bgroup('(',k[2]~T[m]~-~over(T[m]~-~T,Delta[T]), ')')" },
		{ "Lamb_1992","ifelse(T<=To,psi*exp(-1/2*((T-To)/deltaT1)^2),psi*exp(-1/2*((T-To)/deltaT2)^2))","psi=0.004[0.0001,10]|To=25[0,50]|deltaT1=11[0.1,50]|deltaT2=4[0.1,50]", "list(psi~e^{~-~over(1,2)~bgroup('(',over(T~-~T[o],Delta[T[x]]),')')^2},scriptstyle(Delta[T[x]]~' = '~bgroup('{',atop(Delta[T[1]]~~~T<=T[o],Delta[T[2]]~~~T>T[o]),'')))" },
		{ "Lobry&Rosso&Flandrois_1993","TT=pmax(Tb,pmin(Tm,T));psi*((TT-Tm)*(TT-Tb)^2)/((To-Tb)*((To-Tb)*(TT-To)-(To-Tm)*(To+Tb-2*TT)))","psi=0.1[0,1]|Tb=5[-50,50]|To=25[0,50]|Tm=35[0,50]", "psi~over(bgroup('(',T-T[m],')')~bgroup('(',T-T[b],')')^~2,bgroup('(',T[o]-T[b],')')~bgroup('[',bgroup('(',T[o]-T[b],')')~bgroup('(',T-T[o],')')-bgroup('(',T[o]-T[m],')')~bgroup('(',T[o]+T[b]-2~T,')'),']'))" },
		{ "Logan6_1976","psi*(exp(k*T)-exp(k*Tm-(Tm-T)/deltaT))","psi=0.012[0.0001,1]|k=0.14[0.0001,1]|Tm=33[20,50]|deltaT=5[0.5,10]", "psi~bgroup('(',e^{k~T}~-~e^bgroup('(',k~T[m]~-~over(T[m]~-~T,Delta[T]), ')'), ')')" },
		{ "Logan10_1976","psi*(1/(1+k1*exp(-k2*T))-exp(-((Tm-T)/deltaT)))","psi=0.03[0.0001,1]|k1=110[10,1000]|k2=0.23[0,100]|Tm=33[20,50]|deltaT=5[0.1,10]", "psi~bgroup('(',over(1,1+k[1]~e^{~-~k[2]*T})~-~e^{~-~over(T[m]~-~T,Delta[T])},')')" },
		{ "LoganTb","psi*exp(k*(T-Tb)-exp(k*(T-Tb)/deltaT))","psi=0.02[1E-4,1]|k=0.2[-1E4,1]|Tb=30[-50,100]|deltaT=2[0.5,100]", "psi~e^bgroup('(',k~bgroup('(',T~-~T[b],')')~-~e^{~k~over(T~-~T[b],Delta[T])}, ')')" },
		{ "ONeill_1972","beta=(Tm-T)/(Tm-To);psi*beta^k*exp(k*(1-beta))","psi=0.08[0.0001,1.0]|k=1[0,100]|To=25[0,50]|Tm=35[0,50]", "list(psi~beta^~k~e^{~k~(1-beta)},scriptstyle(beta~' = '~over(T[m]~-~T,T[m]~-~T[o]) ))" },
		{ "Poly1","k0+k1*T","k0=0[-1,0.1]|k1=0.02[1E-5,0.1]", "k[0]+k[1]~T"},
		{ "Poly2","k0+k1*T+k2*T^2","k0=0.1[-1E5,1]|k1=0.015[0,1]|k2=-0.0004[-1,0]", "k[0]+k[1]~T+k[2]~T^{~2}" },
		{ "Poly3","k0+k1*T+k2*T^2+k3*T^3","k0=0.2[-1E5,1E5]|k1=0.01[-1E4,1E4]|k2=0[-1E3,1E3]|k3=0[-1E2,1E2]", "k[0]+k[1]~T+k[2]~T^{~2}+k[3]~T^{~3}" },
		{ "Poly4","k0+k1*T+k2*T^2+k3*T^3+k4*T^4","k0=0.2[-1E5,1E5]|k1=0.02[-1E4,1E4]|k2=0[-1E3,1E3]|k3=0[-1E2,1E2]|k4=0[-10,10]", "k[0]+k[1]~T+k[2]~T^{~2}+k[3]~T^{~3}+k[4]~T^{~4}" },
		{ "Pradham_1946","psi*exp(-1/2*((T-To)/deltaT)^2)","psi=0.004[1E-5,10]|To=25[0,50]|deltaT=18[0.1,100]", "psi~e^{-~over(1,2)~bgroup('(',over(T~-~T[o],Delta[T]),')')^~2}" },
		{ "Ratkowsky_1983","psi^2*pmax(0, (T-Tb)*(1-exp(k*(T-Tm))))^2","psi=0.002[1E-5,10]|k=0.02[1E-5,1]|Tb=5[-50,50]|Tm=35[0,50]", "psi^{~2}~bgroup('[',bgroup('(',T-T[b],')')~bgroup('(',1~-~e^{~k~bgroup('(',T-T[m],')')},')'),']')^~2" },
		{ "Regniere_1982","beta=(T-Tb)/(Tm-Tb);ifelse(T>Tb&T<Tm, psi*( exp(k*beta)-exp(k-(1-beta)/deltaT )),0)","psi=0.2[1E-6,1]|k=2[1E-6,20]|Tb=0[-50,50]|Tm=35[0,50]|deltaT=6[1E-6,20]", "list(psi~bgroup('[',e^{k~beta}~-~e^bgroup('(',k~-~over(1-beta, Delta[T]) ,')'),']'), ~~~scriptstyle(beta~' = '~over(T~-~T[b],T[m]~-~T[b])))" },
		{ "Regniere_1987","beta=(T-Tb)/(Tm-Tb);psi*((1/(1+exp(k1-k2*beta)))-(exp((beta-1)/deltaT)))","psi=0.2[1E-6,1]|k1=2[1E-6,30]|k2=6[1E-6,30]|Tb=0[-50,50]|Tm=35[0,50]|deltaT=0.15[1E-6,1]", "list(psi~bgroup('[',bgroup('(',over(1,1+e^{~bgroup('(',k[1]-k[2]~beta,')')}),')')~-~e^bgroup('(',over(beta~-~1, Delta[T]) ,')'),']'), ~~~scriptstyle(~beta~' = '~over(T~-~T[b],T[m]~-~T[b])))" },
		{ "Regniere_2012","ifelse(T>Tb&T<Tm,psi*(exp(k*(T-Tb))-((Tm-T)/(Tm-Tb))*exp(-k*(T-Tb)/deltab)-((T-Tb)/(Tm-Tb))*exp(k*(Tm-Tb)-(Tm-T)/deltam)),0)","psi=0.01[1E-5,1]|k=0.1[0.001,10]|Tb=0[-50,50]|Tm=35[0,100]|deltab=4[0.01,50]|deltam=5[0.01,50]", "psi~bgroup('[',e^{~k~bgroup('(',T~-~T[b],')')}~-~bgroup('(',scriptstyle(bgroup('(',over(T[m]~-~T,T[m]~-~T[b]),')'))~e^{-~k~bgroup('(',over(T~-~T[b], Delta[T[b]]) ,')')},')')~-~scriptstyle(bgroup('(',over(T~-~T[b],T[m]~-~T[b]),')'))~e^{k~bgroup('(',T[m]~-~T[b],')')~-~bgroup('(',over(T[m]~-~T, Delta[T[m]]) ,')')},']')" },
		{ "Room_1986","ifelse(T<=To, psi*exp(-k1*(T-To)^2),psi*exp(-k2*(T-To)^2))","psi=0.08[0.0001,1.0]|k1=1[0,100]|k2=1[0,100]|To=25[0,50]", "list(psi~e^{-~k[x]~bgroup('(',T~-~T[o],')')^2},~~~scriptstyle(k[x]~' = '~bgroup('{',atop(k[1]~~~~T<=T[o],k[2]~~~~T>T[o]),'')))" },
		//{ "Saint-Amant_2019","psi*exp(k1*(T-Toa)+1/(k2*pmin(-0.001,T-Tm)))","psi=0.5[1E-4,1]|k1=0.5[1E-4,1]|k2=0.5[1E-4,10]|Toa=25[-50,50]|Tm=35[0,100]", "psi~e^~bgroup('[',scriptstyle(k[1]~bgroup('(',T-T[omega],')'))~+~bgroup('(',over(1,k[2]~bgroup('(',T-T[m],')')),')'),']')" },
		{ "Saint-Amant_2021","psi*exp(-k1*(Toa-T)^2+1/(-k2*pmax(0.001,Tm-T)))","psi=0.5[1E-4,10]|k1=0.1[1E-4,10]|k2=0.02[1E-4,10]|Toa=25[-10,50]|Tm=35[0,100]", "psi~e^~bgroup('[',scriptstyle(-k[1]~bgroup('(',T[omega]-T,')')^2)~+~bgroup('(',over(1,-k[2]~bgroup('(',T[m]-T,')')),')'),']')" },
		{ "Schoolfield_1981","Tk=T+273.16;(p25*Tk/298*exp(Ha/1.987*(1/298-1/Tk)))/(1+exp(HL/1.987*(1/TL-1/Tk))+exp(HH/1.987*(1/TH-1/Tk)))","p25=0.01[0,1e6]|Ha=-5e4[-1e6,1e6]|HL=-7.5e4[-1e6,1e6]|TL=273[223,323]|HH=6e5[-1e6,1e6]|TH=308[273,373]", "list(over(rho[25]~bgroup('[',over(T[k],298),']')~e^{bgroup('(',over(H[A],1.987),')')~bgroup('(',over(1,298)~-~over(1,T[k]),')')},1+e^{bgroup('(',over(H[L],1.987),')')~bgroup('(',over(1,T[L])~-~over(1,T[k]),')')}+e^{bgroup('(',over(H[H],1.987),')')~bgroup('(',over(1,T[H])~-~over(1,T[k]),')')}))" },//, scriptstyle(T[k]==T+273.16)
		{ "Sharpe&DeMichele_1977","Tk=T+273.16;Tko=To+273.16;TkL=TL+273.16;TkH=TH+273.16;(p25*(Tk/Tko)*exp((Ha/1.987)*(1/Tko-1/Tk)))/(1+exp((HL/1.987)*(1/TkL-1/Tk))+exp((HH/1.987)*(1/TkH-1/Tk)))","p25=1E-4[0,1e6]|To=25[0,50]|Ha=-5e4[-1e6,1e6]|HL=-7.5e4[-1e6,1e6]|TL=0[-50,50]|HH=6e5[-1e6,1e6]|TH=35[0,50]", "list(over(rho[25]~bgroup('[',over(T[k],T[k[o]]),']')~e^{bgroup('(',over(H[A],1.987),')')~bgroup('(',over(1,T[k[o]])~-~over(1,T[k]),')')},1+e^{bgroup('(',over(H[L],1.987),')')~bgroup('(',over(1,T[k[L]])~-~over(1,T[k]),')')}+e^{bgroup('(',over(H[H],1.987),')')~bgroup('(',over(1,T[k[H]])~-~over(1,T[k]),')')}))" },//, atop(atop(T[k]==T+273.16,T[k[o]]==T[o]+273.16),atop(T[k[b]]==T[b]+273.16,T[k[m]]==T[m]+273.16))
		{ "Shi_2011","psi*(1-exp(-k1*(T-Tb)))*(1-exp(k2*(T-Tm)))","psi=0.2[0.001,1000]|k1=0.2[0.001,1000]|k2=0.2[0.001,1000]|Tb=10[-50,50]|Tm=30[0,50]", "psi~bgroup('(',1-e^{-~k[1]~bgroup('(',T-T[b],')')},')')~bgroup('(',1-e^{k[2]~bgroup('(',T-T[m],')')},')')" },
		{ "Shi_2016","psi*(Tm-T)/(Tm-To)*((T-Tb)/(To-Tb))^((To-Tb)/(Tm-To))","psi=0.02[0.001,10]|Tb=5[-50,50]|To=15[0,50]|Tm=30[0,50]" , "psi~bgroup('(',over(T[m]-T,T[m]-T[o]),')')~bgroup('(',over(T-T[b],T[o]-T[b]),')')^{~bgroup('(',over(T[o]~-~T[b],T[m]~-~T[o]),')')}"},
		{ "Stinner_1974","c(psi/(1+exp(k1 + k2*T[T<To])),psi/(1+exp(k1 + k2*(2*To-T[T>=To]))))","psi=0.2101[0.0001,1]|k1=4.0102[-10,10]|k2=-0.2227[-50,50]|To=26[0,50]", "bgroup('{',atop(psi~over(1,1+e^{k[1] + k[2]*T}),psi~over(1,1+e^{k[1] + k[2]*(2%.%To-T)})),'')~~~~~bgroup('',atop(scriptstyle(T<T[o]),scriptstyle(T>=T[o])),'')" },
		{ "Taylor_1981","psi*exp(-1/2*((T-To)/deltaT)^2)","psi=0.08[0.0001,1.0]|To=32[1,50]|deltaT=9.4[0.5,50]", "psi~e^{-~over(1,2)~bgroup('(',over(T-T[o],Delta[T]),')')^2}" },
		{ "Wagner_1988","Tk=T+273.16;p25*(Tk/298.15)*exp((Ha/1.987)*((1/298.15)-1/Tk))/((1+exp((HL/1.987)*((1/TL)-(1/Tk)))))","p25=0.1[0,1e6]|Ha=28500[-1e6,1e6]|HL=130000[-1e6,1e6]|TL=305[250,350]", "list(over(rho[25]~bgroup('(',over(T[k],298.15),')')~e^{~bgroup('(',over(H[A],1.987),')')~bgroup('(',over(1,298.15)~-~over(1,T[k]),')')},1+e^{~bgroup('(',over(H[L],1.987),')')~bgroup('(',over(1,T[L])~-~over(1,T[k]),')')}))" },//, scriptstyle(T[k]==T+273.16)
		{ "Wang&Lan&Ding_1982","(psi*(1/(1+exp(-k*(T-To))))*(1-exp(-pmax(0,T-Tb)/deltaT))*(1-exp(-pmax(0,Tm-T)/deltaT)))","psi=0.04[0.0001,10]|k=0.23[0.0001,10]|Tb=5[0,50]|To=15[0,50]|Tm=35[0,50]|deltaT=2.3[0.5,10]", "psi~bgroup('(',over(1,1+e^{-~k~bgroup('(',T-T[o],')')}),')')~bgroup('(',1-e^{-~over(T-T[b],Delta[T])},')')~bgroup('(',1-e^{-~over(T[m]-T,Delta[T])},')')" },
		{ "Wang&Engel_1998","beta=log(2)/log((Tm-Tb)/(To-Tb));psi*(2*pmax(0,T-Tb)^beta*(To-Tb)^beta-pmax(0,T-Tb)^(2*beta))/((To-Tb)^(2*beta));","psi=0.1[0,1]|Tb=0[-50,50]|To=25[0,50]|Tm=35[0,100]", "list(psi~bgroup('[',over(2~bgroup('(',T-T[b],')')^beta~(T[o]-T[b])^beta-bgroup('(',T-T[b],')')^{2%.%beta},(T[o]-T[b])^{2%.%beta}),']'),~~~beta~' = '~over(ln(2),ln~bgroup('(',over(T[m]~-~T[b],T[o]~-~T[b]),')')))" },
		{ "Yan&Hunt_1999","psi*(Tm-T)/(Tm-To)*(T/To)^(To/(Tm-To))","psi=0.08[0.0001,1.0]|To=25[0,50]|Tm=35[0,50]", "psi~bgroup('(',over(T[m]-T,T[m]-T[o]),')')~bgroup('(',over(T,T[o]),')')^{~over(T[o],T[m]~-~T[o])}" },
		{ "Yin_1995","exp(psi)*(T-Tb)^k1*(Tm-T)^k2","psi=-10.0[-1E3,0]|k1=1[1,4]|k2=1[0.25,4]|Tb=5[0,30]|Tm=35[0,200]", "e^psi~bgroup('(',T~-~T[b],')')^~k[1]~bgroup('(',T[m]~-~T,')')^~k[2]" },

	};

	TDevRateEquation CDevRateEquation::eq(std::string name)
	{
		auto itr = std::find_if(begin(EQUATION), end(EQUATION), [&](auto &s)
		{
			string find = s[0];
			if (IsEqual(find, name))
				return true;
			return false;
		}
		);

		return itr != end(EQUATION) ? eq(std::distance(begin(EQUATION), itr)) : TDevRateEquation::Unknown;
	}


	bool CDevRateEquation::IsParamValid(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P)
	{
		bool bValid = true;
		switch (model)
		{
		case Allahyari_2005:bValid = P[P3] < P[P4]; break;
		case Analytis_1977: bValid = P[P3] < P[P4]; break;
		case Bieri_1983:bValid = P[P2] < P[P3]; break;
		case Briere1_1999:bValid = P[P1] < P[P2]; break;
		case Briere2_1999:bValid = P[P2] < P[P3]; break;
		case Deutsch_2008:bValid = P[P2] < P[P3]; break;
		case DevaHiggis:bValid = P[P3] < P[P4]; break;
		case Hansen_2011:bValid = P[P2] < P[P3]; break;
		case HilbertLogan_1983:bValid = P[P2] < P[P3]; break;
		case HueyStevenson_1979: bValid = P[P2] < P[P3]; break;
		case Johnson_1974:bValid = (P[P2] / P[P1]) > 1; break;
		case Kontodimas_2004:bValid = P[P1] < P[P2]; break;
		case Lobry_Rosso_Flandrois_1993:bValid = (P[P1] < P[P2]) && (P[P2] < P[P3]); break;
		case ONeill_1972:bValid = P[P2] < P[P3]; break;
		case Ratkowsky_1983:bValid = P[P2] < P[P3]; break;
		case Regniere_1982:  bValid = P[P2] < P[P3]; break;
		case Regniere_1987:  bValid = P[P3] < P[P4]; break;
		case Regniere_2012:bValid = P[P2] < P[P3]; break;
		case SaintAmant_2021:bValid = P[P3] < P[P4]; break;
		case Shi_2011: bValid = P[P3] < P[P4]; break;
		case Shi_2016:bValid = (P[P1] < P[P2]) && (P[P2] < P[P3]); break;
		case Schoolfield_1981:bValid = (P[P3] < P[P5]); break;
		case WangLanDing_1982:bValid = (P[P2] < P[P3]) && (P[P3] < P[P4]); break;
		case WangEngel_1998:bValid = (P[P1] < P[P2]) && (P[P2] < P[P3]); break;
		case YanHunt_1999:bValid = P[P1] < P[P2]; break;
		case Yin_1995:bValid = P[P3] < P[P4]; break;

		}

		return bValid;
	}

	static string tolower(string s) {
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	struct comp {
		bool operator() (const std::string& lhs, const std::string& rhs) const {
			return  tolower(lhs) < tolower(rhs);
		}
	};



	ERMsg CDevRateEquation::GetParamfromString(std::string eq_name, std::string str_param, std::vector<double>& P)
	{
		ERMsg msg;

		TDevRateEquation eq = CDevRateEquation::eq(eq_name);
		if (eq != Unknown)
		{
			StringVector tmp(str_param, " ");
			std::map<string, double, comp> map_P;
			for (size_t i = 0; i < tmp.size(); i++)
			{
				StringVector tmp2(tmp[i], "=");
				if (tmp2.size() == 2)
					map_P[tmp2[0]] = ToDouble(tmp2[1]);
				else
					msg.ajoute("Invalid parameter in: " + tmp[i]);
			}

			CSAParameterVector param_info = GetParameters(eq);
			P.resize(param_info.size());

			for (size_t i = 0; i < param_info.size(); i++)
			{
				if (map_P.find(param_info[i].m_name) != map_P.end())
					P[i] = map_P[param_info[i].m_name];
				else
					msg.ajoute("Missing parameter: " + param_info[i].m_name);
			}
		}
		else
		{
			msg.ajoute("Invalid equation name: " + eq_name);
		}

		return msg;
	}


	double CDevRateEquation::GetRate(CDevRateEquation::TDevRateEquation model, const std::vector<double>& P, double T)
	{

		double rT = 0;

		if (model == Allahyari_2005)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];


			T = max(Tb, min(Tm, T));
			rT = psi * pow((T - Tb) / (Tm - Tb), k1)*(1.0 - pow((T - Tb) / (Tm - Tb), k2));
		}
		else if (model == Analytis_1977)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];

			T = max(Tb, min(Tm, T));
			rT = psi * pow(T - Tb, k1) * pow(Tm - T, k2);
		}
		if (model == Angilletta_2006)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double To = P[P3];

			rT = psi * exp(-0.5*pow(abs(T - To) / k1, k2));
		}
		else if (model == Bieri_1983)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];

			rT = psi * (T - Tb) - (k * exp(T - Tm));
		}
		else if (model == Briere1_1999)
		{
			double psi = P[P0];
			double Tb = P[P1];
			double Tm = P[P2];

			T = max(Tb, min(Tm, T));
			rT = psi * T*(T - Tb)*pow(Tm - T, 0.5);
		}
		else if (model == Briere2_1999)//1999
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];

			T = min(Tm, T);
			rT = psi * T*(T - Tb)*pow(Tm - T, 1.0 / k);
		}
		else if (model == Damos_2008)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];

			rT = psi * (k1 - T / 10) * pow(T / 10, k2);
		}
		else if (model == Damos_2011)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];

			rT = psi * 1 / (1.0 + k1 * T + k2 * T*T);
		}
		else if (model == Deutsch_2008)
		{
			double psi = P[P0];
			double k = P[P1];
			double To = P[P2];
			double Tm = P[P3];

			if (T <= To)
				rT = psi * exp(-k * pow(T - To, 2.0));
			else
				rT = psi * (1.0 - pow((T - To) / (To - Tm), 2.0));
		}
		else if (model == WangEngel_1998)
		{
			double psi = P[P0];
			double Tb = P[P1];
			double To = P[P2];
			double Tm = P[P3];

			T = max(Tb, min(Tm, T));
			double alpha = log(2) / log((Tm - Tb) / (To - Tb));
			rT = psi * (2.0 * pow(T - Tb, alpha) * pow(To - Tb, alpha) - pow(T - Tb, 2.0 * alpha)) / pow(To - Tb, 2.0 * alpha);
		}
		else if (model == DevaHiggis)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];

			T = max(Tb, min(Tm, T));
			double beta1 = (T - Tm) / (Tm - Tb) - (1 / (1 + 0.28*k1 + 0.72*log(1 + k1)));
			double beta2 = (1 + k1) / (1 + 1.5*k1 + 0.39*k1*k1);
			double Omega = pow((beta1 + exp(k1*beta1)) / beta2, 2.0);

			rT = psi * pow(10.0, -Omega)*(1 - k2 + k2 * Omega);
		}
		else if (model == Hansen_2011)
		{
			double psi = P[P0];//Lower developmental threshold 
			double k = P[P1];//Peak rate control parameter 
			double Tb = P[P2];//Width of upper thermal boundary layer 
			double Tm = P[P3];//Upper developmental threshold
			double Δm = P[P4];//Low temperature acceleration of rates 

			rT = psi * ((exp(k*(T - Tb)) - 1) - (exp(k*(Tm - Tb)) - 1) * exp(-(Tm - T) / Δm));
		}
		else if (model == HilbertLogan_1983)
		{
			double phi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];
			double ΔT = P[P4];

			T = max(Tb, T);
			rT = phi * ((pow(T - Tb, 2.0) / (pow(T - Tb, 2.0) + k * k)) - exp(-(Tm - (T - Tb)) / ΔT));
		}
		else if (model == HilbertLoganIII)
		{
			double phi = P[P0];
			double k = P[P1];
			double Tm = P[P2];
			double ΔT = P[P3];

			T = max(0.0, T);
			rT = phi * ((T*T) / (T*T + k * k) - exp(-(Tm - T) / ΔT));
		}
		else if (model == HueyStevenson_1979)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];

			T = max(Tb, min(Tm, T));
			rT = psi * (T - Tb) * (1.0 - exp(k * (T - Tm)));
		}
		else if (model == Janisch1_1932)
		{
			double Dm = P[P0];
			double k = P[P1];
			double To = P[P2];

			double psi = (1 / Dm);
			rT = psi * (2 / (exp(k*(T - To)) + exp(-k * (T - To))));
		}
		else if (model == Janisch2_1932)
		{
			double Dm = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tm = P[P3];

			double psi = (1 / Dm);
			rT = psi * (2 / (pow(k1, T - Tm) + pow(k2, Tm - T)));
		}
		else if (model == Johnson_1974)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double To = P[P3];


			double Tk = T + 273.15;
			double Tko = To + 273.15;
			double beta1 = k2 / ((k2 - k1)*Tko*exp(-k1 / Tko));
			double beta2 = k2 / Tko - log(k2 / k1 - 1);

			rT = psi * (beta1*Tk*exp(-k1 / Tk)) / (1 + exp(beta2 - (k2 / Tk)));
		}
		else if (model == Kontodimas_2004)
		{
			double psi = P[P0];
			double Tb = P[P1];
			double Tm = P[P2];

			T = max(Tb, min(Tm, T));
			rT = psi * (pow(T - Tb, 2.0))*(Tm - T);
		}
		else if (model == Lactin1_1995)
		{
			double k = P[P0];
			double Tm = P[P1];
			double ΔT = P[P2];

			if (T <= Tm)
				rT = exp(k * T) - exp(k * Tm - (Tm - T) / ΔT);
		}
		else if (model == Lactin2_1995)
		{
			double k1 = P[P0];
			double k2 = P[P1];
			double Tm = P[P2];
			double ΔT = P[P3];

			if (T <= Tm)
				rT = k1 + exp(k2 * T) - exp(k2 * Tm - (Tm - T) / ΔT);
		}
		else if (model == Lamb_1992)
		{
			double psi = P[P0];
			double Tm = P[P1];
			double ΔT1 = P[P2];
			double ΔT2 = P[P3];

			double ΔT = T < Tm ? ΔT1 : ΔT2;
			rT = psi * exp(-1.0 / 2.0 * pow((T - Tm) / ΔT, 2.0));
		}
		else if (model == Lobry_Rosso_Flandrois_1993)
		{
			double psi = P[P0];
			double Tb = P[P1];
			double To = P[P2];
			double Tm = P[P3];

			T = max(Tb, min(Tm, T));
			double  beta1 = (T - Tm)*pow(T - Tb, 2);
			double beta2 = (To - Tb)*((To - Tb)*(T - To) - (To - Tm)*(To + Tb - 2 * T));
			rT = psi * beta1 / beta2;
		}
		else if (model == Logan6_1976)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tm = P[P2];
			double ΔT = P[P3];

			rT = psi * (exp(k * T) - exp(k * Tm - (Tm - T) / ΔT));
		}
		else if (model == Logan10_1976)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tm = P[P3];
			double ΔT = P[P4];

			rT = psi * (1.0 / (1.0 + k1 * exp(-k2 * T)) - exp(-((Tm - T) / ΔT)));
		}
		else if (model == LoganTb)//  Tb Model (Logan)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double ΔT = P[P3];

			rT = psi * exp(k*(T - Tb) - exp(k*(T - Tb) / ΔT));
		}
		else if (model == ONeill_1972)
		{
			double psi = P[P0];
			double k = P[P1];
			double To = P[P2];
			double Tm = P[P3];

			double beta = (Tm - T) / (Tm - To);
			rT = psi * pow(beta, k)*exp(k*(1 - beta));
		}
		else if (model == Poly1)
		{
			double k0 = P[P0];
			double k1 = P[P1];

			rT = k0 + k1 * T;
		}
		else if (model == Poly2)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];

			rT = k0 + k1 * T + k2 * T *T;
		}
		else if (model == Poly3)//Tanigoshi
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double k3 = P[P3];

			rT = k0 + k1 * T + k2 * T*T + k3 * T*T*T;
		}
		else if (model == Poly4)
		{
			double k0 = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double k3 = P[P3];
			double k4 = P[P4];

			rT = k0 + k1 * T + k2 * T*T + k3 * T*T*T + k4 * T*T*T*T;
		}
		else if (model == Pradham_1946)
		{
			double psi = P[P0];
			double To = P[P1];
			double ΔT = P[P2];

			rT = psi * exp(-0.5*pow((T - To) / ΔT, 2.0));
		}
		else if (model == Ratkowsky_1983)
		{
			double b = P[P0];
			double k1 = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];

			double psi = b * b;
			rT = psi * pow(max(0.0, (T - Tb)*(1.0 - exp(k1*(T - Tm)))), 2.0);
		}
		else if (model == Stinner_1974)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double To = P[P3];

			T = T < To ? T : (2 * To - T);
			rT = psi / (1.0 + exp(k1 + k2 * T));
		}
		else if (model == Taylor_1981)
		{
			double psi = P[P0];
			double To = P[P1];
			double ΔT = P[P2];

			rT = psi * exp(-(0.5)*pow(-(T - To) / ΔT, 2.0));
		}
		else if (model == Regniere_1982)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];
			double ΔT = P[P4];

			if (T >= Tb && T <= Tm)
			{
				double beta = (T - Tb) / (Tm - Tb);
				rT = psi * (exp(k*beta) - exp(k - ((1.0 - beta) / ΔT)));
			}
		}
		else if (model == Regniere_1987)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];
			double ΔT = P[P5];

			T = max(Tb, min(Tm, T));
			double beta = (T - Tb) / (Tm - Tb);
			double omega1 = 1.0 / (1.0 + exp(k1 - k2 * beta));
			double omega2 = exp((beta - 1) / ΔT);

			rT = psi * (omega1 - omega2);
		}
		else if (model == Regniere_2012)
		{
			double psi = P[P0];//ψ
			double k = P[P1];
			double Tb = P[P2];
			double Tm = P[P3];
			double ΔTb = P[P4];
			double ΔTm = P[P5];

			if (T >= Tb && T <= Tm)
			{
				double beta1 = exp(k * (T - Tb));
				double beta2 = ((Tm - T) / (Tm - Tb)) * exp(-k * (T - Tb) / ΔTb);
				double beta3 = ((T - Tb) / (Tm - Tb)) * exp(k * (Tm - Tb) - (Tm - T) / ΔTm);
				rT = psi * (beta1 - beta2 - beta3);
			}
		}
		else if (model == Room_1986)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double To = P[P3];

			double kx = (T <= To) ? k1 : k2;
			rT = psi * exp(-kx * pow(T - To, 2.0));
		}
		//else if (model == SaintAmant_2019)
		//{
		//	double psi = P[P0];
		//	double k1 = P[P1];
		//	double k2 = P[P2];
		//	double Toa = P[P3];
		//	double Tm = P[P4];

		//	T = min(Tm - 0.001, T);
		//	rT = psi * exp(k1*(T - Toa) + 1.0 / (k2*(T - Tm)));
		//}
		else if (model == SaintAmant_2021)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Toa = P[P3];
			double Tm = P[P4];
			

			//T = min( -0.001, T-Tm);
			//rT = psi * (exp(-k1*max(0.0, Tm-T) + 1.0 / (-k2*max(0.001,Tm-T))) - b);
			rT = psi * exp(-k1 * (Toa - T)*(Toa - T) + 1.0 / (-k2 * max(0.001, Tm - T)));
		}
		else if (model == Schoolfield_1981)
		{
			double p25 = P[P0];
			double Ha = P[P1];
			double Hb = P[P2];
			double Tb = P[P3];
			double Hm = P[P4];
			double Tm = P[P5];

			static const double R = 1.987;
			double Tk = T + 273.16;
			rT = (p25*Tk / 298.0 * exp(Ha / R * (1.0 / 298 - 1.0 / Tk))) / (1.0 + exp(Hb / R * (1.0 / Tb - 1.0 / Tk)) + exp(Hm / R * (1.0 / Tm - 1.0 / Tk)));
		}
		else if (model == SharpeDeMichele_1977)
		{
			double p25 = P[P0];
			double To = P[P1];
			double Ha = P[P2];
			double Hb = P[P3];
			double Tb = P[P4];
			double Hm = P[P5];
			double Tm = P[P6];

			double Tk = T + 273.15;
			double Tkb = Tb + 273.15;
			double Tko = To + 273.15;
			double Tkm = Tm + 273.15;

			rT = (p25 * (Tk / Tko) * exp((Ha / 1.987) * ((1.0 / Tko) - (1.0 / Tk)))) /
				(1.0 + exp((Hb / 1.987) * ((1.0 / Tkb) - (1.0 / Tk))) + exp((Hm / 1.987) * ((1.0 / Tkm) - (1.0 / Tk))));
		}
		else if (model == Shi_2011)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];

			rT = psi * (1.0 - exp(-k1 * (T - Tb))) * (1.0 - exp(k2 * (T - Tm)));
		}
		else if (model == Shi_2016)
		{
			double psi = P[P0];
			double Tb = P[P1];
			double To = P[P2];
			double Tm = P[P3];

			T = max(Tb, min(Tm, T));
			rT = psi * (Tm - T) / (Tm - To) * pow(((T - Tb) / (To - Tb)), ((To - Tb) / (Tm - To)));
		}
		else if (model == Wagner_1988)
		{
			double p25 = P[P0];
			double Ha = P[P1];
			double Hb = P[P2];
			double Tb = P[P3];

			double Tk = T + 273.16;
			rT = 1.0 / ((1.0 + exp((Hb / 1.987) * ((1.0 / Tb) - (1.0 / Tk)))) / (p25 * Tk / 298.15 * exp((Ha / 1.987) * ((1.0 / 298.15) - 1.0 / Tk))));
		}
		else if (model == WangLanDing_1982)
		{
			double psi = P[P0];
			double k = P[P1];
			double Tb = P[P2];
			double To = P[P3];
			double Tm = P[P4];
			double ΔT = P[P5];

			T = max(Tb, min(Tm, T));
			rT = psi * (1.0 / (1.0 + exp(-k * (T - To)))) * (1.0 - exp(-(T - Tb) / ΔT)) * (1.0 - exp(-(Tm - T) / ΔT));
		}
		else if (model == YanHunt_1999)
		{
			double psi = P[P0];
			double To = P[P1];
			double Tm = P[P2];

			rT = psi * (Tm - T) / (Tm - To)*pow(T / To, To / (Tm - To));
		}
		else if (model == Yin_1995)
		{
			double psi = P[P0];
			double k1 = P[P1];
			double k2 = P[P2];
			double Tb = P[P3];
			double Tm = P[P4];

			T = max(Tb, min(Tm, T));
			rT = exp(psi) * pow(T - Tb, k1) * pow(Tm - T, k2);
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
		ASSERT(X.size() >= p.size());
		for (size_t i = 0; i < p.size(); i++)
		{
			p[i].m_initialValue = X[i];
		}

		return p;
	}

}






