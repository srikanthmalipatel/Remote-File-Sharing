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
#include "common.h"

using namespace std;


class Server {
private:
    // creator constants
    char* m_name;
    char* m_ubitName;
    char* m_ubEmail;

    // server utilites
    int m_nListenPort;
    char m_ipAddress[INET_ADDRSTRLEN];

public:
    // constructor and destructor
    Server(int port);
    ~Server();

    // Event handler
    void commandShell();

    // These are possible commands supported by server
    void command_help();
    void command_creator();
    void command_display();
    int command_register();
    void command_list();
    void command_terminate(int connectionId);
    void command_quit();

    // Utility functions
    int getListenPort();

private:
    // Utility functions
    CommandID getCommandID(char comnd[]);
    void updateIpAddress();
};

#endif /* !SERVER_H */
