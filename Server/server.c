/* $Id$ */

#define INCLUDE_NET

#include "pftpd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>

extern int sec_count;
extern pftpd_entry_t** sec_entries;

int server_sockets[16];
struct sockaddr_in server_addresses[16];
int server_ports[16];
int server_entries = 0;

#define IS_INVALID_SOCKET(x) ((x) < 0)
#define CLOSE_SOCKET(x) close((x))

int pftpd_server_init(void){
	int yes;
	int i;
	for(i = 0; i < server_entries; i++){
		server_sockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(IS_INVALID_SOCKET(server_sockets[i])){
			fprintf(stderr, "Failed to create a socket\n");
			return 1;
		}
		yes = 1;
		if(setsockopt(server_sockets[i], SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0) {
			CLOSE_SOCKET(server_sockets[i]);
			fprintf(stderr, "Failed to setsockopt (reuseaddr)\n");
			return 1;
		}
		yes = 1;
		if(setsockopt(server_sockets[i], IPPROTO_TCP, TCP_NODELAY, (void*)&yes, sizeof(yes)) < 0) {
			CLOSE_SOCKET(server_sockets[i]);
			fprintf(stderr, "Failed to setsockopt (nodelay)\n");
			return 1;
		}
		server_addresses[i].sin_family = AF_INET;
		server_addresses[i].sin_port = htons(server_ports[i] & 0xffff);
		if(bind(server_sockets[i], (struct sockaddr*)&server_addresses[i], sizeof(server_addresses[i])) < 0) {
			CLOSE_SOCKET(server_sockets[i]);
			fprintf(stderr, "Failed to bind\n");
			return 1;
		}
		if(listen(server_sockets[i], 128) < 0) {
			CLOSE_SOCKET(server_sockets[i]);
			fprintf(stderr, "Failed to listen\n");
			return 1;
		}
	}
	return 0;
}

int pftpd_add_host(const char* host){
	char* cp = malloc(strlen(host) + 1);
	int i;
	strcpy(cp, host);
	for(i = 0; cp[i] != 0; i++){
		if(cp[i] == ':'){
			cp[i] = 0;
			printf("Listening to %s:%s\n", strlen(cp) > 0 ? cp : "0.0.0.0", cp + i + 1);
			server_addresses[server_entries].sin_addr.s_addr = inet_addr(strlen(cp) > 0 ? cp : "0.0.0.0");
			server_ports[server_entries] = atoi(cp + i + 1);
			server_entries++;
		}
	}
	free(cp);
}

int pftpd_timeout(int sock){
#ifdef USE_POLL
	struct pollfd* pollfds = malloc(sizeof(*pollfds));
	int ret;
	pollfds[0].fd = sock;
	pollfds[0].events = POLLIN | POLLPRI;
	ret = poll(pollfds, 1, 30000);
	free(pollfds);
	return ret;
#endif
#ifdef USE_SELECT
	fd_set fdset;
	struct timeval tv;
	int ret;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	ret = select(FD_SETSIZE, &fdset, NULL, NULL, &tv);
	return ret;
#endif
}

int pftpd_write(int sock, int status, const char* message){
	int i;
	int incr = 0;
	char code[4];
	sprintf(code, "%d", status);
	for(i = 0;; i++){
		if(message[i] == '\n' || message[i] == 0){
			send(sock, code, 3, 0);
			send(sock, message[i] == 0 ? " " : "-", 1, 0);
			send(sock, message + incr, i - incr, 0);
			send(sock, "\r\n", 2, 0);
			incr = i + 1;
			if(message[i] == 0) break;
		}
	}
	return 0;
}

int pftpd_read(int sock, char** message){
	char buf[2];
	buf[1] = 0;
	*message = malloc(1);
	*message[0] = 0;
	while(1){
		char* old;
		if(pftpd_timeout(sock) <= 0){
			free(*message);
			*message = NULL;
			return -1;
		}
		if(recv(sock, buf, 1, 0) <= 0){
			free(*message);
			*message = NULL;
			return -1;
		}
		old = *message;
		*message = malloc(strlen(old) + 2);
		strcpy(*message, old);
		strcat(*message, buf);
		free(old);
		if(strlen(*message) >= 2 && (*message)[strlen(*message) - 1] == '\n' && (*message)[strlen(*message) - 2] == '\r'){
			(*message)[strlen(*message) - 2] = 0;
			break;
		}
	}
	return 0;
}

#define PFTPD_WRITE(code,msg) if(pftpd_write(sock, code, msg) != 0) return
#define PFTPD_READ(msg) if(pftpd_read(sock, &msg) != 0) return

void pftpd_handle_socket(int sock, pftpd_state_t* state){
	char* msg = NULL;
	char* login = NULL;
	int logged_in = 0;
	if(state->section->welcome == NULL){
		char welcome[512];
		sprintf(welcome, "pftpd/%s ready (anonymous login %s, local user login %s)", VERSION, state->section->allow_anon ? "allowed" : "not allowed", state->section->allow_local ? "allowed" : "not allowed");
		PFTPD_WRITE(FTP_READY, welcome);
	}else{
		struct stat s;
		FILE* f = fopen(state->section->welcome, "r");
		char* buff;
		if(f == NULL){
			PFTPD_WRITE(FTP_SERVICE_NOT_AVAIL, "Could not open welcome message");
			return;
		}
		stat(state->section->welcome, &s);
		buff = malloc(s.st_size + 1);
		fread(buff, 1, s.st_size, f);
		buff[s.st_size] = 0;
		PFTPD_WRITE(FTP_READY, buff);
		free(buff);
		fclose(f);
	}
	while(1){
		char* cmd = NULL;
		char* arg = NULL;
		int i;
		if(msg != NULL) free(msg);
		PFTPD_READ(msg);
		cmd = msg;
		for(i = 0; msg[i] != 0; i++){
			if(msg[i] == ' '){
				msg[i] = 0;
				arg = msg + i + 1;
				break;
			}
		}
		if(strcmp(cmd, "USER") == 0){
			if(arg == NULL){
				PFTPD_WRITE(FTP_SYNTAX_ERROR_CMD, "USER command requires a parameter");
			}else if(login != NULL){
				PFTPD_WRITE(FTP_BAD_SEQUENCE, "Cannot switch user");
			}else if(strcmp(arg, "ftp") == 0 || strcmp(arg, "anonymous") == 0){
				if(state->section->allow_anon){
					PFTPD_WRITE(FTP_USERNAME_OK, "Anonymous login OK, send your name as password");
					login = malloc(strlen(arg) + 1);
					strcpy(login, arg);
				}else{
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "Anonymous login not allowed");
				}
			}else{
				if(state->section->allow_local){
					PFTPD_WRITE(FTP_USERNAME_OK, "Local user login OK, send your password");
					login = malloc(strlen(arg) + 1);
					strcpy(login, arg);
				}else{
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "Local user login not allowed");
				}
			}
		}else if(strcmp(cmd, "PASS") == 0){
			if(arg == NULL){
				PFTPD_WRITE(FTP_SYNTAX_ERROR_CMD, "PASS command requires a parameter");
			}else if(logged_in){
				PFTPD_WRITE(FTP_BAD_SEQUENCE, "Cannot switch user");
			}else if(login == NULL){
				PFTPD_WRITE(FTP_BAD_SEQUENCE, "Send username first");
			}else if(strcmp(login, "ftp") == 0 || strcmp(login, "anonymous") == 0){
				PFTPD_WRITE(FTP_LOGIN_SUCCESS, "Login successful");
				logged_in = 1;
			}else{
				if(pftpd_validate_password(login, arg) == 0){
					PFTPD_WRITE(FTP_LOGIN_SUCCESS, "Login successful");
					logged_in = 1;
				}else{
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "Login incorrect");
					free(login);
					login = NULL;
				}
			}
			if(logged_in){
				struct passwd* p = getpwnam(login);
				if(state->section->root == NULL){
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "root is not set");
					return;
				}
				if(chdir(state->section->root) != 0){
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "failed to chdir");
					return;
				}
				if(chroot(".") != 0){
					PFTPD_WRITE(FTP_NOT_LOGGED_IN, "failed to chroot");
					return;
				}
				if(strcmp(login, "ftp") != 0 && strcmp(login, "anonymous") != 0){
					if(setuid(p->pw_uid) != 0){
						PFTPD_WRITE(FTP_NOT_LOGGED_IN, "failed to setuid");
						return;
					}
					if(seteuid(p->pw_uid) != 0){
						PFTPD_WRITE(FTP_NOT_LOGGED_IN, "failed to seteuid");
						return;
					}
				}
			}
		}else if(strcmp(cmd, "QUIT") == 0){
			PFTPD_WRITE(FTP_LOGOUT, "Sayonara");
		}else if(logged_in == 0){
			PFTPD_WRITE(FTP_NOT_LOGGED_IN, "Login first");
		}else if(strcmp(cmd, "SYST") == 0){
			PFTPD_WRITE(FTP_NAME, "UNIX");
		}else if(strcmp(cmd, "PASV") == 0){
			if(state->section->pasvaddr != NULL){
				char buf[1024];
				char pasv[512];
				in_addr_t addr;
				int minport;
				int maxport = 65536;
				int port = 64;
				unsigned char* ipv4;
				int i;
				strcpy(buf, state->section->pasvaddr);
				for(i = 0; buf[i] != 0; i++){
					if(buf[i] == ':'){
						buf[i] = 0;
						addr = inet_addr(buf);

						break;
					}
				}
				ipv4 = (unsigned char*)&addr;
				sprintf(pasv, "Entering passive mode (%d,%d,%d,%d,%d,%d)", ipv4[0], ipv4[1], ipv4[2], ipv4[3], (port >> 8) & 0xff, (port) & 0xff);
				PFTPD_WRITE(FTP_ENTER_PASSIVE, pasv);
			}else{
				PFTPD_WRITE(FTP_SYNTAX_ERROR_CMD, "Passive address not set");
			}
		}else if(strcmp(cmd, "EPSV") == 0){
			if(state->section->pasvaddr == NULL){
				char pasv[512];
				sprintf(pasv, "Entering extended passive mode (|||%d|)", 0);
				PFTPD_WRITE(FTP_ENTER_EXT_PASSIVE, pasv);
			}else{
				PFTPD_WRITE(FTP_SYNTAX_ERROR_CMD, "Passive address not set");
			}
		}else if(strcmp(cmd, "PWD") == 0){
			char path[PATH_MAX + 1];
			char* msg;
			getcwd(path, PATH_MAX);
			msg = malloc(1 + strlen(path) + 1 + strlen(" is the current directory") + 1);
			msg[0] = 0;
			strcat(msg, "\"");
			strcat(msg, path);
			strcat(msg, "\" is the current directory");
			PFTPD_WRITE(FTP_CREATED, msg);
		}else if(strcmp(cmd, "FEAT") == 0){
			PFTPD_WRITE(FTP_FILE_NOT_FOUND, "Permission denied");
		}else if(strcmp(cmd, "HELP") == 0){
			PFTPD_WRITE(FTP_HELP, "The following commands are cognized\nCDUP CWD DELE EPSV FEAT HELP LIST PASS PASV QUIT RETR USER\nHelp OK");
		}else{
			PFTPD_WRITE(FTP_SYNTAX_ERROR_CMD, "Unknown command");
			printf("Unknown command: `%s%s%s'\n", cmd, arg == NULL ? "" : " ", arg == NULL ? "" : arg);
		}
	}
}

void pftpd_process_state(pftpd_state_t* state, struct sockaddr_in claddr){
	int i;
	for(i = 0; i < sec_count; i++){
		if(strcmp(sec_entries[i]->name, "global") == 0){
			pftpd_apply_rule(state, claddr, sec_entries[i]);
			break;
		}
	}
	for(i = 0; i < sec_count; i++){
		if(strcmp(sec_entries[i]->name, "global") != 0){
			pftpd_apply_rule(state, claddr, sec_entries[i]);
		}
	}
}

int pftpd_server(void){
	int i;
#ifdef USE_POLL
	struct pollfd* pollfds = malloc(sizeof(*pollfds) * server_entries);
	for(i = 0; i < server_entries; i++){
		pollfds[i].fd = server_sockets[i];
		pollfds[i].events = POLLIN | POLLPRI;
	}
#endif
#ifdef USE_SELECT
	fd_set fdset;
	struct timeval tv;
#endif
	while(1){
		int ret;
#ifdef USE_POLL
		ret = poll(pollfds, server_entries, 1000);
#endif
#ifdef USE_SELECT
		FD_ZERO(&fdset);
		for(i = 0; i < server_entries; i++){
			FD_SET(server_sockets[i], &fdset);
		}
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(FD_SETSIZE, &fdset, NULL, NULL, &tv);
#endif
		if(ret == -1){
			return 1;
		}else if(ret == 0){
		}else if(ret > 0){
			for(i = 0; i < server_entries; i++){
				int conn = 0;
#ifdef USE_POLL
				if(pollfds[i].revents & POLLIN) conn = 1;
#endif
#ifdef USE_SELECT
				if(FD_ISSET(server_sockets[i], &fdset)) conn = 1;
#endif
				if(conn){
					struct sockaddr_in claddr;
					int clen = sizeof(claddr);
					int sock = accept(server_sockets[i], (struct sockaddr*)&claddr, &clen);
					pid_t pid = fork();
					if(pid == 0){
						pftpd_state_t* state = malloc(sizeof(*state));
						state->section = pftpd_create_section();
						state->section->allow_anon = 1;
						state->section->allow_local = 0;
						pftpd_process_state(state, claddr);
						pftpd_handle_socket(sock, state);
						free(state);
						_exit(0);
					}else{
						CLOSE_SOCKET(sock);
					}
				}
			}
		}
	}
	return 0;
}
