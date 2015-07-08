/** @file neural.cc
	 @author Jakub Adamek
	 @date 11/5/2001
	 @version 2.0

	 @brief This file implements the structures defined in neural.h
*/

/* IMPORTANT: You must define EPOS in order to use this file - it is used in vector.cc!!! */

//This file contains markups which allow the Doxygen documentation generator to work on it

/*
 *	epos/src/neural.cc
 *	(c) 2000 jakubadamek@bigfoot.com
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *
 */
//Optimized for tab width: 3

#ifndef EPOS_NEURAL_CC
#define EPOS_NEURAL_CC

#include "epos.h"
#include "neural.h"
#include "xmlutils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

/*
REGSTRUCT (TChar2Float)
REGSTRUCT (CExpression)

REGVECTOR (TChar2Float, TChar2Floats)
REGVECTOR (CExpression, CExpressions)
REGVECTOR (CInt, TNNOutputs)
REGARRAY (CFloat, 256, TFloats256)
*/
extern unit EMPTY; 	//unit.cc

const int MAX_LENGTH_STATEMENT = 250;
double const TChar2Float::CHARTOFLOAT_NULL = -100;

// name of chartofloat defining sonority in config file
static const char CHARTOFLOAT_SONORITY[] = "sonority";

int neuralparse (void *neuralnet);  //BISON

inline UNIT get_level (const char *level_name)
{
	return str2enum (level_name, scfg->unit_levels, U_DEFAULT);
}

CNeuralNet::~CNeuralNet()
{
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	CNeuralNet::read
/**
   Reads the XML config file. */
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void
resolve_vars(char *line, hash *vars, text *file = NULL); //defined in block.cc

extern CString bison_row_buf;
extern const char *bison_row;
extern CExpression *bison_input_result;
extern CNeuralNet *bison_nnet;
	
CString CNeuralNet::read (CRox &xml) 
{
	CString err, s;
	CRox *ch, *ch1, *ch2;
	CFloat f, ch2f_default;
	TChar2Float ch2f;
	char tmp[1000], *ptmp;
	int i, ichar, ich2f;

	xml.AddIncludes(compose_pathname(filename, this_lang->rules_dir, scfg->lang_base_dir));
	xml.AddDefaults();

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/*                        CHAR 2 FLOATS				 */
	
	ch = &xml["char2floats"];	
	if (ch->Exists()) 
	for (ich2f = 0; ich2f < ch->NChildren(); ++ich2f) 
	if (ch->Child(ich2f).Tag() != "char2float") {
		ch1 = &ch->Child(ich2f);
		err += ch1->GetAttr ("value",ch2f.name);
		ch2f_default = TChar2Float::CHARTOFLOAT_NULL;
		err += xml_read (ch1, ch2f_default, "default", false);

		for (ichar = 0; ichar < 256; ++ichar)
			ch2f.val [ichar] = ch2f_default;
		
		err += xml_read (ch1, ch2f.empty, "empty", false);

		for (i=0; i < ch1->NChildren(); ++i) 
		if (ch1->Child(i).Tag() != "chars") {
			ch2 = &ch1->Child(i);
			err += xml_read (ch2,s,"src");
			err += ch2->GetAttr ("value",f);
			tmp[0] = 'a'; //'a' or anything else
			if (s.length()) {
				strcpy (tmp+1, s.c_str());
				tmp[strlen(tmp)+1] = 0;
				resolve_vars (tmp, vars);
				for (ptmp = tmp+1; *ptmp; ++ptmp)
					ch2f.val [(unsigned char)*ptmp] = f;
			}
		}
		char2floats.push_back (ch2f);
	}
	if (err.length()) return err;

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/*                 TRAINDATA and PERCEPTRON_NN			*/

	err += xml_read (&xml, log, "epos_log", false);
	filename = "";
	err += xml_read (&xml, filename, "epos_nn", false);
	trDataPrepare = filename.length() == 0;

	if (!trDataPrepare) {
		XMLFile xmlFile;
		xmlFile.setfile (compose_pathname(filename, this_lang->rules_dir, scfg->lang_base_dir));
		CRox *holder = xmlFile.parse();
		err += perceptronNN.read_all (holder, compose_pathname(filename, this_lang->rules_dir, scfg->lang_base_dir), &trData);
		perceptronNN.initNeurons( );
		if (err.length()) return err;
	}
	if (err.length()) return err;

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/*                          COLUMNS				 */

	int iinp;

	ch = &xml["columns"];
	if (ch->Exists()) {
		inputs.DeleteAll();
		outputs.DeleteAll();
		logUnits.DeleteAll();
		for (iinp = 0; iinp < ch->NChildren(); ++iinp)
		if (ch->Child(iinp).Tag() == "column") {
			ch1 = &ch->Child(iinp);
			CInt use;
			err += xml_read_enum (ch1, use, CUs, "use");
			if (use == CU_INPUT) {
				err += xml_read (ch1, bison_row_buf, "epos");
				bison_row = bison_row_buf.c_str();
				bison_nnet = this;

				/* * * * * * * * * * * * * * * * * * * */
				/*   Running the BISON parser          */
				neuralparse (this);
				
				inputs.push_back (*bison_input_result);
				delete bison_input_result;
			}
			else if (use == CU_OUTPUT) {
				err += xml_read (ch1, bison_row_buf, "epos");
				bison_row = bison_row_buf.c_str();
				bison_nnet = this;

				/* * * * * * * * * * * * * * * * * * * */
				/*   Running the BISON parser          */
				neuralparse (this);
				
				outputs.push_back (*bison_input_result);
				delete bison_input_result;

				CInt output;
				err += xml_read_enum (ch1, output, NOs, "epos_output");
				outputs_placement.push_back (output);
			}
			else if (use == CU_NO) {
				CString level;
				int logUnit = -1;
				err += xml_read (ch1, level, "epos_loglevel",false);
				if (level.length()) 
					logUnit = str2enum (level, scfg->unit_levels, -1);
				logUnits.push_back (logUnit);
			}

		}
	}

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/*                          REREAD				 */

	err += xml_read (&xml, log, "epos_log", false);
	err += xml_read (&xml, reread_find, "epos_reread_find", false);
	reread_last_time = time (NULL);
	
	if (trDataPrepare)
		err += trData.read (&xml);

	return err;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	CNeuralNet::init
/**
   Does all the job - prepares network to process data, 
	processes the config file. */
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void
CNeuralNet::init ()
{
	// #ifndef EPOS
	// 	shriek (861, "You must define EPOS when building this file. It is used in Bang 3 vector.cc file!");
	// #endif
	
	if (initialized) return;
	
	/* Running the XML parser */
	XMLFile xmlFile;
	xmlFile.setfile (compose_pathname(filename, this_lang->rules_dir, scfg->lang_base_dir));
	CString err = read (*xmlFile.parse());
	if (err.length())
		shriek (861, CString (CString("Neuralnet rule: ")+err).c_str());

	D_PRINT(2, "Neuralnet initialized.\n");
	initialized = true;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	CNeuralNet::run
// * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void CNeuralNet::run (unit *myunit)
{
	if (inputs.size() != trData.getColCount(CU_INPUT))
		shriek (861,fmt("Input count doesn't match %d Epos neuralnet definition", (int)inputs.size()));
	if (outputs.size() != trData.getColCount(CU_OUTPUT))
		shriek (861,fmt("Output count doesn't match %d Epos neuralnet definition", (int)outputs.size()));

	trData.clear();
 
	if (reread_find.length() && reread_last_time < time (NULL)) {
		reread_last_time = time (NULL);
		CString file = findFirstFileName (compose_pathname(reread_find, this_lang->rules_dir, scfg->lang_base_dir));
		if (file.length() && file != last_reread_find) {
			last_reread_find = file;
			XMLFile xmlFile;
			xmlFile.setfile (file);
			CRox *holder = xmlFile.parse();
			CString err = perceptronNN.read_all (holder, file, &trData);
			perceptronNN.initNeurons( );
			if (err.length()) shriek (862, err);
		}
	}
	
	double *inputVals = new double [inputs.size()];
	unit *subunit = myunit->LeftMost (target);
	int isub, iCol;
	double f, oldF;

	// are we modelling first or second derivatives?
	int nn_derivatives = 0;

	if (trDataPrepare) {
		for (isub = 0; isub < myunit->count (target); isub ++, subunit = subunit->Next (target)) {
			trData.addRow (ST_TRAIN);
			fill_input (myunit, subunit, inputVals);
			for (iCol=0; iCol < trData.getColCount(CU_INPUT); ++iCol)
				trData.fillColumn (CU_INPUT, iCol, inputVals[iCol]);	
		}
	}

	else {
		for (isub = 0; isub < myunit->count (target); isub ++, subunit = subunit->Next (target)) {
			if (isub < nn_derivatives) continue;
			trData.addRow (ST_TRAIN);
			fill_input (myunit, subunit, inputVals);
			for (iCol=0; iCol < trData.getColCount(CU_INPUT); ++iCol)
				trData.fillColumn (CU_INPUT, iCol, inputVals[iCol]);	
		}
		trData.processWindows();
		subunit = myunit->LeftMost (target);
		isub = 0;
		for (trData.moveToSetStart (ST_TRAIN); trData.getSet() == ST_TRAIN; 
			subunit = subunit->Next (target), trData.moveNext (ST_TRAIN)) {

			perceptronNN.runNetwork (trData.getInputs(), isub ++);
			perceptronNN.copyOutputs (trData.modifyOutputs());

			for (iCol=0; iCol < trData.getColCount(CU_OUTPUT); ++iCol) {
				f = outputs [iCol].calculate (myunit, subunit, &trData);
				if (nn_derivatives) {
					if (isub <= nn_derivatives) f = 100;
					else f += oldF;
					oldF = f;
				}
				switch ((int)outputs_placement[iCol]) {
					case NO_NONE: break;
					case NO_FREQUENCE:  
#define UGLY_POSITION 0.99
						subunit->prospoint(Q_FREQ, (int) (f - cfg->pros_neutral[Q_FREQ]), UGLY_POSITION);
						break;
					case NO_NOTDEF:
						shriek (861, "At this point all outputs must be defined.");
				}
			}
		}
	}
			
	// output to log file
 
	bang_ofstream logF;
	if (log.length()) 
		 logF.open (compose_pathname(log, this_lang->rules_dir, scfg->lang_base_dir), bang_ofstream::out | bang_ofstream::app);
	if (logF) {
		subunit = myunit->LeftMost (target);
		isub = 0;
		for (trData.moveToSetStart (ST_TRAIN); trData.getSet() == ST_TRAIN; 
			subunit = subunit->Next (target), trData.moveNext (ST_TRAIN)) {
			int icol[3] = {0,0,0};
			for (TColumns::const_iterator col = trData.getColumns().begin(); col != trData.getColumns().end(); ++col) {
				switch ((int)col->use) {
				case CU_INPUT: 
					trData.getPostprocessed (CU_INPUT, icol[CU_INPUT], f);
					logF << f;
					break;
				case CU_OUTPUT: 
					if (!trDataPrepare) {
						trData.getPostprocessed (CU_OUTPUT, icol[CU_OUTPUT], f);
						logF << f;
					}
					break;
				case CU_NO:
					if (logUnits[icol[CU_NO]] != -1) {
						int isub = 0;
						int logUnit = logUnits[icol[CU_NO]];
						unit *pomunit = subunit->LeftMost (logUnit);
						for (; isub < subunit->count(logUnit); pomunit = pomunit->Next(logUnit), ++isub)
							logF << (char)pomunit->getCont();
					}
					break;
				}	
				logF << "\t";
				icol[col->use] ++;
			}
			logF << "\n";
		}

		if (trData.seriesSeparator) 
			logF << ((const char *) trData.seriesSeparator) << "\n";
		logF.close();
	}

	delete [] inputVals;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	CNeuralNet::fill_input
/** 
   Prepares the inputs for the input layer.
	Calls calculate on all the input trees. */ 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CNeuralNet::fill_input (unit *scope, unit *myunit, double *input_val) 
{
	FILE *infile = NULL;
	if (infilename.length()) {
		infile = fopen (infilename, "a");
		if (!infile) shriek (812,"CNeuralNet::fill_input:Cannot open file for output.");
	}

	if (!initialized) shriek (861, "CNeuralNet::fill_input:Neuralnet not initialized");
	if (log_level_input > (CInt) -1 && infilename.length()) {
		unit *subunit = myunit->LeftMost (log_level_input);
		for (int i_unit = 0; i_unit < myunit->count (log_level_input); ++i_unit, subunit = subunit->Next(log_level_input))
			fprintf (infile, "%c", subunit->getCont());
		fprintf (infile, "\t");
	}
	for (int i_input = 0; i_input < inputs.size(); ++i_input) {
		input_val [i_input] = inputs [i_input].calculate (scope, myunit, &trData);
		if (infile) fprintf (infile,fmt("%s%df ","%.",format_decdigits[i_input]),input_val [i_input]);
	}
	if (infile) {
		fprintf (infile, "\n");
		fclose (infile);
	}
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//	CExpression::calculate 
/**
	Calculates one input tree - all nodes will have values.
	Contains implementations of all the functions defined in enum_func. */	
// * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double
CExpression::calculate (unit *scope, unit *myunit, CTrainingData *trData)
{
	CFloat f;
	unit *pomunit, *iunit;
	int i;

#define par0 (tree->args[0].v())
#define par1 (tree->args[1].v())
#define par2 (tree->args[2].v())
#define float0 double (par0)
#define float1 double (par1)
#define bool0 (double (par0) != 0)
#define bool1 (double (par1) != 0)
#define int0 int (par0)
#define int1 int (par1)
#define int2 int (par2)
#define result tree->v()

	if (head == NULL) make_list ();

	for (CExpression *tree = head; tree; tree = tree->next) {
		switch (tree->function) {
		case fu_value: break;
		case fu_not: 		result = ! bool0; break;
		case fu_multiply:	result = float0 * float1; break;
		case fu_divide:		if (!bool1) shriek (812,"CNeuralNet::calculate_input:division by zero.");
					result = float0 / float1; break;
		case fu_add: 		result = float0 + float1; break;
		case fu_subtract:	result = float0 - float1; break;
		case fu_less:		result = float0 < float1; break;
		case fu_lessorequals:	result = float0 <= float1; break;
		case fu_greater:	result = float0 > float1; break;
		case fu_greaterorequals:result = float0 >= float1; break;
		case fu_equals:		result = float0 == float1; break;
		case fu_notequals:	result = float0 != float1; break;
		case fu_and:		result = bool0 && bool1; break;
		case fu_or:		result = bool0 || bool1; break;
		case fu_count:
			result = myunit->ancestor( par1 )->count( par0 ); break;
		case fu_index:
			result = 1 + myunit->index( par0, par1 ); break;
		case fu_this:
			result = myunit; break;
		case fu_f0:
			result = myunit->effective(Q_TIME); break;
		case fu_cont:
			result = ((unit *)par0)->getCont(); break;
		case fu_next:
		case fu_prev:
		{
			pomunit = par0;
			int level;
			if (!par1.get_value_type()) 
				level = static_cast<const unit *>(par0)->getDepth();
			else level = par1;
			if (tree->function == fu_next)
				for (i=0; i < int2; ++i)
					pomunit = pomunit->Next(level);
			else 
				for (i=0; i < int2; ++i)
					pomunit = pomunit->Prev(level);
			result = pomunit; 
			break;
		}
		case fu_ancestor:
			result = myunit->ancestor (par0); break;
		case fu_chartofloat: 
			if (par0 == &EMPTY) 
				f = tree->i_chartofloat()->empty;
			else {
				f = tree->i_chartofloat()->val[static_cast<const unit *>(par0)->getCont()]; 	
				if (f == TChar2Float::CHARTOFLOAT_NULL) 
					shriek (812, fmt ("CNeuralNet::calculate_input:on char '%c' is not %s defined",static_cast<const unit *>(par0)->getCont(), 
								tree->i_chartofloat()->name.c_str()));
			}
			result = f;
			break;
		case fu_maxfloat: 
		{
			pomunit = myunit->ancestor (par1);
			CFloat maxf = TChar2Float::CHARTOFLOAT_NULL;
			
			int count = pomunit->count (par0);
			pomunit = pomunit->LeftMost (par0);
			int i;
			for (iunit = pomunit, i=0; i < count; ++i, iunit = iunit->Next(par0))
				if (tree->i_chartofloat()->val[iunit->getCont()] > maxf) {
					maxf = tree->i_chartofloat()->val[iunit->getCont()];
					pomunit = iunit;
		 		}
			result = pomunit;
			break;
		}
		case fu_neural:
		{ 
			double f;
			trData->moveToRow (((unit *)par0)->index (((unit *)par0)->getDepth(), scope->getDepth()));
			trData->getPostprocessed (CU_OUTPUT, int1, f);
			result = f;
			break;
		}
		case fu_nothing: shriek (861, "CNeuralNet::calculate_input:fu_nothing ?!");
		default: shriek (861, fmt ("CNeuralNet::calculate_input:non-handled function %i - add to the source code", tree->function));
		}

	}

	return (which_value);
}//calculate_input		

CNeuralNet::CNeuralNet (const char *my_filename, hash *my_vars)
{
  filename = my_filename; 
  vars = my_vars;
  initialized = false;
  log_level_input = -1;

  init ();
}

void TChar2Float::Init ()
{
	for (int c = 0; c < 256; ++c) val [c] = CHARTOFLOAT_NULL;
	empty = 0;
}

void
CExpression::write_list (FILE *file)
{
	if (head == NULL) make_list ();
	head->write_list_node (file);
	fprintf (file,"\n");
}

void
CExpression::write_list_node (FILE *file)
{
	switch (function) {
		case fu_nothing: fprintf(file,"~ "); break;
		case fu_value: v().print(file); fprintf(file," "); break;
		case fu_chartofloat: fprintf(file,"%s ",(char*) (i_chartofloat()->name)); break;
		default:	fprintf(file," func.%u ",(int) function);
	}

	if (next) next->write_list_node (file);
}	

CExpression *
CExpression::make_list (CExpression **new_head)
{
	next = *new_head;
	*new_head = this;
	for (int i=0; i < args.size(); ++i)
		args[i].make_list (new_head);
	head = *new_head;
	return (head);
}

void 
CExpression::make_list ()
{
	CExpression *new_head = NULL;
	head = make_list (&new_head);
}


void CExpression::Init () 
{
	head = NULL;
	next = NULL;
	owner = NULL;
	function = fu_nothing;
}

TTypedValue::TTypedValue(const TTypedValue&x)
{
	init (x);
}

TTypedValue & TTypedValue::operator= (const TTypedValue&x)
{
	init (x);
	return *this;
}

void TTypedValue::init (const TTypedValue &x)
{
	value_type = x.value_type;
	switch (value_type) {
		case 0: break;
		case 'i': int_val = x.int_val; break;
		case 'f': float_val = x.float_val; break;
		case 'c': char_val = x.char_val; break;
		case 'b': bool_val = x.bool_val; break;
		case 's': string_val = strdup (x.string_val); break;
		case 'u': unit_val = x.unit_val; break;
		default: shriek (861, "TTypedValue:Type not handled in init.");
	}
}

TTypedValue::~TTypedValue ()
{ 
	clear ();
}

void TTypedValue::clear ()
{
	switch (value_type) {
		case 0:
		case 'i':
		case 'f':
		case 'c':
		case 'b':
		case 'u':
			break;
		case 's': free(string_val); break;
		default: shriek (861, fmt ("TTypedValue:Type %c (%i) not handled in destructor.",value_type, value_type));
	}
	value_type = 0;
}

void
CExpression::write (FILE *file) 
{
	fprintf (file, "Tree content:\n");
	write (0);
}

void
CExpression::write (int level, FILE *file) 
{	
	int i;
	for (i=0; i<level; ++i) fprintf (file,"...");
	switch (function) {
		case fu_nothing: fprintf (file,"~"); break;
		case fu_value: break;
		case fu_chartofloat: fprintf (file,"%s\t\t", (char*) (i_chartofloat()->name)); break;
		default: fprintf (STDDBG,"Function %i ?! ", function);
	}
	switch (function) {
		case fu_nothing: break;
		default:
			v().print(file); break;
	}
	fprintf (file, "\n");
	
	for (i=0; i < args.size(); ++i)
		args[i].write (level+1,file);
}


void
TTypedValue::print (FILE *file)
{
	switch (value_type) {
		case 's': fprintf (file, "%s", string_val); break;
		case 'f': fprintf (file, "%.2f", float_val); break;
		case 'i': fprintf (file, "%i", int_val); break;
		case 'c': fprintf (file, "%c", char_val); break;
		case 'b': fprintf (file, "%s", bool_val ? "true" : "false"); break;
		case 'u': fprintf (file, "unit var %lu", (unsigned long)(unit_val)); break;
		default: fprintf (file, "Cannot print value type %c", value_type);
	}
}

#endif
