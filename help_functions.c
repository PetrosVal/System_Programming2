#include "help_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int int_cmp(const void *a, const void*b){
    return ( *(int*)a - *(int*)b );
}

struct mirror_initiator_command create_command(char *mycmd, int length){
    int mychar = *mycmd;
    char content_server_ip[length];
    char args[length];
    int content_server_index = 0;
    int args_index = 0;
    int content_server_ip_found = 0;
    int number_of_args = 0;
    int argv_index = 0;
    char argv_buffer[length];
    int argv_buffer_index = 0;
    struct mirror_initiator_command scmd;
    while(mychar){
        if(!content_server_ip_found && mychar != ':'){
            content_server_ip[content_server_index++] = mychar;
            mycmd++;
            mychar = *mycmd;
        }
        else if(!content_server_ip_found && mychar == ':'){
            /* Found end of cmd name. Consume whitespace. */
            content_server_ip[content_server_index++] = '\0';
            content_server_ip_found = 1;
            while((mychar = *mycmd++) == ':') ;
            if(mychar != '\0') /* End of string */
                number_of_args++;
        }
        else if(mychar == ':'){
            args[args_index++] = mychar;
            /* Consume whitespace. */
            while((mychar = *mycmd++) == ':') ;
            if(mychar != '\0') /* End of string */
                number_of_args++;
        }
        else{
            args[args_index++] = mychar;
            mychar = *mycmd++;
        }
    }
    scmd.argv = malloc(sizeof(char *) * number_of_args);
    for(int i = 0; i < args_index; i++){
        if(args[i] != ':'){
            argv_buffer[argv_buffer_index++] = args[i];
        }
        if(i == args_index - 1 || args[i] == ':'){
            scmd.argv[argv_index] = malloc(sizeof(char) * (argv_buffer_index + 1));
            strncpy(scmd.argv[argv_index], argv_buffer, argv_buffer_index);
            scmd.argv[argv_index][argv_buffer_index] = '\0';
            argv_buffer_index = 0;
            argv_index++;
        }
    }
    
    scmd.argc = number_of_args;
    scmd.content_server_ip = malloc(sizeof(char) * content_server_index);
    strncpy(scmd.content_server_ip, content_server_ip, content_server_index);
    return scmd;
}

void delete_command(struct mirror_initiator_command scmd){
    free(scmd.content_server_ip);
    for(int i = 0; i < scmd.argc; i++){
        free(scmd.argv[i]);
    }
    free(scmd.argv);
}

int is_number(char *str){
    int size = strlen(str);
    for(int i = 0; i < size; i++){
        if(!isdigit(str[i]))
            return 0;
    }
    return 1;
}

int safe_read(int fd, char *buffer, int length){
    int bytesRead;
    int nRead;
    bytesRead = 0;
    nRead = 0;
    
    while(bytesRead < length){
        nRead = read(fd, buffer, length);
        if(nRead == 0)
            return 0;
        if(nRead == -1){
            perror("Socket reading failed. ");
            return 0;
        }
        bytesRead += nRead;
    }
    return 1;
}
