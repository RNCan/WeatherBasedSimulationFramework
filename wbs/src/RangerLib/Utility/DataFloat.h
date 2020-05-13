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
 
#ifndef DATAFLOAT_H_
#define DATAFLOAT_H_

#include <assert.h>
#include "RangerLib/globals.h"
#include "RangerLib/Utility/Data.h"

class DataFloat: public Data {
public:
  DataFloat();
  DataFloat(double* data_double, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols);
  virtual ~DataFloat();

  virtual void reshape(const std::vector<std::string>& names)override;
  

  double get_data(size_t row, size_t col) const {
	  return data[col * num_rows + row];
  }

  void reserveMemory() {
	  externalData = false;
	  if (data != nullptr)
	  {
		  delete[] data;
		  data = nullptr;
	  }

    data = new float[num_cols * num_rows];
  }

  void set(size_t col, size_t row, double value, bool& error) {
    data[col * num_rows + row] = (float) value;
  }

  virtual MemoryMode memory_mode()const{ return MEM_FLOAT; }
private:
  float* data;

  DISALLOW_COPY_AND_ASSIGN(DataFloat);
};

#endif /* DATAFLOAT_H_ */
// #nocov end

