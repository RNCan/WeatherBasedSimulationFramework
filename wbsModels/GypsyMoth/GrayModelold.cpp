// Gray's egg development model.     
// RECODED WITH SOME CHANGES BY JACQUES   
// Prediapause rates and variability from Gray et al. 1991 (Envir. Ent. 20(6)) 
// Diapause rates and variability from Experiment 9                   
// Postdiapause rates and variability from Experiment 10              
//  VERSION DATE:22-01-99    
//***************************  Modifications  *************************
// JR 25/02/99: corrected as per David's recommendation the date referencing
//              in routine sinewave().
// JR 25/02/99: in module gray(), hatch is output (to array) as daily hatch rate
//              rather than cumulative hatch rate.
// JR 24/03/99: Introduced GrayReset() to re-initialize (0.0) globals
// JR 25/03/99: Introduced free_Gray_arrays() to free allocated memory before exit.
//				Also freed allocated arrays in module calc_rates()
//*********************************************************************

#pragma warning( disable : 4201)
#pragma warning( disable : 4514)

#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define max __max
#define MAXEGGS 250
#define N_CLASS1  10        // These values can be changed at will (max:20) 
#define N_CLASS2  5        // but 5 5 10 seems quite enough 
#define N_CLASS3 10
#define MAXCLASSES 20      // to dimension arrays 
#define AGERANGEPOSTDIAPAUSE 21     //0.05 increments from 0 to 1.0, 

void allocate_arrays(void);
void free_Gray_arrays(void);
int round_off(double);
void initialize_ages(void);
void print_memory_error(void);
void sinwave(int);
void calc_rates(void);      //constructs lookup tables of PDRs, inhib. depletion rate and A (activity or effective resistance)
void calc_variability(void);
void strip(char *);            // function to clean-up an input string 
double prediapause_rate(double);
double diapause_rate(double,double);
double depletion_rate(double,double);
double postdiapause_rate(double,double);
void GrayReset();

FILE *fptrjunk;
char junk_file_name[_MAX_PATH]="c:\\gm.out";    
//extern char input_file_name[_MAX_PATH];    // now defined as global
//char output_file_name[_MAX_PATH];   // now defined as global 

//#define T_min T_min
//#define T_max T_max

extern float T_max[730];                //  Array of maximum temperatures
extern float T_min[730];                //  Array of minimum temperatures
double *prediapause_table;   //Lookup table of prediapause rate for each temperature
double **d_inhibitor_table;  //Lookup table of inhibitor depletion rate for each temperature
double **diapause_table;     //Lookup table of diapause rate for each temperature & inhibitor titre combination
double **postdiapause_table; //Lookup table of postdiapause rate for each temperature & inhibitor titre combination
double variability_table[4][MAXCLASSES];    //10 rate classes for each phase plus hatch
double prediapause_age[N_CLASS1];         // age of each class in prediapause
double inhibitor_titre[N_CLASS1][N_CLASS2]; //inhibitor level of each class in diapause
double diapause_age[N_CLASS1][N_CLASS2];         //  age of each class in diapause
double postdiapause_age[N_CLASS1][N_CLASS2][N_CLASS3];  //age of each class in postdiapause
double diapause_eggs[N_CLASS1];
double postdiapause_eggs[N_CLASS1][N_CLASS2];
int number_classes[3]={N_CLASS1,N_CLASS2,N_CLASS3};    //number of variability classes in 3 phases
double *hour_temp;                 //Array of hourly temperatures
double **total_eggs;              //Output array for each julian day: 3 phases plus hatch

extern int GRAY_MODEL;
extern double lower_thresh[3];      //  lower temperature thresholds of phases
extern double upper_thresh[3];      //  upper temperature thresholds of phases

extern int ovip_date;
extern int time_step;
extern double psiI, rhoI,TmI, deltaTI;         // prediapause developmental rate parameters 
extern double PDR_c,PDR_t,PDR_t2,PDR_t4; // diapause PDR parameters
extern double start_inhibitor,RS_c,RS_rp,RP_c;//diapause inhibitor depletion parameters
extern double eff_res_1,eff_res_2; //effective resistance parameters for the inhibitor
extern double tauIII, deltaIII;        // postdiapause developmental rate parameters
extern double omegaIII, kappaIII, psiIII;  //  parameters of phaseIII rate change
extern double alphaI, betaI, gammaI;       // variability parameters for phase I
extern double alphaII, betaII, gammaII;    // variability parameters for phase II
extern double alphaIII, betaIII, gammaIII;   // variability parameters for phase III
extern float **hatching;
extern int first_hatch[];

int age_range_postdiapause=AGERANGEPOSTDIAPAUSE;       //re-initialized in GrayReset

int gray(void)
{

//	FILE *fptrout;                       //output file  
	double class_size[3];                // class_size is calculated later 
	int prediapause_class;
	int diapause_class;
	int postdiapause_class;
	int hour,day;                      //day is date (can exceed 365) 
	int iday=0;                        //iday id day%365
	int last_day;
	int phase;
	double T;
	double rate;
	double depletion;

	double prediap_rate;    //used to store development rates 
	double tot_eggs[4]={0,0,0,0};      //used for accounting 
//  compute class sizes 
	class_size[0]=MAXEGGS/(double)number_classes[0];
	class_size[1]=MAXEGGS/(double)(number_classes[0]*number_classes[1]);
	class_size[2]=MAXEGGS/(double)(number_classes[0]*number_classes[1]*number_classes[2]);


#ifdef _DEBUG
	if((fptrjunk=fopen(junk_file_name,"w"))==NULL)
	{
		printf("Could not open file %s for read\n",junk_file_name);
		exit(0);
	}
#endif

	allocate_arrays();	

	GrayReset();

	calc_rates();			//construct rate tables for each phase
								//construct depletion table for diapause
	calc_variability();		//construct variability factor table for each class of each phase
	tot_eggs[0]=MAXEGGS;	//put all eggs into prediapause phase
	initialize_ages();		//make all ages=0 and inhibitor titres=1

	//daily loop from ovip_date-on 
	for (day=ovip_date; day<ovip_date+365; ++day)
	{
#ifdef _DEBUG
//fprintf(fptrjunk,"%4d %4d %8.6f %8.6f %8.6f\n",day,day-ovip_date,prediapause_age[5],inhibitor_titre[5][2],diapause_age[5][2]);
#endif
		iday=(day-1)%365; // wrap days around 365 modulo 
		//process only non-missing temperatures on two successive days 
		if(T_min[day-1]>-99 && T_min[day] > -99)
		{
			sinwave(day-1);
			for(hour=0; hour<24; hour+=time_step)
			{
				T=hour_temp[hour];
				//prediapause rates are age-independent 
				prediap_rate=prediapause_rate(T);
				//prediapause development
				for(prediapause_class=0; prediapause_class<number_classes[0]; prediapause_class++)
				{
					if(prediapause_age[prediapause_class]<1)  //temperature ranges checked elsewhere
					{
						rate=prediap_rate*variability_table[0][prediapause_class];
						prediapause_age[prediapause_class]+=rate;
						if(prediapause_age[prediapause_class]>=1)
						{
							tot_eggs[0]-=class_size[0];
							tot_eggs[1]+=class_size[0];
							diapause_eggs[prediapause_class]+=class_size[0];
						}
					}
					else //eggs of this classs are PAST the prediapause phase 
					{
						if(diapause_eggs[prediapause_class]>0)
						{
							for(diapause_class=0; diapause_class<number_classes[1]; diapause_class++)
							{
								if(diapause_age[prediapause_class][diapause_class]<1) //diapause development
								{
									rate=diapause_rate(T,inhibitor_titre[prediapause_class][diapause_class])*variability_table[1][diapause_class];
									diapause_age[prediapause_class][diapause_class]+=rate;
 									if(inhibitor_titre[prediapause_class][diapause_class]>0)depletion=depletion_rate(T,inhibitor_titre[prediapause_class][diapause_class]);
									else depletion=0.0;
									inhibitor_titre[prediapause_class][diapause_class]+=max(-inhibitor_titre[prediapause_class][diapause_class],depletion);
									if(diapause_age[prediapause_class][diapause_class]>=1)
									{
										tot_eggs[1]-=class_size[1];
										tot_eggs[2]+=class_size[1];
										postdiapause_eggs[prediapause_class][diapause_class]+=class_size[1];
									}
								}
								else //these eggs are in postdiapause
								{
									if(postdiapause_eggs[prediapause_class][diapause_class]>0)  /*postdiapause development */
									{
										for(postdiapause_class=0; postdiapause_class<number_classes[2]; postdiapause_class++)
										{
											if(postdiapause_age[prediapause_class][diapause_class][postdiapause_class]<1)
											{
												rate=postdiapause_rate(T,postdiapause_age[prediapause_class][diapause_class][postdiapause_class])*variability_table[2][postdiapause_class];
												postdiapause_age[prediapause_class][diapause_class][postdiapause_class]+=rate;
												if(postdiapause_age[prediapause_class][diapause_class][postdiapause_class]>=1)
												{
													tot_eggs[2]-=class_size[2];
													tot_eggs[3]+=class_size[2];
													postdiapause_eggs[prediapause_class][diapause_class]-=class_size[2];
												}
											}
										} //postdiapause classes 
									}
								}
							} //diapause classes
						}
					}
				} //prediapause classes
			} //time_step
		} //valid data 
		for(phase=0; phase<4; phase++)  //update densities for new day
		{
			total_eggs[iday][phase]=tot_eggs[phase];
		}

        if(first_hatch[GRAY_MODEL] == 730 && tot_eggs[3]/MAXEGGS >= 0.001)
			first_hatch[GRAY_MODEL] =  day;
	} //day loop

//output section
	last_day=day;

#ifdef _DEBUG

//	year=365; 
	for(day=ovip_date; day<last_day; day++)
	{
		iday = day%365;
		fprintf(fptrjunk,"%d  %f  %f  %f  %f\n",day,total_eggs[iday][0]*100/MAXEGGS,total_eggs[iday][1]*100/MAXEGGS,total_eggs[iday][2]*100/MAXEGGS,total_eggs[iday][3]*100/MAXEGGS);
	}
//	fclose(fptrout);
	fclose(fptrjunk);

#endif

	float eggpr=0;
	for(day=ovip_date; day<last_day; day++)
	{
		iday = (day-1)%365;
		hatching[day][GRAY_MODEL] = (float)(total_eggs[iday][3]*100/MAXEGGS)-eggpr;
		eggpr=(float)(total_eggs[iday][3]*100/MAXEGGS);
	}

	free_Gray_arrays();

	return(0);
}

//Calculates 24 temperatures from the daily maximum
//and minimum temperatures.
void sinwave(int iday)
{
	int hour;
	int peak=12;
	int tday;         //a variable to let iday exceed 365
	double time_factor=6;  //"rotates" the radian clock to put the peak at the top
	double r_hour;     //hours converted to radians:  24=2*pi 
	double mean1;      //mean temperature for day1
	double range1;     //half the difference of min and max temps of day1
	double mean2;      //mean of max temp (day1) and min temp (day2)
	double range2;     //half the difference of max (day1) and min (day2)
	double mean;       //one or the other of the means
	double range;      //one or the other of the ranges
	double theta;

	tday=iday;

	mean1=(T_max[tday]+T_min[tday])/2;
	range1=(T_max[tday]-T_min[tday])/2;
	mean2=(T_max[tday]+T_min[tday+1])/2;
	range2=(T_max[tday]-T_min[tday+1])/2;
	r_hour=3.14159/12;

	for(hour=0; hour<24; hour++)
	{
		if(hour<peak)mean=mean1;
		else mean=mean2;
		if(hour<peak)range=range1;
		else range=range2;
		theta=((double)hour-time_factor)*r_hour;
		hour_temp[hour]=mean+range*sin(theta);
	}
}

//Calculates the developmental rates and constructs the rate table for each phase.
//Each table will have an extra entry in each dimension because it's needed when
//the table is called by diapause_rate() for example.
void calc_rates(void)
{
	int temp;
	int i;
	int j;
	int temp_range_diapause;
	int inhib_titre_range;
	double rate_zero;
	double a_t; 
	double age;
	double inhibitor_titre;
	double hundreds=100.0;
	double T;      //temperature transforms for the PDR function
	double Z;      //temperature transforms for the RP & effective resistance functions
	double x;      //a temporary variable
	double *RS;
	double *RP;
	double *PDR;    //potential developmental rate in diapause
	double *eff_res;
	double RSK;
	double ln_RP;
	double rate;

	//calculate prediapause rates for a lookup table  
	for(temp=round_off(lower_thresh[0]), i=0; temp<=round_off(upper_thresh[0]); temp++)
	{
		T=(double)temp-lower_thresh[0];
		rate=psiI*(exp(rhoI*T)-exp(rhoI*TmI-(TmI-T)/deltaTI));
		if(rate>0)prediapause_table[i]=rate*(double)time_step/24.;
		else prediapause_table[i]=0;
		i++;
	}
	prediapause_table[i]=prediapause_table[i-1];

	//calculate PDR (rate), RP, RS, and effective resistance in diapause  
	temp_range_diapause=(round_off)(upper_thresh[1]-lower_thresh[1]+1);
	if((eff_res=(double *)malloc(temp_range_diapause*sizeof(double)))==NULL)print_memory_error();
	if((RP=(double *)malloc(temp_range_diapause*sizeof(double)))==NULL)print_memory_error();
	if((RS=(double *)malloc(temp_range_diapause*sizeof(double)))==NULL)print_memory_error();
	if((PDR=(double *)malloc(temp_range_diapause*sizeof(double)))==NULL)print_memory_error();

	for(temp=round_off(lower_thresh[1]), i=0; temp<=round_off(upper_thresh[1]); temp++)
	{
		//Z is a temperature transform for effective resistance and RP.
		Z=(upper_thresh[1]-(double)temp)/(upper_thresh[1]-lower_thresh[1]);
		x=eff_res_1*pow(Z,eff_res_2);
		eff_res[i]=0.3+0.7*pow((1-Z),x);
		RP[i]=1.0+RP_c*pow(exp(Z),6);
		rate=max(0,exp(PDR_c+PDR_t*(double)temp+PDR_t2*pow((double)temp,2)+PDR_t4*pow((double)temp,4)));
		RS[i]=RS_c+RS_rp*RP[i];
		if(rate>0)PDR[i]=rate;
		else PDR[i]=0;
		i++;
	}

	//calculate inhib. depletion rates and actual develop. rates (in diapause) for lookup tables
	inhib_titre_range=(round_off)(start_inhibitor*hundreds);
	for(temp=round_off(lower_thresh[1]), i=0; temp<=round_off(upper_thresh[1]); temp++)
	{
		RSK=start_inhibitor+RS[i];
		ln_RP=log(RP[i]);
		for(j=inhib_titre_range; j>=0; j--)
		{
			inhibitor_titre=(double)j/hundreds;
			d_inhibitor_table[i][j]=max(-inhibitor_titre*(double)time_step/24.0,(inhibitor_titre-RSK)*ln_RP*(double)time_step/24.0);
			if(d_inhibitor_table[i][j]>0)
			{
				printf("Inhibitor titre will INCREASE. You must change parameter values.\n");
				exit(0);
			}
			rate=max(0,(1-inhibitor_titre*eff_res[i]))*PDR[i];
			if(rate<=0)diapause_table[i][j]=0;
				else diapause_table[i][j]=rate*(double)time_step/24.0;
		}
		d_inhibitor_table[i][inhib_titre_range+1]=d_inhibitor_table[i][inhib_titre_range];
		diapause_table[i][inhib_titre_range+1]=diapause_table[i][inhib_titre_range];
		i++;
	}
	for(j=inhib_titre_range+1; j>=0; j--)
	{
		d_inhibitor_table[i][j]=d_inhibitor_table[i-1][j];
		diapause_table[i][j]=diapause_table[i-1][j];
	}


		//calculate postdiapause rates for a lookup table
	for(temp=round_off(lower_thresh[2]), i=0; temp<=round_off(upper_thresh[2]); temp++)
	{
		for(j=0; j<age_range_postdiapause; j++)
		{
			age=(float)j/(age_range_postdiapause-1);
			rate_zero=tauIII+deltaIII*(double)temp;
			a_t=omegaIII+kappaIII*(double)temp+psiIII*pow((double)temp,2);
			rate=a_t*age+rate_zero;
			if(rate<=0)postdiapause_table[i][j]=0;
				else if(rate>=1)postdiapause_table[i][j]=1*(double)time_step/24.;
				else postdiapause_table[i][j]=rate*(double)time_step/24.;
		}
		postdiapause_table[i][j]=postdiapause_table[i][j-1];
		i++;
	}
	for(j=0; j<age_range_postdiapause+1; j++)
	{
		postdiapause_table[i][j]=postdiapause_table[i-1][j];
	}

	//free allocated memory
	free(eff_res);
	free(RP);
	free(RS);
	free(PDR);
}


//Calculates the variability factor for each class in each phase 
void calc_variability(void)
{
	int nClass;
	int phase;
	double x;
	//calculate prediapause variability
	phase=0;
	for(nClass=0; nClass<number_classes[phase]; nClass++)
	{
		x=(double)nClass/number_classes[phase]+0.5/number_classes[phase];
		variability_table[phase][nClass]=pow(gammaI+betaI*(-log(1-x)),(1/alphaI));
	}

	//calculate diapause variability
	phase=1;
	for(nClass=0; nClass<number_classes[phase]; nClass++)
	{
		x=(double)nClass/number_classes[phase]+0.5/number_classes[phase];
		variability_table[phase][nClass]=pow(gammaII+betaII*(-log(1-x)),(1/alphaII));
	}

	//calculate postdiapause variability
	phase=2;
	for(nClass=0; nClass<number_classes[phase]; nClass++)
	{
		x=(double)nClass/number_classes[phase]+0.5/number_classes[phase];
		variability_table[phase][nClass]=pow(gammaIII+betaIII*(-log(1-x)),(1/alphaIII));
	}
}


//Determines the age increment in prediapause from the lookup table
double prediapause_rate(double T)
{
	int TP;
	double fraction;
	double dev_rate=0;

	if(T<lower_thresh[0] || T>upper_thresh[0])
	{
		if(T<lower_thresh[0])TP=0;
		else TP=round_off(upper_thresh[0])-round_off(lower_thresh[0]);
	}
	else TP=(round_off)(T-lower_thresh[0]);

	fraction=T-(lower_thresh[0]+(double)TP);
	dev_rate=prediapause_table[TP]+fraction*(prediapause_table[TP+1]-prediapause_table[TP]);
	return(dev_rate);
}

//Determines the inhibitor depletion from the lookup table
double depletion_rate(double T,double inhib_titre)
{
	int TP;
	int I;
	double T_fraction;
	double I_fraction;
	double deplete_rate=0;

	if(T<lower_thresh[1] || T>upper_thresh[1])
	{
		if(T<lower_thresh[1])TP=0;
		else TP=round_off(upper_thresh[1])-round_off(lower_thresh[1]);
		T_fraction=0.0;
	}
	else
	{
		TP=(round_off)(T-lower_thresh[1]);
		T_fraction=T-(lower_thresh[1]+TP);
	}

	I=(round_off)(inhib_titre*100);
	I_fraction=inhib_titre*100.0-I;
	deplete_rate=d_inhibitor_table[TP][I]+T_fraction*(d_inhibitor_table[TP+1][I]-d_inhibitor_table[TP][I])+I_fraction*(d_inhibitor_table[TP][I+1]-d_inhibitor_table[TP][I]);
	return(deplete_rate);
}

//Determines the age increment in diapause from the lookup table
double diapause_rate(double T,double inhib_titre)
{
	int TP;
	int I;
	double T_fraction;
	double I_fraction;
	double dev_rate=0;

	if(T<lower_thresh[1] || T>upper_thresh[1])
	{
		if(T<lower_thresh[1])TP=0;
		else TP=round_off(upper_thresh[1])-round_off(lower_thresh[1]);
		T_fraction=0.0;
	}
	else
	{
		TP=(round_off)(T-lower_thresh[1]);
		T_fraction=T-(lower_thresh[1]+(double)TP);
	}

	I=(round_off)(inhib_titre*100);
	I_fraction=inhib_titre*100-(double)I;

	dev_rate=diapause_table[TP][I]+T_fraction*(diapause_table[TP+1][I]-diapause_table[TP][I])+I_fraction*(diapause_table[TP][I+1]-diapause_table[TP][I]);
	return(max(0,dev_rate));
 }

//Determines the age increment in postdiapause from the lookup table
double postdiapause_rate(double T,double age)
{
	int TP;
	int Iage;
	double T_fraction;
	double age_intpart;
	double age_fraction;
	double dev_rate=0;

	if(T<lower_thresh[2] || T>upper_thresh[2])
	{
		if(T<lower_thresh[2])TP=0;
		else TP=round_off(upper_thresh[2])-round_off(lower_thresh[2]);
		T_fraction=0.0;
	}
	else
	{
		TP=(round_off)(T-lower_thresh[2]);
		T_fraction=T-(lower_thresh[2]+TP);
	}

	age_fraction=modf(((double)age/.05),&age_intpart);
	Iage=(round_off)(age*20);
	dev_rate=postdiapause_table[TP][Iage]+T_fraction*(postdiapause_table[TP+1][Iage]-postdiapause_table[TP][Iage])+age_fraction*(postdiapause_table[TP][Iage+1]-postdiapause_table[TP][Iage]);
	return(dev_rate);
}

//Setup reads necessary parameters from setup file      
/*void setup(void)
{
	FILE *fptrset;
	char setup_file[50]="current.grs";  // BioSIM's set file naming scheme 
										// .grs means gray model set file 
	int i;
	if((fptrset=fopen(setup_file,"r"))== NULL)
	{
		printf("cannot open set file\n");
		exit(0);
	}

	fgets(input_file_name,80,fptrset);
	strip(input_file_name); //gets rid of carriage return 
	fgets(output_file_name,80,fptrset);
	strip(output_file_name); //gets rid of carriage return 
	fscanf(fptrset,"%d",&ovip_date);
	fscanf(fptrset,"%d",&time_step);
	for(i=0; i<3; i++)
	{
		fscanf(fptrset,"%lf",&lower_thresh[i]);
		fscanf(fptrset,"%lf",&upper_thresh[i]);
	}
	fscanf(fptrset,"%lf",&psiI);
	fscanf(fptrset,"%lf",&rhoI);
	fscanf(fptrset,"%lf",&TmI);
	fscanf(fptrset,"%lf",&deltaTI);
	fscanf(fptrset,"%lf",&start_inhibitor);
	fscanf(fptrset,"%lf",&RS_c);
	fscanf(fptrset,"%lf",&RS_rp);
	fscanf(fptrset,"%lf",&RP_c);
	fscanf(fptrset,"%lf",&eff_res_1);
	fscanf(fptrset,"%lf",&eff_res_2);
	fscanf(fptrset,"%lf",&PDR_c);
	fscanf(fptrset,"%lf",&PDR_t);
	fscanf(fptrset,"%lf",&PDR_t2);
	fscanf(fptrset,"%lf",&PDR_t4);
	fscanf(fptrset,"%lf",&tauIII);
	fscanf(fptrset,"%lf",&deltaIII);
	fscanf(fptrset,"%lf",&omegaIII);
	fscanf(fptrset,"%lf",&kappaIII);
	fscanf(fptrset,"%lf",&psiIII);
	fscanf(fptrset,"%lf",&alphaI);
	fscanf(fptrset,"%lf",&betaI);
	fscanf(fptrset,"%lf",&gammaI);
	fscanf(fptrset,"%lf",&alphaII);
	fscanf(fptrset,"%lf",&betaII);
	fscanf(fptrset,"%lf",&gammaII);
	fscanf(fptrset,"%lf",&alphaIII);
	fscanf(fptrset,"%lf",&betaIII);
	fscanf(fptrset,"%lf",&gammaIII);
	fclose(fptrset);
}*/
//Remove all the spaces, newlines, and null at the end of a string
void strip(char *line)
{
	register int i;
	i = strlen(line) - 1;
	while ((line[i] == '\0' || line[i] == '\n' || line[i] == ' ')&&(i>-1))
		--i;
	line[i+1] = '\0';
	return;
}

//Allocates arrays space.
void allocate_arrays(void)
{
	int temp_range_prediapause; 
	int temp_range_diapause;   
	int temp_range_postdiapause;  
	int inhib_titre_range;     
	int days=366;
	int phases=4;                        //3 development phases plus hatched eggs
	int i;
	int hours=24;

	temp_range_prediapause=round_off(upper_thresh[0])-round_off(lower_thresh[0])+1;
	temp_range_diapause=round_off(upper_thresh[1])-round_off(lower_thresh[1])+1;
	temp_range_postdiapause=round_off(upper_thresh[2])-round_off(lower_thresh[2])+1;
	inhib_titre_range=round_off(start_inhibitor*100)+1;

//The tables have 1 extra entry in each dimension. The last entry will be made
//equal to the penultimtate entry in calc_rates(). This ultimate entry in needed
//in diapause_rate() [and others] where a reference is made to
//"diapause_table[TP+1][I]" or "diapause_table[TP][I+1]".
	if((prediapause_table=(double *)malloc((temp_range_prediapause+1)*sizeof(double)))==NULL)print_memory_error();

	diapause_table=(double **)malloc((temp_range_diapause+1)*sizeof(double *));
	for(i=0; i<temp_range_diapause+1; i++)
	{
		if((diapause_table[i]=(double *)malloc((inhib_titre_range+1)*sizeof(double)))==NULL)
		{
			printf("\nNot enough memory.");
			exit(0);
		}
	}

	d_inhibitor_table=(double **)malloc((temp_range_diapause+1)*sizeof(double *));
	for(i=0; i<temp_range_diapause+1; i++)
	{
		if((d_inhibitor_table[i]=(double *)malloc((inhib_titre_range+1)*sizeof(double)))==NULL)
		{
			printf("\nNot enough memory.");
			exit(0);
		}
	}

	postdiapause_table=(double **)malloc((temp_range_postdiapause+1)*sizeof(double *));
	for(i=0; i<temp_range_postdiapause+1; i++)
	{
		if((postdiapause_table[i]=(double *)malloc((age_range_postdiapause+1)*sizeof(double)))==NULL)
		{
			printf("\nNot enough memory.");
			exit(0);
		}
	}

	if((hour_temp=(double *)malloc(hours*sizeof(double)))==NULL)print_memory_error();

	total_eggs=(double **)malloc(days*sizeof(double *));
	for(i=0; i<days; i++)
	{
		if((total_eggs[i]=(double *)malloc(phases*sizeof(double)))==NULL)print_memory_error();
	}

}

//Rounds off double to integers
int round_off(double input)
{
	if(input<0)return((int)(input-0.5));
	else if(input>0)return((int)(input+0.5));
	else return(0);
}

//Make all ages equal to zero and inhibitor titre equal to 1.0
void initialize_ages(void)
{
	int prediapause_class;
	int diapause_class;
	int postdiapause_class;

	for(prediapause_class=0; prediapause_class<number_classes[0]; prediapause_class++)
	{
		prediapause_age[prediapause_class]=0;
		for(diapause_class=0; diapause_class<number_classes[1]; diapause_class++)
		{
			diapause_age[prediapause_class][diapause_class]=0;
			inhibitor_titre[prediapause_class][diapause_class]=start_inhibitor;
			for(postdiapause_class=0; postdiapause_class<number_classes[2]; postdiapause_class++)
			{
				postdiapause_age[prediapause_class][diapause_class][postdiapause_class]=0;
			}
		}
	}
}

//print_memory_error() prints an error due to insufficient error.
void print_memory_error(void)
{
	printf("\nNot enough memory.");
	exit(0);
}

void GrayReset()
{

	int temp_range_prediapause; 
	int temp_range_diapause;   
	int temp_range_postdiapause;  
	int inhib_titre_range;     
	int days=366;
	int phases=4;                        //3 development phases plus hatched eggs
	int i,j,k;
	int hours=24;

	temp_range_prediapause=round_off(upper_thresh[0])-round_off(lower_thresh[0])+1;
	temp_range_diapause=round_off(upper_thresh[1])-round_off(lower_thresh[1])+1;
	temp_range_postdiapause=round_off(upper_thresh[2])-round_off(lower_thresh[2])+1;
	inhib_titre_range=round_off(start_inhibitor*100)+1;

	age_range_postdiapause=AGERANGEPOSTDIAPAUSE;

	for(i=0;i<=temp_range_prediapause;++i)
		prediapause_table[i]=0.;

	for(i=0;i<=temp_range_diapause;++i) {
		for(j=0;j<=inhib_titre_range;++j) {
			d_inhibitor_table[i][j]=0.;
			diapause_table[i][j]=0.;
		}
	}

	for(i=0;i<=temp_range_postdiapause;++i) {
		for(j=0;j<=age_range_postdiapause;++j) {
			postdiapause_table[i][j]=0.;
		}
	}

	for(i=0;i<hours;++i) hour_temp[i]=0.0;
	for(i=0;i<days;++i)
		for(j=0;j<phases;++j)
			total_eggs[i][j]=0.0;

	for(i=0;i<4;++i) {
		for(j=0;j<MAXCLASSES;++j) {
			variability_table[i][j]=0.0;
		}
	}
	
	for(i=0;i<N_CLASS1;++i) {
		prediapause_age[i]=0.0;
		diapause_eggs[i]=0.0;
		for(j=0;j<N_CLASS2;++j) {
			inhibitor_titre[i][j]=0.0;
			diapause_age[i][j]=0.0;
			for(k=0;k<N_CLASS3;++k) {
				postdiapause_age[i][j][k]=0.0;
			}
		}
	}
	number_classes[0]=N_CLASS1;
	number_classes[1]=N_CLASS2;
	number_classes[2]=N_CLASS3;

}

void free_Gray_arrays(void)
{
	int temp_range_diapause;   
	int temp_range_postdiapause;  
	int i;
	int days=366;
	int hours=24;

	temp_range_diapause=round_off(upper_thresh[1])-round_off(lower_thresh[1])+1;
	temp_range_postdiapause=round_off(upper_thresh[2])-round_off(lower_thresh[2])+1;

	free(prediapause_table);

	for(i=0; i<temp_range_diapause+1; i++)
	{
		free(diapause_table[i]);
	}
	free(diapause_table);

	for(i=0; i<temp_range_diapause+1; i++)
	{
		free(d_inhibitor_table[i]);
	}
	free(d_inhibitor_table);

	for(i=0; i<temp_range_postdiapause+1; i++)
	{
		free(postdiapause_table[i]);
	}
	free(postdiapause_table);

	free(hour_temp);

	for(i=0; i<days; i++)
	{
		free(total_eggs[i]);
	}
	free(total_eggs);

}
