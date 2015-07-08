/** @file traindata.h

  @brief CTrainingData header - Neural Network Training Data

  @author Jakub Adámek
  Last modified $Id: traindata.h,v 1.21 2002/04/22 15:43:23 jakubadamek Exp $
*/

#ifndef __BANG_NNDATA_H__
#define __BANG_NNDATA_H__

#include "utils.h"
#include "xmlstream.h"
#include "assert.h"

SMALLVECTOR (double, CFloat, TSmallVectorFloat)
VECTOR (CFloat, TVectorFloat)
//typedef TVectorFloat TSmallVectorFloat;
VECTOR (CInt, TVectorInt)
MAP (CString, CFloat, TMapStringFloat)

/** @brief training pattern - inputs, desired outputs and set */

enum enumSetType { ST_NULL=-1, ST_EVAL=0, ST_TRAIN=1 };
static const TEnumString SetTypes = "eval;train";

VECTOR (TStream, TStreams)

STRUCT_BEGIN (TPattern)
public:
	void Init ()	{ set = ST_NULL; series=0; }
	/// data[CU_INPUT], data[CU_OUTPUT]
	TSmallVectorFloat data[2];
	/// which set contains this pattern - see enumSetType
	CInt set;
	/// which series contains this pattern
	CInt series;
	/// the rows are sorted by sets 
	bool operator < (const TPattern &pat) const
	{ return pat.set < set; }
STRUCT_END (TPattern)

VECTOR (TPattern, TPatterns)

STRUCT_BEGIN (TTrainingDataFile)
public:
	/// streams containing data (to be joined from left to right)
	TStreams streams;
STRUCT_END (TTrainingDataFile)

VECTOR (TTrainingDataFile, TTrainingDataFiles)

/** C O L U M N   S T A F F */

/** Types of values */
enum filterType { 
	/// boolean - two different values, e.g. 0/1,yes/no
	FT_BOOL, 
	/// integer number
	FT_INT, 
	/// floating point number
	FT_FLOAT, 
	/// category (text value), each row contains one 
	FT_CATEGORY, 
	/// category (text value), each row contains any number of them (0-n)
	FT_MULTICATEGORY 
};

static const TEnumString FTs = "bool;int;float;category;multicategory";

/** How to translate values in column */
enum filterTranslate { 
	/** Default for everything except multi-category 
		- linearly transform from [min;max] into [-1;1]
		For categories: assign order values (0,1,2,...) to them and linearly transform.
		Not usable for multi-categories.
	*/
	FTR_FLOAT, 
	/** uses the parameters "avg" and "stdev": x -> (x - avg) / stdev */
	FTR_LINEAR,
	/** For category / multi-category - assign one input 0/1 to every possible value */
	FTR_BOOLS, 
	/** Don't translate - use values as they are */
	FTR_NONE
};

static const TEnumString FTRs = "float;linear;bools;none";


STRUCT_BEGIN (TFilter)
	void Init ()	{ type=FT_FLOAT; translate=FTR_FLOAT; minR=0; maxR=0; 
			  toMinR=-1; toMaxR=1; toAvg=0; toStdev=1; avg=0; stdev=1; 
 			  digits=-1; }

	
	/// see enum filterType
	CInt type;
	/// see enum filterTranslate
	CInt translate;
	/// interval to transform from by FLOAT
	CFloat minR, maxR;
	/// interval to transform to by FLOAT (usually [-1;+1])
	CFloat toMinR, toMaxR;
	/// average and standard deviation for LINEAR transformation
	CFloat avg, stdev;
	/// average and standard deviation after LINEAR transformation (usually 0,1)
	CFloat toAvg, toStdev;
	/// categories
	TEnumString categories;
	/// for doubles used as categories - number of decimal digits when converting to string
	CInt digits;
	/// number of outputs when preprocessing
	inline int getWidth() const; 

	/** PRE- and POST- PROCESSING
		Following functions all use the last argument to place results.
		That's because some of them may place more than one results and also
		the result type may be specified by overloading, not by function name. */

	/// returns preprocessed value (or array of values on FTR_BOOLS)
	inline void preprocess (double value, double &result);
	/// returns preprocessed value (or array of values on FTR_BOOLS)
	inline void preprocess (const CString &value, double &result);

	/** returns postprocessed double value.	Inverse to preprocess. */
	inline void postprocess (const double &value, double &result) const;
	/** returns postprocessed string value. Inverse to preprocess. */
	inline void postprocess (const double &value, CString &result) const;

	/** returns postprocessed double value, but doesn't add the average - only multiplies */
	inline void postprocessError (const double &value, double &result) const;
STRUCT_END (TFilter)

/** Usage of column */
enum colUse { 
	/// input 
	CU_INPUT, 
	/// output
	CU_OUTPUT, 
	/// not used 
	CU_NO 
};

static const TEnumString CUs = "input;output;no";

STRUCT_BEGIN_FROM (TColumn, TFilter)
	void Init ()	{ use=CU_NO; pos = 0; }
	/// see enum colUse
	CInt use;
	/// starting position in the input or output vector
	CInt pos;
	
	/// window - use only for series: take appropriate values from neighbours
	STRUCT_BEGIN (TWindow)
		void Init()	{ left = right = 0; empty = "0"; }

		CInt left, right;
		/// default value when no such neighbours exist (e.g. left from 1st)
		CString empty;
	STRUCT_END (TWindow)
	TWindow window;

	inline int getColWidth() const;

	/** Returns appropriate input or output column value. For CTR_BOOLS returns the first value.
		"shift" relates to window, ranges in [-left;+right] */
	inline double &val (TPattern &pat, int shift=0) const;
	inline double val (const TPattern &pat, int shift=0) const;
	inline void copy (const TPattern &src, int shiftSrc, TPattern &dst, int shiftDst);
STRUCT_END_FROM (TColumn, TFilter)
VECTOR (TColumn, TColumns)


/** @brief Controls manipulation with training data. Allows to define sets of training data.

	This class has an iterator holding current position in the data. You can move it with move....
*/

STRUCT_BEGIN (CTrainingData)
public:
	void Init ()	{ pos=data.end(); series=false; colCount[CU_INPUT]=colCount[CU_OUTPUT]=0; }

	/// Moves position to beginning. Returns: true if position valid.
	bool moveFirst () const					{ return (pos = data.begin()) != data.end(); }	
	bool moveNext () const					{ return ++pos != data.end(); }
	/** Moves position to next row. Returns: true if position valid.
		Inline to be quickly. */
	bool moveNext (CInt set) const;
	bool movePrev (CInt set) const;

	/// Moves position to start of given set. Returns: true if position valid
	bool moveToSetStart (CInt setType) const;

	/// Moves position to the row of a given index. Returns: true if position valid
	bool moveToRow (CInt row) const;

	/// getSet returns: set of current row or ST_NULL if position not valid
	inline CInt getSet () const
	{ if (pos == data.end()) return ST_NULL; else return (*pos).set; }

	/// getInputs returns: inputs on current row 
	inline const TSmallVectorFloat &getInputs () const	{ return pos->data[CU_INPUT]; }
	/// getOutputs returns: desired outputs on current row 
	inline const TSmallVectorFloat &getOutputs () const	{ return pos->data[CU_OUTPUT]; }
	inline TSmallVectorFloat &modifyOutputs ()		{ return pos->data[CU_OUTPUT]; }

	inline const TPattern &getPattern () const		{ return *pos; }
	inline int getSeries () const				
	{ if (pos == data.end()) return -1; else return (int)pos->series; }
	inline TPatterns::iterator getPos () const		{ return pos; }
	inline void setPos (TPatterns::iterator x) const	{ pos = x; }

	STRUCT_BEGIN (TRange)
		void Init ()		{ from=0; to=0; random=0; rest=false; type=ST_NULL; }
		/** negative values mean size in percent; to = 0 means until the end of file */
		CInt from, to; 
		/** Number of lines to be randomly chosen from this range. Zero means don't use random. */
		CInt random;
		/** Bool: Use the rest of the range - useful especially when another set uses random lines */
		CInt rest;
		/// type of data set - see enumSetType
		CInt type;
	STRUCT_END (TRange)
	VECTORC (TRange, TRanges)

	/// goes through ranges and sets the set info to rows. Returns error or empty string.
	CString processRanges ();

	/// goes through all data and columns and sets the window results
	void processWindows ();

	const TColumns &getColumns () const	{ return columns; }
	CInt getColCount(colUse use) const	{ assert (use==CU_INPUT || use==CU_OUTPUT); return colCount[use]; }
	CInt getRowCount(enumSetType set) const	{ return rowCount[set]; }
	void addColumn (TColumn col);		
	void addFile (const TTrainingDataFile &f){ files.push_back (f); }

	/// sets directly the column value (caller must know about columns), uses the column translation
	//inline void setColDirect (colUse use, int icol, double value);	
	/** returns double column value translated from direct values */
	inline void getPostprocessed (colUse use, int col, double &result);
	/** returns string column value translated from direct values - not yet implemented */
	inline void getPostprocessed (colUse use, int col, CString &result);

	/// returns input or output column of given index (starting with 0)
	const TColumn &getColumn (colUse use, int col) const;	

	/// where to dump configuration (including columns) after processing data - if empty, nowhere
	CString dumpCfg;
	/// bool: go to through columns and set their translations or not?
	CInt examineColumns;

	/// finds minR, maxR, avg, stdev for all columns
	void examine ();

	/// sets the colIndexes array 
	void setColIndexes ();

	int getSeriesCount(enumSetType set) const;

	/// deletes all rows
	void clear ()	{ data.DeleteAll(); pos = data.end(); }
	/// adds new row - set values by fillColumn functions
	void addRow (enumSetType set, int series = 0);
	/// preprocesses and sets value on current row 
	inline void fillColumn (colUse use, int col, double value);
	/// preprocesses and sets value on current row 
	inline void fillColumn (colUse use, int col, const CString &value);

	TRanges ranges;

	/** indexes of columns - e.g. columns' use is CU_NO,CU_INPUT,CU_OUTPUT,...
		=> colIndexes[CU_INPUT][0]=1, colIndexes[CU_OUTPUT][0]=2 */
	TVectorInt colIndexes[2];

	/// holds all the training data
	TPatterns data;
	/// guards the data when a neural network uses them
	Mutex dataMutex;
	/// row count for training and eval set
	CInt rowCount [2];
	/// columns count for input and output cols
	CInt colCount[2];
	TTrainingDataFiles files;
	/// columns description
	TColumns columns;
	/** Bool: Are data organised into series? */
	CInt series;
	/** If a row begins with this, it separates two series */
	CString seriesSeparator;

private:
	/** reads data from files - returns errors if any occur. The first param is the config file name. */
	CString readFromFiles (CString filename, bool onlyCategories = false);
	mutable TPatterns::iterator pos;

public:	
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/*       Configuration READing and PRINTing              */
	
	/// dumps all data to the string
	void dumpData (CString &out);

	/** reads all data and sets columns - returns error description or empty string.
		First param is the config file name. */
	CString readData(CString filename);
		
	/// prints the cfg and the data as local in the file
	CRox *printAllLocal ();

	CRox *printColumns () const;
	CString readColumns (const CRox *xml);

STRUCT_END (CTrainingData)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                               * 
 *                  C T r a i n i n g D a t a                    *
 *                                                               *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

inline bool CTrainingData::moveNext (CInt set) const
{ 
	if (pos == data.end()) return false; 
	do ++pos;
	while (pos != data.end() && pos->set != set);
	return pos != data.end(); 
}

inline bool CTrainingData::movePrev (CInt set) const
{ 
	if (pos == data.begin()) return false; 
	do --pos;
	while (pos != data.begin() && pos->set != set);
	return pos->set == set; 
}

inline void CTrainingData::fillColumn (colUse use, int col, double value)
{
	TColumns::iterator icol = columns.begin()+colIndexes[use][col];
	icol->preprocess (value, icol->val( *pos ));
}

inline void CTrainingData::getPostprocessed (colUse use, int col, double &result)
{
	TColumns::iterator icol = columns.begin()+colIndexes[use][col];
	icol->postprocess (icol->val (*pos), result);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                               * 
 *                        T F i l t e r		                 *
 *                                                               *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

inline void TFilter::preprocess (double value, double &result)
{
	switch ((int)translate) {
	case FTR_FLOAT: // transform into [toMinR;toMaxR]
		if (maxR == minR) result = 0;
		else result = (value - minR) * (toMaxR - toMinR) / (maxR - minR) + toMinR; break;
	case FTR_LINEAR: 
		if (stdev == 0) result = 0;
		else result = (value - avg) * toStdev / stdev + toAvg; break;
	case FTR_NONE:
		result = value; break;
	case FTR_BOOLS:
		if ((int)type == FT_CATEGORY) {
			preprocess (toString (value,digits), result);
			break;
		}
	default: // not yet implemented
		assert (false);
		result = 0; break;
	}
}

inline void TFilter::preprocess (const CString &value, double &result)
{
	switch ((int)type) {
	case FT_INT:
	case FT_FLOAT:
		preprocess (atof (value.c_str()), result); break;
	case FT_CATEGORY: {
		int category = categories[value];
		switch ((int)translate) {
		case FTR_FLOAT:
		case FTR_LINEAR:
			preprocess (category, result);
			break;
		case FTR_BOOLS: {
			for (int i=0; i < categories.size(); ++i)
				(&result)[i] = i == category ? 1 : 0;
			break;	}
		default: assert (false); //not yet implemented
		}
		break;	  }
	default: assert (false); //not yet implemented
	}
}

inline void TFilter::postprocessError (const double &value, double &result) const
{
	switch ((int)translate) {
	case FTR_FLOAT:
		result = value * (maxR - minR) / (toMaxR - toMinR); break;
	case FTR_LINEAR:
		result = value * stdev / toStdev; break;
	case FTR_NONE:
		result = value; break;
	default: // not yet implemented
		assert (false);
		result = 0; break;
	}
}

inline void TFilter::postprocess (const double &value, double &result) const
{
	switch ((int)translate) {
	case FTR_FLOAT:
		result = (value - toMinR) * (maxR - minR) / (toMaxR - toMinR) + minR; break;
	case FTR_LINEAR:
		result = (value - toAvg) * stdev / toStdev + avg; break;
	case FTR_NONE:
		result = value; break;
	default: // not yet implemented
		assert (false);
		result = 0; break;
	}
}

inline void TFilter::postprocess (const double &value, CString &result) const
{
	switch ((int)translate) {
	case FTR_FLOAT:
	case FTR_LINEAR: {
		double dresult;
		postprocess (value, dresult);
		result = toString (dresult);
		break; }
	case FTR_BOOLS: {
		assert (type == FT_CATEGORY);
		int i;
		for (i=0; i < categories.size(); ++i) 
			if ((&value)[i] == 1) {
				result = categories[i];
				return;
			}
		result = -1;
		break; }
	case FTR_NONE:
		result = toString (value);
		break;
	default: // not yet implemented
		assert (false);
		result = "";
		break;
	}
}

inline int TFilter::getWidth() const
{ 
	if (translate != FTR_BOOLS) 
		return 1; 
	else return categories.size(); 
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                               * 
 *                         T C o lu m n		                 *
 *                                                               *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

inline const TColumn &CTrainingData::getColumn (colUse use, int icol) const	
{ 
	assert (use == CU_INPUT || use == CU_OUTPUT);
	return columns[colIndexes[use][icol]];
}

inline double &TColumn::val (TPattern &pat, int shift) const
{
	assert (shift >= -window.left && shift <= window.right && use != CU_NO);
	return pat.data[use][pos + (window.left+shift) * TFilter::getWidth()];
}

inline double TColumn::val (const TPattern &pat, int shift) const
{
	assert (shift >= -window.left && shift <= window.right && use != CU_NO);
	return pat.data[use][pos + (window.left+shift) * TFilter::getWidth()];
}

inline void TColumn::copy (const TPattern &src, int shiftSrc, TPattern &dst, int shiftDst)
{
	assert (shiftSrc >= -window.left && shiftSrc <= window.right && use != CU_NO
		&& shiftDst >= -window.left && shiftDst <= window.right);
	if (use == CU_NO) return;
	for (int i=0; i < TFilter::getWidth(); ++i)
		dst.data[use][pos + (window.left+shiftDst) * TFilter::getWidth() + i]
		= src.data[use][pos + (window.left+shiftSrc) * TFilter::getWidth() + i];
}

inline int TColumn::getColWidth() const	
{ 
	return (window.left + window.right + 1) * TFilter::getWidth(); 
}

#endif

