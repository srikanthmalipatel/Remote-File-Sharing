/*
 * ==========================================================================
 *      
 *      Filename: client.cpp
 *
 *      Description: 
 * 
 *      Date: 2015-09-22 22:37
 *
 *      Author: Srikanth Malipatel
 *
 * ==========================================================================
 */

#include "client.h"

/*
 * Function:    Constructor
 */
Client::Client(int port) {
	cout << "Running program as Client" << endl;
    // update instance variables
    this->m_nListenPort = port;
    this->m_bisRegistered = false;

    // initalize nodeList and its tracker
    this->m_nLatestIndex = 0;
    for(int i=0; i<10; i++)
	{
    	m_nodeList[i].isUsed=false;
    	m_nodeList[i].isServer=false;
	}
    // move this to common.cpp
    updateIpAddress();

    FD_ZERO(&m_masterSet);
	FD_ZERO(&m_readSet);
	memset(&m_cliListenAddr, 0, sizeof(m_cliListenAddr));

	// starting event handler
	eventHandler();
}

/*
 * Function:    Destructor
 */
Client::~Client() {

}

void Client::eventHandler() {
    char buffer[1024];
    startListenClient();

    for(;;) {
        // copy master set to read set
        m_readSet = m_masterSet;
        // start polling on read set, which is blocking indefinitely
        if(select(m_nMaxFd+1, &m_readSet, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // check if there is data on any of the sockets
        int bytesRead;
        char recvBuff[1024];
        for(int i=0; i<=m_nMaxFd; i++) {
            if(FD_ISSET(i, &m_readSet)) {
                if(i == STDIN) {
                    commandShell();
                }
                else if(i == m_nListenSd) {
                    // new connection
                	newConnectionHandler();
                }
                else if(i == m_nServerSd) {
                	// server sends client list updates when ever a new client registers/terminates/quits
                	if( (bytesRead = recv(i, recvBuff, sizeof(recvBuff), 0)) > 0)
                	{
                		printf("Got Updated Server List \n");
                		recvBuff[strlen(recvBuff)]='\0';
                		strcpy(m_srvList,recvBuff);
                		displayServerList();
                		cout << endl;
                		memset(recvBuff, 0, 1024);
                	}
                }
                else {
                    // handle data from connected clients

                }

            }
        }
    }
}

/*
 * Function:    commandHandler()
 * Parameters:  None
 * Returns:     None
 * Description: This functions behaves like shell answering all user commands
 */
void Client::commandShell() {
	int nArgs;
	char *commandLine = NULL;
	size_t size;
	// read the command from stdin
	ssize_t linelen = getline(&commandLine, &size, stdin);
	commandLine[linelen-1] = '\0';
	char **commandTokens = parseLine(commandLine, nArgs, " \0");
	if(nArgs == 0)
		return;
	//printf("command[0] %s \n", m_commandTokens[0]);
	CommandID command_id = getCommandID(commandTokens[0]);
	switch(command_id) {
		case COMMAND_HELP:
			// handle help command
			command_help();
			break;
		case COMMAND_CREATOR:
			// handle CREATOR command
			command_creator();
			break;
		case COMMAND_DISPLAY:
			// handle DISPLAY command
			command_display();
			break;
		case COMMAND_LIST:
			command_list();
			break;
		case COMMAND_REGISTER:
			// if the server is not yet registered and if the number of arguments is same as expected.
			if(!m_bisRegistered && nArgs == 3)
				command_register(commandTokens[1], commandTokens[2]);
			else
				displayUsage();
			break;
		default:
			displayUsage();
			break;
	}
	for(int i=0; i<nArgs; i++)
		free(commandTokens[i]);
}

/*
 * Function:    Command_help()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the commands avaliable to user
 */
void Client::command_help() {
    printf("User command options \n");
    printf("\tCREATOR - Displays creators full name, UBIT name and UB email address\n");
    printf("\tDIPSLAY - Displays the IP address and listening port of this process\n");
    printf("\tLIST - Displays a list of clients registered on this server\n");
    printf("\tREGISTER <server IP> <Port> - Registers with the server\n");
}

/*
 * Function:    Command_creator()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the name, ubitname and mail address of the creator
 */
void Client::command_creator() {
    m_name = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubitName = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubEmail = (char* ) malloc(MAX_CREATOR_LEN);
    strcpy(m_name, "Srikanth Reddy Malipatel\0");
    strcpy(m_ubitName, "smalipat\0");
    strcpy(m_ubEmail, "srmalipat@buffalo.edu\0");
    printf("Creator:%s\tUBITname:%s\tUBEmail:%s \n",m_name, m_ubitName, m_ubEmail);
}

/*
 * Function:    Command_display()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the IP address and the listening port of this process
 */
void Client::command_display() {
    printf("Machine's IP Address is: %s and Listening port of server is: %d \n", m_ipAddress, m_nListenPort);
}

/*
 * Function:    Command_Register()
 * Parameters:  IP address of the client and Listening port number
 * Returns:     1 if successful else 0
 * Description: If the client is not yet registered to the server then this function establishes a TCP connection with the server
 */
int Client::command_register(char *ipAddr, char *port) {
	struct sockaddr_in srvAddr;
	int srvPort = atoi(port);
	strcpy(m_srvIpAddress, ipAddr);

	// populate server address structure and also check if it is a valid ip address
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(srvPort);
	if(inet_pton(AF_INET, m_srvIpAddress, &srvAddr.sin_addr) != 1) {
		perror("[command_register] inet_pton");
		displayUsage();
	}

	if((m_nServerSd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if( connect(m_nServerSd, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}

	// construct register message
	char msg[64] = {0};
	strcat(msg, "REGISTER ");
	char tmpport[4];
	sprintf(tmpport, "%d", m_nListenPort);
	strcat(msg, tmpport);
	cout << "sending message " << msg << " to server" << endl;

	// send register message to server
	int len = strlen(msg);
	if(sendall(m_nServerSd, msg, &len) == -1) {
		cout << "Message sending failed. Please retry" << endl;
	}
	else {
		m_bisRegistered = true;
		cout << "REGISTER message Sent. Waiting for Updated Client List from server" << endl;

		// update server details with the first unused nodeList
		for(int i=0; i<10; i++) {
			if(m_nodeList[i].isUsed == false) {
				m_nodeList[i].sockFd = m_nServerSd;
				m_nodeList[i].isUsed = true;
				m_nodeList[i].isServer = true;
				m_nodeList[i].listenPort = srvPort;
				strcpy(m_nodeList[i].ip_addr, m_srvIpAddress);
				getHostName(m_nodeList[i].ip_addr, m_nodeList[i].hostName);
				m_nLatestIndex = i;
				break;
			}
		}
		cout << "Registered with server " << m_nodeList[m_nLatestIndex].ip_addr << ":" << m_nodeList[m_nLatestIndex].listenPort << " Updated nodeList with server details" << endl;

		// adding this socket to masterset so that client can update nodeList when server sends messages on new client registration or termination/exit.
		FD_SET(m_nServerSd, &m_masterSet);
		if(m_nServerSd > m_nMaxFd)
			m_nMaxFd = m_nServerSd;
	}

	return 0;
}

/*
 * Function:    Command_list()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays a numbered list of all the connections this process is part of. This includes connections initated by this process
 *              and by other process.
 *              Format: <id>    <HostName>      <IP Address>        <Port Number>
 */
void Client::command_list() {
    // Complete this
}

/*
 * Function:    Command_terminate(int connectionID)
 * Parameters:  int connectionId
 * Returns:     None
 * Description: This functions will terminate a connection already in registered_list
 */
void Client::command_terminate(int connectionID) {
    // Complete this
}

/*
 * Function:    Command_quit()
 * Parameters:  None
 * Returns:     None
 * Description: This functions closes all connections and terminates this process
 */
void Client::command_quit() {
    // Complete this
}

void Client::startListenClient() {
	// add stdin to master set
	    FD_SET(STDIN, &m_masterSet);

	    // populate server address structure
	    m_cliListenAddr.sin_family = AF_INET;
	    m_cliListenAddr.sin_port = htons(m_nListenPort);
	    printf("m_ipAddress %s\n", m_ipAddress);
	    if(inet_pton(AF_INET, m_ipAddress, &m_cliListenAddr.sin_addr) != 1) {
	        perror("inet_pton failure");
	        exit(EXIT_FAILURE);
	    }
	    // create a TCP listening socket
	    if((m_nListenSd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
	        perror("Listening socket creation failed");
	        exit(EXIT_FAILURE);
	    }
	    // reuse the socket in case of crash
	    int optval = 1;
	    if(setsockopt(m_nListenSd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
	        perror("setsockopt failed");
	        exit(EXIT_FAILURE);
	    }
	    // bind m_srvAddr to the listening socket
	    if(bind(m_nListenSd, (struct sockaddr *)&m_cliListenAddr, sizeof(m_cliListenAddr)) < 0) {
	        perror("bind failed");
	        exit(EXIT_FAILURE);
	    }
	    printf("Client is now listening on port: %d \n", m_nListenPort);
	    // maximum of 5 pending connections for m_nListenSd socket
	    if(listen(m_nListenSd, 3)) {
	        perror("Listen failed");
	        exit(EXIT_FAILURE);
	    }
	    // add listening socket to master set and update m_nMaxFd
	    FD_SET(m_nListenSd, &m_masterSet);
	    m_nMaxFd = m_nListenSd;
}

void Client::newConnectionHandler() {
	// yet to implement. complete this while implementing connect function
}


// this should also update local copy when ever server sends update
void Client::displayServerList() {
	for(int i=7;i<strlen(m_srvList);i++)
	{
		if(m_srvList[i]==' ')
		{
			cout<<"         ";
		}
		else if(m_srvList[i]=='|')
		{
			cout<<endl;
		}
		else
		{
			cout<<m_srvList[i];
		}
	}
}

/*****************************************************************************
 *                      Server utility functions                             *
 * **************************************************************************/

char** Client::parseLine(char *line, int &nArgs, const char *delim) {
	char **tokens = (char **) malloc(10*sizeof(char *));
	char *tok = strtok(line, delim);
	nArgs = 0;
	while(tok!=NULL) {
		//printf("tok %s \n", tok);
		tokens[nArgs] = (char *) malloc(sizeof(strlen(tok)*sizeof(char)));
		strcpy(tokens[nArgs], tok);
		//printf("tokens[%d] = %s \n", nArgs, tokens[nArgs]);
		tok = strtok(NULL, " \0");
		nArgs++;
	}
	return tokens;
}

/*
 * Function:    getCommandID(char[] comnd)
 * Parameters:  comnd: command to be executed
 * Returns:     if string is found then returns ComandID else -1
 * Description: This functions takes a string and returns appropriate CommandID
 */
CommandID Client::getCommandID(char comnd[]) {
    if(strcasecmp(comnd, "HELP") == 0)
        return COMMAND_HELP;
    else if(strcasecmp(comnd, "CREATOR") == 0)
        return COMMAND_CREATOR;
    else if(strcasecmp(comnd, "DISPLAY") == 0)
        return COMMAND_DISPLAY;
    else if(strcasecmp(comnd, "LIST") == 0)
        return COMMAND_LIST;
    else if(strcasecmp(comnd, "REGISTER") == 0)
    	return COMMAND_REGISTER;
    else
        return COMMAND_NONE;
}

/*
 * Function:    updateIpAddress()
 * Parameters:  None
 * Returns:     None
 * Description: This function updates m_ipAddress buffer with the public interface ip
 */
void Client::updateIpAddress() {
    struct ifaddrs *ifAddr;
    char host[32];

    // ifAddr contains a list of all local interfaces
    getifaddrs(&ifAddr);
    while(ifAddr != NULL) {
        // ignore the nodes which do not contain struct sockaddr
        if(ifAddr->ifa_addr == NULL)
            continue;
        // We need to consider only ipv4 address
        if(ifAddr->ifa_addr->sa_family == AF_INET) {
            // get the ip address of this interface and update if it is not a local or private ip address
            getnameinfo(ifAddr->ifa_addr, sizeof(struct sockaddr_in), host, 32, NULL, 0, NI_NUMERICHOST);
            if(strcmp(host,"127.0.0.1") != 0)
                strcpy(m_ipAddress, host);
        }
        ifAddr = ifAddr->ifa_next;
    }
    return;
}

void Client::displayUsage() {
	printf("Please enter a valid command \n");
	printf("Type Help - to display supported commands \n");
}
