/*
 * ==========================================================================
 *      
 *      Filename: server.cpp
 *
 *      Description: 
 * 
 *      Date: 2015-09-22 22:37
 *
 *      Author: Srikanth Malipatel
 *
 * ==========================================================================
 */

#include "server.h"

/*
 * Function:    Constructor
 */
Server::Server(int port) {
	cout << "Running program as server" << endl;
    this->m_nListenPort = port;
    this->m_nLatestIndex = 0;

    // initalize nodeList and its tracker
	for(int i=0; i<10; i++)
	{
		m_nodeList[i].state=INACTIVE;
		m_nodeList[i].isServer=false;
	}
    updateIpAddress();
    printf("server IP: %s \n", m_ipAddress);

    // initalize server specific parameters
    FD_ZERO(&m_masterSet);
    FD_ZERO(&m_readSet);
    memset(&m_srvAddr, 0, sizeof(m_srvAddr));

    // starting event handler
    eventHandler();
}

/*
 * Function:    Destructor
 */
Server::~Server() {

}

/*
 * Function:    eventHandler()
 * Parameters:  None
 * Returns:     None
 * Description: This function creates a listening socket and accepts new incoming connections, apart from it it also reads from stdin such that server works as a unix shell and also handles incoming connections
 */
void Server::eventHandler() {
    char buffer[1024];
    startListenServer();
    
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
                    // data from stdin, process approriate commands
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strlen(buffer)-1] = '\0';
                    if(strlen(buffer) != 0)
                    	commandShell(buffer);
                }
                else if(i == m_nListenSd) {
                    // new connection
                	newConnectionHandler();
                }
                else {
                    // handle data from existing client
                }

            }
        }
    }
}

/*
 * Function:    commandShell(char *command)
 * Parameters:  None
 * Returns:     None
 * Description: This functions behaves like shell answering all user commands
 */
void Server::commandShell(char *command) {
    CommandID command_id = getCommandID(command); 
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
            // handle LIST command
            command_list();
            break;
        default:
            printf("Please enter a valid command \n");
            printf("Type Help - to display supported commands \n");
            break;
        }
}

/*
 * Function:    Command_help()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the commands avaliable to user
 */
void Server::command_help() {
    printf("User command options \n");
    printf("\tCREATOR - Displays creators full name, UBIT name and UB email address\n");
    printf("\tDIPSLAY - Displays the IP address and listening port of this process\n");
    printf("\tLIST - Displays a list of clients registered on this server\n");
}

/*
 * Function:    Command_creator()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the name, ubitname and mail address of the creator
 */
void Server::command_creator() {
    m_name = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubitName = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubEmail = (char* ) malloc(MAX_CREATOR_LEN);
    strncpy(m_name, "Srikanth Reddy Malipatel\0", MAX_CREATOR_LEN);
    strncpy(m_ubitName, "smalipat\0", MAX_CREATOR_LEN);
    strncpy(m_ubEmail, "smalipat@buffalo.edu\0", MAX_CREATOR_LEN);
    printf("NAME:%s\tUBITname:%s\tUBEmail:%s \n",m_name, m_ubitName, m_ubEmail);
}

/*
 * Function:    Command_display()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the IP address and the listening port of this process
 */
void Server::command_display() {
    printf("Machine's IP Address is: %s and Listening port of server is: %d \n", m_ipAddress, m_nListenPort);
}

/*
 * Function:    Command_list()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays a numbered list of all the connections this process is part of. This includes connections initated by this process
 *              and by other process.
 *              Format: <id>    <HostName>      <IP Address>        <Port Number>
 */
void Server::command_list() {
    // Complete this
}

void Server::startListenServer() {
	// add stdin to master set
	    FD_SET(STDIN, &m_masterSet);

	    // populate server address structure
	    m_srvAddr.sin_family = AF_INET;
	    m_srvAddr.sin_port = htons(m_nListenPort);
	    if(inet_pton(AF_INET, m_ipAddress, &m_srvAddr.sin_addr) != 1) {
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
	    if(bind(m_nListenSd, (struct sockaddr *)&m_srvAddr, sizeof(m_srvAddr)) < 0) {
	        perror("bind failed");
	        exit(EXIT_FAILURE);
	    }
	    printf("Server is now listening on port: %d \n", m_nListenPort);
	    // maximum of 5 pending connections for m_nListenSd socket
	    if(listen(m_nListenSd, 5)) {
	        perror("Listen failed");
	        exit(EXIT_FAILURE);
	    }
	    // add listening socket to master set and update m_nMaxFd
	    FD_SET(m_nListenSd, &m_masterSet);
	    m_nMaxFd = m_nListenSd;
}

void Server::newConnectionHandler() {
	int newConnSd;
	char remoteIP[32];
	struct sockaddr_in remoteaddr;
	memset(&remoteaddr, 0, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	int addrlen = sizeof(remoteaddr);
	if((newConnSd = accept(m_nListenSd, (struct sockaddr*)&remoteaddr, (socklen_t*)&addrlen)) == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("new connection from %s on ""socket %d\n", inet_ntop(AF_INET, &remoteaddr.sin_addr, remoteIP, 32), newConnSd);

	// process Register message from client
	char buffer[64];
	int nbytes;
	if((nbytes = recv(newConnSd, buffer, sizeof(buffer), 0)) > 0)
	{
		buffer[nbytes] = 0;
		//printf("%s", buffer);
	}
	// register message format: REGISTER PORT
	if(strstr(buffer, "REGISTER") != NULL) {
		strtok(buffer," ");
		char *port=strtok(NULL," ");
		// find the first unused node and update the client details
		for(int i=0; i<10; i++) {
			if(m_nodeList[i].state == INACTIVE) {
				m_nodeList[i].state=ACTIVE;
				m_nodeList[i].listenPort =strtol(port, NULL, 10);
				m_nodeList[i].sockFd = newConnSd;
				strcpy(m_nodeList[i].ip_addr,remoteIP);
				getHostName(m_nodeList[i].ip_addr, m_nodeList[i].hostName);
				m_nLatestIndex = i;
				break;
			}
		}

		// send REGISTER OK message to the client
		memset(buffer, 0 ,sizeof(buffer));
		strcat(buffer, "REGISTER OK");
		cout << "Sending Message " << buffer << " to client " << remoteIP << endl;

		int len = strlen(buffer);
		if(sendall(newConnSd, buffer, &len) == -1) {
			cout << "Message sending failed. Please retry" << endl;
			return;
		}

		// update all the clients with the new list
		updateNodesinList();
	}

	// add the client to masterSet such that server can read messages from clients, if any.
	FD_SET(newConnSd, &m_masterSet);
	if (newConnSd > m_nMaxFd) {
		m_nMaxFd = newConnSd;
	}
}

/*****************************************************************************
 *                      Server utility functions                             *
 * **************************************************************************/

/*
 * Function:    getCommandID(char[] comnd)
 * Parameters:  comnd: command to be executed
 * Returns:     if string is found then returns ComandID else -1
 * Description: This functions takes a string and returns appropriate CommandID
 */
CommandID Server::getCommandID(char comnd[]) {
    if(strcasecmp(comnd, "HELP") == 0)
        return COMMAND_HELP;
    else if(strcasecmp(comnd, "CREATOR") == 0)
        return COMMAND_CREATOR;
    else if(strcasecmp(comnd, "DISPLAY") == 0)
        return COMMAND_DISPLAY;
    else if(strcasecmp(comnd, "LIST") == 0)
        return COMMAND_LIST;
    else
        return COMMAND_NONE;
}

/*
 * Function:    updateIpAddress()
 * Parameters:  None
 * Returns:     None
 * Description: This function updates m_ipAddress buffer with the public interface ip
 */
void Server::updateIpAddress() {
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
    //printf("ip: %s", m_ipAddress);
    return;
}

/*void Server::addNodetoList(int sockfd, char *ipAddr, int port) {
	m_cList[m_nClientCount].id = m_nClientCount+1;
	m_cList[m_nClientCount].sockFd = sockfd;
	m_cList[m_nClientCount].port = port;
	strcpy(m_cList[m_nClientCount].ipAddress, ipAddr);

	struct in_addr ipv4addr;
	if(!inet_aton(ipAddr, &ipv4addr)) {
		perror("inet_aton");
		exit(EXIT_FAILURE);
	}
	struct hostent *host;
	if((host = gethostbyaddr((const void*)&ipv4addr, sizeof ipv4addr, AF_INET)) == NULL) {
		perror("gethostbyaddr");
		exit(EXIT_FAILURE);
	}
	strcpy(m_cList[m_nClientCount].hostName, host->h_name);
	printf("Connection Successful from %s[%s] on port %d\n", m_cList[m_nClientCount].hostName, m_cList[m_nClientCount].ipAddress, m_cList[m_nClientCount].port);
	m_nClientCount++;
}*/

void Server::updateNodesinList() {
	cout<<m_nodeList[m_nLatestIndex].ip_addr<<":"<<m_nodeList[m_nLatestIndex].listenPort<<" Registered Successfully. " << endl;
	char msg[512]={0};
	// constructing a message in format: UPDATE CL1.ip CL1.host CL1.port|CL2.ip CL2.host CL2.port|....
	for(int j=0; j<10; j++) {
		memset(msg, 0, sizeof(msg));
		strcat(msg,"UPDATE ");

		// sending data for all used nodes excluding details about it self
		if(m_nodeList[j].state==ACTIVE) {
			for(int i=0; i<10; i++)
			{
				if(m_nodeList[i].state==ACTIVE && i!=j)
				{
					char port[4];
					sprintf(port,"%d",m_nodeList[i].listenPort);
					strcat(msg,m_nodeList[i].ip_addr);
					strcat(msg," ");
					strcat(msg,m_nodeList[i].hostName);
					strcat(msg," ");
					strcat(msg,port);
					strcat(msg,"|");
				}
			}
			int length=sizeof(msg);
			if(length>0) {
				cout << "sending message to " << m_nodeList[j].sockFd << " " << msg <<endl;
				if (sendall(m_nodeList[j].sockFd, msg, &length) == -1)
				{
					cerr<<"Error Sending List Updates"<<endl;
				}
				else
				{
					if(length<sizeof(msg))
					{
						cout << "Only " << length << "bytes sent but msg size " << sizeof(msg) << endl;
					}
				}
			}
		}
	}
	cout << "Updated List Sent to All Clients."<<endl;
}
