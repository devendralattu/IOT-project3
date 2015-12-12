//Server Program

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#define heartBeat     ((const unsigned char *)"Alive")

void * connection_handler(void *);
void * threadReadFun(void *);
void * sendingGatewayThread();
void * readRegistrationInfo();
void * threadGatewayReceive();

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

typedef struct
{
	char name[15];
	char ip[20];
	int port;
	int value;
	int registered;
	int sock;
	int idNum;
}clientStruct;
static clientStruct csArr[15] = {0};

//CH //0 = keychain, 1 = motion, 2 = door, 3 = security
int currState[4] = {0};
int prevState[4] = {0};

int id = -1;
int reg = -1;
char registerIds[300];
char registrationInfo[2000];
int setReg = 1;
int setReg2 = 0;
int regAtSecGateway = 0;
int createGatewayRecvThread = 1;

int backsockfd, securitySockId, primary, gSockFd;
int vectorClock[4] = {0};
FILE *fpOutputFile;
char *file1, *file3;	

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread1, threadRRI, threadGRT;

int startHeartBeatMsg = 0;

int Count;
char Buffer[2000];
int REQUEST_TO_PREPARE = 0;
int VOTE_COMMIT = 0;
int COMMIT = 0;
int DONE = 1;
char globalWrite[1000];

//-----------------------------------------
void writeToFile(char * fileData)
{
	fpOutputFile = fopen(file3, "a+");
	fprintf(fpOutputFile, "%s", fileData);
	fclose(fpOutputFile);
}

void writeToOtherGateway(char *fileMsg)
{
	write(gSockFd, fileMsg, strlen(fileMsg));
	printf("\nWriting to from gateway to gateway\n");	
}

int main(int argc, char *argv[])
{	
	file1 = argv[1];
	file3 = argv[2];
	
	/*****/
	file1 = "GatewayConfiguration1.txt";
	file3 = "output/GatewayOutput1.log";
	/*****/
	
	int sockfd,clientsockfd;
	struct sockaddr_in server, client;
	
	//To read the Config File 
	FILE *fp1 = fopen(file1,"r");	
	
	//To Get registration data from the File
    int i, j;
    size_t len;
    
    char *gatewayName;
    char *gatewayIP; 
    char *pGatewayName, *pGatewayIP;
    int gatewayPort, pGatewayPort, backendPort;
    
    char *backendIP; 
    char *backendName; 
    char *backendAddress = (char *)malloc(sizeof(char)*100);
    char *configInfo = (char *)malloc(sizeof(char)*100);
    char *configInfoSplit = (char *)malloc(sizeof(char)*100);
    char *line = (char *)malloc(sizeof(char)*100);
	
	//1st line
    getline(&backendAddress, &len, fp1);
    
    backendName = strtok(backendAddress, ":");
    backendIP = strtok(NULL, ":");
    backendPort = atoi(strtok(NULL, "\n"));
    
    //2nd line
    getline(&configInfoSplit, &len, fp1);
    strcpy(configInfo, configInfoSplit);
    gatewayName = strtok(configInfoSplit, ":");
    gatewayIP = strtok(NULL, ":");
    gatewayPort = atoi(strtok(NULL, "\n"));
    
    //3rd line
    memset(configInfoSplit, 0, sizeof(configInfoSplit));
    
    getline(&configInfoSplit, &len, fp1);
    pGatewayName = strtok(configInfoSplit, ":");
    pGatewayIP = strtok(NULL, ":");
    pGatewayPort = atoi(strtok(NULL, "\n"));
    
    fclose(fp1);
    
    if(strcmp(gatewayIP, pGatewayIP) == 0 && gatewayPort == pGatewayPort)
    {
    	// this is primary gateway;
    	printf("\nI am primary Gateway\n");
    	primary = 1;
    }
    else
    {
		//connect another gateway	
		struct sockaddr_in pGateway;
		//create the socket
		if((gSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("\nCould not create other Gateway socket.. Please run primary Gateway first\n");
		}

		//Initialise the pGateway socket
		pGateway.sin_family = AF_INET;
		pGateway.sin_addr.s_addr = inet_addr(pGatewayIP);
		pGateway.sin_port = htons( pGatewayPort );

		runagain:
		//Connect gateway to primary gateway
		if((connect(gSockFd, (struct sockaddr *) &pGateway, sizeof(pGateway))) < 0)
		{
			perror("\nUnable to connect Gateway to other Gateway..Waiting for connection from primary Gateway..Trying again..\n");
			sleep(2);
			goto runagain;			
			//return;
		}
	
		puts("gateway: Connected to other Gateway");
	
		write(gSockFd, configInfo, strlen(configInfo), 0);
		puts(configInfo);
		
		//sleep(2);
		//create a new thread for sending messages
		//pthread_create(&thread1, NULL, &sendingGatewayThread, NULL);
		pthread_create(&threadRRI, NULL, &readRegistrationInfo, NULL);
    }
    
	int c;
	strcpy(registerIds,"registered");

	//Create Socket
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("\nCould not create Gateway socket...");
	}
	
	puts("\nGateway socket is created");
	
	//Define SockAddr struct
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( gatewayPort );	
	
	//Bind the socket and server address
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("\nUnable to Bind socket and address");
		return 1;
	}	
	puts("Socket and address binded");
	puts("Waiting for registrations. . .\n");

	//Listen for connections
	listen(sockfd, 10);
	
	//-------------------------------------------------------------------------------------------------------	
	//connect_backend	
	struct sockaddr_in backend;
	//create the socket
	if((backsockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("\nCould not create Sensor socket");
	}

	//Initialise the backend socket
	backend.sin_family = AF_INET;
	backend.sin_addr.s_addr = inet_addr(backendIP);
	backend.sin_port = htons( backendPort );

	//Connect gateway to backend
	if((connect(backsockfd, (struct sockaddr *) &backend, sizeof(backend))) < 0)
	{
		perror("\nUnable to connect Gateway to Backend");
		return;
	}
	
	puts("gateway: Connected to backend");
	//-------------------------------------------------------------------------------------------------------
	
	//Initialize starting values: Assuming user is at home
    currState[0] = 1; //keychain true
    currState[1] = 0; //motion false
    currState[2] = 0; //door closed
    currState[3] = 0; //sec system off
    
	c = sizeof(struct sockaddr_in);
	while (clientsockfd = accept(sockfd,(struct sockaddr *) &client, (socklen_t*)&c))
	{
		clientStruct1 cs1;
		cs1.sock = clientsockfd;
		//Accept a connection
		printf("\nAccepted a connection");
		
		if(pthread_create(&thread1, NULL, connection_handler, (void *) &cs1) < 0)
		{
			perror("\nJob thread creation failed");
			return 1;
		}
		
		puts("\nJob is taken");
	}
	
	if(clientsockfd < 0)
	{
		perror("\nGateway connection terminated");
		return 1;
	}
} 

void * connection_handler(void * cs1)
{	
	sleep(1);
	id++;
	int itemId = id;
    pthread_t thread1;
	clientStruct1 cs = *(clientStruct1*)(cs1);
	csArr[itemId].sock = cs.sock;	

	printf("csArr[%d].sock = %d\n",itemId,cs.sock);		
		
	int msglen;
	
	char readmsg[2000];
	char *regInfo, temp;
	char *writemsg;
	char tempo[200];
	
	writemsg = "Gateway: Connection established";
	
	//Send msg to Client
	write(csArr[itemId].sock, writemsg, strlen(writemsg)); 
	
	msglen = recv(csArr[itemId].sock, readmsg, 2000, 0);
	regInfo = (char *)readmsg;
	
	puts(regInfo);
	//Get Client Registration Information

	strcpy(csArr[itemId].name, strtok(regInfo, ":"));
	strcpy(csArr[itemId].ip, strtok(NULL,":"));
	csArr[itemId].port = atoi(strtok(NULL,":"));
	csArr[itemId].registered = 1;
	csArr[itemId].value = 0;
	csArr[itemId].idNum = itemId;
	
	strcpy(tempo, csArr[itemId].name);
	writemsg = (char *)tempo; 
	strcat(writemsg, " ");
	strcat(writemsg, csArr[itemId].ip);
	strcat(writemsg, " Registration complete.\nWaiting for other devices to connect before we can start\n"); 
	
	//store other Gatewaysockfd value
	if(strstr(csArr[itemId].name, "ateway") != NULL)
	{
		gSockFd = csArr[itemId].sock;
		printf("\ncsArr[itemId].name, \"ateway\" >>> gSockFd = %d\n", gSockFd);
	}
	sleep(1);
	
	puts(writemsg);
	write(csArr[itemId].sock, writemsg, strlen(writemsg));
	memset(readmsg, 0, 2000);
	
	//check if message is received from 3 sensors or security device.
	if((strstr(csArr[itemId].name, "door") != NULL) || (strstr(csArr[itemId].name, "motion") != NULL) || (strstr(csArr[itemId].name, "keychain") != NULL) || (strstr(csArr[itemId].name, "security") != NULL))
	{
		reg++;
		
		if(strstr(csArr[itemId].name, "security") != NULL)
		{
			securitySockId = csArr[itemId].sock;
		}
		//if received from all sensors, order all sensors to proceed
		//some code and logic here
		while(1)
		{
			if(primary)
			{
				if(id >= 2 && reg >= 2)
				{
					break;		
				}	
			}
			else
			{			
				if(id >= 1 && reg >= 1)
				{
					break;		
				}
			}
			sleep(1);
		}	
	
		// Generate only one common register message and pass to all sensors
		int i, port;
		char portStr[10];	
		if(setReg == 1)
		{
			setReg = 0;
			for(i=0; i < (2+primary); i++)
			{
				if((strstr(csArr[i].name, "security") == NULL) && (strstr(csArr[i].name, "atewa") == NULL))
				{
					port = csArr[i].port;
					sprintf(portStr, "%d", port);
			
					strcat(registerIds, ":");
					strcat(registerIds, csArr[i].name);
					strcat(registerIds, ":");
					strcat(registerIds, csArr[i].ip);
					strcat(registerIds, ":");
					strcat(registerIds, portStr);
				}
			}
			printf("\nmessage to pass to P/S Gateway = '''\n%s\n'''\n",registerIds);
			if(primary)
			{
				write(gSockFd, registerIds, strlen(registerIds));
				while(msglen = recv(gSockFd, registrationInfo, 2000, 0) <= 0)
				{
					continue;
				}
				printf("\nInside if part >> registrationInfo = %s\n", registrationInfo);
				setReg2 = 1;
			}
			else
			{
				//wait till we receive and process registerIds from primary gateway and send other gateway's reg info back to it.
				//while(registrationInfo == NULL)
				while(regAtSecGateway == 0)
				{
					continue;
				}
				printf("\nInside else part >> registrationInfo = %s\n", registrationInfo);
				setReg2 = 1;
			}		
		}		
		
		while(setReg2 == 0)
		{
			sleep(0.5);
			continue;
		}
		// Acknowledge sensors that everyone has been registered and they should now proceed.
		if(setReg2)
		{
			puts("Inside setReg2");
			
			write(csArr[itemId].sock, registrationInfo, strlen(registrationInfo));	

			pthread_create(&thread1, NULL, &threadReadFun, (void *)&csArr[itemId]);
			
			//create receiving thread for gateway's messages only once.
			if(createGatewayRecvThread)
			{
				createGatewayRecvThread = 0;
				pthread_create(&threadGRT, NULL, &threadGatewayReceive, NULL);
			}
		}	
	}
	else if(strstr(csArr[itemId].name, "ateway") != NULL)
	{
		reg++;
		/*
		if(startHeartBeatMsg)
		{
			char readmsgGateway[2000];
			printf("\nMessage received from Gateway\n");
		
			while(msglen = recv(gSockFd, readmsgGateway, 2000, 0) > 0 )
			{
				printf("Message received = %s\n", readmsgGateway);
				writeToFile(readmsgGateway);
			
				write(gSockFd, heartBeat, strlen(heartBeat));
				memset(readmsg, 0, sizeof(readmsgGateway));
			}
		}
		*/	
	}																																																			
}

void* threadReadFun(void *cs1)
{
	int msglen, i, vectVal[4] = {0};
	char readmsg[2000];
	char readBackmsg[2000];
	char sendmsg[2000];
	char sendToGMsg[2000];
	char vectInfo[50];
	char fileMsg[100];
	int switcher = 0;
	 
	clientStruct1 cs = *(clientStruct1*)(cs1);
	while(msglen = recv(cs.sock, readmsg, 2000, 0) > 0 )
	{
		printf("Message received from %s = %s\n", cs.name, readmsg);
		strcpy(readBackmsg, readmsg);
		sprintf(fileMsg, "Message received from %s = %s\n", cs.name, readBackmsg);
		sprintf(sendToGMsg, "%d;%s;%s;%d;%s", cs.idNum, cs.name, cs.ip, cs.port, readBackmsg);
		
		/*
		pthread_mutex_lock(&mutex);
		
		//send the same message to other Gateway
		writeToOtherGateway(sendToGMsg);
		memset(sendToGMsg, 0, sizeof(sendToGMsg));
		*/
		
		writeToFile(fileMsg);	
			
		if(primary)
		{
		
			//{
				printf("In Primary sersor recv. . . . .");
				puts(readmsg);
				
				strcat(globalWrite, "\n");
				strcat(globalWrite, sendToGMsg);
				DONE = 0;
				//sprintf(globalGWrite, "%d;%s;%s;%d;%s", cs.idNum, cs.name, cs.ip, cs.port, BUFFER);
				sprintf(sendToGMsg, "Request_To_Prepare:%s\n", globalWrite);
				printf("\nSending message:Req to prep\n ");
				puts(sendToGMsg);
				//memset(BUFFER, 0, sizeof(BUFFER));
			
				//send message to secondary Gateway
				writeToOtherGateway(sendToGMsg);
			
				//memset(readmsg, 0, sizeof(readmsg));
				//memset(sendToGMsg, 0, sizeof(sendToGMsg));
			//}	
		}
		else
		{
				printf("\nSending msg received from sensor to primary gateway\n");
				puts(sendToGMsg);
				writeToOtherGateway(sendToGMsg);
		}	
		
		strcpy(vectInfo, strtok(readmsg, ";"));//timestamp
		strcpy(vectInfo, strtok(NULL, ";"));//state
		
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
		
		memset(sendmsg, 0, sizeof(sendmsg));
		memset(readmsg, 0, sizeof(readmsg));
		memset(readBackmsg, 0, sizeof(readBackmsg));
	}
	if(msglen == 0)
	{
		printf("\nJob %s dropped\n", cs.name);
		fflush(stdout);
	}
	
	else if(msglen < 0)
	{
		perror("\nDisconnected "); 
	}
}

void* sendingGatewayThread()
{
	int msglen;
	char readmsg[2000];
	printf("\n primary gSockFd = %d\n", gSockFd);
	printf("inside while of sendingGatewayThread\n");
	while(1)
	{
		puts(heartBeat);
		write(gSockFd, heartBeat, strlen(heartBeat), 0);

		memset(readmsg, 0, sizeof readmsg);
		while(msglen = recv(gSockFd, readmsg, 2000, 0) <= 0)
		{
			continue;
		}
		printf("\nMessage received = %s\n", readmsg);
		writeToFile(readmsg);
		sleep(5);
	}	
}

void * readRegistrationInfo()
{
	int msglen;
	char readmsg[2000];
	
	gotoLoop:
	while(msglen = recv(gSockFd, readmsg, 2000, 0) <= 0)
	{
		continue;
	}
	
	printf("\nMessage received >>> %s\n", readmsg);
	
	if(strstr(readmsg, "registered:") == NULL)
	{
		memset(readmsg, 0, sizeof readmsg);
		goto gotoLoop;
	}
	
	while(id < 1 && reg < 1)
	{
		sleep(1);		
	}
	
	//let registerIds be filled with some value from other sensors connecting this Gateway
	sleep(4);
	
	strcpy(registrationInfo, readmsg);
	memset(readmsg, 0, sizeof readmsg);
	
	char *foo = &registerIds[0];
	foo += 11; // remove first 12 characters
	strcpy(registerIds, foo);
	
	printf("\nregisterIds === %s\n", registerIds);
	
	strcat(registrationInfo, ":");
	strcat(registrationInfo, registerIds);
	
	printf("registrationInfo >>> %s\n", registrationInfo);
	
	write(gSockFd, registrationInfo, strlen(registrationInfo), 0);
	writeToFile(registrationInfo);
	writeToFile("\n");
	
	regAtSecGateway = 1;
}

void * threadGatewayReceive()
{
	char readmsg[2000];
	int msglen;
	char sendToGMsg[2000];
	char temp[2000];
	
	TGR:
	while(msglen = recv(gSockFd, readmsg, 2000, 0) > 0)
	{
		//strcpy(readBackmsg, readmsg);
		//sprintf(fileMsg, "Message received from %s = %s\n", cs.name, readBackmsg);
		//sprintf(sendToGMsg, );
		/*
		pthread_mutex_lock(&mutex);
		
		//send the same message to other Gateway
		writeToOtherGateway(sendToGMsg);
		memset(sendToGMsg, 0, sizeof(sendToGMsg));
		*/
		
		strcat(readmsg, "\n");
		writeToFile(readmsg);	
			
		if(primary)
		{
			printf("In Primary . . . . .");
			puts(readmsg);
			//strcpy(globalWrite, sendToGMsg);
			//checkForDoneRequest:
			//if(DONE)		
			//{
			if(strstr(readmsg,"Vote_Commit") != NULL)
			{ 
				printf("\nInside Vote commit");
				
				//strtok(readmsg, ":");
				//globalWrite = strtok(NULL, "\n");
				strcpy(temp, globalWrite);
				sprintf(globalWrite, "T%d:%s",++Count, temp);
				write(backsockfd, globalWrite, strlen(globalWrite));
				memset(globalWrite, 0, sizeof(globalWrite));
					
				writeToOtherGateway("Commit\0");
			
				memset(readmsg, 0, sizeof(readmsg));
			}
			
			else if(strstr(readmsg,"Done") != NULL)
			{
				printf("\nInside Done");
				//stop bufferring of messages and send the buffered ones.
				VOTE_COMMIT = 1;
				REQUEST_TO_PREPARE = 1;
				DONE = 1;
			
				memset(readmsg, 0, sizeof(readmsg));
			}
		
			else
			{
				printf("\nInside primary else");
				//DONE = 0;
				//sprintf(globalGWrite, "%d;%s;%s;%d;%s", cs.idNum, cs.name, cs.ip, cs.port, BUFFER);
				sprintf(sendToGMsg, "Request_To_Prepare:%s", readmsg);
				//puts(sendToGMsg);
				//memset(BUFFER, 0, sizeof(BUFFER));
				
				//send message to secondary Gateway
				writeToOtherGateway(sendToGMsg);
				strtok(sendToGMsg, ":");
				//strcat(globalWrite, "\n");
				strcat(globalWrite, strtok(NULL, "\n"));
				memset(readmsg, 0, sizeof(readmsg));
				//memset(sendToGMsg, 0, sizeof(sendToGMsg));
			}
		}
		else
		{
			printf("Other gateway . . .");
			puts(readmsg);
			
			if(strstr(readmsg,"Request_To_Prepare") != NULL)	
			{
				printf("\nInside Req to prepare");
							
				strtok(readmsg, ":");
				strcpy(globalWrite, strtok(NULL, "\n"));
			
				REQUEST_TO_PREPARE = 1;
				VOTE_COMMIT = 1;
				
				//sprintf(globalSecGWrite, "%d;%s;%s;%d;%s\n", cs.idNum, cs.name, cs.ip, cs.port, BUFFER);
				//sprintf(sendToGMsg, "Vote_Commit:%s\n", globalWrite);
				writeToOtherGateway("Vote_Commit\0");
				//memset(BUFFER, 0, sizeof(BUFFER));
				memset(sendToGMsg, 0, sizeof(sendToGMsg));
			}
			
			else if(strstr(readmsg,"Commit") != NULL)
			{
				printf("\nInside commit");
			
				//append and write message
				strcpy(temp, globalWrite);
				sprintf(globalWrite, "T%d:%s",++Count, temp);
				write(backsockfd, globalWrite, strlen(globalWrite));
				
				memset(globalWrite, 0, sizeof(globalWrite)); //changed!!
			
				writeToOtherGateway("Done");
			}
		
			else
			{
				printf("\nInside other gateway else");
			//	writeToOtherGateway(sendToGMsg);
			}
			
			memset(readmsg, 0, sizeof(readmsg));
			memset(sendToGMsg, 0, sizeof(sendToGMsg));
		}
	}
}
