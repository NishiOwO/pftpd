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

pftpd_sec_t* pftpd_create_section(void);

#endif
