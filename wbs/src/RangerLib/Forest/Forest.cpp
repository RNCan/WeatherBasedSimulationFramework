/*-------------------------------------------------------------------------------
 This file is part of Ranger.

 Ranger is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Ranger is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Ranger. If not, see <http://www.gnu.org/licenses/>.

 Written by:

 Marvin N. Wright
 Institut für Medizinische Biometrie und Statistik
 Universität zu Lübeck
 Ratzeburger Allee 160
 23562 Lübeck
 Germany

 http://www.imbs-luebeck.de
 #-------------------------------------------------------------------------------*/

#include <math.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <ctime>
#include <assert.h>
 //#include <math.h>
#ifndef OLD_WIN_R_BUILD
#include <thread>
#include <chrono>
#endif

#include "RangerLib/Utility/utility.h"
#include "RangerLib/Forest/Forest.h"
#include "RangerLib/Utility/DataChar.h"
#include "RangerLib/Utility/DataShort.h"
#include "RangerLib/Utility/DataFloat.h"
#include "RangerLib/Utility/DataDouble.h"

const size_t Forest::FOREST_FILE_VERSION = 1;


Forest::Forest() :
	verbose_out(0), num_trees(DEFAULT_NUM_TREE), mtry(0), min_node_size(0), /*num_variables(0),*/ num_independent_variables(
		0), seed(0), dependent_varID(0), /*num_samples(0),*/ prediction_mode(false), sample_with_replacement(
			true), memory_saving_splitting(false), splitrule(DEFAULT_SPLITRULE), predict_all(false), keep_inbag(false), sample_fraction(
				1), holdout(false), prediction_type(DEFAULT_PREDICTIONTYPE), num_random_splits(DEFAULT_NUM_RANDOM_SPLITS), alpha(
					DEFAULT_ALPHA), minprop(DEFAULT_MINPROP), num_threads(DEFAULT_NUM_THREADS), /*data(0),*/ overall_prediction_error(
						0), importance_mode(DEFAULT_IMPORTANCE_MODE), progress(0), status_varID(-1) {
}

Forest::~Forest() {
	for (auto& tree : trees) {
		delete tree;
	}
}





// #nocov start
Data* Forest::initCpp_grow(std::string dependent_variable_name, MemoryMode memory_mode, std::string input_file, uint mtry,
	uint num_trees, std::ostream* verbose_out, uint seed, uint num_threads,
	/*std::string load_forest_filename,*/ ImportanceMode importance_mode, uint min_node_size,
	std::string split_select_weights_file, std::vector<std::string>& always_split_variable_names,
	std::string status_variable_name, bool sample_with_replacement, std::vector<std::string>& unordered_variable_names,
	bool memory_saving_splitting, SplitRule splitrule, std::string case_weights_file,
	double sample_fraction, double alpha, double minprop, bool holdout, uint num_random_splits,
	std::string file_virtual_cols, std::string ignore_cols_str) {

	this->dependent_variable_name = dependent_variable_name;
	this->verbose_out = verbose_out;

	//allocate space
	Data* training = CreateMemory(memory_mode);

	// Load data
	if (verbose_out)
		*verbose_out << "Loading input file: " << input_file << "." << std::endl;

	bool rounding_error = training->loadFromFile(input_file);
	if (rounding_error) {
		if (verbose_out)
			*verbose_out << "Warning: Rounding or Integer overflow occurred. Use FLOAT or DOUBLE precision to avoid this." << std::endl;
	}
	
	initial_input_cols_name = training->get_initial_input_cols_name();

	//add virtual cols
	if (!file_virtual_cols.empty())
	{
		if (verbose_out)
			*verbose_out << "Loading virtual columns : " << file_virtual_cols << "." << std::endl;

		if (Data::load_expression_file(file_virtual_cols, virtual_cols_txt, virtual_cols_name) == 0)
			throw std::runtime_error("Failed to load virtual columns from: " + file_virtual_cols);

		if (!virtual_cols_name.empty())
		{
			bool rounding_error = training->update_virtual_cols(virtual_cols_txt, virtual_cols_name);
			if (rounding_error) {
				if (verbose_out)
					*verbose_out << "Warning: Rounding or Integer overflow occurred on virtual variables. Use FLOAT or DOUBLE precision to avoid this." << std::endl;
			}

		}
	}

	

	

	//remove ignore columns
	if (!ignore_cols_str.empty())
	{
		std::vector<std::string> cols_name = training->getVariableNames();

		assert(!cols_name.empty());
		ignore_cols = tokenize_string(ignore_cols_str.c_str(), ',');

		//verify that name exists
		for (auto& col_name : ignore_cols)
		{
			if (col_name == dependent_variable_name)
				throw std::runtime_error("Dependant variable can't be ignore: " + col_name);

			auto it = find(cols_name.begin(), cols_name.end(), col_name);
			if (it == cols_name.end())
				throw std::runtime_error("Invalid column to ignore: " + col_name);

			cols_name.erase(it);
		}

		training->reshape(cols_name);
	}

	init_grow(dependent_variable_name, training, mtry, num_trees, seed, num_threads, importance_mode,
		min_node_size, status_variable_name, sample_with_replacement, unordered_variable_names,
		memory_saving_splitting, splitrule, sample_fraction, alpha, minprop, holdout, num_random_splits);

	
	assert(dependent_varID < training->getVariableNames().size() );
	independent_variable_names = training->getVariableNames();
	independent_variable_names.erase(independent_variable_names.begin() + dependent_varID);

	assert(independent_variable_names.size() == num_independent_variables);

	// Set variables to be always considered for splitting
	if (!always_split_variable_names.empty()) {
		setAlwaysSplitVariables(training, always_split_variable_names);
	}

	// TODO: Read 2d weights for tree-wise split select weights
	// Load split select weights from file
	if (!split_select_weights_file.empty()) {
		std::vector<std::vector<double>> split_select_weights;
		split_select_weights.resize(1);
		loadDoubleVectorFromFile(split_select_weights[0], split_select_weights_file);
		if (split_select_weights[0].size() != training->getNumCols() - 1) {
			throw std::runtime_error("Number of split select weights is not equal to number of independent variables.");
		}
		setSplitWeightVector(training, split_select_weights);
	}

	// Load case weights from file
	if (!case_weights_file.empty()) {
		loadDoubleVectorFromFile(case_weights, case_weights_file);
		if (case_weights.size() != training->getNumRows() - 1) {
			throw std::runtime_error("Number of case weights is not equal to number of samples.");
		}
	}


	// Sample from non-zero weights in holdout mode
	if (holdout && !case_weights.empty()) {
		size_t nonzero_weights = 0;
		for (auto& weight : case_weights) {
			if (weight > 0) {
				++nonzero_weights;
			}
		}
		this->sample_fraction = this->sample_fraction * ((double)nonzero_weights / (double)training->getNumRows());
	}

	// Check if all catvars are coded in integers starting at 1
	if (!unordered_variable_names.empty()) {
		std::string error_message = checkUnorderedVariables(training, unordered_variable_names);
		if (!error_message.empty()) {
			throw std::runtime_error(error_message);
		}
	}

	return training;
}

Data* Forest::initCpp_predict(MemoryMode memory_mode, std::string input_file,
	std::ostream* verbose_out, uint seed, uint num_threads,
	std::string load_forest_filename, bool predict_all, PredictionType prediction_type)
{
	this->verbose_out = verbose_out;

	init_predict(seed, num_threads, predict_all, prediction_type);

	
	Data* data = CreateMemory(memory_mode);


	// Load data
	if (verbose_out)
		*verbose_out << "Loading input file: " << input_file << "." << std::endl;

	// Load tree after the data to remove dependant variable if present
	loadFromFile(load_forest_filename);
	

	bool rounding_error = data->loadFromFile(input_file);
	if (rounding_error) {
		if (verbose_out)
			*verbose_out << "Warning: Rounding or Integer overflow occurred. Use FLOAT or DOUBLE precision to avoid this." << std::endl;
	}

	//remove dependant and status columns
	/*if (data->getVariableID(dependent_variable_name) != -1 ||
		data->getVariableID(status_var_name) != -1)
	{
		std::vector<std::string> cols_name = data->getVariableNames();
		
		auto it = std::find(cols_name.begin(), cols_name.end(), dependent_variable_name);
		if(it!= cols_name.end())
			cols_name.erase(it) ;

		it = std::find(cols_name.begin(), cols_name.end(), status_var_name);
		if (it != cols_name.end())
			cols_name.erase(it);

	}
*/



	if (!virtual_cols_name.empty())
	{
		data->update_virtual_cols(virtual_cols_txt, virtual_cols_name);
	}


	//independent_variable_names = data->getVariableNames();
	std::vector<std::string> cols_name = data->getVariableNames();

	if (data->getVariableID(dependent_variable_name) != -1)
		ignore_cols.push_back(dependent_variable_name);
	if (data->getVariableID(status_var_name) != -1)
		ignore_cols.push_back(status_var_name);

	//remove ignore columns
	if (!ignore_cols.empty())
	{
		//verify tha name exists
		for (auto& col_name : ignore_cols)
		{
			auto it = find(cols_name.begin(), cols_name.end(), col_name);
			if (it == cols_name.end())
			{
				throw std::runtime_error("Invalid independant variable column selection: " + col_name);
			}
			cols_name.erase(it);
		}

		data->reshape(cols_name);
	}


	//verify data
	//data->verify_data(independent_variable_names);
	for (size_t i=0, ii = 0;  i < independent_variable_names.size(); i++)
	{
		std::string col_name = independent_variable_names[i];
			
		if (ii >= cols_name.size() || col_name != cols_name[ii])
		{
			if (col_name == dependent_variable_name ||
				col_name == status_var_name)
			{
				ii--;//skip this cols, not in predic data
			}
			else
			{
				if (find(cols_name.begin(), cols_name.end(), col_name) == cols_name.end())
					throw std::runtime_error("Missing independant column: " + col_name);
				else
					throw std::runtime_error("Bad independant column order at: " + col_name);
			}
		}

		ii++;
		
	}

	//	throw std::runtime_error("Invalid prediction input: " + col_name);


	assert(cols_name.size() - independent_variable_names.size() <= 2);//one for predited and status


	


	return data;
}
// #nocov end

//void Forest::initR(std::string dependent_variable_name, Data* data, uint mtry, uint num_trees,
//    std::ostream* verbose_out, uint seed, uint num_threads, ImportanceMode importance_mode, uint min_node_size,
//    std::vector<std::vector<double>>& split_select_weights, std::vector<std::string>& always_split_variable_names,
//    /*std::string status_variable_name, */bool prediction_mode, bool sample_with_replacement,
//    std::vector<std::string>& unordered_variable_names, bool memory_saving_splitting, SplitRule splitrule,
//    std::vector<double>& case_weights, bool predict_all, bool keep_inbag, double sample_fraction, double alpha,
//    double minprop, bool holdout, PredictionType prediction_type, uint num_random_splits) {
//
//  this->verbose_out = verbose_out;
//
//  // Call other init function
//  init(/*dependent_variable_name,*/ /*input_data, */mtry, num_trees, seed, num_threads, importance_mode,
//      min_node_size/*, status_variable_name*/, prediction_mode, sample_with_replacement,/* unordered_variable_names,*/
//      memory_saving_splitting, splitrule, predict_all, sample_fraction, alpha, minprop, holdout, prediction_type,
//      num_random_splits);
//
//  // Set variables to be always considered for splitting
//  if (!always_split_variable_names.empty()) {
//    setAlwaysSplitVariables(always_split_variable_names);
//  }
//
//  // Set split select weights
//  if (!split_select_weights.empty()) {
//    setSplitWeightVector(split_select_weights);
//  }
//
//  // Set case weights
//  if (!case_weights.empty()) {
//	  if (case_weights.size() != data->getNumRows()) {
//      throw std::runtime_error("Number of case weights not equal to number of samples.");
//    }
//    this->case_weights = case_weights;
//  }
//
//  // Keep inbag counts
//  this->keep_inbag = keep_inbag;
//}

void Forest::init_predict(uint seed, uint num_threads, bool predict_all, PredictionType prediction_type) {

	// Initialize random number generator and set seed
	if (seed == 0) {
		std::random_device random_device;
		random_number_generator.seed(random_device());
	}
	else {
		random_number_generator.seed(seed);
	}

	// Set number of threads
	if (num_threads == DEFAULT_NUM_THREADS) {
#ifdef OLD_WIN_R_BUILD
		this->num_threads = 1;
#else
		this->num_threads = std::thread::hardware_concurrency();
#endif
	}
	else {
		this->num_threads = num_threads;
	}

	// Set member variables
	this->seed = seed;
	this->prediction_mode = true;
	this->predict_all = predict_all;
	this->prediction_type = prediction_type;


	// Init split select weights
	split_select_weights.push_back(std::vector<double>());
}

void Forest::init_grow(std::string dependent_variable_name, Data* training, uint mtry,
	uint num_trees, uint seed, uint num_threads, ImportanceMode importance_mode,
	uint min_node_size, std::string status_variable_name/*, bool prediction_mode*/, bool sample_with_replacement,
	std::vector<std::string>& unordered_variable_names, bool memory_saving_splitting, SplitRule splitrule,
	/*bool predict_all,*/ double sample_fraction, double alpha, double minprop, bool holdout,
	/*PredictionType prediction_type,*/ uint num_random_splits)
{


	// Initialize data with memmode
	//this->training = training;

	// Initialize random number generator and set seed
	if (seed == 0) {
		std::random_device random_device;
		random_number_generator.seed(random_device());
	}
	else {
		random_number_generator.seed(seed);
	}

	// Set number of threads
	if (num_threads == DEFAULT_NUM_THREADS) {
#ifdef OLD_WIN_R_BUILD
		this->num_threads = 1;
#else
		this->num_threads = std::thread::hardware_concurrency();
#endif
	}
	else {
		this->num_threads = num_threads;
	}

	// Set member variables
	this->num_trees = num_trees;
	this->mtry = mtry;
	this->seed = seed;
	this->importance_mode = importance_mode;
	this->min_node_size = min_node_size;
	this->prediction_mode = false;
	this->sample_with_replacement = sample_with_replacement;
	this->memory_saving_splitting = memory_saving_splitting;
	this->splitrule = splitrule;
	this->predict_all = predict_all;
	this->sample_fraction = sample_fraction;
	this->holdout = holdout;
	this->alpha = alpha;
	this->minprop = minprop;
	this->prediction_type = prediction_type;
	this->num_random_splits = num_random_splits;
	this->status_var_name = status_variable_name;

	// Convert dependent variable name to ID
	if (!dependent_variable_name.empty()) {
		dependent_varID = training->getVariableID(dependent_variable_name);
		if (dependent_varID >= training->getNumCols())
		{
			throw std::runtime_error("Dependant variable " + dependent_variable_name + " not found.");
		}
	}

	// Set unordered factor variables
	training->setIsOrderedVariable(unordered_variable_names);
	this->training_is_ordered_variable = training->getIsOrderedVariable();

	training->addNoSplitVariable(dependent_varID);

	
	init_internal_grow(training);

	num_independent_variables = training->getNumCols() - training->getNoSplitVariables().size();

	// Init split select weights
	split_select_weights.push_back(std::vector<double>());

	// Check if mtry is in valid range
	if (this->mtry > training->getNumCols() - 1) {
		throw std::runtime_error("mtry can not be larger than number of variables in data.");
	}

	// Check if any observations samples
	if ((size_t)training->getNumRows() * sample_fraction < 1) {
		throw std::runtime_error("sample_fraction too small, no observations sampled.");
	}

	// Permute samples for corrected Gini importance
	if (importance_mode == IMP_GINI_CORRECTED) {
		training->permuteSampleIDs(random_number_generator);
	}

}


//void Forest::run(Data* data) {
//
//	if (prediction_mode) {
//		if (verbose_out) {
//			*verbose_out << "Predicting .." << std::endl;
//		}
//		
//		predict(data);
//	}
//	else {
//		if (verbose_out) {
//			*verbose_out << "Growing trees .." << std::endl;
//		}
//
//		grow(data);
//
//		if (verbose_out) {
//			*verbose_out << "Computing prediction error .." << std::endl;
//		}
//		computePredictionError(data);
//
//		if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW || importance_mode == IMP_PERM_RAW) {
//			if (verbose_out) {
//				*verbose_out << "Computing permutation variable importance .." << std::endl;
//			}
//			computePermutationImportance();
//		}
//	}
//}

void Forest::run_grow(Data* data) {


	if (verbose_out) {
		*verbose_out << "Growing trees .." << std::endl;
	}

	grow(data);

	if (verbose_out) {
		*verbose_out << "Computing prediction error .." << std::endl;
	}

	computePredictionError(data);

	if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW || importance_mode == IMP_PERM_RAW) {
		if (verbose_out) {
			*verbose_out << "Computing permutation variable importance .." << std::endl;
		}
		computePermutationImportance();
	}

}
void Forest::run_predict(Data* data) {


	if (verbose_out) {
		*verbose_out << "Predicting .." << std::endl;
	}

	data->setIsOrderedVariable(training_is_ordered_variable);
	dependent_varID = data->getVariableID(dependent_variable_name);




	//Check if mtry is in valid range
	//if (this->mtry > data->getNumCols() - 1) {
	//throw std::runtime_error("mtry can not be larger than number of variables in data.");
	//}

	// Check if any observations samples
	//if ((size_t)data->getNumRows() * sample_fraction < 1) {
	//throw std::runtime_error("sample_fraction too small, no observations sampled.");
	//}

	// Permute samples for corrected Gini importance
	if (importance_mode == IMP_GINI_CORRECTED) {
		data->permuteSampleIDs(random_number_generator);
	}

	//allocate memory
	init_internal_predict(data);

	if (num_threads == -1)
		predict_no_thread(data);
	else
		predict(data);
}
// #nocov start
void Forest::writeOutput(Data* training, std::string output_prefix) {

	if (verbose_out)
	{
		*verbose_out << std::endl;
		*verbose_out << "Tree type:                         " << std::string(typeid(*this).name()).substr(6) << std::endl;
		writeOutputInternal();
		*verbose_out << "Dependent variable name:           " << dependent_variable_name << std::endl;
		*verbose_out << "Dependent column:                  " << dependent_varID + 1 << std::endl;
		*verbose_out << "Number of trees:                   " << num_trees << std::endl;
		*verbose_out << "Sample size:                       " << training->getNumRows() << std::endl;
		*verbose_out << "Number of input columns:           " << training->get_initial_input_cols_name().size() << std::endl;
		*verbose_out << "Number of virtual columns:         " << virtual_cols_name.size() << std::endl;
		*verbose_out << "Number of ignore columns:          " << ignore_cols.size() << " (" << (ignore_cols.empty() ? "none" : to_string(ignore_cols)) << ")" << std::endl;
		*verbose_out << "Number of independent variables:   " << independent_variable_names.size() << std::endl;
		*verbose_out << "Mtry:                              " << mtry << std::endl;
		*verbose_out << "Target node size:                  " << min_node_size << std::endl;
		*verbose_out << "Variable importance mode:          " << importance_mode << std::endl;
		*verbose_out << "Memory mode:                       " << training->memory_mode() << std::endl;
		*verbose_out << "Seed:                              " << seed << std::endl;
		*verbose_out << "Number of threads:                 " << num_threads << std::endl;
		*verbose_out << std::endl;

		*verbose_out << "Overall OOB prediction error:      " << overall_prediction_error << std::endl;
		*verbose_out << std::endl;

		if (!split_select_weights.empty() & !split_select_weights[0].empty()) {
			*verbose_out
				<< "Warning: Split select weights used. Variable importance measures are only comparable for variables with equal weights."
				<< std::endl;
		}

	}

	if (importance_mode != IMP_NONE) {
		std::string filename = output_prefix + ".importance";
		writeImportanceFile(training, filename);
	}

	std::string filename = output_prefix + ".confusion";
	writeConfusionFile(filename);
	//}
}

void Forest::writeImportanceFile(Data* training, std::string filename) {

	// Open importance file for writing
	//std::string filename = output_prefix + ".importance";
	std::ofstream importance_file;
	importance_file.open(filename, std::ios::out);
	if (!importance_file.good()) {
		throw std::runtime_error("Could not write to importance file: " + filename + ".");
	}

	// Write importance to file
	for (size_t i = 0; i < variable_importance.size(); ++i) {
		size_t varID = i;
		for (auto& skip : training->getNoSplitVariables()) {
			if (varID >= skip) {
				++varID;
			}
		}
		std::string variable_name = training->getVariableNames()[varID];
		importance_file << variable_name << ": " << variable_importance[i] << std::endl;
	}

	importance_file.close();
	if (verbose_out)
		*verbose_out << "Saved variable importance to file " << filename << "." << std::endl;
}

void Forest::saveToFile(std::string filename) {

	if (verbose_out)
		*verbose_out << "Save forest..." << std::endl;

	// Open file for writing
	std::ofstream outfile;
	outfile.open(filename, std::ios::binary);
	if (!outfile.good()) {
		throw std::runtime_error("Could not write to output file: " + filename + ".");
	}

	// Write dependent_varID
	outfile.write((char*)&FOREST_FILE_VERSION, sizeof(FOREST_FILE_VERSION));
	saveVector1D(initial_input_cols_name, outfile);

	outfile.write((char*)&dependent_varID, sizeof(dependent_varID));
	saveString(dependent_variable_name, outfile);
	saveString(status_var_name, outfile);
	
	outfile.write((char*)&num_independent_variables, sizeof(num_independent_variables));
	saveVector1D(independent_variable_names, outfile);

	// Write is_ordered_variable
	saveString(virtual_cols_txt, outfile);
	saveVector1D(virtual_cols_name, outfile);

	saveVector1D(ignore_cols, outfile);
	saveVector1D(training_is_ordered_variable, outfile);


	// Write num_trees
	outfile.write((char*)&num_trees, sizeof(num_trees));
	saveToFileInternal(outfile);

	// Write tree data for each tree
	for (auto& tree : trees) {
		tree->appendToFile(outfile);
	}

	// Close file
	outfile.close();
	if (verbose_out)
		*verbose_out << "Saved forest to file " << filename << "." << std::endl;
}
// #nocov end

Data* Forest::CreateMemory(MemoryMode memory_mode)
{
	Data* data = NULL;
	// Initialize data with memmode
	switch (memory_mode) {
	case MEM_DOUBLE:
		data = new DataDouble();
		break;
	case MEM_FLOAT:
		data = new DataFloat();
		break;
	case MEM_SHORT:
		data = new DataShort();
		break;
	case MEM_CHAR:
		data = new DataChar();
		break;
	}

	return data;
}

void Forest::grow(Data* data) {

	// Create thread ranges
	equalSplit(thread_ranges, 0, uint(num_trees - 1), num_threads);

	// Call special grow functions of subclasses. There trees must be created.
	growInternal(data);

	// Init trees, create a seed for each tree, based on main seed
	std::uniform_int_distribution<uint> udist;
	for (size_t i = 0; i < num_trees; ++i) {
		uint tree_seed;
		if (seed == 0) {
			tree_seed = udist(random_number_generator);
		}
		else {
			tree_seed = (uint)((i + 1) * seed);
		}

		// Get split select weights for tree
		std::vector<double>* tree_split_select_weights;
		if (split_select_weights.size() > 1) {
			tree_split_select_weights = &split_select_weights[i];
		}
		else {
			tree_split_select_weights = &split_select_weights[0];
		}

		trees[i]->init(data, mtry, dependent_varID, data->getNumRows(), tree_seed, &deterministic_varIDs, &split_select_varIDs,
			tree_split_select_weights, importance_mode, min_node_size, sample_with_replacement, memory_saving_splitting,
			splitrule, &case_weights, keep_inbag, sample_fraction, alpha, minprop, holdout, num_random_splits);
	}

	// Init variable importance
	variable_importance.resize(num_independent_variables, 0);

	// Grow trees in multiple threads
#ifdef OLD_WIN_R_BUILD
	progress = 0;
	clock_t start_time = clock();
	clock_t lap_time = clock();
	for (size_t i = 0; i < num_trees; ++i) {
		trees[i]->grow(&variable_importance);
		progress++;
		showProgress("Growing trees..", start_time, lap_time);
	}
#else
	progress = 0;
#ifdef R_BUILD
	aborted = false;
	aborted_threads = 0;
#endif

	std::vector<std::thread> threads;
	threads.reserve(num_threads);

	// Initailize importance per thread
	std::vector<std::vector<double>> variable_importance_threads(num_threads);

	for (uint i = 0; i < num_threads; ++i) {
		if (importance_mode == IMP_GINI || importance_mode == IMP_GINI_CORRECTED) {
			variable_importance_threads[i].resize(num_independent_variables, 0);
		}
		threads.push_back(std::thread(&Forest::growTreesInThread, this, i, &(variable_importance_threads[i])));
	}
	showProgress("Growing trees..", num_trees);
	for (auto &thread : threads) {
		thread.join();
	}

#ifdef R_BUILD
	if (aborted_threads > 0) {
		throw std::runtime_error("User interrupt.");
	}
#endif

	// Sum thread importances
	if (importance_mode == IMP_GINI || importance_mode == IMP_GINI_CORRECTED) {
		variable_importance.resize(num_independent_variables, 0);
		for (size_t i = 0; i < num_independent_variables; ++i) {
			for (uint j = 0; j < num_threads; ++j) {
				variable_importance[i] += variable_importance_threads[j][i];
			}
		}
		variable_importance_threads.clear();
	}

#endif

	// Divide importance by number of trees
	if (importance_mode == IMP_GINI || importance_mode == IMP_GINI_CORRECTED) {
		for (auto& v : variable_importance) {
			v /= num_trees;
		}
	}
}

void Forest::predict(Data* data)
{
	// Predict trees in multiple threads and join the threads with the main thread
#ifdef OLD_WIN_R_BUILD
	progress = 0;
	clock_t start_time = clock();
	clock_t lap_time = clock();
	for (size_t i = 0; i < num_trees; ++i) {
		trees[i]->predict(data, false);
		progress++;
		showProgress("Predicting..", start_time, lap_time);
	}
#else
	progress = 0;
#ifdef R_BUILD
	aborted = false;
	aborted_threads = 0;
#endif

	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	for (uint i = 0; i < num_threads; ++i) {
		threads.push_back(std::thread(&Forest::predictTreesInThread, this, i, data, false));
	}
	showProgress("Predicting..", num_trees);
	for (auto &thread : threads) {
		thread.join();
	}

#ifdef R_BUILD
	if (aborted_threads > 0) {
		throw std::runtime_error("User interrupt.");
	}
#endif
#endif

	// Call special functions for subclasses


	progress = 0;

	//std::vector<std::thread> threads;
	std::vector<uint> predict_ranges;
	equalSplit(predict_ranges, 0, uint(data->getNumRows() - 1), num_threads);

	threads.clear();
	threads.reserve(num_threads);
	for (uint i = 0; i < predict_ranges.size() - 1; ++i) {
		std::pair<uint, uint> range = std::make_pair(predict_ranges[i], predict_ranges[i + 1]);
		threads.push_back(std::thread(&Forest::predictInternalInThread, this, range, data));
	}
	showProgress("Aggregate prediction..", data->getNumRows());
	for (auto &thread : threads) {
		thread.join();
	}


	//for (size_t i = 0; i < data->getNumRows(); ++i) {
	//predictInternal(i, data);
	//}
	//predictInternal(data, predictions);
}

void Forest::predictInternalInThread(const std::pair<uint, uint>& range, const Data* data) {

	for (size_t i = range.first; i < range.second; ++i) {
		predictInternal(i, data);

		// Check for user interrupt
#ifdef R_BUILD
		if (aborted) {
			std::unique_lock<std::mutex> lock(mutex);
			++aborted_threads;
			condition_variable.notify_one();
			return;
		}
#endif

		// Increase progress by 1 tree
		std::unique_lock<std::mutex> lock(mutex);
		++progress;
		condition_variable.notify_one();
	}

}
//void Forest::predictInternalInThread(uint thread_idx) {
//	// Create thread ranges
//	std::vector<uint> predict_ranges;
//	equalSplit(predict_ranges, 0, num_samples - 1, num_threads);
//
//	if (predict_ranges.size() > thread_idx + 1) {
//		for (size_t i = predict_ranges[thread_idx]; i < predict_ranges[thread_idx + 1]; ++i) {
//			predictInternal(i);
//
//			// Check for user interrupt
//#ifdef R_BUILD
//			if (aborted) {
//				std::unique_lock<std::mutex> lock(mutex);
//				++aborted_threads;
//				condition_variable.notify_one();
//				return;
//		}
//#endif
//
//			// Increase progress by 1 tree
//			std::unique_lock<std::mutex> lock(mutex);
//			++progress;
//			condition_variable.notify_one();
//	}
//}
//}

void Forest::predict_no_thread(Data* data)
{
	for (size_t i = 0; i < trees.size(); ++i) {
		trees[i]->predict(data, false);
	}

	// Call special functions for subclasses
	for (size_t i = 0; i < data->getNumRows(); ++i) {
		predictInternal(i, data);
	}
}

void Forest::computePredictionError(Data* data) {

	// Predict trees in multiple threads
#ifdef OLD_WIN_R_BUILD
	progress = 0;
	clock_t start_time = clock();
	clock_t lap_time = clock();
	for (size_t i = 0; i < num_trees; ++i) {
		trees[i]->predict(data, true);
		progress++;
		showProgress("Predicting..", start_time, lap_time);
	}
#else
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	for (uint i = 0; i < num_threads; ++i) {
		threads.push_back(std::thread(&Forest::predictTreesInThread, this, i, data, true));
	}
	showProgress("Computing prediction error..", num_trees);
	for (auto &thread : threads) {
		thread.join();
	}

#ifdef R_BUILD
	if (aborted_threads > 0) {
		throw std::runtime_error("User interrupt.");
	}
#endif
#endif

	// Call special function for subclasses
	computePredictionErrorInternal(data);
}

void Forest::computePermutationImportance() {

	// Compute tree permutation importance in multiple threads
#ifdef OLD_WIN_R_BUILD
	progress = 0;
	clock_t start_time = clock();
	clock_t lap_time = clock();

	// Initailize importance and variance
	variable_importance.resize(num_independent_variables, 0);
	std::vector<double> variance;
	if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW) {
		variance.resize(num_independent_variables, 0);
	}

	// Compute importance
	for (size_t i = 0; i < num_trees; ++i) {
		trees[i]->computePermutationImportance(&variable_importance, &variance);
		progress++;
		showProgress("Computing permutation importance..", start_time, lap_time);
	}
#else
	progress = 0;
#ifdef R_BUILD
	aborted = false;
	aborted_threads = 0;
#endif

	std::vector<std::thread> threads;
	threads.reserve(num_threads);

	// Initailize importance and variance
	std::vector<std::vector<double>> variable_importance_threads(num_threads);
	std::vector<std::vector<double>> variance_threads(num_threads);

	// Compute importance
	for (uint i = 0; i < num_threads; ++i) {
		variable_importance_threads[i].resize(num_independent_variables, 0);
		if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW) {
			variance_threads[i].resize(num_independent_variables, 0);
		}
		threads.push_back(
			std::thread(&Forest::computeTreePermutationImportanceInThread, this, i, &(variable_importance_threads[i]),
				&(variance_threads[i])));
	}
	showProgress("Computing permutation importance..", num_trees);
	for (auto &thread : threads) {
		thread.join();
	}

#ifdef R_BUILD
	if (aborted_threads > 0) {
		throw std::runtime_error("User interrupt.");
	}
#endif

	// Sum thread importances
	variable_importance.resize(num_independent_variables, 0);
	for (size_t i = 0; i < num_independent_variables; ++i) {
		for (uint j = 0; j < num_threads; ++j) {
			variable_importance[i] += variable_importance_threads[j][i];
		}
	}
	variable_importance_threads.clear();

	// Sum thread variances
	std::vector<double> variance(num_independent_variables, 0);
	if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW) {
		for (size_t i = 0; i < num_independent_variables; ++i) {
			for (uint j = 0; j < num_threads; ++j) {
				variance[i] += variance_threads[j][i];
			}
		}
		variance_threads.clear();
	}
#endif

	for (size_t i = 0; i < variable_importance.size(); ++i) {
		variable_importance[i] /= num_trees;

		// Normalize by variance for scaled permutation importance
		if (importance_mode == IMP_PERM_BREIMAN || importance_mode == IMP_PERM_LIAW) {
			if (variance[i] != 0) {
				variance[i] = variance[i] / num_trees - variable_importance[i] * variable_importance[i];
				variable_importance[i] /= sqrt(variance[i] / num_trees);
			}
		}
	}
}

#ifndef OLD_WIN_R_BUILD
void Forest::growTreesInThread(uint thread_idx, std::vector<double>* variable_importance) {
	if (thread_ranges.size() > thread_idx + 1) {
		for (size_t i = thread_ranges[thread_idx]; i < thread_ranges[thread_idx + 1]; ++i) {
			trees[i]->grow(variable_importance);

			// Check for user interrupt
#ifdef R_BUILD
			if (aborted) {
				std::unique_lock<std::mutex> lock(mutex);
				++aborted_threads;
				condition_variable.notify_one();
				return;
			}
#endif

			// Increase progress by 1 tree
			std::unique_lock<std::mutex> lock(mutex);
			++progress;
			condition_variable.notify_one();
		}
	}
}

void Forest::predictTreesInThread(uint thread_idx, const Data* prediction_data, bool oob_prediction) {
	if (thread_ranges.size() > thread_idx + 1) {
		for (size_t i = thread_ranges[thread_idx]; i < thread_ranges[thread_idx + 1]; ++i) {
			trees[i]->predict(prediction_data, oob_prediction);

			// Check for user interrupt
#ifdef R_BUILD
			if (aborted) {
				std::unique_lock<std::mutex> lock(mutex);
				++aborted_threads;
				condition_variable.notify_one();
				return;
			}
#endif

			// Increase progress by 1 tree
			std::unique_lock<std::mutex> lock(mutex);
			++progress;
			condition_variable.notify_one();
		}
	}
}

void Forest::computeTreePermutationImportanceInThread(uint thread_idx, std::vector<double>* importance,
	std::vector<double>* variance) {
	if (thread_ranges.size() > thread_idx + 1) {
		for (size_t i = thread_ranges[thread_idx]; i < thread_ranges[thread_idx + 1]; ++i) {
			trees[i]->computePermutationImportance(importance, variance);

			// Check for user interrupt
#ifdef R_BUILD
			if (aborted) {
				std::unique_lock<std::mutex> lock(mutex);
				++aborted_threads;
				condition_variable.notify_one();
				return;
			}
#endif

			// Increase progress by 1 tree
			std::unique_lock<std::mutex> lock(mutex);
			++progress;
			condition_variable.notify_one();
		}
	}
}
#endif

// #nocov start
void Forest::loadFromFile(std::string filename) {
	if (verbose_out)
		*verbose_out << "Loading forest from file " << filename << "." << std::endl;

	// Open file for reading
	std::ifstream infile;
	infile.open(filename, std::ios::binary);
	if (!infile.good()) {
		throw std::runtime_error("Could not read from input file: " + filename + ".");
	}

	//read version
	size_t version = 0;
	infile.read((char*)&version, sizeof(version));
	if (version != FOREST_FILE_VERSION) {
		throw std::runtime_error("Forest was not create with the same version: " + filename + ".");
	}

	//read input columns
	readVector1D(initial_input_cols_name, infile);

	// Read dependent and status vars 
	infile.read((char*)&dependent_varID, sizeof(dependent_varID));
	readString(dependent_variable_name, infile);
	readString(status_var_name, infile);

	//read independant
	infile.read((char*)&num_independent_variables, sizeof(num_independent_variables));
	readVector1D(independent_variable_names, infile);

	//Read virtual columns 
	readString(virtual_cols_txt, infile);
	readVector1D(virtual_cols_name, infile);

	//read ignore and ordered
	readVector1D(ignore_cols, infile);
	readVector1D(training_is_ordered_variable, infile);

	// Read num_trees
	infile.read((char*)&num_trees, sizeof(num_trees));
	// Read tree data. This is different for tree types -> virtual function
	loadFromFileInternal(infile);

	infile.close();

	// Create thread ranges
	equalSplit(thread_ranges, 0, uint(num_trees - 1), num_threads);

	assert(!dependent_variable_name.empty());
}
// #nocov end

void Forest::setSplitWeightVector(Data* training, std::vector<std::vector<double>>& split_select_weights) {

	// Size should be 1 x num_independent_variables or num_trees x num_independent_variables
	if (split_select_weights.size() != 1 && split_select_weights.size() != num_trees) {
		throw std::runtime_error("Size of split select weights not equal to 1 or number of trees.");
	}

	// Reserve space
	if (split_select_weights.size() == 1) {
		this->split_select_weights[0].resize(num_independent_variables);
	}
	else {
		this->split_select_weights.clear();
		this->split_select_weights.resize(num_trees, std::vector<double>(num_independent_variables));
	}
	this->split_select_varIDs.resize(num_independent_variables);
	deterministic_varIDs.reserve(num_independent_variables);

	// Split up in deterministic and weighted variables, ignore zero weights
	for (size_t i = 0; i < split_select_weights.size(); ++i) {

		// Size should be 1 x num_independent_variables or num_trees x num_independent_variables
		if (split_select_weights[i].size() != num_independent_variables) {
			throw std::runtime_error("Number of split select weights not equal to number of independent variables.");
		}

		for (size_t j = 0; j < split_select_weights[i].size(); ++j) {
			double weight = split_select_weights[i][j];

			if (i == 0) {
				size_t varID = j;
				for (auto& skip : training->getNoSplitVariables()) {
					if (varID >= skip) {
						++varID;
					}
				}

				if (weight == 1) {
					deterministic_varIDs.push_back(varID);
				}
				else if (weight < 1 && weight > 0) {
					this->split_select_varIDs[j] = varID;
					this->split_select_weights[i][j] = weight;
				}
				else if (weight < 0 || weight > 1) {
					throw std::runtime_error("One or more split select weights not in range [0,1].");
				}

			}
			else {
				if (weight < 1 && weight > 0) {
					this->split_select_weights[i][j] = weight;
				}
				else if (weight < 0 || weight > 1) {
					throw std::runtime_error("One or more split select weights not in range [0,1].");
				}
			}
		}
	}

	if (deterministic_varIDs.size() > this->mtry) {
		throw std::runtime_error("Number of ones in split select weights cannot be larger than mtry.");
	}
	if (deterministic_varIDs.size() + split_select_varIDs.size() < mtry) {
		throw std::runtime_error("Too many zeros in split select weights. Need at least mtry variables to split at.");
	}
}

void Forest::setAlwaysSplitVariables(Data* training, std::vector<std::string>& always_split_variable_names) {

	deterministic_varIDs.reserve(num_independent_variables);


	for (auto& variable_name : always_split_variable_names) {
		size_t varID = training->getVariableID(variable_name);
		if (varID >= training->getNumCols())
		{
			throw std::runtime_error("Split variable " + variable_name + " not found.");
		}
		deterministic_varIDs.push_back(varID);
	}

	if (deterministic_varIDs.size() + this->mtry > num_independent_variables) {
		throw std::runtime_error(
			"Number of variables to be always considered for splitting plus mtry cannot be larger than number of independent variables.");
	}
}


#ifdef OLD_WIN_R_BUILD
void Forest::showProgress(std::string operation, clock_t start_time, clock_t& lap_time) {

	// Check for user interrupt
	if (checkInterrupt()) {
		throw std::runtime_error("User interrupt.");
	}

	double elapsed_time = (clock() - lap_time) / CLOCKS_PER_SEC;
	if (elapsed_time > STATUS_INTERVAL) {
		double relative_progress = (double)progress / (double)num_trees;
		double time_from_start = (clock() - start_time) / CLOCKS_PER_SEC;
		uint remaining_time = (1 / relative_progress - 1) * time_from_start;
		if (verbose_out)
			*verbose_out << operation << " Progress: " << round(100 * relative_progress) << "%. Estimated remaining time: " << beautifyTime(remaining_time) << "." << std::endl;
		lap_time = clock();
	}
}
#else
void Forest::showProgress(std::string operation, size_t maxProgress) {
	using std::chrono::steady_clock;
	using std::chrono::duration_cast;
	using std::chrono::seconds;

	steady_clock::time_point start_time = steady_clock::now();
	steady_clock::time_point last_time = steady_clock::now();
	std::unique_lock<std::mutex> lock(mutex);

	// Wait for message from threads and show output if enough time elapsed
	while (progress < maxProgress) {
		condition_variable.wait(lock);
		seconds elapsed_time = duration_cast<seconds>(steady_clock::now() - last_time);

		// Check for user interrupt
#ifdef R_BUILD
		if (!aborted && checkInterrupt()) {
			aborted = true;
		}
		if (aborted && aborted_threads >= num_threads) {
			return;
		}
#endif

		if (progress > 0 && elapsed_time.count() > STATUS_INTERVAL) {
			double relative_progress = (double)progress / (double)maxProgress;
			seconds time_from_start = duration_cast<seconds>(steady_clock::now() - start_time);
			uint remaining_time = (uint)((1 / relative_progress - 1) * time_from_start.count());
			if (verbose_out)
				*verbose_out << operation << " Progress: " << round(100 * relative_progress) << "%. Estimated remaining time: " << beautifyTime(remaining_time) << "." << std::endl;
			last_time = steady_clock::now();
		}
	}
}
#endif
