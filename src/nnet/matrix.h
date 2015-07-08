#ifndef __BANG_MATRIX_H__
#define __BANG_MATRIX_H__

#include "stream.h"

/** @brief Number matrix */

template<class T> class CMatrix 
{
	T *data;
	/** Row and column count */
	int rows, cols;
public:
	~CMatrix ()	{ if (data) delete[] data; }
	CMatrix ()	{ data = NULL; rows = 0; cols = 0; }
	CMatrix (int rows, int cols) : rows(rows),cols(cols)	{ data = new T [rows*cols]; }
	
	CMatrix (const CMatrix &src);
	CMatrix<T> & operator = (const CMatrix &src);

	T & operator () (int row, int col) { assert (data && row < rows && col < cols && row >= 0 && col >= 0);
					 return data[row * cols + col]; }
	T operator () (int row, int col) const { assert (data && row < rows && col < cols && row >= 0 && col >= 0);
					 return data[row * cols + col]; }
	T &d (int row, int col)		{ return operator() (row,col); }
	void Realloc (int rows, int cols);
	/** Returns this * y_transponed  */
	void multiplyByTransponed (const CMatrix &y, CMatrix &retval);
	/** Returns this_transponed * y */
	void transponedMultiply (const CMatrix &y, CMatrix &retval);
	/** Inverts the current matrix (creates A^-1, where A x A^-1 = I) */
	void invert ();
	/** Fill with all zeros */
	void clear ();

	int getRows() const		{ return rows; }
	int getCols() const		{ return cols; }
};

template<class T> CMatrix<T> operator * (const CMatrix<T> & x, const CMatrix<T> & y);
template<class T> bang_ostream & operator << (bang_ostream &str, const CMatrix<T> &m);

#endif

