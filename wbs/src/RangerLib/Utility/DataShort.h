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
 
#ifndef DataShort_H_
#define DataShort_H_

#include <limits.h>

#include "RangerLib/globals.h"
#include "RangerLib/Utility/Data.h"

class DataShort: public Data {
public:
  DataShort();
  DataShort(double* data_double, std::vector<std::string> variable_names, size_t num_rows, size_t num_cols, bool& error);
  virtual ~DataShort();

  //double get(size_t row, size_t col) const {
  //  // Use permuted data for corrected impurity importance
  //  if (col >= num_cols) {
  //    col = getUnpermutedVarID(col);
  //    row = getPermutedSampleID(row);
  //  }

  //  if (col < num_cols_no_snp) {
  //    return data[col * num_rows + row];
  //  } else {
  //    // Get data out of snp storage. -1 because of GenABEL coding.
  //    size_t idx = (col - num_cols_no_snp) * num_rows_rounded + row;
  //    return (((snp_data[idx / 4] & mask[idx % 4]) >> offset[idx % 4]) - 1);
  //  }
  //}

  double get_data(size_t row, size_t col) const {
	  return data[col * num_rows + row];
  }

  void reserveMemory() {
    data = new short[num_cols * num_rows];
  }

  void set(size_t col, size_t row, double value, bool& error) {
	  if (value > SHRT_MAX || value < SHRT_MIN) {
      error = true;
    }
    if (floor(value) != ceil(value)) {
      error = true;
    }
    data[col * num_rows + row] = (short) value;
  }

  virtual MemoryMode memory_mode()const{ return MEM_SHORT; }

private:

  short* data;

  DISALLOW_COPY_AND_ASSIGN(DataShort);
};

#endif /* DataShort_H_ */
// #nocov end