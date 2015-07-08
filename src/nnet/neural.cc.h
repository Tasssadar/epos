typedef union {
	long int int_val;
	double float_val;
	char string_val [MSG_LENGTH];
	unit *unit_val;
	TTypedValue *typed_val;
	CExpression *func_val;
	TFunc func_index;
} YYSTYPE;
#define	EQ	258
#define	NOTEQ	259
#define	LESSEQ	260
#define	GREATEREQ	261
#define	AND	262
#define	OR	263
#define	NOT	264
#define	FLOAT_NUM	265
#define	INT_NUM	266
#define	STRING	267
#define	QUOTED_STRING	268


extern YYSTYPE neurallval;
