
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>


int main(int agrc, char **argv)
{
	setvbuf( stdout, NULL, _IONBF, 0 );

	long size = pathconf(".", _PC_PATH_MAX);
	char* current_dir = getcwd(NULL,(size_t) size);

	char** paths = calloc(64,sizeof(char*));
	// # of paths
	int path_count = 0;
	//pid of current process 
	pid_t pid;
	int status = 0;
	// pid_t* backgorund_pids = calloc();
	// background proccesses count
	int background_count = 0;

	char* MYPATH = calloc(1024, sizeof(char));
	// check if mypath is invoked 
	if(getenv("MYPATH") != NULL){
		strcpy(MYPATH, getenv("MYPATH"));
	}else{
		strcpy(MYPATH,"/bin:.");
	}
	char* path = strtok(MYPATH, ":");

	while(path != NULL){
		paths[path_count] = calloc(100,sizeof(char));
		strcpy(paths[path_count], path);
		paths[path_count][strlen(paths[path_count])] = '\0';
		path_count += 1;
		path = strtok(NULL, ":");
	}

	while ( 1 )
	{
		// check background proccesses
		int i;
		for(i = 0; i < background_count; ++i){
			int status;
			int curr_pid = waitpid(-1,&status,WNOHANG);
			if(curr_pid==0){
				continue;
			}
			if(WIFSIGNALED(status)){
				printf("[process %d terminated abnormally]\n", curr_pid);
				background_count -= 1;
			}
			else {
				printf("[process %d terminated with exit status %d]\n", curr_pid, WEXITSTATUS(status));
				background_count -= 1;
			}
		}

		printf("%s$ ",current_dir);

		char* input = calloc(1024,sizeof(char));
		fgets(input, 1024, stdin);
		input[strlen(input)-1] = '\0';

		char** buffer = calloc(16,sizeof(char*));

		if(!strcmp(input, "exit")){
			int k;
			for(k = 0; k < 16; ++k){
				if(buffer[k] != NULL){
					free(buffer[k]);
				}
			}
			free(buffer);
			free(input);
			free(current_dir);
			int i;
			for (i = 0; i < 64; i++){
				if (paths[i] != NULL){
					free(paths[i]);
				}
			}
			
			free(paths);
			free(MYPATH);
			printf("bye");
			exit(EXIT_SUCCESS);
			break;
		}

		char* token = strtok(input, " ");
		int input_size = 0;
		while (token != NULL){
			char* currToken = calloc(65, sizeof(char));
			strcpy(currToken, token);
			buffer[input_size] = currToken;
			input_size += 1;
			token = strtok(NULL, " ");
		}
		
		// check for pipe
		int pipe_index = -1;
		int c;
		for(c = 0; c < input_size; ++c){
			if(!strcmp(buffer[c],"|")){
				pipe_index = c;
				break;
			}
		}

		// three scenarios: 
		// cd, cd /, cd directory
		if(!strcmp(buffer[0],"cd")){
			int cd_path;
			if(input_size == 1 || (input_size == 2 && !strcmp(buffer[1],"~"))){
				cd_path = chdir(getenv("HOME"));
				if(cd_path == 0){
					free(current_dir);
					current_dir = calloc(strlen(getenv("HOME")+1),sizeof(char));
					strcpy(current_dir,getenv("HOME"));
				}
			}else{
				free(current_dir);
				current_dir = calloc(pathconf(".", _PC_PATH_MAX) + 1, sizeof(char));	
				getcwd(current_dir, pathconf(".", _PC_PATH_MAX));
				
				cd_path = chdir(buffer[1]);
				if(cd_path == 0){
					free(current_dir);
					current_dir = calloc(pathconf(".",_PC_PATH_MAX)+1,sizeof(char));
					getcwd(current_dir,pathconf(".", _PC_PATH_MAX));
				}else{
					fprintf(stderr, "chdir() failed: Not a directory\n");
				}
			}
			int k;
			for(k = 0; k < input_size; ++k){
				if(buffer[k] != NULL){
					free(buffer[k]);
				}
			}
			free(buffer);
			free(input);
			continue;
		}

		char* executable_one = buffer[0];
		
		char* temp_one = calloc(200,sizeof(char));
		char* temp_two = calloc(200,sizeof(char));
		int found_executable = 0;
		int found_executable_two = 0;
		int p[2]; 

		if(pipe_index > 0){
			char** argument_one = calloc(200,sizeof(char*));
			char** argument_two = calloc(200,sizeof(char*));
			int argOneCount = 0, argTwoCount = 0; 
			// argument_one[pipe_index] = NULL;
			int a, b = 0;
			for(a = 0; a < pipe_index; ++a){
				argument_one[a] = buffer[a];
				argOneCount+=1;
			}
			for(a = pipe_index+1; a < input_size; ++a){
				argument_two[b] = buffer[a];
				argTwoCount+=1;
				b+=1;
			}

			char* executable_two = buffer[pipe_index+1];
			// // find first command executale
			// int i;
			for(i = 0; i < path_count; ++i){
				strcpy(temp_one,paths[i]);
				strcat(temp_one,"/");
				strcat(temp_one,executable_one);
				struct stat buf1;
				int rc1 = lstat(temp_one, &buf1);
				if(rc1 == 0){
					if(buf1.st_mode & S_IXUSR){
						// printf("%s\n",temp_one);
						found_executable = 1;
					}
					break;
				}
			}

			for(i = 0; i < path_count; ++i){
				strcpy(temp_two,paths[i]);

				strcat(temp_two,"/");
				strcat(temp_two,executable_two);
				struct stat buf2;
				int rc2 = lstat(temp_two, &buf2);
				if(rc2 == 0){
					if(buf2.st_mode & S_IXUSR){
						// printf("%s\n",temp_two);
						found_executable_two = 1;
					}
					break;
				}
			}	
			// free(executable_two);

			pid = fork();
			pid_t second_pid;

			if(pid == 0){
				if(pipe(p) == -1) continue;
				second_pid = fork();
				if(second_pid == 0){
					// printf("HERE");
					dup2(p[1],1);
					close(p[0]);
					if(execvp(temp_one,argument_one) < 0){
						exit(EXIT_FAILURE);
					}
				}else if(second_pid > 0){
					int status;
					waitpid(second_pid,&status,0);
					close(p[1]);
					dup2(p[0],0);
					if(execvp(temp_two,argument_two) < 0){
						exit(EXIT_FAILURE);
					}
				}else{
					continue;
				}
			}else if(pid > 0){
				waitpid(pid, &status, 0);
			}else{
				continue;
			}
			int k;
			for(k = 0; k < argOneCount+1; ++k){
				free(argument_one[k]);
			}
			free(argument_one);
			for(k = 0; k < argTwoCount+1; ++k){
				free(argument_two[k]);
			}
			free(argument_two);
			// free(temp_one);
			// free(temp_two);
		}else{
			int i;
			for (i = 0; i < path_count; i++){	
				if (paths[i] == NULL){
					break;
				}
				char* tempPath = calloc(200, sizeof(char));
				strcpy(tempPath, paths[i]);
				tempPath[strlen(tempPath)] =  '/';

				int j; 
				for (j = 0; j < strlen(buffer[0]); j++){
					tempPath[strlen(tempPath)] = buffer[0][j];
				}

				struct stat buf;
				int rc = lstat(tempPath, &buf);
				free(tempPath);
				if (rc == 0){
					if (buf.st_mode & S_IXUSR){
						found_executable = 1;
					} 
					break;
				}
			}			
		}
		free(temp_one);
		free(temp_two);

		if((found_executable == 0 || found_executable_two == 0) && pipe_index > 0){
			fprintf(stderr, "ERROR: one or both of the commands not found");
		}else if(found_executable == 0 && pipe_index == -1){
			fprintf(stderr, "ERROR: command \"%s\" not found\n", buffer[0]);
		}

		if(found_executable == 1 && pipe_index == -1){
			int isBackground = 0;

			if(strcmp(buffer[input_size-1],"&") == 0){
				isBackground = 1;
				free(buffer[input_size-1]);
				buffer[input_size-1] = '\0';
			}

			// similar to locating execution
			char* exec = calloc(256, sizeof(char));
			strcpy(exec,paths[path_count-2]);

			strcat(exec,"/");
			strcat(exec,executable_one);
			strcat(exec,"\0");
 
			pid = fork();
			if(pid == -1){
				perror("Failed to fork");
				return EXIT_FAILURE;
			}

			if(pid == 0){
				execv(exec,buffer);
				perror("EXEC FAILED\n");
				return EXIT_FAILURE;
			}else if(pid > 0){
				if(isBackground == 0){
					waitpid(pid,NULL,0);
				}else{
					background_count+=1;
					printf("[running background process \"%s\"]\n",buffer[0]);
					int child_pid = waitpid(-1,NULL,WNOHANG);
					if (child_pid == -1){
						perror("waitpid() error\n" );
						return EXIT_FAILURE;
					}
				}
			}
			free(exec);
		}

		// int x;
		// for(x = 0; x < input_size; ++x){
		// 	if(buffer[x] != NULL){
		// 		// printf("%s ",buffer[x]);
		// 		free(buffer[x]);
		// 	}
		// }
		free(buffer);
		free(input);
	}

	int i;
	for (i = 0; i < 64; i++){
		if (paths[i] != NULL){
			free(paths[i]);
		}
	}
	free(paths);
	free(current_dir);
	free(MYPATH);

	return EXIT_SUCCESS;
}
