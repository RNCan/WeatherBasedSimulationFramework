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

#ifndef FORESTSURVIVAL_H_
#define FORESTSURVIVAL_H_

#include <iostream>
#include <vector>

#include "RangerLib/globals.h"
#include "RangerLib/Forest/Forest.h"
#include "RangerLib/Tree/TreeSurvival.h"

class ForestSurvival : public Forest {
public:
	ForestSurvival();
	virtual ~ForestSurvival();

	void loadForest(size_t dependent_varID, size_t num_trees,
		std::vector<std::vector<std::vector<size_t>> >& forest_child_nodeIDs,
		std::vector<std::vector<size_t>>& forest_split_varIDs, std::vector<std::vector<double>>& forest_split_values,
		size_t status_varID, std::vector<std::vector<std::vector<double>> >& forest_chf,
		std::vector<double>& unique_timepoints/*, std::vector<bool>& is_ordered_variable*/);

	std::vector<std::vector<std::vector<double>>>getChf() {
		std::vector<std::vector<std::vector<double>>> result;
		result.reserve(num_trees);
		for (Tree* tree : trees) {
			TreeSurvival* temp = (TreeSurvival*)tree;
			result.push_back(temp->getChf());
		}
		return result;
	}
	size_t getStatusVarId() const {
		return status_varID;
	}
	const std::vector<double>& getUniqueTimepoints() const {
		return unique_timepoints;
	}

	

private:
	virtual void init_internal_grow(Data* training)override;
	virtual void init_internal_predict(const Data* data)override;

	virtual void growInternal(Data* data);
	virtual void predictInternal(size_t sample_idx, const Data* data);
	
	virtual void computePredictionErrorInternal(Data* data);
	virtual void writeOutputInternal();
	virtual void writeConfusionFile(std::string filename);
	virtual void writePredictionFile(std::string filename);
	virtual void saveToFileInternal(std::ofstream& outfile);
	virtual void loadFromFileInternal(std::ifstream& infile);
	
	virtual double getPredictions(size_t sample_idx, size_t time_point=-1) const override;
	virtual double getUncertainty(size_t sample_idx) const override;


	
	std::vector<double> unique_timepoints;
	std::vector<size_t> response_timepointIDs;

	DISALLOW_COPY_AND_ASSIGN(ForestSurvival);
};

#endif /* FORESTSURVIVAL_H_ */
