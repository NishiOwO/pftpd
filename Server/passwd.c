/* $Id$ */

#include "pftpd.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#ifdef __linux__
#include <shadow.h>
#include <crypt.h>
#endif

char* pwdbuf = NULL;

int pftpd_init_user(void){
	struct stat s;
	FILE* fpwd = fopen("/etc/passwd", "r");
	if(fpwd == NULL){
		fprintf(stderr, "Could not open /etc/passwd\n");
		return 1;
	}
	stat("/etc/passwd", &s);
	if(pwdbuf != NULL) free(pwdbuf);
	pwdbuf = malloc(s.st_size + 1);
	fread(pwdbuf, s.st_size, 1, fpwd);
	pwdbuf[s.st_size] = 0;
	fclose(fpwd);
	return 0;
}

int pftpd_validate_password(const char* user, const char* password){
#ifdef __linux__
	struct spwd* s = getspnam(user);
	if(s == NULL) return -1;
	if(strcmp(s->sp_pwdp, crypt(password, s->sp_pwdp)) == 0) return 0;
	return -1;
#else
	struct passwd* p = getpwnam(user);
	if(p == NULL) return -1;
	if(strcmp(p->pw_passwd, crypt(password, p->pw_passwd)) == 0) return 0;
	return -1;
#endif
}

char* pftpd_find_user(int uid){
	int incr = 0;
	int i;
	for(i = 0;; i++){
		if(pwdbuf[i] == '\n' || pwdbuf[i] == 0){
			char* line = malloc(i - incr + 1);
			memcpy(line, pwdbuf + incr, i - incr);
			line[i - incr] = 0;
			if(strlen(line) > 0){
				int j;
				int count = 0;
				int prv = 0;
				for(j = 0;; j++){
					if(line[j] == ':'){
						line[j] = 0;
						if(count == 3){
							if(atoi(line + prv) == uid) return line;
						}
						prv = j + 1;
						count++;
						if(count == 4) break;
					}else if(line[j] == 0) break;
				}
			}
			free(line);
			incr = i + 1;
			if(pwdbuf[i] == 0) break;
		}
	}
	return NULL;
}
