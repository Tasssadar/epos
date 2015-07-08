#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	char *string_val;
	CXml *xml_val;
	CXml::TAttr *attr_val;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	STRING	257
# define	QUOTED_STRING	258
# define	COMMENT_STRING	259
# define	PCDATA_STRING	260
# define	SPACE_STRING	261


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
