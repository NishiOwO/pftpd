/* $Id$ */

#define USE_POLL

#include "pftpd.h"

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void pftpd_handle_socket(int sock, pftpd_state_t* state){
}

int pftpd_server(void){
#ifdef USE_POLL
	struct pollfd* pollfds = malloc(sizeof(*pollfds) * server_entries);
	int i;
	for(i = 0; i < server_entries; i++){
		pollfds[i].fd = server_sockets[i];
		pollfds[i].events = POLLIN | POLLPRI;
	}
#endif
	while(1){
		int ret;
#ifdef USE_POLL
		ret = poll(pollfds, server_entries, 1000);
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
				if(conn){
					struct sockaddr_in claddr;
					int clen = sizeof(claddr);
					int sock = accept(server_sockets[i], (struct sockaddr*)&claddr, &clen);
					pid_t pid = fork();
					if(pid == 0){
						pftpd_handle_socket(sock, NULL);
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
