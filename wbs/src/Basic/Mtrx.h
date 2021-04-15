//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once
#include <cmath>
#include <climits>

#pragma warning( disable : 4244 )
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/access.hpp>
#include <boost\multi_array.hpp>



namespace WBSF
{
	
	template <class T>
	class CMatrix : public boost::multi_array<T, 2>
	{
	public:

		//Constructor & Copy Constructor
		CMatrix(size_t nRows = 0, size_t nCols = 0) : boost::multi_array<T, 2>(boost::extents[nRows][nCols])
		{}

		CMatrix(const CMatrix& in) : boost::multi_array<T, 2>(boost::extents[in.rows()][in.cols()]) //: public boost::multi_array<T, 2>(in)
		{
			operator=(in);
		}
	
		//void clear();
		void resize(size_t nRows, size_t nCols)
		{
			boost::multi_array<T, 2>::resize(boost::extents[nRows][nCols]);
		}
		void resize_cols(size_t nCols)
		{
			boost::multi_array<T, 2>::resize(boost::extents[rows()][nCols]);
		}
		void resize_rows(size_t nRows)
		{
			boost::multi_array<T, 2>::resize(boost::extents[nRows][cols()]);
		}
		void init(const T& item)
		{
			CMatrix<T>& me = *this;

			for (size_t i=0; i<me.size(); i++)
				for (size_t j = 0; j<me[i].size(); j++)
					me[i][j] = item;
		}

		CMatrix& operator+= (const T& item)
		{
			CMatrix<T>& me = *this;
			for (size_t i=0; i<me.size(); i++)
			{
				for (size_t j=0; j<me[i].size(); j++)
				{
					me[i][j] += item;
				}
			}

		}
		CMatrix& operator-= (const T& item)
		{
			CMatrix<T>& me = *this;
			for (size_t i = 0; i<me.size(); i++)
			{
				for (size_t j = 0; j<me[i].size(); j++)
				{
					me[i][j] -= item;
				}
			}
		}
		
		
		T& operator()(size_t row, size_t col)   // i = row, y = col
		{
			CMatrix<T>& me = *this;
			return me[row][col];
		}

		//Get Size
		size_t size_x()const{ return shape()[1]; }
		size_t size_y()const{ return shape()[0]; }
		size_t cols()const{return size_x();}
		size_t rows()const{return size_y();}
	};
}//namesapce WBSF
