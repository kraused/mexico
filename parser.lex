
/* vi: set tabstop=4 */

%option yylineno
%option noyywrap
%option prefix="mexico_"

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.tab.hpp"

%}

%%


"&"									{ return AMPERSAND; }
"/"									{ return SLASH; }
","									{ return COMMA; }
"="									{ return EQUAL; }

[ \n\t]								{ /* skip whitespace */ }

".true." 	|
".TRUE." 							{ mexico_lval.bval = 1; return BOOL; }

".false."	|
".FALSE."							{ mexico_lval.bval = 0; return BOOL; }

  /* Identifier string */
[a-zA-Z][a-zA-Z0-9_]+				{ snprintf( mexico_lval.sval, sizeof(mexico_lval.sval), "%s", yytext);
                              		  return ID; }

  /* Quoted string */
\"[a-zA-Z0-9_,.-/ ]*\"	\
\'[a-zA-Z0-9_,.-/ ]*\'	 			{ snprintf( mexico_lval.sval, sizeof(mexico_lval.sval), "%s", yytext+1 );
                               	      mexico_lval.sval[strlen(mexico_lval.sval)-1] = '\0';
                               		  return STRING; }

  /* Numbers */
-?[0-9]+							{ mexico_lval.ival = atoi(yytext);
                                      return NUM; }
-?[0-9]+"."[0-9]*             |
-?"."[0-9]+                   |
-?[0-9]+E[-+]?[0-9]+          |
-?[0-9]+"."[0-9]*E[-+]?[0-9]+ |
-?"."[0-9]+E[-+]?[0-9]+           	{ mexico_lval.fval = atof(yytext);
                                      return FLOAT; }

  /* Handle comments */
!.*\n                     			{ }


%%

int mexico_error(const char* msg)
{
	fprintf(stderr, "error at %d: %s at '%s'\n", mexico_lineno, msg, mexico_text);
	return 1;
}

