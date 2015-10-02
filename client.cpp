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
    printf("Starting client on Port: %d \n", port);
    this->m_nListenPort = port;
    this->m_bisRegistered = false;
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
        for(int i=0; i<=m_nMaxFd; i++) {
            if(FD_ISSET(i, &m_readSet)) {
                if(i == STDIN) {
                    commandShell();
                }
                else if(i == m_nListenSd) {
                    // new connection
                	newConnectionHandler();
                }
                else {
                    // handle data from existing connections
                	char recvBuff[1024];
                	int nbytes;
                	if( (nbytes = recv(i, recvBuff, sizeof(recvBuff), 0)) > 0) {
                		recvBuff[nbytes] = 0;
                		printf("%s", recvBuff);
                	}
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
	char **commandTokens = getAndParseCommandLine(nArgs);
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
	int sockfd;

	// populate server address structure and also check if it is a valid ip address
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(srvPort);
	if(inet_pton(AF_INET, m_srvIpAddress, &srvAddr.sin_addr) != 1) {
		perror("[command_register] inet_pton");
		displayUsage();
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if( connect(sockfd, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}
	m_bisRegistered = true;
	FD_SET(sockfd, &m_masterSet);
	if(sockfd > m_nMaxFd)
		m_nMaxFd = sockfd;

	char recvBuff[1024];
	// send the listening port of the client so that server will update the list
	snprintf(recvBuff, sizeof(recvBuff), "%d\r\n", m_nListenPort);
	if(send(sockfd, recvBuff, strlen(recvBuff), 0) != strlen(recvBuff)) {
		perror("send");
		exit(EXIT_FAILURE);
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
	    printf("Server Started listening on port: %d \n", m_nListenPort);
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

/*****************************************************************************
 *                      Server utility functions                             *
 * **************************************************************************/

char** Client::getAndParseCommandLine(int &nArgs) {
	// read the command from stdin
	char *commandLine = NULL;
	size_t size;
	ssize_t linelen;
	linelen = getline(&commandLine, &size, stdin);
	commandLine[linelen-1] = '\0';

	char **tokens = (char **) malloc(4*sizeof(char *));
	char *tok = strtok(commandLine, " \0");
	nArgs = 0;
	while(tok!=NULL && nArgs < 4) {
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
    char host[INET_ADDRSTRLEN];

    // ifAddr contains a list of all local interfaces
    getifaddrs(&ifAddr);
    while(ifAddr != NULL) {
        // ignore the nodes which do not contain struct sockaddr
        if(ifAddr->ifa_addr == NULL)
            continue;
        // We need to consider only ipv4 address
        if(ifAddr->ifa_addr->sa_family == AF_INET) {
            // get the ip address of this interface and update if it is not a local or private ip address
            getnameinfo(ifAddr->ifa_addr, sizeof(struct sockaddr_in), host, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            if(strcmp(host,"127.0.0.1") != 0 && strstr(host, "192.168") == NULL) 
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
