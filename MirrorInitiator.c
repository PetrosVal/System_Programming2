#include "help_functions.h"
#include "Buffer_Queue.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>


int main(int argc, char *argv[]){

	int port = 0;
	char server_name[300] = "";
	char commands[800] = "";

	for(int i = 1; i < argc; i = i + 2){
        if(!strcmp(argv[i], "-n"))
            strcpy(server_name,argv[i + 1]);
        else if(!strcmp(argv[i], "-p"))
            port = atoi(argv[i + 1]);
        else if(!strcmp(argv[i], "-s"))
            strcpy(commands, argv[i + 1]);
    }

    int sock;
    unsigned int serverlen;
    struct sockaddr_in server;
    struct sockaddr *serverptr;
    struct hostent *rem;
   
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Client socket creation failed");
        exit(1);
    }
    if((rem = gethostbyname(server_name)) == NULL){
        perror("gethostbyname by client failed");
        exit(1);
        
    }
    server.sin_family = PF_INET;
    memcpy((char *) &server.sin_addr, (char *) rem->h_addr_list[0], rem->h_length);
    server.sin_port = htons(port);
    serverptr = (struct sockaddr *) &server;
    serverlen = sizeof(server);
    if(connect(sock, serverptr, serverlen) < 0){
        perror("Connection request by client failed");
        exit(1);
    }
    printf("Client requested connection to host %s port %d\n",server_name, port);

    char size_buf[10];
    char *msg;
    int real_size;
  
    while(1){

    	real_size = strlen(commands) + 1;
        sprintf(size_buf, "%09d", real_size);
        printf("\n");
        printf("Sending: %s\n", commands);

        if(write(sock, size_buf, 10) < 0){
            perror("Could not write to socket");
            exit(1);
        }
        /* Send actual message */
        if(write(sock, commands, real_size) < 0){
            perror("Could not write to socket");
            exit(1);
        }

        printf("----------\n");

        if(!safe_read(sock, size_buf, 10)){
            perror("Client could not read");
            exit(1);
        }
        real_size = atoi(size_buf);
        msg = malloc(sizeof(char) * real_size);
        if(!safe_read(sock, msg, real_size)){
            perror("Client could not read from socket");
            exit(1);
        }
        printf("Server Response: %s \n", msg);
        free(msg);

    }

   close(sock);
}
