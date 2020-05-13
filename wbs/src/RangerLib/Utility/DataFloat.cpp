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

// Ignore in coverage report (not used in R package)
// #nocov start
 
#include <iostream>
#include <vector>

#include "DataFloat.h"

DataFloat::DataFloat() :
    data(0) {
}

DataFloat::DataFloat(double* data_double, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols) {
  this->variable_names = variable_names;
  this->num_rows = num_rows;
  this->num_cols = num_cols;
  this->num_cols_no_snp = num_cols;

  reserveMemory();
  for (size_t i = 0; i < num_cols; ++i) {
    for (size_t j = 0; j < num_rows; ++j) {
      data[i * num_rows + j] = (float) data_double[i * num_rows + j];
    }
  }
}

DataFloat::~DataFloat() {
  if (!externalData) {
    delete[] data;
	data = nullptr;
  }
}

void DataFloat::reshape(const std::vector<std::string>& names)
{
	assert(data != nullptr);

	//cols to remove
	float* p_new = new float[names.size() * num_rows];

	size_t i = 0;
	for (auto name : names)
	{
		auto it = find(variable_names.begin(), variable_names.end(), name);
		if (it != variable_names.end())
		{
			size_t col = std::distance(variable_names.begin(), it);
			memcpy(&(p_new[i * num_rows]), &(data[col * num_rows]), num_rows * sizeof(float));
		}
		i++;
	}

	delete[] data;
	data = p_new;

	num_cols = names.size();
	num_cols_no_snp = num_cols;
	variable_names = names;
}