#include "matrix.h"

template<class T> CMatrix<T>::CMatrix (const CMatrix &src)
{
	rows = src.rows;
	cols = src.cols;
	data = new T [rows * cols];
	if (data == NULL) {
		rows = 0;
		cols = 0;
	}
	else memcpy (data, src.data, sizeof(T) * rows * cols);
}

template<class T> CMatrix<T> & CMatrix<T>::operator = (const CMatrix &src)
{
	if (data) delete[] data;
	rows = src.rows;
	cols = src.cols;
	data = new T [rows * cols];
	if (data == NULL) {
		rows = 0;
		cols = 0;
	}
	else memcpy (data, src.data, sizeof(T) * rows * cols);
	return *this;
}


template<class T> void CMatrix<T>::Realloc (int rows, int cols)
{
	if (data) delete[] data;
	data = new T [rows * cols];
	if (data) {
		this->rows = rows;
		this->cols = cols;
	}
	else {
		this->rows = 0;
		this->cols = 0;
	}
}

template<class T> CMatrix<T> operator * (const CMatrix<T> & x, const CMatrix<T> & y)
{
	int i,j,k;
	assert (x.getCols() == y.getRows());
	if (x.getCols() != y.getRows())
		return CMatrix<T> ();

	T sum;
	CMatrix<T> retVal (x.getRows(), y.getCols());
	for (i = 0; i < x.getRows(); ++i)
	for (j = 0; j < y.getCols(); ++j) {
		sum = 0;
		for (k = 0; k < x.getCols(); ++k)
			sum += x (i,k) * y (k,j);
		retVal (i,j) = sum;
	}
	return retVal;
}

template<class T> void CMatrix<T>::multiplyByTransponed (const CMatrix &y, CMatrix &retval)
{
	int i,j,k;
	assert (cols == y.cols);
	if (cols != y.cols) { retval.Realloc (0,0); return; }

	T sum;
	this->retVal.Realloc (rows, y.rows);
	for (i = 0; i < rows; ++i)
	for (j = 0; j < y.rows; ++j) {
		sum = 0;
		for (k = 0; k < cols; ++k)
			sum += d (i,k) * y (j,k);
		this->retVal (i,j) = sum;
	}
}

template<class T> void CMatrix<T>::transponedMultiply (const CMatrix &y, CMatrix &retval)
{
	int i,j,k;
	assert (rows == y.rows);
	if (rows != y.rows) { retval.Realloc (0,0); return; }

	T sum;
	retval.Realloc (cols, y.cols);
	for (i = 0; i < cols; ++i)
	for (j = 0; j < y.cols; ++j) {
		sum = 0;
		for (k = 0; k < rows; ++k)
			sum += d (k,i) * y (k,j);
		retval (i,j) = sum;
	}
}

/* Inverting by the elemental transformation method:
   
     1 3 | 1 0  ->  1  3 |  1 0  ->  1 0 |-.2  .6
     2 1 | 0 1      0 -5 | -2 1      0 1 | .4 -.2
  
   I.e. the last right side is inversion of first left side */

template<class T> void CMatrix<T>::invert ()
{
#define FAIL { delete[] data; rows = cols = 0; data = 0; return; }

	this->assert (rows == cols);
	if (rows != cols) FAIL;
	CMatrix<T> inv (rows,rows);
	int i,j,k;
	double r;

	for (i=0; i < rows; ++i)
	for (j=0; j < rows; ++j)
		inv( i,j ) = i == j ? 1 : 0;
	// elemental transformations:
	for (i=0; i < rows; ++i) {
		// pivot must not be zero
		if (d(i,i) == 0) {
			for (j=i+1; j < rows && d(j,i) == 0; ++j);
			// if you failed here, the matrix can't be inverted!
			this->assert (j < rows); 
			if (j == rows) FAIL;
			// add j-th row to the i-th row
			for (k=i; k < rows; ++k) 
				d(i,k) += d(j,k);
			for (k=0; k < rows; ++k)
				inv(i,k) += inv(j,k);
		}
		
		// create the diagonal 1
		r = d(i,i);
		d(i,i) = 1;
		for (k=i+1; k < rows; ++k) 
			d(i,k) /= r;
		for (k=0; k < rows; ++k) 
			inv(i,k) /= r;

		// subtract this row from all others
		for (j=0; j < rows; ++j) {
			if (j == i) continue;
			r = d(j,i);
			d(j,i) = 0;
			for (k=i+1; k < rows; ++k) 
				d(j,k) -= d(i,k) * r;
			for (k=0; k < rows; ++k) 
				inv(j,k) -= inv(i,k) * r;
		}
	}
	delete[] data;
	data = inv.data;
	inv.data = NULL;
}


template<class T> void CMatrix<T>::clear() 
{
	if (data) memset (data, 0, rows * cols * sizeof (T));
}


template<class T> bang_ostream & operator << (bang_ostream &str, const CMatrix<T> &m)
{
	for (int i=0; i < m.getRows(); ++i) {
		for (int j=0; j < m.getCols(); ++j)
			str << m (i,j) << "\t";
		str << "\n";
	}
	return str;
}

