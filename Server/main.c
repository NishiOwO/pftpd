/* $Id$ */

#include "pftpd.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

const char* conf = PREFIX "/etc/pftpd.conf";

int yyparse(void);
extern FILE* yyin;

void rehash_user(int sig){
	pftpd_init_user();
}

int main(int argc, char** argv){
	int i;
	printf("pftpd/%s (using %s)\n", VERSION,
#ifdef USE_POLL
		"poll"
#endif
#ifdef USE_SELECT
		"select"
#endif
	);
	printf("Under public domain, original by Nishi, 2024.\n");
	for(i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0){
				return 0;
			}else if(strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-C") == 0){
				conf = argv[++i];
				if(conf == NULL){
					fprintf(stderr, "Argument is required for %s\n", argv[i]);
					return 1;
				}
			}else{
				fprintf(stderr, "Invalid flag: %s\n", argv[i]);
				return 1;
			}
		}
	}
	if(getuid() != 0){
		printf("Program is not running as root, attempting to setuid\n");
		if(setuid(0) != 0){
			fprintf(stderr, "Could not setuid, run me as root or set setuid flag + set owner to root\n");
			return 1;
		}
		if(seteuid(0) != 0){
			fprintf(stderr, "Could not seteuid, run me as root or set setuid flag + set owner to root\n");
			return 1;
		}
		printf("Switched to root user successfully\n");
	}
	printf("Parsing %s\n", conf);
	yyin = fopen(conf, "r");
	if(yyin == NULL){
		fprintf(stderr, "Could not open the config file\n");
		return 1;
	}
	if(yyparse() != 0) return 1;
	fclose(yyin);
	if(pftpd_init_user() != 0) return 1;
	signal(SIGHUP, rehash_user);
	signal(SIGCHLD, SIG_IGN);
	printf("Server starting\n");
	if(pftpd_server_init() != 0) return 1;
	printf("Server started\n");
	if(pftpd_server() != 0) return 1;
}
