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

#ifndef FORESTPROBABILITY_H_
#define FORESTPROBABILITY_H_

#include <map>
#include <utility>
#include <vector>

#include "RangerLib/globals.h"
#include "RangerLib/Forest/Forest.h"
#include "RangerLib/Tree/TreeProbability.h"

class ForestProbability : public Forest {
public:
	ForestProbability();
	virtual ~ForestProbability();

	void loadForest(size_t dependent_varID, size_t num_trees,
		std::vector<std::vector<std::vector<size_t>> >& forest_child_nodeIDs,
		std::vector<std::vector<size_t>>& forest_split_varIDs, std::vector<std::vector<double>>& forest_split_values,
		std::vector<double>& class_values, std::vector<std::vector<std::vector<double>>>& forest_terminal_class_counts/*, std::vector<bool>& is_ordered_variable*/);

	std::vector<std::vector<std::vector<double>>> getTerminalClassCounts() {
		std::vector<std::vector<std::vector<double>>> result;
		result.reserve(num_trees);
		for (Tree* tree : trees) {
			TreeProbability* temp = (TreeProbability*)tree;
			result.push_back(temp->getTerminalClassCounts());
		}
		return result;
	}

	const std::vector<double>& getClassValues() const {
		return class_values;
	}

protected:
	virtual void initInternal(Data* data, std::string status_variable_name);
	//  virtual void initInternalData(Data* data, std::string status_variable_name);
	virtual void growInternal(Data* data);
	virtual void predictInternal(size_t sample_idx, const Data* data);
	virtual void allocatePredictMemory(const Data* data);
	virtual void computePredictionErrorInternal(Data* data);
	virtual void writeOutputInternal();
	virtual void writeConfusionFile(std::string filename);
	virtual void writePredictionFile(std::string filename);
	virtual void saveToFileInternal(std::ofstream& outfile);
	virtual void loadFromFileInternal(std::ifstream& infile);
	
	virtual double getPredictions(size_t sample_idx, size_t time_point=-1) const override;
	virtual double getProbability(size_t sample_idx, size_t the_class) const override;
	virtual double getUncertainty(size_t sample_idx) const override;

	// Classes of the dependent variable and classIDs for responses
	std::vector<double> class_values;
	std::vector<uint> response_classIDs;
	std::vector<double> classification;

	// Table with classifications and true classes
	std::map<std::pair<double, double>, size_t> classification_table;

private:
	DISALLOW_COPY_AND_ASSIGN(ForestProbability);
};

#endif /* FORESTPROBABILITY_H_ */
