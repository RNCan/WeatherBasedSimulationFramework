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

#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <random>
#include <stdexcept>
#include <cmath>
#include <string>

#include "RangerLib/Utility/utility.h"
#include "RangerLib/Forest/ForestClassification.h"
#include "RangerLib/Tree/TreeClassification.h"
#include "RangerLib/Utility/Data.h"

ForestClassification::ForestClassification() {
}

ForestClassification::~ForestClassification() {
}

void ForestClassification::loadForest(size_t dependent_varID, size_t num_trees,
	std::vector<std::vector<std::vector<size_t>> >& forest_child_nodeIDs,
	std::vector<std::vector<size_t>>& forest_split_varIDs, std::vector<std::vector<double>>& forest_split_values,
	std::vector<double>& class_values/*, std::vector<bool>& is_ordered_variable*/) {

	this->dependent_varID = dependent_varID;
	this->num_trees = num_trees;
	this->class_values = class_values;
	//data->setIsOrderedVariable(is_ordered_variable);

	// Create trees
	trees.reserve(num_trees);
	for (size_t i = 0; i < num_trees; ++i) {
		Tree* tree = new TreeClassification(forest_child_nodeIDs[i], forest_split_varIDs[i], forest_split_values[i],
			&this->class_values, &response_classIDs);
		trees.push_back(tree);
	}

	// Create thread ranges
	equalSplit(thread_ranges, 0, uint(num_trees - 1), num_threads);
}

void ForestClassification::init_internal_grow(Data* data) {

	// If mtry not set, use floored square root of number of independent variables.
	if (mtry == 0) {
		unsigned long temp = (unsigned long)sqrt((double)(data->getNumCols() - 1));
		mtry = std::max((unsigned long)1, temp);
	}

	// Set minimal node size
	if (min_node_size == 0) {
		min_node_size = DEFAULT_MIN_NODE_SIZE_CLASSIFICATION;
	}

	// Create class_values and response_classIDs
	if (!prediction_mode) {
		for (size_t i = 0; i < data->getNumRows(); ++i) {
			double value = data->get(i, dependent_varID);

			// If classID is already in class_values, use ID. Else create a new one.
			uint classID = (uint)(find(class_values.begin(), class_values.end(), value) - class_values.begin());
			if (classID == class_values.size()) {
				class_values.push_back(value);
			}
			response_classIDs.push_back(classID);
		}
	}

	// Sort data if memory saving mode
	if (!memory_saving_splitting) {
		data->sort();
	}
}


void ForestClassification::growInternal(Data* data) {
	trees.reserve(num_trees);
	for (size_t i = 0; i < num_trees; ++i) {
		trees.push_back(new TreeClassification(&class_values, &response_classIDs));
	}
}

void ForestClassification::init_internal_predict(const Data* data) {
	size_t num_prediction_samples = data->getNumRows();
	if (predict_all || prediction_type == TERMINALNODES) {
		predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(num_prediction_samples, std::vector<double>(num_trees)));
	}
	else {
		predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(1, std::vector<double>(num_prediction_samples)));
	}

	uncertainty = std::vector<double>(std::vector<double>(num_prediction_samples));
}

void ForestClassification::predictInternal(size_t sample_idx, const Data* data) {

	if (predict_all || prediction_type == TERMINALNODES) {
		// Get all tree predictions
		for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
			if (prediction_type == TERMINALNODES) {
				predictions[0][sample_idx][tree_idx] = (double)((TreeClassification*)trees[tree_idx])->getPredictionTerminalNodeID(sample_idx);
			}
			else {
				predictions[0][sample_idx][tree_idx] = ((TreeClassification*)trees[tree_idx])->getPrediction(sample_idx);
			}
		}
	}
	else {
		// Count classes over trees and save class with maximum count
		std::unordered_map<double, size_t> class_count;
		for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
			double value = ((TreeClassification*)trees[tree_idx])->getPrediction(sample_idx);
			++class_count[value];
		}
		predictions[0][0][sample_idx] = mostFrequentValue(class_count, random_number_generator);

		if (class_count.size() < 2)
		{
			uncertainty[sample_idx] = 100;
		}
		else
		{
			std::vector< size_t > result;
			result.reserve(class_count.size());
			for (std::unordered_map< double, size_t>::const_iterator it = class_count.begin(); it != class_count.end(); ++it)
				result.push_back(it->second);

			std::partial_sort(result.begin(), result.begin() + 2, result.end(), std::greater<size_t>());
			uncertainty[sample_idx] = 100 - (((double)result[1] / result[0]) * 100);
		}
	}
}

double ForestClassification::getPredictions(size_t sample_idx, size_t time_point) const { return predictions[0][0][sample_idx]; }
double ForestClassification::getUncertainty(size_t sample_idx) const { return uncertainty[sample_idx]; }

void ForestClassification::computePredictionErrorInternal(Data* data) {

	// Class counts for samples
	std::vector<std::unordered_map<double, size_t>> class_counts;
	class_counts.reserve(data->getNumRows());
	for (size_t i = 0; i < data->getNumRows(); ++i) {
		class_counts.push_back(std::unordered_map<double, size_t>());
	}

	// For each tree loop over OOB samples and count classes
	for (size_t tree_idx = 0; tree_idx < num_trees; ++tree_idx) {
		for (size_t sample_idx = 0; sample_idx < trees[tree_idx]->getNumSamplesOob(); ++sample_idx) {
			size_t sampleID = trees[tree_idx]->getOobSampleIDs()[sample_idx];
			double value = ((TreeClassification*)trees[tree_idx])->getPrediction(sample_idx);
			++class_counts[sampleID][value];
		}
	}

	// Compute majority vote for each sample
	predictions = std::vector<std::vector<std::vector<double>>>(1, std::vector<std::vector<double>>(1, std::vector<double>(data->getNumRows())));
	for (size_t i = 0; i < data->getNumRows(); ++i) {
		if (!class_counts[i].empty()) {
			predictions[0][0][i] = mostFrequentValue(class_counts[i], random_number_generator);
		}
		else {
			predictions[0][0][i] = NAN;
		}
	}

	// Compare predictions with true data
	size_t num_missclassifications = 0;
	size_t num_predictions = 0;
	for (size_t i = 0; i < predictions[0][0].size(); ++i) {
		double predicted_value = predictions[0][0][i];
		if (!std::isnan(predicted_value)) {
			++num_predictions;
			double real_value = data->get(i, dependent_varID);
			if (predicted_value != real_value) {
				++num_missclassifications;
			}
			++classification_table[std::make_pair(real_value, predicted_value)];
		}
	}
	overall_prediction_error = (double)num_missclassifications / (double)num_predictions;
}

// #nocov start
void ForestClassification::writeOutputInternal() {
	if (verbose_out)
	{
		//*verbose_out << "Tree type:                         " << "Classification" << std::endl;
		std::string str;
		for (const auto &piece : class_values) 
			str += std::to_string(int(piece)) + " ";

		*verbose_out << "Classes:                           " << str << std::endl;
	}
}

void ForestClassification::writeConfusionFile(std::string filename) {

	// Open confusion file for writing
	//std::string filename = output_prefix + ".confusion";
	std::ofstream outfile;
	outfile.open(filename, std::ios::out);
	if (!outfile.good()) {
		throw std::runtime_error("Could not write to confusion file: " + filename + ".");
	}

	// Write confusion to file
	outfile << "Overall OOB prediction error (Fraction missclassified): " << overall_prediction_error << std::endl;
	outfile << std::endl;
	outfile << "Class specific prediction errors:" << std::endl;
	outfile << "           ";
	for (auto& class_value : class_values) {
		outfile << "     " << class_value;
	}
	outfile << std::endl;
	for (auto& predicted_value : class_values) {
		outfile << "predicted " << predicted_value << "     ";
		for (auto& real_value : class_values) {
			size_t value = classification_table[std::make_pair(real_value, predicted_value)];
			outfile << value;
			if (value < 10) {
				outfile << "     ";
			}
			else if (value < 100) {
				outfile << "    ";
			}
			else if (value < 1000) {
				outfile << "   ";
			}
			else if (value < 10000) {
				outfile << "  ";
			}
			else if (value < 100000) {
				outfile << " ";
			}
		}
		outfile << std::endl;
	}

	outfile.close();
	if (verbose_out)
		*verbose_out << "Saved confusion matrix to file " << filename << "." << std::endl;
}

void ForestClassification::writePredictionFile(std::string filename) {

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

void ForestClassification::saveToFileInternal(std::ofstream& outfile) {

	// Write num_variables
	size_t num_variables = training_is_ordered_variable.size();// training->getNumCols();
	outfile.write((char*)&num_variables, sizeof(num_variables));

	// Write treetype
	TreeType treetype = TREE_CLASSIFICATION;
	outfile.write((char*)&treetype, sizeof(treetype));

	// Write class_values
	saveVector1D(class_values, outfile);
}

void ForestClassification::loadFromFileInternal(std::ifstream& infile) {

	// Read number of variables
	size_t num_variables_saved;
	infile.read((char*)&num_variables_saved, sizeof(num_variables_saved));
	//num_independent_variables = num_variables_saved - 1; //add by RSA
	// Read treetype
	TreeType treetype;
	infile.read((char*)&treetype, sizeof(treetype));
	if (treetype != TREE_CLASSIFICATION) {
		throw std::runtime_error("Wrong treetype. Loaded file is not a classification forest.");
	}

	// Read class_values
	readVector1D(class_values, infile);

	for (size_t i = 0; i < num_trees; ++i) {

		// Read data
		std::vector<std::vector<size_t>> child_nodeIDs;
		readVector2D(child_nodeIDs, infile);
		std::vector<size_t> split_varIDs;
		readVector1D(split_varIDs, infile);
		std::vector<double> split_values;
		readVector1D(split_values, infile);

		// If dependent variable in test data, change variable IDs accordingly
		for (auto& varID : split_varIDs) {
			if (varID >= dependent_varID) {
				--varID;
			}
		}
		

		// Create tree
		Tree* tree = new TreeClassification(child_nodeIDs, split_varIDs, split_values, &class_values, &response_classIDs);
		trees.push_back(tree);
	}
}


// #nocov end
