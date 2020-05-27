//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2020	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <math.h>
#include <sstream>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include "basic/xml/zen/stl_tools.h"

#include "Basic/OpenMP.h"
#include "Basic/UtilMath.h"
#include "Basic/ModelStat.h"
#include "Basic/CSV.h"
//#include "Basic/FileStamp.h"
#include "Basic/UtilStd.h"
#include "FileManager/FileManager.h"
#include "ModelBase/CommunicationStream.h"
#include "ModelBase/WGInput-ModelInput.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/WeatherGenerator.h"
#include "Simulation/LoadStaticData.h"
#include "Simulation/DevRateParameterization.h"

#include "WeatherBasedSimulationString.h"



using namespace std;
using namespace boost;
using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;
using namespace WBSF::DevRateInput;


namespace WBSF
{

	//**********************************************************************************************

	double Regniere2012(double sigma, double obs_time1, double obs_time2, double sim_time1, double sim_time2)
	{
		ASSERT(obs_time1 >= 0 && obs_time2 > 0);

		std::mt19937 gen;
		boost::math::lognormal_distribution<double> uniformLogNormal(-WBSF::Square(sigma) / 2.0, sigma);
		static const double MIN_VAL = 1E-020;

		double B = obs_time1;
		double D = obs_time2;
		double F = max(MIN_VAL, sim_time1);
		double G = max(MIN_VAL, sim_time2);
		/*double H = max(0.0, min(1.0, (B / F + (D - 1) / G)));
		double I = max(0.0, min(1.0, (B / F + D / G)));*/
		double H = (B / F + (D - 1) / G);
		double I = (B / F + D / G);
		double J = max(MIN_VAL, cdf(uniformLogNormal, I) - cdf(uniformLogNormal, H));
		double K = log(J);
		//double L = I * F;
		//double M = 1 / L;
		//double N = log(I);


		//max likelyhoude = =SOMMEPROD(K2:K73,E2:E73)
		//mean epsylon =SOMMEPROD(N:N,E:E)/SOMME(E:E)

		return K;
	}

	/*double SaintAmant2020(TDevRateEquation  e, CComputationVariable& computation, double Tmin, double Tmax, double obs_time)
	{
		double RDR_s = computation.m_XP.back();

		std::mt19937 gen;
		boost::math::lognormal_distribution<double> LogNormal(-WBSF::Square(RDR_s) / 2.0, RDR_s);
		static const double MIN_VAL = 1E-020;

		double i = 0;
		double j = 0;

		for (size_t h = 0; h < 24; h++)
		{
			double theta = 2 * PI*h / 24.0;
			double T = (Tmax + Tmin) / 2 + (Tmax - Tmin) / 2 * cos(theta);
			double sim_time = max(MIN_VAL, min(1000.0, 1.0 / CDevRateEquation::GetRate(e, computation.m_XP, T)));

			i += ((obs_time - 1) / sim_time)/24;
			j += (obs_time / sim_time) / 24;
		}

		double ij = cdf(LogNormal, j) - cdf(LogNormal, i);
		double epsij = log(ij);


		return epsij;
	}*/


	//double SaintAmant2020(TDevRateEquation  e, CComputationVariable& computation, double Tmin, double Tmax, double obs_time)
	//{
	//	double RDR_s = computation.m_XP.back();

	//	std::mt19937 gen;
	//	boost::math::lognormal_distribution<double> LogNormal(-WBSF::Square(RDR_s) / 2.0, RDR_s);
	//	static const double MIN_VAL = 1E-020;


	//	double epsij = 0;
	//	double delta_t = 1.0 / 24.0;

	//	for (size_t h = 0; h < 24; h++)
	//	{
	//		double theta = 2 * PI*h / 24.0;
	//		double T = (Tmin + Tmax) / 2 + (Tmax - Tmin) / 2 * cos(theta);
	//		//double sim_time = max(MIN_VAL, min(1000.0, 1.0 / CDevRateEquation::GetRate(e, computation.m_XP, T)));
	//		//double i = ((obs_time - delta_t) / sim_time);
	//		//double j = (obs_time / sim_time);
	//		//double ij = cdf(LogNormal, j) - cdf(LogNormal, i);
	//		//epsij += log(ij) / 24;

	//		double r = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T));
	//		double i = (obs_time - delta_t) * r;
	//		double j = obs_time * r;
	//		double ij = cdf(LogNormal, j) - cdf(LogNormal, i);
	//		epsij += log(ij) / 24;
	//	}

	//	return epsij;
	//}


	//double Regniere2020(TDevRateEquation  e, CComputationVariable& computation, size_t TType, double Tmin, double Tmax, double ti)
	//{
	//	double RDR_s = computation.m_XP.back();
	//	boost::math::lognormal_distribution<double> LogNormal(log(1) - Square(RDR_s) / 2.0, RDR_s);

	//	double xi = 0;//integral (hourly summation) of daily rate for variable hourly temperature at day ti
	//	double xiˉ¹ = 0;//integral (hourly summation) of daily rate for variable hourly temperature at day ti-1
	//	for (size_t t = 0; t < ti; t++)
	//	{
	//		for (size_t h = 0; h < 24; h++)
	//		{
	//			double theta = 2 * PI*h / 24.0;
	//			double T = (Tmin + Tmax) / 2 + (Tmax - Tmin) / 2 * sin(theta);
	//			double rate = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T)) / 24.0;//hourly rate

	//			xi += rate;//sum of hourly rate
	//			if(t<(ti-1))
	//				xiˉ¹ += rate;
	//		}
	//	}

	//	//avoid division by zero
	//	xi = max(1E-20, xi);
	//	xiˉ¹ = max(1E-20, xiˉ¹);
	//	
	//	//compute probability of changing stage between ti-1 and ti
	//	double pi = max(1E-20, cdf(LogNormal, 1.0 / xiˉ¹) - cdf(LogNormal, 1.0 / xi));

	//	return log(pi);
	//}
	CStatisticEx GetTimeStat(TDevRateEquation  e, CComputationVariable& computation, const vector<double>& T, double obs_time)
	{
		CStatisticEx stat;
		for (size_t t = 0; t < obs_time; t++)
		{
			for (size_t h = 0; h < 24; h++)
			{
				double rate = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h]));//daily rate
				stat += 1.0/rate;//sum of hourly rate
			}
		}

		return stat;
	}


	CStatisticEx GetRateStat(TDevRateEquation  e, CComputationVariable& computation, const vector<double>& T, double obs_time)
	{
		CStatisticEx stat;
		for (size_t t = 0; t < obs_time; t++)
		{
			for (size_t h = 0; h < 24; h++)
			{
				double rate = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h]));//daily rate
				if (rate < 0)
					rate = -exp(1000 / rate);//let the code to give a chance to converge to the good direction
				else if (rate > 1)
					rate = 1 + exp(-1000 / (rate - 1));

				stat += rate;//sum of hourly rate
			}
		}

		return stat;
	}

	double GetRelDevRate(TDevRateEquation  e, CComputationVariable& computation, const vector<double>& T, double obs_time)
	{
		ASSERT(obs_time > 0);

		double mean_rate = GetRateStat(e, computation, T, obs_time)[MEAN];
		double obs_rate = 1.0 / obs_time;
		return obs_rate / mean_rate;//Relative development rate
	}

	double Regniere2020(TDevRateEquation  e, double sigma, CComputationVariable& computation, const vector<double>& T, double ti)
	{
		//double sigma = computation.m_XP.back();
		ASSERT(sigma > 0);

		boost::math::lognormal_distribution<double> LogNormal(log(1) - Square(sigma) / 2.0, sigma);

		double xi = 0;//integral (hourly summation) of daily rate for variable hourly temperature at day ti
		double xiˉ¹ = 0;//integral (hourly summation) of daily rate for variable hourly temperature at day ti-1
		for (size_t t = 0; t < ti; t++)
		{
			for (size_t h = 0; h < 24; h++)
			{
				double rate = max(0.0, CDevRateEquation::GetRate(e, computation.m_XP, T[t * 24 + h])) / 24.0;//hourly rate

				xi += rate;//sum of hourly rate
				if (t < (ti - 1))
					xiˉ¹ += rate;
			}
		}

		//avoid division by zero
		xi = max(1E-20, xi);
		xiˉ¹ = max(1E-20, xiˉ¹);

		//compute probability of changing stage between ti-1 and ti
		double pi = max(1E-200, cdf(LogNormal, 1.0 / xiˉ¹) - cdf(LogNormal, 1.0 / xi));

		return log(pi);
	}

	//**********************************************************************************************
	//CTobsSeries

	const char* CTobsSeries::INPUT_NAME[NB_TOBS_COL] = { "Tid","T" };

	TTobsCol CTobsSeries::get_input(const std::string& name)
	{
		TTobsCol col = C_UNKNOWN;

		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto &s) {return boost::iequals(s, name); });
		if (it != std::end(INPUT_NAME))
			col = static_cast<TTobsCol>(distance(std::begin(INPUT_NAME), it));

		return col;

	}

	size_t CTobsSeries::get_pos(TTobsCol c)const
	{
		size_t pos = NOT_INIT;
		auto it = std::find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;
	}

	ERMsg CTobsSeries::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		return msg;
	}

	ERMsg CTobsSeries::load(std::istream& io)
	{
		ERMsg msg;

		clear();
		m_input_pos.clear();
		//StringVector header;
		size_t pID = NOT_INIT;
		size_t pT = NOT_INIT;

		for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (m_input_pos.empty())
			{
				for (size_t i = 0; i < loop.Header().size(); i++)
					m_input_pos.push_back(get_input(loop.Header()[i]));

				if (!have_var(C_TID))
				{
					msg.ajoute("Mandatory missing column. \"ID\" must be define");
				}

				//check for mandatory columns
				if (!have_var(C_T))
				{
					msg.ajoute("Mandatory missing column: \"T\" must be define");
				}

				pID = get_pos(C_TID);
				pT = get_pos(C_T);
			}

			if (msg)
			{
				if (pID < loop->size() &&
					pT < loop->size())
				{
					double T = ToDouble((*loop)[pT]);
					(*this)[(*loop)[pID]].push_back(T);
				}
			}
		}



		return msg;
	}

	ERMsg CTobsSeries::verify(const CDevRateDataRowVector& data)const
	{
		ERMsg msg;

		for (size_t i = 0; i < data.size(); i++)
		{
			if (data[i].m_TType == T_OBS)
			{
				if (find(data[i].m_Tprofile) == end())
				{
					msg.ajoute("Temperature ID" + data[i].m_Tprofile + " not found in temperature file");
				}
				else
				{
					size_t nb_hours = at(data[i].m_Tprofile).size();
					size_t needed_hours = (size_t)ceil(data[i].at(I_TIME) * 24);
					if (nb_hours < needed_hours)
					{
						msg.ajoute("Temperature profile for ID " + data[i].m_Tprofile + " don't have enought data");
						msg.ajoute("Profile have " + to_string(nb_hours) + " hours and " + to_string(needed_hours) + " is needed");
					}
				}

			}
		}

		return msg;
	}

	//generate temperature profile
	void CTobsSeries::generate(const CDevRateDataRowVector& data)
	{
		map<string, size_t> Tprofile;
		for (size_t i = 0; i < data.size(); i++)
		{
			if (data[i].m_TType != T_OBS)
			{
				if (Tprofile.find(data[i].m_Tprofile) == Tprofile.end())
				{
					Tprofile[data[i].m_Tprofile] = i;
				}
				else
				{
					if (data[i].at(I_TIME) > data[Tprofile[data[i].m_Tprofile]].at(I_TIME))
						Tprofile[data[i].m_Tprofile] = i;
				}
			}
		}

		for (auto it = Tprofile.begin(); it != Tprofile.end(); it++)
		{
			size_t i = it->second;
			size_t needed_days_max = (size_t)ceil(data[i].at(I_TIME));


			double T = data[i].at(I_T);
			double Tmin = data[i].at(I_TMIN);
			double Tmax = data[i].at(I_TMAX);

			for (size_t t = 0; t < needed_days_max; t++)
			{
				for (size_t h = 0; h < 24; h++)
				{
					double Ti = T;
					if (data[i].m_TType == T_SINUS)
					{
						double theta = 2 * PI*h / 24.0;
						Ti = (Tmin + Tmax) / 2 + (Tmax - Tmin) / 2 * sin(theta);
					}
					else if (data[i].m_TType == T_TRIANGULAR)
					{
						Ti = (Tmin + Tmax) / 2 + (Tmax - Tmin) / 2 * (fabs(12.0 - ((t + 6) % 24)) - 6.0) / 6.0;
					}

					(*this)[data[i].m_Tprofile].push_back(Ti);
				}
			}
		}
	}



	//**********************************************************************************************
			//CDevRateInput

	static const char* DevRateInput::TTYPE_NAME[DevRateInput::NB_TMP_TYPE] = { "const","sin","tri","obs" };
	TTemperature DevRateInput::get_TType(const std::string& name)
	{
		TTemperature  col = T_UNKNOWN;

		auto it = find_if(std::begin(TTYPE_NAME), std::end(TTYPE_NAME), [&](auto &s) {return boost::iequals(s, name); });
		if (it != std::end(TTYPE_NAME))
			col = static_cast<TTemperature>(distance(std::begin(TTYPE_NAME), it));

		return col;
	}

	const char* CDevRateData::INPUT_NAME[NB_DEV_INPUT] = { "Variable","T","Tmin","Tmax", "Tid", "Ttype","Time","TimeSD","N", "RDT", "Q_TIME" };
	TDevTimeCol CDevRateData::get_input(const std::string& name)
	{
		TDevTimeCol col = I_UNKNOWN;

		auto it = find_if(std::begin(INPUT_NAME), std::end(INPUT_NAME), [&](auto &s) {return boost::iequals(s, name); });
		if (it != std::end(INPUT_NAME))
			col = static_cast<TDevTimeCol>(distance(std::begin(INPUT_NAME), it));

		return col;
	}

	size_t CDevRateData::get_pos(TDevTimeCol c)const
	{
		size_t pos = NOT_INIT;
		auto it = find(m_input_pos.begin(), m_input_pos.end(), c);
		if (it != m_input_pos.end())
			pos = std::distance(m_input_pos.begin(), it);

		return pos;
	}



	/*void CDevRateDataRow::SetT(size_t TType, std::string str)
	{
		ASSERT(TType < NB_TMP_TYPE);

		CDevRateDataRow& me = *this;

		m_TType = TType;
		if (TType == T_CONSTANT)
		{
			me[I_T] = me[I_TMIN] = me[I_TMAX] = ToDouble(str);
		}
		else if (TType == T_SINUS || TType == T_TRIANGULAR)
		{

			StringVector tmp(str, "~^");
			ASSERT(tmp.size() == 2);
			me[I_TMIN] = ToDouble(tmp[0]);
			me[I_TMAX] = ToDouble(tmp[1]);
			me[I_T] = (at(I_TMIN) + at(I_TMAX)) / 2.0;
		}
		else
		{
			ASSERT(TType == T_OBS);
			m_T_ID = str;
			me[I_T] = me[I_TMIN] = me[I_TMAX] = -999;
		}
	}
	*/

	ERMsg CDevRateData::load(const std::string& file_path)
	{
		ERMsg msg;

		//begin to read
		ifStream file;
		msg = file.open(file_path);
		if (msg)
		{
			msg = load(file);
			file.close();
		}

		return msg;
	}

	ERMsg CDevRateData::load(std::istream& io)
	{
		ERMsg msg;

		clear();
		m_input_pos.clear();
		m_bIndividual = false;
		//StringVector header;


		for (CSVIterator loop(io, ",;\t|", true, true); loop != CSVIterator() && msg; ++loop)
		{
			if (m_input_pos.empty())
			{
				for (size_t i = 0; i < loop.Header().size(); i++)
					m_input_pos.push_back(get_input(loop.Header()[i]));

				if (!have_var(I_T) && !have_var(I_T_PROFILE) &&
					(!have_var(I_TMIN) && !have_var(I_TMAX)))
				{
					msg.ajoute("Mandatory missing column. \"T\", \"Tid\" or \"Tmin\"/\"Tmax\" must be define");
				}


				//check for mandatory columns
				if (!have_var(I_TIME))
				{
					msg.ajoute("Mandatory missing column: \"Time\" must be define");
				}

				/*if ((have_var(I_TIME1) && !have_var(I_TIME2)) ||
					(!have_var(I_TIME1) && have_var(I_TIME2)))
				{
					msg.ajoute("Bad input. When \"Time1\" or \"Time2\" is used, both must be define. If only one time is needed, used \"Time\" instead");
				}*/

				/*
				a remttre + tard
				if (have_var(I_TIME) && !have_var(I_T))
				{
					msg.ajoute("Column \"T\" is missing.");
				}*/

				/*if ((have_var(I_TMIN) && !have_var(I_TMAX)) &&
					(have_var(I_TMAX) || !have_var(I_TMIN)))
				{
					msg.ajoute("Both column \"Tmin\" and \"Tmax\" must be define for non-constant temperatrue.");
				}*/

				//				if ((have_var(I_TIME1) || have_var(I_TIME2)) && have_var(I_TIME))
					//			{
						//			msg.ajoute("Column \"Time\" can't be used when \"Time1\" and \"Time2\" is used.");
							//	}
			}


			if (msg)
			{
				CDevRateDataRow row;

				string T = (*loop)[m_input_pos[I_T]];
				for (size_t i = 0; i < m_input_pos.size(); i++)
				{
					if (m_input_pos[i] != I_UNKNOWN)
					{
						if (m_input_pos[i] == I_VARIABLE)
							row.m_variable = (*loop)[i];
						else if (m_input_pos[i] == I_TTYPE)
							row.m_TType = get_TType((*loop)[i]);
						else if (m_input_pos[i] == I_T_PROFILE)
							row.m_Tprofile = (*loop)[i];
						else
							row[m_input_pos[i]] = ToDouble((*loop)[i]);
					}
				}



				bool bDead = Find((*loop).GetLastLine(), "Dead");
				if (bDead)
				{

				}
				else
				{
					//if ((have_var(I_TIME1) && have_var(I_TIME2)) && !have_var(I_TIME))
						//row[I_TIME] = row[I_TIME1] + row[I_TIME2];

					if (have_var(I_TTYPE))
					{
						if (row.m_TType == T_UNKNOWN)
							msg.ajoute("Unknow temperature type: " + (*loop)[get_pos(I_TTYPE)]);
					}
					else
					{
						if (have_var(I_T_PROFILE))
							row.m_TType = T_OBS;
						else if (have_var(I_TMIN) && have_var(I_TMAX))
							row.m_TType = T_SINUS;
						else
							row.m_TType = T_CONSTANT;
					}

					if ((have_var(I_TMIN) && have_var(I_TMAX)) && !have_var(I_T))
						row[I_T] = (row[I_TMIN] + row[I_TMAX]) / 2.0;

					if (!have_var(I_N))
						row[I_N] = 1.0;

					if (have_var(I_T) && (!have_var(I_TMIN) && !have_var(I_TMAX)))
					{
						row[I_TMIN] = row[I_T];
						row[I_TMAX] = row[I_T];
					}


					//set temperature profile ID
					if (row.m_TType != T_OBS)
						row.m_Tprofile = row.m_variable + "_" + FormatA("%.1lf-%.1lf", row[I_TMIN], row[I_TMAX]);

					push_back(row);

					for (size_t n = 0; n < row[I_N]; n++)
						m_stats[row.m_Tprofile].Add(row[I_TIME]);
				}


			}
		}


		//compute reative time and qtime
		for (auto it = m_stats.begin(); it != m_stats.end(); it++)
		{
			
			//double median = it->second[MEDIAN];
			double mean = it->second[MEAN];
			double n = it->second[NB_VALUE];

			vector<pair<double, size_t>> qTime;

			//double qTime = 0;
			for (size_t i = 0; i < size(); i++)
			{
				CDevRateDataRow& row = at(i);
				if (row.m_Tprofile == it->first)
				{
					//double rel_time = row[I_TIME] / median;
					double rel_time = row[I_TIME] / mean;
					row[I_RELATIVE_TIME] = rel_time;

					if (n > 1 && !have_var(I_TIME_STD))
					{
						row[I_TIME_STD] = it->second[STD_DEV];
						m_bIndividual = true;
					}

					qTime.push_back(make_pair(rel_time, i));
				}
			}

			sort(qTime.begin(), qTime.end());
			double cumsum = 0;
			for (auto it = qTime.begin(); it != qTime.end(); it++)
			{
				CDevRateDataRow& row = at(it->second);
				double q = max(0.001, min(0.999, (cumsum + row[I_N] / 2) / n));
				row[I_Q_TIME] = q;
				cumsum += row[I_N];
			}


		}

		return msg;
	}



	//**********************************************************************************************
	//CDevRateParameterization
	const char* CDevRateParameterization::DATA_DESCRIPTOR = "DevRateParameterizationData";
	const char* CDevRateParameterization::XML_FLAG = "DevRateParameterization";
	const char* CDevRateParameterization::MEMBERS_NAME[NB_MEMBERS_EX] = { "FitType", "DevRateEquations", "MortalityEquations","EquationsOptions", "InputFileName", "TobsFileName", "OutputFileName", "CalibrateOn", "Control", "Converge01", "CalibrateSigma", "FixeSigma" };
	const int CDevRateParameterization::CLASS_NUMBER = CExecutableFactory::RegisterClass(CDevRateParameterization::GetXMLFlag(), &CDevRateParameterization::CreateObject);

	CDevRateParameterization::CDevRateParameterization()
	{
		Reset();
	}

	CDevRateParameterization::~CDevRateParameterization()
	{}


	CDevRateParameterization::CDevRateParameterization(const CDevRateParameterization& in)
	{
		operator=(in);
	}


	void CDevRateParameterization::Reset()
	{
		CExecutable::Reset();

		m_fitType = F_DEV_RATE;
		m_name = "DevRateParameterization";
		m_eqDevRate.set();
		m_eqMortality.set();
		m_eq_options.clear();
		m_inputFileName.clear();
		m_TobsFileName.clear();
		m_outputFileName.clear();
		m_calibOn = CO_RATE;
		m_bConverge01 = false;
		m_bCalibSigma = false;


		m_ctrl.Reset();
		m_ctrl.m_bMax = true;
		m_ctrl.m_statisticType = LOG_LIKELIHOOD1;
		m_ctrl.m_MAXEVL = 1000000;
		m_ctrl.m_NS = 15;
		m_ctrl.m_NT = 20;
		m_ctrl.m_T = 10;
		m_ctrl.m_RT = 0.5;
	}


	CDevRateParameterization& CDevRateParameterization::operator =(const CDevRateParameterization& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);

			m_fitType = in.m_fitType;
			m_eqDevRate = in.m_eqDevRate;
			m_eqMortality = in.m_eqMortality;
			m_eq_options = in.m_eq_options;
			m_inputFileName = in.m_inputFileName;
			m_TobsFileName = in.m_TobsFileName;
			m_outputFileName = in.m_outputFileName;
			m_calibOn = in.m_calibOn;
			m_bConverge01 = in.m_bConverge01;
			m_bCalibSigma = in.m_bCalibSigma;
			m_bFixeSigma = in.m_bFixeSigma;

			m_ctrl = in.m_ctrl;
		}

		return *this;
	}

	bool CDevRateParameterization::operator == (const CDevRateParameterization& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator!=(in))bEqual = false;
		if (m_fitType != in.m_fitType)bEqual = false;
		if (m_eqDevRate != in.m_eqDevRate) bEqual = false;
		if (m_eqMortality != in.m_eqMortality) bEqual = false;
		if (m_eq_options != in.m_eq_options)bEqual = false;
		if (m_inputFileName != in.m_inputFileName) bEqual = false;
		if (m_TobsFileName != in.m_TobsFileName) bEqual = false;

		if (m_outputFileName != in.m_outputFileName) bEqual = false;
		if (m_calibOn != in.m_calibOn) bEqual = false;
		if (m_ctrl != in.m_ctrl)bEqual = false;
		if (m_bConverge01 != in.m_bConverge01)bEqual = false;
		if (m_bCalibSigma != in.m_bCalibSigma)bEqual = false;
		if (m_bFixeSigma != in.m_bFixeSigma)bEqual = false;
		



		return bEqual;
	}

	ERMsg CDevRateParameterization::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		//		ASSERT(m_pParent);

				//same as weather generator variables
		if (filter[LOCATION])
		{
			info.m_locations.resize(1);
			info.m_locations[0].m_name = "Mean over location";
		}
		if (filter[PARAMETER])
		{
			info.m_parameterset.clear();

			CModelInput modelInput;

			modelInput.SetName("T");
			modelInput.push_back(CModelInputParam("T", "15"));
			info.m_parameterset.push_back(modelInput);

			info.m_parameterset.m_pioneer = modelInput;
			info.m_parameterset.m_variation.SetType(CParametersVariationsDefinition::SYSTEMTIC_VARIATION);
			info.m_parameterset.m_variation.push_back(CParameterVariation("T", true, CModelInputParameterDef::kMVReal, 0, 35, 0.5));

		}
		if (filter[REPLICATION])
		{
			info.m_nbReplications = 1;
		}
		if (filter[TIME_REF])
		{
			CTM TM(CTM::ATEMPORAL, CTM::OVERALL_YEARS);
			info.m_period = CTPeriod(CTRef(YEAR_NOT_INIT, 0, 0, 0, TM), CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));
		}
		if (filter[VARIABLE])
		{
			info.m_variables.clear();

			if (m_fitType == F_DEV_RATE)
			{
				//for all equation
				for (size_t e = 0; e < m_eqDevRate.size(); e++)
				{
					if (m_eqDevRate.test(e))
					{
						TDevRateEquation  eq = CDevRateEquation::eq(e);

						std::string name = CDevRateEquation::GetEquationName(eq);
						std::string title = CDevRateEquation::GetEquationName(eq);
						std::string units = "1/day";
						std::string description = "Development rate";
						info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
					}
				}
			}
			else if (m_fitType == F_MORTALITY)
			{
				for (size_t e = 0; e < m_eqMortality.size(); e++)
				{
					if (m_eqMortality.test(e))
					{
						TMortalityEquation eq = CMortalityEquation::eq(e);

						std::string name = CMortalityEquation::GetEquationName(eq);
						std::string title = CMortalityEquation::GetEquationName(eq);
						std::string units = "%";
						std::string description = "Mortality";
						info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
					}
				}
			}

		}

		return msg;
	}

	string to_string(const CSAParameterVector& P)
	{
		std::ostringstream streamObj;

		for (size_t i = 0; i < P.size(); i++)
		{
			if (i > 0)
				streamObj << " ";

			streamObj << P[i].m_name << "=" << std::scientific << std::setprecision(6) << P[i].m_initialValue;
		}

		// Get string from output string stream
		return streamObj.str();
	}



	ERMsg CDevRateParameterization::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;


		CResult resultDB;
		msg = resultDB.Open(GetDBFilePath(GetPath(fileManager)), std::fstream::out | std::fstream::binary);
		if (msg)
		{
			callback.AddMessage(GetString(IDS_WG_PROCESS_INPUT_ANALYSIS));
			callback.AddMessage(resultDB.GetFilePath(), 1);


			CParentInfo info;
			msg = GetParentInfo(fileManager, info);
			if (msg)
			{
				CDBMetadata& metadata = resultDB.GetMetadata();
				metadata.SetLocations(info.m_locations);
				metadata.SetParameterSet(info.m_parameterset);
				metadata.SetNbReplications(info.m_nbReplications);
				metadata.SetTPeriod(info.m_period);
				metadata.SetOutputDefinition(info.m_variables);

				callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
				callback.AddMessage(resultDB.GetFilePath(), 1);



				//set vMiss value
				m_ctrl.SetVMiss(m_ctrl.AdjustFValue(DBL_MAX));


				string inputFilePath = fileManager.Input().GetFilePath(m_inputFileName);
				msg = m_data.load(inputFilePath);


				if (!m_TobsFileName.empty())
				{
					string TobsFilePath = fileManager.Input().GetFilePath(m_TobsFileName);
					msg += m_Tobs.load(TobsFilePath);

					if (msg)
						msg = m_Tobs.verify(m_data);
				}

				//generate temperature profile
				m_Tobs.generate(m_data);


				if ( (m_bCalibSigma || m_bFixeSigma) &&
					!m_data.m_bIndividual && !m_data.have_var(I_TIME_STD))
				{
					msg.ajoute("Unable to calibrate or fixe sigma when individual or standard deviation of time is unavailable");
				}

				if (!msg)
					return msg;


				CResult result;
				CFitOutputVector output;


				set<string> variables;

				for (size_t s = 0; s < m_data.size(); s++)
					variables.insert(m_data[s].m_variable);


				//for all stage
				for (auto v = variables.begin(); v != variables.end() && msg; v++)
				{
					//for all equation
					if (m_fitType == F_DEV_RATE)
					{
						for (size_t e = 0; e < m_eqDevRate.size() && msg; e++)
						{
							if (m_eqDevRate.test(e))
							{
								TDevRateEquation eq = CDevRateEquation::eq(e);
								string e_name = CDevRateEquation::GetEquationName(eq);

								CSAParameterVector params = CDevRateEquation::GetParameters(eq);
								if (m_eq_options.find(e_name) != m_eq_options.end())
								{
									ASSERT(m_eq_options[e_name].size() == CDevRateEquation::GetParameters(eq).size());
									params = m_eq_options[e_name];
								}

								if (m_bCalibSigma)// add relative development rate variance
									params.push_back(CSAParameter("sigma", 0.2, 0.01, 0.9));
								


								CFitOutput out(*v, eq, params);
								msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
								output.push_back(out);
							}
						}
					}
					else if (m_fitType == F_MORTALITY)
					{
						for (size_t e = 0; e < m_eqMortality.size() && msg; e++)
						{
							if (m_eqMortality.test(e))
							{
								TDevRateEquation eq = CDevRateEquation::eq(e);
								string e_name = CDevRateEquation::GetEquationName(eq);

								CSAParameterVector params = CDevRateEquation::GetParameters(eq);
								if (m_eq_options.find(e_name) != m_eq_options.end())
								{
									ASSERT(m_eq_options[e_name].size() == CDevRateEquation::GetParameters(eq).size());
									params = m_eq_options[e_name];
								}

								//if (m_bCalibSigma)// add relative development rate variance
									//params.push_back(CSAParameter("s", 0.2, 0.01, 0.9));


								CFitOutput out(*v, eq, params);
								//msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
								msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
								output.push_back(out);
							}
						}
					}

				}

				static const char* TYPE_NAME[NB_FIT_TYPE] = { "development rate","mortality" };
				callback.PushTask("Search optimum for " + to_string(TYPE_NAME[m_fitType]) + ": " + to_string(variables.size()) + " variables x " + to_string(m_eqDevRate.count()) + " equations: " + to_string(output.size()) + " curve to fits", output.size());



#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) 
				for (__int64 i = 0; i < (__int64)output.size(); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						msg += Optimize(output[i].m_variable, output[i].m_equation, output[i].m_parameters, output[i].m_computation, callback);
						//Optimize(output[i], callback);

#pragma omp critical(WRITE_INFO)
						{
							if (m_fitType == F_DEV_RATE)
								callback.AddMessage(output[i].m_variable + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(output[i].m_equation)));
							else if (m_fitType == F_MORTALITY)
								callback.AddMessage(output[i].m_variable + ": " + CMortalityEquation::GetEquationName(CMortalityEquation::eq(output[i].m_equation)));

							WriteInfo(output[i].m_parameters, output[i].m_computation, callback);
						}


						//fileManager, result, callback
						msg += callback.StepIt();
#pragma omp flush(msg)
					}
				}

				callback.PopTask();



				callback.AddMessage(GetCurrentTimeString());
				std::string logText = GetOutputString(msg, callback, true);

				std::string filePath = GetLogFilePath(GetPath(fileManager));
				msg += WriteOutputMessage(filePath, logText);

				if (true)
				{
					string outputFilePath = fileManager.GetOutputPath() + (!m_outputFileName.empty() ? m_outputFileName : GetFileTitle(m_inputFileName));
					SetFileExtension(outputFilePath, ".csv");

					//begin to read
					ofStream file;


					ERMsg msg_file = file.open(outputFilePath);
					if (msg_file)//save result event if user cancel or error
					{

						sort(output.begin(), output.end(), [](const CFitOutput& a, const CFitOutput& b) {return a.m_computation.m_Fopt > b.m_computation.m_Fopt; });
						file << "Variable,EqName,P,Eq,RMSE,CD,R2,AICc,MLH" << endl;
						for (auto v = variables.begin(); v != variables.end(); v++)
						{
							for (size_t i = 0; i < output.size(); i++)
							{
								if (output[i].m_variable == *v)
								{
									string name;
									string R_eq;
									string P;
									if (m_fitType == F_DEV_RATE)
									{
										TDevRateEquation eq = CDevRateEquation::eq(output[i].m_equation);
										name = CDevRateEquation::GetEquationName(eq);
										R_eq = CDevRateEquation::GetEquationR(eq);
										P = to_string(CDevRateEquation::GetParameters(eq, output[i].m_computation.m_Xopt));

										if (m_bCalibSigma)
										{
											P += " sigma=" + to_string(output[i].m_computation.m_Xopt.back());
										}
										else if (m_bFixeSigma)
										{
											CStatistic sigma;
											for (__int64 j = 0; j < (__int64)m_data.size(); j++)
											{
												if (WBSF::IsEqualNoCase(m_data[j].m_variable, *v))
												{
													double mean = m_data[j][I_TIME];
													double sd = m_data[j][I_TIME_STD];
													for(size_t k=0; k< m_data[j][I_N]; k++)
														sigma += sqrt(log(1 + Square(sd) / Square(mean)));
												}
											}

											P += " sigma=" + to_string(sigma[MEAN]);
										}


									}
									else if (m_fitType == F_MORTALITY)
									{
										TMortalityEquation eq = CMortalityEquation::eq(output[i].m_equation);
										name = CMortalityEquation::GetEquationName(eq);
										R_eq = CMortalityEquation::GetEquationR(eq);
										P = to_string(CMortalityEquation::GetParameters(eq, output[i].m_computation.m_Xopt));
									}

									file << output[i].m_variable << "," << name << "," << P << ",\"" << R_eq << "\",";
									if (output[i].m_computation.m_Fopt != m_ctrl.GetVMiss())
									{
										file << output[i].m_computation.m_Sopt[RMSE] << "," << output[i].m_computation.m_Sopt[COEF_D] << ",";
										file << output[i].m_computation.m_Sopt[STAT_R²] << "," << output[i].m_computation.m_AICCopt << "," << output[i].m_computation.m_MLHopt;
									}
									else
									{
										file << "0,0,0,0";
									}

									file << endl;
								}
							}
						}

						file.close();
					}

					msg += msg_file;
				}


			}

			resultDB.Close();
		}

		return msg;
	}



	//Initialize input parameter
	//user can override theses methods
	ERMsg CDevRateParameterization::InitialiseComputationVariable(std::string s, size_t e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;

		computation.m_bounds.resize(parameters.size());
		computation.m_C.resize(parameters.size());
		computation.m_X.resize(parameters.size());
		computation.m_XP.resize(parameters.size());
		computation.m_XPstat.resize(parameters.size());
		computation.m_VM.resize(parameters.size());
		computation.m_VMstat.resize(parameters.size());


		for (size_t i = 0; i < parameters.size(); i++)
		{
			computation.m_bounds[i] = parameters[i].m_bounds;
			computation.m_XP[i] = parameters[i].m_initialValue;
			computation.m_C[i] = 2;
			computation.m_VM[i] = computation.m_bounds[i].GetExtent();
			ASSERT(!computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]));
			//If the initial value is out of bounds, notify the user and return
			//to the calling routine.
			if (computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]))
			{
				if (m_fitType == F_DEV_RATE)
					msg.ajoute(s + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::eq(e)));
				else if (m_fitType == F_MORTALITY)
					msg.ajoute(s + ": " + CMortalityEquation::GetEquationName(CMortalityEquation::eq(e)));



				msg.ajoute("The starting value (" + ToString(computation.m_XP[i]) + ") is not inside the bounds [" + ToString(computation.m_bounds[i].GetLowerBound()) + "," + ToString(computation.m_bounds[i].GetUpperBound()) + "].");
				return msg;
			}
		}

		computation.Initialize(m_ctrl.T(), m_ctrl.NEPS(), m_ctrl.GetVMiss());
		
		//  Evaluate the function with input X and return value as F.
		GetFValue(s, e, computation);

		computation.m_NFCNEV++;

		computation.m_X = computation.m_XP;
		computation.m_Xopt = computation.m_XP;

		if (computation.m_FP != m_ctrl.GetVMiss())
		{
			computation.m_S = computation.m_SP;
			computation.m_F = computation.m_FP;
			computation.m_AICC = computation.m_AICCP;
			computation.m_MLH = computation.m_MLHP;

			computation.m_Sopt = computation.m_SP;
			computation.m_Fopt = computation.m_FP;
			computation.m_AICCopt = computation.m_AICCP;
			computation.m_MLHopt = computation.m_MLHP;

			computation.m_FSTAR[0] = computation.m_FP;
		}

		return msg;
	}


	double CDevRateParameterization::Exprep(const double& RDUM)
	{
		//  This function replaces exp to avoid under- and overflows and is
		//  designed for IBM 370 type machines. It may be necessary to modify
		//  it for other machines. Note that the maximum and minimum values of
		//  EXPREP are such that they has no effect on the algorithm.

		double EXPREP = 0;

		if (RDUM > 174.)
		{
			EXPREP = 3.69E+75;
		}
		else if (RDUM < -180.)
		{
			EXPREP = 0.0;
		}
		else
		{
			EXPREP = exp(RDUM);
		}

		return EXPREP;
	}



	void CDevRateParameterization::WriteInfo(const CSAParameterVector& parameters, const CComputationVariable& computation, CCallback& callback)
	{
		string line;

		if (computation.m_Sopt[NB_VALUE] > 0)
		{
			double F = m_ctrl.Max() ? computation.m_Fopt : -computation.m_Fopt;

			line = FormatA("N=%10d\tT=%9.5f\tF=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf\tAICc=%8.5lf\tMLH=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²], computation.m_AICCopt, computation.m_MLHopt);
			callback.AddMessage(line);
		}
		else
		{
			callback.AddMessage("No optimum find yet...");
		}


		line.clear();
		if (computation.m_Xopt.size() == parameters.size())
		{

			bool bShowRange = !computation.m_XPstat.empty() && computation.m_XPstat[0][NB_VALUE] > 0;
			for (size_t i = 0, j = 0; i < parameters.size(); i++)
			{
				string name = parameters[i].m_name; Trim(name);
				string tmp;

				if (bShowRange)
				{
					tmp = FormatA("% -20.20s\t=%10.5lf {%10.5lf,%10.5lf}\tVM={%10.5lf,%10.5lf}\n", name.c_str(), computation.m_Xopt[j], computation.m_XPstat[j][LOWEST], computation.m_XPstat[j][HIGHEST], computation.m_VMstat[j][LOWEST], computation.m_VMstat[j][HIGHEST]);
				}
				else
				{
					tmp = FormatA("%s = %5.3lg  ", name.c_str(), computation.m_Xopt[j]);
				}

				line += tmp;
				j++;
			}

			callback.AddMessage(line);
		}
	}

	std::string CDevRateParameterization::GetPath(const CFileManager& fileManager)const
	{
		if (m_pParent == NULL)
			return fileManager.GetTmpPath() + m_internalName + "\\";

		return m_pParent->GetPath(fileManager) + m_internalName + "\\";
	}


	//**********************************************************************
	//CRandomizeNumber

	//  Version: 3.2
	//  Date: 1/22/94.
	//  Differences compared to Version 2.0:
	//     1. If a trial is out of bounds, a point is randomly selected
	//        from LB(i) to UB(i). Unlike in version 2.0, this trial is
	//        evaluated and is counted in acceptances and rejections.
	//        All corresponding documentation was changed as well.
	//  Differences compared to Version 3.0:
	//     1. If VM(i) > (UB(i) - LB(i)), VM is set to UB(i) - LB(i).
	//        The idea is that if T is high relative to LB & UB, most
	//        points will be accepted, causing VM to rise. But, in this
	//        situation, VM has little meaning; particularly if VM is
	//        larger than the acceptable region. Setting VM to this size
	//        still allows all parts of the allowable region to be selected.
	//  Differences compared to Version 3.1:
	//     1. Test made to see if the initial temperature is positive.
	//     2. WRITE statements prettied up.
	//     3. References to paper updated.
	//
	//  Synopsis:
	//  This routine implements the continuous simulated annealing global
	//  optimization algorithm described in Corana et al.'s article
	//  "Minimizing Multimodal Functions of Continuous Variables with the
	//  "Simulated Annealing" Algorithm" in the September 1987 (vol. 13,
	//  no. 3, pp. 262-280) issue of the ACM Transactions on Mathematical
	//  Software.
	//
	//  A very quick (perhaps too quick) overview of SA:
	//     SA tries to find the global optimum of an N dimensional function.
	//  It moves both up and downhill and as the optimization process
	//  proceeds, it focuses on the most promising area.
	//     To start, it randomly chooses a trial point within the step length
	//  VM (a vector of length N) of the user selected starting point. The
	//  function is evaluated at this trial point and its value is compared
	//  to its value at the initial point.
	//     In a maximization problem, all uphill moves are accepted and the
	//  algorithm continues from that trial point. Downhill moves may be
	//  accepted; the decision is made by the Metropolis criteria. It uses T
	//  (temperature) and the size of the downhill move in a probabilistic
	//  manner. The smaller T and the size of the downhill move are, the more
	//  likely that move will be accepted. If the trial is accepted, the
	//  algorithm moves on from that point. If it is rejected, another point
	//  is chosen instead for a trial evaluation.
	//     Each element of VM periodically adjusted so that half of all
	//  function evaluations in that direction are accepted.
	//     A fall in T is imposed upon the system with the RT variable by
	//  T(i+1) = RT*T(i) where i is the ith iteration. Thus, as T declines,
	//  downhill moves are less likely to be accepted and the percentage of
	//  rejections rise. Given the scheme for the selection for VM, VM falls.
	//  Thus, as T declines, VM falls and SA focuses upon the most promising
	//  area for optimization.
	//
	//  The importance of the parameter T:
	//     The parameter T is crucial in using SA successfully. It influences
	//  VM, the step length over which the algorithm searches for optima. For
	//  a small intial T, the step length may be too small; thus not enough
	//  of the function might be evaluated to find the global optima. The user
	//  should carefully examine VM in the intermediate output (set IPRINT =
	//  1) to make sure that VM is appropriate. The relationship between the
	//  initial temperature and the resulting step length is function
	//  dependent.
	//     To determine the starting temperature that is consistent with
	//  optimizing a function, it is worthwhile to run a trial run first. Set
	//  RT = 1.5 and T = 1.0. With RT > 1.0, the temperature increases and VM
	//  rises as well. Then select the T that produces a large enough VM.
	//
	//  For modifications to the algorithm and many details on its use,
	//  (particularly for econometric applications) see Goffe, Ferrier
	//  and Rogers, "Global Optimization of Statistical Functions with
	//  Simulated Annealing," Journal of Econometrics, vol. 60, no. 1/2, 
	//  Jan./Feb. 1994, pp. 65-100.
	//  For more information, contact 
	//              Bill Goffe
	//              Department of Economics and International Business
	//              University of Southern Mississippi 
	//              Hattiesburg, MS  39506-5072 
	//              (601) 266-4484 (office)
	//              (601) 266-4920 (fax)
	//              bgoffe@whale.st.usm.edu (Internet)
	//
	//  As far as possible, the parameters here have the same name as in
	//  the description of the algorithm on pp. 266-8 of Corana et al.
	//
	//  In this description, SP is single precision, DP is double precision,
	//  INT is integer, L is logical and (N) denotes an array of length n.
	//  Thus, DP(N) denotes a double precision array of length n.
	//
	//  Input Parameters:
	//    Note: The suggested values generally come from Corana et al. To
	//          drastically reduce runtime, see Goffe et al., pp. 90-1 for
	//          suggestions on choosing the appropriate RT and NT.
	//    N - Number of variables in the function to be optimized. (INT)
	//    X - The starting values for the variables of the function to be
	//        optimized. (DP(N))
	//    MAX - Denotes whether the function should be maximized or
	//          minimized. A true value denotes maximization while a false
	//          value denotes minimization. Intermediate output (see IPRINT)
	//          takes this into account. (L)
	//    RT - The temperature reduction factor. The value suggested by
	//         Corana et al. is .85. See Goffe et al. for more advice. (DP)
	//    EPS - Error tolerance for termination. If the final function
	//          values from the last neps temperatures differ from the
	//          corresponding value at the current temperature by less than
	//          EPS and the final function value at the current temperature
	//          differs from the current optimal function value by less than
	//          EPS, execution terminates and IER = 0 is returned. (EP)
	//    NS - Number of cycles. After NS*N function evaluations, each
	//         element of VM is adjusted so that approximately half of
	//         all function evaluations are accepted. The suggested value
	//         is 20. (INT)
	//    NT - Number of iterations before temperature reduction. After
	//         NT*NS*N function evaluations, temperature (T) is changed
	//         by the factor RT. Value suggested by Corana et al. is
	//         MAX(100, 5*N). See Goffe et al. for further advice. (INT)
	//    NEPS - Number of final function values used to decide upon termi-
	//           nation. See EPS. Suggested value is 4. (INT)
	//    MAXEVL - The maximum number of function evaluations. If it is
	//             exceeded, IER = 1. (INT)
	//    LB - The lower bound for the allowable solution variables. (DP(N))
	//    UB - The upper bound for the allowable solution variables. (DP(N))
	//         If the algorithm chooses X(I) .LT. LB(I) or X(I) .GT. UB(I),
	//         I = 1, N, a point is from inside is randomly selected. This
	//         This focuses the algorithm on the region inside UB and LB.
	//         Unless the user wishes to concentrate the search to a par-
	//         ticular region, UB and LB should be set to very large positive
	//         and negative values, respectively. Note that the starting
	//         vector X should be inside this region. Also note that LB and
	//         UB are fixed in position, while VM is centered on the last
	//         accepted trial set of variables that optimizes the function.
	//    C - Vector that controls the step length adjustment. The suggested
	//        value for all elements is 2.0. (DP(N))
	//    IPRINT - controls printing inside SA. (INT)
	//             Values: 0 - Nothing printed.
	//                     1 - Function value for the starting value and
	//                         summary results before each temperature
	//                         reduction. This includes the optimal
	//                         function value found so far, the total
	//                         number of moves (broken up into uphill,
	//                         downhill, accepted and rejected), the
	//                         number of out of bounds trials, the
	//                         number of new optima found at this
	//                         temperature, the current optimal X and
	//                         the step length VM. Note that there are
	//                         N*NS*NT function evalutations before each
	//                         temperature reduction. Finally, notice is
	//                         is also given upon achieveing the termination
	//                         criteria.
	//                     2 - Each new step length (VM), the current optimal
	//                         X (XOPT) and the current trial X (X). This
	//                         gives the user some idea about how far X
	//                         strays from XOPT as well as how VM is adapting
	//                         to the function.
	//                     3 - Each function evaluation, its acceptance or
	//                         rejection and new optima. For many problems,
	//                         this option will likely require a small tree
	//                         if hard copy is used. This option is best
	//                         used to learn about the algorithm. A small
	//                         value for MAXEVL is thus recommended when
	//                         using IPRINT = 3.
	//             Suggested value: 1
	//             Note: For a given value of IPRINT, the lower valued
	//                   options (other than 0) are utilized.
	//    ISEED1 - The first seed for the random number generator RANMAR.
	//             0 <= ISEED1 <= 31328. (INT)
	//    ISEED2 - The second seed for the random number generator RANMAR.
	//             0 <= ISEED2 <= 30081. Different values for ISEED1
	//             and ISEED2 will lead to an entirely different sequence
	//             of trial points and decisions on downhill moves (when
	//             maximizing). See Goffe et al. on how this can be used
	//             to test the results of SA. (INT)
	//
	//  Input/Output Parameters:
	//    T - On input, the initial temperature. See Goffe et al. for advice.
	//        On output, the final temperature. (DP)
	//    VM - The step length vector. On input it should encompass the
	//         region of interest given the starting value X. For point
	//         X(I), the next trial point is selected is from X(I) - VM(I)
	//         to  X(I) + VM(I). Since VM is adjusted so that about half
	//         of all points are accepted, the input value is not very
	//         important (i.e. is the value is off, SA adjusts VM to the
	//         correct value). (DP(N))
	//
	//  Output Parameters:
	//    XOPT - The variables that optimize the function. (DP(N))
	//    FOPT - The optimal value of the function. (DP)
	//    NACC - The number of accepted function evaluations. (INT)
	//    NFCNEV - The total number of function evaluations. In a minor
	//             point, note that the first evaluation is not used in the
	//             core of the algorithm; it simply initializes the
	//             algorithm. (INT).
	//    NOBDS - The total number of trial function evaluations that
	//            would have been out of bounds of LB and UB. Note that
	//            a trial point is randomly selected between LB and UB.
	//            (INT)
	//    IER - The error return number. (INT)
	//          Values: 0 - Normal return; termination criteria achieved.
	//                  1 - Number of function evaluations (NFCNEV) is
	//                      greater than the maximum number (MAXEVL).
	//                  2 - The starting value (X) is not inside the
	//                      bounds (LB and UB).
	//                  3 - The initial temperature is not positive.
	//                  99 - Should not be seen; only used internally.
	//
	//  Work arrays that must be dimensioned in the calling routine:
	//       RWK1 (DP(NEPS))  (FSTAR in SA)
	//       RWK2 (DP(N))     (XP    "  " )
	//       IWK  (INT(N))    (NACP  "  " )
	//
	//  Required Functions (included):
	//    EXPREP - Replaces the function EXP to avoid under- and overflows.
	//             It may have to be modified for non IBM-type main-
	//             frames. (DP)
	//    RMARIN - Initializes the random number generator RANMAR.
	//    RANMAR - The actual random number generator. Note that
	//             RMARIN must run first (SA does this). It produces uniform
	//             random numbers on [0,1]. These routines are from
	//             Usenet's comp.lang.fortran. For a reference, see
	//             "Toward a Universal Random Number Generator"
	//             by George Marsaglia and Arif Zaman, Florida State
	//             University Report: FSU-SCRI-87-50 (1987).
	//             It was later modified by F. James and published in
	//             "A Review of Pseudo-random Number Generators." For
	//             further information, contact stuart@ads.com. These
	//             routines are designed to be portable on any machine
	//             with a 24-bit or more mantissa. I have found it produces
	//             identical results on a IBM 3081 and a Cray Y-MP.
	//
	//  Required Subroutines (included):
	//    PRTVEC - Prints vectors.
	//    PRT1 ... PRT10 - Prints intermediate output.
	//    FCN - Function to be optimized. The form is
	//            SUBROUTINE FCN(N,X,F)
	//            INTEGER N
	//            DOUBLE PRECISION  X(N), F
	//            ...
	//            function code with F = F(X)
	//            ...
	//            RETURN
	//            END
	//          Note: This is the same form used in the multivariable
	//          minimization algorithms in the IMSL edition 10 library.
	//
	//  Machine Specific Features:
	//    1. EXPREP may have to be modified if used on non-IBM type main-
	//       frames. Watch for under- and overflows in EXPREP.
	//    2. Some FORMAT statements use G25.18; this may be excessive for
	//       some machines.
	//    3. RMARIN and RANMAR are designed to be protable; they should not
	//       cause any problems.
	ERMsg CDevRateParameterization::Optimize(string s, size_t  e, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	//ERMsg CDevRateParameterization::Optimize(CFitOutput& output, CCallback& callback)
	{
		ERMsg msg;

		/*string s = output.m_variable;
		size_t  e = output.m_equation;
		CSAParameterVector& parameters = output.m_parameters;
		CComputationVariable& computation = output.m_computation;
*/
		//  Initialize the random number generator RANMAR.
		CRandomizeNumber random(m_ctrl.Seed1(), m_ctrl.Seed2());

		bool bQuit = false;

		//  Start the main loop. Note that it terminates if :
		//(i) the algorithm successfully optimizes the function 
		//(ii) there are too many function evaluations (more than MAXEVL).
		int L = 0;
		do
		{
			L++;

			long NUP = 0;
			long NREJ = 0;
			long NNEW = 0;
			long NDOWN = 0;
			long LNOBDS = 0;

			for (int M = 0; M < m_ctrl.NT() && msg; M++)
			{
				vector<int> NACP;
				NACP.insert(NACP.begin(), computation.m_X.size(), 0);

				for (size_t j = 0; j < m_ctrl.NS() && msg; j++)
				{

					for (size_t h = 0; h < NACP.size() && msg; h++)
					{
						//  If too many function evaluations occur, terminate the algorithm.
						if (computation.m_NFCNEV >= m_ctrl.MAXEVL())
						{
							callback.AddMessage("Number of function evaluations (NFCNEV) is greater than the maximum number (MAXEVL).");
							//msg.ajoute();
							return msg;
						}

						computation.m_XP.resize(computation.m_X.size());
						computation.m_SP.Reset();
						computation.m_FP = m_ctrl.GetVMiss();

						//  Generate XP, the trial value of X. Note use of VM to choose XP.
						for (size_t i = 0; i < computation.m_X.size(); i++)
						{
							if (i == h)
								computation.m_XP[i] = computation.m_X[i] + (random.Ranmar()*2.0 - 1.0) * computation.m_VM[i];
							else
								computation.m_XP[i] = computation.m_X[i];


							//  If XP is out of bounds, select a point in bounds for the trial.
							if (computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]))
							{
								computation.m_XP[i] = computation.m_bounds[i].GetLowerBound() + computation.m_bounds[i].GetExtent()*random.Ranmar();

								LNOBDS++;
								computation.m_NOBDS++;
							}
						}

						//  Evaluate the function with the trial point XP and return as FP.
						GetFValue(s, e, computation);

						//add X value to extreme XP statistic
						for (size_t i = 0; i < computation.m_XP.size() && i < computation.m_XPstat.size(); i++)
							computation.m_XPstat[i] += computation.m_XP[i];

						for (size_t i = 0; i < computation.m_VMstat.size(); i++)
							computation.m_VMstat[i] += computation.m_VM[i];



						//  Accept the new point if the function value increases.
						if (computation.m_FP != m_ctrl.GetVMiss())
						{
							if (computation.m_FP >= computation.m_F)
							{
								computation.m_X = computation.m_XP;
								computation.m_F = computation.m_FP;
								computation.m_AICC = computation.m_AICCP;
								computation.m_MLH = computation.m_MLHP;
								computation.m_S = computation.m_SP;
								computation.m_NACC++;
								NACP[h]++;
								NUP++;

								//  If greater than any other point, record as new optimum.
								if (computation.m_FP > computation.m_Fopt)
								{
									computation.m_Xopt = computation.m_XP;
									computation.m_Fopt = computation.m_FP;
									computation.m_AICCopt = computation.m_AICCP;
									computation.m_MLHopt = computation.m_MLHP;
									computation.m_Sopt = computation.m_SP;
									NNEW++;
								}
							}
							//  If the point is lower, use the Metropolis criteria to decide on
							//  acceptance or rejection.
							else
							{
								double P = Exprep((computation.m_FP - computation.m_F) / computation.m_T);
								double PP = random.Ranmar();
								if (PP < P)
								{
									computation.m_X = computation.m_XP;
									computation.m_F = computation.m_FP;
									computation.m_AICC = computation.m_AICCP;
									computation.m_MLH = computation.m_MLHP;
									computation.m_S = computation.m_SP;
									computation.m_NACC++;
									NACP[h]++;
									NDOWN++;
								}
								else
								{
									NREJ = NREJ + 1;
								}
							} //if
						}
						else
						{
							NREJ = NREJ + 1;
							//Eliminate this evaluation??
							h--;
						}

						computation.m_NFCNEV++;


						//if (msg)
						msg += callback.StepIt(0);
					} //H
				} //J



				//  Adjust VM so that approximately half of all evaluations are accepted.
				ASSERT(computation.m_VM.size() == NACP.size());
				for (int I = 0; I < computation.m_VM.size(); I++)
				{
					double RATIO = double(NACP[I]) / double(m_ctrl.NS());
					if (RATIO > 0.6)
					{
						computation.m_VM[I] = computation.m_VM[I] * (1. + computation.m_C[I] * (RATIO - .6) / .4);
					}
					else if (RATIO < 0.4)
					{
						computation.m_VM[I] = computation.m_VM[I] / (1. + computation.m_C[I] * ((.4 - RATIO) / .4));
					}


					if (computation.m_VM[I] > computation.m_bounds[I].GetExtent())
					{
						computation.m_VM[I] = computation.m_bounds[I].GetExtent();
					}
				}//all VM
			}//M

			for (size_t i = 0; i < computation.m_XPstat.size(); i++)
				computation.m_XPstat[i].Reset();

			//clean VM stats
			for (size_t i = 0; i < computation.m_VMstat.size(); i++)
				computation.m_VMstat[i].Reset();

			//WriteInfo(parameters, computation, callback);

			//  Loop again.
			bQuit = fabs(computation.m_F - computation.m_Fopt) <= m_ctrl.EPS();
			for (int I = 0; I < computation.m_FSTAR.size() && bQuit; I++)
			{
				if (fabs(computation.m_F - computation.m_FSTAR[I]) > m_ctrl.EPS())
					bQuit = false;
			}

			//  If termination criteria is not met, prepare for another loop.
			computation.PrepareForAnotherLoop(m_ctrl.RT());

		} while (!bQuit&&msg);


		return msg;
	}


	bool CDevRateParameterization::GetFValue(string var, size_t e, CComputationVariable& computation)
	{
		bool bValid = false;

			
				


		if (m_fitType == F_DEV_RATE)
		{

			TDevRateEquation eq = CDevRateEquation::eq(e);
			if (CDevRateEquation::IsParamValid(eq, computation.m_XP))
			{
				bool bMaxLogLikelyhoude = true;
				double sigma = 0.01;
				if (m_bCalibSigma)
				{
					sigma = computation.m_XP.back();
					bMaxLogLikelyhoude = true;
				}
				else if (m_bFixeSigma)
				{
					CStatistic stat;
					for (__int64 i = 0; i < (__int64)m_data.size(); i++)
					{
						if (WBSF::IsEqualNoCase(m_data[i].m_variable, var))
						{
							double mean = m_data[i][I_TIME];
							double sd = m_data[i][I_TIME_STD];
							for (size_t k = 0; k < m_data[i][I_N]; k++)
								stat += sqrt(log(1 + Square(sd) / Square(mean)));
						}
					}
						
					//use mean of sigma
					sigma = stat[MEAN];
					bMaxLogLikelyhoude = true;
				}
				else
				{
					bMaxLogLikelyhoude = false;
				}



				CStatisticXYEx stat;

				double max_likelyhoude = 0;
				for (__int64 i = 0; i < (__int64)m_data.size(); i++)
				{
					if (WBSF::IsEqualNoCase(m_data[i].m_variable, var))
					{
						//double rel_time = m_data[i][I_RELATIVE_TIME];

						//double rr = GetRelDevRate(eq, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);
						//double rt = 1 / rr;
						//double mean_rate = GetMeanRate(eq, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);

						//double sigma = computation.m_XP.back();
						//if (sd > 0)
						//{
						//	//double mu = log(Square(obs) / sqrt(Square(obs) + Square(sd)));
						//	double s = sqrt(log(1 + Square(sd) / Square(obs)));
						//	sigma = s * sigma;
						//}

						

						if (bMaxLogLikelyhoude)
						{
							double nII = Regniere2020(eq, sigma, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);
							max_likelyhoude += nII * m_data[i][I_N];

							CStatisticEx time_stat = GetTimeStat(eq, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);
							double mean_time =time_stat[MEAN];
							double obs = m_data[i][I_TIME];

							boost::math::lognormal_distribution<double> LogNormal(log(1) - Square(sigma) / 2.0, sigma);
							double rt = quantile(LogNormal, m_data[i][I_Q_TIME]);
							double sim = ceil(mean_time * rt);//the observed is alwayse the day after the change

							for (size_t n = 0; n < m_data[i][I_N]; n++)
								stat.Add(sim, obs);

						}
						else
						{
							CStatisticEx rate_stat = GetRateStat(eq, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);
							
							double obs = m_data[i][I_TIME];
							double sim_rate = 1.0 / max( 0.001, rate_stat[MEAN]);
							
								
							//compute from rate
							//for (size_t n = 0; n < m_data[i][I_N]; n++)
								//stat.Add(1.0 / sim, 1.0 / obs);

							
							//compute from time
							CStatisticEx time_stat = GetTimeStat(eq, computation, m_Tobs[m_data[i].m_Tprofile], m_data[i][I_TIME]);
							//double obs = m_data[i][I_TIME];
							double sim_time = min(1000.0, time_stat[MEAN]);

							for (size_t n = 0; n < m_data[i][I_N]; n++)
							{
								stat.Add(sim_rate, obs);
								stat.Add(sim_time, obs);
							}


						}



					}//if valid
				}//for


				//  If the function is to be minimized, switch the sign of the function.
				//  Note that all intermediate and final output switches the sign back
				//  to eliminate any possible confusion for the user.
				if (stat[NB_VALUE] > 1)
				{
					if (bMaxLogLikelyhoude)
					{
						//used sigma ML hat instead of classical sigma hat
						double k = computation.m_XP.size();
						double n = stat[NB_VALUE];

						double AIC = 2 * k - 2 * (max_likelyhoude);
						double AICc = AIC + (2 * k*(k + 1) / (n - k - 1));// n/k < 40


						computation.m_AICCP = AICc;
						computation.m_MLHP = max_likelyhoude;



						//if (m_ctrl.m_statisticType == LOG_LIKELIHOOD1)

						computation.m_FP = max_likelyhoude;
						//else if (m_ctrl.m_statisticType == LOG_LIKELIHOOD2)//use AICC instead of LOG_LIKELIHOOD2
							//computation.m_FP = -AICc;
					}
					else
					{
						//used sigma ML hat instead of classical sigma hat
						double k = computation.m_XP.size();
						double n = stat[NB_VALUE];
						double sigma = (sqrt(stat[RSS] / (n - 1))*n) / (n - 1);
						double sigmaML = sigma * sqrt((n - k) / n);

						double LL = 0;
						for (size_t i = 0; i < stat[NB_VALUE]; i++)
						{
							double m = stat.x(i);
							boost::math::normal_distribution<> N(m, sigmaML);

							double x = stat.y(i);
							double p = boost::math::pdf(N, x);

							ASSERT(p > 0);
							LL += log(p);
						}

						double AIC = 2 * k - 2 * LL;
						// n/k < 40
						double AICc = AIC + (2 * k*(k + 1) / (n - k - 1));
						computation.m_AICCP = AICc;
						computation.m_MLHP = LL;


						computation.m_FP = -stat[RSS];
					}
						
					//computation.m_FP = m_ctrl.GetFinalFValue(stat);

					computation.m_SP = stat;
					bValid = true;
				}
			}
		}
		else if (m_fitType == F_MORTALITY)
		{
			TMortalityEquation eq = CMortalityEquation::eq(e);
			if (CMortalityEquation::IsParamValid(eq, computation.m_XP))
			{
				double sigma = 0.01;
				if (m_bCalibSigma)
					sigma = computation.m_XP.back();


				//std::mt19937 gen;
				//boost::math::lognormal_distribution<double> uniformLogNormal(-WBSF::Square(s_2) / 2.0, s_2);


				CStatisticXYEx stat;

				double max_likelyhoude = 0;
				for (__int64 i = 0; i < (__int64)m_data.size(); i++)
				{
					if (WBSF::IsEqualNoCase(m_data[i].m_variable, var))
					{

						//double sim = CMortalityEquation::GetRate(e, computation.m_XP, m_data[i][I_T]);
						//double obs = 1.0 / m_data[i][I_TIME];
						//if (m_bConverge01)
						//{
						//	if (sim < 0)
						//		sim = -exp(1000 / sim);//let the code to give a chance to converge to the good direction
						//	else if (sim > 1)
						//		sim = 1 + exp(-1000 / (sim - 1));

						//	//let the code to give a chance to converge to the good direction
						//}


						//double mT1 = max(0.0, min(1.0, CMortalityEquation::GetMortality(eq, computation.m_XP, m_data[i][I_T1])));
						//double mT2 = max(0.0, min(1.0, CMortalityEquation::GetMortality(eq, computation.m_XP, m_data[i][I_T2])));


						//double nII = Regniere2012(s_2, m_data[i][I_TIME1], m_data[i][I_TIME2], mT1, mT2);
						//double nII = Regniere2020(eq, computation, m_data[i].m_TType, m_data[i][I_TMIN], m_data[i][I_TMAX], m_data[i][I_TIME]);
						//max_likelyhoude += nII * m_data[i][I_N];


						//double rel_time = 1;
						//if (m_bCalibSigma)
						//{
						//	rel_time = m_data[i][I_RELATIVE_TIME];
						//}

						double obs = 0;// (m_data[i][I_TIME1] + m_data[i][I_TIME2]);
						double sim = 0;// (mT1*m_data[i][I_TIME1] + mT2 * m_data[i][I_TIME2]) / (m_data[i][I_TIME1] + m_data[i][I_TIME2])*rel_time;

						//if (m_calibOn == CO_RATE)
						//{
						//	obs = 1.0 / obs;
						//	sim = 1.0 / sim;


						//	if (m_bConverge01)
						//	{
						//		if (sim < 0)
						//			sim = -exp(1000 / sim);//let the code to give a chance to converge to the good direction
						//		else if (sim > 1)
						//			sim = 1 + exp(-1000 / (sim - 1));

						//		//let the code to give a chance to converge to the good direction
						//	}
						//}




						if (!isfinite(sim) || isnan(sim) || sim<-1E8 || sim>1E8)
						{
							stat.Reset();//find other set of parameters
							return false;
						}


						for (size_t n = 0; n < m_data[i][I_N]; n++)
							stat.Add(sim, obs);
					}
				}

				//stat.Reset();
				//stat.Add(max_likelyhoude, 0);

				//  If the function is to be minimized, switch the sign of the function.
				//  Note that all intermediate and final output switches the sign back
				//  to eliminate any possible confusion for the user.
				if (stat[NB_VALUE] > 1)
				{
					

					//computation.m_AICP = -2 * LL + 2 * (computation.m_XP.size() + 1);
					//computation.m_FP = m_ctrl.GetFinalFValue(stat);
					//computation.m_SP = stat;


					//used sigma ML hat instead of classical sigma hat
					double k = computation.m_XP.size();
					double n = stat[NB_VALUE];

					
					double AIC = 2 * k - 2 * (max_likelyhoude);
					double AICc = AIC + (2 * k*(k + 1) / (n - k - 1));// n/k < 40

					computation.m_AICCP = AICc;
					computation.m_MLHP = max_likelyhoude;



					if (m_ctrl.m_statisticType == LOG_LIKELIHOOD1)
						computation.m_FP = max_likelyhoude;
					else if (m_ctrl.m_statisticType == LOG_LIKELIHOOD2)//use AICC instead of LOG_LIKELIHOOD2
						computation.m_FP = -AICc;
					else
						computation.m_FP = m_ctrl.GetFinalFValue(stat);

					computation.m_SP = stat;
					bValid = true;
					////}
					//else
					//{
					//	stat.Reset();//find other set of parameters
					//	bValid = false;
					//}
				}




			}

		}


		return bValid;
	}

	void CDevRateParameterization::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);

		out[GetMemberName(FIT_TYPE)](m_fitType);
		out[GetMemberName(DEV_RATE_EQUATIONS)](m_eqDevRate);
		out[GetMemberName(MORTALITY_EQUATIONS)](m_eqMortality);
		out[GetMemberName(EQ_OPTIONS)](m_eq_options);
		out[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		out[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		out[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		out[GetMemberName(CALIB_ON)](m_calibOn);
		out[GetMemberName(CONVERGE_01)](m_bConverge01);
		out[GetMemberName(CALIB_SIGMA)](m_bCalibSigma);
		out[GetMemberName(FIXE_SIGMA)](m_bFixeSigma);
		out[GetMemberName(CONTROL)](m_ctrl);



	}

	bool CDevRateParameterization::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(FIT_TYPE)](m_fitType);
		in[GetMemberName(DEV_RATE_EQUATIONS)](m_eqDevRate);
		in[GetMemberName(MORTALITY_EQUATIONS)](m_eqMortality);
		in[GetMemberName(EQ_OPTIONS)](m_eq_options);
		in[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		in[GetMemberName(TOBS_FILE_NAME)](m_TobsFileName);
		in[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		in[GetMemberName(CALIB_ON)](m_calibOn);
		in[GetMemberName(CONVERGE_01)](m_bConverge01);
		in[GetMemberName(CALIB_SIGMA)](m_bCalibSigma);
		in[GetMemberName(FIXE_SIGMA)](m_bFixeSigma);
		in[GetMemberName(CONTROL)](m_ctrl);

		return true;
	}
}
