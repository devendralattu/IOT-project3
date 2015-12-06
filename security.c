//Security Device

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <malloc.h>

char *file1, *file3;
FILE * fpOutputFile;

void writeToFile(char * fileData)
{
	fpOutputFile = fopen(file3, "a");
	fprintf(fpOutputFile, "%s\n", fileData);
	fclose(fpOutputFile);
}

int main(int argc, char* argv[])
{
	file1 = argv[1];
	file3 = argv[2];
	
	/****
	file1 = "SecurityConfiguration.txt";
	file3 = "output/SecurityDeviceOutput.log";
	/****/
	
	int sockfd;
	struct sockaddr_in server;
	char readmsg[2000], msglen;
	
	//To read the Config File 
	FILE *fp1 = fopen(file1,"r");
	
	//To Get registration data from the File
    int i, j;
    size_t len;
    char *gatewayIP; 
    char *gatewayPort; 
    char *gatewayAddress = (char *)malloc(sizeof(char)*100);
    char *configInfo = (char *)malloc(sizeof(char)*100);
    char *configInfoSplit = (char *)malloc(sizeof(char)*100);
    
	//1st line
    getline(&gatewayAddress, &len, fp1);
    gatewayIP = strtok(gatewayAddress, ":");
    gatewayPort = strtok(NULL, "\n");
    
    //2nd line
    getline(&configInfo, &len, fp1);
    fclose(fp1);
	
	//create the socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create Sensor socket");
	}
	
	//Initialise the server socket
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(gatewayIP);
	server.sin_port = htons( atoi(gatewayPort) );
	
	//Connect to gateway
	if(connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("\nUnable to connect to Gateway");
		return 1;
	}
	
	puts("Sensor: Connected to Gateway");
	
	msglen = read(sockfd, readmsg, 2000);
	puts(readmsg);
	memset(readmsg, 0, 2000);
	
	write(sockfd, configInfo, strlen(configInfo), 0);
    puts(configInfo);
    
	while(msglen = recv(sockfd, readmsg, 2000, 0) > 0)
	{
		puts(readmsg);
		//write to file
		writeToFile(readmsg);
		
		memset(readmsg, 0, 2000);
	}
	
return 0;	
}	
