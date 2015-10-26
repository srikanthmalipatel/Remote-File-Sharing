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
    this->m_nConnCount = 0;
    this->m_bInSync = false;

    // initalize nodeList and its tracker
    this->m_nLatestIndex = 1;
    for(int i=0; i<10; i++)
	{
    	m_nodeList[i].id = i+1;
    	m_nodeList[i].state=INACTIVE;
    	m_nodeList[i].isServer=false;
	}
    // move this to common.cpp
    updateIpAddress();
    getHostName(m_ipAddress, m_hostName);

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
                if(i == 0) {
                    commandShell();
                }
                else if(i == m_nListenSd) {
                    // new connection
                		newConnectionHandler();
                }
                else if(i == m_nServerSd) {
                	// server sends client list updates when ever a new client registers/terminates/quits
                	if( (bytesRead = recv(i, recvBuff, sizeof(recvBuff), 0)) > 0) {
                		recvBuff[strlen(recvBuff)]='\0';
                		// registration successful
                		// FORMAT: REGISTER OK
						if(strstr(recvBuff, "REGISTER OK")) {
							cout << "Registration Successful. " << endl <<  " Updated List." << endl;
							m_nodeList[0].state = ACTIVE;
						}
						// ** UPDATE REQUIRED ** check for cases in which registration is unsuccessful
						if(strstr(recvBuff, "REGISTER FAIL")) {

						}
						// Received UPDATE message from server contaning list of
                		char *pos;
                		if((pos = strstr(recvBuff, "UPDATE")) != NULL) {
                			printf("SERVER UPDATES \n");
                			strcpy(m_srvList,pos);
                			displayUpdateList();
							cout << endl;
                		}
                		// Received SYNC message from server
                		if(strcmp(recvBuff,"SYNC") == 0) {
							//cout << "Message: " << recvBuff << " from Server " << endl;
							// send SYNC Write message to server
							char buff[1024] = {0};
							sprintf(buff, "SYNC WRITE");
							int len = sizeof buff;
							if(sendall(i, buff, &len) != 0) {
								cout << "Send Failed" << endl;
							}
						}
                		// Received SYNC OK from server
						if(strcmp(recvBuff,"SYNC OK") == 0) {
							//cout << "Message: " << recvBuff << " from Server " << endl;
							// start writing to all connected clients
							start_sync();
							// send SYNC FIN after file transfer to all connected peers is completed
							char buff[1024] = {0};
							sprintf(buff, "SYNC FIN");
							int len = sizeof buff;
							sendall(m_nServerSd, buff, &len);
						}
						// Recieved SYNC READ from server
						if(strstr(recvBuff,"SYNC READ") != NULL) {
							//cout << "Message: " << recvBuff << " from Server " << endl;

							/*strtok(recvBuff, " ");
							strtok(NULL, " ");
							// check if it is connected to the host which is writing files
							for(int j=0; j<10; j++) {
								if(strcmp(m_nodeList[j].hostName, strtok(NULL, " ")) == 0) {
									handle_get(m_nodeList[j].sockFd, );
								}
							}*/

						}
                	}
                	else {
						cout << "Server disconnected: " <<  m_nodeList[i].hostName << endl;
						for(int j=0; j<10; j++) {
							if(m_nodeList[j].sockFd == i)
								handle_terminate(j);
						}
					}
                	memset(recvBuff, 0, 1024);
                }
                else {
                    // handle data from connected clients
                	if( (bytesRead = recv(i, recvBuff, sizeof(recvBuff), 0)) > 0) {
                		recvBuff[strlen(recvBuff)]='\0';

                		// If it is a CONNECT OK message from the peer the update clientList and also increase connection count
                		if(strcmp(recvBuff, "CONNECT OK") == 0) {
                			for(int j=0; j<10; j++) {
                				if(m_nodeList[j].sockFd == i && m_nodeList[j].state == PROGRESS) {
                					cout << "Message: " << recvBuff << " from client " << m_nodeList[j].hostName << endl;
                					cout << "Connection Successful with peer " << m_nodeList[j].ip_addr << " " << m_nodeList[j].listenPort << endl << endl;
                					m_nodeList[j].state = ACTIVE;
                				}
                			}
                			m_nConnCount++;
                			memset(recvBuff, 0, sizeof(recvBuff));
                		}
                		// If a peer refuses connection
                		else if(strcmp(recvBuff, "CONNECT FAIL") == 0) {
                			cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			strtok(recvBuff, " ");
                			for(int j=0; j<10; j++) {
								if(m_nodeList[j].sockFd == i) {
									cout << "Connection Failed with peer " << m_nodeList[j].ip_addr << " " << m_nodeList[j].listenPort << endl;
									cout << "Reason: " << recvBuff+12 << endl;
									cout << endl;
									m_nodeList[j].state = INACTIVE;
									break;
								}
							}
                		}
                		// Process PUT request
                		// FORMAT: PUT <FILENAME> <FILESIZE>
                		else if(strstr(recvBuff, "PUT")) {
                			//cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			strtok(recvBuff, " ");
                			char *fileName = (char *) malloc(32);
							strcpy(fileName, strtok(NULL, " "));
							size_t fileSz = strtol(strtok(NULL, " "), NULL, 10);
                			handle_get(i, fileName, fileSz);
                		}
                		// Process GET request.
                		// FORMAT: GET FILE <FILENAME>
                		else if(strstr(recvBuff, "GET FILE")) {
                			//cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			strtok(recvBuff, " ");
                			strtok(NULL, " ");
							char *fileName = (char *) malloc(32);
							strcpy(fileName, strtok(NULL, " "));
							handle_put(i, fileName);
                		}
                		// Recieved SYNC start message from clients
                		else if(strcmp(recvBuff,"SYNC START") == 0) {
							//cout << "Message: " << recvBuff << " from server " << endl;
							handle_sync();
						}
                		// Recieved TERMINATE message from peer
                		else if(strcmp(recvBuff,"TERMINATE") == 0) {
                			//cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			// find the index associated with this socket id
                			for(int j=0; j<10; j++) {
                				if(m_nodeList[j].sockFd == i)
                					handle_terminate(j);
                			}
                			cout << "TERMINATED connection with client " << m_nodeList[i].hostName << " Successfully" << endl;
                		}
                	}
                	else {
                		//cout << "Client disconnected: " <<  m_nodeList[i].hostName << endl;
                		for(int j=0; j<10; j++) {
							if(m_nodeList[j].sockFd == i)
								handle_terminate(j);
						}
                	}
                	memset(recvBuff, 0, 1024);
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
	int nArgs, connId;
	char arg1[32], arg2[32], arg3[32];
	char *commandLine = NULL;
	size_t size;
	// read the command from stdin
	ssize_t linelen = getline(&commandLine, &size, stdin);
	commandLine[linelen-1] = '\0';
	if(strlen(commandLine) == 0)
		return;

	char *argLine = (char *)malloc(strlen(commandLine));
	strcpy(argLine, commandLine);
	nArgs = getArgCount(argLine, " ");

	CommandID command_id = getCommandID(strtok(commandLine, " "));
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
			if(nArgs != 3) {
				displayUsage();
				break;
			}
			// if the server is not yet registered and if the number of arguments is same as expected.
			if(m_bisRegistered) {
				cout << "Register Error: Client has already been registered to the Server. Please do not try again!" << endl;
				break;
			}

			strcpy(arg1, strtok(NULL, " "));
			strcpy(arg2, strtok(NULL, " "));
			command_register(arg1, arg2);
			break;
		case COMMAND_TERMINATE:
			if(!m_bisRegistered) {
				cout << "PUT Error: Please register to the server first!" << endl;
				break;
			}
			strcpy(arg1, strtok(NULL, " "));
			command_terminate(strtol(arg1, NULL, 10));
			break;
		case COMMAND_QUIT:
			command_quit();
			break;
		case COMMAND_CONNECT:
			if(nArgs != 3) {
				displayUsage();
				break;
			}
			if(!m_bisRegistered) {
				cout << "Connect Error: Please register to the server first!" << endl;
				break;
			}
			else if(m_bisRegistered && m_nConnCount >= 3) {
				cout << "Connect Error: Maximum connections reached!" << endl;
				break;
			}
			strcpy(arg1, strtok(NULL, " "));
			strcpy(arg2, strtok(NULL, " "));
			command_connect(arg1, arg2);
			break;
		case COMMAND_PUT:
			if(nArgs != 3) {
				displayUsage();
				break;
			}
			if(!m_bisRegistered) {
				cout << "PUT Error: Please register to the server first!" << endl;
				break;
			}
			connId = strtol(strtok(NULL, " "), NULL, 10);
			strcpy(arg2, strtok(NULL, " "));
			command_put(connId, arg2);
			break;
		case COMMAND_GET:
			if(nArgs != 3) {
				displayUsage();
				break;
			}
			if(!m_bisRegistered) {
				cout << "PUT Error: Please register to the server first!" << endl;
				break;
			}
			connId = strtol(strtok(NULL, " "), NULL, 10);
			strcpy(arg2, strtok(NULL, " "));
			command_get(connId, arg2);
			break;
		case COMMAND_SYNC:
			if(!m_bisRegistered) {
				cout << "SYNC Error: Please register to the server first!" << endl;
				break;
			}
			command_sync();
			break;
		default:
			displayUsage();
			break;
	}
	memset(arg1, 0, sizeof(arg1));
	memset(arg2, 0, sizeof(arg2));
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
    printf("\tTERMINATE <ID> - Terminates the connection with peer associated with the ID \n");
    printf("\tQUIT - Terminates all connections, unregisters with the server and exits the program \n");
    printf("\tCONNECT <IP/Hostname> <Port> - Connects to a client already registered with the server. *Note: Maximum connections: 3\n");
    printf("\tPUT <CONNECTION ID> <FILE NAME> - Uploads the file to the specified peer\n");
    printf("\tGET <CONNECTION ID> <FILE NAME> - Downloads the file to the specified peer\n");
    printf("\tSYNC - Synchronizes all registered peers with the files on connected peer list\n");
    printf("\n");
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
    printf("Machine's IP Address is: %s and Listening port of client is: %d \n", m_ipAddress, m_nListenPort);
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
		return 0;
	}

	if( connect(m_nServerSd, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
	{
		perror("connect");
		return 0;
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
		return 0;
	}
	else {
		m_bisRegistered = true;
		cout << "REGISTER message Sent. Waiting for Updated Client List from server" << endl;

		// update server details with the first unused nodeList
		//for(int i=0; i<10; i++) {
			//if(m_nodeList[i].state == INACTIVE) {
				m_nodeList[0].id = 1;
				m_nodeList[0].sockFd = m_nServerSd;
				m_nodeList[0].state = PROGRESS;
				m_nodeList[0].isServer = true;
				m_nodeList[0].listenPort = srvPort;
				strcpy(m_nodeList[0].ip_addr, m_srvIpAddress);
				getHostName(m_nodeList[0].ip_addr, m_nodeList[0].hostName);

			//}
		//}

		// adding this socket to masterset so that client can update nodeList when server sends messages on new client registration or termination/exit.
		FD_SET(m_nServerSd, &m_masterSet);
		if(m_nServerSd > m_nMaxFd)
			m_nMaxFd = m_nServerSd;
	}

	return 1;
}

/*
 * Function:    Command_Connect()
 * Parameters:  IP address/Hostname of the peer and Listening port number
 * Returns:
 * Description: Connects to a clients registered with server. Maximum connections are 3
 */
void Client::command_connect(char *ipOrHost, char *port) {
	struct sockaddr_in peerAddr;
		char peerHostName[32], peerIpAddr[32];
		int peerPort = atoi(port);
		int peerSd;

		// validation
		// check if it is a hostname or ipAddress
		bool isIP = false;
		if(getHostName(ipOrHost, peerHostName, false)) {
			strcpy(peerIpAddr, ipOrHost);
		}
		else if(getIPaddress(ipOrHost, peerIpAddr, false)) {
			strcpy(peerHostName, ipOrHost);
		}
		else {
			cout << "Connect Error: Enter a valid IP address or host name" << endl;
			return;
		}

		// reject self connections and connections to already connected clients
		if(strcmp(peerIpAddr, m_ipAddress) == 0 && m_nListenPort == peerPort) {
			cout << "Self connections are prohibited!. Please try to connect to other peers!" << endl;
			return;
		}
		// check if there is an active connection with this peer
		for(int i=0; i<10; i++) {
			if(m_nodeList[i].state == ACTIVE && (strcmp(peerIpAddr, m_nodeList[i].ip_addr) == 0 && m_nodeList[i].listenPort == peerPort)) {
				cout << "Already connected to this peer!. No Duplicate connections!" << endl;
				return;
			}
		}
		// ** UPDATE REQUIRED ** this fails if the ip address is same for all clients and port number is different
		// check if the IP or Host is a registered peer
		if(strstr(m_srvList, ipOrHost) == NULL) {
			cout << "Connect Error: Can only connect to registered peers sent by the server!." << endl;
			return;
		}

		// create a socket and connect to the peer
		memset(&peerAddr, 0, sizeof(peerAddr));
		peerAddr.sin_family = AF_INET;
		peerAddr.sin_port = htons(peerPort);

		if(inet_pton(AF_INET, peerIpAddr, &peerAddr.sin_addr) != 1) {
			perror("[command_register] inet_pton");
			displayUsage();
		}
		if((peerSd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("Connect Error: socket");
			return;
		}
		if( connect(peerSd, (struct sockaddr *)&peerAddr, sizeof(peerAddr)) < 0) {
			perror("Connect Error: connect");
			return;

		}

		// construct CONNECT message
		char msg[64] = {0};
		strcat(msg, "CONNECT ");
		char tmpport[4];
		sprintf(tmpport, "%d", m_nListenPort);
		strcat(msg, tmpport);
		cout << "Sending Message " << msg << " to peer " << peerHostName << endl;

		// send CONNECT message to the peer
		int len = strlen(msg);
		if(sendall(peerSd, msg, &len) == -1) {
			cout << "Connect Error: Message sending failed. Please retry" << endl;
			return;
		}
		else {
			// update peer details with the first unused nodeList
			for(int i=0; i<10; i++) {
				if(m_nodeList[i].state == INACTIVE) {
					m_nodeList[i].sockFd = peerSd;
					m_nodeList[i].state = PROGRESS;
					m_nodeList[i].isServer = true;
					m_nodeList[i].listenPort = peerPort;
					strcpy(m_nodeList[i].ip_addr, peerIpAddr);
					strcpy(m_nodeList[i].hostName, peerHostName);
					//getHostName(m_nodeList[i].ip_addr, m_nodeList[i].hostName);
					m_nodeList[i].id = m_nLatestIndex+1;
					m_nLatestIndex += 1;
					cout << "Adding node " << m_nodeList[i].listenPort << " and socket " << m_nodeList[i].sockFd << endl;
					break;
				}
			}
			FD_SET(peerSd, &m_masterSet);
			if(peerSd > m_nMaxFd)
				m_nMaxFd = peerSd;
		}
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
	printf("%-5s%-35s%-20s%-8s\n", "ID", "HOSTNAME", "IP ADDRESS", "PORT");
	for(int i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE)
			printf("%-5d%-35s%-20s%-8d \n", m_nodeList[i].id, m_nodeList[i].hostName, m_nodeList[i].ip_addr, m_nodeList[i].listenPort);
	}
	cout << endl;
}


/*
 * Function:    Command_terminate(int id)
 * Parameters:  int ID
 * Returns:     None
 * Description: Terminates the connection associated with a particular ID
 */
void Client::command_terminate(int id) {
	if(m_nConnCount == 0) {
		cout << "TERMINATE Error: No connected peers! ignoring this command" << endl;
		return;
	}
	if(id == 1) {
		cout << "TERMINATE Error: Cannot terminate connection with server. Please use EXIT command" << endl;
		return;
	}

	int sockFd = -1, i, size;
	// get socket associated with this id
	for(i=0; i<10; i++) {
		if(m_nodeList[i].id == id && m_nodeList[i].state != ACTIVE) {
			cout << "TERMINATE Error: Please enter a valid connection ID. Type LIST to see active connections" << endl;
			return;
		}
		else if(m_nodeList[i].id == id) {
			cout << "Found Socket with connection ID " << m_nodeList[i].id << endl;
			sockFd = m_nodeList[i].sockFd;
			break;
		}
	}
	if(sockFd == -1) {
		cout << "TERMINATE Error: Please enter a valid connection ID. Type LIST to see active connections" << endl;
		return;
	}
	cout << "TERMINATING connection with peer " << m_nodeList[i].hostName << " " << m_nodeList[i].listenPort << " " << m_nodeList[i].sockFd << endl;
	// send Terminate message to the peer
	char buf[1024] = {0};
	sprintf(buf, "TERMINATE");
	size = sizeof(buf);
	if(sendall(sockFd, buf, &size) != 0) {
		cout << "Total Bytes Sent " << size;
		cout << "TERMINATE Error: send failed" << endl;
		return;
	}
	cout << "removing entry " << m_nodeList[i].id << " " << m_nodeList[i].sockFd << " " << m_nodeList[i].listenPort << " " << m_nodeList[i].state << endl;
	handle_terminate(i);
}

/*
 * Function:    Command_terminate(int id)
 * Parameters:  int ID
 * Returns:     None
 * Description: Terminates the connection associated with a particular ID
 */
void Client::command_quit() {
	// send terminate message to all existing connections
	cout << "Processing Quit Command " << endl;
	char buff[1024] = {0};
	int size = sizeof(buff);
	sprintf(buff, "TERMINATE");
	for(int i=1; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE) {
			cout << "Sending message to client on socket " << m_nodeList[i].sockFd << endl;
			m_nodeList[i].state = INACTIVE;
			if(sendall(m_nodeList[i].sockFd, buff, &size) != 0) {
				cout << "Total Bytes Sent " << size;
				cout << "QUIT Error: send failed" << endl;
				return;
			}
			close(m_nodeList[i].sockFd);
			FD_CLR(m_nodeList[i].sockFd, &m_masterSet);
		}
	}
	// send message to server to unregister
	if(m_bisRegistered) {
		memset(buff, 0, sizeof buff);
		sprintf(buff, "QUIT");
		size = sizeof(buff);
		if(sendall(m_nodeList[0].sockFd, buff, &size) != 0) {
			cout << "Total Bytes Sent " << size;
			cout << "QUIT Error: send failed" << endl;
			return;
		}
		m_bisRegistered = false;
		m_nodeList[0].state = INACTIVE;
		close(m_nodeList[0].sockFd);
		FD_CLR(m_nodeList[0].sockFd, &m_masterSet);
	}
	exit(EXIT_SUCCESS);
}

/*
 * Function:    Command_put()
 * Parameters:  id - connection ID, filename - file to upload
 * Returns:     None
 * Description: This function is used to upload a file to a particular connection
 */
void Client::command_put(int id, char *filename) {
	cout << "Processing PUT Request to connection " << id << endl;
	// check if id = 1, then we cannot exchange files with server
	if(id == 1) {
		cout << "Put Error: Cannot Exchange files with the server" << endl;
		return;
	}

	// get the socket descriptor with respect to connection id
	int sockFd = -1;
	for(int i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && m_nodeList[i].id == id) {
			cout << "Found socket descriptor" << endl;
			sockFd = m_nodeList[i].sockFd;
			break;
		}
	}
	if(sockFd == -1) {
		cout << "PUT Error: Please connect to the peer first!" << endl;
		return;
	}
	handle_put(sockFd, filename, true);
}

/*
 * Function:    Command_put()
 * Parameters:  id - connection ID, filename - file to upload
 * Returns:     None
 * Description: This function is used to download a file to a particular connection
 */
void Client::command_get(int id, char *filename) {
	cout << "Processing GET Request from connection " << id << endl;
	// check if id = 1, then we cannot exchange files with server
	if(id == 1) {
		cout << "GET Error: Cannot Exchange files with the server" << endl;
		return;
	}

	// get the socket descriptor with respect to connection id
	int sockFd = -1;
	char buffer[BYTES512] = {0};
	for(int i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && m_nodeList[i].id == id ) {
			cout << "Found socket descriptor" << endl;
			sockFd = m_nodeList[i].sockFd;
			break;
		}
	}
	if(sockFd == -1) {
		cout << "GET Error: Please connect to the peer first!" << endl;
		return;
	}
	// send GET message
	sprintf(buffer, "GET FILE %s", filename);
	cout << "Sending Message: " << buffer << endl;
	int nRead=sizeof(buffer);
	sendall(sockFd, buffer, &nRead);
	memset(buffer, 0, sizeof(buffer));
}

/*
 * Function:    Command_sync()
 * Parameters:  None
 * Returns:     None
 * Description: Propagates sync to all connected peers and uploads its files on to connected peers
 */
void Client::command_sync() {
	cout << "Processing Sync Command " << endl;
	// send sync message to the server so that it propagates the message to all registered clients
	char buffer[64];
	sprintf(buffer, "SYNC");
	int nRead=sizeof(buffer);
	//cout << "sending sync message to " << m_nodeList[i].listenPort << endl;
	sendall(m_nodeList[0].sockFd, buffer, &nRead);
	if(!m_bInSync)
		m_bInSync = true;
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
	cout << "new connection" << endl;
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
	inet_ntop(AF_INET, &remoteaddr.sin_addr, remoteIP, 32);

	// if this client is not registered or max connections have been reached then send fail message and close connection
	char buffer[1024] = {0};
	if(!m_bisRegistered || m_nConnCount >= 3) {
		cout << "connection refused " << endl;
		sprintf(buffer, "CONNECT FAIL");
		if(!m_bisRegistered)
			strcat(buffer, " Not yet registered. Please try later!");
		else
			strcat(buffer, " Maximum Connections Reached!");
		cout << "Sending Message " << buffer << " to client " << remoteIP << endl;

		int len = strlen(buffer);
		if(sendall(newConnSd, buffer, &len) == -1) {
			cout << "Message sending failed. Please retry" << endl;
			return;
		}
		close(newConnSd);
		return;
	}

	// process CONNECT message from client
	memset(buffer, 0 ,sizeof(buffer));
	int nbytes;
	if((nbytes = recv(newConnSd, buffer, sizeof(buffer), 0)) > 0)
	{
		buffer[nbytes] = 0;
		//printf("%s", buffer);
	}
	// CONNECT message format: CONNECT PORT
	if(strstr(buffer, "CONNECT") != NULL) {
		strtok(buffer," ");
		char *port=strtok(NULL," ");
		// find the first unused node and update the peer details
		int id;
		for(int i=0; i<10; i++) {
			if(m_nodeList[i].state == INACTIVE) {
				m_nodeList[i].id = m_nLatestIndex+1;
				m_nodeList[i].state=ACTIVE;
				m_nodeList[i].listenPort =strtol(port, NULL, 10);
				m_nodeList[i].sockFd = newConnSd;
				strcpy(m_nodeList[i].ip_addr,remoteIP);
				getHostName(m_nodeList[i].ip_addr, m_nodeList[i].hostName);
				m_nLatestIndex += 1;
				cout << "Added node " << m_nodeList[i].listenPort << " with socket " << m_nodeList[i].sockFd << endl << endl;
				break;
			}
			id = m_nodeList[i].id;
			m_nConnCount++;
		}
	}
	printf("Connection successful with peer %s\n", m_nodeList[m_nLatestIndex].ip_addr, m_nodeList[m_nLatestIndex].listenPort);

	// send CONNECT OK message indicating successful connection between peers
	memset(buffer, 0, sizeof(buffer));
	strcat(buffer, "CONNECT OK");
	cout << "Sending Message " << buffer << " to peer " << remoteIP << endl << endl;

	int len = strlen(buffer);
	if(sendall(newConnSd, buffer, &len) == -1) {
		cout << "Message sending failed. Please retry" << endl;
		return;
	}


	// add the client to masterSet such that server can read messages from clients, if any.
	FD_SET(newConnSd, &m_masterSet);
	if (newConnSd > m_nMaxFd) {
		m_nMaxFd = newConnSd;
	}
}

// ****UPDATE REQUIRED****
// Change this function
void Client::displayUpdateList() {
	int len = strlen(m_srvList);
	for(int i=7;i<len;i++) {
		if(m_srvList[i]==' ')
			printf("\t\t");
		else if(m_srvList[i]=='|')
			printf("\n");
		else
			printf("%c", m_srvList[i]);
	}
}

void Client::handle_put(int sockFd, char *filename, bool sendMsg) {
	char buffer[BYTES512]={0};
	struct timeval  begin, end;
	size_t bytesRead = 0,bytesSent=0;
	size_t fileSz, remBytes, remActBytes;

	// check if the file exists, if so get file size
	struct stat statbuf;
	if(stat(filename, &statbuf) == -1) {
		perror("PUT Error: stat");

		return;
	}
	fileSz = statbuf.st_size;
	remBytes = remActBytes = fileSz;

	// open the file for reading
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) {
		cout << "PUT Error: Failed to open file!" << endl;
		return;
	}



	// send message to the peer informing of uploading file along with its size
	// message format PUT <FILENAME> <SIZE>
	sprintf(buffer, "PUT %s %lu", filename, fileSz);
	cout << "sending msg " << buffer << endl;
	int nRead=sizeof(buffer);
	sendall(sockFd, buffer, &nRead);
	memset(buffer, 0, sizeof(buffer));

	// record the time when upload is starting
	gettimeofday(&begin, NULL);
	int i;
	for(i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && m_nodeList[i].sockFd == sockFd) {
			break;
		}
	}
	//cout << "Uploading file " << filename  << " to client!" << m_nodeList[i].hostName << endl;
	while( remActBytes > 0 && remBytes > 0) {
		int len;
		if (remBytes >= BYTES512 )
		{
			bytesRead = fread(buffer, 1, BYTES512 , fp);
			len=bytesRead;
		}
		else if (remBytes<BYTES512)
		{
			bytesRead = fread(buffer, 1, remBytes , fp);
			len=bytesRead;
		}
		//cout << "sending bytes: " << bytesRead << endl;

		if( sendall(sockFd, buffer, &len) < 0)
		{
			cout<<"PUT Error: Failed to upload file!"<<endl;
			remBytes=remActBytes=0;
			break;
		}
		remBytes=remBytes-bytesRead;
		remActBytes = remActBytes - len;
		//cout << "remaning bytes: " << remBytes << endl;
		memset(buffer, '0', sizeof(buffer));
	}

	// either there was an EOF or some error
	if (remBytes == 0 && remActBytes == 0) {
		if (ferror(fp))
			cout << "Put Error: error reading. Failed to Upload !\n" << endl;
		else {
			fclose(fp);
			//calculate the time taken for upload
			gettimeofday(&end, NULL);
			long double uploadTime=1000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000;
			uploadTime /= 1000;
			if(uploadTime <= 0) uploadTime = 1.0;
			long double rate = (long double)(fileSz/1048576.00000)/uploadTime;
			cout << "Upload Successful to client " << m_nodeList[i].hostName << endl;
			cout << fixed << "Tx End: Size=" << fileSz << " Bytes, Time="  << uploadTime << " Seconds, Tx Rate=" << rate << " Bytes/Sec" << endl << endl;
		}
		return;
	}

	else if (remBytes==0 && remActBytes > 0) {
		if (ferror(fp))
		{
			cout << "Put Error: error reading. Failed to Upload !\n" << endl;
			return;
		}
		else
		{
			cout<<"Put Error: Whole File was not sent"<<endl;
			fclose(fp);
		}
		return;
	}
}

void Client::handle_get(int sockFd, char *fileName, size_t fileSz) {
	struct timeval  start, end;
	size_t bytesReceived = 0,bytesWritten=0;
	char buff[BYTES512];
	memset(buff, '0', sizeof(buff));
	FILE *fp = fopen(fileName,"wb");

	size_t bytesLeft = fileSz;
	if(fp==NULL)
	{
		cerr<<"File Open Error";
		return ;
	}
	int i;
	for(i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && m_nodeList[i].sockFd == sockFd) {
			break;
		}
	}
	cout<<"Downloading file " << fileName << " from client " << m_nodeList[i].hostName << endl;
	gettimeofday(&start, NULL);

	/* Receive data in chunks of BUF_SIZE bytes */
	while( bytesLeft > 0)
	{
		bytesReceived = recv(sockFd, buff, BYTES512, 0);
		//cout << "recieved bytes: " << bytesReceived << endl;
		bytesWritten = fwrite(buff,1,bytesReceived, fp);
		//cout << "written bytes: " << bytesWritten << endl;
		if(bytesWritten < bytesReceived)
		{
			cout<<"write failed."<<endl;
		}
		bytesLeft=bytesLeft-bytesWritten;
		//cout << "bytesleft " << bytesLeft << endl;
		memset(buff, '0', sizeof(buff));
	}

	if(bytesReceived < 0 || bytesReceived==0)
	{
		perror("recv");
	}
	else
	{
		fclose(fp);
		gettimeofday(&end, NULL);
		long double downloadTime=1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
		downloadTime /= 1000;
		if(downloadTime <= 0) downloadTime = 1.0;
		long double rate = (long double) (fileSz/1048576.00000)/downloadTime;
		cout << "Download Successful from client " << m_nodeList[i].hostName << endl;
		cout << fixed << "Rx End: Size=" << fileSz << " Bytes, Time="  << downloadTime << " Seconds, Rx Rate=" << rate << " Bytes/Sec" << endl;
	}
}

void Client::handle_sync() {

}

void Client::start_sync() {
	char buffer[BYTES512]={0};
	struct timeval  begin, end;
	size_t bytesRead = 0,bytesSent=0;
	size_t fileSz, remBytes;

	char filename[50], hostName[100];
	getHostName(m_ipAddress, hostName);
	sprintf(filename, "%s.txt", strtok(hostName, "."));

	if(m_nConnCount == 0)
		return;

	cout << "SYNC fileName: " << filename << endl;

	// check if the file exists, if so get file size
	struct stat statbuf;
	if(stat(filename, &statbuf) == -1) {
		perror("PUT Error: stat");

		return;
	}
	fileSz = statbuf.st_size;
	remBytes = fileSz;

	// open the file for reading
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) {
		cout << "PUT Error: Failed to open file!" << endl;
		return;
	}

	// send message to all the connected peers informing of uploading file along with its size
	// message format PUT <FILENAME> <SIZE>
	sprintf(buffer, "PUT %s %lu", filename, fileSz);
	cout << "sending msg " << buffer << endl;
	int nRead=sizeof(buffer);
	for(int i=1; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE)
			sendall(m_nodeList[i].sockFd, buffer, &nRead);
	}
	memset(buffer, 0, sizeof(buffer));

	// record the time when upload is starting
	gettimeofday(&begin, NULL);


	while(remBytes > 0)
	{
		int len;
		if (remBytes >= BYTES512 )
		{
			bytesRead = fread(buffer, 1, BYTES512 , fp);
			len=bytesRead;
		}
		else if (remBytes<BYTES512)
		{
			bytesRead = fread(buffer, 1, BYTES512-remBytes , fp);
			len=bytesRead;
		}
		//cout << "sending bytes: " << bytesRead << endl;
		for(int i=1; i<10; i++) {
			if(m_nodeList[i].state == ACTIVE) {
				//cout << "Uploading file " << filename  << "to client!" << m_nodeList[i].hostName << endl;
				if( sendall(m_nodeList[i].sockFd, buffer, &len) < 0) {
					cout<<"PUT Error: Failed to upload file!"<<endl;
					remBytes=0;
					break;
				}
				//cout << "remaning bytes: " << remBytes << endl;
			}
		}
		remBytes=remBytes-bytesRead;
		memset(buffer, '0', sizeof(buffer));
	}

	// either there was an EOF or some error
	if (remBytes == 0)
	{
		if (ferror(fp))
			cout << "Put Error: error reading. Failed to Upload !\n" << endl;
		else {
			fclose(fp);
			//calculate the time taken for upload
			gettimeofday(&end, NULL);
			double uploadTime=1000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000;
			uploadTime /= 1000;
			cout << "SYNC Completed" << endl;
			//cout << "Tx End: Size=" << fileSz << " Bytes Time="  << uploadTime << " Seconds Tx Rate=" << fileSz/uploadTime << " Bytes/Sec" << endl;
			return;
		}

	}
	return;
}

void Client::handle_terminate(int Ix) {
	// mark the node being terminated as unused and close socket
	cout << "closing socket " << m_nodeList[Ix].sockFd << " and marking index " << Ix << " as Inactive" << endl;
	m_nodeList[Ix].state = INACTIVE;
	close(m_nodeList[Ix].sockFd);
	// remove this socket from master set and update Max file descriptor.
	FD_CLR(m_nodeList[Ix].sockFd, &m_masterSet);
	if (m_nodeList[Ix].sockFd==m_nMaxFd)
	{
		m_nMaxFd--;
	}
	m_nodeList[Ix].sockFd = -1;
	m_nConnCount--;

	if(Ix == 0) {
		cout << "Server Disconnected: Terminating all connections" << endl;
		for(int i=0; i<10; i++) {
			if(m_nodeList[i].state == ACTIVE) {
				m_nodeList[i].state = INACTIVE;
				close(m_nodeList[i].sockFd);
				FD_CLR(m_nodeList[i].sockFd, &m_masterSet);
				if (m_nodeList[i].sockFd==m_nMaxFd)
				{
					m_nMaxFd--;
				}
			}
		}
		m_bisRegistered = false;
	}

	// reorder the nodes as per id
	reorderNodeList(Ix);

	cout << "Connection TERMINATED Successfully" << endl;
}

void Client::reorderNodeList(int Ix) {
	for(int i=Ix+1; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE) {
			m_nodeList[i-1] = m_nodeList[i];
			m_nodeList[i].state = INACTIVE;
		}
	}
	for(int i=0; i<10; i++) {
		m_nodeList[i].id = i+1;
	}
	m_nLatestIndex--;
}

/*****************************************************************************
 *                      Client utility functions                             *
 * **************************************************************************/

int Client::getArgCount(char *line, const char *delim) {
	char *tok = strtok(line, delim);
	int nArgs = 0;
	while(tok!=NULL) {
		nArgs++;
		tok = strtok(NULL, delim);
	}
	return nArgs;
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
    else if(strcasecmp(comnd, "TERMINATE") == 0)
    	return COMMAND_TERMINATE;
    else if(strcasecmp(comnd, "QUIT") == 0)
    	return COMMAND_QUIT;
    else if(strcasecmp(comnd, "CONNECT") == 0)
    	return COMMAND_CONNECT;
    else if(strcasecmp(comnd, "PUT") == 0)
    	return COMMAND_PUT;
    else if(strcasecmp(comnd, "GET") == 0)
    	return COMMAND_GET;
    else if(strcasecmp(comnd, "SYNC") == 0)
    	return COMMAND_SYNC;
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
