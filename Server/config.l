/* $Id$ */

%{
#include "y.tab.h"
#include <stdio.h>

int yywrap(void){
        return 1;
}

int yyerror(const char* str){
        extern char* yytext;
        fprintf(stderr, "Syntax error near `%s': `%s'\n", yytext, str);
        return 0;
}

%}

%%

%%
