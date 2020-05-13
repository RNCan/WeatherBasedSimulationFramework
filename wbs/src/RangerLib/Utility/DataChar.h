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

 http://www.imbs-luebeck.de
 #-------------------------------------------------------------------------------*/

// Ignore in coverage report (not used in R package)
// #nocov start
 
#ifndef DATACHAR_H_
#define DATACHAR_H_

#include <limits.h>
#include <assert.h>

#include "RangerLib/globals.h"
#include "RangerLib/Utility/Data.h"

class DataChar: public Data {
public:
  DataChar();
  DataChar(double* data_double, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols, bool& error);
  virtual ~DataChar();


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

    data = new char[num_cols * num_rows];
  }

  void set(size_t col, size_t row, double value, bool& error) {
    if (value > CHAR_MAX || value < CHAR_MIN) {
      error = true;
    }
    if (floor(value) != ceil(value)) {
      error = true;
    }
    data[col * num_rows + row] = (char) value;
  }

  virtual MemoryMode memory_mode()const{ return MEM_CHAR; }

private:
  char* data;

  DISALLOW_COPY_AND_ASSIGN(DataChar);
};

#endif /* DATACHAR_H_ */
// #nocov end
