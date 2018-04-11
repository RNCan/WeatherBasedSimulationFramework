//**********************************************************************
// 09/04/2018	1.0.0	Rémi Saint-Amant	
//Create from : 
//	Study on Biology and RepRoDUCTIVE METHODEs of European elm scale and the fauna of ELM pests in Esfahan
//	Alireza.Jalalizand
//	2011 International Conference on Life Science and Technology
//	IPCBEE vol.3 (2011)
//**********************************************************************
#include "EuropeanElmScaleModel.h"
#include "Basic/Statistic.h"
#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace std;

//#define DREISTADT 1

namespace WBSF
{

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEuropeanElmScaleModel::CreateObject);
	/*
//calibrated with Iran data 2001
NbVal=   310	Bias= 0.18384	MAE= 0.93641	RMSE= 3.00420	CD= 0.98940	R²= 0.98949
a1                  	=  82.34812
b1                  	=   1.63810
a2                  	= 181.93262
b2                  	=   0.87762
a3                  	= 266.59839
b3                  	=   1.47250
a4                  	= 411.05113
b4                  	=   1.85905
a5                  	= 504.71777
b5                  	=   2.09088
a6                  	= 178.63054
b6                  	=   2.83733
a7                  	= 522.65860
b7                  	=   2.52336
a8                  	=1017.59909
b8                  	=   5.31772
a9                  	= 840.38299
b9                  	=   1.98296
a10                 	=1603.58291
b10                 	=   3.33559

NbVal=   670	Bias= 7.81569	MAE= 9.84479	RMSE=26.84220	CD=-0.11508	R²= 0.41478
Threshold2          	=   6.33936
a6                  	= 101.32419
b6                  	=   5.75855
a7                  	= 432.75379
b7                  	=   2.75850
a8                  	=1052.95957
b8                  	=   3.33844

NbVal=   117	Bias= 0.10132	MAE= 5.19255	RMSE= 8.63095	CD= 0.94738	R²= 0.94836
Threshold1          	=   7.10325
Threshold2          	=  13.24521
a1                  	=  47.04028
b1                  	=   1.40833
a2                  	= 119.06760
b2                  	=   0.78256
a3                  	= 182.84651
b3                  	=   1.49579
a4                  	= 318.61026
b4                  	=   2.63373
a5                  	= 451.10875
b5                  	=   2.99996
a6                  	=  21.35226
b6                  	=   1.75433
a7                  	= 147.35656
b7                  	=   1.81134
a8                  	= 409.26316
b8                  	=   2.97788
a9                  	= 284.22742
b9                  	=   1.63914
a10                 	= 714.63228
b10                 	=   3.13883

NbVal=   117	Bias= 0.21577	MAE= 4.22449	RMSE= 6.92830	CD= 0.96609	R²= 0.96631
Threshold1          	= -10.00243
Threshold2          	=  13.23733
a1                  	= 611.72039
b1                  	=   3.58814
a2                  	=1017.98324
b2                  	=   1.30813
a3                  	=1284.44687
b3                  	=   1.80683
a4                  	=1639.02965
b4                  	=   3.03284
a5                  	=1999.99467
b5                  	=   6.03266
a6                  	=  21.40264
b6                  	=   1.75854
a7                  	= 147.61670
b7                  	=   1.81795
a8                  	= 409.63267
b8                  	=   2.97513
a9                  	= 284.47966
b9                  	=   1.64016
a10                 	= 715.58481
b10                 	=   3.13823

NbVal=    96	Bias= 0.30179	MAE= 5.40989	RMSE= 8.43653	CD= 0.95011	R²= 0.95050
Threshold1          	=   6.00001
Threshold2          	=  11.81299
a1                  	=  64.37699
b1                  	=   1.55465
a2                  	= 150.85519
b2                  	=   0.83828
a3                  	= 223.48437
b3                  	=   1.27219
a4                  	= 353.16505
b4                  	=   2.87740
a5                  	= 506.49296
b5                  	=   5.87729
a6                  	=  33.36606
b6                  	=   2.08711
a7                  	= 188.88424
b7                  	=   1.94698
a8                  	= 503.83621
b8                  	=   3.00771
a9                  	= 351.06319
b9                  	=   1.70991
a10                 	= 837.71498
b10                 	=   3.19140

//12°C
NbVal=    96	Bias= 0.29371	MAE= 5.41221	RMSE= 8.44023	CD= 0.95007	R²= 0.95049
a6                  	=  31.52985
b6                  	=   2.04107
a7                  	= 182.78337
b7                  	=   1.90071
a8                  	= 491.16008
b8                  	=   3.02736
a9                  	= 341.61238
b9                  	=   1.70512
a10                 	= 820.81525
b10                 	=   3.19363

//11.8°C
NbVal=    96	Bias= 0.29578	MAE= 5.41571	RMSE= 8.43818	CD= 0.95009	R²= 0.95049
Threshold2          	=  11.80964
a6                  	=  33.40298
b6                  	=   2.08508
a7                  	= 188.95259
b7                  	=   1.94719
a8                  	= 504.26035
b8                  	=   3.01437
a9                  	= 351.14333
b9                  	=   1.71475
a10                 	= 837.94571
b10                 	=   3.20018

//11°C
NbVal=    96	Bias= 0.28559	MAE= 5.42884	RMSE= 8.46171	CD= 0.94981	R²= 0.95010
a6                  	=  42.51994
b6                  	=   2.22274
a7                  	= 215.88118
b7                  	=   2.16615
a8                  	= 562.59538
b8                  	=   2.94645
a9                  	= 393.95198
b9                  	=   1.75180
a10                 	= 913.06426
b10                 	=   3.22776



NbVal=   117	Bias= 0.13123	MAE= 6.30526	RMSE=11.17164	CD= 0.91184	R²= 0.91331
a1                  	=   8.00009 {   8.00003,1990.90213}	VM={   0.00182,   0.00285}
b1                  	=   3.33458 {   3.33020,   3.34051}	VM={   0.00275,   0.00344}
a2                  	=  22.83155 {  22.12757,  23.54519}	VM={   0.40533,   0.50666}
b2                  	=   0.33476 {   0.33244,   0.34066}	VM={   0.00239,   0.00298}
a3                  	=  45.25632 {  45.15684,  45.53432}	VM={   0.10992,   0.20610}
b3                  	=   0.29469 {   0.25064,   6.90236}	VM={   0.03089,   0.10810}
a4                  	=  98.98609 {  97.77764, 100.11923}	VM={   0.52739,   0.92293}
b4                  	=   2.06371 {   1.98918,   2.18742}	VM={   0.06436,   0.06436}
a5                  	= 166.26690 { 163.88345, 168.96660}	VM={   1.12106,   1.40132}
b5                  	=   4.95604 {   4.71872,   5.13277}	VM={   0.13339,   0.16674}
a6                  	=  21.50469 {  21.28672,  21.96957}	VM={   0.18515,   0.27772}
b6                  	=   1.75762 {   1.64853,   1.86592}	VM={   0.08101,   0.10126}
a7                  	= 148.39710 { 147.13404, 150.46417}	VM={   0.80252,   1.20377}
b7                  	=   1.83093 {   1.77501,   1.88374}	VM={   0.03314,   0.04142}
a8                  	= 412.48674 { 408.06478, 415.92322}	VM={   2.08211,   3.12317}
b8                  	=   2.98037 {   2.86729,   3.03906}	VM={   0.07577,   0.07577}
a9                  	= 285.77093 { 283.23210, 288.52119}	VM={   1.73937,   2.17421}
b9                  	=   1.64801 {   1.57209,   1.72596}	VM={   0.05858,   0.05858}
a10                 	= 717.33767 { 712.93075, 725.20566}	VM={   4.20068,   5.25086}
b10                 	=   3.15571 {   2.90439,   3.33901}	VM={   0.13249,   0.16561}


NbVal = 20	Bias = -1.65000	MAE = 7.35000	RMSE = 8.84590	CD = 0.73460	R² = 0.79322
a12 = 224.92958 
a13 = 366.32084 
b = 1.39300    


//NbVal = 20	Bias = -0.35000	MAE = 4.15000	RMSE = 4.91426	CD = 0.91809	R² = 0.92264
//threshold3 = 6.23059 {   6.22760,   6.23660}	VM = { 0.00087,   0.00292 }
//190.16365 ,1520.40608
//2.23600

*/


	enum TOutput { O_M_NYPH2o, O_M_NYPH2, O_M_PREPUPA, O_M_PUPA, O_M_ADULT, O_M_DEADADULT, O_F_NYPH2o, O_F_NYPH2, O_F_ADULT, O_F_DEAD_ADULT, O_NYPH1, O_NYPH2, O_D_NYPH2, O_D_ADULT, O_D_DEAD_ADULT, NB_OUTPUTS };

	const char EuropeanElmScale_header[] = "MaleNyph2o|MaleNyph2|MalePrepupa|MalePupa|MaleAdult|MaleDeadAdult|FemaleNyph2o|FemaleNyph2|FemaleAdult|FemaleDeadAdult|Nyph1|Nyph2|DFemaleNyph2|DFemaleAdult|DFemaleDeadAdult||DDModel";

	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = VERTICAL;

	const double CEuropeanElmScaleModel::THRESHOLDm = 6;
	const double CEuropeanElmScaleModel::THRESHOLDf = 12;



	

	


	const double CEuropeanElmScaleModel::Am[NB_MALE_PARAMS] =
	{
//		82.34812,181.93262,266.59839,411.05113,504.71777
//		10.00023,20.91792,41.73461,93.08767,157.53361
		64.37699,150.85519,223.48437,353.16505,506.49296
	};

	const double CEuropeanElmScaleModel::Bm[NB_MALE_PARAMS] =
	{
		//1.63810,0.87762,1.47250,1.85905,2.09088
//		3.35765,0.35757,0.25930,2.02215,4.77304
		1.55465,0.83828,1.27219,2.87740,5.87729  //11°C
	};




	const double CEuropeanElmScaleModel::Af[NB_FEMALE_PARAMS] =
	{
		//178.63054,522.65860,1017.59909
		//19.68197,140.56950,394.48497
		//42.51994,215.88118,562.59538//11°C
		//33.40298,188.95259,504.26035//11.8
		31.52985,182.78337,491.16008//12°

	};
	const double CEuropeanElmScaleModel::Bf[NB_FEMALE_PARAMS] =
	{
		//2.83733,2.52336,5.31772
		//1.69913,1.75229,3.00766
		//2.22274,2.16615,2.94645//11°C
		//2.08508,1.94719,3.01437//11.8
		2.04107,1.90071,3.02736//12
	};

	const double CEuropeanElmScaleModel::Ab[NB_BABY_PARAMS] =
	{
		//840.38299,1603.58291
		//273.38215,694.02601
		//393.95198,913.06426//11°C
		//351.14333,837.94571//11.8
		341.61238,820.81525//12
	};

	const double CEuropeanElmScaleModel::Bb[NB_BABY_PARAMS] =
	{
		//1.98296,3.33559
		//1.62714,3.12987
		//1.75180,3.22776//11°C
		//1.71475,3.20018//11.8
		1.70512,3.19363//12
	};


	const double CEuropeanElmScaleModel::Ad[NB_DREISTADT_PARAMS] =
	{
		224.92958,366.32084
		//190.16365 ,1520.40608
	};	  
		

	const double CEuropeanElmScaleModel::Bd =
	{
		1.39300
		//2.23600
	};





	CEuropeanElmScaleModel::CEuropeanElmScaleModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2018)";
		m_firstYear = -999;
		m_lastYear = -999;
	}

	CEuropeanElmScaleModel::~CEuropeanElmScaleModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CEuropeanElmScaleModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		bool bCumul = parameters[c++].GetBool();
		m_CRm.m_startJday = 0;
		m_CRm.m_lowerThreshold = THRESHOLDm;
		m_CRm.m_bCumul = bCumul;
		m_CRm.m_bMultipleVariance = true;
		m_CRm.m_bPercent = true;
		m_CRm.m_bAdjustFinalProportion = true;
		//m_CRm.m_method = CEuropeanElmScaleCRm::DAILY_AVERAGE;
		m_CRm.m_method = CEuropeanElmScaleCRf::SINGLE_SINE;

		m_CRf.m_startJday = 0;
		m_CRf.m_lowerThreshold = THRESHOLDf;
		m_CRf.m_bCumul = bCumul;
		m_CRf.m_bMultipleVariance = true;
		m_CRf.m_bPercent = true;
		m_CRf.m_bAdjustFinalProportion = true;
		m_CRf.m_method = m_CRm.m_method;

		m_CRb.m_startJday = m_CRf.m_startJday;
		m_CRb.m_lowerThreshold = m_CRf.m_lowerThreshold;//same as femelle to keep validity
		m_CRb.m_bCumul = m_CRf.m_bCumul;
		m_CRb.m_bMultipleVariance = m_CRf.m_bMultipleVariance;
		m_CRb.m_bPercent = m_CRf.m_bPercent;
		m_CRb.m_bAdjustFinalProportion = m_CRf.m_bAdjustFinalProportion;
		m_CRb.m_method = m_CRf.m_method;


		m_CRd.m_startJday = 60;
		m_CRd.m_lowerThreshold = 11;//6.25;// //same as femelle to keep validity
		m_CRd.m_bCumul = bCumul;
		m_CRd.m_bMultipleVariance = false;
		m_CRd.m_bPercent = true;
		m_CRd.m_bAdjustFinalProportion = false;
		m_CRd.m_method = CEuropeanElmScaleCRd::SINGLE_SINE;


		for (size_t i = 0; i < NB_MALE_PARAMS; i++)
		{
			m_CRm.m_a[i] = Am[i];
			m_CRm.m_b[i] = Bm[i];
		}

		for (size_t i = 0; i < NB_FEMALE_PARAMS; i++)
		{
			m_CRf.m_a[i] = Af[i];
			m_CRf.m_b[i] = Bf[i];
		}

		for (size_t i = 0; i < NB_BABY_PARAMS; i++)
		{
			m_CRb.m_a[i] = Ab[i];
			m_CRb.m_b[i] = Bb[i];
		}

		for (size_t i = 0; i < NB_DREISTADT_PARAMS; i++)
		{
			m_CRd.m_a[i] = Ad[i];
		}
		//m_CRd.m_b[0] = 1.97098;
		//m_CRd.m_b[1] = 1.82252;

		m_CRd.m_b_ = Bd;

		if (parameters.size() > 1)
		{
			m_CRm.m_lowerThreshold = parameters[c++].GetFloat();
			m_CRf.m_lowerThreshold = parameters[c++].GetFloat();
			
			m_CRb.m_lowerThreshold = m_CRf.m_lowerThreshold;


			for (size_t i = 0; i < NB_MALE_PARAMS; i++)
			{
				m_CRm.m_a[i] = parameters[c++].GetFloat();
				m_CRm.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_FEMALE_PARAMS; i++)
			{
				m_CRf.m_a[i] = parameters[c++].GetFloat();
				m_CRf.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_BABY_PARAMS; i++)
			{
				m_CRb.m_a[i] = parameters[c++].GetFloat();
				m_CRb.m_b[i] = parameters[c++].GetFloat();
			}

			//m_CRd.m_b[0] = parameters[c++].GetFloat();
			parameters[c++].GetFloat();
			
			m_CRd.m_lowerThreshold = parameters[c++].GetFloat();
			for (size_t i = 0; i < NB_DREISTADT_PARAMS; i++)
			{
				m_CRd.m_a[i] = parameters[c++].GetFloat();
			}
			//m_CRd.m_b[1] = parameters[c++].GetFloat();
			m_CRd.m_b_ = parameters[c++].GetFloat();

		}

		return msg;
	}

	ERMsg CEuropeanElmScaleModel::OnExecuteDaily()
	{
		ERMsg msg;




		CModelStatVector output_m;
		CModelStatVector output_f;
		CModelStatVector output_b;
		CModelStatVector output_d;
		//CModelStatVector dd;
		m_CRm.Execute(m_weather, output_m);
		m_CRf.Execute(m_weather, output_f);
		m_CRb.Execute(m_weather, output_b);
		m_CRd.Execute(m_weather, output_d);
		//m_DD.Execute(m_weather, dd);
		//double DD = 0;
		m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUTS);
		ASSERT(output_m.size() == output_m.size());
		for (size_t d = 0; d < output_m.size(); d++)
		{

			m_output[d][O_M_NYPH2o] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 0], 1);
			m_output[d][O_M_NYPH2] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 1], 1);
			m_output[d][O_M_PREPUPA] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 2], 1);
			m_output[d][O_M_PUPA] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 3], 1);
			m_output[d][O_M_ADULT] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 4], 1);
			m_output[d][O_M_DEADADULT] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 5], 1);
			m_output[d][O_F_NYPH2o] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 0], 1);
			m_output[d][O_F_NYPH2] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 1], 1);
			m_output[d][O_F_ADULT] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 2], 1);
			m_output[d][O_F_DEAD_ADULT] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 3], 1);
			m_output[d][O_NYPH1] = WBSF::Round(output_b[d][CEuropeanElmScaleCRb::O_FIRST_STAGE + 1], 1);
			m_output[d][O_NYPH2] = WBSF::Round(output_b[d][CEuropeanElmScaleCRb::O_FIRST_STAGE + 2], 1);
			m_output[d][O_D_NYPH2] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 0], 1);
			m_output[d][O_D_ADULT] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1], 1);
			m_output[d][O_D_DEAD_ADULT] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 2], 1);

			//if(d>60)
				//DD+=dd[d][0];

			//m_output[d][O_DD_MODEL] = (DD >= 641.25 && DD <= 771.1) ? 100 : 0;

		}
		//Using Growing Degree Days For Insect Management
		//Nancy E.Adams
		//Extension Educator, Agricultural Resources
		//base 50°F (10°C)
		//GDD data is collected beginning March 1
		//1026 - 1388 (641.25 - 771.1)

		//Dreistadt, S. H. and K.S. Hagen. 1994. European elm scale (Homoptera: Eriococcidae) abundance and parasitism in northern California. Pan-Pacific Entomologist 70:240-252.
		//Location of study : Northern California(field studies)
		//	Developmental threshold
		//	Lower : 11°C
		//	Method of calculation : Single Sine
		//	Degree - day accumulations required for each stage of development
		//	Biofix : March 1
		//	Host : English Elm and Siberian Elm	DD(°C)
		//	Female scale peak	301


		return msg;
	}


	void CEuropeanElmScaleModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		CTRef ref(ToShort(data[1]), ToShort(data[2]) - 1, ToShort(data[3]) - 1);

		ASSERT(header.size() == 18);

		std::vector<double> obs(NB_INPUT+1);
		for (size_t i = 0; i < NB_INPUT; i++)
		{
			obs[i] = ToDouble(data[i + 4]);//Cumulative
		}
		obs[NB_INPUT] = ToInt(data[0]);

		m_SAResult.push_back(CSAResult(ref, obs));
	}

	void CEuropeanElmScaleModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//remove unused years
			if (m_firstYear == -999 && m_lastYear == -999)
			{
				CStatistic years;
				for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
					years += p->m_ref.GetYear();

				m_firstYear = (int)years[LOWEST];
				m_lastYear = (int)years[HIGHEST];
				while (m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < m_firstYear)
					m_weather.erase(m_weather.begin());

				while (m_weather.GetNbYears() > 1 && m_weather.GetLastYear() > m_lastYear)
					m_weather.erase(--m_weather.end());

			}


			//look to see if all ai are in growing order
			bool bValid = true;

			for (int i = 1; i < NB_MALE_PARAMS&&bValid; i++)
			{
				if (m_CRm.m_a[i] < m_CRm.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRm.m_b[i] - m_CRm.m_b[i - 1]) > 3)
					bValid = false;
			}


			for (int i = 1; i < NB_FEMALE_PARAMS&&bValid; i++)
			{
				if (m_CRf.m_a[i] < m_CRf.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRf.m_b[i] - m_CRf.m_b[i - 1]) > 3)
					bValid = false;
			}

			/*	if (m_CRb.m_a[EGG_NYPH1] < m_CRf.m_a[F_NYPH2_ADULT])
					bValid = false;

				if (fabs(m_CRb.m_b[EGG_NYPH1] - m_CRf.m_b[F_NYPH2_ADULT]) > 3)
					bValid = false;*/

			for (int i = 1; i < NB_BABY_PARAMS&&bValid; i++)
			{
				if (m_CRb.m_a[i] < m_CRb.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRb.m_b[i] - m_CRb.m_b[i - 1]) > 3)
					bValid = false;
			}

			for (int i = 1; i < NB_DREISTADT_PARAMS&&bValid; i++)
			{
				if (m_CRd.m_a[i] < m_CRd.m_a[i - 1])
					bValid = false;
			}


			if (bValid)
			{
				//******************************************************************************************************************************************************
				//Vertical lookup

#ifndef DREISTADT
					//Male
				{
					CModelStatVector output;
					m_CRm.m_bCumul = false;
					m_CRm.Execute(m_weather, output);

					/*CStatistic obsStat[NB_MALE_PARAMS];
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						for (size_t v = 0; v < NB_MALE_PARAMS; v++)
						{
							if (m_SAResult[i].m_obs[I_M_NYPH2o + v] > 0)
								obsStat[v] += m_SAResult[i].m_obs[I_M_NYPH2o + v];
						}
					}


					CStatistic simStat[NB_MALE_PARAMS];
					for (size_t i = 0; i < output.size(); i++)
					{
						for (size_t v = 0; v < NB_MALE_PARAMS; v++)
						{
							if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] > 0)
								simStat[v] += output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v];
						}
					}
*/
//if (simStat[NB_MALE_PARAMS - 1][HIGHEST] > 0)
					{
						for (size_t v = 0; v < NB_MALE_PARAMS; v++)
						{
							//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
							double c = 1;// obsStat[v][(v == 0 ? HIGHEST : MEAN)] / simStat[v][(v == 0 ? HIGHEST : MEAN)];
							for (size_t i = 0; i < m_SAResult.size(); i++)
							{
								double obs = m_SAResult[i].m_obs[I_M_NYPH2o + v];
								if (obs > -999 &&
									output.IsInside(m_SAResult[i].m_ref))
								{
									double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] * c;
									stat.Add(sim, obs);
								}
							}
						}
					}
				}
				 
				//Female
				{

					CModelStatVector output;
					m_CRf.m_bCumul = false;
					m_CRf.Execute(m_weather, output);

					//CStatistic obsStat[NB_FEMALE_PARAMS];
					//for (size_t i = 0; i < m_SAResult.size(); i++)
					//{
					//	for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
					//	{
					//		if (m_SAResult[i].m_obs[I_F_NYPH2o + v] > 0)
					//			obsStat[v] += m_SAResult[i].m_obs[I_F_NYPH2o + v];
					//	}
					//}


					//CStatistic simStat[NB_FEMALE_PARAMS];
					//for (size_t i = 0; i < output.size(); i++)
					//{
					//	for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
					//	{
					//		if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] > 0)
					//			simStat[v] += output[i][CEuropeanElmScaleCRf::O_FIRST_STAGE + v];
					//	}
					//}

					//if (simStat[NB_FEMALE_PARAMS - 1][HIGHEST])
					{ 
						for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
						{
							//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
							double c = 1;// obsStat[v][MEAN] / simStat[v][MEAN];
							for (size_t i = 0; i < m_SAResult.size(); i++)
							{
								double obs = m_SAResult[i].m_obs[I_F_NYPH2o + v];
								if (obs > -999 &&
									output.IsInside(m_SAResult[i].m_ref))
								{
									double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRf::O_FIRST_STAGE + v] * c;
									stat.Add(sim, obs);
								}
							}
						}
					}
				}


				//Baby
				{

					CModelStatVector output;
					m_CRb.m_bCumul = false;
					m_CRb.Execute(m_weather, output);

					/*CStatistic obsStat[NB_BABY_PARAMS];
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						for (size_t v = 0; v < NB_BABY_PARAMS; v++)
						{
							if (m_SAResult[i].m_obs[I_NYPH1 + v] > 0)
								obsStat[v] += m_SAResult[i].m_obs[I_NYPH1 + v];
						}
					}


					CStatistic simStat[NB_BABY_PARAMS];
					for (size_t i = 0; i < output.size(); i++)
					{
						for (size_t v = 0; v < NB_BABY_PARAMS; v++)
						{
							if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v + 1] > 0)
								simStat[v] += output[i][CEuropeanElmScaleCRb::O_FIRST_STAGE v + 1];
						}
					}
*/
//if (simStat[NB_BABY_PARAMS - 1][HIGHEST])
//{
					for (size_t v = 0; v < NB_BABY_PARAMS; v++)
					{
						//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
						double c = 1;// obsStat[v][(v == 0 ? MEAN : HIGHEST)] / simStat[v][(v == 0 ? MEAN : HIGHEST)];
						for (size_t i = 0; i < m_SAResult.size(); i++)
						{
							double obs = m_SAResult[i].m_obs[I_EGG + v];
							if (obs > -999 &&
								output.IsInside(m_SAResult[i].m_ref))
							{
								double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRb::O_FIRST_STAGE + v] * c;
								stat.Add(sim, obs);
							}
						}
					}
					//}
				}
#else
				{
					CModelStatVector output;
					m_CRd.m_bCumul = false;
					m_CRd.Execute(m_weather, output);

					CTPeriod p = output.GetTPeriod();
					for (size_t y = 0; y < p.GetNbYears(); y++)
					{
						CTRef peakTRef;
						double peak = 0;
						CStatistic::SetVMiss(-999);
						
						for (CTRef d = p.GetFirstAnnualTRef(y); d <= p.GetLastAnnualTRef(y); d++)
						{
							if (output[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1] > peak)
							{
								peak = output[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1];
								peakTRef = d;
							}
						}
						if (peakTRef.IsInit())
						{
							ASSERT(m_SAResult.size() == 1);

							size_t i = (m_SAResult[0].m_obs[NB_INPUT]<=4)?0:1;
							//static const double obs[2] = { 301, 305 };
							static const double obs[2] = { 161, 125 };
							//for (size_t t = 0; t < 2; t++)
							{
								//double sim = statDD[t == 0 ? MEAN : STD_DEV];
								//double DD = output[peakTRef][CEuropeanElmScaleCRd::O_DD];
								//stat.Add(DD, obs[i]);
								stat.Add(peakTRef.GetJDay(), obs[i]);
							}
						}
					}
				}
#endif

			}
		}
	}
}
