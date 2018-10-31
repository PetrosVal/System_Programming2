#include "help_functions.h"
#include "Buffer_Queue.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>


#define BACKLOG 100
#define max_servers 10
#define MAX_SOCKETS 1000

void * worker_job(void * arg);
void * Mirror_Manager(void * arg);


int mirror_initialize(char *filename, fileptr *data);

/* Socket queue with jobs coming from clients */
pthread_mutex_t socket_queue_mtx;
pthread_cond_t socket_queue_cond_empty;
pthread_cond_t all_done;


pthread_mutex_t mirror_manager_mutex;

pthread_t mirror_managers[max_servers];

int port = 0;
int queue_size=0;
int thread_num = 0;
char file_name[300] = "";
int count=0;
struct mirror_initiator_command* initiator_commands;
fileptr data_mirror;

int sock;
struct sockaddr_in client;
unsigned int clientlen;
struct sockaddr *clientptr;
struct hostent *rem;
DIR* dir_ptr;
int st;


int main(int argc, char *argv[]){
	int newsock;
	const char s[2] = ",";
	char* token;

	for(int i = 1; i < argc; i = i + 2){
        if(!strcmp(argv[i], "-p"))
            port = atoi(argv[i + 1]);
        else if(!strcmp(argv[i], "-m"))
            strcpy(file_name,argv[i + 1]);
        else if(!strcmp(argv[i], "-w"))
            thread_num = atoi(argv[i + 1]);       
    }
    st=mkdir(file_name,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ((dir_ptr = opendir(file_name)) == NULL) {
		fprintf(stderr, "cannot open or create %s\n", file_name);
		exit(1);
	}


    printf("MirrorServer initialized with: \n");
    printf("Port: %d \n", port);
    printf("Thread_num: %d\n", thread_num);
    printf("filename: %s\n", file_name);

    /* Preventing server from crashing when client is killed. */
    signal(SIGPIPE, SIG_IGN);

    /* Initializing mutexes and cond vars */
    pthread_mutex_init(&socket_queue_mtx, 0);
    pthread_mutex_init(&mirror_manager_mutex, 0);
    pthread_cond_init(&socket_queue_cond_empty, 0);
    pthread_cond_init(&all_done, 0);

     /* Creating our thread pool */
    pthread_t workers[thread_num];
    for(int i = 0; i < thread_num; i++){
        pthread_create(&(workers[i]), 0, worker_job, 0);
    }


     /* Initializing sockets */
    struct sockaddr_in server;
    unsigned int serverlen;
    struct sockaddr *serverptr;

    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        exit(1);
    }
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    serverptr = (struct sockaddr *) &server;
    serverlen = sizeof(server);
    if(bind(sock, serverptr, serverlen) < 0){
        perror("Could not bind socket to address");
        exit(1);
    }
    if(listen(sock, BACKLOG) < 0){
        perror("Listening for connections failed");
        exit(1);
    }
    clientptr = (struct sockaddr *) &client;
    clientlen = sizeof(client); // TODO: check this here

    printf("MirrorServer running on port: [%d]\n", port);

    while(1){

    	char *mymsg;
        char size_buf[10];
        int content_server_commands_size;
        initiator_commands=malloc(max_servers*sizeof(struct mirror_initiator_command));

    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0){

              perror("Could not accept connection");
              exit(1);          
        }

        if((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr),client.sin_family)) == NULL){

              perror("gethostbyaddr failed");
              exit(1);            
         } 
         printf("Accepted connection from %s (Socket: %d)\n", rem->h_name, newsock);   

          while(1){ /* Loops around till client leaves */
            /* Read size of command coming */ 
            if(!safe_read(newsock, size_buf, 10)){
                printf("Client terminated connection.\n");
                break;
            }
            content_server_commands_size = atoi(size_buf);
            mymsg = malloc(sizeof(char) * content_server_commands_size);
            if(!safe_read(newsock, mymsg, content_server_commands_size)){
                printf("Client terminated connection.\n");
                break;
            }
             printf("Commands received: %s\n", mymsg);
             token = strtok(mymsg, s);
             while(token!=NULL){   

             	initiator_commands[count]=create_command(token,strlen(token)+1);
             	token=strtok(NULL,s);
             	count++;
             }    
             //printf("count_mirrir:%d\n",count);
             for(int i=count;i<max_servers;i++){

             	initiator_commands[i].content_server_ip=NULL;
             	initiator_commands[i].argc=0;
             }
             /*for(int i=0;i<count;i++)
             	printf("server_ip %s with args %d\n",initiator_commands[i].content_server_ip,initiator_commands[i].argc);*/
              for(int i=0;i<count;i++){

               	pthread_create(&mirror_managers[i], 0, Mirror_Manager, 0);

             }
             for(int i=0;i<count;i++){

               	  pthread_join(mirror_managers[i], NULL);
             }
             for(int i=0;i<queue_size;i++){

               	  pthread_join(workers[i], NULL);
             }

             //pthread_cond_wait(&all_done, &socket_queue_mtx);     
           } 
    }
}


void * Mirror_Manager(void * arg){

	int contentsock,contentport=0;
    unsigned int contentserverlen;
    struct sockaddr_in contentserver;
    struct sockaddr *contentserverptr;
    struct hostent *contentrem;
    struct in_addr myaddress ;
    FILE* fp;

    pthread_mutex_lock(&mirror_manager_mutex);
   if(count>0){

    if((contentsock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Client socket creation failed");
        exit(1);
    }
    // printf("count:%d\n",count);
     //printf("server_ip:%s\n",initiator_commands[count-1].content_server_ip );
     inet_aton(initiator_commands[count-1].content_server_ip , &myaddress );
     
    if((contentrem = gethostbyaddr(( const char *) &myaddress,sizeof( myaddress),AF_INET)) == NULL){
            perror("gethostbyaddr failed");
            exit(1);            
     } 
    contentport=atoi(initiator_commands[count-1].argv[0]);
    contentserver.sin_family = PF_INET;
    memcpy((char *) &contentserver.sin_addr, (char *) contentrem->h_addr_list[0], contentrem->h_length);
    contentserver.sin_port = htons(contentport);
    contentserverptr = (struct sockaddr *) &contentserver;
    contentserverlen = sizeof(contentserver);
    if(connect(contentsock, contentserverptr, contentserverlen) < 0){
        perror("Connection request by client failed");
        exit(1);
    } 
    count--;
    printf("MirrorManager requested connection to contentserver %s port %d\n",contentrem->h_name, contentport);          
    char size_buf_1[10]="LIST";
    char buf_1[10];
    char *msg_1;
    int size_delay,flag=0;;
    size_delay=strlen(initiator_commands[count].argv[2])+1;
    msg_1=malloc(size_delay*sizeof(char));
    strcpy(msg_1,initiator_commands[count].argv[2]);
    char size_buf_3[10];
    char size_buf_4[30];
    //int size_filename=strlen(file_name)+1;
    char* buffer;
    int real_size_2;
	while(1){
		
	 if(flag==0){

        if(write(contentsock,size_buf_1,10) < 0){
            perror("Could not write to socket");
            exit(1);
        }
        sprintf(buf_1,"%09d",size_delay);
        if(write(contentsock,buf_1, 10) < 0){
            perror("Could not write to socket");
            exit(1);
        }
        if(write(contentsock,msg_1,size_delay) < 0){
            perror("Could not write to socket");
            exit(1);
        }
        if(!safe_read(contentsock,size_buf_3,10)){
              printf("Client terminated connection.\n");
              break;
        }
        real_size_2=atoi(size_buf_3);

        buffer=malloc(real_size_2*sizeof(char));
        if(!safe_read(contentsock,buffer,real_size_2)){
              printf("Client terminated connection.\n");
              break;
        }
        if(!safe_read(contentsock,size_buf_4,30)){
              printf("Client terminated connection.\n");
              break;
        }
        //printf("size_buf_4:%s\n",size_buf_4);
        if ( (fp = fopen(size_buf_4,"rb")) == NULL )
	    {
		       perror("fopen");
	    }
	    fwrite(buffer,real_size_2,1,fp);
	    fclose(fp);

	    mirror_initialize(size_buf_4,&data_mirror);
	    printfile(data_mirror);

        flag=1;
      }
      pthread_mutex_unlock(&mirror_manager_mutex);
      /* Let workers know. */
      pthread_cond_signal(&socket_queue_cond_empty);
	}
  }	
 close(contentsock);
 pthread_exit(0);
}


void * worker_job(void * arg){

    int contentsock,contentport=0;
    unsigned int contentserverlen;
    struct sockaddr_in contentserver;
    struct sockaddr *contentserverptr;
    struct hostent *contentrem;
    struct in_addr myaddress ;
    FILE* fp;
     char onoma[50]="";
     char onoma_arxeiou[400]="";
	  pthread_mutex_lock(&socket_queue_mtx);
       while(queue_size == 0){

            pthread_cond_wait(&socket_queue_cond_empty, &socket_queue_mtx);
        }
        if(queue_size>0){

        	int flag=0;
   
            if((contentsock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
                  perror("Client socket creation failed");
                  exit(1);
             }
             inet_aton(data_mirror->peers->ip , &myaddress );
             if((contentrem = gethostbyaddr(( const char *) &myaddress,sizeof( myaddress),AF_INET)) == NULL){
                  perror("gethostbyaddr failed");
                  exit(1);            
             } 
            contentport=atoi(data_mirror->peers->port);
            contentserver.sin_family = PF_INET;
            memcpy((char *) &contentserver.sin_addr, (char *) contentrem->h_addr_list[0], contentrem->h_length);
            contentserver.sin_port = htons(contentport);
            contentserverptr = (struct sockaddr *) &contentserver;
            contentserverlen = sizeof(contentserver);
            if(connect(contentsock, contentserverptr, contentserverlen) < 0){
              perror("Connection request by client failed");
              exit(1);
             } 
            strcpy(onoma,data_mirror->filename);

            strcpy(onoma_arxeiou,file_name);
            strcat(onoma_arxeiou,data_mirror->peers->ip);
            strcat(onoma_arxeiou,"_");
            strcat(onoma_arxeiou,data_mirror->peers->port);
            strcat(onoma_arxeiou,"_");
            //printf("onoma_arxeiou:%s\n",onoma_arxeiou);

            delete_file(&data_mirror,data_mirror->filename);
        	queue_size--;
        	pthread_mutex_unlock(&socket_queue_mtx);
        	while(1){
        		char size_buf_3[10];
                char size_buf_4[30];
                char* buffer;
                int real_size_2;

        	 if(flag==0){

        	   char size_buf_1[10]="FETCH";
               char buf_1[10];
               int size_filename;
               char* msg1;
               size_filename=strlen(onoma)+1;
               msg1=malloc(size_filename*sizeof(char));
               strcpy(msg1,onoma);

        	   if(write(contentsock,size_buf_1,10) < 0){
                   perror("Could not write to socket");
                   exit(1);
               }
               sprintf(buf_1,"%09d",size_filename);
               if(write(contentsock,buf_1, 10) < 0){
                  perror("Could not write to socket");
                  exit(1);
               }
               if(write(contentsock,msg1,size_filename) < 0){
                  perror("Could not write to socket");
                  exit(1);
               }
               if(!safe_read(contentsock,size_buf_3,10)){
                   printf("Client terminated connection.\n");
                   break;
               }
               real_size_2=atoi(size_buf_3);
               //printf("real_size_2:%d\n",real_size_2);
               buffer=malloc(real_size_2*sizeof(char));
               if(!safe_read(contentsock,size_buf_4,size_filename)){
                    printf("Client terminated connection.\n");
                    break;
               }
              // printf("size_buf_4:%s\n",size_buf_4);
               if(!safe_read(contentsock,buffer,real_size_2)){
                   printf("Client terminated connection.\n");
                   break;
               }
               strcat(onoma_arxeiou,size_buf_4);
               printf("onoma_arxeiou:%s\n",onoma_arxeiou);
               if ( (fp = fopen(onoma_arxeiou,"w")) == NULL )
	          {
		            perror("fopen");
	          }
	          fwrite(buffer,real_size_2,1,fp);
	          fclose(fp);

               flag=1;
            }
            strcpy(onoma_arxeiou,"");
            strcpy(size_buf_4,"");
        }
    }   
}




int mirror_initialize(char *filename, fileptr *data)
{
	DIR             *dip;
	struct dirent   *dit;
	int             i = 0;
	fileptr tempnode;
	//anoigma katalogou
	if ((dip = opendir(filename)) == NULL)
	{
		perror("opendir");
		return -1;
	}
	//update-mono nea arxeia mpainoun sti lista
	while ((dit = readdir(dip)) != NULL)
	{
		i++;
		if (dit->d_type!=DT_REG)
			continue;

		if ( !strcmp(dit->d_name,".") || !strcmp(dit->d_name,"..") )
			continue;

		if ( !file_in(*data,dit->d_name) ){
			add_file(data,dit->d_name);
			tempnode=filenode(*data,dit->d_name);
			//printf("count:%d\n",count);
			//printf("port:%s\n",initiator_commands[count].argv[0]);
			//printf("ip:%s\n",initiator_commands[count].content_server_ip);
			add_peer(&tempnode,initiator_commands[count].content_server_ip,initiator_commands[count].argv[0]);
			queue_size++;
		}
	}
	//kleisimo katalogou
	if (closedir(dip) == -1)
	{
		perror("closedir");
		return -1;
	}
	return 0;
}