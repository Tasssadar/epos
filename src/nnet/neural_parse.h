/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EQ = 258,
     NOTEQ = 259,
     LESSEQ = 260,
     GREATEREQ = 261,
     AND = 262,
     OR = 263,
     NOT = 264,
     FLOAT_NUM = 265,
     INT_NUM = 266,
     STRING = 267,
     QUOTED_STRING = 268,
     COUNT = 269,
     INDEX = 270,
     THIS = 271,
     ANCESTOR = 272,
     PREV = 273,
     NEXT = 274,
     NEURAL = 275,
     BASAL_F = 276,
     CONT = 277
   };
#endif
/* Tokens.  */
#define EQ 258
#define NOTEQ 259
#define LESSEQ 260
#define GREATEREQ 261
#define AND 262
#define OR 263
#define NOT 264
#define FLOAT_NUM 265
#define INT_NUM 266
#define STRING 267
#define QUOTED_STRING 268
#define COUNT 269
#define INDEX 270
#define THIS 271
#define ANCESTOR 272
#define PREV 273
#define NEXT 274
#define NEURAL 275
#define BASAL_F 276
#define CONT 277




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 110 "neural_parse.yy"

	int int_val;
	double float_val;
	char string_val [MSG_LENGTH];
	unit *unit_val;
	TTypedValue *typed_val;
	CExpression *func_val;
	TFunc func_index;



/* Line 2068 of yacc.c  */
#line 106 "neural_parse.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


