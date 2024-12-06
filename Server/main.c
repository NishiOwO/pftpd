/* $Id$ */

#include <stdio.h>
#include <string.h>

const char* conf = PREFIX "/etc/pftpd.conf";

int main(int argc, char** argv){
	int i;
	printf("pftpd/%s\n", VERSION);
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
	printf("Parsing %s\n", conf);
}
