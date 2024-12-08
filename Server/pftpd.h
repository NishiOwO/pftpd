/* $Id$ */

#ifndef __PFTPD_H__
#define __PFTPD_H__

#define USE_POLL

#ifdef INCLUDE_NET
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifdef USE_POLL
#include <sys/poll.h>
#endif

#ifdef USE_SELECT
#include <sys/select.h>
#endif
#endif

#define SYMBOL_SUBNET	'*'
#define SYMBOL_GROUP	'@'

enum FTP_CODES {
	FTP_OK			= 200,
	FTP_NOT_IMPL_FINE	= 202,
	FTP_SYSTEM_STATUS	= 211,
	FTP_DIR_STATUS,
	FTP_FILE_STATUS,
	FTP_HELP,
	FTP_NAME,
	FTP_READY		= 220,
	FTP_LOGOUT,
	FTP_DATA_OPEN		= 225,
	FTP_DATA_SUCCES,
	FTP_ENTER_PASSIVE,
	FTP_LOGIN_SUCCESS	= 230,
	FTP_ACTION_OK		= 250,
	FTP_CREATED		= 257,

	FTP_USERNAME_OK		= 331,
	FTP_NEED_ACCOUNT,
	FTP_ACTION_PENDING	= 350,

	FTP_SERVICE_NOT_AVAIL	= 421,
	FTP_CANT_OPEN_DATA	= 425,
	FTP_CONNECTION_CLOSED,
	FTP_FILE_UNAVAIL	= 450,
	FTP_ACTION_ABORTED,
	FTP_INSUFF_STORAGE,

	FTP_SYNTAX_ERROR_CMD	= 500,
	FTP_SYNTAX_ERROR_PARAM,
	FTP_NOT_IMPL,
	FTP_BAD_SEQUENCE,
	FTP_NOT_IMPL_PARAM,
	FTP_NOT_LOGGED_IN	= 530,
	FTP_NEED_ACCOUNT_STORE	= 532,
	FTP_FILE_NOT_FOUND	= 550,
	FTP_PAGE_TYPE_UNKNOWN,
	FTP_EXCEEDED_STOR_ALLOC,
	FTP_FILE_NOT_ALLOWED
};

typedef struct pftpd_section {
	char* welcome;
	char* root;
	char* group;
	int pass;
	int allow_anon;
	int allow_local;
} pftpd_sec_t;

typedef struct pftpd_entry {
	char* name;
	pftpd_sec_t* section;
} pftpd_entry_t;

typedef struct pftpd_state {
	pftpd_sec_t* section;
} pftpd_state_t;

/* pftpd.c */

pftpd_sec_t* pftpd_create_section(void);
#ifdef INCLUDE_NET
void pftpd_apply_rule(pftpd_state_t* state, struct sockaddr_in claddr, pftpd_entry_t* entry);
#endif

/* passwd.c */

int pftpd_init_user(void);
int pftpd_validate_password(const char* user, const char* password);
char* pftpd_find_user(int uid);

/* server.c */

int pftpd_server_init(void);
int pftpd_server(void);
int pftpd_add_host(const char* host);

#endif
