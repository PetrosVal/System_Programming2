#include "help_functions.h"
#include "Buffer_Queue.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>



#define BACKLOG 100


int port = 0;
char file_name[300] = "";
fileptr data;
char str_port[5];
int delay;


int contentsock;
struct sockaddr_in contentclient;
unsigned int contentclientlen;
struct sockaddr *contentclientptr;
struct hostent *contentrem;

int initialize(char *filename, fileptr *data);


int main(int argc, char *argv[]){
	int content_newsock;
	struct stat filestat;
	ssize_t len;
	char filesize[10];
	int sent_bytes = 0;
	char* buffer;
	int offset;
  int remain_data;
  FILE* fp;


	for(int i = 1; i < argc; i = i + 2){

        if(!strcmp(argv[i], "-p"))
            port = atoi(argv[i + 1]);
        else if(!strcmp(argv[i], "-d"))
            strcpy(file_name,argv[i + 1]);      
    }


    printf("ContentServer initialized with: \n");
    printf("Port: %d \n", port);
    printf("filename: %s\n", file_name);

    /* Preventing server from crashing when client is killed. */
    signal(SIGPIPE, SIG_IGN);

    if ( chdir(file_name) < 0 )			//metavasi sto katalogo 
	{
		perror("chdir");
		return -1;
	}

	data=NULL;
	sprintf(str_port,"%d",port);

    struct sockaddr_in contentserver;
    unsigned int contentserverlen;
    struct sockaddr *contentserverptr;

    if((contentsock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        exit(1);
    }
    contentserver.sin_family = PF_INET;
    contentserver.sin_addr.s_addr = htonl(INADDR_ANY);
    contentserver.sin_port = htons(port);
    contentserverptr = (struct sockaddr *) &contentserver;
    contentserverlen = sizeof(contentserver);
    if(bind(contentsock, contentserverptr, contentserverlen) < 0){
        perror("Could not bind socket to address");
        exit(1);
    }
     if(listen(contentsock, BACKLOG) < 0){
        perror("Listening for connections failed");
        exit(1);
    }
    contentclientptr = (struct sockaddr *) &contentclient;
    contentclientlen = sizeof(contentclient); // TODO: check this here

    printf("ContentServer running on port: [%d]\n", port);
  while(1){
     while(1){

    	if ((content_newsock = accept(contentsock, contentclientptr, &contentclientlen)) < 0){

              perror("Could not accept connection");
              exit(1);          
        }
         if((contentrem = gethostbyaddr((char *) &contentclient.sin_addr.s_addr, sizeof(contentclient.sin_addr.s_addr),contentclient.sin_family)) == NULL){

              perror("gethostbyaddr failed");
              exit(1);            
         } 

        printf("Accepted connection from %s (Socket: %d)\n", contentrem->h_name, content_newsock);

        	char *mymsg;
            char buf[10],buf_1[10];
            int size_1;
            int size_2;
        	if(!safe_read(content_newsock,buf,10)){
                printf("Client terminated connection.\n");
                break;
            }
            if(!strcmp(buf,"LIST")){

             if(!safe_read(content_newsock,buf_1, 10)){
                printf("Client terminated connection.\n");
                break;
              }
             size_1=atoi(buf_1);
             mymsg=malloc(size_1*sizeof(char));
    
              if(!safe_read(content_newsock,mymsg, size_1)){
                printf("Client terminated connection.\n");
                break;
              }
              delay=atoi(mymsg);        
              initialize("./",&data);
                 
              if(stat(file_name,&filestat) < 0)    
                  return 1;
             
               sprintf(filesize,"%ld",filestat.st_size);
                          
               if(write(content_newsock, filesize, 10)<0){
               	perror("write:exit");
               	exit(1);
               }      
               if ( (fp = fopen(file_name,"rb")) == NULL )
	           {
		           perror("fopen");
	           }
	           size_2=(int)filestat.st_size;
	           buffer=malloc(size_2*sizeof(char));
               fread(buffer,size_2,1,fp);
		       fclose(fp);
               if(write(content_newsock,buffer,size_2)<0){
               	  perror("write:exit");
               	  exit(1);
               }
                 if(write(content_newsock,file_name,30)<0){
               	  perror("write:exit");
               	  exit(1);
               }
               
               free(buffer);
               free(mymsg);           
            }else if(!strcmp(buf,"FETCH")){
            	
             if(!safe_read(content_newsock,buf_1, 10)){
                printf("Client terminated connection.\n");
                break;
              }
             size_1=atoi(buf_1);
             mymsg=malloc(size_1*sizeof(char));
             
              if(!safe_read(content_newsock,mymsg, size_1)){
                printf("Client terminated connection.\n");
                break;
              }
              if(stat(mymsg,&filestat) < 0)    
                  return 1;
  
               sprintf(filesize,"%ld",filestat.st_size);
                          
               if(write(content_newsock, filesize, 10)<0){
               	perror("write:exit");
               	exit(1);
               }      
	           size_2=(int)filestat.st_size;
	           buffer=malloc(size_2*sizeof(char));
	           if ( (fp = fopen(mymsg,"rb")) == NULL )
	           {
		           perror("fopen");
	           }
            fread(buffer,size_2,1,fp);
		        fclose(fp);
		       if(write(content_newsock,mymsg,size_1)<0){
               	  perror("write:exit");
               	  exit(1);
            }
            if(write(content_newsock,buffer,size_2)<0){
               	  perror("write:exit");
               	  exit(1);
            }
            sleep(delay);   //kane sleep to delay pou phres
            free(buffer);
            free(mymsg);          
            }
       }        
    }
}



int initialize(char *filename, fileptr *data)
{
	DIR             *dip;
	struct dirent   *dit;
	int             i = 0;
	//fileptr tempnode;
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
		if ( dit->d_type != DT_REG )
			continue;
		if ( !strcmp(dit->d_name,".") || !strcmp(dit->d_name,"..") )
			continue;
		if ( !file_in(*data,dit->d_name) ){
			add_file(data,dit->d_name);
			//tempnode=filenode(*data,dit->d_name);
			//add_peer(&tempnode,contentrem->h_name,str_port);
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


