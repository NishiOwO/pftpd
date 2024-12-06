/* $Id$ */

%{
#include "y.tab.h"

int yylex();
int yyerror(const char*);
%}

%token CIDR NEWLINE STRING

/* Sections */
%token SUBNET GLOBAL

/* Directives */
%token WELCOME ROOT STOP PASS ALLOWANON DENYANON ALLOWLOCAL DENYLOCAL

%start list

%%

list		: list_component
		| list list_component;

list_component	: subnet_block NEWLINE
		| global_block NEWLINE
		| NEWLINE;

subnet_block	: SUBNET spaces CIDR spaces '{' NEWLINE directives '}'
		| GLOBAL spaces '{' NEWLINE directives '}';

directives	: directive
		| directives directive;

directive	: WELCOME spaces STRING NEWLINE
		| ROOT spaces STRING NEWLINE
		| STOP NEWLINE
		| PASS NEWLINE
		| ALLOWANON NEWLINE
		| DENYANON NEWLINE
		| ALLOWLOCAL NEWLINE
		| DENYLOCAL NEWLINE
		| NEWLINE;

spaces		: space
		| spaces space;

space		: ' '
		| '\t';

%%
