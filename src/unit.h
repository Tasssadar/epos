/*
 *	epos/src/unit.h
 *	(c) geo@cuni.cz
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
 *	This file defines the "main" structure we use as the internal structure
 *	for representing the text we're going to process. One instance of this 
 *	class can be a segment, phone, syllable, ... , whole text (see UNIT
 *	defined in epos.h), depending on its "depth". Its contents is a
 *	bi-directional linked list between "firstborn" and "lastborn"; all of
 *	its elements have their "depth" lower by one. These and maybe other
 *	assumptions about the structure can be found in unit::sanity().
 */
 
#define LITERAL_ZERO		'0'
#define ABSENT_CHARACTER	'\031'

#define JUNCTURE      ABSENT_CHARACTER    // scope boundary in assim environment
#define DELETE_ME     ABSENT_CHARACTER    // changing cont to this one is fatal

#define RATIO_TOTAL	   100	  // 100 % (unit::smooth percent sum)

// #define MAX_GATHER       16384          // Maximum word size (for buffer allocation)
// #define MAX_GATHER       16          // Maximum word size (for buffer allocation)

#define SMOOTH_CQ_SIZE    16		// max smooth request length, see unit::smooth()

class marker;
class CNeuralNet;

class unit
{
	friend void epos_catharsis();	  // necessary only #ifdef WANT_DMALLOC
	friend class r_inside;
	friend class t_neuralnet;	  // neuralnet directly writes into f,i,t

    public:
	unit *next, *prev;                //same layer
	unit *firstborn, *lastborn;       //layer lower by one
	unit *father;                     //layer greater by one
	UNIT depth;        // 0=segment 1=phone 2=syllable 3=word...
	bool scope;                       //true=don't pass on Next/Prev requests
	short int  cont;         // content (or terminating character)
	void insert_begin(unit *member, unit*to);         //insert a train of units
	void insert_end(unit *from, unit *member);        //insert as the last member
				      //if NULL, shrieks unintuitively
	void set_father(unit *new_fath);  //brothers will do that as well
	void fdump(FILE *outf);           //only for debugging
	void nnet_dump(FILE *outf);
	float nnet_type();
    
	inline bool subst(hash *table,	char*s2b,char*s3b); //inner, see implem.
	inline void subst();          //replace this unit by sb
	bool subst(hash *table, regex_t *fastmatch);	// M_SUBSTR stuff
	void syll_break(char *sonority, unit *before);
	void syllabify(char *sonority);  //May split "father" just before "this", if sonority minimum
	void analyze(UNIT target, hash *table, int unanal_unit_penalty, int unanal_part_penalty);
	void assim(charxlat *fn, charclass *left, charclass *right);
//	void sseg(hash *templates, char symbol, int *quantity);
	void seg(hash *segm_inventory);  //Will create up to one segment. Go see it if curious.
    
	void sanity() { if (!scfg->trusted) do_sanity(); };
	void do_sanity();                 //check that this unit is sanely linked to the others
	void insane(const char *token);   //called exclusively by sanity() in case of a problem
    
//	int f,i,t;
	float t;
	marker *m;
//  public:
		unit(UNIT layer, parser *);
		unit(UNIT layer, int content); 
		unit();               //(empty unit) constructor
		~unit();
	void delete_children();
	int  write_segs(segment *whither, int starting_at, int max);
                                      //Writes the segments out to an array of
                                      // struct segment. Returns: how many written
                                      // starting_at==0 for the first segment
	int write_ssif_head(char *whither); // ^ how many chars written
	int write_ssif(char *whither, int starting_at, int bs);
	void show_phones();	      // printf() the phones
	void nnet_out(const char *filename, const char *dirname);
	void filedump (char *filename);      // for external use of hierarchy information
	void dumpunitrecursive (FILE *outf);
	void fout(char *filename);        //stdout if NULL
	void fprintln(FILE *outf);        //does not recurse, prints cont,f,i,t
	char *gather(char *buffer_start, char *buffer_end, bool suprasegm);
	char *gather(bool delimited, bool suprasegm);	// returns in gb, gblen
             // gather() returns the END of the string (which is unterminated!)
        void insert(UNIT target, bool backwards, char what, charclass *left, charclass *right);
	bool subst(hash *table, regex_t *fastmatch, SUBST_METHOD method);
	bool relabel(hash *table, regex_t *fastmatch, SUBST_METHOD method, UNIT target);				      
#ifdef WANT_REGEX
	void regex(regex_t *regex, int subexps, regmatch_t *subexp, const char *repl);
#endif
	void assim(UNIT target, bool backwards, charxlat *fn, charclass *left, charclass *right);
	void split(unit *before);         //Split this unit just before "before"
                                      //not too robust
	void syllabify(UNIT target, char *sonority);
                                      // Will split units (syllables),
                                      // according to sonority[cont] of "target"
                                      // units (phones) contained there
        bool contains(UNIT target, charclass *set);
//        void sseg(UNIT target, hash *templates);
        			      // Take freq, time or intensity from the hash*
        void absol(hash *dict, UNIT target);
	void prospoint(FIT_IDX what, int value, float position);
	void contour(UNIT target, int *recipe, int rec_len,
			int padd_start, FIT_IDX what, bool additive);
        void smooth(UNIT target, int *ratio, int base, int len, FIT_IDX what);
	void project_extents();
        void project_one_level(float sum);
        void project(UNIT target);
        void raise(charclass *what, charclass *when, UNIT whither, UNIT whence);
        			      // Move characters between levels
	void segs(UNIT target, hash *segm_inventory);
                                      //Will create the segments
	void unlink(REPARENT rmethod);//Delete this unit, possibly reparenting children 
	int  forall(UNIT target, bool userfn(unit *patiens));
				      //^how many times applied
				      // userfn ^ whether applied
	int effective(FIT_IDX which_quantity);  //evaluate total F, I or T
	inline unsigned char inside() { return (unsigned char)cont; };
	inline unsigned char inside_or_zero() { return cont == ABSENT_CHARACTER ? LITERAL_ZERO : inside(); };
	unit *ancestor(UNIT level);   // the unit (depth level) wherein this lies
	int  index(UNIT what, UNIT where);
	int  count(UNIT what);
                                      //The following four ones use indirect recursion
	unit *RightMost(UNIT target);     //If no targets inside, will try Next->RightMost 
	unit *LeftMost(UNIT target);      //  (or Prev->LeftMost); if no targets within 
                                      //  the scope, returns EMPTY
	unit *Next(UNIT target);          //If no targets follow (or precede) within 
	unit *Prev(UNIT target);          //  the current scope, returns EMPTY

	void *operator new(size_t size);
	void operator delete(void *ptr);

	static char *gb;		// gather buffer
	static int gbsize;			// allocated size
	static int gblen;			// actual data
	static char *sb;		// subst buffer
	static int sbsize;
	static void done();		// free buffers
	static void assert_sbsize(int);	// ensure at least that big sbsize

	int getCont () const { return cont; }
	int getDepth () const{ return depth; }

	void neural (UNIT target, CNeuralNet *); 		//JA: Apply neural network described in some cfg file
//	int getF () const	{ return f; }
//	void setF (int ff)	{ f = ff; }
};


/****************************************************************************
 unit::Right/LeftMost
 ****************************************************************************/
 
extern unit EMPTY;

inline unit*
unit::RightMost(UNIT target)
{
	sanity();
	if(target == depth || this == &EMPTY) return this;
	if(!lastborn) return scope ? &EMPTY : Prev(depth)->RightMost(target);
	return(lastborn->RightMost(target));
}

inline unit*
unit::LeftMost(UNIT target)
{
	sanity();
	if(target == depth || this == &EMPTY) return this;
	if(!firstborn) return scope ? &EMPTY : Next(depth)->LeftMost(target);
	return(firstborn->LeftMost(target));
}

/****************************************************************************
 unit::Next/Prev
 ****************************************************************************/
	
inline unit *
unit::Next(UNIT target)
{
	sanity();
	if (scope)  return &EMPTY;                //never cross the scope
	if (next)   return next->LeftMost(target);  //next exists, but is not the target
	if (father) return(father->Next(target));
	return &EMPTY;
}

inline unit *
unit::Prev(UNIT target)
{
	sanity();
	if (scope)  return &EMPTY;
	if (prev)   return prev->RightMost(target);
	if (father) return father->Prev(target);
	else return &EMPTY;
}

inline int
unit::index(UNIT what, UNIT where)
{
	int i=0;
	if (what > where) shriek(862, "Wrong order of arguments to unit::index");
	if (what < depth) shriek(862, "Underindexing in unit::index %d",what);
	unit *lookfor = ancestor(what);
	unit *lookin = ancestor(where);
	unit *tmpu;
	lookin->scope = true;
	for (tmpu = lookin->LeftMost(what); tmpu != lookfor; tmpu = tmpu->Next(what)) i++;
	lookin->scope = false;
	return i;
}


// extern char * _subst_buff;
// extern char * _gather_buff;
extern unit * _unit_just_unlinked;

void shutdown_units();
