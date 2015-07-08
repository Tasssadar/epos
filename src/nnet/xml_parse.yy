%{
#define yyparse xmlparse
#define yylex xmllex
#define yyerror xmlerror
#define yylval xmllval
#define yychar xmlchar
#define yydebug xmldebug
#define yynerrs xmlnerrs

#include "epos.h"
#include "neural.h"
#include "utils.h" // toString
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#define YYPARSE_PARAM xml
//to make debug information visible, uncomment the next bison_row 
//and assign somewhere yydebug=1
#define YYDEBUG 1

extern CXml *bison_xml_result;
extern CString sxml;
extern int ixml;
bool inside_tag;

int yylex ();
int yyerror (const char *s);

%}
     
/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                    SYMBOLS                        */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */

%token_table
//%raw bison nefunguje s raw! (aspon yylex ne)

%union {
	char *string_val;
	CXml *xml_val;
	CXml::TAttr *attr_val;
}

%token <string_val> STRING
%token <string_val> QUOTED_STRING
%token <string_val> COMMENT_STRING
%token <string_val> PCDATA_STRING
%token <string_val> SPACE_STRING

%type <xml_val> tag;
%type <xml_val> tagstart;
%type <string_val> tagend;
%type <xml_val> tagclosed;
%type <xml_val> tags;
%type <xml_val> attrs;
%type <attr_val> attr;

//%expect 1 //let bison expect one shift/reduce conflict

%%

/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                    INPUTS                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */



all: { yydebug = 0; }
	 tag						{ bison_xml_result = $2; }
;
tagstart: '<' STRING '>'		{ $$ = new CXml ($2); }
	    | '<' STRING attrs '>'  { $3->SetTag ($2); $$ = $3; }
;
tagend: '<' '/' STRING '>'		{ $$ = $3; }
;
tagclosed: '<' STRING '/' '>'	{ $$ = new CXml ($2); }
		 | '<' STRING attrs '/' '>' { $3->SetTag ($2); $$ = $3; }
;
tag: tagstart tagend		
		{ 
		  /* if ($1->Tag() != $2) yyerror (CString("Tags do not match: ")+$1->Tag()+" x "+$2); */
			$$ = $1;
		}
   | tagstart tags tagend
		{
		  /* if ($1->Tag() != $3) yyerror (CString("Tags do not match: ")+$1->Tag()+" x "+$3);*/
			for(int i = 0; i < $2->NChildren(); i ++) 
				$1->AddChild ($2->Child(i));
			delete $2;
			free ($3);
			$$ = $1;
		}
   | tagclosed				{ $$ = $1; }
   | COMMENT_STRING			{ $$ = new CXml ($1); $$->SetSubtype(COMMENT); }
   | PCDATA_STRING			{ $$ = new CXml ($1); $$->SetSubtype(TEXT); }
;
tags: tag						
		{ 
			$$ = new CXml();
			$$->AddChild (*$1);	
			delete $1;
		}
	| tags tag
		{
			$1->AddChild (*$2);
			delete $2;
			$$ = $1;
		}
;
attrs: attr
		{
			$$ = new CXml();
			$$->AddAttr ($1->name, $1->value);
			delete $1;
		}
	 | attrs attr
		{
			$1->AddAttr ($2->name, $2->value);
			delete $2;
			$$ = $1;
		}
;
attr: STRING '=' QUOTED_STRING	{ $$ = new CXml::TAttr ($1, $3); }
;
%%
     
#include <ctype.h>


/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                     YYERROR                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */

int yyerror (const char *s)
{
	shriek (812, fmt ("BISON:yyerror:xml parser: XML is errornous. %s\n", s));
	return -1;
}
 

/* * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                      YYLEX                        */
/* * * * * * * * * * * * * * * * * * * * * * * * * * */

int yylex ()
{
	//DBG (0,10,fprintf(STDDBG,"yylex source: %s\n", bison_row));

	if (ixml >= (int)strlen(sxml))
		return 0; // EOF

	if (ixml == 0)
		inside_tag = false;

	CString lex;
	int lexstart = ixml;

	// comments
	if (sxml.substr (ixml,4) == "<!--") {
		while (ixml < sxml.length()-2 && sxml.substr (ixml,3) != "-->")
			ixml ++;
		if (sxml.substr (ixml,3) == "-->") {
			ixml += 3;
			yylval.string_val = strdup (sxml.substr(lexstart+4,ixml-lexstart-4));
			return COMMENT_STRING;
		}
		else yyerror ("Unterminated comment");
	}

	if (inside_tag) {
		// eat whitespace
		while (ixml < sxml.length() && isspace (sxml[ixml])) 
			ixml ++;
		lexstart = ixml;
	}

	if (sxml[ixml] == '<')
		inside_tag = true;
	else if (sxml[ixml] == '>')
		inside_tag = false;

	if (strchr ("</>=", sxml[ixml]))
		return sxml[ixml ++];

/*
		for (int i = 0; i < YYNTOKENS; i++) {
			if (yytname[i] == '"' + lex + '"')
			  return yytoknum [i]; //this is an undocumented feature: you must use yytoknum
		}
*/	

	//quoted string - return without quotas
	if (strchr ("\"'", sxml[ixml])) {
		char quote = sxml[ixml];
		do ixml ++;
		while (ixml < sxml.length() && (sxml[ixml] != quote || sxml[ixml-1] == '\\'));
		if (sxml[ixml] == quote) {
			ixml ++;
			yylval.string_val = strdup (sxml.substr(lexstart+1,ixml-lexstart-2));
			if (yydebug) printf ("\n*** QUOTED %s\n", yylval.string_val);
			return QUOTED_STRING;
		}
		else yyerror ("Unterminated quotas");
	}

	// other string or PCDATA

	if (inside_tag) {
		// eat whitespace
		while (ixml < sxml.length() && isspace (sxml[ixml])) 
			ixml ++;
		lexstart = ixml;

		// find word
		while (ixml < sxml.length() && isalnum(sxml[ixml]) || sxml[ixml] == '_' || sxml[ixml] > 127)
			ixml ++;

		yylval.string_val = strdup (sxml.substr(lexstart,ixml-lexstart));
		if (yydebug) printf ("\n*** STRING: %s\n", yylval.string_val);
		return STRING;
	}

	else {
		while (ixml < sxml.length() && sxml[ixml] != '<')
			ixml ++;
		lex = sxml.substr(lexstart,ixml-lexstart);

		// only spaces?
		int i;
		for (i = 0; i < lex.length() && isspace(lex[i]); i ++);

		// no 
		if (i < lex.length()) {
			if (sxml[ixml] != '<')
				yyerror ("Unterminated PCDATA (expected '<' but EOF encountered).");
			yylval.string_val = strdup (lex);
			return PCDATA_STRING;
		}

		// yes => don't generate PCDATA
		else if (sxml[ixml] == '<') {
			ixml ++;
			inside_tag = true;
			return '<';
		}
		else return 0; // EOF			
	}
}
	
 
