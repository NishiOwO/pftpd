/* $Id$ */

#define INCLUDE_NET

#include "pftpd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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
		if(msg != NULL) free(msg);
		PFTPD_READ(msg);
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
