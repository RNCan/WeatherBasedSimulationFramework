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

#include <algorithm>
#include <stdexcept>
#include <string>

#include "RangerLib/utility/utility.h"
#include "RangerLib/Forest/ForestRegression.h"
#include "RangerLib/Tree/TreeRegression.h"
#include "RangerLib/utility/Data.h"

ForestRegression::ForestRegression() {
}

ForestRegression::~ForestRegression() {
}

void ForestRegression::loadForest(size_t dependent_varID, size_t num_trees,
	std::vector<std::vector<std::vector<size_t>> >& forest_child_nodeIDs,
	std::vector<std::vector<size_t>>& forest_split_varIDs, std::vector<std::vector<double>>& forest_split_values/*,
	std::vector<bool>& is_ordered_variable*/) {

	this->dependent_varID = dependent_varID;
	this->num_trees = num_trees;
	//data->setIsOrderedVariable(is_ordered_variable);

	// Create trees
	trees.reserve(num_trees);
	for (size_t i = 0; i < num_trees; ++i) {
		Tree* tree = new TreeRegression(forest_child_nodeIDs[i], forest_split_varIDs[i], forest_split_values[i]);
		trees.push_back(tree);
	}

	// Create thread ranges
	equalSplit(thread_ranges, 0, uint(num_trees - 1), num_threads);
}

void ForestRegression::initInternal(Data* data, std::string status_variable_name) {

	// If mtry not set, use floored square root of number of independent variables
	if (mtry == 0) {
		unsigned long temp = (unsigned long)sqrt((double)(data->getNumCols() - 1));
		mtry = std::max((unsigned long)1, temp);
	}

	// Set minimal node size
	if (min_node_size == 0) {
		min_node_size = DEFAULT_MIN_NODE_SIZE_REGRESSION;
	}

	// Sort data if memory saving mode
	if (!memory_saving_splitting) {
		data->sort();
	}
}

void ForestRegression::growInternal(Data* data) {
	trees.reserve(num_trees);
	for (size_t i = 0; i < num_trees; ++i) {
		trees.push_back(new TreeRegression());
	}
}

void ForestRegression::allocatePredictMemory(const Data* data) {
	size_t num_prediction_samples = data->getNumRows();
	if (predict_all || prediction_type == TERMINALNODES) {
		predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(num_prediction_samples, std::vector<double>(num_trees)));
	}
	else {
		predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(1, std::vector<double>(num_prediction_samples)));
	}

	uncertainty = std::vector<double>(std::vector<double>(num_prediction_samples));
}

void ForestRegression::predictInternal(size_t sample_idx, const Data* data) {

	/*size_t num_prediction_samples = data->getNumRows();
	if (predict_all || prediction_type == TERMINALNODES) {
	predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(num_prediction_samples, std::vector<double>(num_trees)));
	} else {
	predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(1, std::vector<double>(num_prediction_samples)));
	}
	*/
	// For all samples get tree predictions
	//for (size_t sample_idx = 0; sample_idx < num_prediction_samples; ++sample_idx) {

	if (predict_all || prediction_type == TERMINALNODES) {
		// Get all tree predictions
		for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
			if (prediction_type == TERMINALNODES) {
				predictions[0][sample_idx][tree_idx] = (double)(((TreeRegression*)trees[tree_idx])->getPredictionTerminalNodeID(sample_idx));
			}
			else {
				predictions[0][sample_idx][tree_idx] = ((TreeRegression*)trees[tree_idx])->getPrediction(sample_idx);
			}
		}
	}
	else {
		// Mean over trees
		double sum = 0;
		double sum² = 0;
		for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
			double pred = ((TreeRegression*)trees[tree_idx])->getPrediction(sample_idx);
			sum += pred;
			sum² += pred * pred;
		}
		predictions[0][0][sample_idx] = sum / num_trees;
		uncertainty[sample_idx] = sqrt(std::max(0.0, (sum² - ((sum*sum) / num_trees)) / num_trees)) / (sum / num_trees);
	}
	//}
}

void ForestRegression::computePredictionErrorInternal(Data* data) {

	// For each sample sum over trees where sample is OOB
	std::vector<size_t> samples_oob_count;
	predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(1, std::vector<double>(data->getNumRows(), 0)));
	samples_oob_count.resize(data->getNumRows(), 0);
	for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
		for (size_t sample_idx = 0; sample_idx < trees[tree_idx]->getNumSamplesOob(); ++sample_idx) {
			size_t sampleID = trees[tree_idx]->getOobSampleIDs()[sample_idx];
			double value = ((TreeRegression*)trees[tree_idx])->getPrediction(sample_idx);

			predictions[0][0][sampleID] += value;
			++samples_oob_count[sampleID];
		}
	}

	// MSE with predictions and true data
	size_t num_predictions = 0;
	for (size_t i = 0; i < predictions[0][0].size(); ++i) {
		if (samples_oob_count[i] > 0) {
			++num_predictions;
			predictions[0][0][i] /= (double)samples_oob_count[i];
			double predicted_value = predictions[0][0][i];
			double real_value = data->get(i, dependent_varID);
			overall_prediction_error += (predicted_value - real_value) * (predicted_value - real_value);
		}
		else {
			predictions[0][0][i] = NAN;
		}
	}

	overall_prediction_error /= (double)num_predictions;
}

// #nocov start
void ForestRegression::writeOutputInternal() {
	if (verbose_out)
		*verbose_out << "Tree type:                         " << "Regression" << std::endl;
}

void ForestRegression::writeConfusionFile(std::string filename) {

	// Open confusion file for writing
	//std::string filename = output_prefix + ".confusion";
	std::ofstream outfile;
	outfile.open(filename, std::ios::out);
	if (!outfile.good()) {
		throw std::runtime_error("Could not write to confusion file: " + filename + ".");
	}

	// Write confusion to file
	outfile << "Overall OOB prediction error (MSE): " << overall_prediction_error << std::endl;

	outfile.close();
	if (verbose_out)
		*verbose_out << "Saved prediction error to file " << filename << "." << std::endl;
}

void ForestRegression::writePredictionFile(std::string filename) {

	// Open prediction file for writing
	//std::string filename = output_prefix + ".prediction";
	std::ofstream outfile;
	outfile.open(filename, std::ios::out);
	if (!outfile.good()) {
		throw std::runtime_error("Could not write to prediction file: " + filename + ".");
	}

	// Write
	outfile << "Predictions: " << std::endl;
	if (predict_all) {
		for (size_t k = 0; k < num_trees; ++k) {
			outfile << "Tree " << k << ":" << std::endl;
			for (size_t i = 0; i < predictions.size(); ++i) {
				for (size_t j = 0; j < predictions[i].size(); ++j) {
					outfile << predictions[i][j][k] << std::endl;
				}
			}
			outfile << std::endl;
		}
	}
	else {
		for (size_t i = 0; i < predictions.size(); ++i) {
			for (size_t j = 0; j < predictions[i].size(); ++j) {
				for (size_t k = 0; k < predictions[i][j].size(); ++k) {
					outfile << predictions[i][j][k] << std::endl;
				}
			}
		}
	}

	if (verbose_out)
		*verbose_out << "Saved predictions to file " << filename << "." << std::endl;
}

void ForestRegression::saveToFileInternal(std::ofstream& outfile) {

	// Write num_variables
	size_t num_variables = training_is_ordered_variable.size(); // training->getNumRows();
	outfile.write((char*)&num_variables, sizeof(num_variables));

	// Write treetype
	TreeType treetype = TREE_REGRESSION;
	outfile.write((char*)&treetype, sizeof(treetype));
}

void ForestRegression::loadFromFileInternal(std::ifstream& infile) {

	// Read number of variables
	size_t num_variables_saved;
	infile.read((char*)&num_variables_saved, sizeof(num_variables_saved));
	num_independent_variables = num_variables_saved - 1; //add by RSA
	// Read treetype
	TreeType treetype;
	infile.read((char*)&treetype, sizeof(treetype));
	if (treetype != TREE_REGRESSION) {
		throw std::runtime_error("Wrong treetype. Loaded file is not a regression forest.");
	}

	for (size_t i = 0; i < num_trees; ++i) {

		// Read data
		std::vector<std::vector<size_t>> child_nodeIDs;
		readVector2D(child_nodeIDs, infile);
		std::vector<size_t> split_varIDs;
		readVector1D(split_varIDs, infile);
		std::vector<double> split_values;
		readVector1D(split_values, infile);

		// If dependent variable not in test data, change variable IDs accordingly
		//always assume that input file don't have dependent variable
		//if (num_variables_saved > num_variables) {
		for (auto& varID : split_varIDs) {
			if (varID >= dependent_varID) {
				--varID;
			}
		}
		// }

		// Create tree
		Tree* tree = new TreeRegression(child_nodeIDs, split_varIDs, split_values);
		trees.push_back(tree);
	}
}
// #nocov end
