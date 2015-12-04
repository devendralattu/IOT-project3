//Backend

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

int sockfd, gatewaysockfd, c;

FILE * fpOutputFile;
char *file1, *file3;

char *backendName, *backendIP;
int backendPort;


void writeToFile(char * fileData)
{
	fpOutputFile = fopen(file3, "a");
	fprintf(fpOutputFile, "%s\n", fileData);
	fclose(fpOutputFile);
}

int main(int argc, char* argv[])
{
	struct sockaddr_in gate, backend;
	
	file1 = argv[1];
	file3 = argv[2];
	
	/*** 
	file1 = "BackendConfiguration.txt";
	file3 = "PersistentStorage.txt";
	***/
	
	//Database
	//FILE *fp = fopen("PersistentStorage.txt", "w");	
	
	//To read the Config File 
	FILE *fp1 = fopen(file1,"r");	
	
	//To Get registration data from the File
    int i, j;
    size_t len;
    
    char *backendAddress = (char *)malloc(sizeof(char)*100);
    
	//1st line
    getline(&backendAddress, &len, fp1);
    backendIP = strtok(backendAddress, ":");
    backendPort = atoi(strtok(NULL, "\n"));
    fclose(fp1);
    	
	//Create Socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create Backend socket");
	}
	puts("\nBackend socket is created");	
	
	backend.sin_family = AF_INET;
	backend.sin_addr.s_addr = INADDR_ANY;
	backend.sin_port = htons( backendPort );
	
	//Bind the socket and server address
	if((bind(sockfd, (struct sockaddr *) &backend, sizeof(backend))) < 0)
	{
		perror("\nUnable to Bind socket and address");
		return 1;
	}	
	puts("Socket and address binded");
	puts("Waiting for registrations. . .\n");

	//Listen for connections
	listen(sockfd, 5);
	
	c = sizeof(struct sockaddr_in);
	while (gatewaysockfd = accept(sockfd,(struct sockaddr *) &gate, (socklen_t*)&c))
	{
		char readmsg[200];
		char entry[200];
		char temporary[200];
		size_t msglen;

		while((msglen = recv(gatewaysockfd, readmsg, 200, 0)) > 0 )
		{
			readmsg[msglen] = '\0';
			printf("<<<<<<<Received Message: %s>>>>>>\n", readmsg);
			
			writeToFile(readmsg);  
			
			memset(readmsg, 0, 2000);	
		}
	}	
}
