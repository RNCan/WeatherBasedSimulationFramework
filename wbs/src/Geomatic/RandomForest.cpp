//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	R�mi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "RandomForest.h"
#include "RangerLib\Forest\ForestSurvival.h"
#include "RangerLib\Forest\ForestClassification.h"
#include "RangerLib\Forest\ForestProbability.h"
#include "RangerLib\Forest\ForestRegression.h"
#include "RangerLib\Utility\DataFloat.h"
#include "Basic/UtilStd.h"
#include "Basic/Shore.h"

using namespace std;

namespace WBSF
{

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CRandomForest::CRandomForest():
		m_pForest(nullptr)
	{
		Reset();
	}

	void CRandomForest::Reset()
	{
	}

	CRandomForest::~CRandomForest()
	{
		if (m_pForest)
		{
			delete m_pForest;
			m_pForest = nullptr;
		}

	}


	//**************************************************************************
	ERMsg CRandomForest::CreateForest(size_t treetype, size_t nbTrees, size_t importance_mode)
	{
		ERMsg msg;

		if (m_pForest)
		{
			delete m_pForest;
			m_pForest = nullptr;
		}

		try {


			// Create forest object
			switch (treetype) 
			{
			case TREE_CLASSIFICATION:	m_pForest = new ForestClassification; break;
			case TREE_REGRESSION:		m_pForest = new ForestRegression; break;
			case TREE_SURVIVAL:			m_pForest = new ForestSurvival; break;
			case TREE_PROBABILITY:		m_pForest = new ForestProbability;  break;
			default: ASSERT(false);
			}

			// Verbose output to logfile if non-verbose mode
			//std::ostream* verbose_out;
			/*if (arg_handler.verbose) {
				verbose_out = &std::cout;
			}
			else {
				std::ofstream* logfile = new std::ofstream();
				logfile->open(arg_handler.outprefix + ".log");
				if (!logfile->good()) {
					throw std::runtime_error("Could not write to logfile.");
				}
				verbose_out = logfile;
			}
*/

			//this->verbose_out = verbose_out;

			// Initialize data with memmode
			/*if (m_pData)
			{
				delete m_pData;
				m_pData = nullptr;
			}
*/
			CGridPointVector* pPts = m_pPts.get();

			double xValPercent = max(0.0, min(1.0, m_param.m_XvalPoints));
			size_t nbPoints = max(1.0, (1 - xValPercent)*m_pPts->size());
			m_inc = max(1.0, (double)pPts->size() / nbPoints);

			StringVector names("X|Y|Z|Elev|Expo|Shore","|");

			DataFloat input;
			input.setVariableNames(names);
			input.resize(pPts->size(), 6);
			
			for (size_t i = 0, ii = 0; ii < pPts->size(); ++i, ii = i * m_inc)
			{
				const CGridPoint& ptTmp = pPts->at(int(ii));

				bool error = false;
				if (ptTmp.IsProjected())
				{
					ASSERT(false);
				}
				else
				{
					input.set(0, 0, ptTmp[0], error);
					input.set(1, 0, ptTmp[1], error);
					input.set(2, 0, ptTmp[2], error);
					input.set(3, 0, ptTmp.m_alt, error);
					input.set(4, 0, ptTmp.GetExposition(), error);
					input.set(5, 0, CShore::GetShoreDistance(ptTmp), error);
				}
			}

			// Load data
			//*verbose_out << "Loading input file: " << input_file << "." << std::endl;
			//bool rounding_error = data->loadFromFile(input_file);
			//if (rounding_error) {
			//	*verbose_out << "Warning: Rounding or Integer overflow occurred. Use FLOAT or DOUBLE precision to avoid this."
			//		<< std::endl;
			//}

			// Set prediction mode
			
			// Call other init function
			m_pForest->init_grow("Variable", &input, 0, (uint)nbTrees, 0, DEFAULT_NUM_THREADS, (ImportanceMode)importance_mode, 0, "", true, std::vector<std::string>(),
				false, DEFAULT_SPLITRULE, 1, DEFAULT_ALPHA, DEFAULT_MINPROP, false, DEFAULT_NUM_RANDOM_SPLITS);

			
			// Set variables to be always considered for splitting
		//	if (!always_split_variable_names.empty()) {
			//	setAlwaysSplitVariables(always_split_variable_names);
			//}

			// TODO: Read 2d weights for tree-wise split select weights
			// Load split select weights from file
			//if (!split_select_weights_file.empty()) {
				//std::vector<std::vector<double>> split_select_weights;
				//split_select_weights.resize(1);
				//loadDoubleVectorFromFile(split_select_weights[0], split_select_weights_file);
				//if (split_select_weights[0].size() != num_variables - 1) {
					//throw std::runtime_error("Number of split select weights is not equal to number of independent variables.");
				//}
				//setSplitWeightVector(split_select_weights);
			//}

			// Load case weights from file
			//if (!case_weights_file.empty()) {
			//	loadDoubleVectorFromFile(case_weights, case_weights_file);
			//	if (case_weights.size() != num_samples - 1) {
			//		throw std::runtime_error("Number of case weights is not equal to number of samples.");
			//	}
			//}

			// Sample from non-zero weights in holdout mode
			//if (holdout && !case_weights.empty()) {
			//	size_t nonzero_weights = 0;
			//	for (auto& weight : case_weights) {
			//		if (weight > 0) {
			//			++nonzero_weights;
			//		}
			//	}
			//	this->sample_fraction = this->sample_fraction * ((double)nonzero_weights / (double)num_samples);
			//}

			// Check if all catvars are coded in integers starting at 1
			//if (!unordered_variable_names.empty()) {
			//	std::string error_message = checkUnorderedVariables(data, unordered_variable_names);
			//	if (!error_message.empty()) {
			//		throw std::runtime_error(error_message);
			//	}
			//}


			// Call Ranger
			//*verbose_out << "Starting Ranger." << std::endl;

			m_pForest->run_grow(&input);
			
			
		}
		catch (std::exception& e) 
		{
			delete m_pForest;
			m_pForest = nullptr;

			msg.ajoute(  "Error: Ranger fail to create forest." );
			msg.ajoute(e.what());
			
			
		}

		return msg;
	}

	ERMsg CRandomForest::Initialization(CCallback& callback)
	{
		ERMsg msg = CGridInterpolBase::Initialization(callback);

		if (!m_bInit)
		{
			//Compute forest
			//msg = CreateForest(m_param.m_treeType, m_param.m_nbTrees, m_param.m_importance_mode);
			msg = CreateForest(TREE_REGRESSION, 500, DEFAULT_IMPORTANCE_MODE);
			if (msg)
			{
				//now init for prediction
				m_pForest->init_predict(0, 1, false, DEFAULT_PREDICTIONTYPE);
				m_bInit = true;
			}
				

		}


		return msg;
	}


	double CRandomForest::Evaluate(const CGridPoint& pt, int iXval)const
	{

		if (iXval >= 0)
		{
			int l = (int)ceil((iXval) / m_inc);
			if (int(l*m_inc) == iXval)
				return m_param.m_noData;
		}

		DataFloat input;
		input.resize(1, 6);

		bool error = false;
		if (pt.IsProjected())
		{
			ASSERT(false);
		}
		else
		{
			input.set(0, 0, pt[0], error);
			input.set(1, 0, pt[1], error);
			input.set(2, 0, pt[2], error);
			input.set(3, 0, pt.m_alt, error);
			input.set(4, 0, pt.GetExposition(), error);
			input.set(5, 0, CShore::GetShoreDistance(pt), error);
		}
		



		m_pForest->run_predict(&input);
		double value = m_pForest->getPredictions().at(0).at(0).at(0);
		value = m_prePostTransfo.InvertTransform(value, m_param.m_noData);
		

		if ( m_param.m_bGlobalLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV] || value>m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bGlobalLimitToBound)
					value = min(m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV], max(m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV], value));
				else
					value = m_param.m_noData;
			}
		}

		if ( m_param.m_bGlobalMinMaxLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_param.m_globalMinLimit || value>m_param.m_globalMaxLimit;
			if (bOutside)
			{
				if (m_param.m_bGlobalMinMaxLimitToBound)
					value = min(m_param.m_globalMaxLimit, max(m_param.m_globalMinLimit, value));
				else
					value = m_param.m_noData;
			}
		}

		return value;
	}

	string CRandomForest::GetFeedbackBestParam()const
	{
		string str;// = string(CGridInterpolParam::GetMemberName(CGridInterpolParam::REGRESSION_MODEL)) + " = " + m_regression.GetEquation();

		//str.Format( "%s = %s\n", 
		//	CGridInterpolParam::GetMemberName(CGridInterpolParam::REGRESSION_MODEL),
		//	//m_param.GetMember(CGridInterpolParam::REGRESSION_MODEL),
		//	(LPCTSTR)m_regression.GetEquation()
		//	);

		return str;
	};

	//double CRandomForest::GetOptimizedR�()const
	//{
	//	//Don't call parent because we don't want to do Xvalidation
	//	return 1;
	//}

	void CRandomForest::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

		//if (m_param.m_regressionModel.empty())
		//{
		//	//this is to call GetOptimiseR� and initialise term with StepWise
		//	parameterset.resize(1, m_param);

		//	//Compute best regression
		//	StepWise(parameterset[0].m_regressionModel);
		//}
	}



}