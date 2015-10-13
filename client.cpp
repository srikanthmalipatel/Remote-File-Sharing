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

    // initalize nodeList and its tracker
    this->m_nLatestIndex = 0;
    for(int i=0; i<10; i++)
	{
    	m_nodeList[i].state=INACTIVE;
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
                	if(m_bisRegistered && m_nConnCount < 3)
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
                			printf("Got Updated Server List \n");
                			strcpy(m_srvList,pos);
							displayServerList();
							cout << endl;
                		}
                	}
                	memset(recvBuff, 0, 1024);
                }
                else {
                    // handle data from connected clients
                	if( (bytesRead = recv(i, recvBuff, sizeof(recvBuff), 0)) > 0) {
                		recvBuff[strlen(recvBuff)]='\0';

                		// If it is a CONNECT OK message from the peer the update clientList and also increase connection count
                		if(strstr(recvBuff, "CONNECT OK") != NULL) {
                			for(int j=0; j<10; j++) {
                				if(m_nodeList[j].sockFd == i) {
                					cout << "Connection Successful with peer " << m_nodeList[j].ip_addr << " " << m_nodeList[j].listenPort << endl;
                					m_nodeList[j].state = ACTIVE;
                				}
                			}
                			m_nConnCount++;
                		}
                		// If a peer refuses connection
                		if(strstr(recvBuff, "CONNECT FAIL") != NULL) {
                			strtok(recvBuff, " ");
                			for(int j=0; j<10; j++) {
								if(m_nodeList[j].sockFd == i) {
									cout << "Connection Failed with peer " << m_nodeList[j].ip_addr << " " << m_nodeList[j].listenPort << endl;
									cout << "Reason: " << recvBuff+12 << endl;
									cout << endl;
									m_nodeList[j].state = INACTIVE;
								}
							}
                		}
                		// Process PUT request
                		// FORMAT: PUT <FILENAME> <FILESIZE>
                		if(strstr(recvBuff, "PUT")) {
                			cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			strtok(recvBuff, " ");
                			char *fileName = (char *) malloc(32);
							strcpy(fileName, strtok(NULL, " "));
							size_t fileSz = strtol(strtok(NULL, " "), NULL, 10);
                			handle_get(i, fileName, fileSz);
                		}
                		// Process GET request.
                		// FORMAT: GET FILE <FILENAME>
                		if(strstr(recvBuff, "GET FILE")) {
                			cout << "Message: " << recvBuff << " from client " << m_nodeList[i].hostName << endl;
                			strtok(recvBuff, " ");
                			strtok(NULL, " ");
							char *fileName = (char *) malloc(32);
							strcpy(fileName, strtok(NULL, " "));
							handle_put(i, fileName);
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
	int nArgs;
	char arg1[32], arg2[32], arg3[32];
	char *commandLine = NULL;
	size_t size;
	// read the command from stdin
	ssize_t linelen = getline(&commandLine, &size, stdin);
	commandLine[linelen-1] = '\0';
	if(strlen(commandLine) == 0)
		return;
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
			// if the server is not yet registered and if the number of arguments is same as expected.
			if(m_bisRegistered) {
				cout << "Register Error: Client has already been registered to the Server. Please do not try again!" << endl;
				break;
			}

			strcpy(arg1, strtok(NULL, " "));
			strcpy(arg2, strtok(NULL, " "));
			command_register(arg1, arg2);
			break;
		case COMMAND_CONNECT:
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
			if(!m_bisRegistered) {
				cout << "PUT Error: Please register to the server first!" << endl;
				break;
			}
			strcpy(arg1, strtok(NULL, " "));
			strcpy(arg2, strtok(NULL, " "));
			strcpy(arg3, strtok(NULL, " "));
			command_put(arg1,arg2, arg3);
			break;
		case COMMAND_GET:
			if(!m_bisRegistered) {
				cout << "PUT Error: Please register to the server first!" << endl;
				break;
			}
			strcpy(arg1, strtok(NULL, " "));
			strcpy(arg2, strtok(NULL, " "));
			strcpy(arg3, strtok(NULL, " "));
			command_get(arg1,arg2, arg3);
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
    printf("\tCONNECT <IP/Hostname> <Port> - Connects to a client already registered with the server. *Note: Maximum connections: 3\n");
    printf("\tPUT <CONNECTION ID> <FILE NAME> - Uploads the file to the specified peer\n");
    printf("\tGET <CONNECTION ID> <FILE NAME> - Downloads the file to the specified peer\n");
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
			if(m_nodeList[i].state == INACTIVE) {
				m_nodeList[i].sockFd = m_nServerSd;
				m_nodeList[i].state = PROGRESS;
				m_nodeList[i].isServer = true;
				m_nodeList[i].listenPort = srvPort;
				strcpy(m_nodeList[i].ip_addr, m_srvIpAddress);
				getHostName(m_nodeList[i].ip_addr, m_nodeList[i].hostName);
				m_nLatestIndex = i;
				break;
			}
		}

		// adding this socket to masterset so that client can update nodeList when server sends messages on new client registration or termination/exit.
		FD_SET(m_nServerSd, &m_masterSet);
		if(m_nServerSd > m_nMaxFd)
			m_nMaxFd = m_nServerSd;
	}

	return 0;
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


		// create socket and initate connection
		memset(&peerAddr, 0, sizeof(peerAddr));
		peerAddr.sin_family = AF_INET;
		peerAddr.sin_port = htons(peerPort);

		if(inet_pton(AF_INET, peerIpAddr, &peerAddr.sin_addr) != 1) {
			perror("[command_register] inet_pton");
			displayUsage();
		}
		if((peerSd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("Connect Error: socket");
			return;
		}
		if( connect(peerSd, (struct sockaddr *)&peerAddr, sizeof(peerAddr)) < 0)
		{
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
					m_nLatestIndex = i;
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
}

/*
 * Function:    Command_put()
 * Parameters:  id - connection ID, filename - file to upload
 * Returns:     None
 * Description: This function is used to upload a file to a particular connection
 */
void Client::command_put(char *id, char *port, char *filename) {
	cout << "Processing PUT Request" << id << " " << port << endl;
	// get the socket descriptor with respect to connection id or ip
	int sockFd = -1;
	for(int i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && (strcmp(m_nodeList[i].ip_addr, id) == 0) && m_nodeList[i].listenPort == atoi(port) ) {
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
void Client::command_get(char *id, char *port, char *filename) {
	cout << "Processing GET Request" << id << " " << port << endl;
	// get the socket descriptor with respect to connection id or ip
	int sockFd = -1;
	char buffer[BYTES512] = {0};
	for(int i=0; i<10; i++) {
		if(m_nodeList[i].state == ACTIVE && (strcmp(m_nodeList[i].ip_addr, id) == 0) && m_nodeList[i].listenPort == atoi(port) ) {
			cout << "Found socket descriptor" << endl;
			sockFd = m_nodeList[i].sockFd;
			break;
		}
	}
	if(sockFd == -1) {
		cout << "PUT Error: Please connect to the peer first!" << endl;
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
	char buffer[1024];
	if(!m_bisRegistered || m_nConnCount >= 3) {
		cout << "connection refused " << endl;
		strcat(buffer, "CONNECT FAIL");
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
			m_nConnCount++;
		}
	}
	printf("Connection successful with peer %s\n", m_nodeList[m_nLatestIndex].ip_addr, m_nodeList[m_nLatestIndex].listenPort);

	// send CONNECT OK message indicating successful connection between peers
	memset(buffer, 0, sizeof(buffer));
	strcat(buffer, "CONNECT OK");
	cout << "Sending Message " << buffer << " to peer " << remoteIP << endl;

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

void Client::handle_put(int sockFd, char *filename, bool sendMsg) {
	char buffer[BYTES512]={0};
	struct timeval  begin, end;
	size_t bytesRead = 0,bytesSent=0;
	size_t fileSz, remBytes;

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



	// send message to the peer informing of uploading file along with its size
	// message format PUT <FILENAME> <SIZE>
	sprintf(buffer, "PUT %s %lu", filename, fileSz);
	cout << "sending msg " << buffer << endl;
	int nRead=sizeof(buffer);
	sendall(sockFd, buffer, &nRead);
	memset(buffer, 0, sizeof(buffer));

	// record the time when upload is starting
	gettimeofday(&begin, NULL);
	cout << "Upload Starting...!" << endl;
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
		int res=sendall(sockFd, buffer, &len);
		if( res < 0)
		{
			cout<<"PUT Error: Failed to upload file!"<<endl;
			remBytes=0;
			break;
		}
		remBytes=remBytes-bytesRead;
		memset(buffer, '0', sizeof(buffer));
	}

	// either there was an EOF or some error
	if (remBytes == 0)
	{
		if (feof(fp)) {
			fclose(fp);
			//calculate the time taken for upload
			gettimeofday(&end, NULL);
			double uploadTime=1000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000;
			cout << "Transfer Successful " << uploadTime << " milliseconds" << endl;
			return;
		}
		if (ferror(fp))
			cout << "Put Error: error reading. Failed to Upload !\n" << endl;
	}
	return;
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

	cout<<"Starting to Download file  " << fileName << endl;
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
		cout<<"Download Complete"<<endl;
		fclose(fp);
		gettimeofday(&end, NULL);
		double timeTaken=1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
		cout<<"Downloaded FILE:" << fileName << "in " << timeTaken/1000 << " ms"  <<endl;
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
		tok = strtok(NULL, delim);
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
    else if(strcasecmp(comnd, "CONNECT") == 0)
    	return COMMAND_CONNECT;
    else if(strcasecmp(comnd, "PUT") == 0)
    	return COMMAND_PUT;
    else if(strcasecmp(comnd, "GET") == 0)
    	return COMMAND_GET;
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
