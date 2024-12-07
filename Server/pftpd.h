/* $Id$ */

#ifndef __PFTPD_H__
#define __PFTPD_H__

typedef struct pftpd_section {
	char* welcome;
	char* root;
	char* group;
	int pass;
	int allow_anon;
	int allow_local;
	void* next;
} pftpd_sec_t;

typedef struct pftpd_entry {
	char* name;
	pftpd_sec_t* section;
} pftpd_entry_t;

/* pftpd.c */

pftpd_sec_t* pftpd_create_section(void);

/* passwd.c */

int pftpd_init_user(void);
char* pftpd_find_user(int uid);

#endif
