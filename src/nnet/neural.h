/** 	@file neural.h
	@author Jakub Adamek jakubadamek@bigfoot.com
	@date 5/11/2001
	@version 2.0

	@brief This file defines a neuralnet rule to be compiled with Bang 3 source files

	The syntax of the language is desribed in the neural.y bison source file.

	In the current version only feedforward multi-layer perceptron networks are supported.
	But I hope the code is written transparently enough you can easily add some another network architecture.
*/

/* IMPORTANT: You must define EPOS in order to use this file - it is used in vector.cc!!! */

//This file contains markups which allow the Doxygen documentation generator to work on it

/*
 *	epos/src/neural.h
 *	(c) 2000-2002 jakubadamek@bigfoot.com
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 */

//Intendation optimized for tab width: 8 (vim command: set tabstop=8)

#ifndef EPOS_NEURAL_H
#define EPOS_NEURAL_H

#include "nnettypes.h"
#include "perceptron.h"
#include "traindata.h"
#include <stdlib.h>
#include <math.h>

VECTOR (CFloat, TVectorFloat)

///Functions available for describing neuralnet inputs 
enum TFunc { 
	fu_nothing, 	///< empty node
	fu_value,	///< leaf node containing a typed value (as defined farther)
	fu_chartofloat,	///< double myfunction (unit *) .. user-defined char to double function

	fu_add,		///< +
	fu_subtract, 	///< -
	fu_multiply, 	///< *
	fu_divide,	///< / 

	/** Functions operating with units:
	    These functions always operate on the calling unit or on ancestors of it.
		 Epos has it's Text Structure Representation which consists of units organized in levels. E.g. each word-unit has it's ancestor sentence-unit. See more in the Epos documentation.
	 */
	
	/** int count (string level, string up_level), e.g. count("syll","sent")
		 Count of syllables in the sentence. */
	fu_count, 			
	
	/** int index (string level, string up_level), e.g. index("phone","word")
		 Order of the phone in the word starting with 1.			*/
	fu_index, 
	fu_this,	///< unit *this .. the calling unit itself. Useful only to apply a user-defined chartofloat function. 
	
	/** unit *next (string level, int how_many), e.g. next ("syll",2)
		 Syllable two to the right.
		 If there are no more syllables there, it returns the EMPTY unit. */
	fu_next, 			
	fu_prev,	///< like next, but in the opposite direction (to the left)
	
	/** unit *ancestor (string level), e.g. ancestor("sent")
		 Ancestor on the sentence level. */
	fu_ancestor,	
	
	/** unit *maxfloat (string chartofloat, string level1, string level2) finds the unit on level2
	with max chartofloat value (and if there are more possibilities, the rightmost) 
	inside the level1 unit */
	fu_maxfloat,

	/** float neural (unit, int output_index). Takes an output of the neural network */
	fu_neural,

	fu_equals,	///< logical == .. 2 == 2
	fu_notequals,	///< logical != .. 3 != 2
	fu_greater, 	///< logical > .. 3 > 2
	fu_less, 	///< logical < .. 2 < 3
	fu_and, 	///< logical && .. 2 == 2 && 3 == 3
	fu_or, 		///< logical || .. 2 < 3 || 3 < 2
	fu_not,		///< logical ! .. !(2 > 3)
	fu_lessorequals,///< logical <= .. 2 <= 2
	fu_greaterorequals, ///< logical >= .. 5 >= 1
	
	/** Special function: int basal_f (unit *) - f0 of given unit */

	fu_f0,

	/** Unit content */
	fu_cont
};

///User-defined function from char to double. The @b cont of some unit is used as the char.

STRUCT_BEGIN (TChar2Float)
public:
	void Init();
	static const double CHARTOFLOAT_NULL;
	double val[256]; 	///<each char has its value 
	CFloat empty;
	CString name;
STRUCT_END (TChar2Float)

VECTOR (TChar2Float, TChar2Floats);

///Helps to manage different result types and parameter types.
/** 	A nice type-secure typed union, see Stroustrup: Programovaci jazyk C++, p. 175.
  		Contains overtyped operators = and type casting operators. You can use the value in place of a string, int, char, unit, bool, double. 
		
		You have to use the type casting when it is ambiguous whether to convert to more than one type: e.g. when writing <code>val1 / val2</code> ... you have to write e.g. <code>(double) val1 / (double) val2<code>.

		Has built-in type conversion, which allows only sencefull conversions (like int to double) and doesn't allow another ones (like double to int).
*/

class TTypedValue
{
private:
	union {
		char *string_val;
		int int_val;
		double float_val;
		/// only a reference to a TSR unit
		unit *unit_val;
		char char_val;
		bool bool_val;
	};

	//As it is an inline function, the conversion is as quickly as possible:
	void try_convert(char type) { if (!convert (type)) 
		shriek (861,"TTypedValue: demanding non-convertable value type"); }
	char value_type;

public:
	TTypedValue () : value_type (0) {}; 
	TTypedValue (const double x)	{ *this = x; }
	TTypedValue (const int x)	{ *this = x; }
	TTypedValue (unit *x)		{ *this = x; }
	TTypedValue (const char *x)	{ *this = x; }
	TTypedValue (const bool x)	{ *this = x; }
	TTypedValue (const char x)	{ *this = x; }

	TTypedValue(const TTypedValue&x);
	TTypedValue& operator= (const TTypedValue&x); 

	~TTypedValue ();
	// throws away contents
	void clear ();
	// fills with contents from another typed value
	void init (const TTypedValue &x);

	char get_value_type () const { return value_type; }

	void print (FILE * = stdout);

	operator const char * (){ try_convert ('s'); return string_val; }
	operator int ()		{ try_convert ('i'); return int_val; }
	operator double ()	{ try_convert ('f'); return float_val; }
	operator unit * ()	{ try_convert ('u'); return unit_val; }
	operator char ()	{ try_convert ('c'); return char_val; }
	operator bool ()	{ try_convert ('b'); return bool_val; }

	TTypedValue& operator= (const double x)		{ value_type = 'f'; float_val = x; return (*this); }
	TTypedValue& operator= (const int x)		{ value_type = 'i'; int_val = x; return (*this); }
	TTypedValue& operator= (const char x)		{ value_type = 'c'; char_val = x; return (*this); }
	TTypedValue& operator= (const bool x)		{ value_type = 'b'; bool_val = x; return (*this); }
	TTypedValue& operator= (unit *x)		{ value_type = 'u'; unit_val = x; return (*this); }
	TTypedValue& operator= (const char *x)		{ value_type = 's'; string_val = strdup (x); return (*this); }

	/** Tries to convert to another type. Returns whether succeeded.
	  * Used in calculate_input, thus should be very fast, thus inline.
	*/
	
	inline bool convert (char new_type); //result: true = OK, false = not convertable
};

class CNeuralNet;
class CExpression;
VECTOR (CExpression, CExpressions)

/// Expression tree: describes the expression by which a neuralnet input will be calculated
/** The main work - creating the tree - is done in the bison input neural.y
  		
  	 A node may contain:
	 <UL>
	 	 <LI>a constant only - than it is a leaf - or
		 <LI>a function (or user-defined char-to-double) AND (during the evaluation time) a constant - result of the function - than it is "this" function or not a leaf	
	 </UL>	 
*/ 

STRUCT_BEGIN (CExpression)
public:
	void Init();

	CExpressions args;

	CNeuralNet *owner;			///<Used to get the chartofloat user-function definitions
  	TFunc function;			///<Which function does this node represent 
	
	TTypedValue &v ()			{ return which_value; }			
	
	TChar2Floats::iterator i_chartofloat ()	{ return which_char2float; }

	void write (FILE *file = stdout);	///<writes contents of the tree
	/// Calculates the expression on given unit.
	double calculate (unit *scope, unit *, CTrainingData *trData);	

	/** In the list there each node is calculated only after calculating its subnodes (therefore the list begins which some leaf). 
		 This way it allows to go on without any recursion, which spares all the function calling time.	
	*/
	CExpression *head;			///<the head of the whole list (some leaf)
	CExpression *next;			///<next node
	void make_list ();			///<creates the whole list 
	CExpression *make_list (CExpression **head); ///<recursively called on sons and brothers. Returns head of the list.
	void write_list_node (FILE *); ///<recursively writes the rest of the list

	///  Prints the list form of the tree
	void write_list (FILE *file = stdout);

	/// writes with indent appropriate to level .. used by public @b write
	void write (int level, FILE *file = stdout);		
	
	/// Used when function=fu_chartofloat
	TChar2Floats::iterator which_char2float;		
	/// Used when function=fu_value
	TTypedValue which_value;		
STRUCT_END (CExpression)

///What to do with one neuralnet output: where to place it (@b type) and a constant multiplier.

enum TNNOutput {
	NO_NONE, 	///<discards the output - does nothing with it
	NO_FREQUENCE, 	///<overwrites the value of the basal frequence by the output
	NO_NOTDEF		///<not defined
};

static const TEnumString NOs = "none;frequence";

VECTOR (CInt, TNNOutputs)
VECTOR (CInt, TVectorInt)

/** Main part of the neuralnet rule. */

class CNeuralNet
{
public:
	friend int yyparse (void *neuralnet); 			///<BISON parser
/*	friend CExpression *add_prefix_func (TFunc, const char *, CExpression *); 
	friend void read_weight_or_bias_file (char *filename, bool bias);
	friend class CExpression;
	friend void add_chartofloat (const char *name);*/

	/// The constructor only fills initial values - the configuration file is parsed in CNeuralNet::init
  
	CNeuralNet (const char *my_filename, hash *my_vars); 
	~CNeuralNet ();
  	/// Fills the structures with the config file contents. Called from the constructor.
	void init (); 											
	/// Runs the network on some unit and places appropriately the output.
	void run (unit *);									

	/// Returns error description or empty string
	CString read (CRox &xml);
	
	TChar2Floats char2floats;

	/** The output depends on the debugging level set for cfg: if it is less than 2, the output file will contain outputs from all layers.
		 These two variables are used to place the input coming to the net and the outgoing output. */

	CString outfilename, infilename;	 			
	TVectorInt format_decdigits;		///< count of decimal digits printed for each input in input log
  	CInt initialized;			///< bool: helps to avoid multiple initialization
	CInt log_level_input;			///< char: level the units of which will be written into input log

	TNNOutputs outputs_placement;			

	void fill_input (unit *scope, unit *target, double *input_val);	///<fills input_val with numbers calculated from given unit
	hash *vars; 					///<e.g. $voiced = bdïgvz¾Z®hø; got from next_rule in block.cc
	CString filename;												///<config file
	CExpressions inputs, outputs;

	CPerceptronNN perceptronNN;
	CTrainingData trData;
	/** If true, data are dumped to given file and not processed - no perceptronNN specification is needed.
		If false, column specifications are read from given file and perceptronNN used to generate outputs.*/
	CInt trDataPrepare;
	/// file to dump training data when trDataPrepare is on
	CString log;
	/// unit from which to write the content into log
	TVectorInt logUnits;
	/// target level from the neuralnet rule 
	UNIT target;

	/// if the first file name with given mask (e.g. nn_%) changes, it is reread
	CString reread_find;
	CString last_reread_find;
	/// look for the file name only once in a second
	int reread_last_time;
};

///result: true = OK, false = not convertable
inline bool TTypedValue::convert (char new_type)
{
	if (new_type == value_type) return (true);
	switch (new_type) {
		case 'b':
			switch (value_type) {
				case 'c': bool_val = char_val != 0; break;
				case 'i': bool_val = int_val != 0; break;
				case 'f': bool_val = float_val != 0; break;
				default: return (false);
			}
		case 'c':
			switch (value_type) {
				case 'b': char_val = (char) bool_val; break;
				default: return (false);
			}
			break;
		case 'i':
			switch (value_type) {
				case 'b': int_val = (int) bool_val; break;
				case 'c': int_val = (int) char_val; break;
				default: return (false);
			}
			break;
		case 'f':
			switch (value_type) {
				case 'b': float_val = (double) bool_val; break;
				case 'c': float_val = (double) char_val; break;
				case 'i': float_val = (double) int_val; break;
				default: return (false);
			}
			break;
		default:
			return (false);
	}
	value_type = new_type;
	return (true);
}

#endif
