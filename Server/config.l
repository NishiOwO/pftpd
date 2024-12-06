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

%x STR

%%

\"								{
	BEGIN(STR);
}

<STR>[^\\"]*							{
}

<STR>\\.							{
}

<STR>\"								{
	BEGIN(0);
	return STRING;
}

([ \t]*#[^\n]+)							;

subnet								{
	return SUBNET;
}

global								{
	return GLOBAL;
}

root								{
	return ROOT;
}

welcome								{
	return WELCOME;
}

stop								{
	return STOP;
}

pass								{
	return PASS;
}

allow-anonymous							{
	return ALLOWANON;
}

deny-anonymous							{
	return DENYANON;
}

allow-local							{
	return ALLOWLOCAL;
}

deny-local							{
	return DENYLOCAL;
}

([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\/[0-9]{1,2})	{
	return CIDR;
}

([ \t]*\n[ \t]*)						{
	return NEWLINE;
}

.								{
	return yytext[0];
}

%%
