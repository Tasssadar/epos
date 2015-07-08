/** @file traindata.cpp

  @brief CTrainingData definition

  @author Jakub Adámek
  Last modified $Id: traindata.cc,v 1.19 2002/04/22 15:43:23 jakubadamek Exp $
*/

#include "traindata.h"
#include "utils.h"
#include "xmlutils.h"
#include <time.h>
#include <math.h>

REGVECTOR (TPattern, TPatterns)
REGVECTOR (TTrainingDataFile, TTrainingDataFiles)
REGVECTOR (TStream, TStreams)
REGMAP (CString, CFloat, TMapStringFloat)

REGVECTORC (CTrainingData, TRange, TRanges)
REGVECTOR (TColumn, TColumns)

REGSTRUCT (TPattern)
REGSTRUCT (TTrainingDataFile)
REGSTRUCTC (CTrainingData, TRange)
REGSTRUCT (CTrainingData)
REGSTRUCT (TColumn)
REGSTRUCTC (TColumn, TWindow)
REGSTRUCT (TFilter)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                                                               * 
 *                  C T r a i n i n g D a t a                    *
 *                                                               *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

bool CTrainingData::moveToSetStart (CInt set) const
{ 
	pos = data.begin();
	while (pos != data.end() && pos->set != set) ++pos;
	return pos != data.end();
}

bool CTrainingData::moveToRow (CInt row) const
{
	if (data.size() <= row) return false;
	pos = data.begin() + row;
	return true;
}

int CTrainingData::getSeriesCount(enumSetType set) const
{
	int iser=0;
	if (!series) return 0;
	int retval=0;
	TPatterns::iterator irow;
	for (irow=data.begin(); irow != data.end(); ++irow)
		if (irow->series != iser) {
			iser = irow->series;
			if (irow->set == set) ++retval;
		}
	return retval;
}

CString CTrainingData::processRanges ()
{
	if (data.size() == 0) return "";

	CString errors;
	TRanges::iterator rangeIter;
	int size = series ? series : data.size();
	for (rangeIter = ranges.begin(); rangeIter != ranges.end(); ++rangeIter) {
		CString errorDesc = CString("Range (")+toString (rangeIter->from)+"->";
		if (rangeIter->to == -1) errorDesc += "end"; else errorDesc += toString (rangeIter->to) + "): ";
		int rfrom = rangeIter->from; 
		if (rfrom < 0) rfrom = int (rfrom * size / -100.0 + 0.5);
		int rto = rangeIter->to;     
		if (rto == 0 || rto == -100) rto = size-1;
		// care that from=20% and to=20% don't overlap
		if (rto < 0) rto = int (size * rto / -100.0 - 0.5);
		// series are indexed beginning from 1
		if (series) {
			rfrom ++;
			rto ++;
		}
		// count of random rows already used
		int irandom = 0;
		int random = rangeIter->random;
		if (random < 0) random = int (random * size / -100.0 + 0.5);
		if (random) srand( (unsigned)time( NULL ) );
		bool useMe = true;
		
		if (series) {
			int irow, iser = 0;
			for (irow = 0; data[irow].series < (CInt)rfrom; ++irow);
			for (; irow < data.size() && data[irow].series <= rto; ++irow) {
				if (random && iser != data[irow].series) {
					useMe = false;
					iser = data[irow].series;
					if (rto-iser+1 <= random-irandom) useMe = true;
					else useMe = rand()%100 < 100 * (random-irandom) / (rto-iser+1);
					if (useMe) ++irandom;
				}
				if ((int)data[irow].set != ST_NULL) {
					if ((int)rangeIter->rest == 0) errors += errorDesc+"the ranges overlap.";
				}
				else if (useMe) {
					data[irow].set = rangeIter->type;
					rowCount[(int)rangeIter->type]++;
				}
			}
		}
		else {
			for (int irow = rfrom; irow <= rto; ++irow) {
				if (random) {
					useMe = false;
					if (rto-irow+1 <= random-irandom) useMe = true;
					else useMe = rand()%100 < 100 * (random-irandom) / (rto-irow+1);
					if (useMe) ++irandom;
				}
				if ((int)data[irow].set != ST_NULL) {
					if ((int)rangeIter->rest == 0) errors += errorDesc+"the ranges overlap.";
				}
				else if (useMe) {
					data[irow].set = rangeIter->type;
					rowCount[(int)rangeIter->type]++;
				}
			}
		}
	}
	return errors;
}


void CTrainingData::examine ()
{
	if (data.size() == 0) return;
	TColumns oldColumns = columns;

	TColumns::iterator col;
	for (col = columns.begin(); col != columns.end(); ++col) {
		if (col->use == CU_NO) continue;
		col->minR = col->maxR = col->val (*data.begin());
		col->stdev = 0;
		col->avg = 0;
	}
	TPatterns::iterator pat;
	double val;

	for (pat = data.begin(); pat != data.end(); ++pat) {
		for (col = columns.begin(); col != columns.end(); ++col) {
			if (col->use == CU_NO) continue;
			val = col->val (*pat);
			if (val < col->minR) col->minR = val;
			if (val > col->maxR) col->maxR = val;
			col->avg += val;
		}
	}
	for (col = columns.begin(); col != columns.end(); ++col) 
		col->avg /= data.size();

	// find standard deviations
	for (pat = data.begin(); pat != data.end(); ++pat) {
		for (col = columns.begin(); col != columns.end(); ++col) {
			if (col->use == CU_NO) continue;
			val = col->val (*pat);
			col->stdev += sqr (val - col->avg);
		}
	}
	for (col = columns.begin(); col != columns.end(); ++col) 
		col->stdev = sqrt (col->stdev / (data.size()-1));

	setColIndexes();

	TColumns::iterator oldcol;
	
	// re-translate the data 
	for (pat = data.begin(); pat != data.end(); ++pat) {
		for (oldcol = oldColumns.begin(),
			col = columns.begin(); col != columns.end(); ++col, ++oldcol) {
			if (col->use != CU_NO) switch ((int)col->translate) {
			case FTR_FLOAT:
			case FTR_LINEAR:
				val = oldcol->val(*pat);
				col->preprocess (val, col->val(*pat));
				break;
			}
		}
	}
}

void CTrainingData::processWindows ()
{
	TColumns::iterator col;
	int shift;
	int row;
	for (row = 0; row < data.size(); ++row)
	for (col = columns.begin(); col != columns.end(); ++col) 
	if (col->use != CU_NO)
	for (shift = - col->window.left; shift <= col->window.right; ++shift) {
		if (row + shift < 0 || row + shift >= data.size() 
			|| data[row+shift].series != data[row].series)
			col->preprocess (col->window.empty, col->val(data[row],shift));
		else col->copy(data[row+shift],0,data[row],shift);
	}

	// debug log
	if (false) {
		bang_ofstream w ("win.txt");
		w << "Data dump after processWindows" << bang_endl;
		w << "Series\tInputs\tDesired Outputs" << bang_endl;
		for (row = 0; row < data.size(); ++row) {
			w << data[row].series << "\t";
			for (shift = 0; shift < data[row].data[CU_INPUT].size(); ++shift)
				w << data[row].data[CU_INPUT][shift] << "\t";
			for (shift = 0; shift < data[row].data[CU_OUTPUT].size(); ++shift)
				w << data[row].data[CU_OUTPUT][shift] << "\t";
			w << bang_endl;
		}
		w.close();
	}
}


CString CTrainingData::readFromFiles (CString filename, bool onlyCategories)
{
	CString errors;

	TTrainingDataFiles::iterator fileIter;
	TColumns::iterator colIter;
	TStreams::iterator streamIter;

	if (columns.size() == 0) return "readFromFiles error: No columns";
	setColIndexes();
	TPattern *pattern = new TPattern;
	int width[2] = {0,0};
	for (colIter = columns.begin(); colIter != columns.end(); ++colIter)
		if (colIter->use != CU_NO) width[colIter->use] += colIter->getColWidth();
	pattern->data[CU_INPUT].Realloc (width[CU_INPUT]);
	pattern->data[CU_OUTPUT].Realloc (width[CU_OUTPUT]);
	pattern->set = ST_NULL;
	pattern->series = -1;

	data.DeleteAll();
	// beginning of this file
	TPatterns::iterator fileFirstPattern = data.begin();
	// active pattern
	TPatterns::iterator filePattern;
	bang_ifstream file;
	static const int buflen = 1000;
		// contains the row read
	char	row[buflen];
	TVectorString values;
	TVectorString::iterator ival;
	int iInput, iOutput;
	rowCount[ST_TRAIN] = rowCount[ST_EVAL] = 0;
	CString srow;
	if (series) series = 1;

	for (fileIter = files.begin(); fileIter != files.end(); ++fileIter) {
		// streams represent vertical splitting of data - where does this stream begin?
		int strFirstCol = 0,
			strFirstInput = 0,
			strFirstOutput = 0;
		// row count - should be the same for all streams
		int nRow = 0;
		for (streamIter = fileIter->streams.begin(); streamIter != fileIter->streams.end(); ++streamIter) {
			filePattern = fileFirstPattern;
			if (streamIter->value.length()) 
				file.open (getFilePath(filename) + streamIter->value);
			if (!file) {
				errors += CString("Could not open for reading file ")+streamIter->value;
				continue;
			}
			// row index
			int iRow = 0;
			// column count should be the same on every row
			int colCount = 0;
			while (!file.eof()) {
				file.getline (row, buflen);
				if (strlen (row) == 0) continue;
				if (!strchr (row,'\n') && !file.eof()) 
					errors += CString("File ")+streamIter->value+",row no. "+toString(iRow+1)+" too long\n";
				if (strchr (row,'\n')) *strchr (row,'\n') = '\x0';
				srow = row;
				if (series && srow.substr (0,strlen(seriesSeparator.c_str())) == seriesSeparator) {
					++series;
					continue;
				}
				values = split (srow, streamIter->delimiter);
				if (colCount == 0) colCount = values.size();
				if (colCount && values.size() && colCount != values.size()) {
					errors += CString("File ")+streamIter->value+",row no. "+toString(iRow+1)+" - bad column count\n";
					continue;
				}
				if (strFirstCol) {
					if (filePattern == data.end()) {
						errors += CString("File ")+streamIter->value+" has too many rows.\n";
						break;
					}
					pattern = & (*filePattern);
					++filePattern;
				}
				iInput = columns[strFirstInput].pos;
				iOutput = columns[strFirstOutput].pos;
				ival = values.begin();
				TColumns::iterator icol = columns.begin()+strFirstCol; 
				while (icol != columns.begin()+strFirstCol+colCount && icol != columns.end()) { 
					if (onlyCategories) { 
						if (icol->type == FT_CATEGORY || icol->categories.size() < 3)
							icol->categories.addString (*ival);
					}
					else if (icol->use != CU_NO) {
						if (icol->translate != FTR_BOOLS)
							icol->val(*pattern) = atof (ival->c_str());
						else
							icol->preprocess( *ival, icol->val(*pattern) );
						switch ((int)icol->use) {
						case CU_INPUT: iInput += icol->getColWidth(); break;
						case CU_OUTPUT:iOutput += icol->getColWidth(); break;
						}
					}
					++icol;
					++ival;
				}
				pattern->series = series;
				if (!strFirstCol && !onlyCategories)
					data.push_back (*pattern);
				++iRow;
			}
			strFirstInput = iInput; strFirstOutput = iOutput;
			file.close();
			if (nRow && nRow != iRow) errors += CString("File ")+streamIter->value+" - row count doesn't match (should be "+toString(nRow)+")\n";
			if (nRow == 0) nRow = iRow;
		}
	}
	
	if (!onlyCategories)
		errors += processRanges();
	else {
		for (TColumns::iterator icol = columns.begin(); icol != columns.end(); ++icol) {
			if (icol->categories.size() < 3 && icol->translate == FTR_LINEAR)
				icol->translate = FTR_FLOAT;
			if (icol->type != FT_CATEGORY)
				icol->categories = "";
		}
	}
	return errors;
}

void CTrainingData::setColIndexes ()
{
	colIndexes[CU_INPUT].DeleteAll();
	colIndexes[CU_OUTPUT].DeleteAll();
	int pos[2] = {0,0};

	int index = 0;
	TColumns::iterator icol;
	for (icol = columns.begin(); icol != columns.end(); ++icol, ++index) 
	if (icol->use == CU_INPUT || icol->use == CU_OUTPUT) {
		icol->pos = (CInt)pos[icol->use];
		pos[icol->use] += icol->getColWidth();
		colIndexes[icol->use].push_back (index);
	}

	colCount[CU_INPUT] = colCount[CU_OUTPUT] = 0;
	for (icol = columns.begin(); icol != columns.end(); ++icol) {
		if (icol->use == CU_INPUT || icol->use == CU_OUTPUT)
			++colCount[icol->use];
	}
}

void CTrainingData::addColumn (TColumn col)	
{ 
	int pos = 0;
	TColumns::iterator icol;
	for (icol = columns.begin(); icol != columns.end(); ++icol)
		if (icol->use == col.use) 
			pos += icol->getColWidth();
	col.pos = (CInt)pos;
	if (col.use == CU_INPUT || col.use == CU_OUTPUT)
		colIndexes[col.use].push_back (columns.size());
	columns.push_back (col);
}

void CTrainingData::addRow (enumSetType set, int series)
{
	int inp = 0, outp = 0;
	TPattern pat;
	if (data.size()) {
		inp = data.begin()->data[CU_INPUT].size();
		outp= data.begin()->data[CU_OUTPUT].size();
	}
	else {
		TColumns::iterator icol;
		for (icol = columns.begin(); icol != columns.end(); ++icol) {
			if (icol->use == CU_INPUT)
				inp += icol->getColWidth();
			else if (icol->use == CU_OUTPUT)
				outp += icol->getColWidth();
		}
	}
	pat.data[CU_INPUT].Realloc (inp);
	pat.data[CU_OUTPUT].Realloc (outp);
	pat.set = set;
	pat.series = (CInt)series;
	data.push_back(pat);
	pos = data.end()-1;
}

CString CTrainingData::readColumns (const CRox *xml)
{
	CString err = xml_read_container (xml,columns,"columns");
	setColIndexes ();
	return err;
}

CRox *CTrainingData::printColumns () const
{
	return xml_print_container (columns,"columns");
}

void CTrainingData::dumpData (CString &out)
{
	out = "";
	if (data.size() == 0) return;
	int iseries = data.begin()->series;
	CString myval;
	TPatterns::iterator pos;
	TColumns::iterator col;
	for (pos = data.begin(); pos != data.end(); ++pos) {
		if (series && pos->series != iseries) {
			out += seriesSeparator + "\n";
			iseries = pos->series;
		}
		for (col = columns.begin(); col != columns.end(); ++col) {
			if (col->use != CU_NO) {
				col->postprocess (col->val(*pos), myval);
				out += myval + "\t";
			}
		}
		out += "\n";
	}		
}	

CRox *CTrainingData::printAllLocal ()
{
	TStream str;
	str.encoding = SE_TEXT;
	str.type = ST_LOCAL;
	str.delimiter = "\t";

	files.DeleteAll();
	TTrainingDataFile f;
	f.streams.push_back (str);
	files.push_back (f);
	dumpData (files[0].streams[0].value);

	return print ();
}

CString CTrainingData::readData(CString filename)
{
	CString errors;
//	if (dataMutex.Trylock())
//		return "CTrainingData::readData failed - data are in use.";
	setColIndexes ();
	if (examineColumns)
		errors = readFromFiles(filename, true);
	errors += readFromFiles(filename);
	if (examineColumns) examine();
	processWindows();
//	dataMutex.Unlock();
	return errors;
}
