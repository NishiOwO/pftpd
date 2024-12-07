/* $Id$ */

#include "pftpd.h"

#include <stdlib.h>
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
	sec->next = NULL;
	return sec;
}
