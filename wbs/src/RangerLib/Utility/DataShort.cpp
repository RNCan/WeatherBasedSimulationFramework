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

#include <limits.h>
#include <math.h>
#include <iostream>
#include <vector>

#include "DataShort.h"

DataShort::DataShort() :
	data(nullptr) {
	externalData = false;
}

DataShort::DataShort(double* data_double, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols, bool& error)
{
	this->variable_names = variable_names;
	this->num_rows = num_rows;
	this->num_cols = num_cols;
	this->num_cols_no_snp = num_cols;

	reserveMemory();

	// Save data and report errors
	for (size_t i = 0; i < num_cols; ++i) {
		for (size_t j = 0; j < num_rows; ++j) {
			double value = data_double[i * num_rows + j];
			if (value > SHRT_MAX || value < SHRT_MIN) {
				error = true;
			}
			if (floor(value) != ceil(value)) {
				error = true;
			}
			data[i * num_rows + j] = (short)value;
		}
	}
}

DataShort::~DataShort() {
	if (!externalData) {
		delete[] data;
		data = nullptr;
	}
}

void DataShort::reshape(const std::vector<std::string>& names)
{
	assert(data != nullptr);

	//cols to remove
	short* p_new = new short[names.size() * num_rows];

	size_t i = 0;
	for (auto name : names)
	{
		auto it = find(variable_names.begin(), variable_names.end(), name);
		if (it != variable_names.end())
		{
			size_t col = std::distance(variable_names.begin(), it);
			memcpy(&(p_new[i * num_rows]), &(data[col * num_rows]), num_rows * sizeof(short));
		}
		i++;
	}

	delete[] data;
	data = p_new;

	num_cols = names.size();
	num_cols_no_snp = num_cols;
	variable_names = names;
}

// #nocov end
