/* $Id$ */

#define INCLUDE_NET

#include "pftpd.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

int sec_count = 0;
pftpd_entry_t** sec_entries = NULL;

pftpd_sec_t* pftpd_create_section(void){
	pftpd_sec_t* sec = malloc(sizeof(*sec));
	sec->welcome = NULL;
	sec->root = NULL;
	sec->group = NULL;
	sec->pass = 1;
	sec->allow_anon = -1;
	sec->allow_local = -1;
	return sec;
}

void pftpd_apply_rule(pftpd_state_t* state, struct sockaddr_in claddr, pftpd_entry_t* entry){
	int if_apply = strcmp(entry->name, "global") == 0 ? 1 : 0;
	unsigned char* address = (unsigned char*)&claddr.sin_addr.s_addr;
	unsigned char mask[4];
	if(!state->section->pass) return;
	if(!if_apply){
		if(entry->name[0] == SYMBOL_SUBNET){
			char* cidr = malloc(strlen(entry->name + 1) + 1);
			int i;
			strcpy(cidr, entry->name + 1);
			for(i = 0; cidr[i] != 0; i++){
				if(cidr[i] == '/'){
					int netmask;
					int j;
					int match = 1;
					unsigned char* subnet_addr;
					in_addr_t inet;
					cidr[i] = 0;
					inet = inet_addr(cidr);
					subnet_addr = (unsigned char*)&inet;
					netmask = atoi(cidr + i + 1);
					for(j = 0; j < 4; j++){
						mask[j] = (0xff << (8 - (netmask > 8 ? 8 : netmask))) & 0xff;
						netmask -= netmask > 8 ? 8 : netmask;
					}
					for(j = 0; j < 4; j++){
						if((address[j] & mask[j]) != (subnet_addr[j] & mask[j])){
							match = 0;
							break;
						}
					}
					if(match) if_apply = 1;
					break;
				}
			}
			free(cidr);
		}else if(entry->name[0] == SYMBOL_GROUP){
			if(state->section->group != NULL && strcmp(state->section->group, entry->name + 1) == 0) if_apply = 1;
		}
	}
	if(if_apply){
		if(entry->section->welcome != NULL) state->section->welcome = entry->section->welcome;
		if(entry->section->root != NULL) state->section->root = entry->section->root;
		if(entry->section->group != NULL) state->section->group = entry->section->group;
		if(entry->section->allow_anon != -1) state->section->allow_anon = entry->section->allow_anon;
		if(entry->section->allow_local != -1) state->section->allow_local = entry->section->allow_local;
		state->section->pass = entry->section->pass;
	}
}
