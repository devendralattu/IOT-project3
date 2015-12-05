//Door

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

void * connection_handler(void *);
void * connection_handler_2(void *);
void * doorServerThread(void * args);
void * doorConnectMotionThread(void * args);
void * doorConnectKeychainThread(void * args);

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

struct sockaddr_in motion, keychain;
int sockfdMotion = 0, sockfdKeychain = 0;
int vectorClock[4] = {0};
FILE *fpOutputFile;
char *file1, *file2, *file3;

char *keyIP, *motionIP;
char *doorName, *doorIP;
int keyPort, motionPort, doorPort;

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
	
	/***** 
	file1 = "DoorConfiguration.txt";
	file2 = "DoorInput.txt";
	file3 = "DoorOutput.log";
	*****/
	
	int sockfd;
	struct sockaddr_in server;
	char readmsg[2000], msglen;
	pthread_t doSerThread, doConnThread;	
		
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
    doorName = strtok(configInfoSplit, ":");
    doorIP = strtok(NULL, ":");
    doorPort = atoi(strtok(NULL, "\n"));
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
		else if(strcmp(line, "motionsensor") == 0)
		{
			motionIP = strtok(NULL, ":");
			line = strtok(NULL, ":");	
			motionPort = atoi(line);
		}		
		else if(strcmp(line, "door") == 0)
		{
			line = strtok(NULL, ":");
			line = strtok(NULL, ":");
		}
	}
	
	sleep(1);	
	//Create server and connect with other sensors
	pthread_create(&doSerThread, NULL, &doorServerThread, NULL);	
	
	sleep(5);
	//Connect with other sensors
	pthread_create(&doConnThread, NULL, &doorConnectMotionThread, NULL);	
	pthread_create(&doConnThread, NULL, &doorConnectKeychainThread, NULL);	
	
	sleep(2);
	//To read state file and send values to Gateway
	FILE *fp2 = fopen(file2,"r");
	char *state = (char *)malloc(sizeof(char)*100);
	char copyState[20];
	int nextTime;
	char * value;
	char valueNew[50];
	char *temp;
	char msgToOtherSensors[2000];
	char fileMsg[100];
	
	i = 0;
	getline(&state, &len, fp2);
	
	temp = strtok(state,";");
	nextTime = atoi(temp);
	value = strtok(NULL,";");
	sprintf(valueNew, "%s", value);
	valueNew[strlen(valueNew) - 1] = '\0';
	
	//Pushing values to Gateway
	while(i <= nextTime)
	{
		if(i == nextTime)
		{						
			if(sockfdMotion > 0 && sockfdKeychain > 0)
			{		
				vectorClock[2] = vectorClock[2] + 1;
				printf("[%d,%d,%d]",vectorClock[0],vectorClock[1],vectorClock[2]);

				sprintf(copyState, "%d;%s;%d;%d;%d", time(NULL), valueNew, vectorClock[0], vectorClock[1], vectorClock[2]);
				write(sockfd, copyState, strlen(copyState));
				
				sprintf(msgToOtherSensors, "door;%d;%d;%d", vectorClock[0],vectorClock[1],vectorClock[2]);
				write(sockfdMotion, msgToOtherSensors, strlen(msgToOtherSensors));
				write(sockfdKeychain, msgToOtherSensors, strlen(msgToOtherSensors));
				
				sprintf(fileMsg, "Sent:[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
				writeToFile(fileMsg);	
			}
			
			puts(copyState);
			if(getline(&state, &len, fp2) < 0)
			{
				break;
			}
				
			temp = strtok(state,";");
			nextTime = atoi(temp);
			value = strtok(NULL,";");
			sprintf(valueNew, "%s", value);
			valueNew[strlen(valueNew) - 1] = '\0';
		}
		sleep(1);
		i++;
	}
	fclose(fp2);
	return 0;
} 

void * doorServerThread(void * args)
{	
	int sockfdDoor, clientsockfd;
	struct sockaddr_in serverDoor, client;
	pthread_t thread1;
	int c;
	
	//Create Socket
	if((sockfdDoor = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("\nCould not create Door socket...");
	}
	
	puts("\nDoor socket is created");
	
	//Define SockAddr struct
	serverDoor.sin_family = AF_INET;
	serverDoor.sin_addr.s_addr = INADDR_ANY;
	serverDoor.sin_port = htons(doorPort);	
	
	//Bind the socket and server address
	if(bind(sockfdDoor, (struct sockaddr *) &serverDoor, sizeof(serverDoor)) < 0)
	{
		perror("\nDoor : Unable to Bind socket and address");
	}	
	puts("Door : Socket and address binded");
	puts("Door : Waiting for registrations. . .\n");

	//Listen for connections
	listen(sockfdDoor, 10);
	
	printf("sockfdDoor = %d\n",sockfdDoor);
	sleep(1);
	
	c = sizeof(struct sockaddr_in);
	//Accept a connection
	while (clientsockfd = accept(sockfdDoor,(struct sockaddr *) &client, (socklen_t*)&c))
	{
		clientStruct1 cs1;
		cs1.sock = clientsockfd;
		
		printf("\nAccepted a connection");
		
		if(pthread_create(&thread1, NULL, connection_handler, (void *) &cs1) < 0)
		{
			perror("\nDoor : Job thread creation failed");
		}
		
		puts("\nDoor : Job is taken");
	}
	
	if(clientsockfd < 0)
	{
		perror("\nDoor connection terminated");
	}
}

void * connection_handler(void * cs1)
{		
	pthread_t doConn2Thread;
	clientStruct1 cs = *(clientStruct1*)(cs1);

	char *writemsg = "Message from door";
	int msglen;
	char readmsg[2000];
	
	printf("Writing message from door to cs.sock = %d\n",cs.sock);			
	
	// ------ receive message ---------
	pthread_create(&doConn2Thread, NULL, &connection_handler_2, (void *) &cs);
}

void * connection_handler_2(void * cs2)
{
	clientStruct1 cs_r = *(clientStruct1*)(cs2);
	char *writemsg = "Message from door";
	int msglen, i, vectVal[4] = {0};
	char readmsg[2000];
	char vectInfo[50];
	char fileMsg[100];
	
	printf("Reading message from cs_r.sock = %d\n", cs_r.sock);			
	while(msglen = recv(cs_r.sock, readmsg, 2000, 0) > 0)
	{
		puts(readmsg);
		if(strstr(readmsg, "motion;") != NULL)
		{
			strcpy(vectInfo, strtok(readmsg, ";"));//motion
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
		printf("[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
		sprintf(fileMsg, "Recv:[%d,%d,%d]\n",vectorClock[0],vectorClock[1],vectorClock[2]);
		writeToFile(fileMsg);
		memset(readmsg, 0, 2000);
	}
	if(msglen == 0)
	{
		printf("\nJob dropped\n");
		fflush(stdout);
	}
}

void * doorConnectMotionThread(void * args)
{
	//create MOTION socket
	if((sockfdMotion = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create sockfdMotion Sensor socket");
	}

	//Initialise the backend socket
	motion.sin_family = AF_INET;
	motion.sin_addr.s_addr = inet_addr(motionIP);
	motion.sin_port = htons(motionPort);

	//Connect to gateway
	if((connect(sockfdMotion, (struct sockaddr *) &motion, sizeof(motion))) < 0)
	{
		perror("\nUnable to connect to motion");
	}
	
	puts("door: Connected to motion");	
}

void * doorConnectKeychainThread(void * args)
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
	
	puts("door: Connected to keychain");
}

