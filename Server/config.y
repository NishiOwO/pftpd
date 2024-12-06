/* $Id$ */

%{
#include "pftpd.h"
#include "y.tab.h"

int yylex();
int yyerror(const char*);
%}

%union {
	char* value;
}

%token CIDR STRING NEWLINE

/* Sections */
%token SUBNET GLOBAL GROUP

/* Directives */
%token WELCOME ROOT STOP PASS ALLOWANON DENYANON ALLOWLOCAL DENYLOCAL

%start list

%%

list		: list_component
		| list list_component;

list_component	: subnet_block NEWLINE
		| global_block NEWLINE
		| NEWLINE;

subnet_block	: SUBNET spaces CIDR spaces '{' NEWLINE directives '}'	{
}
		| GLOBAL spaces '{' NEWLINE directives '}'		{
}
		| GROUP spaces STRING spaces '{' NEWLINE directives '}'		{
};

directives	: directive
		| directives directive;

directive	: WELCOME spaces STRING NEWLINE		{
}
		| ROOT spaces STRING NEWLINE		{
}
		| GROUP spaces STRING NEWLINE		{
}
		| STOP NEWLINE				{
}
		| PASS NEWLINE				{
}
		| ALLOWANON NEWLINE			{
}
		| DENYANON NEWLINE			{
}
		| ALLOWLOCAL NEWLINE			{
}
		| DENYLOCAL NEWLINE			{
}
		| NEWLINE;

spaces		: space
		| spaces space;

space		: ' '
		| '\t';

%%
