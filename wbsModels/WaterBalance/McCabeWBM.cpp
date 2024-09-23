//Monthly Water-Balance Model
//
//2024-09-16	Remi Saint-Amant	Convert from McCabe 2010 article and java code
//



#include <assert.h>
#include <math.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include "McCabeWBM.h"

namespace McCabeWBM
{

	double McCabeWaterBalanceModel::getDayLength(int month, double latitude)
	{
		static const double MID_DOY_MONTH[] = { 15.0, 45.0, 74.0, 105.0, 135.0, 166.0, 196.0,
											   227.0, 258.0, 288.0, 319.0, 349.0
		};


		assert(month >= 1 && month <= 12);
		assert(latitude >= -90 && latitude <= 90);

		double dl = 12.0;

		double dayl = MID_DOY_MONTH[month - 1] - 80.;


		double decd = 23.45 * sin(dayl / 365. * 6.2832);
		double decr = decd * 0.017453;
		double alat = latitude * 0.017453;

		double sintheta = tan(alat) * tan(decr);
		double theta = asin(sintheta);

		if (sintheta <= -1.0)
		{
			dl = 0.0;
		}
		else if (sintheta >= 1.0)
		{
			dl = 24.0;
		}
		else
		{
			dl = 12. + 2. * theta * 24. / (2.0 * 3.174);
		}

		return dl;
	}

	double McCabeWaterBalanceModel::getSnowProportion(double tc, double p, double tsnow, double train)

	{
		double psnow = 0.0;
		if (tc <= tsnow) psnow = p;
		else if (tc < train) psnow = p * ((train - tc) / (train - tsnow));

		return psnow;
	}


	double McCabeWaterBalanceModel::getHammonPET(int month, double tc, double dl)
	{
		static const double NB_DAY_PER_MONTH[] = { 31.0, 28.0, 31.0, 30.0, 31.0, 30.0, 31.0,
												  31.0, 30.0, 31.0, 30.0, 31.0
		};

		double pt = 4.95 * exp(0.062 * tc) / 100.;
		double pe = 0.55 * ((dl / 12.0) * (dl / 12.0)) * pt * NB_DAY_PER_MONTH[month - 1];
		if (pe <= 0.0) pe = 0.0;
		pe = pe * 25.4;
		return pe;
	}


	double McCabeWaterBalanceModel::getSnowMelt(double tc, double snstor, double tsnow, double train, double xmeltmax)
	{
		double snowMelt = 0.0;
		if (snstor > 0.0 && tc > tsnow)
		{
			if (snstor <= 10.0)
			{
				snowMelt = snstor;
			}
			else
			{
				double fracMelt = ((tc - tsnow) / ((train - tsnow) + 0.0001)) * xmeltmax;
				if (fracMelt > xmeltmax) fracMelt = xmeltmax;
				snowMelt = fracMelt * snstor;
			}
		}

		return snowMelt;
	}

	

	Output McCabeWaterBalanceModel::ComputeWaterBalance(Parameters params, Input input, double& soilStor, double& remain, double& snowStor)
	{
		Output output;
		//  Input parameters 
		double waterHoldingCapacity = params["soilMoistureStorage"].m_value;
		double runoffFactor = params["runoffFactor"].m_value / 100.0;
		double Train = params["rainTempThreshold"].m_value;
		double Tsnow = params["snowTempThreshold"].m_value;
		double directfac = params["directRunoffFactor"].m_value / 100.0;
		double xmeltmax = params["maxMeltRate"].m_value / 100.0;
		double latitude = params["latitude"].m_value;

		int month = input.m_month;
		double tc = input.m_temp;
		double p = input.m_prcp;

		//  calculate day length
		double dl = getDayLength(month, latitude);

		//  calculate Hamon PET
		double pe = getHammonPET(month, tc, dl);

		// COMPUTE SNOW AND RAIN
		double psnow = getSnowProportion(tc, p, Tsnow, Train);
		double prain = p - psnow;
		double directRunoff = prain * directfac;

		//adjust rain and snow strorage
		prain = prain - directRunoff;
		snowStor = snowStor + psnow;


		// COMPUTE SNOW MELT
		double snowMelt = getSnowMelt(tc, snowStor, Tsnow, Train, xmeltmax);

		//remove snow melt from snow storage
		snowStor = std::max(0.0, snowStor - snowMelt);


		// ADD SNOW MELT TO PRAIN
		prain = prain + snowMelt;


		// Compute pmpe
		double pmpe = prain - pe;

		// COMPUTE SOIL-MOISTURE STORAGE, AE, SURPLUS, AND DEFICIT
		double surplus = 0.0;
		double ae = 0.0;
		if (pmpe < 0.0)
		{
			double stor = std::max(0.0, soilStor - abs(pmpe * soilStor / waterHoldingCapacity));
			double delstor = stor - soilStor;
			ae = prain + delstor * (-1.0);
			soilStor = stor;
			surplus = 0.0;

		}
		else
		{
			ae = pe;

			double stor = soilStor + pmpe;
			if (stor > waterHoldingCapacity)
			{
				surplus = stor - waterHoldingCapacity;
				stor = waterHoldingCapacity;
			}
			soilStor = stor;
		}

		double deficit = pe - ae;


		//  RUNOFF CALCULATIONS
		double runoff = (surplus + remain) * runoffFactor;
		remain = std::max(0.0, (surplus + remain) - runoff);
		double totalRunoff = runoff + directRunoff;

		//  Put the calculated variables into the output data
		output.m_year = input.m_year;
		output.m_month = input.m_month;
		output.m_temp = tc;
		output.m_prcp = p;
		output.m_pet = pe;
		output.m_precipPet = pmpe;
		output.m_soilMoist = soilStor;
		output.m_aet = ae;
		output.m_petAet = deficit;
		output.m_snow = snowStor;
		output.m_surplus = surplus;
		output.m_totalRunoff = totalRunoff;
		output.m_directRunoff = directRunoff;
		output.m_snowMelt = snowMelt;

		return output;

	}

	Outputs McCabeWaterBalanceModel::Execute(Parameters params, Inputs inputs)
	{
		Outputs outputs(inputs.size());

		double soilStor = 150.0;
		double remain = 25.4;
		double snowStor = 0.0;
		

		//init with the 2 first years
		for (size_t i = 0; i < std::min(size_t(24), inputs.size()); i++)
		{
			ComputeWaterBalance(params, inputs[i], soilStor, remain, snowStor);
		}

		//run for all inputs
		for (size_t i = 0; i < inputs.size(); i++)
		{
			outputs[i] = ComputeWaterBalance(params, inputs[i], soilStor, remain, snowStor);
		}

		return outputs;
	}







	//************************************************************************************************
	//Parameters
	Parameters GetDefaultParameters()
	{
		Parameters params;

		params["soilMoistureStorage"] = Parameter(200, 0, 1500);//Millimeters
		params["runoffFactor"] = Parameter(50, 0, 100);//%
		params["rainTempThreshold"] = Parameter(0, 0, 5);//Degrees Celsius
		params["snowTempThreshold"] = Parameter(0, -15, 0);//Degrees Celsius
		params["directRunoffFactor"] = Parameter(5, 0, 100);//%
		params["maxMeltRate"] = Parameter(50, 0, 100);//%
		params["latitude"] = Parameter(45, -90, 90);//Degrees



		return params;
	}

	//************************************************************************************************
	//Inputs

	Inputs ReadInputFile(std::string filePath)
	{
		Inputs inputs;


		std::ifstream inFile;
		inFile.open(filePath);

		if (inFile.is_open())
		{
			std::string line;
			while (getline(inFile, line, '\n'))
			{
				std::istringstream s(line);

				std::vector<double> vec;
				double i = 0;//this variable will be used to add element into the vector
				//take input(from s to i) and then checks stream's eof flag status
				while (s >> i || !s.eof())
				{
					if (s.fail())
					{
						//clear the error state to allow further operations on s
						s.clear();
						std::string temp;
						s >> temp;
						continue;
					}
					else
					{
						vec.push_back(i);
					}


				}

				if (vec.size() == 4)
				{
					assert(vec[0] >= 1700 && vec[0] <= 2100);
					assert(vec[1] >= 1 && vec[1] <= 12);
					assert(vec[2] >= -60 && vec[2] <= 60);
					assert(vec[3] >= 0 && vec[3] <= 9999);
					inputs.push_back(Input(int(vec[0]), int(vec[1]), vec[2], vec[3]));
				}
			}

			inFile.close();
		}
		else
		{
			std::cout << "file could not be read" << std::endl;
		}

		return inputs;

	}




}
