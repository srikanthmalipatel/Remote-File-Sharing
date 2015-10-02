/*
 * server.h
 * Copyright (C) 2015 SrikanthMalipatel <smalipat@buffalo.edu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
// if common functionality is moved to different file then move these headers also
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include "common.h"

using namespace std;

#define MAX_CREATOR_LEN 100

typedef struct {
	int id;
	int sockFd;
	char hostName[1024];
	char ipAddress[INET_ADDRSTRLEN];
	int port;
}clientList;

class Client {
private:
    // creator constants
    char* m_name;
    char* m_ubitName;
    char* m_ubEmail;

    // client utilites
    int m_nListenPort;
    char m_ipAddress[INET_ADDRSTRLEN];
    char m_srvIpAddress[INET_ADDRSTRLEN];

    // socket specific members
	int m_nListenSd;        // listen socket descriptor
	int m_nMaxFd;           // Maximum file descriptor number
	fd_set m_masterSet;    // list which holds all the active connections including listening socket and stdin
	fd_set m_readSet;      // list which is a copy of m_nMasterSet and is passed to select() call, since select() call changes the list we don't intend to change m_nMasterSet
	struct sockaddr_in m_cliListenAddr; // this holds its address, port and family and this is bind() to listening socket

	clientList m_cList[10];	// maximum of 10 clients
        int m_nClientCount;

    // register
    bool m_bisRegistered;

public:
    // constructor and destructor
    Client(int port);
    ~Client();

    // Utility functions
    int getListenPort();

private:
    void eventHandler();
    void commandShell();

    // These are possible commands supported by server
	void command_help();
	void command_creator();
	void command_display();
	int command_register(char *ipAddr, char *port);
	void command_list();
	void command_terminate(int connectionId);
	void command_quit();

	void startListenClient();
	void newConnectionHandler();

	// utility functions
    char** getAndParseCommandLine(int &nArgs);
    CommandID getCommandID(char *comnd);
    void updateIpAddress();
    void displayUsage();
    bool checkValipIPPort();
};

#endif /* !CLIENT_H */
