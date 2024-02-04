#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


int hashFunction(char *word, int size){
    int hash = 0;
    while (*word) {
        hash += (int)(*word++);
    }
    return hash % size;
}

int isInteger(const char *str) {
    while (*str) {
        if (!isdigit(*str) && *str != '-' && *str != '+') {
            return 0; // Not an integer
        }
        str++;
    }
    return 1; // Integer
}


int main(int argc, char **argv){

    if(argc < 3){
        fprintf(stderr,"ERROR: Not enough arguments");
        return 1;
    }

    FILE *file;
    file = fopen(*(argv+2),"r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Unable to open the file\n");
        return 1;
    }

    if(isInteger(*(argv+1)) != 1){
        fprintf( stderr, "ERROR: Invalid Cache Size.\n" );
        return 1;
    }

    int cache_size = atoi(*(argv+1));
    char** cache = calloc(cache_size,sizeof(char*));
    char* buffer = calloc(128,sizeof(char));


    if ( cache == NULL ) {
        fprintf( stderr, "ERROR: calloc() failed.\n" );
        return 1;
    }

    int i = 0;
    // read file
    while(1){
        char n = fgetc(file);
        if( feof(file) ) {
            break ;
        }
        if(isalnum(n)){
            *(buffer+i) = n;
            i++;
        }else{
            *(buffer+i) = '\0';
            if(strlen(buffer)>=3){
                int hash = hashFunction(buffer,cache_size);
                if(*(cache+hash)==0){
                    printf("Word \"%s\" ==> %d (calloc)\n",buffer,hash);
                    char* temp = calloc(strlen(buffer)+1,sizeof(char));
                    strcpy(temp,buffer);
                    *(cache+hash) = temp;
                }else{
                    printf("Word \"%s\" ==> %d (realloc)\n",buffer,hash);
                    char* temp = realloc(*(cache + hash), strlen(buffer) + 1);
                    strcpy(temp, buffer);
                    *(cache + hash) = temp;
                }
            }
            memset(buffer, 0, 128);
		    i = 0;
        }
    }
    fclose(file);
    for (int j = 0; j < cache_size; ++j) {
        if (*(cache + j) != 0) {
            printf("Cache index %d ==> \"%s\"\n", j, *(cache + j));
        }
    }
    // Free allocated memory
    for (int j = 0; j < cache_size; ++j) {
        if (*(cache + j) != 0) {
            free(*(cache + j));
        }
    }
    free(cache);
    free(buffer);
    return 0;
}