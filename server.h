/*
 * server.h
 * Copyright (C) 2015 SrikanthMalipatel <smalipat@buffalo.edu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
// if you want to move common functionality to a parent class then move these headers also
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
// above headers required for updateIpAddress() function
#include <sys/socket.h>
#include <unistd.h>
#include "common.h"

using namespace std;

typedef struct {
	int id;
	int sockFd;
	char hostName[1024];
	char ipAddress[INET_ADDRSTRLEN];
	int port;
}clientList;


class Server {
private:
    // creator constants
    char* m_name;
    char* m_ubitName;
    char* m_ubEmail;

    // server details
    int m_nListenPort;
    char m_ipAddress[INET_ADDRSTRLEN];

    // socket specific members
    int m_nListenSd;        // listen socket descriptor
    int m_nMaxFd;           // Maximum file descriptor number
    fd_set m_masterSet;    // list which holds all the active connections including listening socket and stdin
    fd_set m_readSet;      // list which is a copy of m_nMasterSet and is passed to select() call, since select() call changes the list we don't intend to change m_nMasterSet
    struct sockaddr_in m_srvAddr; // this holds the server address, port and family and this is bind() to listening socket

    clientList m_cList[10];	// maximum of 10 clients
    int m_nClientCount;

public:
    // constructor and destructor
    Server(int port);
    ~Server();

    // Utility functions
    int getListenPort();
private:
    // Event handlers for incoming connections and command handlers
    void eventHandler();
    void commandShell(char *command);

    // These are possible commands supported by server
    void command_help();
    void command_creator();
    void command_display();
    void command_list();

    void startListenServer();
    void newConnectionHandler();
    // Utility functions
    CommandID getCommandID(char comnd[]);
    void updateIpAddress();
    void addClienttoList(int sockfd, char *ipAddr, int port);
    void updateClients();
};

#endif /* !SERVER_H */
