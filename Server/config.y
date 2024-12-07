/* $Id$ */

%{
#include "pftpd.h"
#include "y.tab.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

pftpd_sec_t* sec = NULL;

extern int sec_count;
extern pftpd_entry_t** sec_entries;

int yylex();
int yyerror(const char*);
void add_group(const char* str, pftpd_sec_t* section);
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

subnet_block	: SUBNET spaces CIDR spaces '{' NEWLINE directives '}'		{
	char* str = malloc(1 + strlen($<value>3) + 1);
	str[0] = 0;
	strcat(str, "*");
	strcat(str, $<value>3);
	add_group(str, sec);
	sec = NULL;
}
		| GLOBAL spaces '{' NEWLINE directives '}'			{
	char* str = malloc(6 + 1);
	str[0] = 0;
	strcat(str, "global");
	add_group(str, sec);
	sec = NULL;
}
		| GROUP spaces STRING spaces '{' NEWLINE directives '}'		{
	char* str = malloc(1 + strlen($<value>3) + 1);
	str[0] = 0;
	strcat(str, "@");
	strcat(str, $<value>3);
	add_group(str, sec);
	sec = NULL;
};

directives	: directive
		| directives directive;

directive	: WELCOME spaces STRING NEWLINE		{
	if(sec == NULL) sec = pftpd_create_section();
	sec->welcome = $<value>3;
}
		| ROOT spaces STRING NEWLINE		{
	if(sec == NULL) sec = pftpd_create_section();
	sec->root = $<value>3;
}
		| GROUP spaces STRING NEWLINE		{
	if(sec == NULL) sec = pftpd_create_section();
	sec->group = $<value>3;
}
		| STOP NEWLINE				{
	if(sec == NULL) sec = pftpd_create_section();
	sec->pass = 0;
}
		| PASS NEWLINE				{
	if(sec == NULL) sec = pftpd_create_section();
	sec->pass = 1;
}
		| ALLOWANON NEWLINE			{
	if(sec == NULL) sec = pftpd_create_section();
	sec->allow_anon = 1;
}
		| DENYANON NEWLINE			{
	if(sec == NULL) sec = pftpd_create_section();
	sec->allow_anon = 0;
}
		| ALLOWLOCAL NEWLINE			{
	if(sec == NULL) sec = pftpd_create_section();
	sec->allow_local = 1;
}
		| DENYLOCAL NEWLINE			{
	if(sec == NULL) sec = pftpd_create_section();
	sec->allow_local = 0;
}
		| NEWLINE;

spaces		: space
		| spaces space;

space		: ' '
		| '\t';

%%

void add_group(const char* str, pftpd_sec_t* section){
	if(sec_entries == NULL){
		sec_entries = malloc(sizeof(*sec_entries));
		sec_entries[0] = malloc(sizeof(**sec_entries));
	}else{
		pftpd_entry_t** old = sec_entries;
		sec_entries = malloc(sizeof(*sec_entries) * (sec_count + 1));
		int i;
		for(i = 0; i < sec_count; i++) sec_entries[i] = old[i];
		free(old);
		sec_entries[sec_count] = malloc(sizeof(**sec_entries));
	}
	sec_entries[sec_count]->name = malloc(strlen(str) + 1);
	strcpy(sec_entries[sec_count]->name, str);
	sec_entries[sec_count]->section = section;
	sec_count++;
}
