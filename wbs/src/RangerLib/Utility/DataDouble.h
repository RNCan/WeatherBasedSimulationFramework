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

#ifndef DATADOUBLE_H_
#define DATADOUBLE_H_

#include <assert.h>
#include "RangerLib/globals.h"
#include "RangerLib/Utility/utility.h"
#include "RangerLib/Utility/Data.h"

class DataDouble: public Data {
public:
  DataDouble();
  DataDouble(double* data, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols) :
      data(data) {
    this->variable_names = variable_names;
    this->num_rows = num_rows;
    this->num_cols = num_cols;
    this->num_cols_no_snp = num_cols;
	this->externalData = true;
  }
  virtual ~DataDouble();

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

    data = new double[num_cols * num_rows];
  }

  void set(size_t col, size_t row, double value, bool& error) {
    data[col * num_rows + row] = value;
  }

  virtual MemoryMode memory_mode()const{ return MEM_DOUBLE; }

private:
  double* data;

  DISALLOW_COPY_AND_ASSIGN(DataDouble);
};

#endif /* DATADOUBLE_H_ */
