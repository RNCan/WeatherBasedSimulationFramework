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

#ifndef FOREST_H_
#define FOREST_H_

#include <vector>
#include <iostream>
#include <random>
#include <ctime>
#ifndef OLD_WIN_R_BUILD
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#endif

#include "globals.h"
#include "Tree/Tree.h"
#include "Utility/Data.h"

inline TreeType GetTreeType(std::string forest_file_path)
{
	TreeType treetype;
	if (forest_file_path.find(GetTreeTypeStr(TREE_CLASSIFICATION))!=std::string::npos)
		treetype = TREE_CLASSIFICATION;
	else if (forest_file_path.find(GetTreeTypeStr(TREE_REGRESSION)) != std::string::npos)
		treetype = TREE_REGRESSION;
	else if (forest_file_path.find(GetTreeTypeStr(TREE_SURVIVAL)) != std::string::npos)
		treetype = TREE_SURVIVAL;
	else if (forest_file_path.find(GetTreeTypeStr(TREE_PROBABILITY)) != std::string::npos)
		treetype = TREE_PROBABILITY;

	return treetype;
}

class Forest {
public:

	static Data* Forest::CreateMemory(MemoryMode memory_mode);

  Forest();
  virtual ~Forest();

  // Init from c++ main or Rcpp from R
  Data* initCpp_grow(std::string dependent_variable_name, MemoryMode memory_mode,/*Data* data,*/ std::string input_file, uint mtry,
      uint num_trees, std::ostream* verbose_out, uint seed, uint num_threads,
      /*std::string load_forest_filename,*/ ImportanceMode importance_mode, uint min_node_size,
      std::string split_select_weights_file, std::vector<std::string>& always_split_variable_names,
      std::string status_variable_name, bool sample_with_replacement,
      std::vector<std::string>& unordered_variable_names, bool memory_saving_splitting, SplitRule splitrule,
      std::string case_weights_file,/* bool predict_all,*/ double sample_fraction, double alpha, double minprop,
      bool holdout, /*PredictionType prediction_type,*/ uint num_random_splits);
  //void initR(std::string dependent_variable_name, Data* input_data, uint mtry, uint num_trees,
  //    std::ostream* verbose_out, uint seed, uint num_threads, ImportanceMode importance_mode, uint min_node_size,
  //    std::vector<std::vector<double>>& split_select_weights, std::vector<std::string>& always_split_variable_names,
  //    /*std::string status_variable_name,*/ bool prediction_mode, bool sample_with_replacement,
  //    std::vector<std::string>& unordered_variable_names, bool memory_saving_splitting, SplitRule splitrule,
  //    std::vector<double>& case_weights, bool predict_all, bool keep_inbag, double sample_fraction, double alpha,
  //    double minprop, bool holdout, PredictionType prediction_type, uint num_random_splits);
  void init_grow(std::string dependent_variable_name, Data* training, uint mtry,
      uint num_trees, uint seed, uint num_threads, ImportanceMode importance_mode,
      uint min_node_size, std::string status_variable_name/*, bool prediction_mode*/, bool sample_with_replacement,
      std::vector<std::string>& unordered_variable_names, bool memory_saving_splitting, SplitRule splitrule,
      /*bool predict_all,*/ double sample_fraction, double alpha, double minprop, bool holdout,
     /* PredictionType prediction_type,*/ uint num_random_splits);

  Data* initCpp_predict(MemoryMode memory_mode, std::string input_file, /*uint num_trees, */std::ostream* verbose_out, uint seed, uint num_threads,
	  std::string load_forest_filename, bool predict_all, PredictionType prediction_type);
  void init_predict(uint seed, uint num_threads, bool predict_all, PredictionType prediction_type);

  virtual void initInternal(Data* data, std::string status_variable_name) = 0;
  //virtual void initInternalData(Data* data, std::string status_variable_name) = 0;
  
  //void init_data(Data* data, std::string dependent_variable_name, std::string status_variable_name, std::vector<std::string>& unordered_variable_names);

  // Grow or predict
  //void run(Data* data);
  void run_grow(Data* data);
  void run_predict(Data* data);
  
  void set_verbose(std::ostream* verbose_out = NULL){ this->verbose_out = verbose_out; }
  // Write results to output files
  void writeOutput(Data* training, std::string output_prefix);
  virtual void writeOutputInternal() = 0;
  virtual void writeConfusionFile(std::string filename) = 0;
  virtual void writePredictionFile(std::string filename) = 0;
  void writeImportanceFile(Data* training, std::string filename);

  // Load forest from file
  void loadFromFile(std::string filename);
  // Save forest to file
  void saveToFile(std::string filename);
  

  std::vector<std::vector<std::vector<size_t>>>getChildNodeIDs() {
    std::vector<std::vector<std::vector<size_t>>> result;
    for (auto& tree : trees) {
      result.push_back(tree->getChildNodeIDs());
    }
    return result;
  }
  std::vector<std::vector<size_t>> getSplitVarIDs() {
    std::vector<std::vector<size_t>> result;
    for (auto& tree : trees) {
      result.push_back(tree->getSplitVarIDs());
    }
    return result;
  }
  std::vector<std::vector<double>> getSplitValues() {
    std::vector<std::vector<double>> result;
    for (auto& tree : trees) {
      result.push_back(tree->getSplitValues());
    }
    return result;
  }
  const std::vector<double>& getVariableImportance() const {
    return variable_importance;
  }
  double getOverallPredictionError() const {
    return overall_prediction_error;
  }
  const std::vector<std::vector<std::vector<double>> >& getPredictions() const {
    return predictions;
  }
  size_t getDependentVarId() const {
    return dependent_varID;
  }
  size_t getNumTrees() const {
    return num_trees;
  }
  uint getMtry() const {
    return mtry;
  }
  uint getMinNodeSize() const {
    return min_node_size;
  }
  size_t getNumIndependentVariables() const {
    return num_independent_variables;
  }

  /*const std::vector<bool>& getIsOrderedVariable() const {
    return data->getIsOrderedVariable();
  }*/

  std::vector<std::vector<size_t>> getInbagCounts() const {
    std::vector<std::vector<size_t>> result;
    for (auto& tree : trees) {
      result.push_back(tree->getInbagCounts());
    }
    return result;
  }

protected:
	
	void grow(Data* data);
	void predict(Data* data);
	//void predict_no_thread(Data* data);
	virtual void growInternal(Data* training) = 0;
  // Predict using existing tree from file and data as prediction data
	virtual void predictInternal(size_t sample_idx, const Data* data) = 0;
	virtual void allocatePredictMemory(const Data* data)=0;
	void computePredictionError(Data* data);
	virtual void computePredictionErrorInternal(Data* data) = 0;

  void computePermutationImportance();

  // Multithreading methods for growing/prediction/importance, called by each thread
  void growTreesInThread(uint thread_idx, std::vector<double>* variable_importance);
  void predictTreesInThread(uint thread_idx, const Data* prediction_data, bool oob_prediction);
  void computeTreePermutationImportanceInThread(uint thread_idx, std::vector<double>* importance, std::vector<double>* variance);
  void predictInternalInThread(const std::pair<uint, uint>& range, const Data* prediction_data);

  virtual void loadFromFileInternal(std::ifstream& infile) = 0;
  virtual void saveToFileInternal(std::ofstream& outfile) = 0;
  

  // Set split select weights and variables to be always considered for splitting
  void setSplitWeightVector(Data* training, std::vector<std::vector<double>>& split_select_weights);
  void setAlwaysSplitVariables(Data* training, std::vector<std::string>& always_split_variable_names);

  // Show progress every few seconds
#ifdef OLD_WIN_R_BUILD
  void showProgress(std::string operation, clock_t start_time, clock_t& lap_time);
#else
  void showProgress(std::string operation);
#endif

  // Verbose output stream, cout if verbose==true, logfile if not
  std::ostream* verbose_out;

  size_t num_trees;
  uint mtry;
  uint min_node_size;
  //size_t num_variables;
  size_t num_independent_variables;
  //size_t status_varID;//for survival
  uint seed;
  size_t dependent_varID;
  //size_t num_samples;
  bool prediction_mode;
 // MemoryMode memory_mode;
  bool sample_with_replacement;
  bool memory_saving_splitting;
  SplitRule splitrule;
  bool predict_all;
  bool keep_inbag;
  double sample_fraction;
  bool holdout;
  PredictionType prediction_type;
  uint num_random_splits;

  // MAXSTAT splitrule
  double alpha;
  double minprop;

  // Multithreading
  uint num_threads;
  std::vector<uint> thread_ranges;
#ifndef OLD_WIN_R_BUILD
  std::mutex mutex;
  std::condition_variable condition_variable;
#endif

  std::vector<Tree*> trees;
  std::vector<bool> training_is_ordered_variable;
  //std::vector<size_t> training_no_split_variables;
  //size_t training_variableID;
  
  //Data* data;
  //Data* training;

  std::vector<std::vector<std::vector<double>>> predictions;
  double overall_prediction_error;

  // Weight vector for selecting possible split variables, one weight between 0 (never select) and 1 (always select) for each variable
  // Deterministic variables are always selected
  std::vector<size_t> deterministic_varIDs;
  std::vector<size_t> split_select_varIDs;
  std::vector<std::vector<double>> split_select_weights;

  // Bootstrap weights
  std::vector<double> case_weights;

  // Random number generator
  std::mt19937_64 random_number_generator;

  //std::string output_prefix;
  ImportanceMode importance_mode;

  // Variable importance for all variables in forest
  std::vector<double> variable_importance;

  // Computation progress (finished trees)
  size_t progress;
#ifdef R_BUILD
  size_t aborted_threads;
  bool aborted;
#endif

private:
  DISALLOW_COPY_AND_ASSIGN(Forest);
};

#endif /* FOREST_H_ */
