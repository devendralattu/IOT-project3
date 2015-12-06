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

int backsockfd, securitySockId, pGatewaysockfd, primary, oGatewaysockfd;
int vectorClock[4] = {0};
FILE *fpOutputFile;
char *file1, *file3;	

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int startHeartBeatMsg = 0;
//-----------------------------------------
void writeToFile(char * fileData)
{
	fpOutputFile = fopen(file3, "a+");
	fprintf(fpOutputFile, "%s", fileData);
	fclose(fpOutputFile);
}

void writeToGateway(char *fileMsg)
{
	if(primary)
		write(oGatewaysockfd, fileMsg, strlen(fileMsg));
	else
		write(pGatewaysockfd, fileMsg, strlen(fileMsg));
	printf("\nWriting to other gateway\n");	
}
//Check if user or thief has entered or left
void checkUser()
{
    char fileMsg[100];
    int msg;
    msg = 0;

    //User has left home
    if(//(prevState[2] == 0 && currState[2] == 1) //Door 
		//&& prevState[1] == 1 && currState[1] == 0 //Motion
		(prevState[0] == 1 && currState[0] == 0))//Key
	{
	    sprintf(fileMsg,"%d.) Security System is ON\n", msg);
		write(backsockfd, fileMsg, strlen(fileMsg));
		write(securitySockId, fileMsg, strlen(fileMsg));
	}
   
	//User has come home
	if( //currState[1] == 1 //motion 
	prevState[0] == 0 && currState[0] == 1)//Key
	{
	    sprintf(fileMsg,"%d.) Security System is OFF\n", msg);
		write(backsockfd, fileMsg, strlen(fileMsg));
		write(securitySockId, fileMsg, strlen(fileMsg));
		writeToGateway(fileMsg);
	} 
}

void checkThief()
{
    char fileMsg[100];
    int msg;
    msg = 1;
   
	//Thief has come home
	if(((prevState[2] == 0 && currState[2] == 1)//Door
	|| (prevState[1] == 0 && currState[1] == 1))//Motion
	&& (prevState[0] == 0 && currState[0] == 0))//Key
	{
		sprintf(fileMsg,"%d.) Burglar ALARM!!!\n ", msg);
		write(backsockfd, fileMsg, strlen(fileMsg));
	}  
}
//--------------------------------------------------------------------------


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
	pthread_t thread1;
	
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
		if((pGatewaysockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("\nCould not create other Gateway socket.. Please run primary Gateway first\n");
		}

		//Initialise the pGateway socket
		pGateway.sin_family = AF_INET;
		pGateway.sin_addr.s_addr = inet_addr(pGatewayIP);
		pGateway.sin_port = htons( pGatewayPort );

		runagain:
		//Connect gateway to pGatewaysockfd
		if((connect(pGatewaysockfd, (struct sockaddr *) &pGateway, sizeof(pGateway))) < 0)
		{
			perror("\nUnable to connect Gateway to other Gateway..Waiting for connection from primary Gateway..Trying again..\n");
			sleep(2);
			goto runagain;			
			//return;
		}
	
		puts("gateway: Connected to other Gateway");
	
		write(pGatewaysockfd, configInfo, strlen(configInfo), 0);
		puts(configInfo);
		
		//sleep(2);
		//create a new thread for sending messages
		//pthread_create(&thread1, NULL, &sendingGatewayThread, NULL);
		pthread_create(&thread1, NULL, &readRegistrationInfo, NULL);
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
	
	//store oGatewaysockfd value
	if(strstr(csArr[itemId].name, "ateway") != NULL)
	{
		oGatewaysockfd = csArr[itemId].sock;
		printf("\ncsArr[itemId].name, \"ateway\" >>> oGatewaysockfd = %d\n", oGatewaysockfd);
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
				write(oGatewaysockfd, registerIds, strlen(registerIds));
				while(msglen = recv(oGatewaysockfd, registrationInfo, 2000, 0) <= 0)
				{
					continue;
				}
				printf("\nInside if part >> registrationInfo = %s\n", registrationInfo);
				setReg2 = 1;
			}
			else
			{
				//wait till we receive and process registerIds from primary gateway and send other gateway's reg info back to it.
				//while(!((registrationInfo != NULL) && (registrationInfo[0] == '\0')))
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
		}	
	}
	else if(strstr(csArr[itemId].name, "ateway") != NULL)
	{
		reg++;
		if(startHeartBeatMsg)
		{
			char readmsgGateway[2000];
			printf("\nMessage received from Gateway\n");
		
			while(msglen = recv(oGatewaysockfd, readmsgGateway, 2000, 0) > 0 )
			{
				printf("Message received = %s\n", readmsgGateway);
				writeToFile(readmsgGateway);
			
				write(oGatewaysockfd, heartBeat, strlen(heartBeat));
				memset(readmsg, 0, sizeof(readmsgGateway));
			}
		}	
	}																																																			
}

void* threadReadFun(void *cs1)
{
	int msglen, i, vectVal[4] = {0};
	char readmsg[2000];
	char readBackmsg[2000];
	char sendmsg[2000];
	char vectInfo[50];
	char fileMsg[100];
	int switcher = 0;
	 
	clientStruct1 cs = *(clientStruct1*)(cs1);
	while(msglen = recv(cs.sock, readmsg, 2000, 0) > 0 )
	{
		printf("Message received from %s = %s\n", cs.name, readmsg);
		strcpy(readBackmsg, readmsg);
		sprintf(fileMsg, "Message received from %s = %s\n", cs.name, readBackmsg);
		
		pthread_mutex_lock(&mutex);
		writeToFile(fileMsg);		
			
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
		
		//------------------------------Logic for Security System------------------------------

		if(strcmp(cs.name, "keychain") == 0)
				switcher = 0;
		else if (strcmp(cs.name, "motionsensor") == 0)
				switcher = 1;
		else if (strcmp(cs.name, "door") == 0)
				switcher = 2;
			       

        switch(switcher)
        {
            case 0: //keychain
                prevState[0] = currState[0];
                if(strcmp(vectInfo, "True") == 0)
                {
                    currState[0] = 1;
                }    
                else
                    currState[0] = 0;

				checkUser();
				checkThief();
                break;
           
            case 1: //motionsensor
                prevState[1] = currState[1];
                if(strcmp(vectInfo, "True") == 0)
                    currState[1] = 1;
                else
                    currState[1] = 0;   
                checkThief();
                break;
           
            case 2: //door
                prevState[2] = currState[2];
                if(strcmp(vectInfo, "Open") == 0)
                    currState[2] = 1;
                else   
                    currState[2] = 0;
                break;
           
            default:
                printf("\n In default case");
                break;
        }  
		
		sprintf(sendmsg, "%d;%s;%s;%d;%s", cs.idNum, cs.name, cs.ip, cs.port, readBackmsg);
		write(backsockfd, sendmsg, strlen(sendmsg));	
		writeToGateway(sendmsg);
					
		pthread_mutex_unlock(&mutex);		
		printf("Message sent to backend = %s\n",sendmsg);
		
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
	printf("\n pGatewaysockfd = %d\n", pGatewaysockfd);
	printf("inside while of sendingGatewayThread\n");
	while(1)
	{
		puts(heartBeat);
		write(pGatewaysockfd, heartBeat, strlen(heartBeat), 0);

		memset(readmsg, 0, sizeof readmsg);
		while(msglen = recv(pGatewaysockfd, readmsg, 2000, 0) <= 0)
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
	char * line;
	
	gotoLoop:
	while(msglen = recv(pGatewaysockfd, readmsg, 2000, 0) <= 0)
	{
		continue;
	}
	
	printf("\nMessage received >>> %s\n", readmsg);
	
	if(strstr(readmsg, "register:") == NULL)
	{
		memset(readmsg, 0, sizeof readmsg);
		goto gotoLoop;
	}
	
	strcpy(registrationInfo, readmsg);
	memset(readmsg, 0, sizeof readmsg);
	
	line = strtok(registerIds, ":"); // removed 'registered'
	
	printf("\nline === %s && registerIds === %s\n", line, registerIds);
	
	strcat(registrationInfo, ":");
	strcat(registrationInfo, registerIds);
	
	write(pGatewaysockfd, registrationInfo, strlen(registrationInfo), 0);
	writeToFile(registrationInfo);
	
	regAtSecGateway = 1;
}
