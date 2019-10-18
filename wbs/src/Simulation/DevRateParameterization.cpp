//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <math.h>
#include <sstream>
#include <boost/math/distributions/normal.hpp>

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


using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;

using namespace std;

namespace WBSF
{
	//**********************************************************************************************
	//CDevRateParameterization
	const char* CDevRateParameterization::DATA_DESCRIPTOR = "DevRateParameterizationData";
	const char* CDevRateParameterization::XML_FLAG = "DevRateParameterization";
	const char* CDevRateParameterization::MEMBERS_NAME[NB_MEMBERS_EX] = { "Equation", "InputFileName",  "OutputFileName", "Control", "Converge01" };
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
		m_name = "DevRateParameterization";

		m_equations.set();
		m_inputFileName.clear();
		m_outputFileName.clear();
		m_bConverge01 = false;

		m_ctrl.Reset();
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

			m_equations = in.m_equations;
			m_inputFileName = in.m_inputFileName;
			m_outputFileName = in.m_outputFileName;
			m_bConverge01 = in.m_bConverge01;
			m_ctrl = in.m_ctrl;
		}

		return *this;
	}

	bool CDevRateParameterization::operator == (const CDevRateParameterization& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator!=(in))bEqual = false;
		if (m_equations != in.m_equations) bEqual = false;
		if (m_inputFileName != in.m_inputFileName) bEqual = false;
		if (m_outputFileName != in.m_outputFileName) bEqual = false;
		if (m_ctrl != in.m_ctrl)bEqual = false;
		if (m_bConverge01 != in.m_bConverge01)bEqual = false;


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

			//			for (auto s = stages.begin(); s != stages.end(); s++)
			{
				//for all equation
				for (size_t e = 0; e < m_equations.size(); e++)
				{
					if (m_equations.test(e))
					{
						CDevRateEquation::TDevRateEquation  eq = CDevRateEquation::e(e);

						std::string name = CDevRateEquation::GetEquationName(eq);
						std::string title = CDevRateEquation::GetEquationName(eq);
						std::string units = "1/day";
						std::string description = "Development rate";
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
				//begin to read
				ifStream file;
				msg = file.open(inputFilePath);
				if (msg)
				{
					for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
					{
						if (loop->size() >= 3)
						{
							m_data.push_back({ (*loop)[0], stod((*loop)[1]), stod((*loop)[2]), 1.0 });
						}
						
						if (loop->size() >= 4)
						{
							try
							{
								m_data.back().m_n = stod((*loop)[3]);
							}
							catch (...)
							{
							}
							
						}
						/*else
						{
							msg.ajoute("Invalid line at " + to_string(m_data.size() + 1));
							msg.ajoute(loop->GetLastLine());
						}*/

					}

					file.close();
				}

				if (!msg)
					return msg;


				CResult result;
				CCDevRateOutputVector output;
//				CCDevRateOutputVector output2;
				set<string> variables;



				for (size_t s = 0; s < m_data.size(); s++)
					variables.insert(m_data[s].m_variable);


				//for all stage
				for (auto v = variables.begin(); v != variables.end() && msg; v++)
				{
					//for all equation
					for (size_t e = 0; e < m_equations.size() && msg; e++)
					{
						if (m_equations.test(e))
						{
							CDevRateOutput out(*v, CDevRateEquation::e(e));
							msg += InitialiseComputationVariable(out.m_variable, out.m_equation, out.m_parameters, out.m_computation, callback);
							output.push_back(out);
							//if(m_bConverge01)
							//	output2.push_back(out);

							//std::set<double> test;
							////test initial parameters
							//for (double T = 10; T <= 30; T += 1)
							//{
							//	double sim = CDevRateEquation::GetFValue(out.m_equation, out.m_computation.m_XP, T);
							//	if (sim < 0 || sim > 1)
							//		test.insert(T);
							//}

							//if(!test.empty())
							//	callback.AddMessage(string("Test ") + CDevRateEquation::GetEquationName(out.m_equation) + " failed : [" + to_string(*test.begin()) + ":" + to_string(*test.rbegin()) +"]");
							//

						}
					}
				}

				callback.PushTask("Search optimum. " + to_string(variables.size()) + " variables x " + to_string(m_equations.count()) + " equations: " + to_string(output.size()) + " curve to fits", output.size());


#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) 
				for (__int64 i = 0; i < (__int64)output.size(); i++)
	//			__int64 i = 2;
				{
#pragma omp flush(msg)
					if (msg)
					{

						msg += Optimize(output[i].m_variable, output[i].m_equation, m_bConverge01, output[i].m_parameters, output[i].m_computation, callback);
						//if (!output2.empty())
						//{
						//	msg += Optimize(output2[i].m_variable, output2[i].m_equation, true, output2[i].m_parameters, output2[i].m_computation, callback);
						//	if (output2[i].m_computation.m_Sopt[STAT_R²] > output1[i].m_computation.m_Sopt[STAT_R²] &&
						//		output2[i].m_computation.m_AICopt > output1[i].m_computation.m_AICopt)
						//	{
						//		//select output2
						//		output2[i].m_parameters = output1[i].m_parameters;
						//		output2[i].m_computation = output1[i].m_computation;
						//	}
						//}
						//	

#pragma omp critical(WRITE_INFO)
						{
							callback.AddMessage(output[i].m_variable + ": " + CDevRateEquation::GetEquationName(output[i].m_equation));
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

						sort(output.begin(), output.end(), [](const CDevRateOutput& a, const CDevRateOutput& b) {return a.m_computation.m_Fopt > b.m_computation.m_Fopt; });
						file << "Variable,EqName,P,Eq,RMSE,CD,R2,AIC" << endl;
						for (auto v = variables.begin(); v != variables.end() && msg; v++)
						{
							for (size_t i = 0; i < output.size(); i++)
							{
								if (output[i].m_variable == *v)
								{
									string name = CDevRateEquation::GetEquationName(output[i].m_equation);
									string eq = CDevRateEquation::GetEquationR(output[i].m_equation);
									string P = to_string(CDevRateEquation::GetParameters(output[i].m_equation, output[i].m_computation.m_Xopt));
									file << output[i].m_variable << "," << name << "," << P << ",\"" << eq << "\",";
									if (output[i].m_computation.m_Fopt > m_ctrl.GetVMiss())
									{
										file << output[i].m_computation.m_Sopt[RMSE] << "," << output[i].m_computation.m_Sopt[COEF_D] << ",";
										file << output[i].m_computation.m_Sopt[STAT_R²] << "," << output[i].m_computation.m_AICopt;
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
	ERMsg CDevRateParameterization::InitialiseComputationVariable(std::string s, TDevRateEquation  e, const CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
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
				msg.ajoute(s + ": " + CDevRateEquation::GetEquationName(CDevRateEquation::e(e)));
				msg.ajoute("The starting value (" + ToString(computation.m_XP[i]) + ") is not inside the bounds [" + ToString(computation.m_bounds[i].GetLowerBound()) + "," + ToString(computation.m_bounds[i].GetUpperBound()) + "].");
				return msg;
			}
		}

		computation.Initialize(m_ctrl.T(), m_ctrl.NEPS(), m_ctrl.GetVMiss());
		//  Evaluate the function with input X and return value as F.
		GetFValue(s, e, false, computation);

		computation.m_NFCNEV++;

		computation.m_X = computation.m_XP;
		computation.m_Xopt = computation.m_XP;

		if (computation.m_FP != m_ctrl.GetVMiss())
		{
			computation.m_S = computation.m_SP;
			computation.m_F = computation.m_FP;
			computation.m_AIC = computation.m_AICP;

			computation.m_Sopt = computation.m_SP;
			computation.m_Fopt = computation.m_FP;
			computation.m_AICopt = computation.m_AICP;

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

			line = FormatA("N=%10d\tT=%9.5f\tF=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf\tAIC=%8.5lf", computation.m_NFCNEV, computation.m_T, F, computation.m_Sopt[NB_VALUE], computation.m_Sopt[BIAS], computation.m_Sopt[MAE], computation.m_Sopt[RMSE], computation.m_Sopt[COEF_D], computation.m_Sopt[STAT_R²], computation.m_AICopt);
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
	ERMsg CDevRateParameterization::Optimize(string s, TDevRateEquation  e, bool bConverge01, CSAParameterVector& parameters, CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;

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

						//for (size_t k = 0; k < 20 && computation.m_FP == m_ctrl.GetVMiss()&&msg; k++)
						//{
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
						GetFValue(s, e, bConverge01, computation);
						//}


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
								computation.m_AIC = computation.m_AICP;
								computation.m_S = computation.m_SP;
								computation.m_NACC++;
								NACP[h]++;
								NUP++;

								//  If greater than any other point, record as new optimum.
								if (computation.m_FP > computation.m_Fopt)
								{
									computation.m_Xopt = computation.m_XP;
									computation.m_Fopt = computation.m_FP;
									computation.m_AICopt = computation.m_AICP;
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
									computation.m_AIC = computation.m_AICP;
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


	bool CDevRateParameterization::GetFValue(string s, TDevRateEquation  e, bool bConverge01, CComputationVariable& computation)
	{
		bool bValid = false;


		if (CDevRateEquation::IsParamValid(e, computation.m_XP))
		{
			CStatisticXYEx stat;

			for (__int64 i = 0; i < (__int64)m_data.size(); i++)
			{
				if (WBSF::IsEqualNoCase(m_data[i].m_variable, s))
				{
					double sim = CDevRateEquation::GetRate(e, computation.m_XP, m_data[i].m_T);

					if (bConverge01)
					{
						if (sim < 0)
							sim = -exp(1000 / sim);//let the code to give a chance to converge to the good direction
						else if (sim > 1)
							sim = 1 + exp(-1000 / (sim - 1));

						//let the code to give a chance to converge to the good direction
						//if (sim < 0)
//							sim = -log(1-sim);
	//					else if (sim > 1)
		//					sim = 1 + log(sim);
					}

					if (!isfinite(sim) || isnan(sim) || sim<-1E8 || sim>1E8)
					{
						stat.Reset();//find other set of parameters
						return false;
					}

					double obs = m_data[i].m_rate;
					for (size_t n = 0; n < m_data[i].m_n; n++)
						stat.Add(sim, obs);
				}
			}

			//  If the function is to be minimized, switch the sign of the function.
				//  Note that all intermediate and final output switches the sign back
				//  to eliminate any possible confusion for the user.
			if (stat[NB_VALUE] > 1)
			{
				double LL = 0;

				//used sigma ML hat instead of classical sigma hat
				double sigma = sqrt(stat[RSS] / (stat[NB_VALUE] - 1))*(stat[NB_VALUE]) / (stat[NB_VALUE] - 1);
				double sigmaML = sigma * sqrt((stat[NB_VALUE] - computation.m_XP.size()) / stat[NB_VALUE]);
				if (sigmaML > 0)
				{
					for (size_t i = 0; i < stat[NB_VALUE]; i++)
					{
						double m = stat.x(i);
						boost::math::normal_distribution<> N(m, sigmaML);

						double x = stat.y(i);
						double p = boost::math::pdf(N, x);

						ASSERT(p > 0);
						LL += log(p);
					}
					computation.m_AICP = -2 * LL + 2 * (computation.m_XP.size() + 1);
					computation.m_FP = m_ctrl.GetFinalFValue(stat);
					computation.m_SP = stat;
				}
				else
				{
					stat.Reset();//find other set of parameters
					bValid = false;
				}
			}
		}

		return bValid;
	}

	void CDevRateParameterization::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(EQUATIONS)](m_equations);
		out[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		out[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		out[GetMemberName(CONVERGE_01)](m_bConverge01);
		out[GetMemberName(CONTROL)](m_ctrl);

	}

	bool CDevRateParameterization::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(EQUATIONS)](m_equations);
		in[GetMemberName(INPUT_FILE_NAME)](m_inputFileName);
		in[GetMemberName(OUTPUT_FILE_NAME)](m_outputFileName);
		in[GetMemberName(CONVERGE_01)](m_bConverge01);
		in[GetMemberName(CONTROL)](m_ctrl);

		return true;
	}
}
