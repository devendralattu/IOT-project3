//MotionSensor

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

void * connection_handler(void *);
void * connection_handler_2(void *);
void * motionServerThread(void * args);
void * motionConnectKeychainThread(void * args);
void * motionConnectDoorThread(void * args);

typedef struct
{
	char name[15];
	char ip[20];
	int port;
	int value;
	int registered;
	int sock;
	int idNum;
}clientStruct1;

struct sockaddr_in keychain, door;
int sockfdKeychain = 0, sockfdDoor = 0;
int vectorClock[4] = {0};
FILE *fpOutputFile;
char *file1, *file2, *file3;

char *keyIP, *doorIP;
char *motionName, *motionIP;
int keyPort, doorPort, motionPort;

void writeToFile(char * fileData)
{
	fpOutputFile = fopen(file3, "a+");
	fprintf(fpOutputFile, "%s", fileData);
	fclose(fpOutputFile);
}
	
int main(int argc, char* argv[])
{	
	file1 = argv[1];
	file2 = argv[2];
	file3 = argv[3];
/*
	file1 = "MotionSensorConfigurationFile.txt";
	file2 = "MotionSensorStateFile.txt";
	file3 = "MotionSensorOutputFile.txt";
*/	
	int sockfd;
	struct sockaddr_in server;
	char readmsg[2000], msglen;
	pthread_t motSerThread, motConnThread;			

	//To read the Config File 
	FILE *fp1 = fopen(file1,"r");	
	
	//To Get registration data from the File
    int i, j;
    size_t len;
    
    char *gatewayIP; 
    char *gatewayPort; 
    char *gatewayAddress = (char *)malloc(sizeof(char)*100);
    char *configInfoSplit = (char *)malloc(sizeof(char)*100);
    char *configInfo = (char *)malloc(sizeof(char)*100);
    char *line = (char *)malloc(sizeof(char)*100);
	
	//1st line
    getline(&gatewayAddress, &len, fp1);
    gatewayIP = strtok(gatewayAddress, ":");
    gatewayPort = strtok(NULL, "\n");
    
    //2nd line
    getline(&configInfoSplit, &len, fp1);
    strcpy(configInfo, configInfoSplit);
    motionName = strtok(configInfoSplit, ":");
    motionIP = strtok(NULL, ":");
    motionPort = atoi(strtok(NULL, "\n"));
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
    
	msglen = recv(sockfd, readmsg, 2000, 0);
	readmsg[msglen] = '\0';
	puts(readmsg);
	
	//get message to proceed!
	msglen = recv(sockfd, readmsg, 200, 0);
	readmsg[msglen] = '\0';
	puts(readmsg);
	while(strcmp(readmsg, "registered") < 0)
	{
		puts("Gateway asked to proceed");
		break;	
	}
	
	//Read IP Address and port of other sensors
	line = strtok(readmsg, ":");
	i = 3;
	while(i--)
	{
		line = strtok(NULL, ":");
		if(strcmp(line, "keychain") == 0)
		{
			keyIP = strtok(NULL, ":");
			line = strtok(NULL, ":");
			keyPort = atoi(line);
		}
		else if(strcmp(line, "door") == 0)
		{
			doorIP = strtok(NULL, ":");
			line = strtok(NULL, ":");	
			doorPort = atoi(line);
		}		
		else if(strcmp(line, "motionsensor") == 0)
		{
			line = strtok(NULL, ":");
			line = strtok(NULL, ":");
		}
	}
	
	sleep(1);	
	//Create server and connect with other sensors
	pthread_create(&motSerThread, NULL, &motionServerThread, NULL);	
	
	sleep(7);
	//Connect with other sensors
	pthread_create(&motConnThread, NULL, &motionConnectKeychainThread, NULL);	
	pthread_create(&motConnThread, NULL, &motionConnectDoorThread, NULL);	
	
	sleep(2);
	//To read state file and send values to Gateway
	FILE *fp2 = fopen(file2,"r");
	char *state = (char *)malloc(sizeof(char)*100);
	char *temp = (char *)malloc(sizeof(char)*100);
	char *msgToOtherSensors = (char *)malloc(sizeof(char)*100);
	int currTime;
	int nextTime = 0;
	char *value = (char *)malloc(sizeof(char)*100);
	char valCopy[5];
	char fileMsg[100];
			
	//Getting individual parts of state file : initial time, end time, value
	i = 0;
	
	//Sending values to gateway every 5 seconds
	while(i <= nextTime)
	{
		if(i == nextTime)
		{		
			if(getline(&state, &len, fp2) < 0)
			{				
				if(sockfdKeychain > 0 && sockfdDoor > 0)
				{
					vectorClock[1] = vectorClock[1] + 1;
					printf("[%d,%d,%d]", vectorClock[0], vectorClock[1], vectorClock[2]);		

					sprintf(temp, "%d;%s;%d;%d;%d",time(NULL), valCopy, vectorClock[0], vectorClock[1], vectorClock[2]);
					write(sockfd, temp, strlen(temp));

					sprintf(msgToOtherSensors, "motion;%d;%d;%d", vectorClock[0], vectorClock[1], vectorClock[2]);
					write(sockfdKeychain, msgToOtherSensors, strlen(msgToOtherSensors));
					write(sockfdDoor, msgToOtherSensors, strlen(msgToOtherSensors));

					sprintf(fileMsg, "Sent:[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
					writeToFile(fileMsg);
				}	
				puts(temp);
				break;
				/*
				rewind(fp2);
				i = 0;
				nextTime = 0;
				continue;
				*/
			}
			temp = strtok(state, ";");
			currTime = atoi(temp);
			temp = strtok(NULL, ";");
			nextTime = atoi(temp);
			value = strtok(NULL, ";");
			strcpy(valCopy, value);
			valCopy[strlen(valCopy) - 1] = '\0';
		}
		
		if((i % 5) == 0)
		{	
			
			if(sockfdKeychain > 0 && sockfdDoor > 0)
			{
				vectorClock[1] = vectorClock[1] + 1;
				printf("[%d,%d,%d]",vectorClock[0],vectorClock[1],vectorClock[2]);

				sprintf(temp, "%d;%s;%d;%d;%d", time(NULL), valCopy, vectorClock[0], vectorClock[1], vectorClock[2]);
				write(sockfd, temp, strlen(temp));

				sprintf(msgToOtherSensors, "motion;%d;%d;%d", vectorClock[0],vectorClock[1],vectorClock[2]);
				write(sockfdKeychain, msgToOtherSensors, strlen(msgToOtherSensors));
				write(sockfdDoor, msgToOtherSensors, strlen(msgToOtherSensors));

				sprintf(fileMsg, "Sent:[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
					writeToFile(fileMsg);
			}
			puts(temp);
		}
		
		sleep(1);
		i++;
	}
	fclose(fp2);
	return 0;
} 

void * motionServerThread(void * args)
{
	int sockfdMotion, clientsockfd;
	struct sockaddr_in serverMotion, client;
	pthread_t thread1;
	int c;
	
	//Create Socket
	if((sockfdMotion = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("\nCould not create Motion socket...");
	}
	
	puts("\nMotion socket is created");
	
	//Define SockAddr struct
	serverMotion.sin_family = AF_INET;
	serverMotion.sin_addr.s_addr = INADDR_ANY;
	serverMotion.sin_port = htons(motionPort);	
	
	//Bind the socket and server address
	if(bind(sockfdMotion, (struct sockaddr *) &serverMotion, sizeof(serverMotion)) < 0)
	{
		perror("\nMotion : Unable to Bind socket and address");
		//return 1;
	}	
	puts("Motion : Socket and address binded");
	puts("Motion : Waiting for registrations. . .\n");

	//Listen for connections
	listen(sockfdMotion, 10);
	
	printf("sockfdMotion = %d\n",sockfdMotion);
	
	sleep(1);
	
	c = sizeof(struct sockaddr_in);
	//Accept a connection
	while (clientsockfd = accept(sockfdMotion,(struct sockaddr *) &client, (socklen_t*)&c))
	{
		clientStruct1 cs1;
		cs1.sock = clientsockfd;

		printf("\nAccepted a connection");
		
		if(pthread_create(&thread1, NULL, connection_handler, (void *) &cs1) < 0)
		{
			perror("\nMotion : Job thread creation failed");
		}
		
		puts("\nMotion : Job is taken");
	}
	
	if(clientsockfd < 0)
	{
		perror("\nMotion connection terminated");
	}
}

void * connection_handler(void * cs1)
{	
	pthread_t motConn2Thread;
	clientStruct1 cs = *(clientStruct1*)(cs1);
	char *writemsg = "Message from motion";
	int msglen;
	char readmsg[2000];
	
	printf("Writing message from motion to cs.sock = %d\n",cs.sock);			
	
	// ------ receive message ---------
	pthread_create(&motConn2Thread, NULL, &connection_handler_2, (void *) &cs);
}

void * connection_handler_2(void * cs2)
{
	clientStruct1 cs_r = *(clientStruct1*)(cs2);
	char *writemsg = "Message from keychain";
	int msglen, i, vectVal[4] = {0};
	char readmsg[2000];
	char vectInfo[50];
	char fileMsg[100];
	
	printf("Reading message from cs_r.sock = %d\n", cs_r.sock);			
	while(msglen = recv(cs_r.sock, readmsg, 2000, 0) > 0)
	{
		puts(readmsg);
		if(strstr(readmsg, "keychain;") != NULL)
		{
			strcpy(vectInfo, strtok(readmsg, ";"));//keychain
			printf("vectInfo = %s\n", vectInfo);			

			for(i=0; i<3; i++)
			{
				vectVal[i] = atoi(strtok(NULL,";"));
				printf("vectVal[%d] = %d ; ",i,vectVal[i]);
				if(vectVal[i] > vectorClock[i])
				{
					vectorClock[i] = vectVal[i];
				}
			}
			printf("\n");
		}
		if(strstr(readmsg, "door;") != NULL)
		{
			strcpy(vectInfo, strtok(readmsg, ";"));//door
			printf("vectInfo = %s\n", vectInfo);			

			for(i=0; i<3; i++)
			{
				vectVal[i] = atoi(strtok(NULL,";"));
				printf("vectVal[%d] = %d ; ",i,vectVal[i]);
				if(vectVal[i] > vectorClock[i])
				{
					vectorClock[i] = vectVal[i];
				}
			}
			printf("\n");
		}
		printf("[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);	
		sprintf(fileMsg, "Recv:[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
		writeToFile(fileMsg);	
		memset(readmsg, 0, 2000);
	}
	if(msglen == 0)
	{
		printf("\nKeychain Job dropped\n");
		fflush(stdout);
	}
}

void * motionConnectKeychainThread(void * args)
{
	//create KEYCHAIN socket
	if((sockfdKeychain = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create sockfdKeychain Sensor socket");
	}

	//Initialise the backend socket
	keychain.sin_family = AF_INET;
	keychain.sin_addr.s_addr = inet_addr(keyIP);
	keychain.sin_port = htons(keyPort);

	//Connect to gateway
	if((connect(sockfdKeychain, (struct sockaddr *) &keychain, sizeof(keychain))) < 0)
	{
		perror("\nUnable to connect to keychain");
	}
	
	puts("motion: Connected to keychain");	
}

void * motionConnectDoorThread(void * args)
{
	//create DOOR socket
	if((sockfdDoor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create sockfdDoor Sensor socket");
	}

	//Initialise the backend socket
	door.sin_family = AF_INET;
	door.sin_addr.s_addr = inet_addr(doorIP);
	door.sin_port = htons(doorPort);

	//Connect to gateway
	if((connect(sockfdDoor, (struct sockaddr *) &door, sizeof(door))) < 0)
	{
		perror("\nUnable to connect to door");
	}
	
	puts("motion: Connected to door");
}

