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
#include "base.h"

using namespace std;

#define MAX_CREATOR_LEN 100

class Client:public Base {
private:
    // creator constants
    char* m_name;
    char* m_ubitName;
    char* m_ubEmail;

    // client utilites
    int m_nListenPort;		// listening port of this client.
    int m_nConnCount;		// active connections count
    char m_ipAddress[32];	// host ipaddress in numbers and dot format.
    char m_srvIpAddress[32]; // this would no longer be required becz nodeList[0] contains server details.
    char m_srvList[1024];	// this holds the updates sent by server when ever there is a new client has registered/terminated/exited.

    // socket specific members
	int m_nListenSd;        // listen socket descriptor.
	int m_nServerSd;		// socket used for connecting to the server, this is used when server sends client list updates.
	int m_nMaxFd;           // Maximum file descriptor number.
	fd_set m_masterSet;    // list which holds all the active connections including listening socket and stdin.
	fd_set m_readSet;      // list which is a copy of m_nMasterSet and is passed to select() call, since select() call changes the list we don't intend to change m_nMasterSet.
	struct sockaddr_in m_cliListenAddr; // this holds its address, port and family and this is bind() to listening socket.

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
	void command_connect(char *ipAddr, char *port);
	void command_list();
	void command_terminate(int connectionId);
	void command_quit();

	void startListenClient();
	void newConnectionHandler();
	void displayServerList();

	// utility functions
    char** parseLine(char *line, int &nArgs, const char *delim);
    CommandID getCommandID(char *comnd);
    void updateIpAddress();
    void displayUsage();
    bool checkValipIPPort();
};

#endif /* !CLIENT_H */
