#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


// int max_squares;
int max_squares;
int ***dead_end_boards;
int dead_end_board_count ;
int dead_end_capacity;
pthread_t main_tid;

// third argument
int x;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    int m, n;
    int **board;
    int move_number;
    int curr_pos[2];
    int square_seen;
} ThreadArgs;

void *findNextMove(void *arg);


void freeBoard(int **board, int m){
    int i;
    for(i = 0; i < m; ++i){
        free(board[i]);
    }
    free(board);
}

void addDeadEndBoard(int **board, int m, int n, int seen){
    if(seen < x){
        freeBoard(board,m);
        return;
    }

    pthread_mutex_lock(&mutex);
    if(dead_end_board_count+1 >= dead_end_capacity){
        dead_end_boards = realloc(dead_end_boards, (dead_end_capacity + 10) * sizeof(int**));
        dead_end_capacity += 10;
    }

    dead_end_boards[dead_end_board_count++] = board;
    pthread_mutex_unlock(&mutex);
}

void printDeadEndBoards(int m, int n){
	printf("THREAD %ld: Dead end boards:\n", pthread_self());
    for(int i = 0; i < dead_end_board_count; ++i){
        printf("THREAD %ld: > ", pthread_self());
        for(int j = 0; j < m; ++j){
            if(j !=0 ){
                printf("THREAD %ld:   ",pthread_self());
            }
            for(int k = 0; k < n; ++k){
                if(dead_end_boards[i][j][k] == 0){
                    printf(".");
                }else{
                    printf("S");
                }
            }
            printf("\n");
        }
    }
}

void searchPossibleMoves(int **board, int m, int n, int *curr_pos, int* num_moves, int** next_moves){
    int move_x[] = {-1, -2, -2, -1, 1, 2, 2, 1}; // clockwise x offsets
    int move_y[] = {-2, -1, 1, 2, 2, 1, -1, -2}; 
    *num_moves = 0;
    for(int i = 0; i < 8; ++i){
        int next_x = curr_pos[0] + move_x[i];
        int next_y = curr_pos[1] + move_y[i];
        if (next_x >= 0 && next_x < m && next_y >= 0 && next_y < n && board[next_x][next_y] == 0) {
            next_moves[*num_moves][0] = next_x;
            next_moves[*num_moves][1] = next_y;
            (*num_moves)++;
        }
    }
}



void* findNextMove(void *arg){
    ThreadArgs *args = (ThreadArgs *)arg;

    if(args->move_number == 1){
		printf("THREAD %ld: Solving Sonny's knight's tour problem for a %dx%d board\n", pthread_self(), args->m, args->n);
    }

    int num_moves = 0;
    int highest;

    int **next_moves = calloc(8,sizeof(int*));
    for(int i = 0; i < 8; ++i){
        next_moves[i] = calloc(2,sizeof(int));
    }

    int **board = args->board;
    int m = args->m;
    int n = args->n;
    int move_number = args->move_number;
    int curr_pos[2];
    int square_seen = args->square_seen;
    int* ans = calloc(1,sizeof(int));
    *ans = 0;
    highest = move_number;
    curr_pos[0] = args->curr_pos[0];
    curr_pos[1] = args->curr_pos[1];
    
    searchPossibleMoves(board, m, n, curr_pos, &num_moves, next_moves);

    // dead or compeete
    if(num_moves == 0) { 
        // complete
        free(args);

        if(square_seen == m * n){
            printf("THREAD %ld: Sonny found a full knight's tour!\n", pthread_self());
            freeBoard(board, m);
        }else{ 
            // deadend board
            printf("THREAD %ld: Dead end after move #%d\n",pthread_self(),move_number);
            addDeadEndBoard(board,m,n,square_seen);
        }

        if(move_number > max_squares){
            max_squares = move_number;
        }

        for(int i = 0; i < 8; ++i){
            free(next_moves[i]);
        }        

        free(next_moves);

        *ans = move_number;
        pthread_exit(ans);

    }
    else if(num_moves == 1){

        int next_pos[2];
        for(int i = 0; i<8; ++i){
            if(!(next_moves[i][0] == 0 && next_moves[i][1] == 0)){
                next_pos[0] = next_moves[i][0];
                next_pos[1] = next_moves[i][1];
                break;
            }
        }

        board[next_pos[0]][next_pos[1]] = move_number+1;

        ThreadArgs* next_arg = malloc(sizeof(ThreadArgs));
        next_arg->board = board;
        next_arg->m = m;
        next_arg->n = n;     
        next_arg->curr_pos[0] = next_pos[0];
        next_arg->curr_pos[1] = next_pos[1];   
        next_arg->move_number = move_number+1;
        next_arg->square_seen = square_seen+1;

        for(int i = 0; i < 8; ++i){
            free(next_moves[i]);
        }
        free(ans);
        free(next_moves);
        free(args);

        findNextMove(next_arg);

    }

    else if(num_moves > 1){
        free(args);
        printf("THREAD %ld: %d moves possible after move #%d; creating threads...\n", pthread_self(), num_moves, move_number);
        pthread_t tid[8];
        
        // each direction
        for(int i = 0; i < 8; ++i){
            if(!(next_moves[i][0] == 0 && next_moves[i][1] == 0)){
                int** next_board = calloc(m,sizeof(int*));
                int j, k;
                for(j = 0; j < m; ++j){
                    next_board[j] = calloc(n,sizeof(int));
                    for(k = 0; k < n; ++k){
                        next_board[j][k] = board[j][k];
                    }
                }
                next_board[next_moves[i][0]][next_moves[i][1]] = move_number+1;
                
                int curr_pos[2];
                curr_pos[0] = next_moves[i][0];
                curr_pos[1] = next_moves[i][1];

                ThreadArgs* next_arg = malloc(sizeof(ThreadArgs));
                next_arg->board = next_board;
                next_arg->m = m;
                next_arg->n = n;     
                next_arg->curr_pos[0] = curr_pos[0];
                next_arg->curr_pos[1] = curr_pos[1];   
                next_arg->move_number = move_number+1;
                next_arg->square_seen = square_seen+1;

                // create thread
                pthread_create(&tid[i],NULL,&findNextMove,next_arg);
                #ifdef NO_PARALLEL 
				void* returnValue = malloc(0);
				free(returnValue);

				pthread_join(tid[i], (void**) &returnValue);

				if (*(int*) returnValue > highest){
					highest = *(int*) returnValue;
				}	

				printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[i], *(int*) returnValue);
				free(returnValue);
				#endif
            }
        }

		for (int i = 0; i < 8; i++){
			free(next_moves[i]);
		}
		free(next_moves);
    }

    if (pthread_self() != main_tid){
	    *ans = move_number;
		if (highest > *ans){
			*ans = highest;
        }
        for (int i = 0; i < m; i++){
			free(board[i]);
		}
		free(board);
        pthread_exit(ans);
    }

    free(ans);
    return ans;
}

int main(int argc, char **argv){

    setvbuf( stdout, NULL, _IONBF, 0 );

    if(argc < 3){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
		return EXIT_FAILURE;	
    }

    int m = atoi(*(argv+1));
    int n = atoi(*(argv+2));

    if(n <=2 || m <= 2){
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
		return EXIT_FAILURE;	
    }

    if(argc == 4){
        x = atoi(*(argv+3));
        if(x > n*m){
            fprintf(stderr,"ERROR: Invalid argument(s)\n");
            fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
            return EXIT_FAILURE;
        }
    }else{
        x = 0;
    }

    dead_end_capacity = 10;
    // Initialize board3
    dead_end_boards = calloc(dead_end_capacity,sizeof(int**));
    max_squares = 0;
    dead_end_board_count = 0;

    int** board = calloc(m,sizeof(int*));
    for(int i = 0; i < m; ++i){
        board[i] = calloc(n,sizeof(int));
        for(int j = 0; j < n; ++j){
            board[i][j] = 0;
        }
    }
    // initial position
    board[0][0] = 1;
       
    ThreadArgs* args = malloc(sizeof(ThreadArgs));

    // initialize thread argument
    args->m = m;
    args->n = n;
    args->board=board;
    args->move_number = 1;
    args->square_seen = 1;
    // Starting position (0,0)
    args->curr_pos[0] = 0;
    args->curr_pos[1] = 0;

    main_tid = pthread_self();

    findNextMove(args);

    printf("THREAD %ld: Best solution(s) found visit %d squares (out of %d)\n", pthread_self(), max_squares, n * m);

    printDeadEndBoards(m,n);

    freeBoard(board,m);

    for(int i = 0; i < dead_end_board_count; ++i){
        for(int j = 0; j < m; ++j){
            free(dead_end_boards[i][j]);
        }
        free(dead_end_boards[i]);
    }
    free(dead_end_boards);

    pthread_mutex_destroy(&mutex);


    return EXIT_SUCCESS;
}