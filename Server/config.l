/* $Id$ */

%{
#include "y.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yywrap(void){
        return 1;
}

int yyerror(const char* str){
        extern char* yytext;
        fprintf(stderr, "Syntax error near `%s': `%s'\n", yytext, str);
        return 0;
}

char* yytmpbuf;

%}

%x STR

%%

\"								{
	BEGIN(STR);
	yytmpbuf = malloc(1);
	yytmpbuf[0] = 0;
}

<STR>[^\\"]*							{
	char* old = yytmpbuf;
	yytmpbuf = malloc(strlen(old) + strlen(yytext) + 1);
	strcpy(yytmpbuf, old);
	strcpy(yytmpbuf + strlen(old), yytext);
	free(old);
}

<STR>\\.							{
	char* old = yytmpbuf;
	yytmpbuf = malloc(strlen(old) + 2);
	strcpy(yytmpbuf, old);
	strcpy(yytmpbuf + strlen(old), yytext + 1);
	free(old);
}

<STR>\"								{
	BEGIN(0);
	yylval.value = yytmpbuf;
	return STRING;
}

([ \t]*#[^\n]+)							;

subnet								{
	return SUBNET;
}

global								{
	return GLOBAL;
}

group								{
	return GROUP;
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

listen								{
	return LISTEN;
}

([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\/[0-9]{1,2})	{
	yylval.value = malloc(strlen(yytext) + 1);
	strcpy(yylval.value, yytext);
	return CIDR;
}

([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}:[0-9]{1,5})	{
	yylval.value = malloc(strlen(yytext) + 1);
	strcpy(yylval.value, yytext);
	return HOST;
}

([ \t]*\n[ \t]*)						{
	return NEWLINE;
}

.								{
	return yytext[0];
}

%%
