#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <signal.h>



#define MAXBUFFER 16

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;

pthread_mutex_t total_guesses_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_wins_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_losses_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t words_mutex = PTHREAD_MUTEX_INITIALIZER;


short guesses = 6;

char* filename;
FILE* file;

typedef struct{
	int *client_socket;
	int remaining_guesses;
	char* current_guesses;
	char* client_hidden_word;
} ClientGameState;

bool isValidWorld(const char* guess){

	FILE* file = fopen(filename,"r");

	char* buffer = calloc(5,sizeof(char));

	while(fscanf(file,"%s",buffer) == 1){
		if(strcmp(buffer, guess) == 0){
			fclose(file);
			free(buffer);
			return true;
		}

	}

	fclose(file);
	free(buffer);
	return false;
}

void exit_handler(int signum){
	printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
	printf("MAIN: Wordle server shutting down...\n");
	printf("\n");
	printf("MAIN: guesses: %d\n", total_guesses);
	printf("MAIN: wins: %d\n", total_wins);
	printf("MAIN: losses: %d\n", total_losses);
	printf("\n");

	// char* upper = calloc(5,sizeof(char));
	// int i = 0;
	// while(*(hidden_word+i) != '\0'){
	// 	*(upper+i) = toupper(*(hidden_word+i));
	// 	i++;
	// }
	// printf("MAIN: word(s) played: %s", upper);
	// free(upper);


	exit(EXIT_SUCCESS);
}

void *handle_words(void *newsd){

	ClientGameState *state = (ClientGameState* )newsd;

	int remaining_guesses = state->remaining_guesses;
	char* hidden_word = state->client_hidden_word;

	pthread_mutex_lock(&words_mutex);
	char* upper = calloc(5,sizeof(char));
	int i = 0;
	while(*(hidden_word+i) != '\0'){
		*(upper+i) = toupper(*(hidden_word+i));
		i++;
	}
	i=0;
	while(*(words+i)!=NULL){i++;}
	*(words+i) = calloc(5,sizeof(char));
	strcpy(*(words+i),upper);
	free(upper);
	pthread_mutex_unlock(&words_mutex);

	int client_socket = *((int *) state->client_socket);
	free(newsd);
	

	char * buffer = calloc(MAXBUFFER+1,sizeof(char));
	int n;
	printf("THREAD %ld: waiting for guess\n", pthread_self());


	do{
		n = recv(client_socket,buffer,MAXBUFFER,0);

		if(n == -1){
			perror( "recv() failed" );
            exit(EXIT_FAILURE);
		}else if(n == 0){

			printf( "THREAD %ld: client gave up; closing TCP connection...\n" ,pthread_self());
			char* upper = calloc(5,sizeof(char));
			int i = 0;
			while(*(hidden_word+i) != '\0'){
				*(upper+i) = toupper(*(hidden_word+i));
				i++;
			}
			printf("THREAD %ld: game over; word was %s!\n",pthread_self(),upper);
			free(upper);
			pthread_mutex_lock(&total_losses_mutex);
			total_losses += 1;
			pthread_mutex_unlock(&total_losses_mutex);

		}else{
			// printf("incoming word: %s", buffer);
			// *(buffer+n-1) = '\0';
			printf("THREAD %ld: rcvd guess: %s\n",pthread_self(), buffer);

			char* server_reply = calloc(8,sizeof(char));
			char* valid_guess = calloc(2,sizeof(char));
			// short* guesses_remaining = calloc(2,sizeof(short));
			uint16_t* network_short = calloc(1,sizeof(uint16_t));
			
			if(strcmp(buffer,hidden_word) == 0){
				remaining_guesses -= 1;

				pthread_mutex_lock(&total_guesses_mutex);
				total_guesses+=1;
				pthread_mutex_unlock(&total_guesses_mutex);

				pthread_mutex_lock(&total_wins_mutex);
				total_wins+=1;
				pthread_mutex_unlock(&total_wins_mutex);

				// add valid word to total
				

				*network_short = htons(remaining_guesses);
				memcpy((server_reply+1),network_short,sizeof(uint16_t));
				free(network_short);

				strcpy(valid_guess,"Y");
    			*server_reply = *valid_guess;
				free(valid_guess);

				char* upper = calloc(5,sizeof(char));
				i = 0;
				while(*(hidden_word+i) != '\0'){
					*(upper+i) = toupper(*(hidden_word+i));
					i++;
				}
				strcpy(server_reply+3,upper);

				printf("Thread %ld: sending reply: %s (%d guess%s left)\n", pthread_self(), server_reply+3, remaining_guesses , remaining_guesses  == 1 ? "" : "es");

				n = send(client_socket,server_reply,8,0);

				if ( n != 8 )
				{
					perror( "send() failed" );
					exit(EXIT_FAILURE);
				}

				printf("Thread %ld: game over; word was %s!\n", pthread_self(),server_reply+3);

				printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
				printf("MAIN: Wordle server shutting down...\n");
				printf("\n");
				printf("MAIN: guesses: %d\n", total_guesses);
				printf("MAIN: wins: %d\n", total_wins);
				printf("MAIN: losses: %d\n", total_losses);
				printf("\n");
				printf("MAIN: word(s) played: %s",upper);
				free(upper);

				// pthread_exit(NULL);
				// signal(SIGUSR1,exit_handler);
				// close(client_socket);
				return EXIT_SUCCESS;

			}else{

				// none words match
				if(!isValidWorld(buffer)){

					strcpy(valid_guess,"N");
					*server_reply = *valid_guess;
					free(valid_guess);

					*network_short = htons(remaining_guesses);
					memcpy((server_reply+1),network_short,sizeof(uint16_t));
					free(network_short);

					char* message = calloc(6,sizeof(char));
					strcpy(message,"?????");
					strncpy(server_reply+3, message,5);
					free(message);

					printf("Thread %ld: invalid guess; sending reply: %s (%d guesses left)\n", pthread_self(), server_reply+3, remaining_guesses);
					printf("THREAD %ld: waiting for guess\n", pthread_self());

					n = send(client_socket,server_reply,8,0);
					free(server_reply);

					if ( n != 8 )
					{
						perror( "send() failed" );
						exit(EXIT_FAILURE);
					}
				}
				// valid guess
				else{
					
					remaining_guesses -= 1;

					pthread_mutex_lock(&total_guesses_mutex);
					total_guesses+=1;
					pthread_mutex_unlock(&total_guesses_mutex);

					strcpy(valid_guess,"Y");
					*server_reply = *valid_guess;
					free(valid_guess);


					*network_short = htons(remaining_guesses);
					memcpy((server_reply+1),network_short,sizeof(uint16_t));
					free(network_short);

					char* message = calloc(5,sizeof(char));
					int i = 0;
					while(*(buffer+i)!='\0'){
						
						if(*(buffer+i) == *(hidden_word+i)){
							*(message+i) = toupper(*(buffer+i));
						}else{
							if(strchr(hidden_word,*(buffer+i)) != NULL){
								*(message+i) = tolower(*(buffer+i));
							}else{
								*(message+i) = '-';
							}
						}
						i++;
					}

					*(message+5) = '\0';
					strcpy(server_reply+3,message);
					free(message);


					printf("Thread %ld: sending reply: %s (%d guess%s left)\n", pthread_self(), server_reply+3, remaining_guesses , remaining_guesses  == 1 ? "" : "es");

					n = send(client_socket,server_reply,8,0);
					free(server_reply);

					if(remaining_guesses == 0){

						pthread_mutex_lock(&total_losses_mutex);
						total_losses += 1;
						pthread_mutex_unlock(&total_losses_mutex);

						i = 0;
						char* upper = calloc(5,sizeof(char));
						while(*(hidden_word+i) != '\0'){
							*(upper+i) = toupper(*(hidden_word+i));
							i++;
						}
						printf("THREAD %ld: game over; word was %s!\n",pthread_self(),upper);
						free(upper);

						printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
						printf("MAIN: Wordle server shutting down...\n");
						printf("\n");
						printf("MAIN: guesses: %d\n", total_guesses);
						printf("MAIN: wins: %d\n", total_wins);
						printf("MAIN: losses: %d\n", total_losses);
						printf("\n");

						int i = 0;
						printf("MAIN: word(s) played:");
						while(*(words+i) != NULL){
							printf(" %s",*(words+i));
							++i;
						}

						return EXIT_SUCCESS;
					}else{
						printf("THREAD %ld: waiting for guess\n", pthread_self());

					}

					if ( n != 8 )
					{
						perror( "send() failed" );
						exit(EXIT_FAILURE);
					}
				}				
			}
		}
	}while(n>0);
	close(client_socket);

	return 0;
}

int wordle_server( int argc, char ** argv ){
	setvbuf( stdout, NULL, _IONBF, 0 );
	
	if(argc != 5){
	fprintf(stderr, "Usage: %s <listener-port> <seed> <word-filename> <num-words>\n", *argv);
	exit(EXIT_FAILURE);
	}

	int port = atoi(*(argv+1));
	int seed = atoi(*(argv+2));
	file = fopen(*(argv+3),"r");
	int num_words = atoi(*(argv+4));

	filename = (*(argv+3));

	if(file == NULL){
		fprintf(stderr, "ERROR: Unable to open the file\n");
		return EXIT_FAILURE;
	}

	srand(seed);

	// create socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket == -1){
		return EXIT_FAILURE;
	}

  	struct sockaddr_in server;
	socklen_t length = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);	
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind() failed");
		return EXIT_FAILURE;
	}

	if (getsockname(server_socket, (struct sockaddr *) &server, &length) < 0)
	{
		perror("getsockname() failed: ");
		return EXIT_FAILURE;
	}

	if (listen(server_socket, 5) < 0)
	{
		perror("listen() failed: ");
		return EXIT_FAILURE;
	}

	// For each connection request received by your server,
	// create a child thread via pthread_create() to handle that specific connection.
	printf("MAIN: opened %s (%d words)\n",*(argv+3),num_words);
	printf("MAIN: Wordle server listening on port {%d}\n",port);

	while(1){
		struct sockaddr_in client;
		socklen_t client_address_len = sizeof(client);

		int *newsd = calloc(16,sizeof(int));
		*newsd = accept(server_socket, (struct sockaddr *)&client, &client_address_len);
		printf("%ls", newsd);
		if(*newsd < 0){
			perror("accept() failed");
			return EXIT_FAILURE;
		}

		printf("MAIN: rcvd incoming connection request\n");

		pthread_t p;
		ClientGameState *state = calloc(1,sizeof(ClientGameState));
		state->client_hidden_word = calloc(5,sizeof(char));
		state->client_socket = newsd;
		state->remaining_guesses = 6;

		int w = rand() % num_words;
		int curr_index = 0;

		while (fgets(state->client_hidden_word,sizeof(state->client_hidden_word), file) != NULL) {
			if (curr_index == w) {
				break;
			}
			curr_index++;
		}
		*(state->client_hidden_word+5) = '\0';

		fseek(file,0,SEEK_SET);
		
		pthread_create(&p,NULL,handle_words,state);
		pthread_detach(p);

	}

	return 0;
}



