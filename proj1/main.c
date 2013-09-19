#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define FILE_FLAG O_CREAT | O_RDWR  | O_TRUNC

int main(int argc, char *argv[])
{
	if(argc < 2 || strcmp(argv[1], "--help") == 0){
		printf("usage: %s photos/*.jpg\n", argv[0]);
		exit(0);
	}
	pid_t childPid;
	int status;
	char *convert = "/usr/bin/convert";
	char *display = "/usr/bin/display";
	char thumb[PATH_MAX];
	char medium[PATH_MAX];
	char *argVec[8];
	FILE *file;
	if((file = fopen("index.html", "w+")) == NULL){
		fprintf(stderr, "Error: open\n");
		exit(1);
	}
	fprintf(file, "<html><head>\n<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"><title>a sample index.html</title>\n");
	fprintf(file, "</head><body><h1>index.html</h1>\n");
	fprintf(file, "Please click on a thumbnail to view a medium-size image\n");
	int i;
	for(i = 1; i < argc; i++){
		char *filename = strrchr(argv[i], '/');
		if(filename == NULL)
			filename = argv[i];
		else
			filename += 1;
		strcpy(thumb, filename);
		strcpy(medium, filename);
		childPid = fork();
		switch(childPid){
			case -1:
				fprintf(stderr, "Error: fork\n");
				exit(1);
			case 0:
				argVec[0] = strrchr(convert, '/') + 1;
				argVec[1] = "-geometry";
				argVec[2] = "10%";
				argVec[3] = argv[i];
				argVec[4] = thumb;
				argVec[5] = NULL;
				execv(convert, argVec);
			default:
				if(waitpid(childPid, &status, 0) == -1){
					fprintf(stderr, "Error: waitpid\n");
					exit(1);
				}
		}
		char buffer[10];
		int degree = 0;
		int cont = 1;
		while(cont){
			childPid = fork();
			switch(childPid){
				case -1:
					fprintf(stderr, "Error: fork\n");
					exit(1);
				case 0:
					argVec[0] = strrchr(display, '/') + 1;
					argVec[1] = thumb;
					argVec[2] = NULL;
					execv(display, argVec);
				default:
					while(1){
						puts("Rotate?(a to rotate left, d to rotate right enter to quit)");
						if(gets(buffer) != NULL){
							if(strcmp(buffer, "") == 0){
								cont = 0;
								break;
							}
							else if(strcmp(buffer, "a") == 0){
								degree -= 90;
								argVec[2] = "-90";
							}
							else if(strcmp(buffer, "d") == 0){
								degree += 90;
								argVec[2] = "90";
							}
							else
								continue;
							if(kill(childPid, SIGTERM) == -1){
								fprintf(stderr, "Error: kill\n");
								exit(1);
							}
							if(waitpid(childPid, &status, 0) == -1){
								fprintf(stderr, "Error: waitpid\n");
								exit(1);
							}
							childPid = fork();
							switch(childPid){
								case -1:
									fprintf(stderr, "Error: fork\n");
									exit(1);
								case 0:
									argVec[0] = strrchr(convert, '/') + 1;
									argVec[1] = "-rotate";
									argVec[3] = argVec[4] = thumb;
									argVec[5] = NULL;
									execv(convert, argVec);
								default:
									if(waitpid(childPid, &status, 0) == -1){
										fprintf(stderr, "Error: waitpid\n");
										exit(1);
									}
							}
							break;
						}
						else
							cont = 0;
							break;
					}
			}
		}
		char caption[30];
		while(1){
			puts("Enter a caption(less than 30 characters)");
			if(gets(caption) == NULL)
				continue;
			break;
		}
		fprintf(file, "<h2>%s</h2>\n", caption);
		fprintf(file, "<a href=\"%s\"><img src=\"%s\" border=\"1\"></a>\n", medium, thumb);
		if(kill(childPid, SIGTERM) == -1){
			fprintf(stderr, "Error: kill\n");
			exit(1);
		}
		if(waitpid(childPid, &status, 0) == -1){
			fprintf(stderr, "Error: waitpid\n");
			exit(1);
		}
		degree %= 360;
		char deg[4];
		sprintf(deg, "%d", degree);
		childPid = fork();
		switch(childPid){
			case -1:
				fprintf(stderr, "Error: fork\n");
				exit(1);
			case 0:
				argVec[0] = strrchr(convert, '/') + 1;
				argVec[1] = "-geometry";
				argVec[2] = "25%";
				argVec[3] = "-rotate";
				argVec[4] = deg; 
				argVec[5] = argv[i];
				argVec[6] = medium;
				argVec[7] = NULL;
				execv(convert, argVec);
			default:
				break;
		}
	}
	fprintf(file, "</body></html>");
	if(fclose(file) != 0)
		fprintf(stderr, "Error: fclose\n");
}
