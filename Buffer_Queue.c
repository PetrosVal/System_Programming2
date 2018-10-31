#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "Buffer_Queue.h"


//epistrefei 1 an uparxei to antistoixo arxeio sti lista//
int file_in(fileptr metadata, char *filename)
{
   while (metadata != NULL) {

	   if (!strcmp(metadata->filename,filename)) 
			return 1;

	   metadata=metadata->next;
	}

   return 0;	
}


//prosthetei to file sti lista an to file den uparxei(ara kalw prwta tin "in")
void add_file(fileptr *metadata, char *filename)
{
	fileptr tmpmeta;
	
	if ( (tmpmeta=calloc(1,sizeof(struct file))) == NULL )
	{
		perror("calloc");
		return;
	}
	strcpy(tmpmeta->filename,filename);
	tmpmeta->peers=NULL;
	tmpmeta->next = *metadata;
	*metadata = tmpmeta; 
	
}


//delete file(kaleitai mono an uparxei to stoixeio sti lista)
void delete_file(fileptr *metadata, char *filename)
{

	fileptr tmp;
	fileptr previous;
	
	if (!strcmp((*metadata)->filename,filename))
	{
		tmp = *metadata;
		*metadata = (*metadata)->next;
		free(tmp);
	}
	else
	{
		previous = *metadata;
		while ( previous->next != NULL) 
		{
			if (!strcmp(previous->next->filename,filename)) 
			{
				tmp = previous->next;
				previous->next=tmp->next;
				free(tmp);
				break;
			}
			previous=previous->next;
		}
	}
}


//release all
void release_metadata(fileptr metadata)
{
	fileptr tmpmeta;
	
	while ((metadata)!=NULL) {
		tmpmeta=metadata;
		metadata=metadata->next;
		free(tmpmeta);
	}
	
}


//epistrefei ton komvo ston opoio vrisketai to antistixo arxeio
fileptr filenode(fileptr metadata, char *filename)
{	
	
		while (metadata != NULL) {

			if (!strcmp(metadata->filename,filename) ) 
				return metadata;

			metadata=metadata->next;
		}

		return NULL;
}


//epistrefei 1 anb o peer uparxei stin lista tou komvou pou pairnei ws orisma
int peer_in(fileptr node, char *ip, char *port)
{
	peerptr temp;
	
	temp=node->peers;
	
	while ( temp != NULL) {
		if ( !strcmp(temp->ip,ip) && !strcmp(temp->port,port)) 
			return 1;
		temp=temp->next;
		
	}
	return 0;
	
}


//prosthetei ton peer ston antisixo komvo
void add_peer(fileptr *node, char *ip, char *port)
{
	peerptr tmp;
	tmp=calloc(1,sizeof(struct peer));
	if ( tmp == NULL )
	{
		perror("calloc");
		return;
	}
	strcpy(tmp->ip,ip);
	strcpy(tmp->port,port);
	tmp->next = (*node)->peers;
	(*node)->peers = tmp; 

}


//delete peer
void delete_peer(fileptr *node, char *ip, char *port)
{
	peerptr tmp;
	peerptr previous;
	
	tmp=(*node)->peers;
	
	if (!strcmp(tmp->ip,ip) && !strcmp(tmp->port,port)) 
	{
		tmp = (*node)->peers;
		(*node)->peers = (*node)->peers->next;
		free(tmp);
	}
	else
	{
		previous = (*node)->peers;
		while ( previous->next != NULL) 
		{
			if (!strcmp(previous->next->ip,ip) && !strcmp(previous->next->port,port)) 
			{
				tmp = previous->next;
				previous->next=tmp->next;
				free(tmp);
				break;
			}
			previous=previous->next;
		}
	}
	
}

//release peers
void release_peers(fileptr node)
{
	peerptr tmp;
	
	while ((node->peers)!=NULL) {
		tmp=node->peers;
		node->peers=node->peers->next;
		free(tmp);
	}
	
}

void printfile(fileptr metadata)
{
	while (metadata != NULL) {
		printf("<%s> \n",metadata->filename);
		printf("Peers:\n");
		printpeer(metadata);
		metadata = metadata->next; 
	}
	printf("---------\n\n\n");
}

void printpeer(fileptr node)
{
	peerptr temp;
	temp=node->peers;
	while (temp != NULL) {
		printf("%s %.32s  \n",temp->ip,temp->port);
		temp=temp->next;
	}
	printf("-\n");
}

